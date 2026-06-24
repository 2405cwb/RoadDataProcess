#pragma once

/************************************************************************/
/*定义自定义消息，向主界面发送                                          */
/************************************************************************/
#define WM_WS_REFRESH		       (WM_USER + 1)                                     // 刷新地面工作区treectrl
#define WM_WS_DELETE_CTRLPT	       (WM_USER + 2)
#define WM_ISCAN_WS_REFRESH	       (WM_USER + 3)                                     // 刷新iScan工作区treectrl
#define WM_SEND_ERROR              (WM_USER + 4)                                     //发送提示信息
#define WM_WS_REFRESH_DOM          (WM_USER + 5)                                     // 刷新地面工作区矢量图层
#define WM_WS_DEL_FITLINE          (WM_USER + 6)                                      //删除地面站工作区上的拟合线
#define WM_WS_DEL_FITPLANE         (WM_USER + 7)                                      //删除地面站工作区上的拟合面

#define WM_WM_INFORMATION	       (WM_USER + 10)
#define WM_WM_WARNING		       (WM_USER + 11)
#define WM_WM_ERROR			       (WM_USER + 12)

#define WM_USER_REFRESH_STATUSBAR  (WM_USER + 13)                                    // 刷新状态栏 

#define WM_USER_UPDATEPLANE        (WM_USER + 97)									     // 添加平面消息
#define WM_USER_ClOSE_DEM          (WM_USER + 98)								        // 关闭dem
#define WM_USER_CLOSEAPP	       (WM_USER + 99)										// 关闭主程序
#define WM_USER_CLOSEVIEW	       (WM_USER + 100)
#define WM_USER_AUTOSAVE	       (WM_USER + 101)										// 自动保存工作区
#define WM_USER_UPDATEPANO         (WM_USER + 102)										// 全景平移旋转消息
#define WM_USER_UPDATELABEL        (WM_USER + 103)										// 添加标注消息
#define WM_USER_UPDATESPHERE       (WM_USER + 104)									// 添加靶球消息
#define WM_USER_UPDATEEXPORT       (WM_USER + 105)									// 添加选中导出消息
#define WM_USER_UPDATACHESSBOARD   (WM_USER + 106)								// 添加棋盘格标靶消息
#define WM_USER_UPDATAFEATUREPOINT (WM_USER + 107)
#define WM_USER_UPDATAORIENTPOINT  (WM_USER + 108)								// 添加定向点消息
#define WM_USER_SELECTPOINT_INFO   (WM_USER + 109)								// 显示点选信息
#define WM_USER_3DBOX_INFO         (WM_USER + 110)								// 显示3DBOX信息
#define WM_USER_2LINEINSET_PTINFO  (WM_USER + 111)                             // 2线交点信息
#define WM_USER_ClOSE_TIN          (WM_USER + 112)								        // 关闭tin

//****************************************扫描过滤自定义消息*********************************************//
#define WM_USER_OUTLIERFILTER       (WM_USER + 113)									// 离群过滤对话框消息
#define  WM_USER_FILTERSTATISTIC    (WM_USER + 114)								// 统计过滤对话框消息
#define  WM_USER_FILTERDISTANCE     (WM_USER + 115)									// 距离过滤对话框消息
#define  WM_USER_FILTERRADIUS       (WM_USER + 116)									// 半径邻域过滤对话框消息
#define WM_USER_FILTERSMOOTH        (WM_USER + 117)									// 平滑过滤对话框消息
#define WM_USER_FILTERANGLE         (WM_USER + 118)									// 角度过滤对话框消息
#define WM_USER_UPDATAFILTER        (WM_USER + 119)									// 添加选中过滤消息
#define WM_USER_ClOSE_SEAPCD        (WM_USER + 120)								     // 关闭海量点云

#define WM_OPEN_SLTCTRLPT_ZOOMWND   (WM_USER + 121)								// 显示控制点选择放大镜窗口
#define WM_CLOSE_SLTCTRLPT_ZOOMWND  (WM_USER + 122)								// 关闭控制点选择放大镜窗口
#define WM_SELECT_POINTINFO_ZOOMWND (WM_USER + 123)								// 放大镜窗口中选点的信息
#define WM_UPDATE_SELECTPT_TODLG	(WM_USER + 124)
#define WM_USER_MULT_PT_PICK_INFO   (WM_USER + 125)                               // 多次选点模式下发送消息以更新dlg
#define WM_USER_HIDE_IMG_WND        (WM_USER + 126)                              // 发消息隐藏img面板视图
#define WM_USER_FILTERBYPOSHT       (WM_USER + 127)                              // 按POS高度过滤消息
#define WM_USER_FILTERBYPOSDST      (WM_USER + 128)                              // 按POS距离过滤消息
#define WM_USER_FILTERBYHT          (WM_USER + 129)                              // 按高程过滤消息

