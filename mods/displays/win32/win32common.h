/**************************************************************************
	dismod_waitmsg
 **************************************************************************/
TMODAPI TVOID dismod_waitmsg(TMOD_DISMOD *dismod)
{
	WaitMessage();
}

/**************************************************************************
	dismod_getmsg
 **************************************************************************/
TMODAPI TBOOL dismod_getmsg(TMOD_DISMOD *dismod,TDISMSG *dismsg)
{
	MSG msg;

	while(PeekMessage(&msg, dismod->hwnd, 0, 0, PM_REMOVE))
	{
		if(dismod->msgcode)
		{
			dismsg->code=dismod->msgcode;
			Dismod_ProcessMessage(dismod,dismsg);
			dismod->msgcode=0;
			DispatchMessage(&msg);
			return TTRUE;
		}
		else
			DispatchMessage(&msg);
	}

	if(dismod->newmsgloop)
	{
		IDirectInputDevice_GetDeviceState(dismod->dikbDevice,256,(LPVOID)&dismod->keymap);

		if(dismod->deltamouse && dismod->dimouseDevice)
			IDirectInputDevice_GetDeviceState(dismod->dimouseDevice,sizeof(DIMOUSESTATE),(LPVOID)&dismod->mousestate);

		dismod->newmsgloop=TFALSE;
		dismod->keycount=-1;
	}

	while(dismod->keycount<255)
	{
		dismod->keycount++;
		if(!dismod->keystate[dismod->keycount])
		{
			if((dismod->keymap[dismod->keycount] & 0x80))
			{
				dismod->keystate[dismod->keycount]=1;

				if(dismod->keycount==TDISKEY_LSHIFT || dismod->keycount==TDISKEY_RSHIFT)
				{
					dismod->keyqual|=TDISKEYQUAL_SHIFT;
					dismod->key.qualifier=0;
				}
				else if(dismod->keycount==TDISKEY_LALT || dismod->keycount==TDISKEY_RALT)
				{
					dismod->keyqual|=TDISKEYQUAL_ALT;
					dismod->key.qualifier=0;
				}
				else if(dismod->keycount==TDISKEY_LCTRL || dismod->keycount==TDISKEY_RCTRL)
				{
					dismod->keyqual|=TDISKEYQUAL_CTRL;
					dismod->key.qualifier=0;
				}
				else
					dismod->key.qualifier=dismod->keyqual;

				dismod->key.code=dismod->keycount;
				dismod->msgcode=TDISMSG_KEYDOWN;
				return TTRUE;
			}
		}
		else
		{
			if(!(dismod->keymap[dismod->keycount] & 0x80))
			{
				dismod->keystate[dismod->keycount]=0;

				if(dismod->keycount==TDISKEY_LSHIFT || dismod->keycount==TDISKEY_RSHIFT)
					dismod->keyqual &= (TDISKEYQUAL_ALT | TDISKEYQUAL_CTRL);
				else if(dismod->keycount==TDISKEY_LALT || dismod->keycount==TDISKEY_RALT)
					dismod->keyqual &= (TDISKEYQUAL_SHIFT | TDISKEYQUAL_CTRL);
				else if(dismod->keycount==TDISKEY_LCTRL || dismod->keycount==TDISKEY_RCTRL)
					dismod->keyqual &= (TDISKEYQUAL_SHIFT | TDISKEYQUAL_ALT);
				
				dismod->key.qualifier=0;
				dismod->key.code=dismod->keycount;
				dismod->msgcode=TDISMSG_KEYUP;
				return TTRUE;
			}
		}
	}

	if(dismod->deltamouse)
	{
		if(dismod->mousestate.lX || dismod->mousestate.lY)
		{
			dismod->mmove.x=dismod->mousestate.lX;
			dismod->mmove.y=dismod->mousestate.lY;
			dismsg->code=TDISMSG_MOUSEMOVE;
			Dismod_ProcessMessage(dismod,dismsg);
			dismod->mousestate.lX=0;
			dismod->mousestate.lY=0;
			return TTRUE;
		}
	}

	if(dismod->msgcode)
	{
		dismsg->code=dismod->msgcode;
		Dismod_ProcessMessage(dismod,dismsg);
		dismod->msgcode=0;
		return TTRUE;
	}

	dismod->newmsgloop=TTRUE;
	return TFALSE;
}

