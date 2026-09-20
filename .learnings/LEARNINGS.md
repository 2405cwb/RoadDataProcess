# Learnings

## [LRN-20260831-3DIDX] correction

**Logged**: 2026-08-31T14:45:00+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
The missing `Pavement-cam-1.idx` import prompt is a separate 3D-project preflight failure, not the 2D `RoadImg` blank-image problem.

### Details
For a 2D+3D import, the validator requires the 3D project's `Image/Pavement-cam-1.idx`. When that configured file is missing or unreadable, validation intentionally rejects the entire import before project initialization. This must remain separate from later 2D road-image discovery and display behavior.

### Suggested Action
Classify customer image failures by stage and data source: import-time 3D index validation versus post-import 2D `RoadImg/Camera0/Image_####` loading.

### Metadata
- Source: user_feedback
- Related Files: hnRoadDataProcess/hnProjectImportValidator.cpp
- Tags: import-preflight, 2d3d, pavement-index, road-image

---

## [LRN-20260825-001] correction

**Logged**: 2026-08-25T16:28:00+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
校桩偏移修复必须在扣减完成后按最终“真实桩号 + 相对 DMI”再次去重。

### Details
G75 同时包含标准起点 `1490933 -> 0` 和外业起点 `1490933 -> 3704`。旧实现只在扣减前去重，外业起点扣除 `3704` 后与标准起点重合，最终数据库留下两条起点零值。

### Suggested Action
先生成最终相对 DMI 校桩集合，再去重并连续重排 ID；标准起终点优先保留。已修复但存在重复锚点的老库应以偏移量 `0` 只清理重复项，不再扣减打标，也不新增偏移修复日志。

### Metadata
- Source: user_feedback
- Related Files: hnProject/hnProject.cpp, hnDataTable/hnDBSqlite.cpp, hnDataTable/hnDBSqlite.h
- Tags: sqlite, dmi, normalization, deduplication, idempotence
- Pattern-Key: normalize.dedupe_after_transform
- Recurrence-Count: 1
- First-Seen: 2026-08-25
- Last-Seen: 2026-08-25

### Resolution
- **Resolved**: 2026-08-25T16:28:00+08:00
- **Notes**: 增加扣减后去重和连续 ID 重排；G75 首次修复、老库清理、第二次打开幂等性及一万组随机属性验证均通过。

---

## [LRN-20260831-PBI] correction

**Logged**: 2026-08-31T00:00:00+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
Do not treat a jump-report stake label as the direct `resample.txt` distance index.

### Details
The 10 m jump report first converts each displayed true-mile/stake boundary through `trueMileToEncl()`. `readPbiValueFromFile()` then uses the resulting start/end encoder miles to index `resample.txt`. Diagnosis must compare that converted encoder interval with the landscape encoder distance, not compare the displayed stake label directly with a landscape image position.

### Suggested Action
For customer mismatch diagnosis, print or calculate the exact pair `displayed stake interval -> start/end DMI` before attributing the difference to calibration, encoder accuracy, or camera coverage.

### Metadata
- Source: user_feedback
- Related Files: hnRoadDataProcess/hnOutExcelMileManage.cpp, hnProject/hnProject.cpp, hnApplication/hnStreetCameraView.cpp
- Tags: jump-report, pbi, resample, mileage-mapping, landscape

---

## [LRN-20260828-GJ] correction

**Logged**: 2026-08-28T00:00:00+08:00
**Priority**: high
**Status**: resolved
**Area**: frontend

### Summary
国检转换批次默认只包含同一个县区，县级行政区划代码应由用户整批输入一次。

### Details
用户一次可能导入上百条道路，不能要求逐工程填写行政区划代码。转换前应明确提示当前导入工程必须属于同一个县区；界面只收集一次 6 位县级行政区划代码，服务层对整批复用该代码，并在发现工程已有代码互相冲突或与批次代码不一致时阻止写入。

