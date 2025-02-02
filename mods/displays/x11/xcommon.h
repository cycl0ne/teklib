/**************************************************************************
	dismod_waitmessage
 **************************************************************************/
TMODAPI TVOID dismod_waitmsg(TMOD_DISMOD *dismod)
{
	if(dismod->window_ready)
	{
		XEvent event;
		XPeekEvent(dismod->theXDisplay, &event);
	}
}

/**************************************************************************
	dismod_getmsg
 **************************************************************************/
TMODAPI TBOOL dismod_getmsg(TMOD_DISMOD *dismod,TDISMSG *dismsg)
{
	XEvent event;

	if(dismod->newmsgloop)
	{
		dismod->mousemoved=TFALSE;
		dismod->newmsgloop=TFALSE;
	}

	while((XPending(dismod->theXDisplay)) > 0)
	{
		XNextEvent(dismod->theXDisplay, &event);
		Dismod_ProcessEvents(dismod,&event);

		if(dismod->msgcode)
		{
			dismsg->code=dismod->msgcode;
			Dismod_ProcessMessage(dismod,dismsg);
			dismod->msgcode=0;
			return TTRUE;
		}
	}
	dismod->newmsgloop=TTRUE;
	return TFALSE;
}

/********************************************************************************
	Display ProcessEvents
 ********************************************************************************/
