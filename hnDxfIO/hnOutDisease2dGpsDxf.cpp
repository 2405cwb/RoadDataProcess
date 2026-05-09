#include "stdafx.h"
#include "hnOutDisease2dGpsDxf.h"
#include <unordered_set>

hnOutDisease2dGpsDxf::hnOutDisease2dGpsDxf()
{
}


hnOutDisease2dGpsDxf::~hnOutDisease2dGpsDxf()
{
}


bool hnOutDisease2dGpsDxf::outDiseaseDxf(const char* filePath, std::vector<hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>*> diss, int disType)
{
	DL_Dxf* dxf = new DL_Dxf();
	DL_Codes::version exportVersion = DL_Codes::AC1015;

	DL_WriterA* dw = dxf->out(filePath, exportVersion);
	if (dw == NULL)
	{
		MessageBox(NULL, L"dxf无法打开", NULL, MB_OK);
		return false;
	}
	initCAD(dxf, dw);
	dw->sectionEntities();
	//对病害进行分类 统计有多少类别的
	std::unordered_set<std::string> uniqueNames;
	for (auto dis : diss)
	{
		std::string fullName = dis->s_diseaseName;
		auto index = fullName.find('.');
		std::string name;
		if (index != std::string::npos)
		{
			name = fullName.substr(0, index);
		}
		else
		{
			name = fullName;
		}

		uniqueNames.insert(name);
	}
	//创建图层
	dw->tableLayers(uniqueNames.size());
	{
		for (auto name : uniqueNames)
		{
			dxf->writeLayer(*dw,
				DL_LayerData(name, 0),
				DL_Attributes(
					std::string(""),
					DL_Codes::red,
					50,
					"CONTINUOUS", 1.0));
		}
		dw->tableEnd();
	}

	dw->sectionEntities();
	for (int diseaseIndex = 0; diseaseIndex < diss.size(); ++diseaseIndex)
	{
		hnCommon::hn2dDiseaseDef<hnCommon::hn2dGpsPoint>* disease = diss.at(diseaseIndex);
		//判断 病害所在图层名称
		std::string disName = disease->s_diseaseName;

		unsigned vertexCount = 4;
		int flags = 1;
		// flags |= 8;  //3D Polyline

		//获得图层名称
		std::string layoutName;
		for (auto tempName : uniqueNames)
		{
			if (disName.find(tempName) != std::string::npos)
			{
				layoutName = tempName;
				break;
			}
			else
			{
				layoutName = disName; //不可能发生
			}
		}
		double xLength = disease->d1.x - disease->d0.x;
		double yLenght = disease->d1.y - disease->d0.y;
		double length = sqrt(xLength*xLength + yLenght * yLenght);
		double slops = yLenght / xLength;
		double angleRadians = atan2(yLenght, xLength);
		double textRatio = length * 0.1;

		//将弧度转为度数
		double textAngle = angleRadians * 180.0 / M_PI;
		if (textAngle <= 0)
		{
			textAngle += 360;
		}
		textAngle = textAngle / 180.0*M_PI;
		//写入多段线
		dxf->writePolyline(*dw, DL_PolylineData(static_cast<int>(vertexCount), 0, 0, flags), DL_Attributes(layoutName, 256, -1.0, "BYLAYER", 8.0));
		dxf->writeVertex(*dw, DL_VertexData(disease->d0.x, disease->d0.y, 0.0));
		dxf->writeVertex(*dw, DL_VertexData(disease->d1.x, disease->d1.y, 0.0));
		dxf->writeVertex(*dw, DL_VertexData(disease->d2.x, disease->d2.y, 0.0));
		dxf->writeVertex(*dw, DL_VertexData(disease->d3.x, disease->d3.y, 0.0));
		dxf->writePolylineEnd(*dw);
		//计算得到矩形框中心点
		double centerX = (disease->d0.x + disease->d1.x + disease->d2.x + disease->d3.x) / 4;
		double centerY = (disease->d0.y + disease->d1.y + disease->d2.y + disease->d3.y) / 4;
		dxf->writeText(*dw, DL_TextData(centerX, centerY, 0, disease->d1.x, disease->d1.y, 0, textRatio, 1, 0, 0, 0, disName, "Standard", textAngle), DL_Attributes(layoutName, 256, -1, "BYLAYER", 1.0));

	}
	dw->sectionEnd();
	// 写入dxf
	dxf->writeObjects(*dw);
	dxf->writeObjectsEnd(*dw);

	dw->dxfEOF();
	dw->close();
	delete dw;
	dw = NULL;
	delete dxf;
	dxf = NULL;
	return true;
}
 