//****************************************点云拟合自定义消息*********************************************//
#define  WM_USER_FITTINGPLANE        (WM_USER + 130)									// 添加平面拟合消息
#define  WM_USER_FITTINGSPHERE       (WM_USER + 131)									// 添加球面拟合消息
#define  WM_USER_FITTINGCYLINDRICAL  (WM_USER + 132)								// 添加柱面拟合消息
#define WM_USER_UPDATAPLANE         (WM_USER + 133)                                 // 添加平面消息

//****************************************平差自定义消息*************************************************//
#define WM_USER_BLOCKADJUSTMENT        (WM_USER + 140)								// 区域平差对话框消息
																				   
#define WM_USER_POINTCLOUD_CHANGE      (WM_USER + 150)								// 点云加载或卸载
																				   
#define WM_USER_DONOTSHOWMENU          (WM_USER + 160)								// 选择右键是否显示菜单
#define WM_USER_MODELVIE_DONOTSHOWMENU (WM_USER + 161)						     	// 模型视图选择右键是否显示菜单

#define WM_USER_REG_PANO_HLZ_ADDCTRLPOINT  (WM_USER + 170)                          // 全景与HLZ点云配准时增加控制点

#define WM_USER_ADD_SCANROUTE          (WM_USER + 180)                              // 加载轨迹工程（加载之后发送，参数1为轨迹指针）
#define WM_USER_REMOVE_SCANROUTE       (WM_USER + 181)                              // 移除轨迹工程（移除之前发送，参数1为轨迹指针）
#define WM_USER_IMPORT_PANORAMA        (WM_USER + 182)                              // 导入全景数据（导入之后发送，参数1为轨迹指针）
#define WM_USER_DELETE_PANORAMA        (WM_USER + 183)                              // 删除全景数据（删除之后发送，参数1为轨迹指针）

#define WM_USER_ADDCTRLPOINT           (WM_USER + 201)									// 全景控制点选中
#define WM_USER_CLEARPOINTS            (WM_USER + 202)									// 全景控制点清除
#define WM_USER_PLANARSEL_MARKER       (WM_USER + 203)								    // 平面视图选取标靶

//****************************************iScan数据浏览自定义消息*****************************************//
#define WM_USER_REFRESHROUTEPANO     (WM_USER + 204)								// 全景轨迹点更新
#define WM_USER_ISCAN_PANO_MOVE      (WM_USER + 205)								// 全景浏览上下帧
#define WM_USER_ISCAN_SCAN_BY_DIS    (WM_USER + 206)								// 切入全景球内部后，按距离时间显示点云
#define WM_USER_ISCAN_PANO_ROTATE    (WM_USER + 207)								// 全景球旋转浏览
#define WM_USER_ISCAN_PANO_ADJUEST   (WM_USER + 208)                                // 全景调整快捷键发送消息
#define WM_USER_ISCAN_REMOVE_SECTOR  (WM_USER + 209)                                // 快速视图deactivate时发消息移除轨迹视图的扇形
#define WM_USER_ROUTEPOINT_INFO      (WM_USER + 210)								// 显示轨迹点信息
//****************************************绘制到CAD自定义消息*********************************************//
#define WM_USER_DRAW2CAD_PLINE	           (WM_USER + 220)							// 绘制Pline到CAD
#define WM_USER_DRAW2CAD_POINT	           (WM_USER + 221)							// 绘制点到CAD
#define WM_USER_DRAW2CAD_CADSTATE          (WM_USER + 222)							// CAD的状态：是否关闭
#define WM_USER_CADCAMERA_ANGLE			   (WM_USER + 223)			                //在鼠标释放时，发送相机的当前角度,供C#模块调用  --liangjia 2014/3/22

