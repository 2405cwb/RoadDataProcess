#pragma once

#include "hnContinuouslyBrowsePixWidget.h"  
#include "hn2dPixWidget.h"
#include "hnapplication_global.h"

//HNAPPLICATION_EXPORT
class HNAPPLICATION_EXPORT hn2dPixScrollWidget : public hnContinuouslyBrowsePixWidget
{
	Q_OBJECT

public:
	hn2dPixScrollWidget(QWidget *parent = Q_NULLPTR);
	
	//加载路面影像
	void loadRoadPicture();
private:
	void initConnect();
protected:
	void keyPressEvent(QKeyEvent *event) override;

	int browseStep()   const;

	bool is2DView() const;
public slots:
       void slot_BlockValueChanged(int) override;

	   void slot_JumpToUserMile() override;

	   void slot_Show3dDeepExample() override;

	   void slot_roadMileAndDmiChanged(int index);
	   void slot_sdkBottomEncoderMileChanged(double encoderMile);
	   
public:
	hn2dPixWidget * getPixWidget();

signals:

	//进度条发生变化传出 桩号与里程
	void signal_roadMileAndDmiChanged(double mile, double dmi);
 

private:
	hn2dPixWidget *m_roadDamageBrowserPixWidget;

};
