# PackReadBenchmark 读取速度测试工具

## 1. 工具用途

`PackReadBenchmark` 用来测试后处理读取速度，重点回答两个问题：

1. 通过 `PackSdk` 从图片包读取 JPEG 字节有多快。
2. 和旧方案“直接读取单张 jpg 文件”相比差距多大。

注意：本工具测的是磁盘读取和 SDK 解包速度，不测 JPEG 像素解码速度。真正解码速度需要在 Qt/C# 里额外测试。

## 2. 构建

Windows：

```bat
msbuild code\PackSdk\PackSdk.vcxproj /p:Configuration=Release /p:Platform=x64
msbuild code\PackReadBenchmark\PackReadBenchmark.vcxproj /p:Configuration=Release /p:Platform=x64
```

Ubuntu：

```bash
cmake -S code/PackReadBenchmark -B build/packbench -DCMAKE_BUILD_TYPE=Release
cmake --build build/packbench
```

## 3. 参数

```text
-pack <dir>       pack 输出目录，目录下包含 Pack_*.idx / Pack_*.dat
-jpgDir <dir>     旧单图目录，递归读取 *.jpg / *.jpeg
-exportDir <dir>  从 pack 导出前 N 张 jpg，生成可比较的旧单图基准目录
-count N          只测试前 N 张；0 表示全部
-loops N          重复读取次数
-batchSize N      按每批 N 张统计 avgMsPerBatch
-random           随机顺序读取
-warmup           正式计时前先读一遍，减少冷缓存波动
```

## 4. 测 pack 顺序读取

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -pack "bin\硬盘测试\output\20260616_105858_665" ^
  -count 5000 ^
  -batchSize 100 ^
  -loops 3 ^
  -warmup
```

## 5. 测 pack 随机读取

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -pack "bin\硬盘测试\output\20260616_105858_665" ^
  -count 5000 ^
  -batchSize 100 ^
  -loops 3 ^
  -random ^
  -warmup
```

## 6. 生成旧单图基准目录

为了公平比较，旧单图目录应来自同一份 pack 数据：

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -pack "bin\硬盘测试\output\20260616_105858_665" ^
  -exportDir "bin\Release-x64\single_baseline_5000" ^
  -count 5000
```

## 7. 测旧单图读取

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -jpgDir "bin\Release-x64\single_baseline_5000" ^
  -count 5000 ^
  -batchSize 100 ^
  -loops 3 ^
  -warmup
```

随机读取旧单图：

```bat
bin\Release-x64\PackReadBenchmark.exe ^
  -jpgDir "bin\Release-x64\single_baseline_5000" ^
  -count 5000 ^
  -batchSize 100 ^
  -loops 3 ^
  -random ^
  -warmup
```

## 8. 输出指标含义

```text
Pack open/index seconds : 打开 pack 并建立索引耗时
framesRead              : 实际读取图片张数
bytesRead               : 实际读取字节数
elapsedSeconds          : 读取耗时
throughputFPS           : 每秒读取张数
throughputMBps          : 每秒读取 MB
avgMsPerFrame           : 每张平均耗时
avgMsPerBatch           : 每批平均耗时
```

如果 `batchSize=100`，`avgMsPerBatch=25` 表示平均每读取 100 张耗时约 25 ms。

## 9. 如何判断差距

建议至少测试四组：

1. pack 顺序读。
2. pack 随机读。
3. 旧单图顺序读。
4. 旧单图随机读。

判断时要注意：

- 热缓存下结果会非常快，不能完全代表冷盘。
- SSD、机械盘、移动硬盘差异很大。
- 图片大小不同会显著影响 MB/s 和 FPS。
- pack 的主要收益在采集写入阶段，读取端如果需要极限速度，还可以继续做 dat 文件句柄缓存和批量读取接口。

## 10. 当前实测样例

在当前机器上，用 `20260616_105858_665` 数据测试前 1000 张 pack 图片，热缓存下曾得到约：

```text
Pack SDK sequential read: 约 3800 FPS，约 2990 MB/s
Pack SDK random read    : 约 4100 FPS，约 3280 MB/s
```

这个数值只作为“工具能跑通”的参考，不应直接作为最终性能结论。正式结论应在目标工控机、目标硬盘、目标图片大小和目标批处理流程下重新测试。