/**************************************************************************
  Dismod WndProc
 **************************************************************************/
LRESULT CALLBACK Dismod_WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	TMOD_DISMOD *dismod = (TMOD_DISMOD *) GetWindowLong(hwnd, GWL_USERDATA);

	if(dismod)
	{
		switch (msg)
		{
			case WM_CLOSE: case WM_DESTROY: case WM_QUIT:
				dismod->msgcode=TDISMSG_CLOSE;
				return 0;
			break;

			case WM_SIZE:
				if(!dismod->fullscreen && dismod->window_ready && !IsIconic(hwnd))
				{
					GetWindowRect(hwnd,&dismod->window_rect);

					dismod->drect.x=dismod->window_rect.left;
					dismod->drect.y=dismod->window_rect.top;
					dismod->drect.width=LOWORD(lParam);
					dismod->drect.height=HIWORD(lParam);

					dismod->window_xpos=dismod->drect.x;
					dismod->window_ypos=dismod->drect.y;
					dismod->width=dismod->drect.width;
					dismod->height=dismod->drect.height;

					SetRect( &dismod->m_rcScreenRect, dismod->window_xpos, dismod->window_ypos, dismod->window_xpos + dismod->width, dismod->window_ypos + dismod->height );
					GetWindowRect(dismod->hwnd,&dismod->window_rect);

					dismod->msgcode=TDISMSG_RESIZE;
				}
				return 0;
			break;

			case WM_MOVE: case WM_DISPLAYCHANGE:
				if(!dismod->fullscreen && dismod->window_ready && !IsIconic(hwnd))
				{
					dismod->window_xpos=(SHORT)LOWORD(lParam);
					dismod->window_ypos=(SHORT)HIWORD(lParam);
					SetRect( &dismod->m_rcScreenRect, dismod->window_xpos, dismod->window_ypos,
							 dismod->window_xpos + dismod->width, dismod->window_ypos + dismod->height );

					GetWindowRect(hwnd,&dismod->window_rect);

					dismod->drect.x=dismod->window_rect.left;
					dismod->drect.y=dismod->window_rect.top;
					dismod->drect.width=dismod->width;
					dismod->drect.height=dismod->height;

					dismod->msgcode=TDISMSG_MOVE;
				}
				return 0;
			break;

			case WM_ACTIVATE:
			{
				TUINT16 fActive = LOWORD(wParam);           /* activation flag */
				TBOOL fMinimized = (TBOOL) HIWORD(wParam);  /* minimized flag  */

				if(fMinimized)
				{
					dismod->window_ready=TFALSE;
					dismod->msgcode=TDISMSG_ICONIC;
				}
				else
				{
					dismod->window_ready=TTRUE;
					if(fActive==WA_INACTIVE)
						dismod->msgcode=TDISMSG_DEACTIVATED;
					else
					{
						if(dismod->dikbDevice)
							IDirectInputDevice_Acquire(dismod->dikbDevice);

						if(dismod->dimouseDevice)
							IDirectInputDevice_Acquire(dismod->dimouseDevice);

						dismod->msgcode=TDISMSG_ACTIVATED;
					}
				}
				return 0;
			}
			break;

			case WM_CREATE:
				dismod->msgcode=TDISMSG_REDRAW;
				return 0;
			break;

			case WM_PAINT: case WM_NCPAINT:
				if(dismod->msgcode!=TDISMSG_RESIZE)
					dismod->msgcode=TDISMSG_REDRAW;
			break;

			case WM_SYSCOMMAND:
				if(wParam==SC_KEYMENU || wParam==SC_MOUSEMENU || wParam==SC_TASKLIST)
					return 0;
			break;

			case WM_MOUSEMOVE:
				SetCursor(dismod->mousepointer);
				if(!dismod->deltamouse)
				{
					dismod->mmove.x=LOWORD(lParam);
					dismod->mmove.y=HIWORD(lParam);
					dismod->msgcode=TDISMSG_MOUSEMOVE;
				}
			break;

			case WM_LBUTTONDOWN:
				dismod->mbutton.code=TDISMB_LBUTTON;
				dismod->msgcode=TDISMSG_MBUTTONDOWN;
			break;

			case WM_MBUTTONDOWN:
				dismod->mbutton.code=TDISMB_MBUTTON;
				dismod->msgcode=TDISMSG_MBUTTONDOWN;
			break;

			case WM_RBUTTONDOWN:
				dismod->mbutton.code=TDISMB_RBUTTON;
				dismod->msgcode=TDISMSG_MBUTTONDOWN;
			break;

			case WM_LBUTTONUP:
				dismod->mbutton.code=TDISMB_LBUTTON;
				dismod->msgcode=TDISMSG_MBUTTONUP;
			break;

			case WM_MBUTTONUP:
				dismod->mbutton.code=TDISMB_MBUTTON;
				dismod->msgcode=TDISMSG_MBUTTONUP;
			break;

			case WM_RBUTTONUP:
				dismod->mbutton.code=TDISMB_RBUTTON;
				dismod->msgcode=TDISMSG_MBUTTONUP;
			break;

			case 0x020a:
			{
				TINT zdelta = (TINT16) HIWORD(wParam);
				if(zdelta>0)
					dismod->msgcode=TDISMSG_MWHEELUP;
				else if(zdelta<0)
					dismod->msgcode=TDISMSG_MWHEELDOWN;
			}
			break;

			case 0x020b:
			{
				if((TINT16)HIWORD(wParam)==1)
				{
					dismod->mbutton.code=TDISMB_E1BUTTON;
					dismod->msgcode=TDISMSG_MBUTTONDOWN;
				}
				else if((TINT16)HIWORD(wParam)==2)
				{
					dismod->mbutton.code=TDISMB_E2BUTTON;
					dismod->msgcode=TDISMSG_MBUTTONDOWN;
				}
			}
			break;

			case 0x020c:
			{
				if((TINT16)HIWORD(wParam)==1)
				{
					dismod->mbutton.code=TDISMB_E1BUTTON;
					dismod->msgcode=TDISMSG_MBUTTONUP;
				}
				else if((TINT16)HIWORD(wParam)==2)
				{
					dismod->mbutton.code=TDISMB_E2BUTTON;
					dismod->msgcode=TDISMSG_MBUTTONUP;
				}
			}
			break;
		}
	}
	return DefWindowProc(hwnd,msg,wParam,lParam);
}