void hnOutDisease2dGpsDxf::initCAD(DL_Dxf* dxf, DL_WriterA* dw)
{
	if (dw == NULL)
		printf("Cannot open file 'file.dxf' for writing.");

	dxf->writeHeader(*dw);
	dw->sectionEnd();
	dw->sectionTables();
	dxf->writeVPort(*dw);
	dw->tableLinetypes(8);
	dxf->writeLinetype(*dw, DL_LinetypeData("BYBLOCK", "BYBLOCK", 0, 0, 0.0));
	dxf->writeLinetype(*dw, DL_LinetypeData("BYLAYER", "BYLAYER", 0, 0, 0.0));
	dxf->writeLinetype(*dw, DL_LinetypeData("CONTINUOUS", "Continuous", 0, 0, 0.0));
	dxf->writeLinetype(*dw, DL_LinetypeData("DASHED", "DASHED", 0, 0, 0.0));
	dxf->writeLinetype(*dw, DL_LinetypeData("DASHED2", "DASHED2", 0, 0, 0.0));
	double patternCenter[4] = { 31.75 ,-6.349999999999997 ,6.349999999999997 ,-6.349999999999997 };
	dxf->writeLinetype(*dw, DL_LinetypeData("CENTER", "Center ____ _ ____ _ ____ _ ____ _ ____ _ ____", 0/*70*/, 4/*73*/, 50.8/*40*/, patternCenter));
	double patternDot[4] = { 24.0 ,-3.0 ,0.0 ,-3.0 };
	dxf->writeLinetype(*dw, DL_LinetypeData("ACAD_ISO04W100", "ISO long-dash dot ____ . ____ . ____ . ____ . _", 0/*70*/, 4/*73*/, 30.0/*40*/, patternDot));

	double patternCenter2[4] = { 0.4 ,-0.1 ,0.1 ,-0.1 };
	dxf->writeLinetype(*dw, DL_LinetypeData("CENTERBLOCK", "Center ____ _ ____ _ ____ _ ____ _ ____ _ ____", 0/*70*/, 4/*73*/, 0.7/*40*/, patternCenter2));
	dw->tableEnd();

	int numberOfLayers = 1;
	dw->tableLayers(numberOfLayers);

	//0必须存在
	dxf->writeLayer(*dw,
		DL_LayerData("0", 0),
		DL_Attributes(
			std::string(""),
			DL_Codes::black,
			256,
			"CONTINUOUS", 1.0));

	dw->tableEnd();

	dw->tableStyle(1);
	DL_StyleData style("Standard", 0, 0.0, 1, 0.0, 0, 2.0, "宋体", "");
	style.bold = false;
	style.italic = false;
	dxf->writeStyle(*dw, style);
	dw->tableEnd();

	dxf->writeView(*dw);
	dxf->writeUcs(*dw);

	dw->tableAppid(1);
	dxf->writeAppid(*dw, "ACAD");
	dw->tableEnd();

	dxf->writeDimStyle(*dw, 1, 1, 1, 1, 1);

	dxf->writeBlockRecord(*dw);


	dw->tableEnd();

	dw->sectionEnd();

	dw->sectionBlocks();
	dxf->writeBlock(*dw, DL_BlockData("*Model_Space", 0, 0.0, 0.0, 0.0));
	dxf->writeEndBlock(*dw, "*Model_Space");
	dxf->writeBlock(*dw, DL_BlockData("*Paper_Space", 0, 0.0, 0.0, 0.0));
	dxf->writeEndBlock(*dw, "*Paper_Space");
	dxf->writeBlock(*dw, DL_BlockData("*Paper_Space0", 0, 0.0, 0.0, 0.0));
	dxf->writeEndBlock(*dw, "*Paper_Space0");
	dw->sectionEnd();
}


unsigned int hnOutDisease2dGpsDxf::simpleHash(const std::string&str)
{
	unsigned int hash = 0;
	for (char c : str)
	{
		hash = hash * 31 + c;
	}
	return hash;
}
