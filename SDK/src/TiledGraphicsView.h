#ifndef TILEDGRAPHICSVIEW_H
#define TILEDGRAPHICSVIEW_H
#include<QCoreApplication> 
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QElapsedTimer>
#include <QTimer>
#include <QHash>
#include <vector>
#include "TunnelSectionItem.h"
#include "TunnelGlobal.h" 
#include "DefectManager.h"
#include <opencv2/opencv.hpp>

using namespace std;

class DefectDrawTool; // Forward declaration

// Bottom anchor used by application code to map SDK pixels to business mileage.
struct TiledViewAnchor
{
	bool valid = false;
	QString imageName;
	int imageIndex = -1;
	QPointF scenePos;
	QPointF imagePixelPos;
};

/**
 * @brief          (SDK    )
 *
 *      
 * 1.            (addLayer)
 * 2.    LOD         (onScroll)
 * 3.        WASD   Ctrl+         
 * 4.           (      )
 */
class TiledGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
	enum RenderBackend
	{
		RenderBackend_Raster,
		RenderBackend_OpenGL
	};

    // Static initialization. Call before QApplication when high-DPI scaling is needed.
    static void init() {
                #if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
                // Must be called before QApplication is created.
                QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
                #endif
	}


    explicit TiledGraphicsView(QWidget* parent = nullptr);
    ~TiledGraphicsView();

    // ==========================================
    // SDK core API
    // ==========================================

    /**
     * @brief          
     *                         (Scene)      
     * @param imageName   /    
     * @param localX        X   
     * @param localY        Y   
     * @return        Scene               QPointF(0,0)
     */
    QPointF mapToGlobalScene(const QString& imageName, qreal localX, qreal localY);
	bool GlobalSceneToMap(QPointF pt, QString& imageName, int& localX, int& localY);
	// Discard image-name aliases accumulated by drawing/hit testing before a full overlay rebuild.
	void clearImageItemLookupCache();
 
    // Focus on a global scene position. targetScale > 0 applies zoom after jump.
    void focusOnPosition(const QPointF& scenePos, double targetScale = -1.0);

	// Current view center in scene coordinates.
	QPointF currentCenterScenePos() const;

	// Current view center scene Y, used by vertical-stitch mileage mapping.
	double currentCenterSceneY() const;

	// Current bottom-center point in scene coordinates.
	QPointF currentBottomCenterScenePos() const;

	// Current bottom-center anchor image and local pixel.
	TiledViewAnchor currentBottomAnchor() const;

	// Scroll to scene Y while keeping current X as much as possible.
	void scrollToSceneY(double sceneY);

	// Place the specified image pixel Y at the bottom of the viewport.
	void scrollToImagePixel(int imageIndex, double pixelY, bool anchorBottom = true);

	// Locate by image name and local pixel; more stable than item index.
	void scrollToImagePixel(const QString& imageName, double pixelY, bool anchorBottom = true);

    /**
     * @brief         
     * @param source       (     )
     *                
     */
    void addLayer(AbstractTileSource* source);

    /**
     * @brief       
     * @param mode Mode_Browse(  )   Mode_Draw(  )
     */
    void setViewMode(ViewMode mode);

    /**
     * @brief       
     */
    void clear();

    /**
     * @brief          (          Item)
     */
    QGraphicsScene* scene() const { return m_scene; }


    // Allow application code to enable or disable built-in keyboard navigation.
    void setKeyboardNavigationEnabled(bool enable) { m_enableKeyNav = enable; }

	// Render backend. Raster is the default stable mode for complex QWidget UIs.
	void setRenderBackend(RenderBackend backend);
	RenderBackend renderBackend() const { return m_renderBackend; }

    // Image sequence layout orientation.
    void setLayoutOrientation(LayoutOrientation orientation) {
        m_orientation = orientation;
    }

    void set_scrollSpeed(int speed)
    {
		m_scrollSpeed = speed;
    }

	// Set view name.
	void setViewName(QString strName);
    
	// Get view name.
	QString getViewName();

	// Tell SDK what shape to draw.
	void startDrawingDefect(DrawShape shapeType);
	void setDrawingGeometry(DrawShape shapeType);

	// Internal callback used by draw tool.
	void emitGeometryDrawn(DrawShape shapeType, const QPainterPath& path) {
		emit sigGeometryDrawn(shapeType, path); 
	}

    // Set LOD level (1-5).
    // 1: conservative loading; 5: aggressive loading.
    void setLodLevel(int level);


    void updateVisibleTiles();          // LOD     

	void undoLastDrawPoint();

	void cancelCurrentDrawing();

	// Image dimensions.
	double getImageHeight();
	double getImageWidth();

	DefectManager* defectManager() const { return m_defectManager; }
	DefectManager* cp3Manager() const { return m_vecCp3Manager; }
	DefectManager* platformManager() const { return m_vecPlatformManager; }
	DefectManager* chainManager() const { return m_vecChainManager; }
	DefectManager* tunnelLocManager() const { return m_vecTunnelLocManager; }
	DefectManager* ringInfoManager() const { return m_vecRingInfoManager; }
	DefectManager* sectionManager() const { return m_vecSectionManager; }
	DefectManager* reAutoRingManager() const { return m_vecReAutoRingManager; }
	ViewMode viewMode() const { return m_currentMode; }

	// Set highlighted element.
	void setHighLightElement(int uuid, ElementType ele);

	/**
	*                           (    )
	*  sceneRect              
	*  quality      (      )
	*/
	QPixmap exportRegionData(const QRectF& sceneRect, ExportQuality quality = Export_HighRes, bool drawDefects = true);
	 
	QPixmap exportRegionDataToWord(const QRectF& sceneRect, ExportQuality quality = Export_HighRes, bool drawDefects = true);

	cv::Mat exportRegionGrayMat(const QRectF& sceneRect,
		ExportQuality quality = Export_HighRes,
		bool drawDefects = true);

	cv::Mat exportRegionMat(const QRectF& sceneRect,
		ExportQuality quality = Export_HighRes,
		bool drawDefects = true,
		bool returnBgr = true);

	// Get Mat by image name.
	cv::Mat getMatByImageName(QString qstrImageName);

	// Access underlying image items.
	QList<TunnelSectionItem*> getTunnelSectionItem() { return m_items; }   //        

