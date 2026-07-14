# SDK 迁移任务交接

## 当前目标

本阶段核心目标：让 SDK 主导 2D/3D 路面图片浏览、联动，以及病害绘制时的鼠标输入和临时显示。旧外层滚动条已经从 2D/3D 浏览外壳移除；SDK 的 `TiledGraphicsView` 直接接收滚轮、拖动、缩放和键盘事件。病害保存、数据库字段、深度计算等业务逻辑暂时继续复用旧函数，避免一次把业务链路全部打散。

样例工程：`D:\job\测试数据\二三维数据\k1114-1113下1`

本项目 2D/3D 路面图片按纵向拼接处理，联动主方向是 scene Y。

## 已完成

- SDK 增加底部锚点能力：
  - `currentBottomCenterScenePos()`：取当前视口底部中心点对应的 scene 坐标。
  - `currentBottomAnchor()`：返回底部所在图片名、图片序号、图片内像素坐标和 scene 坐标。
  - `scrollToImagePixel(int imageIndex, double pixelY, bool anchorBottom)`：把指定图片的指定像素 Y 放到视图底部。
  - `sigViewBottomAnchorChanged(TiledViewAnchor anchor)`：滚轮、拖动、缩放、程序定位后都会发出。
  - `sigUserViewBottomAnchorChanged(TiledViewAnchor anchor)`：只在用户滚轮、键盘、中键拖动或手动拖 SDK 滚动条时发出，程序调用 `scrollToImagePixel()` / `scrollToSceneY()` 不会触发这条业务联动信号。
- `hn2d3dPixBaseWidget` 已改成 SDK 浏览适配层：
  - 不再给 SDK 设置 `WA_TransparentForMouseEvents=true`，鼠标和键盘事件由 SDK 自己处理。
  - SDK 内置滚动条使用 `ScrollBarAsNeeded` 显示，旧外层滚动条已从 `hnContinuouslyBrowsePixWidget` 中移除，避免一个视图出现两个进度条。
  - 新增 `currentBottomEncoderMile()`，按底部锚点计算连续编码器里程。
  - 新增 `scrollBottomToEncoderMile(double)`，把目标里程换成图片序号和图片内像素 Y，再调用 SDK 精确定位。
  - SDK 底部锚点变化后，只同步旧坐标换算需要的少量字段，不再调用旧 `slot_updateCurrentScrollBar()` 触发半帧重绘。
  - 程序化定位期间用 `m_isProgrammaticSdkScroll` 延迟屏蔽业务联动信号，避免 `centerOn()` 后续滚动条事件被误判成用户滚动。
  - 普通底部锚点变化只发 `signal_sdkBottomEncoderMileChanged()`，用于顶部里程/桩号和兼容状态刷新；真正驱动 2D/3D 联动的是 `signal_sdkUserBottomEncoderMileChanged()`。
- `hnBrowsePixWidget` 增加 `setSkipLegacyImagePaint(bool)`：
  - SDK 接管 2D/3D 显示后，旧控件不再执行 `drawPicture()` / `drawAllPixOnLabel()`。
  - 这样 `ensureImageLoaded()`、`schedulePreloadImages()`、`updateImageMapBasedOnBottomFrameIdx()` 不会再因为 SDK 浏览刷新而被触发。
  - 旧控件只保留坐标换算和病害逻辑需要的状态字段，避免旧半帧拼图链路反向影响 SDK 滚动。
- 已清空旧外层滚动条浏览主链路：
  - `hnContinuouslyBrowsePixWidget` 不再包含 `m_scrollbar` 成员，不再创建、布局或隐藏外层滚动条。
  - `CustomScrollBar.h/.cpp/.ui` 已从 `hnContinuousBrowsePix` 工程中移除，并删除对应孤立文件。
  - `hnContinuouslyBrowsePixWidget::initSigSlot()` 不再连接任何旧滚动条信号，也不再发旧 `signal_scrollValueChanged()`。
  - `wheelEvent()`、2D/3D 外壳 `keyPressEvent()`、播放按钮不再通过滚动条翻页或自动播放。
  - `slot_updateScrollBarValue()`、`setCurrentScrollBarValue()`、`getCurrentScrollBarValue()`、`getMaxScrollBarValue()` 等旧接口已删除。
  - 病害列表跳转、区域跳转、工程树跳转、最近工程恢复位置都改为直接调用 SDK 的 `scrollBottomToEncoderMile()`。
- 2D 顶部里程/桩号显示已改为监听 SDK 底部连续里程，不再依赖隐藏的旧滚动条 value。
- 2D/3D 联动已改到底部锚点里程：
  - 2D 驱动 3D：`target3dMile = bottom2dMile - get2d3dMileDiff()`。
  - 3D 驱动 2D：`target2dMile = bottom3dMile + get2d3dMileDiff()`。
  - 主窗口现在只监听 `signal_sdkUserBottomEncoderMileChanged()` 做联动。程序定位另一侧视图时，另一侧只刷新自身显示状态，不会反向再拖回来源视图。
  - 使用 `m_isProgrammaticViewSync` 防止 2D/3D 连续信号互相回环。
- 景观图联动已改成最近图：
  - 项目层继续传当前 2D 连续里程。
  - `hnStreetCameraView::addImage()` 内部用 `qRound(distance / interval)` 选择最近的景观图，而不是只向下取整。
- 二三维里程矫正保留原按钮和 `Shift+C`：
  - `2D3D_DIFF = 2D编码器里程 - 3D编码器里程` 的定义不变。
  - 保存仍写入 2D 工程目录下 `2d3dDiffSetting.ini` 的 `CONFIG/2D3D_DIFF`。
  - SDK 接管浏览后，`GET_MILE` 左键点击已通过 SDK scene 坐标桥接，能按点击像素计算 double 里程。
- 病害绘制主路径已从旧 widget 坐标桥接改成 SDK scene 坐标：
  - 根因：SDK 接管浏览后，旧病害绘制还依赖 `screenToSingleImagePoint()`、`screenPointToBigImagePoint()`、`m_tmpPixImageWithoutDisease.height()` 和旧半帧拼图缓存；旧缓存不再刷新后，大框/小框会拿到错误坐标，表现为“点了但画不上”。
  - `hn2d3dPixBaseWidget` 现在直接拦截 SDK viewport 的鼠标事件，用 `currentAnchorAtScenePos()` 把 scene 点换成图片序号、图片内像素和 `pixImagePoint`。
  - 大框病害：左键起点、移动预览、左键终点，临时矩形直接画到 SDK scene 上，再调用 2D/3D 子类的 `sdkCommitBigFrameDisease()` 复用旧保存弹框和入库逻辑。
  - 小框 D 模式：左键起点、移动时按当前框实时计算被覆盖的小格子并用 SDK scene item 显示，左键终点后调用 `sdkCommitLittleFrameDisease()`。
  - 小框 B 模式：连续左键打点，SDK scene 上显示折线和被折线穿过的小格子；右键结束后弹出病害选择框。
  - 小格子生成仍复用各子类原来的 `createLittleFrameRects()`，这样 2D/3D 不同图像尺寸、道路宽度、高度和业务参数不用在基类里重复写一套。
  - 临时预览使用 `QGraphicsRectItem` / `QGraphicsPathItem` 加到 SDK scene，提交或取消时统一清理，不再依赖旧 `drawPicture()` 刷新。
  - 历史病害显示、删除、编辑、合并等还保留旧兼容层和必要事件转发；这部分涉及列表联动、选中状态、数据库记录，建议下一阶段再逐步替换成 SDK item。
- 2026-06-26 追加修复：
  - 病害仍画不出来的直接原因是 `sdkDiseaseOverlay` 被 `raise()` 到 SDK 上方后没有设置 `WA_TransparentForMouseEvents`，鼠标事件被覆盖层吃掉，SDK viewport 的 eventFilter 收不到点击。现在覆盖层只显示，不再拦截鼠标。
  - `Ctrl+滚轮` 后跳到第一张的原因是反向纵向拼接时 sceneY 可能为负，旧代码按 sceneY 计算里程并 clamp 到 0，随后联动把视图拉回 0。现在里程计算改为按图片名和图片内像素 Y 计算。
  - 2D/3D SDK 图片序列改为 `LayoutOrientation::VerticalReverse`，满足“小里程在屏幕下面，大里程在屏幕上面”的浏览方向。
  - SDK 新增按图片名定位接口 `scrollToImagePixel(QString imageName, double pixelY, bool anchorBottom)`，避免反向拼接后 SDK 内部 item 下标和项目图片序号不一致。
- 2026-06-26 再次追加修复：
  - 大框和小框 D 模式的 SDK 绘制交互改成“左键按下开始、拖动预览、左键释放提交”。之前 SDK 分支是“第二次左键提交”，和旧使用习惯不一致，容易让旧/新事件链混在一起。
  - `Ctrl+滚轮` 缩放时不再发 `sigUserViewBottomAnchorChanged()`，缩放只更新 SDK 自己的锚点状态，不再触发 2D/3D 用户联动，避免缩放被当成浏览跳转。
  - SDK viewport 普通鼠标移动会把坐标翻译给旧 `mouseMoveEvent()`，恢复状态栏里的图片底部桩号、里程、路面标准、等级、单张图片坐标和图片名称。
  - SDK 滚轮后用一次延迟的鼠标移动事件刷新旧状态栏，避免鼠标不动时状态栏停在旧图信息。
- 2026-06-26 本轮针对用户反馈继续修复：
  - 病害“松开鼠标会弹框但图上看不到”的直接原因是提交成功后仍把 SDK 临时图元清掉了。现在提交成功后会把临时图元转入 `m_sdkCommittedDiseaseItems`，只有重新加载图片序列时才清空，保证新增大框/小框提交后能留在 SDK scene 上。
  - SDK 视图和 viewport 都打开 mouse tracking，鼠标按下时同时给旧控件和 SDK 视图设置焦点，避免切到 SDK 浏览后旧快捷键没有事件入口。
  - SDK viewport 收到键盘事件时，绘制/删除等旧业务模式转发给旧 `keyPressEvent()`；普通浏览键盘则转发给 `TiledGraphicsView`，这样 B/D/Delete 等病害快捷键和 WASD/方向键浏览可以分开处理。
  - SDK 底部锚点变化时也会补发一次旧 `mouseMoveEvent()`，不是只等鼠标真的移动。这样滚轮、键盘、程序联动后，状态栏的图片底部桩号、里程、标准、等级和图片名称会按当前视图中心刷新。
  - `Ctrl+滚轮` 缩放继续保留缩放前的底部图片名和图片内 pixelY，再用 `scrollToImagePixel(imageName, pixelY, true)` 放回底部锚点，避免缩放后被拉到第一张。
