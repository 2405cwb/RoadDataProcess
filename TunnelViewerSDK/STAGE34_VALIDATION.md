# Stage 3 + Stage 4 验证清单

## 已完成静态验证

- `ViewMode` 枚举块与 Stage12 基线一致；
- `DrawShape` 枚举块与 Stage12 基线一致；
- `ElementType` 枚举块与 Stage12 基线一致；
- `AnnotationData` 结构体字段块与 Stage12 基线一致；
- 新 `formatVersion/typeKey/layerKey/legacyElementType` 没有直接进入 `AnnotationData`；
- `IDefectStorage` 原纯虚函数集合未改变；
- 10 个旧 `DefectManager*` 成员仍全部存在；
- `TunnelViewerSDK.vcxproj` / `.filters` XML 可解析；
- 修改文件括号/花括号静态平衡检查通过；
- `git diff --check` 通过；
- DB 二分候选算法用随机不同尺寸图片，对 Vertical/Horizontal/正序/反序分别与旧线性 intersects 扫描对照，没有发现候选遗漏。

## 当前环境无法完成

当前容器没有 VS2015、Qt 5.8.0、MSVC v140，因此不能替代你的目标开发机完成实际编译/链接。

## 建议目标机第一轮验证

1. 用原 VS2015 + Qt5.8 配置直接覆盖 Stage34 增量源码并 Rebuild；
2. 打开一份旧 Disease 工程，只浏览不保存；
3. 打开一份旧 Sleeper 工程，验证 Move 模式只能按旧规则移动；
4. 打开旧 TunnelViewer 7/8 数据，验证拱脚定位线和 UserLine 解释不变；
5. 对旧工程分别执行：缩放、滚动、框选、拖动、导出；
6. 保存为副本后重新打开，对比图元数量、UUID、Path、typeCode、位置；
7. 横向和纵向 DB 工程都快速滚动，观察边界图片是否存在漏加载/黑块；
8. 对 1000+、10000+ 图片工程记录 `updateVisibleTiles` CPU 占用及首次加载时间。

## 新扩展能力验证

### Factory

- 注册一个新的 layerKey；
- 注册新的 typeKey Factory；
- `addDefect` 后确认派生 Item 正常显示；
- 保存后重开，确认仍恢复成派生 Item；
- 注销 Factory 后重开，确认安全回退为 `DefectShapeItem`，而不是崩溃。

### Storage attributes

确认业务 Storage 对 `DefectData::attributes` 为完整透传：

- `typeKey`
- `layerKey`
- `formatVersion`
- `legacyElementType`
- 自定义业务属性

### Layer

分别测试：

- visible false -> true：原本单独隐藏的 Item 仍保持隐藏；
- locked true -> false：原本自身 locked 的 Item 仍然锁定；
- selectable false：点击/拖动引擎不能绕过；
- exportable false：界面仍可见但导出不出现；
- zValue override -> clear：恢复各 Item 原 Z 顺序。

### Draw Context

同时连接旧 `sigGeometryDrawn` 和新 `sigAnnotationGeometryDrawn`：

- 旧内置绘图仍收到旧信号；
- 自定义 Tool 能收到 layer/type/context；
- 从自定义 Tool 切回 `startDrawingDefect()` 后不会携带旧 Context。

## 性能对照建议

建议同一数据集分别记录 Stage12 / Stage34：

- DB 图片数量：1k / 10k / 30k；
- 首次 addLayer 总耗时；
- 连续快速滚动时主线程峰值；
- HDD/网络盘读取请求数量；
- Thumbnail/Full 缓存命中情况；
- 10万+ Annotation 情况下标签开启/关闭的 FPS。

如果这些验证通过，再进入后续 P2（Annotation 虚拟化、Adaptive Decode、ViewportUpdateMode 实测）会更稳妥。
