#pragma once
#include "..\hdFramework\hdCommand.h"
#include "stdafx.h"
using namespace hd::fm;
namespace hd
{
	namespace fm
	{

		class HD3DSCENE_API CHd3DCmdViewRight :
			public CHdCommand
		{
		public:
			CHd3DCmdViewRight(void);
			virtual ~CHd3DCmdViewRight(void);

			virtual void OnClick();

			virtual void OnCreate( CHdApp* app );

			virtual bool GetEnable();

		};

	}
}