- Debug x64 编译验证已通过：
  - `TunnelViewerSDK`
  - `hnApplication`
  - `hnRoadDataProcess`
  - 生成目标：`bin\Debug-x64\hnRoadDataProcess.exe`

## 核心设计

联动主状态不是旧滚动条整数 value，而是“底部锚点对应的连续编码器里程”。

换算规则：

```text
frameIndex = 项目图片序号，从 1 开始
pixelY = 图片内 Y 坐标，0 在图顶，imageHeight 在图底
imageBeginMile = (frameIndex - 1) * imageDistanceMeters
encoderMile = imageBeginMile + (imageHeight - pixelY) / imageHeight * imageDistanceMeters
```

反向定位：

```text
imageIndex = floor(encoderMile / imageDistanceMeters)
imageName = m_pixNameMap[imageIndex + 1]
offsetInImage = encoderMile - imageIndex * imageDistanceMeters
pixelY = imageHeight - offsetInImage / imageDistanceMeters * imageHeight
SDK.scrollToImagePixel(imageName, pixelY, AnchorBottom)
```

这样 2D 和 3D 都可以停在任意像素位置，不再被半张图或整张图的整数进度条卡住，并且单张图内部保持“底部小里程、顶部大里程”。

## 距离来源

- 2D 路面图单张距离：`hn2DProject::_RoadImgDis`，来源是 `Setting.ini / Parm / RoadDis`。
- 3D 路面图单张距离：`hn3dPixWidget::m_3dImageDistanceMeters`，默认 `8.0` 米，后续可以从文件读取后覆盖。
- 左景观图单张距离：`hn2DProject::_StreetImgDis`，来源是 `StreetDis`。
- 右景观图单张距离：`hn2DProject::_StreeRightImgDis`，来源是 `StreetDis2`；如果为 0，沿用左景观间距。

旧代码里 `projectSetInfo.dRoadLength` 或写死的 `8/2` 不再作为 2D/3D 精确联动主依据。

## 当前注意点

- 当前可见滚动条应只来自 SDK 的 `TiledGraphicsView`。如果界面再次出现第二个滚动条，优先检查是否有新代码重新包了一层外部滚动控件。
- 旧外层滚动条已经不再驱动 2D/3D 浏览，也不再参与 2D/3D 联动。
- `ensureImageLoaded()` 仍保留给未迁移到 SDK 的旧浏览场景使用，但在当前 2D/3D SDK 显示模式下不应再被普通浏览触发。
- 如果视图仍出现“自己滚”，优先检查是否又把 `sigViewBottomAnchorChanged()` / `signal_sdkBottomEncoderMileChanged()` 接回了 2D/3D 联动链路。联动必须只监听用户信号 `sigUserViewBottomAnchorChanged()` / `signal_sdkUserBottomEncoderMileChanged()`。
- 新增病害绘制主路径已经走 SDK scene 坐标，但保存和业务计算仍复用旧函数。也就是说“怎么点、怎么预览”已经切到 SDK，“保存成什么病害、怎么写库、怎么算深度”还沿用旧业务代码。
- 历史病害显示和编辑删除如果在缩放或横向拖动后出现偏差，下一步优先把数据库病害渲染迁移为 SDK scene item；不要再回到旧滚动条或旧拼图缓存。
- 如果样例工程验证时发现上下方向相反，优先检查 `LayoutOrientation::VerticalReverse`、`m_pixNameMap` 图片排序、以及 `pixelY = imageHeight - offset * imageHeight / imageDistanceMeters` 这条反向定位公式，不要改 `2D3D_DIFF` 的定义。
- 景观图仍是单张刷新，不做连续滚动；它只需要跟随当前里程切到最近图。

## 重构建议和执行顺序

当前代码最大问题不是 SDK 能不能显示，而是 `hn2d3dPixBaseWidget` 同时承担了 SDK 视图、旧坐标兼容、病害交互、临时绘制、状态栏刷新和业务保存桥接。后续不要再继续往这个类里堆补丁，按下面顺序逐项拆开。

1. SDK scene 图元层：把临时/已提交病害图元管理从 `hn2d3dPixBaseWidget` 抽出，基类只提交矩形或路径，不直接保存 `QGraphicsItem`。
2. 状态栏刷新层：不要再伪造 `mouseMoveEvent()` 刷新状态栏，改成明确的 `pixImagePoint -> statusInfo` 接口。
3. 病害交互控制器：把大框、小框 D、小框 B、右键结束、B/D/Delete 快捷键状态从视图基类里抽到 `DiseaseAnnotationController` 一类的对象里。
4. 病害业务提交接口：视图层只产出 `DiseaseDraft`，2D/3D 子类负责把 draft 转旧业务结构，保存、弹框、数据库继续留在业务层。
5. 旧继承链整理：最后再处理 `hn2d3dPixBaseWidget` 继承的 `drawDiseases/rectAlgorithm/depthCaculate/mergeDisease/lineAlgorithm` 等能力，能组合就组合，不能一次删的先隔离为明确服务对象。

2026-06-26 已开始执行第 1 项：

- 新增 `hnSdkDiseaseGraphicsLayer`，专门管理 SDK scene 上的临时图元和提交后保留图元。
- `hn2d3dPixBaseWidget` 不再直接持有 `m_sdkTemporaryDiseaseItems/m_sdkCommittedDiseaseItems`，也不再直接创建和删除 `QGraphicsRectItem/QGraphicsPathItem`。
- 当前行为保持不变：绘制中是 temporary，弹框接受后 commit，取消或失败后 discard，重新加载图片时 clearAll。
- 新文件注释先使用 ASCII，避免老 MSVC/Qt 工程按 936 代码页解析 UTF-8 注释时把头文件读坏。

## 已执行验证

```powershell
MSBuild hnRoadDataProcess.sln /t:TunnelViewerSDK /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
```

结果：三个目标均编译通过。仍有一些旧代码和第三方库 warning，本次没有新增阻断 Debug 编译的 error。

2026-06-25 追加验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:TunnelViewerSDK /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:2 /v:minimal
```

结果：

- `TunnelViewerSDK` 编译通过。
- `hnApplication` 编译通过。
- `hnRoadDataProcess.exe` 已完成链接并生成到 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 最后 `BSCMAKE` 报 `BK1506: 无法打开文件 x64\Debug\moc_hnRoadDataProcess.sbr: Permission denied`，这是浏览信息 `.sbr` 文件占用/权限问题，不是本次 SDK 联动代码的编译或链接错误。

2026-06-25 旧滚动条逻辑清理验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:hnContinuousBrowsePix /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

结果：三个目标均编译通过，主程序生成到 `bin\Debug-x64\hnRoadDataProcess.exe`。仍有旧工程和第三方库 warning，本次没有新增阻断错误。

2026-06-25 外层旧滚动条移除验证：

```powershell
rg -n "m_scrollbar|getCurrentScrollBarValue\(|getMaxScrollBarValue\(|setCurrentScrollBarValue\(|slot_setScrollBarMaxValue|slot_updateScrollBarValue|signal_scrollValueChanged|signal_scrollBarLeftouseRelease" hnContinuousBrowsePix hnApplication hnRoadDataProcess SDK
MSBuild hnRoadDataProcess.sln /t:hnContinuousBrowsePix /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

结果：

- 代码目录中已无 `m_scrollbar`、旧滚动条 get/set 接口、旧滚动条信号和旧滚动条槽。
- `CustomScrollBar.h/.cpp/.ui` 已从工程移除并删除；当前仅文档中保留删除记录。
- `hnContinuousBrowsePix`、`hnApplication`、`hnRoadDataProcess` 三个 Debug x64 目标均编译通过，主程序生成到 `bin\Debug-x64\hnRoadDataProcess.exe`。

2026-06-26 病害绘制 SDK 主路径验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

结果：

- `hnApplication` 编译通过。
- `hnRoadDataProcess` 编译通过，主程序生成到 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 本次修复重点是 SDK 接管浏览后的病害绘制主路径：SDK scene 负责鼠标点位、临时大框、小框格子和折线预览；旧业务函数只保留为提交保存入口。

2026-06-26 病害、缩放和里程方向修复验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

结果：

- `hnApplication` 编译通过。
- `hnRoadDataProcess` 编译通过，主程序生成到 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 本次修复重点是：覆盖层不再拦截鼠标；`Ctrl+滚轮` 后里程不再通过 sceneY clamp 到 0；2D/3D 按反向纵向拼接浏览。

2026-06-26 事件流和状态栏修复验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

结果：

- `hnApplication` 编译通过。
- `hnRoadDataProcess` 编译通过，主程序生成到 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 本次修复重点是：大框/小框 D 按释放提交；`Ctrl+滚轮` 不再发用户联动信号；状态栏信息重新走旧 `generateStatusInfo()`。

2026-06-26 本轮追加验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
```

结果：

- `TunnelViewerSDK` 和 `hnApplication` 编译通过。
- 本次修复重点是：新增病害提交后保留 SDK scene 图元；SDK 事件焦点和键盘转发增强；SDK 锚点变化后主动刷新旧状态栏。

2026-06-26 重构第 1 项验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
```

结果：

- `hnApplication` 编译通过。
- 本次只做结构收敛，不改变病害绘制交互和保存流程。
- 仍有旧工程和第三方库 warning，没有新增阻断错误。

