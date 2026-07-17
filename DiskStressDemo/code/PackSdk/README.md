# PackSdk 后处理读取 SDK

## 1. 项目定位

`PackSdk` 是采集端图片打包方案的后处理读取 SDK。采集端在高速采集时不再每张图片单独落盘，而是把多张 JPEG 图片顺序写入 `Pack_*.dat`，同时把每张图的偏移、大小、采集帧号和时间写入 `Pack_*.idx`。后处理软件通过本 SDK 读取图片包，避免每个软件都重复解析 pack 格式。

SDK 当前只处理“解包读取 JPEG 原始字节”，不负责 JPEG 像素解码。也就是说：

- `Pack_ReadJpeg` 返回的是 `.jpg` 文件内容级别的二进制字节。
- Qt 继续用 `QImage::loadFromData()` 解码。
- C# 继续用 `Image.FromStream()` 或原有图像库解码。
- 如果要测“JPEG 解码耗时”，需要在调用方单独测，不要和 SDK 磁盘读取耗时混在一起。

## 2. 目录结构

```text
code/PackSdk/
    PackSdk.h              C ABI 头文件，Qt/C++/C# 共同使用
    PackSdk.cpp            SDK 实现
    PackSdk.vcxproj        Windows VS2015 x64 DLL 工程
    CMakeLists.txt         Windows/Ubuntu CMake 构建入口
    csharp/PackReader.cs   C# P/Invoke 封装
    qt/PackReaderQt.h      Qt/C++ 封装
```

相关工具：

```text
code/PackReadBenchmark/    读取速度测试工具
code/PackViewerQt/         Qt 图片包浏览器示例
PACK_FORMAT.md             pack 文件格式说明
```

## 3. 输入数据格式

SDK 读取采集端生成的单次输出目录，例如：

```text
output/20260616_105858_665/
    Pack_000000.dat
    Pack_000000.idx
    Pack_000001.dat
    Pack_000001.idx
    ...
    SaveLog.txt
    SaveSummary.txt
```

读取逻辑：

1. 扫描目录下所有 `Pack_*.idx`。
2. 按 pack 编号排序。
3. 读取 idx 文件头和索引记录。
4. 建立全局图片索引。
5. 读取某张图时，根据 `datOffset + jpgSize` 定位 `.dat` 文件里的 JPEG 字节。

当前支持 `version=1` 的 pack 格式。

## 4. 核心接口

### 打开和关闭

```cpp
Pack_Open(rootDir, options, &handle)
Pack_OpenUtf8(rootDir, options, &handle)
Pack_Close(handle)
```

- Windows/C# 可以使用宽字符版 `Pack_Open`。
- Ubuntu/Linux 和跨平台 Qt 推荐使用 UTF-8 版 `Pack_OpenUtf8`。
- `Pack_Open` 会扫描 idx/dat 并建立内存索引，后续读取直接按全局序号访问。

### 数据集信息

```cpp
Pack_GetDatasetInfo(handle, &info)
Pack_GetFrameCount(handle, &count)
```

可得到 pack 数量、总帧数、时间范围、警告数量等信息。

### 单帧信息和定位

```cpp
Pack_GetFrameInfo(handle, globalIndex, &frameInfo)
Pack_FindBySourceIndex(handle, sourceIndex, &globalIndex)
```

`globalIndex` 是 SDK 建立的全局连续序号，范围为 `[0, count)`。

`sourceIndex` 是采集端写入的原始帧号，通常从 1 开始。老软件如果原来按采集帧号定位图片，可以用 `Pack_FindBySourceIndex` 映射到 `globalIndex`。

### 读取和保存图片

```cpp
Pack_ReadJpeg(handle, globalIndex, buffer, bufferSize, &requiredSize)
Pack_SaveJpeg(handle, globalIndex, outputPath)
Pack_SaveJpegUtf8(handle, globalIndex, outputPath)
```

推荐读取流程：

```cpp
uint32_t required = 0;
Pack_ReadJpeg(handle, index, NULL, 0, &required);

std::vector<unsigned char> jpg(required);
Pack_ReadJpeg(handle, index, jpg.data(), required, &required);
```

如果老后处理软件必须拿到图片路径，可以先用 `Pack_SaveJpeg` 把某张图导出到临时目录，再把临时 jpg 路径传给原逻辑。

### 批量导出和校验

```cpp
Pack_ExportAll(handle, outputDir, namingMode, callback, userData)
Pack_ExportAllUtf8(handle, outputDir, namingMode, callback, userData)
Pack_Verify(handle, &report)
```

`Pack_ExportAll` 用于完全兼容旧软件：把图片包一次性导出成旧的单 jpg 目录。

`Pack_Verify` 用于检查 idx/dat 是否一致，包括文件头、记录长度、dat 帧头、偏移、大小、帧号和时间等。

## 5. C++ 调用示例

```cpp
#include "PackSdk.h"
#include <vector>

PACK_HANDLE handle = NULL;
PACK_OPEN_OPTIONS opt = {};
opt.structSize = sizeof(opt);
opt.flags = PACK_OPEN_DEFAULT;

int ret = Pack_Open(L"D:\\data\\output\\20260616_105858_665", &opt, &handle);
if (ret != PACK_OK)
{
    const wchar_t* err = Pack_GetLastError(NULL);
    return ret;
}

uint64_t count = 0;
Pack_GetFrameCount(handle, &count);

uint32_t required = 0;
Pack_ReadJpeg(handle, 0, NULL, 0, &required);
std::vector<unsigned char> jpg(required);
Pack_ReadJpeg(handle, 0, jpg.data(), required, &required);

Pack_SaveJpeg(handle, 0, L"D:\\temp\\first.jpg");
Pack_Close(handle);
```