#define WM_USER_PLANARSEL_HIDE_MARKER      (WM_USER + 224)                          // 单点拼接完成后发送消息隐藏该对话框窗口
#define WM_USER_DOMODAL_PROGRESS_DLG       (WM_USER + 225)                          // 启动进度条对话框
#define WM_USER_END_RUNNING_PROGRESS_DLG   (WM_USER + 226)                          // 结束进度条对话框
#define WM_USER_START_RUNNING_PROGRESS_DLG (WM_USER + 227)                          // 启动进度条对话框
#define WM_USER_TERMINATE_THREAD           (WM_USER + 228)                          // 强制终止线程
#define WM_USER_CALCU_THREAD_DATA          (WM_USER + 229)                          // 发消息将计算线程类对象传入对话框，这样对话框取消操作时可以控制停止计算线程
#define WM_USER_ISCAN_REFRESH_ROUTE_DOM    (WM_USER + 230)                          // va工程由轨迹显示缩略dom
#define WM_USER_ISCAN_DOM_BIGSIZE          (WM_USER + 231)                          // va工程由缩略dom显示完整尺寸

//****************************************点云分类自定义消息**********************************************//
#define WM_USER_CLASSIFY_VERTICAL_SECTION	(WM_USER + 401)						// 俯视图任意拉选矩形框选择
#define WM_USER_CLASSIFY_REFRESH			(WM_USER + 402)						// 手动分类完成后，窗口刷新同步消息
#define WM_USER_CLASSIFY_UPORBELOW_LINE		(WM_USER + 403)						// 分类工具-选择线之上或者之下
#define WM_USER_CLASSIFY_BETWEEN_LINE		(WM_USER + 404)						// 分类工具-选择线之间
#define WM_USER_CLASSIFY_SETTING			(WM_USER + 405)						// 分类类别随时更新
#define WM_USER_CLASSIFY_DERECT_ASSIGN		(WM_USER + 406)						// 手动分类 直接指定类别
#define WM_USER_CLASSIFY_SLIDER_FEATURE		(WM_USER + 407)						// 手动分类 通过滑动条二分类确定
#define WM_USER_CLASSIFY_VERTICAL_RECTSEL	(WM_USER + 408)						// 俯视图拉规则矩形框选择
#define WM_USER_CLASSIFY_ROAD				(WM_USER + 409)						// 分类道路
#define WM_USER_CLASSIFY_ROADMARK_AUTO		(WM_USER + 410)						// 道路标志线自动提取
#define WM_USER_CLASSIFY_ROADMARK_SEMI		(WM_USER + 411)						// 道路标志线半自动提取
#define WM_USER_CLASSIFY_CLASSIFIEDCOUNT    (WM_USER + 412)                     // 分类点数
#define WM_USER_CLASSIFY_NOISE				(WM_USER + 413)						// 分类噪声
#define WM_USER_CLASSIFY_ROAD_INFO			(WM_USER + 414)						// 分类的道路点云信息
#define WM_USER_HIDE_RM_DIALOG				(WM_USER + 415)						// 隐藏道路和标志线分类对话框
#define WM_USER_GETSN_FROM_CONTAIN_DLG		(WM_USER + 416)						// 通过包含对话框获得点云SceneNode
#define WM_USER_GETPOINT_FROM_CONTAIN_DLG	(WM_USER + 417)						// 通过包含对话框获得点云						

//****************************************点云测站调整自定义消息**********************************************//
#define WM_USER_MULTISCAN_ADJUST			(WM_USER + 501)						// 调整测站的点云 平移or旋转
#define WM_USER_UPDATE_ROTATECENTER			(WM_USER + 502)						// 测站旋转调整 更新旋转中心
#define WM_USER_UPDATE_ROTATEANGLE			(WM_USER + 503)						// 测站旋转调整 抛出旋转角度
#define WM_USER_HIDE_ROTATEDIALOG			(WM_USER + 504)						// 测站旋转调整 隐藏旋转控制对话框
#define WM_USER_UPDATE_OFFSETVALUE			(WM_USER + 510)						// 测站平移调整 抛出平移值
#define WM_USER_HIDE_OFFSETDIALOG			(WM_USER + 511)						// 测站平移调整 隐藏平移控制对话框
#define WM_USER_UPDATE_ADJUSTPARA			(WM_USER + 512)						// 测站调整 调整对象切换时更新信息
#define WM_USER_SET_COMMAND_TOOL			(WM_USER + 513)							// 设置工具消息
#define WM_USER_SHETCH_ADJECT_OFFSET        (WM_USER + 514)                     // 调整测站点云 平移
#define WM_USER_SHETCH_ADJECT_ROTATE        (WM_USER + 515)                     // 调整测站点云 旋转
#define WM_USER_DIALOG_ADJUST_SKETCH        (WM_USER + 516)                     // 微调微调对话框平移和旋转值
#define WM_USER_SHETCH_DLG_CLOSE            (WM_USER + 517)                     // 控制草图视图调整微调对话框显隐
//********************************************************************************************************//
//****************************************iScan工程调整自定义消息**********************************************//
#define WM_USER_ISCAN_UPDATE_ROTATECENTER			(WM_USER + 522)						// iScan旋转调整 更新旋转中心
#define WM_USER_ISCAN_UPDATE_ROTATEANGLE			(WM_USER + 523)						// iScan旋转调整 抛出旋转角度
#define WM_USER_ISCAN_HIDE_ROTATEDIALOG				(WM_USER + 524)						// iScan旋转调整 隐藏旋转控制对话框
#define WM_USER_ISCAN_UPDATE_OFFSETVALUE			(WM_USER + 525)						// iScan平移调整 抛出平移值
#define WM_USER_ISCAN_HIDE_OFFSETDIALOG				(WM_USER + 526)						// iScan平移调整 隐藏平移控制对话框
//********************************************************************************************************//