2026-06-26 重构第 2 项验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
```

结果：

- `hnApplication` 编译通过。
- 状态栏刷新不再靠伪造 `QMouseEvent` 去触发旧 `mouseMoveEvent()`。
- 新增 `refreshStatusInfoFromSdkViewportPoint()` 和 `sdkStatusInfoFromWidgetPoint()`，SDK 只负责给出视口点，2D/3D 子类继续复用自己的 `generateStatusInfo()`。
- 这一步是为了先把“浏览事件”和“状态栏文字”拆开，后面抽病害交互控制器时不会继续被鼠标事件副作用牵着走。

2026-06-29 SDK OpenGL 残影问题处理：

现象：

- SDK 视图区域有时像截屏一样残留整个软件窗口、按钮、菜单和其它控件内容。
- 这类问题通常不是业务图像数据错了，而是 `QGraphicsView + QOpenGLWidget viewport` 在复杂 QWidget 界面、远程桌面或部分显卡驱动下的合成/刷新问题。

处理：

- `TiledGraphicsView` 不再默认创建 `QOpenGLWidget`。
- SDK 新增 `RenderBackend_Raster` / `RenderBackend_OpenGL` 和 `setRenderBackend()`。
- 默认使用普通 QWidget viewport，优先保证显示稳定。
- OpenGL 仍保留为可选后端，并补了 `NoPartialUpdate`、不透明绘制等属性，后面确实需要硬件加速时可以显式打开。

验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:TunnelViewerSDK /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
```

结果：

- `TunnelViewerSDK` 编译通过。
- `hnApplication` 编译通过。
- 仍有旧工程和第三方库 warning，没有新增阻断错误。

2026-06-30 SDK 状态栏坐标与工程信息修复：

问题：

- 状态栏仍通过 SDK 点位反推旧 widget 坐标，再调用旧 `generateStatusInfo()`。这条链路依赖旧半帧拼图状态，SDK 单图模式下滚动、鼠标移动、图片名、单张图片坐标、拼接图片坐标和底部桩号容易不同步。
- SDK 命中的图片名有时是文件名或 baseName，而旧工程映射常用完整路径，导致 `m_reversePixNameMap` / 里程表查找失败后状态栏显示空路面标准、空路面等级或 0 里程。
- 鼠标移动时如果进入病害绘制逻辑，状态栏刷新可能被跳过；滚轮、键盘、滚动条变化时也缺少统一的 SDK anchor 状态文本生成入口。

处理：

- `hn2d3dPixBaseWidget` 新增 `SdkStatusContext`，统一从 SDK viewport 点或 `TiledViewAnchor` 生成：工程图片名、图片序号、SDK 视口坐标、scene 坐标、单张图片像素坐标、整路拼接坐标和连续编码器里程。
- 新增 `resolveSdkImageName()`，支持 SDK 图片名、文件名、baseName 和工程完整路径之间互相匹配；`sdkScenePointToPixPoint()`、`sdkPixPointToEncoderMile()`、`sdkPixPointToBigImagePoint()` 都先解析到工程图片名。
- 2D/3D 子类新增 `sdkStatusInfoFromContext()`，SDK 状态栏不再绕回旧 widget 坐标。2D 从 `m_pixNameHnMileMap` 和当前工程设置生成桩号、里程、路面标准、路面材质、路面等级和病害模式；3D 保留 23D 与单 3D 的底部里程差异，单 3D 继续用 `get3DProject()->getMileByImage()`。
- 鼠标移动进入 SDK viewport 后先刷新状态栏，再处理病害绘制；SDK 底部 anchor 变化继续同步旧浏览状态并刷新状态栏。
- 状态栏坐标语义明确为：`屏幕坐标` 是 SDK viewport 坐标；`拼接图片坐标` 是整条路连续拼接坐标；`单张图片坐标` 是当前命中图片内像素坐标；`图片名称` 直接来自 SDK 命中图片解析后的工程图片名。
- 路面标准和路面等级增加工程设置兜底，避免里程记录字段缺失时状态栏仍为空。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:TunnelViewerSDK /p:Configuration=Debug /p:Platform=x64 /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- `TunnelViewerSDK` 编译通过。
- `hnApplication` 编译通过。
- `hnRoadDataProcess` 编译通过。
- 仍有旧工程和第三方库 warning，主要是 OpenCV 编码、旧头文件格式、QXlsx PDB 和既有未引用参数 warning，没有新增阻断错误。

2026-06-30 状态栏字段和翻转坐标继续修正：

问题：

- 状态栏仍显示 `屏幕坐标`，用户不需要该字段。
- `单张图片坐标` 应显示原始图坐标；开启水平/垂直翻转后，SDK 命中的显示坐标没有换回原始图坐标，边界位置还可能出现负值。
- 鼠标移动和滚轮滚动会分别走 SDK 鼠标点和 SDK 底部 anchor 两条刷新路径；在病害模式或事件转发场景下，旧 `mouseMoveEvent()` 还可能继续发旧状态栏文本，造成同一视图里字段格式和图片名称表现不一致。
- 旧 2D 状态栏会把图片名显示成父目录加文件名，用户只需要图片文件名。

处理：

- 2D/3D 的旧状态文本和 SDK 状态文本都移除 `屏幕坐标` 字段，并同步调整 `.arg()` 参数顺序。
- SDK 状态上下文生成时先把命中的显示坐标夹取到 `0..width-1`、`0..height-1`，再按 `m_isHMirrored` / `m_isVMirrored` 换回原始图坐标，用于 `单张图片坐标`、`拼接图片坐标` 和状态里程计算。
- 旧非 SDK `generateStatusInfo()` 的单张图坐标也做同样的夹取和翻转换算，避免旧路径出现负坐标。
- SDK 视图存在时，2D/3D 旧 `mouseMoveEvent()` 不再发 `generateStatusInfo()` 的旧状态文本，防止覆盖 SDK 状态栏。
- 2D 图片名称统一改为 `QFileInfo(...).fileName()`，不再显示父目录或完整路径；SDK 2D/3D 路径也继续只显示文件名。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- `hnApplication` 编译通过。
- `hnRoadDataProcess` 编译通过，生成 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 仍有旧工程和第三方库 warning，主要是 OpenCV 编码、旧头文件格式和 QXlsx PDB warning，没有新增阻断错误。

2026-06-29 SDK 快捷键和病害绘制修复：

问题：

- `W/A/S/D` 和方向键在部分模式下不生效，因为 `hn2d3dPixBaseWidget::eventFilter()` 在 `ADD_MODE/GET_MILE` 等业务模式下把所有按键都转给旧控件，SDK 的 `TiledGraphicsView::keyPressEvent()` 收不到导航键。
- 病害绘制点位不稳定，SDK 坐标转换仍有旧横向/图片名兜底逻辑，纵向反向拼接下容易取不到正确图片内坐标。
- 临时病害图元虽然放进了 SDK scene，但旧 QWidget 覆盖层仍在最上面，绘制过程中可能挡住临时框、小框格子或线段。

处理：

- 导航键优先交给 SDK：`W/A/S/D`、方向键、空格不再被旧业务模式统一吃掉。
- `D` 键保留一个业务例外：在小框添加模式下仍交给旧逻辑切换小框绘制方式，其它时候作为 SDK 向右浏览。
- `TiledGraphicsView::GlobalSceneToMap()` 改为按 `TunnelSectionItem::sceneBoundingRect()` 和 item 本地坐标判断，不再依赖 `scene->items()` 命中顺序。
- `mapToGlobalScene()` 找不到图片名时不再尝试解析文件名里程，改成按首尾图片边界兜底。
- SDK 临时病害图元设置为不接收鼠标、不参与选择，避免盖住图片后影响后续取点。
- 绘制过程中隐藏旧 QWidget 覆盖层，临时病害显示以 SDK scene 图元为准；绘制结束后再恢复旧覆盖层兼容历史病害显示。

验证：

```powershell
MSBuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
MSBuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

结果：

- `hnApplication` 编译通过。
- `hnRoadDataProcess` 编译通过，生成 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 仍有旧工程和第三方库 warning，没有新增阻断错误。

2026-06-29 SDK 初始铺满、滚轮鼠标复位和人工病害拖动修复：

问题：

- 2D/3D 图片序列当前按纵向拼接显示，刚导入时应该横向完整铺满；但 `getFitScale()` 没有预留纵向滚动条宽度，首次 fit 后可能被滚动条挤出横向缺口或横向滚动条。
- 普通滚轮浏览后鼠标会被程序自动移动到上方或下方，这是旧的小框跨页续画逻辑 `scheduleMoveCursorToBestContinuePointAfterBrowse()` 间接触发 `QCursor::setPos()` 导致的；SDK 接管浏览后这条自动复位不应再由滚轮触发。
- 人工大框/小框 D 病害仍表现为拖动过程中没有任何框，松开才弹出病害选择框。根因不是保存逻辑，而是绘制交互仍混着两套入口：一套是“按下开始、拖动预览、释放提交”，另一套是旧的“第二次左键按下提交”；同时旧 QWidget 覆盖层仍可能盖住 SDK scene 临时图元。

处理：

- `TiledGraphicsView::getFitScale()` 改为纵向拼接按可用宽度适配；当长图必然需要纵向滚动条时，先扣掉 `verticalScrollBar()->sizeHint().width()`，保证刚导入时图片横向完整铺满。
- `scheduleMoveCursorToBestContinuePointAfterBrowse()` 现在直接返回，不再调度 `moveCursorToBestContinuePointAfterBrowse()`，滚轮浏览不会再移动物理鼠标。保留旧函数体是为了后续如果专门重做“小框跨页续画”时还有参考，但当前浏览链路不再调用。
- `beginSdkDiseaseAt()` 在按下开始绘制时立即调用 `updateSdkDiseaseOverlay()`，绘制期间隐藏旧覆盖层，让 SDK scene 上的临时矩形/小框格子可见。
- 大框和小框 D 模式的 `handleSdkDiseaseMousePress()` 只负责开始绘制，不再在已经绘制中时提交；提交统一放到 `handleSdkDiseaseMouseRelease()`。
- `handleSdkDiseaseMouseMove()` 更新终点后也刷新覆盖层状态并重建 SDK 临时图元，拖动过程中应能看到临时框。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess.sln /t:TunnelViewerSDK /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

结果：

- `TunnelViewerSDK` 编译通过。
- `hnApplication` 编译通过。
- `hnRoadDataProcess` 编译通过，生成 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 仍有旧工程和第三方库 warning，主要是 OpenCV/Qt/QXlsx/旧头文件相关 warning，没有新增阻断错误。

## 下一步建议

1. 继续执行重构第 3 项：抽病害交互控制器，先接管大框和小框 D，再接管小框 B。
2. 再执行重构第 4 项：把病害保存、弹框、属性写入收敛成清晰的业务提交接口。
3. 最后执行重构第 5 项：整理旧继承链，逐步弱化 `hnBrowsePixWidget` / `hnContinuouslyBrowsePixWidget` 对新 SDK 浏览路径的影响。
4. 每完成一项都先跑 `hnApplication` Debug 编译，再用样例工程验证病害绘制和 2D/3D 联动。

2026-06-29 SDK 状态栏、横向滚动条和人工病害提交修复：

问题：

- 状态栏里的图片底部桩号、里程、路面标准等信息不再跟随浏览变化。直接原因是 SDK 底部锚点变化后仍用“视口中心点”去反推旧控件状态，和用户关心的图片底部信息不一致。
- Ctrl+滚轮缩小时 SDK 内部横向滚动条又出现，和外层窗口已有横向滚动条叠在一起，形成同一窗口两条横向进度条。
- 人工病害拖拽仍不稳定，甚至释放后不再弹出病害窗口。复查发现 SDK viewport 的事件处理末尾仍默认回落到 `hnBrowsePixWidget::eventFilter()`，导致同一个滚轮/鼠标事件同时进入 SDK 和旧浏览链路；同时释放点只要转换失败就直接 `return false`，小框 D 的 `m_tmpLittleFrameDiseaseRects` 没有重建时 `littleFrameProcess()` 会直接失败。

处理：

- 新增 `refreshStatusInfoFromSdkAnchor()`，`sigViewBottomAnchorChanged()` / `sigUserViewBottomAnchorChanged()` 现在直接按 SDK 底部锚点换算旧状态栏点位，状态栏刷新锚定图片底部。
- 2D/3D 嵌入层的 SDK 视图横向滚动条改为 `Qt::ScrollBarAlwaysOff`，避免 Ctrl 缩放后内部横条和外层横条重复显示。
- SDK viewport 的 `eventFilter()` 不再在末尾默认回落旧 `hnBrowsePixWidget::eventFilter()`；普通滚轮交回 SDK 自己处理，绘制病害时滚轮直接吃掉，避免旧浏览链路移动鼠标或改变绘制状态。
- `handleSdkDiseaseMouseMove()` 在绘制中遇到无效映射点时直接消费事件，不再转给旧鼠标移动逻辑。
- `handleSdkDiseaseMouseRelease()` 释放阶段如果当前释放点转换成功就更新终点；如果转换失败但已有最后一次有效拖动点，则继续用该点提交。大框提交前刷新临时图元，小框 D 提交前强制重建预览和临时格子，避免 `littleFrameProcess()` 因临时格子为空而静默失败。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /v:minimal
```