### Suggested Action
批量转换参数应按业务作用域设计：县级行政区划代码属于批次参数，不属于逐工程参数；逐工程只保留路线、方向、规范、绘制模式等自身属性。

### Metadata
- Source: user_feedback
- Related Files: hnRoadDataProcess/hnRoadDataProcess.cpp, hnRoadDataProcess/hnGjConvertSourceService.h, hnRoadDataProcess/hnGjConvertSourceService.cpp
- Tags: gj-convert, batch, county-code, preflight

---

## [LRN-20260827-MILESTART] correction

**Logged**: 2026-08-27T11:20:00+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
Absence of the project-start calibration record in `MileStoneCaliInfo.txt` means the source DMI offset is zero; it is not an invalid project.

### Suggested Action
Allow automatic import and manual calibration/mark reimport to continue with offset zero, while still generating protected database start/end anchors and rejecting genuinely malformed, ambiguous, negative, or out-of-range data.

### Metadata
- Source: user_feedback
- Related Files: hnProject/hnProject.cpp
- Tags: mileage-pile, dmi-offset, reimport

---

## [LRN-20260827-ROUTE] correction

**Logged**: 2026-08-27T09:12:00+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
Trace the exact report filename to its exporter before changing a similarly named project-information function.

### Details
The customer workbook named `路面病害面积统计表` uses `exportLMBHMJTJB_Smart()` and `exportProjectInfoSheet()`. The earlier material fix was applied only to `exportProjectInfoSheet_GZQT()`, which belongs to a different report route, so the customer output remained blank.
`GZQT` in this project means `贵州乾通`; do not misidentify it as a Guangdong-specific route. Both the standard and GZQT engineering-information exporters should follow the same database-backed material-mark rule.

### Suggested Action
For report defects, verify the complete runtime path from selected report type and output filename through exporter, helper, sheet, and cell before editing.

### Metadata
- Source: user_feedback
- Related Files: hnRoadDataProcess/hnOutExcelManage.cpp
- Tags: report-export, call-path, project-info, road-surface

---

## [LRN-20260826-001] correction

**Logged**: 2026-08-26T10:26:20+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
Do not treat the optional `IRIMTD` directory as a mandatory project-import prerequisite.

### Details
The import validator incorrectly rejected projects without `IRIMTD`. In `hn2DProject::init()`, this path is passed to `setPath(..., false)`, and a missing directory still returns success. The user also confirmed that `IRIMTD` is not business-required.

### Suggested Action
Base mandatory import checks on the actual runtime contract and acquisition mode. Optional equipment-data directories may be diagnosed when their function is requested, but must not block general project import.

### Metadata
- Source: user_feedback
- Related Files: hnProject/hn2DProject.cpp, hnRoadDataProcess/hnProjectImportValidator.cpp
- Tags: qt, import-validation, optional-data, irimtd

### Resolution
- **Resolved**: 2026-08-26T10:26:20+08:00
- **Notes**: Removed the `IRIMTD` required-directory check from the project import validator.

---

Corrections, insights, and knowledge gaps captured during development.

**Categories**: correction | insight | knowledge_gap | best_practice

---

## [LRN-20260715-001] correction

**Logged**: 2026-07-15T16:35:00+08:00
**Priority**: high
**Status**: pending
**Area**: config

### Summary
Line-camera acquisition configuration is owned by Camera0 at `RoadImg/Camera0/LineCamSetting.ini`.

### Details
The production format uses `MmPerPix_W`, `MmPerPix_H`, and `DiffValue`, and values may contain inline comments introduced by either ASCII `;` or full-width `；`. Camera0 is the authoritative left-camera source. Camera1 is only a reserved dual-camera interface for now and must not affect current calculations.

### Suggested Action
Detect line-camera projects from Camera0, parse numeric prefixes robustly, and keep Camera1 metadata isolated until dual-camera composition is explicitly implemented.

