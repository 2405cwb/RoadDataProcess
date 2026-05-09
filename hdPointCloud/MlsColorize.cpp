#include "StdAfx.h"
#include "MlsColorize.h"

namespace hd
{
	MlsColorize::MlsColorize(void)
	{
	}


	MlsColorize::~MlsColorize(void)
	{
	}


	void MlsColorize::SegPointCloudByPanoPos(void (*processCallback)(float,const char*))
	{
		vector<CommonTime> PanoTimes;
		vector<CommonTime> LoopTimes;

		// 读取全景点文件 hdi
		FILE* pfile = fopen(m_panoPathName,"rt");
		if (pfile == NULL)
		{
			AfxMessageBox("全景点文件不存在！");
			return;
		}

		char str[512] = {0};
		fgets(str,512,pfile);
		//
		float camerano = 0;
		int Year = 0,Month = 0, Day = 0,Hour = 0,Minute = 0,Second = 0,MiniSecond = 0;
		double cx = 0.0f,cy = 0.0f,cz = 0.0f;
		double yaw = 0.0f,pitch = 0.0f,roll = 0.0f;
		double longtitude = 0;
		double Lattitude = 0;
		int count = 0;
		POS pos;
		CommonTime ct;
		while (!feof(pfile))
		{
			fscanf(pfile,"%s\t%f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
				str,&camerano,&Year,&Month,&Day,&Hour,&Minute,&Second,&MiniSecond,&cx,&cy,&cz,&longtitude,&Lattitude,&yaw,&pitch,&roll);
			CString imgName(str);
			m_ImgFileName[count] = imgName;

			pos.x = cx;
			pos.y = cy;
			pos.z = cz;
			pos.yaw = yaw;
			pos.pitch = pitch;
			pos.roll = roll;
			m_PanoPos.push_back(pos);

			ct.hour = Hour;
			ct.minute = Minute;
			ct.second = Second;
			ct.milliSecond = MiniSecond;

			PanoTimes.push_back(ct);
			count++;

		}
		fclose(pfile);

		count = 0;
		// 读取圈位置文件
		pfile = fopen(m_CirclepathName,"rt");
		if (pfile == NULL)
		{
			AfxMessageBox("圈位置文件不存在！");
			return;
		}
		fgets(str,512,pfile);
		//
		float Time = 0;
		while (!feof(pfile))
		{
			fscanf(pfile,"%f\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t\n",
				&Time,&Year,&Month,&Day,&Hour,&Minute,&Second,&MiniSecond,&cx,&cy,&cz,&yaw,&pitch,&roll);

			pos.x = cx;
			pos.y = cy;
			pos.z = cz;
			pos.yaw = yaw;
			pos.pitch = pitch;
			pos.roll = roll;

			m_LoopPos.push_back(pos);

			ct.hour = Hour + 8;
			ct.minute = Minute;
			ct.second = Second;
			ct.milliSecond = MiniSecond;
			LoopTimes.push_back(ct);
			count++;
		}
		maxCircle = count;
		fclose(pfile);
		////////////////////////////////////////////////
		// 点云分段 -按时间最近邻查找
		m_RangeCircleIDs = new int[m_PanoPos.size() + 1];
		memset(m_RangeCircleIDs,0,sizeof(int)*(m_PanoPos.size() + 1));

		double ph = 0,pM = 0,pS = 0, pMs = 0;
		double lh = 0,lm = 0,ls = 0, lMs = 0;
		for (unsigned int i = 0; i< m_PanoPos.size();i++)
		{
			ph = PanoTimes[i].hour;
			pM = PanoTimes[i].minute;
			pS = PanoTimes[i].second;
			pMs = PanoTimes[i].milliSecond;
			double panotime = ph*3600 + pM*60 + pS + (pMs)/1000.0;

			lh = LoopTimes[0].hour;
			lm =  LoopTimes[0].minute;
			ls =  LoopTimes[0].second;
			lMs =  LoopTimes[0].milliSecond;
			double looptime = lh*3600 + lm*60 + ls + (lMs)/1000.0;

			double minRange = fabs(panotime - looptime);

			for (unsigned int j = 1; j< m_LoopPos.size();j++)
			{
				lh = LoopTimes[j].hour;
				lm =  LoopTimes[j].minute;
				ls =  LoopTimes[j].second;
				lMs =  LoopTimes[j].milliSecond;
				looptime =  looptime = lh*3600 + lm*60 + ls + (lMs)/1000.0;

				double range = fabs(panotime - looptime);

				if (range < minRange)
				{
					minRange = range;
					m_RangeCircleIDs[i] = j;
				}
			}
			if (processCallback && (i % 10) == 0)
			{
				processCallback(i/ (float)m_PanoPos.size(),"正在查找...");
			}
		}

		vector<int> dbArr;
		for (unsigned int i = 0; i< m_PanoPos.size() + 1; i++)
		{
			dbArr.push_back(m_RangeCircleIDs[i]);
		}
		for (unsigned int i = 0; i< m_PanoPos.size() + 1;i++)
		{
			if (i == 0)
			{
				m_RangeCircleIDs[i] = 0;
			}
			else if (i == m_PanoPos.size())
			{
				m_RangeCircleIDs[i] = maxCircle;
			}
			else
			{
				if (dbArr[i] < dbArr[i - 1])
				{
					m_RangeCircleIDs[i] = 0;
				}
				else
				{
					m_RangeCircleIDs[i] = hd::hd_round((float)(dbArr[i] + dbArr[i - 1])/2);
				}
			}
			if (processCallback && (i % 10) == 0)
			{
				processCallback(i/ (float)m_PanoPos.size(),"正在分段...");
			}
		}

		//for (unsigned int i = 0; i< m_PanoPos.size() + 1; i++)
		//{
		//	dbArr[i] = m_RangeCircleIDs[i];
		//}
		m_bIsSeg = true;
		//AfxMessageBox("分块完成！");
	}

