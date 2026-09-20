# 当前 ImagePack 接入说明

本工程只支持用户当前提供的 **ImagePack SDK V1.1.1** 正式格式，不保留任何历史 Pack 兼容代码。

## 当前格式

```text
img_<tag>.jph
img_<tag>_0001.jpd
img_<tag>_0002.jpd
...
```

TunnelViewerSDK 读取时可传：

- 具体 `img_<tag>.jph`；
- 或一个只包含 **一个** 当前 Pack 的目录。

如果目录里存在多个 `img_*.jph`，SDK 会返回失败，不会自行猜“最新一个”，避免打开错误工程。

## 单图和 Pack 的统一显示流程

```text
单图路径列表 -> FileSequenceFrameSource ----┐
                                         ├-> ISequenceFrameSource
当前 Pack ----> PackSequenceFrameSource ----┘
                                                     |
                                                     v
                                            loadSequenceSource
                                                     |
                                                     v
                              ImageSequenceModel / SequenceChunkItem
                                                     |
                       缩略图浏览 <-> 高清图按需加载 / 缓存 / 预加载
                                                     |
                                                     v
                                   亮度 / 对比度 / 锐化 / 绘制
```

因此两种模式从 `ISequenceFrameSource` 往后完全相同。Pack 不再通过伪 URI、单帧 TileSource 或 `AsyncImageLoader` 的 Pack 特判显示。

## 当前 Pack Reader 做什么

`ImagePackReader` 只负责：

1. 动态加载当前 `ImagePack.dll`；
2. 打开 `img_<tag>.jph`；
3. 分页获取轻量帧索引；
4. 按需读取某一帧 JPEG 原始字节。

它不负责缩略图策略、高清切换、缓存、绘制或图像调节，这些全部由统一虚拟序列层处理。

## 关于写入

TunnelViewerSDK 是浏览/显示 SDK，本身不重复封装 Writer。生成 Pack 的程序继续直接使用本工程
`third_party/ImagePackSDK/include/ImagePackApi.h` 中的当前 Writer API：

- `ImagePack_OpenWriter`
- `ImagePack_Append`
- `ImagePack_Close`

参考 `third_party/ImagePackSDK/ImagePackSdkExample.cpp`。这样读写双方共用同一个正式格式定义，不会出现两套实现漂移。

## 部署

最终 EXE 目录必须存在对应配置的 `ImagePack.dll`。VS/qmake 工程已经配置 Debug/Release 构建后复制 DLL。

本次没有修改业务软件版本号，也没有提升 TunnelViewerSDK 对外版本号。