### Metadata
- Source: user_feedback
- Related Files: hnProject/hnLineCameraConfig.h, hnProject/hnLineCameraConfig.cpp
- Tags: line-camera, camera0, ini, config

---

## [LRN-20260715-006] correction

**Logged**: 2026-07-15T15:10:00+08:00
**Priority**: high
**Status**: resolved
**Area**: frontend

### Summary
Verify complete image names and reproduce the paint path before attributing a virtual-sequence black screen to folder-local numeric prefixes.

### Details
The Image_0001 folder restarts the three-digit prefix at 000, but the timestamp suffix keeps every complete basename unique. The initial collision hypothesis used only the prefix and was wrong. Full-name intersection was zero and all 159 JPEGs in Image_0001 decoded successfully. Runtime reproduction showed the same cached frame returned immediately after minimize/restore, proving the failure was viewport painting rather than file lookup or decode. The actual virtual chunk lacked ItemUsesExtendedStyleOption, so Qt supplied the full 512-frame chunk bounds as exposedRect and each small scroll could process the entire chunk.

### Suggested Action
For acquisition folders, compare complete basenames or absolute paths before declaring collisions. For QGraphicsItem paint virtualization, enable ItemUsesExtendedStyleOption and verify that exposedRect represents the viewport dirty region rather than the complete chunk bounds.

### Metadata
- Source: user_feedback
- Related Files: SDK/src/VirtualImageSequence.cpp, UintTest/test_VirtualImageSequence.cpp
- Tags: virtual-sequence, black-screen, exposed-rect, image-name

### Resolution
- **Resolved**: 2026-07-15T15:10:00+08:00
- **Notes**: Removed the speculative collision changes, enabled the extended style option on SequenceChunkItem, added a regression test, and passed Debug x64 build plus five virtual-sequence tests.

---

## [LRN-20260715-005] correction

**Logged**: 2026-07-15T14:35:00+08:00
**Priority**: critical
**Status**: in_progress
**Area**: frontend

### Summary
Do not remove virtual-sequence stale-request cancellation without measuring queue backlog and runtime black-frame duration.

### Details
The first black-screen fix removed `QThreadPool::clear()` from every viewport refresh and split the image caches based only on source inspection and successful compilation. The user reported that black screens became substantially worse. Keeping stale full-image and prefetch jobs allows old viewports to occupy the decode pool, so current visible thumbnails can wait behind obsolete work while paint renders the null-image background.

### Suggested Action
Restore queued-work cancellation, but add a viewport request generation so already-running old jobs cannot clear or overwrite bookkeeping for the current viewport. Validate with runtime counters for visible cache misses, queued/running jobs, decode completion, and repaint latency before declaring the black-screen issue fixed.

### Metadata
- Source: user_feedback
- Related Files: SDK/src/TiledGraphicsView.cpp, SDK/src/TiledGraphicsView.h, SDK/src/VirtualImageSequence.cpp
- Tags: virtual-sequence, black-screen, decode-queue, regression, runtime-validation

---

## [LRN-20260715-002] correction

**Logged**: 2026-07-15T10:30:00+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
Do not apply SDK image-count limits based on whether a project uses a line-scan or area camera.

### Details
Both line-scan and area-camera road projects can contain tens of thousands of ordered images. Retaining the old 2000-image limit for area-camera projects reproduces the same empty SDK scene and black 2D view. Camera type determines physical scale, not a valid maximum image count.

### Suggested Action
The ordered `loadImages()` entry must accept the caller's complete image list for every camera type. Address very large projects through scene virtualization and bounded caches rather than an arbitrary project-count rejection.

### Metadata
- Source: user_feedback
- Related Files: SDK/src/TunnelViewerController.cpp, hnApplication/hn2d3dPixBaseWidget.cpp
- Tags: sdk, image-count, area-camera, line-camera, virtualization
- Pattern-Key: harden.camera_independent_image_count
- Recurrence-Count: 1
- First-Seen: 2026-07-15
- Last-Seen: 2026-07-15