结果：

- `hnApplication` 编译通过。
- `hnRoadDataProcess` 编译通过，生成 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 仍有旧工程和第三方库 warning，主要是 OpenCV 编码、QXlsx PDB、旧头文件格式 warning，没有新增阻断错误。

2026-06-29 SDK 图片横向铺满黑边修复：

问题：

- 2D/3D 纵向拼接视图右侧仍有十几个像素黑边，图片横向没有真正铺满。
- 直接原因在 `TiledGraphicsView::getFitScale()`：代码使用的是 `viewport()->size()`，而 Qt 的 `viewport()` 在滚动条已经显示时已经扣掉了滚动条宽度；此前又手动减了一次 `verticalScrollBar()->sizeHint().width()`，导致纵向视图宽度少算约一个滚动条宽度。

处理：

- `getFitScale()` 保留“滚动条尚未显示但即将显示时预扣一次”的逻辑。
- 如果滚动条已经可见，不再重复扣宽度或高度，避免右侧黑边。
- 横向拼接分支同样改成只在水平滚动条尚未显示时预扣高度，防止同类重复扣减。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess.sln /t:TunnelViewerSDK /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m:4 /v:minimal
```

结果：

- `TunnelViewerSDK` 编译通过。
- `hnApplication` 编译通过。
- 仍有旧工程和第三方库 warning，没有新增阻断错误。

2026-06-30 SDK 状态栏滚轮/W/S 与鼠标移动一致性修复：

问题：

- 鼠标移动时状态栏按 SDK viewport 下的当前鼠标点刷新。
- 滚轮、W/S/方向键浏览时状态栏按 SDK 底部锚点刷新。
- 这两条路径的参考点不同，所以同一视图下图片名称、单张图坐标、拼接坐标、桩号/里程会出现“鼠标移动一套，滚动/按键又一套”的不一致。

处理：

- 新增 `refreshStatusInfoFromSdkCurrentMouseOrAnchor()`：如果当前鼠标仍在 SDK 视口内，状态栏统一按鼠标所在图片点刷新；如果鼠标不在视口内，才回退到底部锚点刷新。
- `sigViewBottomAnchorChanged()`、`sigUserViewBottomAnchorChanged()` 和滚轮后的延迟刷新都改为调用该统一入口。
- 保留底部锚点里程信号用于 2D/3D 联动和旧浏览状态同步；只把状态栏文本刷新改成优先跟随鼠标点，避免和鼠标移动显示不一致。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- hnApplication 编译通过，依赖的 TunnelViewerSDK 同步编译通过。
- hnRoadDataProcess 编译通过，生成 in\Debug-x64\hnRoadDataProcess.exe。
- 仍有旧工程和第三方库 warning，主要是 OpenCV/Qt/QXlsx/旧头文件相关 warning，没有新增阻断错误。

2026-06-30 景观图按里程联动与 2D/3D 校准修复：

问题：

- 景观图不应该像路面 2D/3D 一样连续拼接显示，但它仍需要通过滚轮、方向键或进度条式跳转按图片间隔翻页，并和 2D/3D 当前里程联动。
- 状态栏里的“景观图片”此前跟随鼠标命中点变化，不符合业务含义；景观图应按路面界面当前浏览里程和景观图间隔取图。
- 旧的 2D/3D 里程校准入口虽然存在，但 SDK 浏览模式下需要确保选点里程来自 SDK 命中点，且不能沿用上一次残留选点。
- 景观图主动翻页时只同步了 2D，23D 项目下没有按 `2D3D_DIFF` 同步 3D。

处理：

- 保留景观图单张翻页逻辑，`hnStreetCameraView` 的滚轮/方向键仍按 `m_pictureInterval` 前后翻页；`hnStreetWidget::updateViewImage()` 继续按传入距离除以图片间隔定位景观图片。
- 2D/3D 状态栏中的“景观图片”改为按当前视图底部浏览里程取景观图文件名，不再跟随鼠标点变化；3D 在 23D 项目下先加回 `get2d3dMileDiff()`，换算到等价 2D 里程后再取景观图。
- 进入“二三维里程校准”模式时清空 2D/3D 上一次选点和编码器里程；`drawDiseases::claerSelectPoint()` 同步重置 `m_encoderMile`，避免只点一个视图时沿用旧值。
- Shift+C 校准时要求 2D/3D 都已经选中有效里程，按 `2D里程 - 3D里程` 写入 `2D3D_DIFF`，保存后立即把 3D 滚动到 `2D里程 - diff`，并同步景观图。
- 景观图翻页信号现在按传出的行驶距离同步 2D，并在可用连续浏览联动时同步 3D 到 `2D距离 - 2D3D_DIFF`。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- hnApplication 编译通过。
- hnRoadDataProcess 编译通过，生成 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 仍有旧工程和第三方库 warning，主要是 OpenCV 编码页、旧头文件和 QXlsx PDB warning，没有新增阻断错误。

2026-06-30 景观状态栏、景观翻页联动和 2D/3D 校准参考线修复：

问题：

- 景观窗口已经翻到下一张图片，但状态栏“景观图片”仍显示旧文件名。原因是状态栏用 2D/3D 当前底部里程重新推算景观图，而不是读取景观控件实际正在显示的图片。
- 景观翻页联动在部分路径下会被断开：鼠标进入 2D/3D 视图时曾主动 `disconnect` 景观翻页信号，最近工程恢复流程里也残留了一处 `disconnect`。
- 2D/3D 里程校准点击后黄色参考线不贴合鼠标点，且 3D 点击后要再滚动一下才显示。原因是 SDK 模式下参考线仍通过旧的整幅图覆盖层绘制，坐标和刷新时机都不稳定。
- Shift+C 保存差值后只写入了 `2D3D_DIFF`，没有强制刷新 SDK 视图、状态栏和景观联动，所以用户看到“成功”但画面不动。

处理：

- `hnStreetCameraView` 增加当前景观图片路径查询；`hnStreetWidget` 汇总当前显示侧的景观图片路径。
- 2D/3D 状态栏优先使用主窗口写入的“当前景观窗口实际图片名”，只有没有该值时才按里程回退推算。
- 景观翻页信号保持常连；从 2D/3D 程序同步景观时使用 `QSignalBlocker` 屏蔽回调，避免循环联动。
- 最近工程恢复时不再断开景观翻页信号，并在恢复 2D 位置后同步景观图。
- SDK `GET_MILE` 校准参考线改为直接画到 SDK scene 临时图元上，点击后立即刷新，不再依赖旧覆盖层和下一次滚动。
- `scrollBottomToEncoderMile()` 和 Shift+C 校准成功后都会主动刷新 SDK 视图状态、状态栏和临时图元。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' .\hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- hnApplication 编译通过。
- hnRoadDataProcess 编译通过，生成 `bin\Debug-x64\hnRoadDataProcess.exe`。
- 仍有旧工程和第三方库 warning，主要是 OpenCV 编码页、旧头文件和 QXlsx PDB warning，没有新增阻断错误。

2026-07-02 SDK 正式病害图层迁移与选中/居中修复：

问题：
- SDK 单图模式下，临时绘制框已经在 SDK scene 中，但正式病害仍由旧 overlay 绘制，导致绘制完成后可能出现“临时框 + 正式框”两个框。
- 右键选中仍混用旧坐标和旧列表状态，可能一次选中多个病害。
- Ctrl 缩放时旧 overlay 病害框不跟随 SDK scene 缩放和平移。
- 病害列表双击定位只滚动到里程附近，没有把选中病害中心放到视图中央。

处理：
- 新增 `hnSdkDiseaseGraphicsLayer` 正式病害 item 管理能力，按病害 ID 管理 SDK scene item，支持清空、添加 path、单选高亮、scene 命中和按 ID 获取包围盒。
- `hn2d3dPixBaseWidget` 在 SDK 模式下不再显示旧 `SdkDiseaseOverlayWidget` 的正式病害层，正式病害统一由 SDK scene item 绘制；滚动锚点变化时同步重建当前窗口病害 item。
- 大框、小框和线状小框提交成功后清空临时 item，并从病害数据重建正式 item，避免绘制完成后保留临时虚线/临时框。
- 2D/3D 分别把 `vec2dRect`、`vec3dRect` 转换为当前视图的大图坐标，再映射到 SDK scene；线状病害按折线点映射到 SDK scene。
- SDK 视图右键优先在 scene item 上命中病害，只设置一个 `selectedDiseaseId`，同步旧选择集合和病害列表选择信号。
- 病害列表选中/双击后延迟调用 `centerSdkDiseaseInView()`，根据 SDK item 的 scene 包围盒中心执行 `centerOn()`，不改变当前缩放比例。
- `hnBrowsePixWidget::setSelectedDiseaseId()` 改为 virtual，允许 SDK 基类同步更新 scene 选中态。

验证：
```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe' hnContinuousBrowsePix\hnContinuousBrowsePix.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：
- `hnContinuousBrowsePix` Debug x64 完整构建通过。
- `hnApplication` Debug x64 完整构建通过，并确认 `hnSdkDiseaseGraphicsLayer.cpp/.h` 已加入 vcxproj 和 filters。
- `hnRoadDataProcess` Debug x64 完整构建生成 `bin\Debug-x64\hnRoadDataProcess.exe`，项目自身 tlog 无 `unsuccessfulbuild` 标记。
- 仍需用实际工程手工验证：右键单选、绘制完成单框、列表双击居中、Ctrl 缩放时病害框跟随图像。

