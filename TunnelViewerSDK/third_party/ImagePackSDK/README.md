# ImagePack SDK 发布目录

调用方不需要 `Core` 源码。

## x64 Release 发布内容

```text
include/ImagePackApi.h
bin/x64/Release/ImagePack.dll
lib/x64/Release/ImagePack.lib
examples/ImagePackSdkExample.cpp
docs/ImagePack_FileFormat_and_SDK_Interface_V3_1.docx
```

所有 `const char*` 路径参数按 UTF-8 传入。DLL 边界只使用 C ABI、整数、POD 结构体、调用方 buffer 和 `IMAGEPACK_HANDLE`。

## 固定宽度整数类型

为避免 VS2015/v140 的 CRT/STL 头文件混用冲突，公共头使用 `IMAGEPACK_U8 / IMAGEPACK_U32 / IMAGEPACK_U64 / IMAGEPACK_I32`。调用方请直接使用这些类型或与之等宽的自身类型。


## Pack 文件命名

当前 SDK 只使用以下正式命名：

```text
img_<tag>.jph
img_<tag>_0001.jpd
img_<tag>_0002.jpd
```

Writer 只生成上述新命名；Reader 仍兼容旧命名 `img-<tag>.jph`、`0001-img-<tag>.jpd`。