### Resolution
- **Resolved**: 2026-07-15T10:32:00+08:00
- **Notes**: Removed the hard count rejection from ordered image loading and removed the camera-specific limit override.

---

## [LRN-20260715-001] best_practice

**Logged**: 2026-07-15T09:35:00+08:00
**Priority**: high
**Status**: resolved
**Area**: frontend

### Summary
Do not use Qt 5.8 `QPixmap(path)` for line-camera preview JPEGs; decode file bytes with the project's OpenCV path and convert to RGB first.

### Details
The sample line-camera frames are valid 4096x2000 grayscale JPEGs. `QImageReader::size()` succeeds because it only probes the header, while `QPixmap(path)` in Qt5Guid.dll reproducibly raises 0xC0000005 during full decode. Pillow and OpenCV decode the same files successfully. A focused Qt program reproduced the crash before the dialog created any graphics scene and verified the OpenCV replacement for both first-use and configured-area flows.

### Suggested Action
For this legacy Qt build, validate preview images by actually decoding them. Read through `QFile`, use `cv::imdecode(..., IMREAD_COLOR)`, convert BGR to RGB, and copy into `QImage` before creating a `QPixmap`.

### Metadata
- Source: user_feedback
- Related Files: hnRoadDataProcess/hnLineCameraValidAreaDialog.cpp, hnProject/hnLineCameraConfig.cpp
- Tags: qt5, jpeg, grayscale, opencv, access-violation
- Pattern-Key: harden.legacy_qt_image_decode
- Recurrence-Count: 1
- First-Seen: 2026-07-15
- Last-Seen: 2026-07-15

### Resolution
- **Resolved**: 2026-07-15T09:35:00+08:00
- **Notes**: Replaced the preview decoder and verified both first-use and configured-area dialog lifecycles against the real sample frame.

---

## [LRN-20260713-005] correction

**Logged**: 2026-07-13T17:00:00+08:00
**Priority**: medium
**Status**: wont_fix
**Area**: frontend

### Summary
Magnifier region side length is the fixed on-screen lens size; magnification must not multiply that size.

### Details
The initial SDK implementation copied the legacy formula and rendered a lens of `sideLength * magnification`. The expected interaction is a stable lens footprint: increasing magnification samples a smaller source region and enlarges its contents into the same configured lens size.

### Suggested Action
Use `sourceSide = displaySide / magnification` and always scale the result to `displaySide`; keep overlay geometry and crosshair widths independent of magnification.

### Metadata
- Source: user_feedback
- Related Files: hnApplication/hnMagnify.cpp, hnApplication/hn2d3dPixBaseWidget.cpp
- Tags: magnifier, ui-semantics, zoom

### Resolution
- **Resolved**: 2026-07-13T17:20:00+08:00
- **Notes**: Superseded by removing the custom magnifier and using SDK Ctrl+wheel zoom.

---

## [LRN-20260713-006] correction

**Logged**: 2026-07-13T17:20:00+08:00
**Priority**: medium
**Status**: resolved
**Area**: frontend

### Summary
Use the SDK Ctrl+wheel zoom and remove the duplicate custom magnifier UI.

### Details
The SDK already provides continuous zoom. Its mouse anchor was being overridden after scaling by restoring the previous bottom image anchor, making zoom appear independent of cursor position.

### Suggested Action
Preserve the scene point under the wheel event explicitly across scaling and do not restore a fixed bottom anchor afterward.

### Metadata
- Source: user_feedback
- Related Files: SDK/src/TiledGraphicsView.cpp, hnRoadDataProcess/hnRoadDataProcess.cpp
- Tags: sdk-zoom, mouse-anchor, remove-duplicate-feature

---

## [LRN-20260713-001] correction