2026-07-02 SDK 病害选择唯一键、详情样式和刷新补丁：

问题：

- SDK 正式病害图层上一版只按 `nID` 管理选中态，不足以区分不同病害表中相同 ID 的记录。
- 右键选中后只高亮框，旧逻辑中的详细文字、字体变大和注释引线没有在 SDK scene 图层恢复。
- 重新导入工程并选择新工程但尚未双击工程列表时，2D/3D SDK 视图仍可能显示上一工程画面。
- 病害列表右键删除后，数据库和列表已变化，但 2D/3D 当前 scene 图层没有立即刷新。

处理：

- `hnSdkDiseaseGraphicsLayer` 继续保持通用，不写入数据库表语义；它只接收应用层传入的不透明 `QString diseaseKey`。
- `hn2d3dPixBaseWidget` 在应用层生成 `表名#ID` 作为 SDK 病害 key，并用该 key 做右键命中、选中高亮、列表双击居中和 scene 包围盒查询。
- SDK 正式病害 item 恢复选中详情样式：选中病害使用详细 label、较大字体、黄色注释引线；未选中病害保持短 label。
- 病害列表双击信号从只传 `int id` 改为传完整 `hnRoadDiseaseInfo`，避免列表跳转/居中时丢失表名。
- 监听 `hnDiseaseService::diseaseDeleted`，删除病害后立即刷新 2D/3D SDK 视图和旧绘制路径。
- `openProjectSlot()`、`openLastProjectSlot()` 关闭旧工程时同时清理 2D SDK、3D SDK 和景观视图，避免新工程选择阶段残留上一工程画面。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：

- `hnApplication` Debug x64 编译通过。
- `hnRoadDataProcess` Debug x64 编译通过，生成主程序。
- 仍有既有 warning，主要是 OpenCV/SDK 头文件编码页、旧头文件 typedef、QXlsx PDB 等，没有新增阻断错误。

2026-07-02 SDK 病害删除即时刷新与标注引线调整：

问题：

- 病害列表右键删除、删除模式点击删除后，数据库和列表已更新，但 SDK scene 上的正式病害 item 没有立即重建，需要滚动视图后才消失。
- SDK 病害文字标注上一版固定贴在框右上角，和旧 overlay 中“文字在框外侧 + 引线连接”的显示习惯不一致。

处理：

- `hn2d3dPixBaseWidget` 增加 `refreshSdkDiseaseLayer()`：SDK 模式下重建正式病害 item 并刷新 scene/viewport；非 SDK 模式下退化为普通 `update()`，不破坏旧路径。
- `hnDiseaseService::diseaseDeleted` 主窗口连接改为调用 `refreshSdkDiseaseLayer()`，保证列表右键删除后 2D/3D 立即刷新。
- 删除模式 `commonDeleteDisease()` 在 `deleteOneDisease()` 后也主动调用 `refreshSdkDiseaseLayer()`，保证鼠标删除当前视图病害后立即消失。
- `slot_deleteDisease()` 不再空实现，收到列表删除同步信号时会刷新病害图层。
- `hnSdkDiseaseGraphicsLayer` 的 label 布局改为按文字块大小放在病害框外侧，并从框边界绘制黄色引线到文字块锚点，接近旧 `drawDiseaseCalloutLabel()` 行为。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：

- `hnApplication` Debug x64 编译通过。
- `hnRoadDataProcess` Debug x64 编译通过。
- 仍有既有 warning，主要是 OpenCV/SDK 头文件编码页、旧 typedef、QXlsx PDB 等，没有新增阻断错误。


2026-07-02 SDK 视图工程清理、线状/自动化病害、材质标识修复：

问题：

- 导入/最近工程打开新工程时，只清理了 2D/3D/景观图，工程信息、采集打标、校桩、病害列表、地图和状态栏仍可能残留上一工程信息。
- SDK 设计模式线状病害右键被当成取消绘制，不能弹出病害选择框提交。
- SDK 小框自动化轨迹模式没有稳定进入“开始采样 - 移动追加轨迹 - 再次点击提交”的流程。
- 路面材质打标在 SDK scene 中没有清晰边界提示，旧非 SDK 路径的粗线也容易遮挡路面。
- 病害已经完全不在当前视口时，标签仍被夹到窗口边界显示，挡住当前路面。

处理：

- `hnRoadDataProcess` 新增 `clearCurrentProjectUiState()`，统一清空 2D/3D SDK 与旧图像、景观图、病害列表、工程信息面板、工程树、状态栏和地图。
- `openProjectSlot()`、`openLastProjectSlot()`、`slot_clearProjectSlot()` 和打开工程冲突失败回退都接入统一清理入口。
- `projectView` 增加 `clearProjectInfo()`，清空工程信息、采集打标表和校桩表等子控件状态。
- `hnDiseaseListWidget` 增加 `clearDiseases()`，清空模型并恢复表头/筛选框。
- `CustomBaiduMapView` 增加 `clearMapData()`，清空 GPS/里程缓存、输入框和地图页面。
- `hn2d3dPixBaseWidget::lineDiseaseAddDisease()` 改为返回成功状态，并在 SDK 右键线状病害时走提交流程，继续通过 `hnDiseaseService::addDisease()` 入库，提交成功后自动选中新病害。
- SDK 小框自动化轨迹模式在第一次左键开始采样，鼠标移动追加点，第二次左键调用 `finishSdkLittleFrameDisease()`；提交后的 release 被 SDK 消费，不再落回旧绘制链路。
- `hnSdkDiseaseGraphicsLayer` 增加材质边界 item 管理，SDK 视图对 `nType == 0` 的路面材质打标绘制半透明色带、横向边界线和标签。
- 旧非 SDK `drawMarkValue()` 样式同步为较细边界线、半透明色带和可读标签。
- SDK 病害 label 在添加前先判断病害本体包围盒是否与当前可见 scene rect 相交；完全不可见时不绘制文字和引线。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：

- `hnApplication` Debug x64 编译通过。
- `hnRoadDataProcess` Debug x64 编译通过。
- 仍有既有 warning，主要是 OpenCV/SDK 头文件编码页、旧 typedef、QXlsx PDB、`QStringLiteral` 历史警告和 QXlsx PDB 链接警告，没有新增阻断错误。

2026-07-02 SDK 病害绘制后自动选中与可见区域标签引线修复：

问题：

- SDK 手工绘制病害提交成功后只刷新图层，没有把刚新增的病害设为当前选中态，导致新病害不会立即显示详细信息。
- SDK 正式病害标签仍按固定右上角放置，靠近右侧窗口边缘时文字会被裁掉。
- 选中病害的详细标签字体依赖图层内部 selected key，但重建病害 item 时 `clearDiseases()` 会先清空图层 selected key，导致字体放大不稳定。

处理：

- `hn2d3dPixBaseWidget` 增加“最近 SDK 新增病害”缓存，提交前清空，2D/3D `addDisease(diseaseInfo)` 成功后记录完整 `hnRoadDiseaseInfo`。
- `finishSdkBigFrameDisease()`、`finishSdkLittleFrameDisease()` 提交成功后优先 `setSelectedDisease(新增病害)` 并发出 `signal_selectDisease`，让病害列表、SDK scene 选中态和详细标签同步。
- `setSelectedDisease()` 同步维护旧的 `m_seclectedDiseases`，继续兼容旧右键详情/合并相关逻辑。
- `hnSdkDiseaseGraphicsLayer` 增加标签布局上下文：接收当前 SDK viewport 可见 scene rect、视图缩放比例和 callout gap。
- SDK 标签布局参考旧 `drawDiseaseCalloutLabel()`：优先放右侧，右侧放不下放左侧，再不够放下方，并夹到当前可见区域内；引线从病害框边界连接到文字块锚点。
- 滚轮/Ctrl 缩放后的延迟刷新会重建 SDK 病害 item，使标签和引线按最新可见区域重新避让。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：

- `hnApplication` Debug x64 编译通过。
- `hnRoadDataProcess` Debug x64 编译通过。
- 仍有既有 warning，主要是 OpenCV/SDK 头文件编码页、旧 typedef、QXlsx PDB 等，没有新增阻断错误。

2026-07-03 SDK 小框快捷键、取消状态与绘制中滚轮修复：

问题：

