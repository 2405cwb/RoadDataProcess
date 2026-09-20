# 共用图像显示 SDK 第一、二阶段升级说明

## 目标

本次升级只做底层共用化和扩展基础，不重写现有图像算法，不改变旧业务 API 的名称与核心语义。

核心原则：

1. 旧 `TiledGraphicsView`、`DefectManager`、`DefectShapeItem`、`ElementType` 继续可用。
2. 旧 `ElementType` 数值完全不变。
3. `AnnotationData/DefectData` 原 8 个字段的名称、顺序和数量完全保持不变；新增 SDK 元数据全部写入原有 `attributes`，不再改变结构体布局。
4. 老 TunnelViewer 的 ElementType 7/8 冲突继续通过 `ElementTypePersistenceProfile::TunnelViewerLegacy` 显式解释。
5. 不修改 `ImageSequenceModel` 的累计布局算法、不修改现有 Scene 坐标含义、不修改缩放/平移/LOD 的既有计算流程。

---

## 第一阶段：共用 Annotation 底层

### 1. Manager Registry

`TiledGraphicsView` 新增：

- `registerAnnotationManager(layerKey)`
- `annotationManager(layerKey)`
- `annotationLayerKeys()`
- `clearAnnotationLayer(layerKey)`
- `clearAnnotations()`

旧成员 `m_defectManager / m_vecSleeperManager / ...` 仍然保留，并且默认都指向 Registry 中对应的同一个 Manager。

### 2. 旧业务类型冻结 + 新通用 key

`ElementType` 保持原数值不变；新标注可使用：

- `layerKey`
- `typeKey`
- `QVariantMap attributes`

不再要求为了新增业务类型继续扩展 `ElementType`。

### 3. AnnotationData V2 向后兼容扩展

`AnnotationData/DefectData` **不追加任何成员**，继续保持原 8 个字段的名称、顺序和数量。新增 SDK 元数据统一写入原本已有的 `QVariantMap attributes`：

- `__sdk.formatVersion`
- `layerKey`
- `typeKey`
- `__sdk.legacyElementType`

旧存储/旧数据没有这些 key 时，Manager 会根据旧 `ElementType`、所属 layer 和现有兼容 profile 自动补齐，不要求迁移历史数据。这样既保证旧格式继续可用，也避免因为给 `DefectData` 追加成员而扩大二进制布局风险。

### 4. 旧数据兼容映射

每个内置 Manager 设置自己的旧 `ElementType` 默认值：

- cp3 -> `Type_Cp3`
- platform -> `Type_Platform`
- chain -> `Type_Chain`
- sleeper -> `Type_Sleeper`
- disease -> `Type_Disease`
- tunnel_location -> `Type_SPRINGING_LOC`
- ring -> `Type_Ring`
- section -> `Type_Section`
- auto_ring -> `Type_AUTORING`
- user_line -> `Type_UserLine`

其中 `tunnel_location` 和 `user_line` 继续使用 `TunnelViewerLegacy` profile，保证历史 7/8 数据语义不漂移。

### 5. attributes 行为配置

统一增加以下行为 key：

- `locked`
- `movable`
- `selectable`
- `rubberBandSelectable`
- `moveAxis`
- `showLabel`
- `exportable`
- `hitTolerance`

显式属性优先；没有设置时继续执行旧 `ElementType` 行为。

`AnnotationTypeConfig::attributes` 现在会作为 Item 默认属性真正应用，Item 自身属性优先覆盖配置默认值。

### 6. AbstractTool 对外开放

`TiledGraphicsView::m_currentTool` 内部改为 `AbstractTool*`，新增：

- `setActiveTool(AbstractTool*, bool takeOwnership)`
- `activeTool()`

旧 `startDrawingDefect()` 继续使用原 `DefectDrawTool`，行为保持不变；外部可直接注入新的 Tool。

### 7. 生命周期整理

- Manager 监听 Item 销毁，自动清理内部索引。
- `clearDefects()` 先清账本再删除 Item，避免 destroyed 回调修改正在遍历的 Hash。
- `clear()` 会清理所有注册 Manager，包括以后外部注册的新图层。
- 新增 `clearImages()`，允许只清图像不清 Annotation。
- 虚拟序列的 Chunk Item 现在由 `clearVirtualSequence()` 自己释放，不再依赖最后一次 `scene->clear()` 才释放。

---

## 第二阶段：图像显示核心收口

### 1. IImageSource

新增 `include/ImageSource.h`：

- `IImageSource = ISequenceFrameSource`
- `FileImageSource = FileSequenceFrameSource`
- `PackImageSource = PackSequenceFrameSource`

这是同一接口的通用命名，不复制解码链路。

### 2. ImageLayoutManager

新增 `include/ImageLayoutManager.h`：

- `ImageLayoutManager = ImageSequenceModel`

继续复用当前累计 Scene 坐标、正反布局、名称 Hash、O(logN) 可视定位算法。

### 3. ImageCacheManager

新增 `include/ImageCacheManager.h`，虚拟序列已实际接入。

特点：

- 以“字节数”配置缓存上限；
- 内部继续使用 Qt 5.8 可用的 `QCache`；
- cost 统一换算为 KiB；
- 不再从业务角度按“缓存多少张图”估算。

`SequenceLoadOptions::decodedCacheBytes` 直接控制这一缓存。

### 4. ImageCoordinateMapper

新增 `include/ImageCoordinateMapper.h` 和 `ImageCoordinate`。

提供：

- image index/local pos -> Scene
- image name/local pos -> Scene
- Scene -> image index/name/local pos

`TiledGraphicsView` 新增统一接口：

- `imageCoordinateToScene()`
- `sceneToImageCoordinate()`

虚拟序列的 `GlobalSceneToMap()` 和 `tryMapToGlobalScene()` 已接入 Mapper；数据库瓦片仍保持原有 Item 映射逻辑。

### 5. AsyncImageLoader 基础增强

旧数据库瓦片加载器不替换，新增：

- `setCacheMemoryLimits()`
- `setThumbnailThreadCount()`
- `clearCaches()`
- 缓存占用查询

因此旧 DB 瓦片路径不受影响，同时已经具备统一配置入口。

### 6. 图像与 Annotation 分层清理

现在至少可以分别执行：

- `clearImages()`
- `clearAnnotations()`
- `clearAnnotationLayer(layerKey)`
- 旧 `clear()`（仍然清全部）

为后续完整 Scene Layer/Style/Factory 阶段打基础。

---

## 旧数据使用要求

### 普通旧数据

无需迁移。旧存储实现返回原来的 `DefectData` 字段即可；新字段保持默认值时，Manager 会按所属图层补齐。

### 老 TunnelViewer 7/8

仍然不要直接 `static_cast<ElementType>(7/8)`。

- tunnel_location Manager 已配置 `TunnelViewerLegacy`
- user_line Manager 已配置 `TunnelViewerLegacy`

如果业务在 Manager 外自行解析历史整数，继续使用：

```cpp
ElementType type = elementTypeFromPersistedValue(
    oldValue,
    ElementTypePersistenceProfile::TunnelViewerLegacy);
```

---

## 本阶段刻意没有修改的内容

- `ElementType` 原数值；
- `DrawShape` 原数值；
- ViewMode 原数值；
- `ImageSequenceModel::build()` 布局算法；
- 现有图片排列规则；
- 现有缩放和平移规则；
- 现有 LOD 判定主体；
- 原图/缩略图解码结果；
- 数据库瓦片表读取逻辑；
- 原有 `TiledGraphicsView` 对外函数名称；
- 原有公开 Manager 成员名称。
