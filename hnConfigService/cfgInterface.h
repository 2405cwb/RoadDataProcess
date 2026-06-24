#include <QString>
 class CfgInterface
{
public:
  virtual  QString  ReadString(const QString&  Section, const QString&  Key, const QString&  Default)=0;
  virtual int ReadInteger(const QString&  Section, const QString&  Key, int Default)=0;
  virtual bool ReadBool(const QString&  Section, const QString&  Key, bool Default)=0;
  virtual double ReadDouble(const QString&  Section, const QString&  Key, double Default)=0;
  virtual void WriteInteger(const QString&  Section, const QString&  Key, int Value)=0;
  virtual void WriteString(const QString&  Section, const QString&  Key, const QString&  Value)=0;
  virtual void WriteBool(const QString&  Section, const QString&  Key, bool Value)=0;
  virtual void WriteDouble(const QString&  Section, const QString&  Key, double Value)=0;
  virtual ~CfgInterface() = default;

private:

};
