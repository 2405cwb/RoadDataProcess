# TunnelViewerSDK 三分支统一版说明（2026-09-01）

## 1. 合并基线

本版本以 **SDK0831** 为主干，因为它已经包含 Pack/普通整图虚拟序列、轨枕、移动模式、双视图同步和新绘图交互。

在此基础上合入：

- `SDK.rar`：`ImageDisplayAdjustments`，恢复亮度 / 对比度 / 锐化完整显示调节。
- `TunnelViewerSDK.rar`：卷帘对比、真实起止里程、DXF 测量、拱脚线 / UserLine 兼容能力。

目标是让三个软件以后只维护这一套 SDK，而不是再复制三份源码分别修改。

## 2. 已处理的关键冲突

### ElementType 历史数值冲突

旧 TunnelViewer 曾把持久化值 `7/8` 用作 `Type_SPRINGING_LOC / Type_UserLine`；新主干又把 `7/8` 用作 `Type_CustomOverlay / Type_Sleeper`。

统一版采用：

- 新主干值 `7/8` 保持不变，避免现有新软件数据失配；
- `Type_SPRINGING_LOC=9`、`Type_UserLine=10`；
- `TunnelGlobal.h` 增加 `ElementTypePersistenceProfile`、`elementTypeFromPersistedValue()` 和 `elementTypeToPersistedValue()`。

**重要：历史 7/8 本身无法自动判断来自哪个软件。** 老 TunnelViewer 从数据库读出旧整数时，请在数据边界明确使用 `TunnelViewerLegacy` 做一次转换，不要直接 `static_cast<ElementType>(7/8)`。

示例：

```cpp
ElementType type = elementTypeFromPersistedValue(
    dbValue,
    ElementTypePersistenceProfile::TunnelViewerLegacy);
```

### 图片显示接口

同时保留：

```cpp
setImageBrightness(int value);        // 0831 旧接口
setImageAdjustments(adjustments);     // 亮度/对比度/锐化完整接口
```

`setImageBrightness()` 现在只是兼容包装，不会清空已经设置的对比度和锐化。

### 轨枕移动冲突

0831 原代码一边在 `Mode_Move` 里只允许移动 `Type_Sleeper`，一边又在 `DefectManager` 把 Sleeper 锁死。

统一版修为：

- Sleeper 默认不锁；
- 默认仅允许垂直移动；
- `Mode_Move` 仍只选择 Sleeper。

### 双视图同步空指针

修复 `synchronizeToImagePixel(QString, ...)` 在判断虚拟序列是否存在之前访问 `m_sequenceModel` 的问题。

DB 模式现在按图片名查找对应 `TunnelSectionItem`，虚拟序列模式才访问 `m_sequenceModel`。

### 三点矩形

`Shape_ThreePointRectangle` 原来只有枚举和提示文字，没有真正独立的三点绘制流程。

统一版将 `Shape_ThreePointRectangle` 与三点斜矩形算法统一：

1. 第一点：基准边起点；
2. 第二点：基准边终点；
3. 第三点：确定垂直宽度并完成闭合矩形。

### 细线点击区域

旧 TunnelViewer 曾在 `paint()` 中通过 `setPath(stroker.createStroke(path()))` 扩大细线点击范围，这会直接改掉真实几何。

统一版改为重写 `DefectShapeItem::shape()`：只扩大鼠标命中区域，不改变真实 `path()`，因此保存、计算、导出都仍使用原始几何。

## 3. 卷帘对比

卷帘功能作为可选增强保留，不改变 `AbstractTileSource`：

```cpp
addLayer(source);                 // 原接口，另外两个软件继续使用
addLayer(source, dbImageInfo);    // 带真实里程元数据的 TunnelViewer 路径
```

控制器保留：

```cpp
loadCurtainCompareRoute(...)
setCurtainCompareEnabled(...)
setCurtainOrientation(...)
setCurtainPosition(...)
clearCurtainCompare()
```

数据库 `info` 表同时兼容：

- `key/value` 两列结构；
- 单行多列结构；
- 多种历史真实里程字段名称。

## 4. 老 TunnelViewer 专项接口

保留/恢复：

- `setDxfMeasureEnabled()` / `sigDxfMeasurePoint()`；
- `setTunnelLocationMoveEnabled()`；
- `Type_SPRINGING_LOC` / `Type_UserLine`；
- `m_vecArchitraveLineManager`；
- `getAllItemDBPath()`；
- 严格图片名映射接口 `hasExactImageLayer()` / `tryMapToExactImageLayer()`。

为了源码兼容，历史上公开的 Manager 成员继续公开；新代码建议逐步改用 accessor。

## 5. 工程依赖

本包已把 OpenCV include/lib 路径统一到 SDK 自带的 `opencv480` 目录。

2026-09-01 按当前 ImagePackSDK V1.1.1 收敛 Pack 读取层：

- 只支持正式 `img_<tag>.jph + img_<tag>_0001.jpd ...`；
- `ImagePackReader` 底层通过 `QLibrary` 动态加载当前 `ImagePack.dll` C ABI；业务软件无需新增 `ImagePack.lib` 链接项；
- `loadPackRoute(QString, ...)` 保持现有入口，可传唯一 Pack 目录或具体 `img_<tag>.jph`；
- Pack 元数据使用 `ImagePack_GetImageList` 每 4096 条分页读取；
- Pack 与单图只在 `ISequenceFrameSource` 读取实现不同，显示链路完全共用；
- 没有提升 TunnelViewerSDK / 三个业务软件的版本号。

ImagePack 开发文件已经放在 `third_party/ImagePackSDK`，不再需要从其他工程引用相对路径。

## 6. 后续维护规则

以后只保留一个 `main` SDK：

- 通用能力：直接进 SDK Core；
- 专项功能：独立 Feature/Manager，默认关闭；
- 软件自己的默认参数、按钮、业务文案：留在软件层；
- 公共枚举必须显式赋值；
- 已发布 API 尽量不删除，用兼容包装转到新 API；
- 禁止为某一个软件重新复制一份 SDK 源码长期维护。


## 2026-09-01 当前 Pack 最终收敛

根据当前 ImagePackSDK V1.1.1，Pack 支持重新收敛：

- 删除 `PackImageTileSource`；
- 删除 `packv2://` 虚拟 URI；
- 删除 `AsyncImageLoader` 中所有 Pack 特判与 Reader 缓存；
- 删除 Controller 中 `#if 0` 的旧逐帧 Pack 显示实现；
- 内部 Reader 改名为 `ImagePackReader`，只识别 `img_<tag>.jph` 当前命名；
- 目录存在多个当前 Pack 时明确失败，不自动猜选；
- `FileSequenceFrameSource` 与 `PackSequenceFrameSource` 最终都汇入 `loadSequenceSource()`；
- 缩略图/高清图、缓存、预加载、滚动和图像调节只维护一套；
- Pack 写入继续使用随包附带的正式 `ImagePackApi.h` / `ImagePackSdkExample.cpp`，TunnelViewerSDK 不复制 Writer 实现；
- 软件/SDK 对外版本号未修改。