- SDK 视图中 `D` 同时承担右移浏览和小框拉框模式切换，导致快捷键语义冲突。
- 小框轨迹/折线弹出病害选择框后，如果用户按 `Esc` 或关闭窗口取消，临时绘制状态没有完全退出，鼠标移动会继续生成红框。
- SDK 小框 `B` 折线模式结束和中间帧补点仍混用旧逻辑，跨帧时可能使用无意义的 `(100,100)` 补点。
- 病害绘制过程中 SDK wheel 事件被直接拦截，无法正常滚轮翻页或 `Ctrl+滚轮` 缩放。

处理：

- SDK eventFilter 不再绕行 `D` 给小框模式，`W/A/S/D/方向键/Space` 统一交给 `TiledGraphicsView` 浏览。
- 2D/3D 小框拉框/轨迹模式切换快捷键从 `D` 改为 `R`，含义为 Rect/Rectangle；工程设置界面的快捷键提示同步改为 `R`。
- `hn2d3dPixBaseWidget` 增加 `resetSdkDiseaseDrawingState()`，统一清理 SDK 临时 item、小框矩形缓存、轨迹点、折线点、起止点、自动移动锚点和旧选择状态，并恢复绘制/联动标志。
- `finishSdkBigFrameDisease()`、`finishSdkLittleFrameDisease()` 在用户取消弹窗、未选择病害、校验失败或入库失败时调用统一清理方法，取消后必须再次左键才开始新一轮绘制。
- SDK 小框 `B` 折线模式下，`N` 和右键统一走 `finishSdkLineLittleFrameDisease()`；提交前用相邻点线性插值补齐跨图片中间点，不再在 SDK 路径里使用固定 `(100,100)` 补点。
- SDK wheel 事件不再因为 `m_isDrawingDisease` 被直接 accept，普通滚轮和 `Ctrl+滚轮` 继续交给 SDK 浏览/缩放；滚轮后延迟刷新临时病害、正式病害和状态栏，并把当前鼠标所在 SDK 点作为继续绘制锚点。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' SDK\TunnelViewerSDK.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：

- `TunnelViewerSDK` Debug x64 编译通过，0 warning / 0 error。
- `hnApplication` Debug x64 编译通过，0 warning / 0 error。
- `hnRoadDataProcess` Debug x64 编译通过，0 error；仍有既有 warning，主要是 OpenCV/SDK 头文件编码页、旧 typedef、历史 `QStringLiteral` 警告和链接阶段 QXlsx PDB warning，没有新增阻断错误。

2026-07-03 SDK 病害提交后显示与取消弹窗退出修复：

问题：

- 病害选择确认后，病害列表能出现记录，但 SDK 视图上可能没有立即出现正式病害框。
- 用户在病害选择框中按 `Esc` 或关闭窗口取消后，代码只清了一部分绘制变量，仍停留在 `ADD_MODE`，后续鼠标移动可能继续生成临时病害。
- 线状病害成功入库后调用 `slot_cancelDrawDiseases()`，会把“刚新增病害”缓存清掉，导致自动选中和 SDK 图层刷新丢失。

处理：

- `resetSdkDiseaseDrawingState()` 增加 `exitAddMode` 参数；选择框取消、校验失败、入库失败和显式取消时会退出 `ADD_MODE`，鼠标移动不再继续绘制。
- 2D/3D 大框和小框选择框取消分支直接调用 `resetSdkDiseaseDrawingState(true, true)`，明确退出 `ADD_MODE`；`slot_cancelDrawDiseases()` 只清临时绘制状态，不退出当前工作模式。
- 新增 `addSdkDiseaseItem()` 和 `ensureSdkDiseaseItemVisible()`，提交成功后如果当前范围刷新没有生成正式 SDK item，就直接按新增病害兜底绘制到 SDK scene。
- `refreshSdkDiseaseItems()` 也改为统一调用 `addSdkDiseaseItem()`，避免常规滚动刷新和新增后兜底显示使用两套不同的正式病害绘制逻辑。
- 线状病害成功路径不再通过 `slot_cancelDrawDiseases()` 清理新增缓存，改为清临时状态后立即 `setSelectedDisease()` 并确保 SDK item 可见。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：

- `hnApplication` Debug x64 编译通过，0 error；仍有既有 warning，主要是 OpenCV/SDK 头文件编码页和旧 typedef，没有新增阻断错误。
- `hnRoadDataProcess` Debug x64 编译通过，0 error；仍有既有 warning，主要是 OpenCV/SDK 头文件编码页、旧 typedef、历史 `QStringLiteral` 警告和链接阶段 QXlsx PDB warning，没有新增阻断错误。

2026-07-03 SDK 自动化病害进入模式后立即失效修复：

问题：

- 主窗口切换到新增病害、设计面病害、设计线病害时，调用顺序是先 `setAddDiseaseMode()`，再调用 `slot_cancelDrawDiseases()` 清理上一轮临时绘制。
- 上一轮修复把 `slot_cancelDrawDiseases()` 改成了会退出 `ADD_MODE`，导致刚进入绘制模式就被清回 `NO_MODE`。
- 结果是 SDK 鼠标左键按下时直接被 `m_workMode != ADD_MODE` 分支拦截，自动化小框不会出现临时框，也不会进入病害选择弹窗；已有病害图层刷新也容易被后续状态清理影响。

处理：

- 恢复 `slot_cancelDrawDiseases()` 的旧语义：只清临时绘制、临时 SDK item 和旧选择，不退出当前工作模式。
- 真正需要退出绘制模式的路径不再绕 `slot_cancelDrawDiseases()`，而是直接调用 `resetSdkDiseaseDrawingState(..., exitAddMode=true)`。
- 保持病害选择框 `Esc`、关闭窗口、未选择病害、校验失败、入库失败时退出 `ADD_MODE`，避免取消后鼠标移动继续画病害。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：

- `hnApplication` Debug x64 编译通过，0 error；仍有既有 warning，主要是 OpenCV/SDK 头文件编码页和旧 typedef，没有新增阻断错误。
- `hnRoadDataProcess` Debug x64 编译通过，0 error；仍有既有 warning，主要是链接阶段 QXlsx PDB warning，没有新增阻断错误。

2026-07-03 SDK 视图显示所有权收口与病害范围刷新修复：

问题：

- SDK 单图模式仍让旧 `hnBrowsePixWidget::paintEvent()` 生成黑底图并调用旧 `drawSomeThingOnImage()`，容易和 SDK scene 的正式病害、临时病害、标签显示混在一起。
- SDK 模式仍保留旧透明 overlay 创建/抬升逻辑，虽然多数时候被隐藏，但结构上仍可能遮挡或误导排查。
- SDK 当前视口里程使用 `m_imageDistanceMeters` 计算，但旧浏览状态里的 `m_beginEncoderMile/m_endEncoderMile` 仍可能按 `m_heightScale * m_pixHeight` 推算，导致数据库有病害但 `getRoadDiseasesInRange()` 查不到。
- 外部病害增删改后的 `slotDiseaseChanged()` 仍只刷新旧 QWidget/overlay，没有统一重建 SDK scene 病害层。

处理：

- `hnBrowsePixWidget::paintEvent()` 在 `m_skipLegacyImagePaint == true` 时直接返回，SDK 模式不再绘制旧黑底、不再调用旧 overlay 绘制。
- `hn2d3dPixBaseWidget` 在 SDK 模式下不再创建旧 `SdkDiseaseOverlayWidget`；已有 overlay 一律隐藏，`paintSdkDiseaseOverlay()` 保留为空实现兼容旧对象生命周期。
- `syncLegacyBrowseStateFromEncoderMile()` 在 SDK 模式下按 `m_imageDistanceMeters` 重算 `m_beginEncoderMile/m_endEncoderMile`，让正式病害查询范围和 SDK 单图里程保持一致。
- `scrollBottomToEncoderMile()` 在程序定位后立即调用 `refreshSdkDiseaseItems()`，避免列表跳转或联动定位后要等下一次滚动才显示数据库病害。
- 2D/3D `slotDiseaseChanged()` 改为统一调用 `refreshSdkDiseaseLayer()`；非 SDK 路径仍由基类退化为普通 `update()`。
- 列表选中病害居中时，如果当前视口刷新没有该病害 item，则按病害自身 2D/3D 几何兜底生成 SDK scene item 后再居中，不依赖可能有 2D/3D 差值歧义的 `dDmi`。

验证：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnContinuousBrowsePix\hnContinuousBrowsePix.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnApplication\hnApplication.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
& 'C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe' hnRoadDataProcess\hnRoadDataProcess.vcxproj /p:Configuration=Debug /p:Platform=x64 /m:1 /nologo
```

结果：

- `hnContinuousBrowsePix` Debug x64 编译通过，0 error；仍有既有 `hnImagePainter.h` 引用限定符和 `getPixResoluion` 返回路径 warning。
- `hnApplication` Debug x64 编译通过，0 error；仍有既有 OpenCV/SDK 头文件编码页、旧 typedef、旧头文件格式 warning。
- `hnRoadDataProcess` Debug x64 编译通过，0 error；仍有既有 OpenCV 编码页、旧 typedef、`QStringLiteral` 历史 warning 和 QXlsx PDB 链接 warning。

2026-07-03 SDK 全量打标显示与跨道路属性边界校验：

问题：

- SDK 打标图层之前只迁移了 `nType == 0` 的路面材质打标，路面单元、路面等级、路面标准、路面情况都不会在 2D/3D SDK 视图上显示。
- 打标编辑对话框里如果只增删非道路属性打标，旧逻辑不一定重载路面/三维影像，SDK scene 也不会立即重建打标图层。
- 病害绘制主要依赖采样点对应的 `hnMile` 路面类型/标准集合判断，遇到病害范围跨过材质、等级、标准打标边界时不够直接，容易漏掉边界内的切换点。
- 3D SDK 视图使用三维自己的编码器里程，工程打标是二维基准里程；如果不做换算，3D 打标和跨边界校验可能整体偏移一个 `2D3D_DIFF`。

处理：

- `refreshSdkMaterialMarks()` 保留旧函数名但语义扩展为 SDK 道路打标图层刷新，遍历当前工程所有 `hnMarkInfo`，不再只处理材质打标。
- 新增 SDK 打标类型和颜色规则：材质、等级、标准使用醒目的彩色实线和半透明横向色带；路面单元、路面情况使用较细的虚线和低透明色带，减少对路面图像的遮挡。
- `hnSdkDiseaseGraphicsLayer::addMaterialBoundaryMark()` 增加线宽、线型、色带高度、标签行参数，支持不同打标类型共用一个 scene 绘制入口。
- 新增 `projectEncoderMileToSdkViewEncoderMile()`：2D 默认不变，3D 按 `projectEncoderMile - 2D3D_DIFF` 换算，保证 3D 打标显示和跨边界校验使用三维视图里程基准。
- 新增 `temporaryDiseaseEncoderMileRange()` 和 `isTmpDiseaseRoadMarkRangeValid()`，把人工框、小框集合、线状点转换为临时病害覆盖里程范围；如果范围内部穿过材质、等级、标准打标边界，直接判定病害无效。
- 2D/3D `isTmpDiseaseRoadTypeValid()` 在原有路面标准/类型集合判断后，继续调用打标边界校验；线状病害提交也补了明确的跨打标边界提示。
- `hnRoadDataProcess::updateAllWidget()` 统一刷新 2D/3D SDK 病害/打标图层；`slot_markInfoSlot()` 接受打标编辑后无论是否需要重算 `hnMile` 都会触发一次全量刷新。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hnApplication` Debug x64 编译通过，0 error；仍有既有 OpenCV/SDK 头文件编码页、旧 typedef、旧头文件格式 warning。
- `hnRoadDataProcess` Debug x64 编译通过，0 error；仍有既有 OpenCV 编码页、旧 typedef、`QStringLiteral` 历史 warning 和 QXlsx PDB 链接 warning。