public slots:
    // ?? 1.                        
    void resetToFit();
	void updateHUD();
signals:
    // ==========================================
    //        (     UI   )
    // ==========================================

	//                                  
	void sigGeometryDrawn(DrawShape shapeType, QPainterPath path);

    /**
     * @brief          (          Label)
     * @param info            "  : Tunnel_01 |   : (100, 200)"
     */
     // ??                    
    void sigCursorInfoChanged(QString imgName, int x, int y);

	/**
	* @brief            
	* @param info            "  : Tunnel_01 |   : (100, 200)"
	*/
	void sigViewInfoChanged(QString imgNameTL, int xTL, int yTL, QString imgNameBR, int xBR, int yBR);

    /**
     * @brief          (     HUD)
     * @param stats    FPS                 
     */
    void sigStatsUpdated(QString stats);

    // ??                                  
    void sigContextMenuRequested(QPoint globalPos, QList<QGraphicsItem*> itemsUnderMouse);

	// ??                                  
	void sigDoubleClickedLeft(QPointF globalPos, QList<QGraphicsItem*> itemsUnderMouse);

	// 
	void sigDefectsSelected(QList<DefectShapeItem*> selectedDefects);

	//                    
	void sigRegionExported(QPixmap pixmap);

	//                     double sceneY       
	void sigViewCenterSceneChanged(QPointF centerScenePos);

	//                             
	void sigViewBottomAnchorChanged(TiledViewAnchor anchor);

	//                                       
	void sigUserViewBottomAnchorChanged(TiledViewAnchor anchor);
protected:
    // ==========================================
    // ??      
    // ==========================================
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;


    //              
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

	void mouseDoubleClickEvent(QMouseEvent* event) override;

    //      WASD   
    void keyPressEvent(QKeyEvent* event) override;

	void keyReleaseEvent(QKeyEvent* event) override;

private:

  void  setupGraphicsView();

  void setupConnections();

  //        viewport        new QOpenGLWidget 
  void applyRenderBackend();

  //                              
  void emitViewCenterChanged();

  //                      scrollTo             
  void emitUserViewBottomAnchorChanged();
    // ---        ---

    double getFitScale() const;   //             

    //      (      )
    QList<QGraphicsItem*> getVisualItems(QPoint viewPos);

	//         
	DrawShape m_curDrawShape;

	 

private:
	//       
	DefectManager* m_vecCp3Manager = nullptr;

	//      
	DefectManager* m_vecPlatformManager = nullptr;

	//       
	DefectManager* m_vecChainManager = nullptr;

	//        
	DefectManager* m_defectManager = nullptr;

	//        
	DefectManager* m_vecTunnelLocManager = nullptr;

	//      
	DefectManager* m_vecRingInfoManager = nullptr;

	//      
	DefectManager* m_vecSectionManager = nullptr;

	//          
	DefectManager* m_vecReAutoRingManager = nullptr;

	//    
	ViewMode m_currentMode = Mode_Browse;
	
private:
	QPoint m_lastMousePos; //          

	bool  m_isPanning =false; 

private:
    QGraphicsScene* m_scene;
    QList<TunnelSectionItem*> m_items; //        

    //        item           /              m_items 
    mutable QHash<QString, TunnelSectionItem*> m_imageItemCache;
    QString DrawModelStr;
    //     
    double m_currentScale = 1.0;

    //     
    QElapsedTimer m_perfTimer;

    //         
    QTimer* m_debounceTimer;

    LayoutOrientation m_orientation;

	RenderBackend m_renderBackend = RenderBackend_Raster;


	DefectDrawTool* m_currentTool = nullptr; //                  

    //      (  / )
    int m_scrollSpeed  ;
    bool m_enableKeyNav = true; //     
    bool m_isFastScrolling = false; // ??            

    //     1.0 (     3)
    //                
    double m_lodThresholdMultiplier = 1.0;

	//            
	bool m_isDraggingDefects = false; 

	//               
	QPointF m_lastDragScenePos; 

	//    1680*1680       
	QGraphicsRectItem * m_exportBoxItem = nullptr;

	int exportBoxSize = 1680;
protected:
	QString m_strViewName;
     
};

#endif // TILEDGRAPHICSVIEW_H
