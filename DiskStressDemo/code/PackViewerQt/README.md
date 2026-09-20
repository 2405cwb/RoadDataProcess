# PackViewerQt 图片包浏览器

## 1. 项目定位

`PackViewerQt` 是一个 Qt Widgets 示例程序，用来验证后处理软件如何从“直接读取单张 jpg 文件”迁移到“通过 `PackSdk` 读取图片包”。它不是采集程序，也不是完整业务后处理软件，而是一个可运行的 SDK 调用样例。

它展示了这些典型后处理需求：

- 选择一个采集输出图片包目录。
- 通过 SDK 获取图片总数和图片索引列表。
- 点击列表跳转到某张图片。
- 上一张 / 下一张顺序浏览。
- 按全局序号或采集帧号定位图片。
- 从 SDK 读取 JPEG 字节，并用 Qt 解码显示。
- 保存当前图到本地。
- 批量导出全部图片。
- 调用 SDK 校验 pack 数据。

## 2. 程序结构

```text
code/PackViewerQt/
    main.cpp                Qt 程序入口
    MainWindow.h            主窗口定义
    MainWindow.cpp          主窗口逻辑
    QtMsvcCompat.h          旧 MSVC + 新 Windows SDK 兼容头
    PackViewerQt.pro        qmake 工程
    PackViewerQt.vcxproj    Qt VS Tools 工程
    CMakeLists.txt          Windows/Ubuntu CMake 工程
```

依赖：

```text
code/PackSdk/               SDK C ABI 和 Qt 封装
bin/Release-x64/PackSdk.dll Windows Release SDK DLL
```

## 3. 界面功能

### 打开图片包

点击“打开目录”后选择类似下面的目录：

```text
output/20260616_105858_665/
```

该目录下应包含：

```text
Pack_000000.dat
Pack_000000.idx
Pack_000001.dat
Pack_000001.idx
...
```

程序会调用：

```cpp
PackReaderQt reader(path);
reader.count();
reader.frameInfo(index);
```

然后把所有图片显示到左侧列表。

### 浏览图片

支持：

- 点击左侧列表项跳转。
- “上一张 / 下一张”顺序浏览。
- 输入 `globalIndex` 跳转。
- 输入 `sourceIndex` 跳转。

读取路径：

```text
用户选择图片 -> PackReaderQt::readJpeg -> Pack_ReadJpeg -> QImage::loadFromData
```

注意：SDK 只返回 JPEG 字节，真正图像解码由 Qt 完成。

### 图片信息

界面会显示：

- `globalIndex`
- `sourceIndex`
- `timeValue`
- `packNo`
- `packFrameIndex`
- `datOffset`
- `jpgSize`
- Qt 解码后的图片宽高
- SDK 状态标记

这些信息都来自 SDK 或 Qt 解码结果。

### 保存和导出

“保存当前图”调用：

```cpp
Pack_SaveJpeg / Pack_SaveJpegUtf8
```

“导出全部”调用：

```cpp
Pack_ExportAll / Pack_ExportAllUtf8
```

导出功能适合完全不想改造的旧后处理软件：先导出成旧单图目录，再让旧软件继续按 jpg 文件夹处理。

批量导出时界面会弹出进度条，并实时显示：

- 已导出张数 / 总张数
- 已耗时
- 当前平均导出速度，单位为张/秒
- 当前平均写入吞吐，单位为 MB/秒
- 已写入总字节数

导出结束后，右侧信息框会显示完整统计：

- 计划导出数量
- 成功导出数量
- 失败数量
- 导出总字节数
- 总耗时
- 平均张/秒
- 平均 MB/秒
- 平均每张耗时
- 最后导出文件路径

这些统计放在“批量导出”动作里最合适，因为它们描述的是一次导出任务的整体性能，而不是某一张图片的静态属性。

### 校验

“校验”调用：

```cpp
Pack_Verify
```

用于检查 idx/dat 的基本一致性。发现异常时，界面会显示 SDK 返回的错误或摘要信息。

## 4. Windows 构建

先构建 SDK：

```bat
msbuild code\PackSdk\PackSdk.vcxproj /p:Configuration=Release /p:Platform=x64
```

然后构建 Qt 示例：

