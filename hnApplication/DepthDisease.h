#include <QString>
class DepthDisease
{
public:
	DepthDisease();
	~DepthDisease();
public: 
	QString getName();
	QString getGrad();
	int getThreshold();
	void setName(QString name);
	void setGrad(QString grad);
	void setThreshold(int value);

	QString getFrameType() const;
	void setFrameType(const QString &frameType);

	int m_thresholdUp;
	int m_thresholdDown;
	//如果这个值不为-1  说明 判断条件为    深度或者面积
	int  m_thresholdAreaUp;
	int  m_thresholdAreaDown;
private:
	QString m_name;
	QString m_grad; 
	//框选类型  人工模式/自动化模式
	QString m_frameType;

};