/**************************************************************************
	CreateDeltaMouse
 **************************************************************************/
TBOOL Dismod_CreateDeltaMouse(TMOD_DISMOD *dismod)
{
	DIPROPDWORD dipdw =
	{ { sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE },
		DIPROPAXISMODE_REL
	};

	if( FAILED(IDirectInput_CreateDevice( dismod->di, &GUID_SysMouse,&dismod->dimouseDevice, NULL)))
		return TFALSE;

	IDirectInputDevice_SetDataFormat(dismod->dimouseDevice,&c_dfDIMouse);
	IDirectInputDevice_SetCooperativeLevel(dismod->dimouseDevice,dismod->hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	IDirectInputDevice_SetProperty(dismod->dimouseDevice,DIPROP_AXISMODE, &dipdw.diph);
	IDirectInputDevice_Acquire(dismod->dimouseDevice);

	return TTRUE;
}

/**************************************************************************
	DestroyDeltaMouse
 **************************************************************************/
TVOID Dismod_DestroyDeltaMouse(TMOD_DISMOD *dismod)
{
	if(dismod->dimouseDevice)
	{
		IDirectInputDevice_Unacquire(dismod->dimouseDevice); 
		IDirectInputDevice_Release(dismod->dimouseDevice);
		dismod->dimouseDevice=TNULL;
	}
}

