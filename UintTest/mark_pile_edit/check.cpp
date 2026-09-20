#include <QApplication>
#include <QTextCodec>
#include <QTextStream>
#include <QDir>
#include <QTimer>
#include <QTableWidget>
#include <limits>
#include "hnMarkPileEditDialog.h"
#include "projectView.h"
#include "../hnProject/hnProjectManager.h"
#define CHECK(x) do { if(!(x)) { out<<"FAIL "<<__LINE__<<" "<<#x<<" "<<error<<"\n";out.flush();return 1;} } while(0)
int main(int argc,char**argv)
{
 QApplication app(argc,argv);QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));QTextStream out(stdout);out.setCodec("UTF-8");QString error;
 if(argc<2)return 2;QString root=QString::fromLocal8Bit(argv[1]);if(!root.contains("_codex_build_verify/mark-pile-edit/"))return 2;
 HnXRSettings::getInstance()->SetConfigFilePath(QDir(root).filePath("test-settings.ini"));HnXRSettings::getInstance()->Init();HnXRSettings::getInstance()->mile2dmiToInt=false;
 auto data=hnApp::hnDataManager::getDataManager();CHECK(data->initRoadStandardInfo());
 vector<hnProjectDataInfo> projects;PROJECT_TYPE type=PROJECT_2D_TYPE;CHECK(data->getAllProject(root,projects,type));data->setProjectDiseaseVector(projects);
 CHECK(data->initProject(projects));data->getProjectManager()->initAllProjectMileVector();
 auto project=data->getProjectManager()->getAllBaseProject().front();CHECK(data->setCurrentProject(project->get2DProName()));
 auto settings=project->getCurProSetInfo();int dir=settings.nLineType;double start=settings.dBegMile;auto db=project->getDB();
 hnMarkInfo original;hnMilePile originalPile;
 for(auto m:project->getCurrentMarkVector())if(m.nID==100)original=m;
 for(auto p:project->getCurrentMilePileVector())if(p.nID==2)originalPile=p;
 CHECK(original.nID==100&&originalPile.nID==2);
 if(argc>2){CHECK(qAbs(originalPile.dTrueMile-(start+dir*520))<0.001);CHECK(original.dEnclMile==600);CHECK(qAbs(original.dTrueMile-project->enclToTrueMile(600))<0.001);out<<"PASS reopen\n";return 0;}
 CHECK(qAbs(originalPile.dTrueMile-(start+dir*500))<0.001);
 hnMarkPileEditDialog cancel(project,false,100);cancel.findChild<QDoubleSpinBox*>("editTrueMile")->setValue(start+dir*650);cancel.reject();
 for(auto m:project->getCurrentMarkVector()) if(m.nID==100) { CHECK(m.dTrueMile==original.dTrueMile); }
 auto mark=original;CHECK(project->updateMark(100,mark,&error));
 mark.dTrueMile=start+dir*1200;CHECK(!project->updateMark(100,mark,&error));mark.dTrueMile=std::numeric_limits<double>::quiet_NaN();CHECK(!project->updateMark(100,mark,&error));
 out<<"CODEC "<<QTextCodec::codecForLocale()->name()<<" originalHex "<<QByteArray(original.strMark).toHex()<<"\n";out.flush();mark=original;mark.dTrueMile=start+dir*700;strcpy_s(mark.strMark,QStringLiteral("另一记录").toLocal8Bit().constData());CHECK(!project->updateMark(100,mark,&error));
 strcpy_s(mark.strMark,QStringLiteral("不同说明").toLocal8Bit().constData());CHECK(project->updateMark(100,mark,&error));
 mark.nType=0;mark.dTrueMile=start+dir*800;strcpy_s(mark.strMark,QStringLiteral("水泥").toLocal8Bit().constData());CHECK(!project->updateMark(100,mark,&error));
 mark.nType=4;CHECK(project->updateMark(100,mark,&error));CHECK(project->updateMark(100,original,&error));
 CHECK(db->executeDB("BEGIN IMMEDIATE"));mark=original;mark.dTrueMile=start+dir*620;CHECK(!project->updateMark(100,mark,&error));CHECK(db->executeDB("ROLLBACK"));
 for(auto m:project->getCurrentMarkVector())if(m.nID==100) { CHECK(m.dTrueMile==original.dTrueMile); }
 out<<"PASS mark cancel, unchanged, bounds, NaN, duplicate, type conflict, rollback\n";out.flush();
 auto candidate=originalPile;CHECK(project->updateMilePile(2,candidate,&error));CHECK(!project->updateMilePile(0,candidate,&error));
 candidate.dTrueMile=start+dir*1300;CHECK(!project->updateMilePile(2,candidate,&error));candidate=originalPile;candidate.dEnclMile=-2;CHECK(!project->updateMilePile(2,candidate,&error));
 candidate.dEnclMile=std::numeric_limits<double>::infinity();CHECK(!project->updateMilePile(2,candidate,&error));
 hnMilePile other;other.dTrueMile=start+dir*700;other.dEnclMile=700;CHECK(project->addMilePile(other,&error));
 candidate=originalPile;candidate.dTrueMile=other.dTrueMile;CHECK(!project->updateMilePile(2,candidate,&error));
 candidate=originalPile;candidate.dEnclMile=700;CHECK(!project->updateMilePile(2,candidate,&error));
 candidate=originalPile;candidate.dTrueMile=start+dir*750;CHECK(!project->updateMilePile(2,candidate,&error));CHECK(project->deleteMilePile(other.nID,&error));
 CHECK(db->executeDB("CREATE TRIGGER reject_edit BEFORE INSERT ON MARK_INFO BEGIN SELECT RAISE(ABORT,'test rollback'); END;"));
 candidate=originalPile;candidate.dTrueMile=start+dir*520;error.clear();CHECK(!project->updateMilePile(2,candidate,&error));CHECK(db->executeDB("DROP TRIGGER reject_edit"));
 vector<hnMilePile> disk;CHECK(db->m_milePileTable.readData(disk));for(auto p:disk)if(p.nID==2) { CHECK(p.dTrueMile==originalPile.dTrueMile); }
 for(auto p:project->getCurrentMilePileVector())if(p.nID==2) { CHECK(p.dTrueMile==originalPile.dTrueMile); }
 out<<"PASS pile anchors, unchanged, bounds, duplicate stake/DMI, crossing, atomic rollback\n";out.flush();

 hnMarkPileEditDialog move(project,true,2);move.findChild<QGroupBox*>("editPositionGroup")->setChecked(true);
 move.findChild<QDoubleSpinBox*>("editDmi")->setValue(510);move.findChild<QDoubleSpinBox*>("editTrueMile")->setValue(start+dir*510);
 move.accept();CHECK(move.result()==QDialog::Accepted);
 for(auto p:project->getCurrentMilePileVector())if(p.nID==2) { CHECK(p.dEnclMile==510); }
 CHECK(project->updateMilePile(2,originalPile,&error));
 hnMarkPileEditDialog cancelPile(project,true,2);cancelPile.findChild<QDoubleSpinBox*>("editTrueMile")->setValue(start+dir*525);cancelPile.reject();
 for(auto p:project->getCurrentMilePileVector())if(p.nID==2) { CHECK(p.dTrueMile==originalPile.dTrueMile); }
 out<<"PASS position edit and pile cancel\n";
 hnMarkPileEditDialog dialog(project,true,2);dialog.setAttribute(Qt::WA_DontShowOnScreen);dialog.show();app.processEvents();dialog.grab().save(QDir(root).filePath("pile-edit.png"));
 dialog.findChild<QDoubleSpinBox*>("editTrueMile")->setValue(start+dir*1200);dialog.accept();CHECK(dialog.result()!=QDialog::Accepted);CHECK(!dialog.findChild<QLabel*>("editError")->text().isEmpty());
 dialog.findChild<QDoubleSpinBox*>("editTrueMile")->setValue(start+dir*520);dialog.accept();CHECK(dialog.result()==QDialog::Accepted);
 for(auto m:project->getCurrentMarkVector())if(m.nID==100){CHECK(m.dEnclMile==600);CHECK(qAbs(m.dTrueMile-project->enclToTrueMile(600))<0.001);}
 hnMarkPileEditDialog md(project,false,100);md.setAttribute(Qt::WA_DontShowOnScreen);md.show();app.processEvents();md.grab().save(QDir(root).filePath("mark-edit.png"));

 md.findChild<QComboBox*>("editMarkContent")->setEditText("");md.accept();CHECK(md.result()!=QDialog::Accepted);
 md.findChild<QComboBox*>("editMarkContent")->setEditText(QString(300,QChar('x')));md.accept();CHECK(md.result()!=QDialog::Accepted);
 md.findChild<QComboBox*>("editMarkContent")->setEditText(QStringLiteral("修改后的备注"));md.accept();CHECK(md.result()==QDialog::Accepted);
 projectView view;view.setAttribute(Qt::WA_DontShowOnScreen);view.slot_updateProjectSetting(settings,project->getCurrentMarkVector(),project->getCurrentMilePileVector());
 auto table=view.findChild<QTableWidget*>("pileTableWidget");CHECK(table);int row=-1,anchor=-1;
 for(int i=0;i<table->rowCount();++i){int id=table->item(i,0)->data(Qt::UserRole).toInt();if(id==2)row=i;if(id==0)anchor=i;}
 CHECK(row>=0&&anchor>=0);table->selectRow(row);CHECK(view.findChild<QPushButton*>("editSelectedPile")->isEnabled());
 bool saved=false;QTimer::singleShot(0,[&](){for(auto w:QApplication::topLevelWidgets())if(w->objectName()=="markPileEditDialog"&&w->isModal()){auto e=dynamic_cast<hnMarkPileEditDialog*>(w);e->accept();saved=e->result()==QDialog::Accepted;}});
 view.editPile();CHECK(saved);CHECK(table->item(table->currentRow(),0)->data(Qt::UserRole).toInt()==2);
 for(int i=0;i<table->rowCount();++i)if(table->item(i,0)->data(Qt::UserRole).toInt()==0)anchor=i;
 table->selectRow(anchor);CHECK(!view.findChild<QPushButton*>("editSelectedPile")->isEnabled());
 out<<"PASS dialogs, linked stake, selection, system protection; direction "<<dir<<"\n";out.flush();return 0;
}
