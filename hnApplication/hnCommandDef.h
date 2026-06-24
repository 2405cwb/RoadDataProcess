#ifndef _HN_COMMAND_DEF_H_
#define _HN_COMMAND_DEF_H_

//! Enumeration for all mouse input events
enum EMOUSE_INPUT_EVENT
{
	//! Left mouse button was pressed down.
	EMIE_LMOUSE_PRESSED_DOWN = 0,

	//! Right mouse button was pressed down.
	EMIE_RMOUSE_PRESSED_DOWN,

	//! Middle mouse button was pressed down.
	EMIE_MMOUSE_PRESSED_DOWN,

	//! Left mouse button was left up.
	EMIE_LMOUSE_LEFT_UP,

	//! Right mouse button was left up.
	EMIE_RMOUSE_LEFT_UP,

	//! Middle mouse button was left up.
	EMIE_MMOUSE_LEFT_UP,

	//! The mouse cursor changed its position.
	EMIE_MOUSE_MOVED,

	//! The mouse wheel was moved. Use Wheel value in event data to find out
	//! in what direction and how fast.
	EMIE_MOUSE_WHEEL,

	//! Left mouse button double click.
	//! This event is generated after the second EMIE_LMOUSE_PRESSED_DOWN event.
	EMIE_LMOUSE_DOUBLE_CLICK,

	//! Right mouse button double click.
	//! This event is generated after the second EMIE_RMOUSE_PRESSED_DOWN event.
	EMIE_RMOUSE_DOUBLE_CLICK,

	//! Middle mouse button double click.
	//! This event is generated after the second EMIE_MMOUSE_PRESSED_DOWN event.
	EMIE_MMOUSE_DOUBLE_CLICK,

	//! Left mouse button triple click.
	//! This event is generated after the third EMIE_LMOUSE_PRESSED_DOWN event.
	EMIE_LMOUSE_TRIPLE_CLICK,

	//! Right mouse button triple click.
	//! This event is generated after the third EMIE_RMOUSE_PRESSED_DOWN event.
	EMIE_RMOUSE_TRIPLE_CLICK,

	//! Middle mouse button triple click.
	//! This event is generated after the third EMIE_MMOUSE_PRESSED_DOWN event.
	EMIE_MMOUSE_TRIPLE_CLICK,

	//! No real event. Just for convenience to get number of events
	EMIE_COUNT
};

//! Masks for mouse button states
enum E_MOUSE_BUTTON_STATE_MASK
{
	EMBSM_LEFT = 0x01,
	EMBSM_RIGHT = 0x02,
	EMBSM_MIDDLE = 0x04,

	//! currently only on windows
	EMBSM_EXTRA1 = 0x08,

	//! currently only on windows
	EMBSM_EXTRA2 = 0x10,

	EMBSM_FORCE_32_BIT = 0x7fffffff
};

// 影像浏览
#define COMMAND_IMAGE_BROWSE         0
#define COMMAND_ADD_POINT_DISEASE    1
#define COMMAND_ADD_LINE_DISEASE   2
#define COMMAND_ADD_PLANE_DISEASE  3

//识别CP3
#define COMMAND_DISTINGUI_CP3 4

//编辑CP3
#define COMMAND_EDIT_CP3 5
//修正CP3
#define COMMAND_MODI_CP3 6

// 编辑病害
#define COMMAND_EDIT_DISEASE  7

//轨道板编号
#define COMMAND_WOOD_ID 8

//添加长短链
#define COMMAND_ADD_CHAIN 9

//添加CP3
#define COMMAND_ADD_CP3 10

//删除CP3
#define COMMAND_REMOVE_CP3 11

//修正轨道板
#define COMMAND_MODI_SLEEPER_TIE 12

//添加轨道板
#define COMMAND_ADD_SLEEPER_TIE 13

//删除轨道板
#define COMMAND_REMOVE_SLEEPER_TIE 14

// 设置侵限检测位置
#define COMMAND_SEL_LIMIT_POS 15

//创建声屏障深度图
#define COMMAND_CREATE_DEPTH 16

//识别桥梁隧道
#define COMMAND_DISTINGUI_BRIDGE_TUNNEL 17

//编辑桥梁隧道
#define COMMAND_EDIT_BRIDGE_TUNNEL 18

// 编辑里程桩
#define COMMAND_EDIT_MILEAGE_PILE 19

// 相机标定
#define COMMAND_CAM_REG  20

// 平移病害
#define COMMAND_OFFSET_DISEASE 21

// 打断线条
#define COMMAND_CUT_LINE 22

// 合并线条
#define COMMAND_COMMBINE_LINE 23

#define COMMAND_ADD_TUNNEL_LOC 24

#define COMMAND_OFFSET_TUNNEL_LOC 25

#endif