TVOID Dismod_ProcessEvents(TMOD_DISMOD *dismod,XEvent *event)
{
	TUINT8 key;
	KeySym keysym;

	switch(event->type)
	{
		case Expose:
			dismod->msgcode=TDISMSG_REDRAW;
		break;

		case VisibilityNotify:
		{
			XVisibilityEvent *ev=(XVisibilityEvent*)event;
			if(ev->state==VisibilityFullyObscured)
				dismod->msgcode=TDISMSG_DEACTIVATED;
			else
				dismod->msgcode=TDISMSG_ACTIVATED;
		}
		break;

		case ConfigureNotify:
			if(!dismod->fullscreen)
			{
				dismod->drect.x=event->xconfigure.x;
				dismod->drect.y=event->xconfigure.y;
				dismod->drect.width=event->xconfigure.width;
				dismod->drect.height=event->xconfigure.height;
				dismod->window_xpos=dismod->drect.x;
				dismod->window_ypos=dismod->drect.y;

				if(dismod->drect.width!=dismod->width || dismod->drect.height!=dismod->height)
				{
					dismod->width=dismod->drect.width;
					dismod->height=dismod->drect.height;
					dismod->msgcode=TDISMSG_RESIZE;
				}
				else
				{
					dismod->msgcode=TDISMSG_MOVE;
				}
			}
		break;

		case ClientMessage:
			if(!dismod->fullscreen)
			{
				if(((XClientMessageEvent *) event)->data.l[0] == dismod->wmdeleteatom)
					dismod->msgcode=TDISMSG_CLOSE;
			}
		break;

		case KeyPress:
			keysym = XLookupKeysym( &event->xkey, 0 );
			
			if(keysym==0xff20)
				key=dismod->keytranstable[0x02];
			else if(keysym==0xff67)
				key=dismod->keytranstable[0x03];
			else
				key=dismod->keytranstable[((TUINT8)(keysym)) & 0x000000ff];

			if(key==TDISKEY_LSHIFT || key==TDISKEY_RSHIFT)
			{
				dismod->keyqual|=TDISKEYQUAL_SHIFT;
				dismod->key.qualifier=0;
			}
			else if(key==TDISKEY_LALT || key==TDISKEY_RALT)
			{
				dismod->keyqual|=TDISKEYQUAL_ALT;
				dismod->key.qualifier=0;
			}
			else if(key==TDISKEY_LCTRL || key==TDISKEY_RCTRL)
			{
				dismod->keyqual|=TDISKEYQUAL_CTRL;
				dismod->key.qualifier=0;
			}
			else
				dismod->key.qualifier=dismod->keyqual;

			dismod->key.code=key;
			dismod->msgcode=TDISMSG_KEYDOWN;
		break;

		case KeyRelease:
			keysym = XLookupKeysym( &event->xkey, 0 );
			if(keysym==0xff20)
				key=dismod->keytranstable[0x02];
			else if(keysym==0xff67)
				key=dismod->keytranstable[0x03];
			else
				key=dismod->keytranstable[((TUINT8)(keysym)) & 0x000000ff];

			if(key==TDISKEY_LSHIFT || key==TDISKEY_RSHIFT)
				dismod->keyqual &= (TDISKEYQUAL_ALT | TDISKEYQUAL_CTRL);
			else if(key==TDISKEY_LALT || key==TDISKEY_RALT)
				dismod->keyqual &= (TDISKEYQUAL_SHIFT | TDISKEYQUAL_CTRL);
			else if(key==TDISKEY_LCTRL || key==TDISKEY_RCTRL)
				dismod->keyqual &= (TDISKEYQUAL_SHIFT | TDISKEYQUAL_ALT);

			dismod->key.code=key;
			dismod->key.qualifier=0;
			dismod->msgcode=TDISMSG_KEYUP;
		break;

		case MotionNotify:
			if(dismod->deltamouse)
			{
				if(!dismod->mousemoved)
				{
					Window r, c;
					int rx,ry,wx,wy,mask;
					int mx,my;

					XQueryPointer(dismod->theXDisplay,dismod->theXWindow,
								&r, &c,
								&rx,&ry,
								&wx,&wy,
								&mask);


					mx=wx - dismod->width/2;
					my=wy - dismod->height/2;

					if(mx || my)
					{
						XWarpPointer(dismod->theXDisplay,
									0,
									dismod->theXWindow,
									0,0,0,0,
									dismod->width/2,dismod->height/2);

						dismod->mousemoved=TTRUE;

						dismod->mmove.x=mx;
						dismod->mmove.y=my;
						dismod->msgcode=TDISMSG_MOUSEMOVE;
					}
				}
			}
			else
			{
				dismod->mmove.x=event->xmotion.x;
				dismod->mmove.y=event->xmotion.y;
				dismod->msgcode=TDISMSG_MOUSEMOVE;
			}
		break;

		case ButtonPress:
			switch(event->xbutton.button)
			{
				case 1:
					dismod->mbutton.code=TDISMB_LBUTTON;
					dismod->msgcode=TDISMSG_MBUTTONDOWN;
				break;

				case 2:
					dismod->mbutton.code=TDISMB_MBUTTON;
					dismod->msgcode=TDISMSG_MBUTTONDOWN;
				break;

				case 3:
					dismod->mbutton.code=TDISMB_RBUTTON;
					dismod->msgcode=TDISMSG_MBUTTONDOWN;
				break;

				case 4:
					dismod->msgcode=TDISMSG_MWHEELUP;
				break;

				case 5:
					dismod->msgcode=TDISMSG_MWHEELDOWN;
				break;
			}
		break;

		case ButtonRelease:
			switch(event->xbutton.button)
			{
				case 1:
					dismod->mbutton.code=TDISMB_LBUTTON;
					dismod->msgcode=TDISMSG_MBUTTONUP;
				break;

				case 2:
					dismod->mbutton.code=TDISMB_MBUTTON;
					dismod->msgcode=TDISMSG_MBUTTONUP;
				break;

				case 3:
					dismod->mbutton.code=TDISMB_RBUTTON;
					dismod->msgcode=TDISMSG_MBUTTONUP;
				break;
			}
		break;
	}
}

/********************************************************************************
	Dismod MakeKeytransTable
 ********************************************************************************/
