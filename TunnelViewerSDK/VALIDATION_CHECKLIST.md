# 三软件首次回归检查清单

建议第一次替换统一 SDK 后按下面顺序测试。

## 公共能力

- [ ] SDK x64 Debug / Release 均能编译、链接。
- [ ] DB 路线加载、滚动、缩放、LOD 正常。
- [ ] 普通整图加载正常。
- [ ] 当前正式 `img_<tag>.jph + img_<tag>_0001.jpd` Pack 路线加载正常。
- [ ] `loadPackRoute(目录)` 和 `loadPackRoute(具体.jph)` 两种方式都能读取。
- [ ] 最终 EXE 运行目录存在与构建配置对应的 `ImagePack.dll`。
- [ ] 清空工程后重新打开另一工程无崩溃。

## SDK.rar 对应软件

- [ ] `setImageBrightness()` 正常。
- [ ] `setImageAdjustments()` 的亮度、对比度、锐化正常。
- [ ] GridSelection 正常。
- [ ] VirtualSequence 正常。

## SDK0831 对应软件

- [ ] `Mode_Move` 只能按原业务移动轨枕。
- [ ] Sleeper 能移动，不再因 `locked=true` 失效。
- [ ] 双视图按图片名同步时，DB 模式和虚拟序列模式都不崩溃。
- [ ] 矩形 / 斜矩形 / 三点矩形绘制正常。

## TunnelViewer 对应软件

- [ ] 老数据库的 ElementType 7/8 在读取边界使用 `TunnelViewerLegacy` 映射。
- [ ] 拱脚线显示正常、文字标签不遮挡。
- [ ] 默认不能拖拱脚线；开启 `setTunnelLocationMoveEnabled(true)` 后可移动。
- [ ] DXF 点选信号正常。
- [ ] 基准期数据库能读到真实起止里程。
- [ ] 卷帘对比能开启、拖动分割线、切换横/竖方向。
- [ ] 卷帘关闭和 `clear()` 后重新加载工程正常。

## 数据安全重点

- [ ] 用一份历史工程副本检查枚举映射，不要第一次直接写回正式数据库。
- [ ] 检查病害/拱脚线/轨枕保存后的类型值是否符合各软件预期。


## 当前 Pack 专项

- [ ] `loadPackRoute(单一 Pack 目录)` 能打开当前 `img_<tag>.jph/.jpd`。
- [ ] `loadPackRoute(具体 img_<tag>.jph)` 能打开同一 Pack。
- [ ] 一个目录放两个 `img_*.jph` 时应明确返回失败，不自动选择。
- [ ] Pack 浏览缩小时使用缩略解码，放大到高清阈值后按需读取完整 JPEG。
- [ ] 单图和 Pack 在滚动、镜像、亮度、对比度、锐化、缓存和预加载上的行为一致。
- [ ] 最终 EXE 目录存在当前 V1.1.1 `ImagePack.dll`。
