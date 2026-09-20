# TunnelViewerSDK 最新完整源码（Stage 1-4 + 最终收束修正）

本目录为完整工程，不是增量覆盖包。已合并：

- 原始 TunnelViewerSDK 工程主体与第三方依赖；
- Stage 1/2：Annotation 通用化基础、Manager Registry、ImageSource/Cache/Layout/CoordinateMapper 等；
- Stage 3/4：Item Factory、Layer Runtime State、交互策略、导出过滤、DB/整图二分调度、缓存与预取优化等；
- 最终收束修正：旧高亮语义恢复、ImageCoordinate.imageIndex 统一为 visualIndex、AbstractTool mouseRelease 转发、typeKey 归一化等。

## 兼容边界

- `ViewMode` / `DrawShape` / `ElementType` 旧枚举数值保持不变；
- `DefectData` / `AnnotationData` 保持旧 8 字段布局；
- `IDefectStorage` 原有纯虚接口保持不变；
- TunnelViewer 历史 7/8 类型映射保留；
- 新 `typeKey/layerKey/formatVersion/legacyElementType` 通过原有 `QVariantMap attributes` 扩展；
- 旧业务源码应使用本 SDK 重新编译，不承诺直接替换旧 EXE 所使用静态库的 ABI 二进制兼容。

## 建议验证环境

Visual Studio 2015 / MSVC v140 + Qt 5.8.0。当前生成环境未具备该工具链，因此请在目标开发机完成最终编译、链接和真实历史数据回归。