**Logged**: 2026-07-13T00:00:00+08:00
**Priority**: high
**Status**: resolved
**Area**: frontend

### Summary
Selection propagation must not reapply selection to the view that originated the signal.

### Details
The SDK view applies the right-click selection locally before emitting `signal_selectDisease`. The coordinator then updated both views, causing the source view to clear and rebuild its disease layer a second time. Newly added diseases are especially sensitive because the database-backed cache is being invalidated and rebuilt at the same time.

### Suggested Action
Pass the selection source into the coordinator, update only the other view and the disease list, and keep selected and normal disease colors visually distinct.

### Metadata
- Source: user_feedback
- Related Files: hnRoadDataProcess/hnRoadDataProcess.cpp, hnApplication/drawDiseases.h
- Tags: disease-selection, recursion, duplicate-refresh, sdk-layer

---

## [LRN-20260713-002] correction

**Logged**: 2026-07-13T00:00:00+08:00
**Priority**: high
**Status**: resolved
**Area**: frontend

### Summary
Cross-view selection must ensure the selected disease item exists in the target scene, and list matching must use stored row objects.

### Details
Newly drawn diseases already used `ensureSdkDiseaseItemVisible`, while existing diseases synchronized through `setSelectedDisease` only rebuilt the target view's current mileage range. The list also searched only visible proxy rows by display strings even though each row stores the complete disease object in `Qt::UserRole`.

### Suggested Action
After target-view refresh, ensure the selected disease geometry is present. Match list rows through the stored disease object's table name and ID, revealing the full list when the selected row is filtered out.

### Metadata
- Source: user_feedback
- Related Files: hnApplication/hn2d3dPixBaseWidget.cpp, hnRoadDataProcess/hnDiseaseListWidget.cpp
- Tags: cross-view-selection, target-scene, proxy-filter, disease-list

---

## [LRN-20260711-002] correction

**Logged**: 2026-07-11T15:10:00+08:00
**Priority**: high
**Status**: resolved
**Area**: frontend

### Summary
Little-frame display cells must map all four corners through one anchor image.

### Details
The user reported false long rectangles after the little-frame optimization. The merge builder was not active for the 42-cell reproduction because exact-cell rendering is used up to 256 cells. The real regression was that each corner of one stored grid cell independently resolved its image by mile before the four scene points were united into one QRectF. If a boundary or mirrored cell resolved different corners to different images, a normal cell became a long rectangle.

### Suggested Action
For little-frame display and single-cell highlight, resolve the cell image once from p0, map p0/p1/p2/p3 on that same image, and keep original vec2dRect/vec3dRect for hit testing and deletion.

### Metadata
- Source: user_feedback
- Related Files: hnApplication/hn2dPixWidget.cpp, hnApplication/hn3dPixWidget.cpp
- Tags: little-frame, scene-mapping, regression, false-long-rect
- See Also: LRN-20260711-001

---

## [LRN-20260713-003] correction

**Logged**: 2026-07-13T00:00:00+08:00
**Priority**: high
**Status**: in_progress
**Area**: frontend

### Summary
Diagnose visible long little-frame cells from the final runtime QRect, not merely from database cell counts or outer-bound rendering code.

### Details
The user identified a single light-green vertical rectangle among normal small grids. The initial diagnosis incorrectly attributed it to the blue outer-bound rectangle. The stored `DisPSB#270` cells decode as normal grids, and the screenshot color proves the anomalous rectangle is produced on the normal-cell path after coordinate conversion.

Later reproduction established that the same image-local drawing position is normal near the bottom of the viewport but can stretch near the middle/top. Existing database diseases can also stretch immediately after adding a new disease, then become normal after reimport. Runtime evidence for `DisPSB#337` changed from `379x3124` to `379x651` in one process while all seven stored cells remained about `126x130`; the extra 2473 pixels equal one 2603-pixel image height minus one 130-pixel cell.