2026-07-03 SDK 打标双击定位与可见性过滤修复：

问题：

- 双击采集打标列表后，主窗口只按普通桩号跳转，打标线可能落在当前视图边缘或因为 SDK 状态尚未同步而没有立即显示。
- `hnSdkDiseaseGraphicsLayer::addMaterialBoundaryMark()` 使用打标色带 `bandRect` 和当前可见 scene rect 做相交判断；当跳转后的可见 rect 与刷新时的打标 sceneY 暂时不同步时，会在 `isSceneRectVisible(bandRect)` 处直接 return，导致打标 item 根本没有加入 scene。
- `centerOnEncoderMile()` 初始实现把中心里程直接传给旧状态同步函数，容易把中心点误当底部里程，影响后续范围刷新。

处理：

- 打标表双击新增专用信号 `signal_jumpToMark(int markId, double trueMile)`；校桩表双击仍走原有 `signal_jumpToMile()`。
- 主窗口新增 `slot_jumpToMark()`，按打标真实桩号换算编码器里程后，通过 SDK 视图居中定位到打标里程，并触发 2D/3D 联动刷新。
- `hn2d3dPixBaseWidget` 新增 `centerOnEncoderMile()`，用于把指定编码器里程定位到 SDK 视图中心；同步旧状态时改为读取当前 SDK 底部 anchor，而不是把中心里程当底部里程。
- `addMaterialBoundaryMark()` 不再因为当前 visible rect 与 `bandRect` 不相交而跳过创建打标 item。打标本身已经在业务层按当前里程范围过滤，scene item 应该创建出来，避免跳转/刷新时序导致完全看不到。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hnApplication` Debug x64 编译通过，0 error；仍有既有 OpenCV/SDK 头文件编码页、旧 typedef warning。
- `hnRoadDataProcess` Debug x64 编译通过，0 error；仍有既有 OpenCV 编码页、旧 typedef、历史 `QStringLiteral` warning 和 QXlsx PDB 链接 warning。

2026-07-03 SDK 打标跳转里程与标签锚定修复：

问题：

- 打标列表双击虽然传了 `markId`，但 `slot_jumpToMark()` 里没有使用它，而是继续用表格展示的桩号文本反算编码器里程；遇到桩号显示精度、上下行或校桩换算边界时，可能反算到 0 附近，表现为双击跳到工程开头。
- 打标标签 y 坐标被 `visibleRect.top/bottom` 夹到当前窗口内，导致滚轮滚动时文字看起来固定在左下角，而不是跟着桩号对应的道路位置移动。
- 打标位置缺少明确指示线，用户不容易一眼看出标签对应哪一条桩号边界。

处理：

- `slot_jumpToMark()` 改为优先通过 `markId` 在当前工程 `getCurrentMarkVector()` 中查找原始 `hnMarkInfo.dEnclMile`，找不到时才退回 `trueMileToEncl(trueMile)`。
- `hnSdkDiseaseGraphicsLayer::addMaterialBoundaryMark()` 的标签 y 坐标改为直接基于 `sceneY` 计算，不再按可见窗口上下边界夹紧；标签会随桩号对应的 SDK scene 位置一起滚动。
- 标签只在打标横线本体与当前可见区域相交时显示，避免横线不可见时文字残留在窗口边界。
- 增加从标签到打标横线的引线和箭头短线，配合原有横向边界线，明确指向打标位置。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hnApplication` 本轮修改的源文件已进入编译阶段并通过语法/编译检查。
- 最终链接失败：`LINK : fatal error LNK1104: 无法打开文件“..\bin\Debug-x64\hnApplication.dll”`。
- 原因是当前正在运行 `hnRoadDataProcess.exe`，进程占用了 `hnApplication.dll`；关闭软件后重新编译即可完成链接验证。

2026-07-03 SDK 小框病害显示与翻页卡顿修复：

问题：

- 小框病害进入当前视图范围后浏览翻页明显变卡。
- 小框病害确认入库后，病害列表已有记录，但 SDK 视图上看不到正式病害框。

处理：

- `hn2d3dPixBaseWidget::addSdkDiseaseItem()` 不再对小框病害的每一个小格子分别创建一个 `QGraphicsPathItem`。
- SDK 正式病害刷新时先把同一个病害的所有小框 rect 合并为一个 `QPainterPath`，再一次性加入 `hnSdkDiseaseGraphicsLayer`，降低 scene item 数量和滚动重建成本。
- `sdkPathFromBigImageRect()` 改为先把大图 rect 转为单图 rect，再映射到 SDK scene；转不出图片名或有效 rect 时直接跳过，不再让无效点落到 `(0,0)`。
- 选中病害如果没有可显示 rect 或 scene path，会输出 `[HN_SDK_DISEASE_RENDER_SKIP]` 日志，包含病害 key、绘制类型、rect 数量、里程和当前刷新范围，便于继续判断是查询范围问题还是几何数据为空。
- 选中病害成功进入 SDK scene 时输出 `[HN_SDK_DISEASE_RENDER]`，记录原始 rect 数量和有效 rect 数量。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hn2d3dPixBaseWidget.cpp` 已完成编译，0 个本轮代码错误。
- 最终链接仍失败：`LINK : fatal error LNK1104: 无法打开文件“..\bin\Debug-x64\hnApplication.dll”`。
- 当前运行中的 `hnRoadDataProcess.exe` pid 30344 占用了 `hnApplication.dll`；关闭软件后重新编译即可完成链接验证。

2026-07-06 SDK 打标双击跳转与按张翻页移除修复：

问题：

- 打标表格双击仍可能跳到工程开头。根因是表格只把显示桩号传给主窗口，主窗口再从显示桩号反算编码器里程；当显示桩号精度、上下行、校桩区间或 markId 匹配不稳定时，会得到 0 附近的里程。
- SDK 打标刷新仍可能使用旧的可见 scene 上下文，导致进入 `addMaterialBoundaryMark()` 后判断不到当前打标线可见，标签不显示。
- 打标标签沿用病害标签的“贴当前窗口边界”思路，滚动时看起来像固定在界面角落，而不是固定在道路桩号对应位置。
- 旧工具栏仍保留“按张翻页”勾选框和 `wheelScrollOneImage` 配置，这与当前 SDK 连续浏览逻辑重复。

处理：

- `projectView::updateMarkFrom()` 在打标表第一列缓存 `mark.nID`、`mark.dTrueMile`、`mark.dEnclMile`。
- `projectView::signal_jumpToMark()` 扩展为 `signal_jumpToMark(int markId, double trueMile, double encoderMile)`；双击打标表时直接把行内原始编码器里程传给主窗口。
- `hnRoadDataProcess::slot_jumpToMark()` 优先使用表格缓存的 `dEnclMile`，再退回按 `markId` 查当前工程打标，最后才用真实桩号反算，避免误跳到 0。
- `hn2d3dPixBaseWidget::refreshSdkMaterialMarks()` 在刷新打标层前主动同步 SDK 当前可见 scene rect 和缩放比例，并用当前 viewport 推算可见编码器里程范围。
- `hnSdkDiseaseGraphicsLayer::addMaterialBoundaryMark()` 的文字和箭头锚定到打标 `sceneY`，不再把 y 坐标夹到当前窗口边界；打标本体不可见时不显示标签。
- 移除工具栏“按张翻页”勾选框、`addWheelScrollStepOption()`、`wheelOneImageChechBox` 和 `HnXRSettings::wheelScrollOneImage` 配置读写；旧连续浏览步长固定走连续滚动。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hnApplication` Debug x64 编译通过，0 error；仍有既有第三方头文件编码页、旧 typedef 等 warning。
- `hnRoadDataProcess` Debug x64 编译通过，0 error；第一次完整重建时遇到旧 `.sbr` 浏览信息损坏，清理主程序 Debug 中间目录下 `.sbr` 后增量构建通过。

2026-07-06 SDK 打标反向拼接坐标修复：

问题：

- 打标表格第一列本来就是桩号，双击信号也已经连接，但实际跳转和打标线显示仍不对。
- 根因是 SDK 单图序列当前使用 `LayoutOrientation::VerticalReverse`：第一张图在 scene 底部，里程越大 `sceneY` 越小；上一轮 `encoderMileToSdkSceneY()` / `sdkSceneYToEncoderMile()` 仍按正向拼接计算，导致打标线被画到相反方向的 scene 位置，可见范围过滤和居中跳转都会错。
- 部分旧工程/旧库可能存在 `MarkInfo.EnclMile` 为 0 或与真实桩号不一致的记录；单纯信任表格缓存的 `dEnclMile` 会继续跳到工程开头。

