#ifndef TUNNELVIEWERCONTROLLER_H
#define TUNNELVIEWERCONTROLLER_H

#include <QObject>
#include <QGraphicsView>
#include <QProgressDialog> // 🟢 引入进度条
#include <QApplication>    // 🟢 引入 App
#include <QSharedPointer>
#include "TunnelGlobal.h"
// 只要前置声明，不需要包含具体头文件，编译更快
class TiledGraphicsView;
class AbstractSourceFactory;
class ISequenceFrameSource;

class TunnelViewerController : public QObject
{
    Q_OBJECT
public:
    // 构造函数传入 View，控制器就接管这个 View 的内容了
    explicit TunnelViewerController(TiledGraphicsView* view, QObject* parent = nullptr);
    ~TunnelViewerController();

    // 1. 设置数据源工厂（ 
    void setSourceFactory(AbstractSourceFactory* factory);

    // 2. 核心接口：一键加载整个项目
	// path: 包含所有切片文件夹的根目录，例如 "C:/Projects/Tunnel_01"
	bool loadRoute(const QString& rootPath);
	// 旧项目已经把图片顺序排好了，就走这个入口直接加载，不再让 SDK 重新猜目录结构。
	bool loadImages(const QStringList& imagePaths);
	bool loadImages(const QStringList& imagePaths, const SequenceLoadOptions& options);


	bool loadPackRoute(const QString& packPath, const PackRouteOptions& options = PackRouteOptions());


	QList<PackRouteFrameInfo> packFrameInfos() const;
	// Original image names intersecting the current SDK viewport.
	QStringList visibleImageFileNames() const;
	QList<DbImageInfo> scanDatabaseFolder(const QString& rootFolder);

    // 老 TunnelViewer 卷帘对比接口：主路线和对比路线共用同一套数据库扫描器。
    bool loadCurtainCompareRoute(const QString& compareRootPath);
    bool setCurtainCompareEnabled(bool enabled);
    bool isCurtainCompareEnabled() const;
    void setCurtainOrientation(CurtainOrientation orientation);
    void setCurtainPosition(qreal ratio);
    void clearCurtainCompare();
	void setMaxRouteSections(int maxSections);
	int maxRouteSections() const;

    QString getRootPath()
		const {
		return m_rootPath;
	}
    // 4. 清空场景
    void clear();

	// 区间图像名
	QString m_strBegImageName;
	QString m_strEndImageName; 

    QStringList m_curTunnelNames;

private:
	void restoreViewUpdates(bool blockSignals);
    // 单图与 Pack 的统一显示入口。两种数据源只在 ISequenceFrameSource 的解码实现上不同。
    bool loadSequenceSource(const QSharedPointer<ISequenceFrameSource>& source,
        const SequenceLoadOptions& sequenceOptions, LayoutOrientation orientation,
        bool hMirrored, bool vMirrored, int scrollSpeed);
    QList<DbImageInfo> scanDatabaseFolderInternal(const QString& rootFolder, bool applyCurrentImageRange);

    TiledGraphicsView* m_view;
    AbstractSourceFactory* m_factory;

    //图片根目录
    QString m_rootPath; 
	int m_maxRouteSections;
	QList<PackRouteFrameInfo> m_packFrameInfos;

    // 记录当前加载到了多长（像素），方便追加
    //double m_currentTotalLength;
};

#endif // TUNNELVIEWERCONTROLLER_H