The stricter image-alias cache fix did not solve the bug: the next reproduction had no `HN_SDK_IMAGE_LOOKUP_CACHE_REJECT` messages. `DisPSB#428` stored four normal 2D cells and four normal 3D cells, but its 2D runtime bounds were `253x2733` while 3D was `133x150`. The fourth 2D cell touches `y=0`; `2733 = 2603 image height + 130 cell height`. The remaining defect was four separate scene lookups per cell. Even with one image-name string, a boundary corner could still resolve onto an adjacent scene item.

### Suggested Action
For little-frame rendering in both 2D and 3D, resolve exactly one stable scene anchor per stored cell and derive the opposite corner from the image-local cell width and height. Do not run four independent scene-item lookups for four corners of one cell. Keep strict alias validation and cache clearing as defensive measures, but do not treat the cache as the root cause without rejection evidence.

### Metadata
- Source: user_feedback
- Related Files: hnApplication/hn2dPixWidget.cpp, hnApplication/drawDiseases.cpp
- Tags: little-frame, runtime-geometry, false-long-rect, diagnosis
- See Also: LRN-20260711-001, LRN-20260711-002

### Resolution
- **Resolved**: 2026-07-13T12:00:00+08:00
- **Notes**: The image-mile and strict-alias changes were insufficient. The verified structural fix maps one scene anchor per little-frame cell in both views, then applies the stored local width and height. Source compilation and an alternate-name full link passed; runtime UI verification still requires rebuilding the official DLL after the running application releases it.

---

## [LRN-20260711-001] best_practice

**Logged**: 2026-07-11T14:27:00+08:00
**Priority**: high
**Status**: resolved
**Area**: frontend

### Summary
An unresolved image alias must map all of its local points to one stable scene item.

### Details
The old fallback selected the first or last image item from each point's local Y coordinate. For a grid cell touching an image boundary, corners at y == 0 went to the first item while the other corners went to the last item, stretching a normal cell into a false long rectangle. The stored disease geometry remained correct.

### Suggested Action
Resolve or cache the fallback per image name, never per point coordinate, and validate boundary cells with vertical mirroring and cross-image diseases.

### Metadata
- Source: user_feedback
- Related Files: SDK/src/TiledGraphicsView.cpp, hnApplication/hn2dPixWidget.cpp
- Tags: little-frame, scene-mapping, image-boundary, regression

---

## [LRN-20260713-004] correction

**Logged**: 2026-07-13T15:25:00+08:00
**Priority**: high
**Status**: in_progress
**Area**: frontend

### Summary
3D little-frame rectangles may legitimately carry different source-image miles on different corners.

### Details
After the same-image single-anchor rendering fix, disease `DisPSB#490` rendered correctly in 2D (`127x781`) but as `67x4250` in 3D. The database contained six 3D rectangles; the first rectangle crossed the 504/512 image boundary, with its top corners at image mile 512 and bottom corners at image mile 504. `hn3dPointWithMileI::bottomEncoderMile` is stored per corner, so this is a valid cross-image representation rather than a corrupt rectangle. Treating the entire rectangle as belonging to p0's image turns the two valid local coordinate ranges into one full-image-height rectangle.

### Suggested Action
Use the stable single-anchor path only when all four corners have the same `bottomEncoderMile`. When a cell spans images, resolve and map every corner using its own image mile, then build the small scene rectangle from those global points. Apply the same distinction to rendering, hit testing, and cell scene bounds.

### Metadata
- Source: user_feedback
- Related Files: hnApplication/hn3dPixWidget.cpp, hnCommon/hn3dPointWithMile.h
- Tags: little-frame, 3d, cross-image, image-boundary, runtime-geometry
- See Also: LRN-20260713-003, LRN-20260711-002

### Resolution
- **Resolved**: 2026-07-13T15:25:00+08:00
- **Notes**: Implemented same-image versus cross-image branches in the three 3D SDK little-frame paths. Debug x64 source compilation and official DLL linking passed; UI verification of disease 490 is pending.

