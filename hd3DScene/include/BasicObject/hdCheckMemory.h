// 只能包含在Cpp文件中

#ifdef _DEBUG
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#endif

//与下句代码对应，最好放在构造函数中
// 检测内存泄露
// _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

//// 或者直接采用MFC的内存检测语句
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif
