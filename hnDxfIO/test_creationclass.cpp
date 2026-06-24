/*
 * @file test_creationclass.cpp
 */

/*****************************************************************************
**  $Id: test_creationclass.cpp 8865 2008-02-04 18:54:02Z andrew $
**
**  This is part of the dxflib library
**  Copyright (C) 2001 Andrew Mustun
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU Library General Public License as
**  published by the Free Software Foundation.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Library General Public License for more details.
**
**  You should have received a copy of the GNU Library General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
******************************************************************************/
#include "test_creationclass.h"
#include "StdAfx.h"
#include <iostream>
#include <stdio.h>


/**
 * Default constructor.
 */
Test_CreationClass::Test_CreationClass() {}


/**
 * Sample implementation of the method which handles layers.
 */
void Test_CreationClass::addLayer(const DL_LayerData& data) {
    printf("LAYER: %s flags: %d\n", data.name.c_str(), data.flags);
    printAttributes();
}

/**
 * Sample implementation of the method which handles point entities.
 */
void Test_CreationClass::addPoint(const DL_PointData& data) 
{
	if (attributes.getLayer() == "mainlayer" || attributes.getLayer() == "Red")
	{
		m_vecPointCloudData.push_back(data);
	}
	if (attributes.getLayer() == "ellipse" || attributes.getLayer() == "Black")
	{
		m_vecElliPointData.push_back(data);
	}

  /*  printf("POINT    (%6.3f, %6.3f, %6.3f)\n",
           data.x, data.y, data.z);
    printAttributes();*/
}

/**
 * Sample implementation of the method which handles line entities.
 */
void Test_CreationClass::addLine(const DL_LineData& data) 
{
	if (attributes.getLayer() == "long_axis" || attributes.getLayer() == "Green")
	{
		m_vecLongLineData.push_back(data);
	}

	if (attributes.getLayer() == "short_axis" || attributes.getLayer() == "Blue")
	{
		m_vecShortLineData.push_back(data);
	}

	if (attributes.getLayer() == "hor_axis" || attributes.getLayer() == "Black")
	{
		if (attributes.getLayer() == "hor_axis")
		{
			if (attributes.getColor() == 256 && attributes.getColor24() == -1)
			{
				m_vecHoriLineData.push_back(data);
			}
		}
		else
		{
			m_vecHoriLineData.push_back(data);
		}
		
		
	}


	
	/* printf("LINE     (%6.3f, %6.3f, %6.3f) (%6.3f, %6.3f, %6.3f)\n",
			data.x1, data.y1, data.z1, data.x2, data.y2, data.z2);
    printAttributes();*/
}

/**
 * Sample implementation of the method which handles arc entities.
 */
void Test_CreationClass::addArc(const DL_ArcData& data) {
    printf("ARC      (%6.3f, %6.3f, %6.3f) %6.3f, %6.3f, %6.3f\n",
           data.cx, data.cy, data.cz,
           data.radius, data.angle1, data.angle2);
    printAttributes();
}

/**
 * Sample implementation of the method which handles circle entities.
 */
void Test_CreationClass::addCircle(const DL_CircleData& data) {
  
	if (attributes.getLayer() == "mainlayer" || attributes.getLayer() == "Red")
	{
		//m_DL_CircleData=data;
	}
	
	printf("CIRCLE   (%6.3f, %6.3f, %6.3f) %6.3f\n",
           data.cx, data.cy, data.cz,
           data.radius);
    printAttributes();
}


/**
 * Sample implementation of the method which handles polyline entities.  折线
 */
void Test_CreationClass::addPolyline(const DL_PolylineData& data) {
    printf("POLYLINE \n");
    printf("flags: %d\n", (int)data.flags);
    printAttributes();
}


/**
 * Sample implementation of the method which handles vertices.
 */
void Test_CreationClass::addVertex(const DL_VertexData& data) {
    printf("VERTEX   (%6.3f, %6.3f, %6.3f) %6.3f\n",
           data.x, data.y, data.z,
           data.bulge);
    printAttributes();
}


void Test_CreationClass::add3dFace(const DL_3dFaceData& data) {
    printf("3DFACE\n");
    for (int i=0; i<4; i++) {
        printf("   corner %d: %6.3f %6.3f %6.3f\n", 
            i, data.x[i], data.y[i], data.z[i]);
    }
    printAttributes();
}


void Test_CreationClass::printAttributes() {
    printf("  Attributes: Layer: %s, ", attributes.getLayer().c_str());
    printf(" Color: ");
	
    if (attributes.getColor()==256)	{
        printf("BYLAYER");
    } else if (attributes.getColor()==0) {
        printf("BYBLOCK");
    } else {
        printf("%d", attributes.getColor());
    }
    printf(" Width: ");
    if (attributes.getWidth()==-1) {
        printf("BYLAYER");
    } else if (attributes.getWidth()==-2) {
        printf("BYBLOCK");
    } else if (attributes.getWidth()==-3) {
        printf("DEFAULT");
    } else {
        printf("%d", attributes.getWidth());
    }
    printf(" Type: %s\n", attributes.getLinetype().c_str());
}
    
//添加快
//void Test_CreationClass::add3dFace(const DL_3dFaceData& data) {
//	printf("3DFACE\n");
//	for (int i = 0; i < 4; i++) {
//		printf("   corner %d: %6.3f %6.3f %6.3f\n",
//			i, data.x[i], data.y[i], data.z[i]);
//	}
//	printAttributes();
//}

//获得点信息
void Test_CreationClass::GetVecPointsInfo(vector<DL_PointData>&vecPointCloudData, vector<DL_PointData>&vecElliPointData)
{
	vecPointCloudData = m_vecPointCloudData;
	vecElliPointData = m_vecElliPointData;
	
}

//获得线信息
void Test_CreationClass::GetVecLinesInfo(vector<DL_LineData>&vecLongLineData, vector<DL_LineData>&vecShortLineData,
	vector<DL_LineData>&vecHoriLineData)
{
	vecLongLineData = m_vecLongLineData;
	vecShortLineData = m_vecShortLineData;
	vecHoriLineData = m_vecHoriLineData;
	
}

////获得中心圆
//void Test_CreationClass::GetCenterCircle(DL_CircleData &pCircleData)
//{
//	pCircleData = m_DL_CircleData;
//
//}

// EOF
