# TunnelViewerSDK 第三、第四阶段优化说明

本次更新以第一、二阶段版本为基线继续优化，目标是：

1. 第三阶段完成 Annotation 外部扩展闭环；
2. 第四阶段优先处理低风险、高收益的真实性能瓶颈；
3. 不破坏现有历史数据、旧枚举、旧 Manager、旧 Storage 接口和 Scene 坐标语义。

> 兼容底线：`AnnotationData/DefectData` 仍保持第一、二阶段确认过的旧 8 字段布局；新增身份、行为和扩展元数据仍放在已有 `QVariantMap attributes` 中。

---

## 一、第三阶段：开放式 Annotation 扩展闭环

### 1. Annotation Identity Resolver

`DefectManager` 新增统一身份解析：

- 优先读取 `attributes[typeKey/layerKey/legacyElementType/formatVersion]`；
- 新字段不存在时继续按 `Manager + typeCode + legacy ElementType` 恢复；
- TunnelViewer 历史 7/8 映射仍使用原有 `ElementTypePersistenceProfile`；
- 解析过程不修改 Storage 返回的原始数据。

这使新旧数据在进入 Item 创建流程前统一成 `AnnotationIdentity`，避免 Factory、Layer、Tool 各自重复猜旧格式。

### 2. Annotation Item Factory

`DefectManager` 增加按 `typeKey` 注册外部 Item Factory 的能力：

```cpp
manager->registerItemFactory("inspection.clearance",
    [](const DefectData& data) -> DefectShapeItem* {
        return new ClearanceItem(data.shape, data.name, data.type, data.uuid);
    });
```

也可通过 View 注册：

```cpp
view->registerAnnotationItemFactory("inspection", "inspection.clearance", creator);
```

加载流程：

```text
旧/新 DefectData
    -> Identity Resolver
    -> typeKey Factory
       -> 成功：创建外部派生 Item
       -> 无 Factory/创建失败：回退 DefectShapeItem
    -> Manager 统一 fromData / 旧枚举 / layerKey / typeKey / style / interaction
```

因此 Factory 不存在不会造成旧工程打不开。

### 3. Layer Runtime State

新增 `AnnotationLayerState`：

- `visible`
- `locked`
- `selectable`
- `exportable`
- 可选 `zValue`

Layer 状态是运行时 UI 状态，不写入历史 Annotation 数据。

View 提供：

```cpp
setAnnotationLayerVisible(...);
setAnnotationLayerLocked(...);
setAnnotationLayerSelectable(...);
setAnnotationLayerExportable(...);
setAnnotationLayerState(...);
```

特别处理：

- Layer 锁定和 Item 自身 locked 分开，解除 Layer 锁不会丢失 Item 原锁定状态；
- Layer 临时 Z 覆盖会记录每个 Item 原始 Z，解除后恢复；
- Layer 临时隐藏会记录每个 Item 原可见状态，重新显示 Layer 时不会把业务本来隐藏的 Item 强制显示。

### 4. Interaction Policy 收口

`TiledGraphicsView` 不再直接根据新业务 `ElementType` 决定点击/拖动。

新增通用属性：

- `interactiveInBrowseMode`
- `interactiveInMoveMode`

没有设置时仍采用旧兼容规则：

- Move 模式默认仍只处理旧 `Type_Sleeper`；
- Browse 模式旧拱脚定位线仍按原 `m_tunnelLocationMoveEnabled` 行为；
- CP3、Chain、Sleeper 等原移动轴策略仍由 `DefectManager` 的 legacy fallback 保留。

Layer `selectable=false` 会同步影响最终 `ItemIsSelectable`，View 的自定义拖动引擎也不会绕过该状态。

### 5. Draw Context

保留旧信号：

```cpp
sigGeometryDrawn(DrawShape, QPainterPath)
```

新增：

```cpp
AnnotationDrawContext
sigAnnotationGeometryDrawn(context, shape, path)
```

外部自定义 `AbstractTool` 可在绘图前设置：

```cpp
AnnotationDrawContext ctx;
ctx.toolKey = "clearanceTool";
ctx.layerKey = "inspection";
ctx.typeKey = "inspection.clearance";
ctx.attributes["businessId"] = 1001;
view->setAnnotationDrawContext(ctx);
```

旧 `startDrawingDefect()` 会清掉自定义 Context，并继续走原 `DefectDrawTool`，避免旧绘图结果携带上一次新业务 Context。

### 6. 通用 Highlight

新增：

```cpp
setHighLightAnnotation(layerKey, uuid, margin)
```

旧 `setHighLightElement(uuid, ElementType)` 保留原历史语义：原来只有 Disease 真正执行高亮的分支仍保持如此，不借此次升级偷偷改变旧接口行为。

### 7. Export Filter 闭环

此前 `attributes[exportable]` 只是属性，`scene->render()` 仍会把 Scene 中 Item 全部画出。

本次增加导出 RAII 过滤：

- Layer `exportable=false` -> 临时隐藏该 Layer Annotation；
- Item `exportable=false` -> 临时隐藏该 Item；
- render 完毕后恢复原可见状态；
- 不修改业务数据，不破坏旧 `drawDefects=false` 路径。

DB 导出和 VirtualSequence 导出均接入。

### 8. Label LOD 与 paint 微优化

新增：

```text
attributes["labelMinLod"]
```

