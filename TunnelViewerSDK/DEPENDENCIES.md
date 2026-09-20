# 编译依赖

## 必需

- Qt Widgets
- Qt Concurrent
- Qt Sql / QSQLITE
- OpenCV 4.8.0（本包已有 `opencv480`）

## 当前 ImagePack V1.1.1

本工程直接携带用户本次提供的正式开发/运行文件：

```text
third_party/ImagePackSDK/include/ImagePackApi.h
third_party/ImagePackSDK/lib/x64/Debug/ImagePack.lib
third_party/ImagePackSDK/lib/x64/Release/ImagePack.lib
third_party/ImagePackSDK/bin/x64/Debug/ImagePack.dll
third_party/ImagePackSDK/bin/x64/Release/ImagePack.dll
third_party/ImagePackSDK/ImagePackSdkExample.cpp
```

TunnelViewerSDK 的显示读取层通过 `QLibrary` 动态调用当前 `ImagePack.dll`，因此三个业务软件不需要额外增加 `ImagePack.lib` 链接项。
`.pro` 和 `.vcxproj` 均会在构建后复制对应 Debug/Release `ImagePack.dll`。

最终应用运行目录必须能找到 `ImagePack.dll`。

当前只支持正式文件名：

```text
img_<tag>.jph
img_<tag>_0001.jpd
img_<tag>_0002.jpd
...
```

Pack 写入程序继续直接使用正式 `ImagePackApi.h` 的 Writer API；TunnelViewerSDK 不重复实现写入格式。

## 编译环境说明

当前执行环境不是 Windows + Qt 5.8 + VS2015/v140，因此无法完成最终 MSVC 二进制编译；已完成源码结构、项目 XML、依赖路径、DLL 导出接口、当前 Pack 接入逻辑和压缩包完整性检查。