TVOID Dismod_MakeKeytransTable(TMOD_DISMOD *dismod)
{
	dismod->keytranstable=TExecAlloc0(dismod->exec,TNULL,256);

	dismod->keytranstable[0x1B] = TDISKEY_ESCAPE;
	dismod->keytranstable[0xBE] = TDISKEY_F1;
	dismod->keytranstable[0xBF] = TDISKEY_F2;
	dismod->keytranstable[0xC0] = TDISKEY_F3;
	dismod->keytranstable[0xC1] = TDISKEY_F4;
	dismod->keytranstable[0xC2] = TDISKEY_F5;
	dismod->keytranstable[0xC3] = TDISKEY_F6;
	dismod->keytranstable[0xC4] = TDISKEY_F7;
	dismod->keytranstable[0xC5] = TDISKEY_F8;
	dismod->keytranstable[0xC6] = TDISKEY_F9;
	dismod->keytranstable[0xC7] = TDISKEY_F10;
	dismod->keytranstable[0xC8] = TDISKEY_F11;
	dismod->keytranstable[0xC9] = TDISKEY_F12;

	dismod->keytranstable[0x5E] = TDISKEY_GRAVE;
	dismod->keytranstable[0x31] = TDISKEY_1;
	dismod->keytranstable[0x32] = TDISKEY_2;
	dismod->keytranstable[0x33] = TDISKEY_3;
	dismod->keytranstable[0x34] = TDISKEY_4;
	dismod->keytranstable[0x35] = TDISKEY_5;
	dismod->keytranstable[0x36] = TDISKEY_6;
	dismod->keytranstable[0x37] = TDISKEY_7;
	dismod->keytranstable[0x38] = TDISKEY_8;
	dismod->keytranstable[0x39] = TDISKEY_9;
	dismod->keytranstable[0x30] = TDISKEY_0;
	dismod->keytranstable[0xDF] = TDISKEY_MINUS;
	dismod->keytranstable[0x27] = TDISKEY_EQUALS;
	dismod->keytranstable[0x08] = TDISKEY_BACKSPACE;

	dismod->keytranstable[0x09] = TDISKEY_TAB;
	dismod->keytranstable[0x71] = TDISKEY_q;
	dismod->keytranstable[0x77] = TDISKEY_w;
	dismod->keytranstable[0x65] = TDISKEY_e;
	dismod->keytranstable[0x72] = TDISKEY_r;
	dismod->keytranstable[0x74] = TDISKEY_t;
	dismod->keytranstable[0x7A] = TDISKEY_y;
	dismod->keytranstable[0x75] = TDISKEY_u;
	dismod->keytranstable[0x69] = TDISKEY_i;
	dismod->keytranstable[0x6F] = TDISKEY_o;
	dismod->keytranstable[0x70] = TDISKEY_p;
	dismod->keytranstable[0xFC] = TDISKEY_LEFTBRACKET;
	dismod->keytranstable[0x2B] = TDISKEY_RIGHTBRACKET;
	dismod->keytranstable[0x0D] = TDISKEY_RETURN;

	dismod->keytranstable[0xE5] = TDISKEY_CAPSLOCK;
	dismod->keytranstable[0x61] = TDISKEY_a;
	dismod->keytranstable[0x73] = TDISKEY_s;
	dismod->keytranstable[0x64] = TDISKEY_d;
	dismod->keytranstable[0x66] = TDISKEY_f;
	dismod->keytranstable[0x67] = TDISKEY_g;
	dismod->keytranstable[0x68] = TDISKEY_h;
	dismod->keytranstable[0x6A] = TDISKEY_j;
	dismod->keytranstable[0x6B] = TDISKEY_k;
	dismod->keytranstable[0x6C] = TDISKEY_l;
	dismod->keytranstable[0xF6] = TDISKEY_SEMICOLON;
	dismod->keytranstable[0xE4] = TDISKEY_APOSTROPH;
	dismod->keytranstable[0x23] = TDISKEY_BACKSLASH;

	dismod->keytranstable[0xE1] = TDISKEY_LSHIFT;
	dismod->keytranstable[0x3C] = TDISKEY_EXTRA1;
	dismod->keytranstable[0x79] = TDISKEY_z;
	dismod->keytranstable[0x78] = TDISKEY_x;
	dismod->keytranstable[0x63] = TDISKEY_c;
	dismod->keytranstable[0x76] = TDISKEY_v;
	dismod->keytranstable[0x62] = TDISKEY_b;
	dismod->keytranstable[0x6E] = TDISKEY_n;
	dismod->keytranstable[0x6D] = TDISKEY_m;
	dismod->keytranstable[0x2C] = TDISKEY_COMMA;
	dismod->keytranstable[0x2E] = TDISKEY_PERIOD;
	dismod->keytranstable[0x2D] = TDISKEY_SLASH;
	dismod->keytranstable[0xE2] = TDISKEY_RSHIFT;

	dismod->keytranstable[0xE3] = TDISKEY_LCTRL;
	dismod->keytranstable[0xE9] = TDISKEY_LALT;
	dismod->keytranstable[0x20] = TDISKEY_SPACE;
	dismod->keytranstable[0x7E] = TDISKEY_RALT;
	dismod->keytranstable[0xE4] = TDISKEY_RCTRL;

	dismod->keytranstable[0x52] = TDISKEY_UP;
	dismod->keytranstable[0x51] = TDISKEY_LEFT;
	dismod->keytranstable[0x53] = TDISKEY_RIGHT;
	dismod->keytranstable[0x54] = TDISKEY_DOWN;

	dismod->keytranstable[0x61] = TDISKEY_PRINT;
	dismod->keytranstable[0x14] = TDISKEY_SCROLLOCK;
	dismod->keytranstable[0x13] = TDISKEY_PAUSE;

	dismod->keytranstable[0x63] = TDISKEY_INSERT;
	dismod->keytranstable[0x50] = TDISKEY_HOME;
	dismod->keytranstable[0x55] = TDISKEY_PAGEUP;
	dismod->keytranstable[0xFF] = TDISKEY_DELETE;
	dismod->keytranstable[0x57] = TDISKEY_END;
	dismod->keytranstable[0x56] = TDISKEY_PAGEDOWN;

	dismod->keytranstable[0x7F] = TDISKEY_NUMLOCK;
	dismod->keytranstable[0xAF] = TDISKEY_KP_DIVIDE;
	dismod->keytranstable[0xAA] = TDISKEY_KP_MULTIPLY;
	dismod->keytranstable[0xAD] = TDISKEY_KP_MINUS;
	dismod->keytranstable[0xAB] = TDISKEY_KP_PLUS;
	dismod->keytranstable[0x8D] = TDISKEY_KP_ENTER;

	dismod->keytranstable[0x95] = TDISKEY_KP7;
	dismod->keytranstable[0x97] = TDISKEY_KP8;
	dismod->keytranstable[0x9A] = TDISKEY_KP9;
	dismod->keytranstable[0x96] = TDISKEY_KP4;
	dismod->keytranstable[0x9D] = TDISKEY_KP5;
	dismod->keytranstable[0x98] = TDISKEY_KP6;
	dismod->keytranstable[0x9C] = TDISKEY_KP1;
	dismod->keytranstable[0x99] = TDISKEY_KP2;
	dismod->keytranstable[0x9B] = TDISKEY_KP3;
	dismod->keytranstable[0x9E] = TDISKEY_KP0;
	dismod->keytranstable[0x9F] = TDISKEY_KP_PERIOD;

	dismod->keytranstable[0x00] = TDISKEY_OEM1;
	dismod->keytranstable[0x02] = TDISKEY_OEM2;
	dismod->keytranstable[0x03] = TDISKEY_OEM3;
}