---
## [LRN-20260826-XLS] correction

**Logged**: 2026-08-26T18:30:00+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
For QXlsx customer exports, replacing shared formulas with normal formulas does not provide usable cached values.

### Details
The generated workbook contained the correct formulas but cached zero values, so PCI, grade, and statistics ratios still appeared wrong until recalculation. Customer-visible report outputs that must be correct on first open need direct numeric/text values when QXlsx cannot calculate formula caches.

### Suggested Action
Write the already-computed ratio, PCI, and evaluation grade values directly for report-only cells, and verify the newly generated XLSX rather than stopping at compile success.

### Metadata
- Source: user_feedback
- Related Files: hnRoadDataProcess/hnOutExcelManage.cpp
- Tags: qxlsx, excel, formula-cache, export

---

## [LRN-20260827-XLS] correction

**Logged**: 2026-08-27T08:55:00+08:00
**Priority**: high
**Status**: resolved
**Area**: backend

### Summary
When the report contract requires editable Excel formulas, do not replace them with fixed values to work around cached results.

### Details
The user explicitly requires statistics ratios, PCI, and evaluation cells to remain formulas. The earlier fixed-value workaround changed the workbook contract even though the displayed numbers were available immediately. QXlsx formulas should be written normally and Excel/WPS should perform recalculation when the workbook opens.

### Suggested Action
Preserve formulas whenever the requested workbook is intended to remain auditable and recalculable. Treat formula cache behavior as a separate compatibility concern and do not silently change formulas into results.

### Metadata
- Source: user_feedback
- Related Files: hnRoadDataProcess/hnOutExcelManage.cpp
- Tags: qxlsx, excel, formulas, export-contract

---

## [LRN-20260901-001] correction

**Logged**: 2026-09-01T10:30:00+08:00
**Priority**: high
**Status**: resolved
**Area**: config

### Summary
Compare every `rutcfg*.ini` variant and its timestamps before claiming two rut projects used identical calculation settings.

### Details
The first audit compared only the active `camera0/rutcfg.ini` files. The user correctly pointed out that the 2026 project also retained `rutcfg - 副本.ini`; it preserves different `rutastart=150` and `rutcend=2360` values, while the active file was modified later to match the 2025 values `230` and `2230`.

### Suggested Action
Enumerate all configuration variants, compare hashes and exact parameter deltas, and use creation/modification times plus output times to distinguish acquisition-time settings from later reprocessing settings.

### Metadata
- Source: user_feedback
- Related Files: customer project camera0/rutcfg.ini
- Tags: rut, configuration, provenance, comparison

---

## [LRN-20260901-002] insight

**Logged**: 2026-09-01T10:45:00+08:00
**Priority**: high
**Status**: pending
**Area**: backend

### Summary
A missing optional speed file can silently prevent all RDI initialization and make the export write fixed 100 values.

### Details
When `outSpeedAndMarkExcel=True`, `writeSpeedValue()` returns false if neither DAQ speed file contains data. `handelRoadSplietVec()` then returns before `StartCalculate()` and `calculateRUTScore()`. Callers ignore the false result and still export, while `getRutExcelStr()` converts the empty RDI formula to literal `100` and the evaluation column stays blank.

### Suggested Action
Do not gate independent RDI calculation on optional speed output. Propagate `initSegmentInterval()` failure to the report caller, and replace the silent `100` fallback with a concrete Chinese error that names the missing speed paths or parameter lookup failure.

### Metadata
- Source: error
- Related Files: hnRoadDataProcess/hnOutExcelMileManage.cpp, hnRoadDataProcess/hnOutExcelMile.cpp, hnRoadDataProcess/hnOutExcelManage.cpp
- Tags: export, rdi, speed, error-propagation, silent-fallback

---