处理：

- `hn2d3dPixBaseWidget::encoderMileToSdkSceneY()` 改为反向拼接公式：`sceneY = m_pixHeight - encoderMile * m_pixHeight / m_imageDistanceMeters`，并按 SDK scene rect 夹紧。
- `hn2d3dPixBaseWidget::sdkSceneYToEncoderMile()` 同步改为反向公式，修正打标刷新可见里程范围、SDK 中心里程信号和基于 sceneY 的定位。
- `hnRoadDataProcess::slot_jumpToMark()` 对表格缓存 `dEnclMile` 做真实桩号一致性校验；若 `enclToTrueMile(dEnclMile)` 与表格桩号相差超过 5m，则不信任该值，改用 mark 原始桩号或表格桩号重新 `trueMileToEncl()`。
- 打标双击不再走“把打标里程当 2D 底部里程”的统一同步入口，而是按打标线里程居中 2D；有 3D 时按 `2D3D_DIFF` 居中 3D；景观图按 2D 打标里程同步。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hnApplication` Debug x64 编译通过，0 error。
- `hnRoadDataProcess` Debug x64 编译通过，0 error。

2026-07-06 SDK 小框病害入库后 2D 几何为空修复：

问题：
- 用户日志显示小框病害已经写入数据库和病害列表，但 2D SDK 视图仍看不到正式框。
- 关键诊断日志为 `DisJL#3 drawType=1 pathEmpty=true vec2d=0 vec3d=17`；同一病害在另一路径中 3D path 可生成，说明不是病害服务或 SDK scene 没刷新，而是入库时 2D 小框几何没有生成。
- 入库前还有大量 `makeLittleFrameHn2dRect invalid center ... centerSinglePoint=QPoint(...,-3971)`，说明旧的“大图 rect -> 单图 rect”反算依赖当前窗口拼接状态，弹出病害选择框、SDK 同步或滚动状态变化后会得到负 y，最终 `vec2dRect` 为空。

处理：
- `hn2d3dPixBaseWidget` 新增 `m_currentLittleFrameSingleSelections`，在 SDK 小框预览阶段就记录稳定的 `pixName + singleRect`。
- `updateSdkLittleFramePreview()` 的拉框分支和轨迹分支都会在弹出病害选择框前刷新这份稳定单图选择；之前拉框分支的调用被放在 `return` 后，本轮已移动到 `return` 前。
- `clearLittleRectDrawSelection()` 同步清理 `m_currentLittleFrameSingleSelections`，避免取消或下一笔绘制复用旧格子。
- `hn2dPixWidget::generateLittleFrameHn2dRectVector()` 优先使用稳定的 `pixName + singleRect` 直接生成 `vec2dRect`；只有没有 SDK 稳定选择时才退回旧的 `makeLittleFrameHn2dRect()`。
- 这样小框确认入库后，2D/3D 几何都应存在，`refreshSdkDiseaseLayer()` 再从数据库重建正式 SDK item 时不再因为 `vec2d=0/pathEmpty=true` 跳过。

验证：
```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /nr:false /v:minimal
```

结果：
- `hnApplication` Debug x64 编译通过，0 error。
- `hnRoadDataProcess` Debug x64 编译通过，0 error。
- 第一次主程序编译曾因上一次超时残留的 `MSBuild/CL.exe` 占用 `vc140.pdb/tlog` 失败；停止残留编译进程并使用 `/m:1 /nr:false` 后通过。仍有既有第三方头文件编码页、QXlsx PDB 等 warning。

2026-07-06 SDK 小框病害变成短水平线修复：

问题：
- 用户断点确认 `hn2dPixWidget::generateLittleFrameHn2dRectVector()` 中 `selection.singleRect` 的 y 为负数，例如 `y=-2083`。
- 后续 `toHnPoint()` 对 y 做 `qBound(0, y, m_pixHeight - 1)`，再叠加垂直翻转，导致 `p0/p1/p2/p3` 的 y 都变成同一个值，例如 `2602`，正式病害显示成一根水平线。
- 根因是上一轮虽然缓存了 `pixName + singleRect`，但缓存来源仍是 `bigImageRectToSingleImageRect(bigRect)`；SDK 模式下这个旧“大图坐标”依赖 `m_buttomFrameIdx/currentPaintImageSize()`，滚动、弹窗、SDK scene 坐标都会让反算结果漂移。

处理：
- `hn2d3dPixBaseWidget` 新增 SDK 小框单图格子生成逻辑：
  - `littleFrameSingleCellForPoint()`：直接按 SDK 命中的 `pixImagePoint.pixName + pixPoint` 计算 0.1m 小框格子。
  - `rebuildCurrentLittleFrameSingleSelectionsFromRect()`：拉框模式从起止 SDK 单图点直接生成单图格子。
  - `rebuildCurrentLittleFrameSingleSelectionsFromPoints()`：轨迹/B 折线模式从 SDK 单图轨迹点生成单图格子，并对相邻点做插值补格。
- `updateSdkLittleFramePreview()` 不再用旧大图 rect 反算单图 rect，而是按 SDK 起止点/轨迹点刷新 `m_currentLittleFrameSingleSelections`。
- `commitCurrentLittleRectDrawSelection()` 不再从 `m_tmpLittleFrameDiseaseRects` 反算单图 rect，而是提交当前 SDK 单图格子缓存；提交后 `m_currentLittleFrameSingleSelections` 与 committed selections 保持一致。
- `finishSdkLineLittleFrameDisease()` 在 B 折线提交前也刷新 SDK 单图格子缓存。

验证：
```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m:1 /nr:false /v:minimal
```

结果：
- `hnApplication` Debug x64 编译通过，0 error。
- `hnRoadDataProcess` Debug x64 编译通过，0 error。
- 仍有既有第三方头文件编码页、旧代码返回值、QXlsx PDB warning。

2026-07-06 SDK 小框正式病害显示修复：

问题：

- 小框病害确认后数据库和病害列表已有记录，但 SDK 视图上看不到正式病害框。
- 根因是 SDK 正式病害渲染仍先把数据库病害点转成旧“当前窗口大图坐标”，再由 `sdkPathFromBigImageRect()` 转回 SDK scene。旧大图坐标依赖 `m_buttomFrameIdx`、当前窗口拼图高度和半张图显示状态，SDK 单图全局 scene 下容易转换为空路径或错误位置。

处理：

- `hn2d3dPixBaseWidget` 新增 `sdkDiseaseScenePath()`，`addSdkDiseaseItem()` 统一通过 scene path 生成正式病害 item。
- 2D/3D 子类重写 `sdkDiseaseScenePath()`：小框和框状病害直接使用数据库里的 `vec2dRect` / `vec3dRect` 点位，按里程找回图片名，再把单图坐标直接映射到 SDK scene。
- 2D 的 `getPreviousStakeIterator()` 参数从 `int` 改为 `double`，避免病害点里程被截断后找错图片。
- 这样新增小框入库后不再依赖旧窗口大图坐标，`ensureSdkDiseaseItemVisible()` 可以直接把正式框画到 SDK scene。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hnApplication` Debug x64 编译通过，0 error。
- `hnRoadDataProcess` Debug x64 编译通过，0 error。
- 仍有既有 `QXlsx.pdb` 缺少调试信息的链接 warning，不影响 exe 生成。

2026-07-06 SDK 小框取消后等待下一次左键修复：

问题：

- 上一轮修复让病害选择框 `Esc`/关闭后保留 `ADD_MODE`，解决了“下一次左键不能重新画”的问题。
- 但如果只保留 `ADD_MODE` 而不区分“当前这一笔已经取消”，鼠标移动仍可能进入小框轨迹/折线预览逻辑，表现为取消弹窗后还没再次按左键，移动鼠标就继续画红框。

处理：

- `hn2d3dPixBaseWidget` 增加 `m_waitLittleFrameLeftPressAfterCancel`，表示“小框取消后必须等待下一次左键按下”。
- `resetSdkDiseaseDrawingState()` 在保留 `ADD_MODE` 且当前是 `LITTLE_FRAME` 时设置该标志；`handleSdkDiseaseMouseMove()` 看到该标志后直接返回，不再追加轨迹点、折线点或临时小框。
- SDK/旧 2D/3D 鼠标左键按下时清除该标志，下一笔绘制从新的左键起点重新开始。
- 2D/3D 旧 `mouseMoveEvent()` 的小框临时绘制条件同步加上该标志，避免旧兼容路径在取消后继续画。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hnApplication` Debug x64 编译通过，0 error。
- `hnRoadDataProcess` Debug x64 编译通过，0 error。

2026-07-06 SDK 小框绘制取消后保留绘制模式修复：

问题：

- SDK 小框模式下，左键绘制并弹出病害选择框后，如果用户按 `Esc` 或关闭窗口取消选择，当前临时病害会被取消，但下一次左键点击视图不能重新开始绘制。
- 根因是取消弹窗/提交失败路径调用 `resetSdkDiseaseDrawingState(..., true)`，第二个参数会在 `resetSdkDiseaseDrawingState()` 内执行 `setMode(WorkMode::NO_MODE)`，把整个添加病害模式退出了；用户预期只是取消当前这一笔绘制。

处理：

- `finishSdkBigFrameDisease()`、`finishSdkLittleFrameDisease()` 的失败清理分支改为 `resetSdkDiseaseDrawingState(false, false)`，只清临时绘制状态，不退出当前添加模式。
- `hn2dPixWidget::littleFrameProcess()` 和 `hn3dPixWidget::littleFrameProcess()` 中病害选择框取消分支改为 `resetSdkDiseaseDrawingState(true, false)`，取消当前小框并清除选择，但保留 `ADD_MODE`。
- 这样 Esc/关闭病害选择框后，下一次左键仍会重新开始一笔小框绘制。

验证：

```powershell
$msbuild = (Get-Command msbuild.exe -ErrorAction SilentlyContinue).Source
& $msbuild hnRoadDataProcess.sln /t:hnApplication /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
& $msbuild hnRoadDataProcess.sln /t:hnRoadDataProcess /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

结果：

- `hnApplication` Debug x64 编译通过，0 error。
- `hnRoadDataProcess` Debug x64 编译通过，0 error。