/********************************************************************************
	Dismod SetMousePtrMode
 ********************************************************************************/
TVOID Dismod_SetMousePtrMode(TMOD_DISMOD *dismod, TUINT ptr)
{
	switch(ptr)
	{
		case TDISPTR_NORMAL:
			XUndefineCursor(dismod->theXDisplay,dismod->theXWindow);
		break;

		case TDISPTR_BUSY:
			if(!dismod->busyCursor)
				dismod->theCursor=XCreateFontCursor(dismod->theXDisplay,XC_watch);

			XDefineCursor(dismod->theXDisplay,dismod->theXWindow,dismod->busyCursor);
		break;

		case TDISPTR_INVISIBLE:
			if(!dismod->hiddenCursor)
			{
				XColor a,b;
				Pixmap p;

				a.pixel=0;
				a.red = 0;
				a.green = 0;
				a.blue = 0;
				a.flags=DoRed|DoGreen|DoBlue;
				b.pixel=0;
				b.red = 0;
				b.green = 0;
				b.blue = 0;
				b.flags=DoRed|DoGreen|DoBlue;
				p=XCreatePixmap(dismod->theXDisplay,dismod->theXWindow,1,1,1);
				dismod->hiddenCursor=XCreatePixmapCursor(dismod->theXDisplay,p,p,&a,&b,0,0);
				XFreePixmap(dismod->theXDisplay,p);
			}
			XDefineCursor(dismod->theXDisplay,dismod->theXWindow,dismod->hiddenCursor);
		break;
	}
}