//****************************************拼接工程自定义消息**********************************************//
#define WM_USER_ADD_SCANPAIR				    (WM_USER + 601)						// 添加拼接对
#define WM_USER_ADDCTRLPT3DVIEW					(WM_USER + 602)					// 在点云三维视图中添加控制点
#define WM_USER_UPDATE_REGPROJ_TREE_ITEM		(WM_USER + 603)						// 根据内存对象拼接工程树控件
#define	WM_USER_ADD_REGPROJ_TREE_ITEM			(WM_USER + 604)					// 在已有控件中增加拼接组或拼接对
#define WM_USER_ADD_EXIST_CTRLPTS				(WM_USER + 605)					// 添加拼接对的控制点
#define WM_USER_REGPAIR_DELETE_CTRLPT			(WM_USER + 606)					// 删除手选控制点
#define WM_USER_REGPAIR_DELETE_FITPT_SN			(WM_USER + 617)					// 删除拟合控制点
#define WM_USER_REGPAIR_DELETE_SN               (WM_USER + 622)                 // 删除拟合控制点(内存中删除, 同名面, 靶球, 标靶)
#define WM_USER_REGPAIR_PREVIEW					(WM_USER + 607)					// 拼接对预览
#define WM_USER_REGPAIR_LOCATE_CTRLPT			(WM_USER + 608)					// 定位控制点
#define WM_USER_REGPAIR_SETNEW_TRANSMODEL		(WM_USER + 609)					// 对多测站预览视图设置新的转换模型
#define WM_USER_REGPAIR_UPDATE_GLOALSCAN		(WM_USER + 610)					// 更新全局参考站何其图标
#define WM_USER_ACTIVE_VIEW                     (WM_USER + 611)                 // 发消息到主框架，调用地面工作区激活视图函数
#define WM_USER_AUTOFIND_SPHERE					(WM_USER + 612)
#define WM_USER_AUTOFIND_CHESSBOARD				(WM_USER + 613)
#define WM_USER_AUTOFIND_PLANE					(WM_USER + 614)
#define WM_USER_PREVIEW_GROUP_SCANS				(WM_USER + 615)
#define WM_USER_REFRESH_TREEVIEW				(WM_USER + 616)
#define WM_USER_SAVE_OPEN_REGPROJ				(WM_USER + 618)
#define WM_USER_ADD_CTRLNODE_TO_GREYREG         (WM_USER + 619)  // 将三维拼接视图添加的控制点SN同步更新至灰度图拼接视图 

#define WM_USER_HDVIEW_TREE_CHECKED             (WM_USER + 620)  // 树控件发消息通知面板更改状态
#define WM_USER_OPEN_TMPREGPROJ				    (WM_USER + 621)  // 打开临时拼接工程 
#define WM_USER_REFRESH_GLOBSCAN				(WM_USER + 622)  // 刷新全局参考站坐标 
#define WM_USER_IS_SAVE_COPY                    (WM_USER + 623)  // 保存临时工作区时是否拷贝
#define WM_USER_ADD_REG_LIST                    (WM_USER + 624)  // 增加拼接列表并重新拼接当前拼接对

