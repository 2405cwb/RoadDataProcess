win32:CONFIG += win64
QT       += widgets concurrent sql
TARGET    = TunnelViewerSDK
TEMPLATE  = lib
CONFIG   += staticlib  # 🟢 声明为静态库，方便同事直接集成

# 🟢 2. C++ 标准配置
CONFIG += c++11 console
# 如果用的是较新的 Qt6，可能需要 c++17
# CONFIG += c++17
 
 
# 定义头文件搜索路径，方便内部引用
INCLUDEPATH += $$PWD/include \
               $$PWD/include/items \
               $$PWD/include/tools \
               $$PWD/src \
               $$PWD/../3rd/opencv3.2.0/include \
               $$PWD/third_party/ImagePackSDK/include

win32-msvc*:CONFIG(debug, debug|release) {
    LIBS += -L$$PWD/../3rd/opencv3.2.0/lib -lopencv_world320d
    QMAKE_POST_LINK += $$quote(cmd /c copy /Y $$shell_path($$PWD/third_party/ImagePackSDK/bin/x64/Debug/ImagePack.dll) $$shell_path($$OUT_PWD/ImagePack.dll))
}

win32-msvc*:CONFIG(release, debug|release) {
    LIBS += -L$$PWD/../3rd/opencv3.2.0/lib -lopencv_world320
    QMAKE_POST_LINK += $$quote(cmd /c copy /Y $$shell_path($$PWD/third_party/ImagePackSDK/bin/x64/Release/ImagePack.dll) $$shell_path($$OUT_PWD/ImagePack.dll))
}

# 公开给同事的接口头文件
HEADERS += \
    include/TunnelGlobal.h \
    include/ImageDisplayAdjustments.h \
    include/ImageCacheManager.h \
    include/ImageCoordinateMapper.h \
    include/ImageLayoutManager.h \
    include/ImageSource.h \
    include/AbstractSourceFactory.h \
    include/AbstractTileSource.h \
    include/IDefectStorage.h \
    include/DefectManager.h \
    include/ImagePackReader.h \
    third_party/ImagePackSDK/include/ImagePackApi.h \
    include/WholeImageTileSource.h \
    include/WholeImageSourceFactory.h \
	include/VirtualImageSequence.h \
    include/tools/AbstractTool.h \
    include/tools/GridSelectionTool.h \
    src/TiledGraphicsView.h \
    src/TunnelViewerController.h \
    src/TunnelSectionItem.h \
	include/items/DefectShapeItem.h \
	src/AsyncImageLoader.h

# 内部实现文件
SOURCES += \
    src/TiledGraphicsView.cpp \
    src/TunnelViewerController.cpp \
    src/TunnelSectionItem.cpp \
    src/AsyncImageLoader.cpp \
    src/ImagePackReader.cpp \
    src/GridSelectionTool.cpp \
	src/VirtualImageSequence.cpp \
	include/items/DefectShapeItem.cpp
	
win32-msvc* {
    
    # 强制 UTF-8，解决乱码
    QMAKE_CXXFLAGS += /utf-8
}