```bat
msbuild code\PackViewerQt\PackViewerQt.vcxproj /p:Configuration=Release /p:Platform=x64
```

也可以在 Visual Studio 中打开解决方案：

```text
code/DiskStressDemo.sln
```

选择 `PackViewerQt`，配置为 `Release|x64` 或 `Debug|x64` 后编译。

## 5. Qt Creator / qmake 构建

用 Qt Creator 打开：

```text
code/PackViewerQt/PackViewerQt.pro
```

确认 Qt Kit 与 SDK 位数一致，例如都是 x64。构建前应先生成：

```text
bin/Release-x64/PackSdk.dll
bin/Release-x64/PackSdk.lib
```

## 6. CMake 构建

Windows：

```bat
cmake -S code\PackViewerQt -B build\packviewer -DCMAKE_PREFIX_PATH=C:\Qt\Qt5.8.0\5.8\msvc2015_64
cmake --build build\packviewer --config Release
```

Ubuntu：

```bash
sudo apt install build-essential cmake qtbase5-dev
cmake -S code/PackViewerQt -B build/packviewer -DCMAKE_BUILD_TYPE=Release
cmake --build build/packviewer
```

Ubuntu 下 Qt 封装会自动调用：

```text
Pack_OpenUtf8
Pack_SaveJpegUtf8
Pack_ExportAllUtf8
```

路径按 UTF-8 处理。

## 7. 常见编译问题

### `_mm_loadu_si64` 找不到标识符

如果使用 VS2015/VS2017 + 很新的 Windows SDK，例如 `10.0.26100.0`，可能遇到：

```text
C3861: "_mm_loadu_si64": 找不到标识符
```

原因是新版 UCRT 头文件假定编译器提供 `_mm_loadu_si64`，但旧 MSVC 没有这个 intrinsic。

本项目已经在：

```text
QtMsvcCompat.h
```

里补了兼容实现，并且 `main.cpp` / `MainWindow.h` 会优先包含它。如果把示例代码拆到其他工程，也要保证 `QtMsvcCompat.h` 在 Qt/CRT 头文件之前被包含。

### Qt 5.8 + CMake + Windows 中文路径

Qt 5.8 的 `moc.exe` 在 Windows 中文路径下可能把路径转成乱码，导致无法生成 `moc_MainWindow.cpp`。解决方式：

- 优先使用本目录的 `.pro` 或 `.vcxproj` 构建。
- 或者把源码放到纯英文路径后再用 CMake 构建。
- Ubuntu 下一般不会遇到 Windows 代码页路径问题。

## 8. 如何用它验证 SDK

推荐流程：

1. 用采集程序生成一组 pack 数据。
2. 启动 `PackViewerQt.exe`。
3. 打开 pack 输出目录。
4. 检查左侧列表数量是否等于 `SaveSummary.txt` 中的保存数量。
5. 浏览第一张、中间一张、最后一张。
6. 用 `sourceIndex` 跳转，确认帧号定位正确。
7. 保存当前图，用 Windows 图片查看器打开。
8. 点击校验，确认坏帧数量为 0。

## 9. 和真实后处理软件的迁移关系

老软件常见逻辑：

```text
枚举 jpg 文件 -> 读取图片路径 -> 解码 -> 业务处理
```

推荐改造为：

```text
ImageSource 抽象 -> 文件夹图片源 / Pack 图片源 -> 统一返回 JPEG 字节或临时文件路径
```

最小改造：

- 老软件仍然要路径：调用 `Pack_SaveJpeg` 按需保存到临时目录。
- 老软件能接受字节：调用 `Pack_ReadJpeg`，再用原来的图像库解码。
- 完全不想改业务：调用 `Pack_ExportAll` 导出成旧单图目录。

`PackViewerQt` 就是“Pack 图片源”的一个完整示例。

## 10. 后续可扩展功能

可以继续在界面上加这些功能：

- 当前图另存为 png/bmp。
- 显示 JPEG 解码耗时。
- 显示 SDK 读取耗时。
- 显示连续播放帧率。
- 增加缩略图列表。
- 增加坏帧过滤视图。
- 增加按时间范围筛选。
- 增加相机编号筛选，前提是 pack v2 写入相机编号。