/////////////////////////////////////////////////插件测试消息/////////////////////////////////////////////////
#define WM_USER_PLUGIN_SHOW_DOCKPANE		    (WM_USER + 709)					// 插件面板显示控制
#define WM_USER_PLUGIN_CUTFILL_VOLUME_VIEW      (WM_USER + 710)                 // 插件发消息至主框架，打开一个视图
#define WM_USER_PLNDRAW2CAD_PLINE	(WM_USER + 711)									// 插件中绘制Pline到CAD
/////////////////////////////////////////////////MDC相关//////////////////////////////////////////////////////
#define WM_USER_UPDATE_SELECT_MDCVIEW			(WM_USER + 801)					// 三维选择同步到屏幕视图
#define WM_USER_BISNEWCTRL_PTS					(WM_USER + 802)					// 新建控制点对
#define WM_USER_DEL_IMGCTRLS_PTS				(WM_USER + 803)					// 删除内存中以及视图中对应的控制点	
#define WM_USER_UPDATE_IMGPTS					(WM_USER + 804)					// 更新控制点对
#define WM_USER_UPDATE_PTS					    (WM_USER + 805)					// 更新控制点对
#define WM_USER_ADD_IMGCTRL_PTS					(WM_USER + 806)					// 添加控制点到三维视图
//********************************************************************************************************//
#define WM_USER_SAVE_IMAGE_TEST                 (WM_USER + 807)                 // 测试代码

#define WM_USER_OPEN_DLGVIEW                    (WM_USER + 808)

#define WM_USER_LOAD_TITLEPANO					(WM_USER + 809)					// 加载全景切片
#define WM_USER_RELOAD_TITLEPANO				(WM_USER + 810)					// 重新加载全景切片
#define WM_USER_DELETE_POLY_SN                   (WM_USER + 811)                 //删除场景中的线场景节点

/////////////////////////////////////////////////道路截面特征线提取 add by lixialiang --2016/01/18////////////////////////////////
#define WM_USER_ADD_SECTION_STATION              (WM_USER + 812)                  //在fileview视图中添加道路截面桩号
#define WM_USER_UPADTE_SECTION_PROGRESS          (WM_USER + 814)                  //更新截面提取对话框中的进度条
#define WM_USER_END_EXTRACT_SECTION              (WM_USER + 815)                  //结束截面数据的计算
#define WM_USER_DELETE_SECTION_LINE              (WM_USER + 816)                  //删除道路截面的特征线
#define WM_USER_UPDATA_SECTIOMN_LINE_COORD       (WM_USER + 817)                  //投影道路截面特征线上的节点坐标
#define WM_USER_DELETE_ALL_SECTION               (WM_USER + 818)                  //清空所有的截面数据

#define WM_USER_UPDATE_ADDPOINT_FITPLANE		 (WM_USER + 820)				  // 添加选点拟合平面消息
#define WM_USER_UPDATE_ADDRECT_FITPLANE			 (WM_USER + 821)				  // 添加拉框拟合平面消息
#define WM_USER_DELETE_POINT_SN                  (WM_USER + 822)                   // 移除场景中的点SCENENODE
#define WM_USER_ADD_FITPOINT_SN                  (WM_USER + 823)                   // 增加交点
#define WM_USER_UPDATE_ADDRECT_FITPLINE			 (WM_USER + 824)				  // 添加拉框拟合墙线消息
#define WM_USER_UPDATE_ADDRECT_FITPOINT			 (WM_USER + 825)				  // 添加拉框拟合墙角点消息
#define WM_USER_UPDATE_ADDSINGLEPOINT_FITPOINT	 (WM_USER + 826)				  // 添加单点拟合面消息

////******************************************************************************************************************
#define WM_USER_UPDATE_PROPERTY_DAILOG	 (WM_USER + 827)                   //属性对话框动态刷新消息 byliuzhaoliang
////******************************************************************************************************************
#define WM_USER_SERIALIZE_DRAFT_ADJACENT      (WM_USER + 828)                   //保存读取草图视图下邻接关系

////******************************************************************************************************************
#define WM_USER_OPEN_QUICK_REG_WND                  (WM_USER + 829)                 // 显示拼接视图
#define WM_USER_CLOSE_QUICK_REG_WND                 (WM_USER + 830)                 // 隐藏拼接视图

//********************************************************************************************************//
#define WM_USER_EXTRACT_PARKINGSPACE_COMPLETED      (WM_USER + 831)                 // 道路停车位提取完毕
#define WM_USER_EXTRACT_ROADTREE_COMPLETED          (WM_USER + 832)                 // 行道树提取完毕
#define WM_USER_EXTRACT_ROADLAMP_COMPLETED          (WM_USER + 833)                 // 路灯提取完毕

//********************************************************************************************************//
#define WM_USER_SHOW_EXPORT_DIALOG					(WM_USER + 834)                 // 显示导出对话框
#define WM_USER_SHOW_POLYEXPORT_DIALOG				(WM_USER + 835)                 // 显示多边形选择导出对话框