未设置时仍使用旧硬编码 `0.01`，保证老工程显示效果不变。

同时一次 `paint()` 内只计算一次复杂 Path 的 `boundingRect()`，减少重复几何计算。

---

## 二、第四阶段：低风险性能优化

### 1. DB/整图模式可视 Item 调度：O(N) -> O(logN + K)

旧 `updateVisibleTiles()` 每次滚动都会遍历全部 `m_items`。

本次利用 `addLayer()` 已保证的主轴有序布局，通过二分定位当前 `scheduleRect` 的首 Item，再只遍历附近 K 个候选 Item。

适用于：

- Vertical
- Horizontal
- VerticalReverse
- HorizontalReverse
- 每张图片尺寸不同

随机尺寸/方向模拟已与旧全量 `intersects()` 扫描对照，结果一致。

同一索引也用于：

- `visibleImageFileNames()`
- `currentBottomAnchor()`
- `GlobalSceneToMap()`
- `sceneToImageCoordinate()`
- DB 区域导出

> 该优化的前提仍是数据库图片由 SDK 的 `addLayer()` 按既有规则拼接；这也是现有 SDK 的标准使用方式。

### 2. addLayer SceneRect：累计 O(N²) -> O(N)

旧逻辑每增加一张图片都重新扫描所有已加载 Item 求总 SceneRect。

现在维护：

```cpp
m_databaseImageSceneRect
```

每次只：

```cpp
sceneRect = sceneRect.united(newItemRect);
```

单次 O(1)，加载 N 张整体 O(N)。

### 3. HUD 切片统计增量化

旧 HUD 刷新会扫描全部 `TunnelSectionItem` 累加已加载切片数。

现在每个 Item `sigTilesUpdated` 时只更新自己的 old/new count 差值，总数 O(1) 更新。

### 4. DB 调度只清理上一批离开范围的 Item

旧逻辑为了卸载不可见资源同样需要扫描全部图片。

现在保存：

```cpp
m_databaseScheduledItems
```

只对“上一批在调度区、本批已经离开”的 Item 执行 `releaseThumbnail()/unloadAll()`。

### 5. 横/纵整图预取窗口修正

旧整图预取窗口固定主要向 Y 轴扩展，对 Horizontal 工程并不合理。

现在依据 DB 实际布局主轴：

- Vertical -> Y 方向多屏预取；
- Horizontal -> X 方向多屏预取。

不改变图片坐标和排列，仅减少错误方向 IO。

### 6. VirtualSequence Preview / Full Cache 分离

旧虚拟序列缩略图和高清图共用一套 Cache，大高清图可能快速逐出大量预览图。

现在拆为：

```text
Preview Cache 约 25%
Full Cache    约 75%
```

总预算仍取原 `SequenceLoadOptions::decodedCacheBytes`，因此不会额外扩大默认总缓存预算。

### 7. 动态 Prefetch

保留原 `prefetchForwardScreens/prefetchBackwardScreens` 作为基础值，在此基础上根据最近一次实际滚动方向调整：

- 向前滚 -> 增加前向预取、压低反向预取；
- 向后滚 -> 反向处理；
- 快速滚动比慢速滚动增加更多前向屏数；
- Reverse layout 仍先沿用原正反方向交换逻辑。

### 8. Decode Priority 按距离细分

同一请求 Range 内：

- 当前视口附近优先级最高；
- 1~1.5 屏附近次之；
- 更远预取保持基础优先级。

这样高速滚动时更容易优先清晰当前用户即将看到的位置。

---

## 三、明确没有改的内容

为避免兼容风险，本次没有做以下改动：

- 不修改 `AnnotationData/DefectData` 8 字段布局；
- 不修改 `ViewMode/DrawShape/ElementType` 数值；
- 不删除/重命名 10 个旧 Manager 成员；
- 不给 `IDefectStorage` 增加纯虚函数；
- 不改变历史 TunnelViewer 7/8 profile；
- 不改变 Scene 坐标语义；
- 不重写已有 `VirtualSequence` Chunk/LOD 主流程；
- 不强制启用 OpenGL；
- 不做 Annotation 大规模虚拟化/批渲染；
- 不改现有图像几何或业务算法。

---

## 四、外部 Storage 的重要约定

SDK 项目中没有发现 `IDefectStorage` 的具体实现，它们仍由各业务软件提供。

因此：

- **旧数据兼容不依赖 attributes 新字段**：旧 Manager/旧 typeCode/旧 profile 仍可恢复；
- 若业务开始使用新的 `typeKey/layerKey/Factory`，外部 Storage 必须完整保存和恢复 `DefectData::attributes`；
- 如果旧 Storage 丢弃 attributes，历史数据仍可工作，但新自定义类型保存后可能只能按 legacy fallback 恢复。

这不是本次修改新增的文件格式强制迁移，而是启用新扩展能力时的持久化契约。

---

## 五、暂缓到后续实测后再做的高风险项

本次没有贸然实施第四阶段 P2：

- Annotation Viewport Virtualization；
- 百万级 Annotation Range Incremental Load；
- 非交互图元 Batch Renderer；
- Adaptive Decode 接口；
- Raster `MinimalViewportUpdate/BoundingRectViewportUpdate` 强制替换；
- 更深入 OpenGL/GPU Renderer。

这些优化可能收益很大，但会影响交互、刷新或外部数据源契约，应在当前版本用真实大工程确认瓶颈后再决定。
