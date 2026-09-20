# 打标、校桩修改回归验证

使用 Qt 5.8 / VS2015 v140，链接最新 Release x64 的 hnProject、hnDataTable、hnApplication。
测试入口为 check.cpp；check.pro 可用 qmake/nmake 编译，测试程序目录需要 RoadParamDB。
只允许传入 `_codex_build_verify/mark-pile-edit/` 下的独立工程副本，禁止传入客户原件。

## 本次验证

构建及运行日志位于 `_codex_build_verify/mark-pile-edit`。up/down 副本均通过；第二次传入 `reopen` 参数验证持久化。

- 桩号 6514～7574，采集长度 1060 米；下行反转起终点。
- 校桩 ID 0/1 为系统起终点，ID 2 位于 DMI 500、相应桩号。
- 打标 ID 100/101 为 DMI 600/700 的情况说明，ID 102 为 DMI 800 的材质记录。
- 测试编辑取消、原值保存、无效数字、越界、重复桩号/位置、校桩顺序交叉、同类型属性冲突。
- SQLite 触发器拒绝打标写入，验证校桩与打标整体回滚；事务开启失败验证打标原记录保留。
- 通过真实 Qt 弹窗保存位置、内容，验证失败时不关闭、列表选中 ID 保留、系统锚点按钮禁用。
- 重新打开验证修改持久化；对比所有其他数据库表，确认病害等记录未改动。

源码中文采用 UTF-8 BOM，以便 v140 正确识别；被测旧 Qt/C++ 文件保留原 CP936 编码。
