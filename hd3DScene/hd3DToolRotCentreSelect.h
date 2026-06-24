#pragma once
#include "..\..\hdFramework\hdtool.h"
#include "hd3DCamera.h"

namespace hd
{
	namespace fm
	{

		class HD3DSCENE_API CHd3DToolRotCentreSelect :
			public CHdTool
		{
		private:
		public:
			CHd3DToolRotCentreSelect(void);
			virtual ~CHd3DToolRotCentreSelect(void);

			virtual void OnClick();

			virtual void OnCreate( CHdApp* app );

			virtual bool getEnable();

			virtual void OnMouseDown( int Button, int Shift, int X, int Y );

			virtual void OnMouseUp( int Button, int Shift, int X, int Y );

			virtual void Deactivate();

			virtual bool OnContextMenu( int X, int Y );

			virtual void OnDblClick( int Button, int Shift, int X, int Y );

			virtual void OnKeyDown( int keyCode, int Shift );

			virtual void OnKeyUp( int keyCode, int Shift );

			virtual void OnMouseMove( int Button, int Shift, int X, int Y );

			virtual void OnMouseWheel( UINT nFlags, short zDelta, int X, int Y );

		};

	}
}