Ubuntu/Linux 推荐使用 UTF-8 路径接口：

```cpp
PACK_HANDLE handle = NULL;
PACK_OPEN_OPTIONS opt = {};
opt.structSize = sizeof(opt);

int ret = Pack_OpenUtf8("/data/output/20260616_105858_665", &opt, &handle);
if (ret != PACK_OK)
{
    const char* err = Pack_GetLastErrorUtf8(NULL);
    return ret;
}

Pack_SaveJpegUtf8(handle, 0, "/tmp/first.jpg");
Pack_Close(handle);
```

## 6. Qt 调用示例

```cpp
#include "PackSdk/qt/PackReaderQt.h"
#include <QImage>

PackReaderQt reader("D:/data/output/20260616_105858_665");
QByteArray bytes = reader.readJpeg(0);

QImage image;
image.loadFromData(bytes, "JPG");
```

## 7. C# 调用示例

```csharp
using PackSdk;

using (var reader = new PackReader(@"D:\data\output\20260616_105858_665"))
{
    Console.WriteLine(reader.Count);
    byte[] jpg = reader.ReadJpegBytes(0);
    reader.SaveJpeg(0, @"D:\temp\first.jpg");
}
```

运行 C# 程序时，`PackSdk.dll` 要放在 exe 同目录，或者放进系统 DLL 搜索路径。

## 8. 如何测试读取速度

读取速度测试工具在：

```text
code/PackReadBenchmark/
```

构建：

```bat
msbuild code\PackSdk\PackSdk.vcxproj /p:Configuration=Release /p:Platform=x64
msbuild code\PackReadBenchmark\PackReadBenchmark.vcxproj /p:Configuration=Release /p:Platform=x64
```

测试 pack 顺序读取：

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -pack "bin\硬盘测试\output\20260616_105858_665" ^
  -count 5000 ^
  -batchSize 100 ^
  -loops 3 ^
  -warmup
```

测试 pack 随机读取：

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -pack "bin\硬盘测试\output\20260616_105858_665" ^
  -count 5000 ^
  -batchSize 100 ^
  -loops 3 ^
  -random ^
  -warmup
```

生成一份旧单图基准目录：

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -pack "bin\硬盘测试\output\20260616_105858_665" ^
  -exportDir "bin\Release-x64\single_baseline_5000" ^
  -count 5000
```

测试旧单图目录读取：

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -jpgDir "bin\Release-x64\single_baseline_5000" ^
  -count 5000 ^
  -batchSize 100 ^
  -loops 3 ^
  -warmup
```

关键指标：

- `throughputFPS`：每秒读取多少张 JPEG。
- `throughputMBps`：每秒读取多少 MB。
- `avgMsPerFrame`：平均每张图读取耗时。
- `avgMsPerBatch`：按 `batchSize` 分组后，每批平均耗时。
- `Pack open/index seconds`：SDK 打开目录并建立索引的耗时。

## 9. Pack 和旧单图读取差距怎么看

这个方案的主要收益在采集端写入阶段：

- 旧方案：每张图创建一个 jpg 文件，文件数量巨大，目录项和文件系统元数据压力大。
- pack 方案：少量大文件顺序写入，减少文件创建次数，降低磁盘和文件系统压力。

后处理读取阶段的差距取决于场景：

- 顺序批量读取时，pack 理论上更接近连续读，适合大吞吐。
- 随机读取少量图片时，pack 需要通过 idx 定位 dat 偏移，和直接打开单 jpg 的差距通常不大。
- 热缓存测试时，Windows 文件缓存会让两者都很快，结果可能不能代表冷盘真实吞吐。
- 当前 SDK 每次读取仍会校验 dat 帧头，优先保证数据可靠性；如果后续需要极限速度，可以增加“批量读取接口”或“.dat 文件句柄缓存”。

建议测试至少分四组：

1. pack 顺序读。
2. pack 随机读。
3. 旧单图顺序读。
4. 旧单图随机读。

每组至少跑 3 次，记录平均值。测试时 pack 数据和旧单图数据应来自同一批图片，避免图片大小不同导致结论失真。

## 10. 构建

Windows / Visual Studio:

```bat
msbuild code\PackSdk\PackSdk.vcxproj /p:Configuration=Release /p:Platform=x64
```

Ubuntu:

```bash
cmake -S code/PackSdk -B build/packsdk -DCMAKE_BUILD_TYPE=Release
cmake --build build/packsdk
```

Qt 浏览器示例：

```bash
sudo apt install build-essential cmake qtbase5-dev
cmake -S code/PackViewerQt -B build/packviewer -DCMAKE_BUILD_TYPE=Release
cmake --build build/packviewer
```

## 11. 当前限制和后续优化方向

当前 pack v1 没有保存宽高、相机编号、像素格式、CRC。SDK 首版遵守现有格式：

- `PACK_FRAME_INFO.width = -1`
- `PACK_FRAME_INFO.height = -1`
- 图片内容按 JPEG 原始字节返回
- 不自动重建 `.idx`
- 不在 SDK 内进行 JPEG 解码

后续建议：

- 增加 `Pack_ReadJpegBatch` 批量读取接口，减少调用开销。
- 增加 `.dat` 文件句柄缓存，提高连续读取速度。
- 在 pack v2 中加入 CRC、相机编号、宽高、图像类型等元数据。
- 增加 idx 修复工具，应对异常断电或异常退出后的尾部恢复。
