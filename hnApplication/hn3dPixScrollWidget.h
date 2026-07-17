#pragma once
#include "hn3dPixWidget.h"
#include <QWidget>
#include "hnContinuouslyBrowsePixWidget.h"
#include <QSharedPointer>
#include "hnapplication_global.h"
class HNAPPLICATION_EXPORT hn3dPixScrollWidget : public hnContinuouslyBrowsePixWidget
{
	Q_OBJECT

public:
	hn3dPixScrollWidget(QWidget *parent = Q_NULLPTR);
	~hn3dPixScrollWidget();

public:
	// 加载影像
	void load3dImage();

	//加载图片，并保留当前位置 此函数用于切换浏览模式后调用
	void laodPicRetainScrollBarValue();

	hn3dPixWidget *getPixWidget();
private:
	void initConnect();
protected:
void	keyPressEvent(QKeyEvent *event) override;

int browseStep()   const;

bool is2DView() const;
bool stepOneImage() override;
void mousePressEvent(QMouseEvent *event) override;
public slots:
void slot_BlockValueChanged(int) override;
void slot_JumpToUserMile() override;

void slot_Show3dDeepExample() override;
private:
	hn3dPixWidget* m_p3dImageViewWidget;
};
