# 第一、二阶段静态兼容验证

已完成的静态检查：

- PASS：`ViewMode` 数值与升级前完全一致。
- PASS：`DrawShape` 数值与升级前完全一致。
- PASS：`ElementType` 0~10 数值与升级前完全一致。
- PASS：`AnnotationData/DefectData` 原 8 个字段保持同名、同顺序、同数量；新元数据只使用既有 `attributes`。
- PASS：原 `TiledGraphicsView` 公共方法名称未删除。
- PASS：原 10 个公开 `DefectManager*` 成员全部保留。
- PASS：TunnelViewer 历史定位线/用户线 Manager 使用 `TunnelViewerLegacy` profile。
- PASS：Visual Studio `.vcxproj` XML 可正常解析。
- PASS：`.vcxproj` 新增引用的源/头文件均存在。
- PASS：虚拟序列缓存已由 `SequenceLoadOptions::decodedCacheBytes` 接入按字节管理的 `ImageCacheManager`。
- PASS：虚拟序列模型建立后同步绑定 `ImageCoordinateMapper`，清理时同步解除。
- PASS：`clearVirtualSequence()` 主动释放 Chunk Item，`clearImages()` 不依赖 `scene->clear()`。

## 建议在 VS2015 + Qt 5.8.0 的最终回归顺序

1. 原三个软件不改业务代码，直接重新编译 SDK。
2. 各打开一个历史工程，检查病害、轨枕、环片、CP3、站台等加载数量和位置。
3. 老 TunnelViewer 专门检查旧值 7/8 的拱脚/用户线类型、可选范围和移动限制。
4. 打开后不保存，核对显示结果；再保存副本并重新打开核对一次。
5. 对同一工程分别验证缩放、滚动、图片跳转、坐标反算、导出。
6. 对普通单图序列和 ImagePack 各做一次高速滚动，确认缩略图 -> 高清恢复行为没有变化。
7. 调用一次新 `clearImages()`，确认 Annotation 保留；调用 `clearAnnotationLayer()`，确认只删除目标业务层。
8. 注册一个外部测试 Annotation Manager 和一个自定义 AbstractTool，确认不修改 SDK 也能接入。

说明：当前执行环境没有用户的 Windows/VS2015/Qt 5.8 编译环境，因此本报告是源码结构、工程 XML 和兼容不变量检查；最终二进制编译与真实历史 DB 回归仍应在目标开发环境执行。