	void MlsColorize::ColorizePointCloud(void (*processCallback)(float,const char*))
	{
		CHdSx3DView* pView = dynamic_cast<CHdSx3DView*>(CHdApplication::getAppInstance()->GetActiveView());

		double u = 0.0,v = 0.0;
		PointCloud* pcd = CHdApplication::getAppInstance()->GetPointCloud(pView->GetScanIndex(),false);

		CString imgPath;
		if (pcd && pcd->isNormalPointCloud())
		{
			double M[16];
			pcd->m_header.computeMatrix(M);
			for (unsigned int i = 0 ;i< m_PanoPos.size();i++)
			{
				imgPath = m_ImgPathName +"\\"+ m_ImgFileName[i] + ".jpg";
				const char *img_path = imgPath.GetBuffer(imgPath.GetLength());
				//获取起始列与终止列
				int startCol = m_RangeCircleIDs[i];
				int endCol = m_RangeCircleIDs[i + 1];

				if (endCol >= pcd->m_simpleHeader.number_of_col)
				{
					endCol = pcd->m_simpleHeader.number_of_col - 1;
				}
				IplImage* pImg = cvLoadImage(img_path,CV_LOAD_IMAGE_COLOR);

				if (!pImg)
				{
					continue;
				}
				m_PanoRslX = pImg->width;
				m_PanoRslY = pImg->height;

				double dx = m_PanoRslX/2;			// 全景影像横向180度对应的像素大小
				double dy = m_PanoRslY;				// 全景影像纵向180度对应的像素大小			
				CPanoCalibrate panoCalib;
				panoCalib.m_dx = dx;
				panoCalib.m_dy = dy;
				panoCalib.m_bCalibrated = true;
				panoCalib.setExtOrt(m_PanoPos[i].x,m_PanoPos[i].y,m_PanoPos[i].z,m_PanoPos[i].pitch,m_PanoPos[i].roll,-m_PanoPos[i].yaw);

				for (unsigned int iCol = startCol;iCol < endCol;iCol++)
				{
					double iScale = (double)iCol / pcd->m_header.number_of_col;
					int simpleCol = (int)(iScale * pcd->m_simpleHeader.number_of_col);

					for (unsigned int iRow = 0;iRow < pcd->m_simpleHeader.number_of_row;iRow++)
					{
						int index = simpleCol * pcd->m_simpleHeader.number_of_row + iRow;
						PointXYZIPRGBA& pt = (*pcd)[index];
						if (!pt.isValid())
						{
							continue;
						}

						double x = pt.x;
						double y = pt.y;
						double z = pt.z;
						hdHomogeneousTransformPoint(M,x,y,z);
						panoCalib.GetPixelByXYZ(x,y,z,u,v);
						if (u >= (int)m_PanoRslX - 1 || v >= (int)m_PanoRslY  - 1|| u < 0 || v < 0)
						{
							continue;
						}
						CvScalar cs ={ 0,0,0,0 };
						int imgCol = cvRound(u);
						int imgRow = cvRound(v);
						// 行在前，列在后
						cs = cvGet2D(pImg,imgRow,imgCol);

						pt.r = cs.val[2];
						pt.g = cs.val[1];
						pt.b = cs.val[0];

					}
				}
				cvReleaseImage(&pImg);

				if (processCallback && (i % 10) == 0)
				{
					processCallback(i/ (float)m_PanoPos.size(),"正在赋色...");
				}
			}
			//pcd->m_header.set_pointformat(HLS_POINTFORMAT_XYZIRGB);
			pcd->m_simpleHeader.set_pointformat(HLS_POINTFORMAT_XYZIRGB);
		}

		if (processCallback )
		{
			processCallback(0.0,"完成");
		}

		AfxMessageBox("赋色完成！");
	}
}

