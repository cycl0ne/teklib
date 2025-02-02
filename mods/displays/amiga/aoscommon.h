/**************************************************************************
	dismod_waitmessage
 **************************************************************************/
TMODAPI TVOID dismod_waitmsg(TMOD_DISMOD *dismod)
{
    if(dismod->window_ready)
	WaitPort(dismod->theWindow->UserPort);
}

/**************************************************************************
	dismod_getmsg
 **************************************************************************/
TMODAPI TBOOL dismod_getmsg(TMOD_DISMOD *dismod,TDISMSG *dismsg)
{
    struct IntuiMessage *msg;

    while((msg = (struct IntuiMessage *)GetMsg(dismod->theWindow->UserPort)) != NULL)
    {
	ReplyMsg((struct Message *)msg);

	Dismod_ProcessEvents(dismod,msg);
	if(dismod->msgcode)
	{
	    dismsg->code=dismod->msgcode;
	    Dismod_ProcessMessage(dismod,dismsg);
	    dismod->msgcode=0;
	    return TTRUE;
	}
    }
    return TFALSE;
}

/********************************************************************************
	Dismod ProcessEvents
 ********************************************************************************/
TVOID Dismod_ProcessEvents(TMOD_DISMOD *dismod,struct IntuiMessage *msg)
{
    TUINT8 key;
    TINT keysym;

    switch(msg->Class)
    {
	case IDCMP_REFRESHWINDOW:
	    BeginRefresh(dismod->theWindow);
	    EndRefresh(dismod->theWindow,TRUE);
	    dismod->msgcode=TDISMSG_REDRAW;
	break;

	case IDCMP_CHANGEWINDOW:
	    if(!dismod->fullscreen)
	    {
		dismod->window_xpos=dismod->drect.x;
		dismod->window_ypos=dismod->drect.y;
		dismod->drect.x=dismod->window_xpos;
		dismod->drect.y=dismod->window_ypos;
		dismod->drect.width=dismod->theWindow->Width - dismod->borderwidth;
		dismod->drect.height=dismod->theWindow->Height - dismod->borderheight;

		if(dismod->drect.width!=dismod->width ||
		   dismod->drect.height!=dismod->height)
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

	case IDCMP_CLOSEWINDOW:
	    dismod->msgcode=TDISMSG_CLOSE;
	break;

	case IDCMP_ACTIVEWINDOW:
	    dismod->msgcode=TDISMSG_ACTIVATED;
	break;

	case IDCMP_INACTIVEWINDOW:
	    dismod->msgcode=TDISMSG_DEACTIVATED;
	break;

	case IDCMP_RAWKEY:
	    if(msg->Code & IECODE_UP_PREFIX)
	    {
		keysym=msg->Code & 0x7f;
		key=dismod->keytranstable[((TUINT8)(keysym)) & 0x000000ff];

		if(dismod->keymap[key])
		{
		    if(key==TDISKEY_LSHIFT || key==TDISKEY_RSHIFT)
			    dismod->keyqual &= (TDISKEYQUAL_ALT | TDISKEYQUAL_CTRL);
		    else if(key==TDISKEY_LALT || key==TDISKEY_RALT)
			    dismod->keyqual &= (TDISKEYQUAL_SHIFT | TDISKEYQUAL_CTRL);
		    else if(key==TDISKEY_LCTRL || key==TDISKEY_RCTRL)
			    dismod->keyqual &= (TDISKEYQUAL_SHIFT | TDISKEYQUAL_ALT);

		    dismod->key.code=key;
		    dismod->key.qualifier=0;
		    dismod->msgcode=TDISMSG_KEYUP;

		    dismod->keymap[key]=0;
		}
	    }
	    else
	    {
		keysym=msg->Code;
		key=dismod->keytranstable[((TUINT8)(keysym)) & 0x000000ff];

		if(!dismod->keymap[key])
		{
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

		    dismod->keymap[key]=1;
		}
	    }
	break;

	case IDCMP_MOUSEMOVE:
	{
	    TINT mx=dismod->theWindow->GZZMouseX;
	    TINT my=dismod->theWindow->GZZMouseY;
	    if(dismod->deltamouse)
	    {
		dismod->mmove.x=msg->MouseX;
		dismod->mmove.y=msg->MouseY;
		dismod->msgcode=TDISMSG_MOUSEMOVE;

		if(!dismod->fullscreen)
		{
		    if(mx>=0 && mx<=dismod->width && my>=0 && my<=dismod->height)
		    {
			if(dismod->outside)
			{
			    Dismod_SetPointerMode(dismod,TDISPTR_INVISIBLE);
			    dismod->outside=TFALSE;
			}
		    }
		    else if(!dismod->outside)
		    {
			Dismod_SetPointerMode(dismod,TDISPTR_NORMAL);
			dismod->outside=TTRUE;
		    }
		}
	    }
	    else
	    {
		if(!dismod->fullscreen)
		{
		    if(mx>=0 && mx<=dismod->width && my>=0 && my<=dismod->height)
		    {
			dismod->mmove.x=mx;
			dismod->mmove.y=my;
			dismod->msgcode=TDISMSG_MOUSEMOVE;

			if(dismod->outside)
			{
			    Dismod_SetPointerMode(dismod,dismod->ptrmode);
			    dismod->outside=TFALSE;
			}
		    }
		    else if(!dismod->outside)
		    {
			Dismod_SetPointerMode(dismod,TDISPTR_NORMAL);
			dismod->outside=TTRUE;
		    }
		}
		else
		{
		    dismod->mmove.x=mx;
		    dismod->mmove.y=my;
		    dismod->msgcode=TDISMSG_MOUSEMOVE;
		}
	    }
	}
	break;

	case IDCMP_MOUSEBUTTONS:
	    switch(msg->Code)
	    {
		case SELECTDOWN:
		    if(!dismod->ldown)
		    {
			dismod->mbutton.code=TDISMB_LBUTTON;
			dismod->msgcode=TDISMSG_MBUTTONDOWN;
			dismod->ldown=TTRUE;
		    }
		break;

		case MIDDLEDOWN:
		    if(!dismod->mdown)
		    {
			dismod->mbutton.code=TDISMB_MBUTTON;
			dismod->msgcode=TDISMSG_MBUTTONDOWN;
			dismod->mdown=TTRUE;
		    }
		break;

		case MENUDOWN:
		    if(!dismod->rdown)
		    {
			dismod->mbutton.code=TDISMB_RBUTTON;
			dismod->msgcode=TDISMSG_MBUTTONDOWN;
			dismod->rdown=TTRUE;
		    }
		break;

		case SELECTUP:
		    if(dismod->ldown)
		    {
			dismod->mbutton.code=TDISMB_LBUTTON;
			dismod->msgcode=TDISMSG_MBUTTONUP;
			dismod->ldown=TFALSE;
		    }
		break;

		case MIDDLEUP:
		    if(dismod->mdown)
		    {
			dismod->mbutton.code=TDISMB_MBUTTON;
			dismod->msgcode=TDISMSG_MBUTTONUP;
			dismod->mdown=TFALSE;
		    }
		break;

		case MENUUP:
		    if(dismod->rdown)
		    {
			dismod->mbutton.code=TDISMB_RBUTTON;
			dismod->msgcode=TDISMSG_MBUTTONUP;
			dismod->rdown=TFALSE;
		    }
		break;
	    }
	break;
    }
}

/********************************************************************************
	Display MakeKeytransTable
 ********************************************************************************/
TVOID Dismod_MakeKeytransTable(TMOD_DISMOD *dismod)
{
    dismod->keytranstable=TExecAlloc0(dismod->exec,TNULL,256);

    dismod->keytranstable[0x45] = TDISKEY_ESCAPE;
    dismod->keytranstable[0x50] = TDISKEY_F1;
    dismod->keytranstable[0x51] = TDISKEY_F2;
    dismod->keytranstable[0x52] = TDISKEY_F3;
    dismod->keytranstable[0x53] = TDISKEY_F4;
    dismod->keytranstable[0x54] = TDISKEY_F5;
    dismod->keytranstable[0x55] = TDISKEY_F6;
    dismod->keytranstable[0x56] = TDISKEY_F7;
    dismod->keytranstable[0x57] = TDISKEY_F8;
    dismod->keytranstable[0x58] = TDISKEY_F9;
    dismod->keytranstable[0x59] = TDISKEY_F10;

    dismod->keytranstable[0x00] = TDISKEY_GRAVE; // this isn't really a GRAVE, more a "BACKAPOSTROPH" -> HOW TO DEFINE???
    dismod->keytranstable[0x01] = TDISKEY_1;
    dismod->keytranstable[0x02] = TDISKEY_2;
    dismod->keytranstable[0x03] = TDISKEY_3;
    dismod->keytranstable[0x04] = TDISKEY_4;
    dismod->keytranstable[0x05] = TDISKEY_5;
    dismod->keytranstable[0x06] = TDISKEY_6;
    dismod->keytranstable[0x07] = TDISKEY_7;
    dismod->keytranstable[0x08] = TDISKEY_8;
    dismod->keytranstable[0x09] = TDISKEY_9;
    dismod->keytranstable[0x0A] = TDISKEY_0;
    dismod->keytranstable[0x0B] = TDISKEY_MINUS;
    dismod->keytranstable[0x0C] = TDISKEY_EQUALS;
    dismod->keytranstable[0x0D] = TDISKEY_BACKSLASH;
    dismod->keytranstable[0x41] = TDISKEY_BACKSPACE;

    dismod->keytranstable[0x42] = TDISKEY_TAB;
    dismod->keytranstable[0x10] = TDISKEY_q;
    dismod->keytranstable[0x11] = TDISKEY_w;
    dismod->keytranstable[0x12] = TDISKEY_e;
    dismod->keytranstable[0x13] = TDISKEY_r;
    dismod->keytranstable[0x14] = TDISKEY_t;
    dismod->keytranstable[0x15] = TDISKEY_y;
    dismod->keytranstable[0x16] = TDISKEY_u;
    dismod->keytranstable[0x17] = TDISKEY_i;
    dismod->keytranstable[0x18] = TDISKEY_o;
    dismod->keytranstable[0x19] = TDISKEY_p;
    dismod->keytranstable[0x1A] = TDISKEY_LEFTBRACKET;
    dismod->keytranstable[0x1B] = TDISKEY_RIGHTBRACKET;
    dismod->keytranstable[0x44] = TDISKEY_RETURN;

    dismod->keytranstable[0x62] = TDISKEY_CAPSLOCK;
    dismod->keytranstable[0x20] = TDISKEY_a;
    dismod->keytranstable[0x21] = TDISKEY_s;
    dismod->keytranstable[0x22] = TDISKEY_d;
    dismod->keytranstable[0x23] = TDISKEY_f;
    dismod->keytranstable[0x24] = TDISKEY_g;
    dismod->keytranstable[0x25] = TDISKEY_h;
    dismod->keytranstable[0x26] = TDISKEY_j;
    dismod->keytranstable[0x27] = TDISKEY_k;
    dismod->keytranstable[0x28] = TDISKEY_l;
    dismod->keytranstable[0x29] = TDISKEY_SEMICOLON;
    dismod->keytranstable[0x2A] = TDISKEY_APOSTROPH;
    dismod->keytranstable[0x2B] = TDISKEY_EXTRA2; // empty on american keyboards, this contains the SHARP/GRAVE on german and others... HOW TO DEFINE???

    dismod->keytranstable[0x60] = TDISKEY_LSHIFT;
    dismod->keytranstable[0x30] = TDISKEY_EXTRA1; // on german keyboards this is <>
    dismod->keytranstable[0x31] = TDISKEY_z;
    dismod->keytranstable[0x32] = TDISKEY_x;
    dismod->keytranstable[0x33] = TDISKEY_c;
    dismod->keytranstable[0x34] = TDISKEY_v;
    dismod->keytranstable[0x35] = TDISKEY_b;
    dismod->keytranstable[0x36] = TDISKEY_n;
    dismod->keytranstable[0x37] = TDISKEY_m;
    dismod->keytranstable[0x38] = TDISKEY_COMMA;
    dismod->keytranstable[0x39] = TDISKEY_PERIOD;
    dismod->keytranstable[0x3A] = TDISKEY_SLASH;
    dismod->keytranstable[0x61] = TDISKEY_RSHIFT;

    dismod->keytranstable[0x63] = TDISKEY_LCTRL; // on Amiga we have only one CTRL-Key
    dismod->keytranstable[0x64] = TDISKEY_LALT;
    dismod->keytranstable[0x40] = TDISKEY_SPACE;
    dismod->keytranstable[0x65] = TDISKEY_RALT;

    dismod->keytranstable[0x4C] = TDISKEY_UP;
    dismod->keytranstable[0x4F] = TDISKEY_LEFT;
    dismod->keytranstable[0x4E] = TDISKEY_RIGHT;
    dismod->keytranstable[0x4D] = TDISKEY_DOWN;

    dismod->keytranstable[0x5D] = TDISKEY_PRINT;
    dismod->keytranstable[0x5B] = TDISKEY_SCROLLOCK;

    dismod->keytranstable[0x0F] = TDISKEY_INSERT;
    dismod->keytranstable[0x3D] = TDISKEY_HOME;
    dismod->keytranstable[0x3F] = TDISKEY_PAGEUP;
    dismod->keytranstable[0x46] = TDISKEY_DELETE;
    dismod->keytranstable[0x1D] = TDISKEY_END;
    dismod->keytranstable[0x1F] = TDISKEY_PAGEDOWN;

    dismod->keytranstable[0x5A] = TDISKEY_NUMLOCK;
    dismod->keytranstable[0x5C] = TDISKEY_KP_DIVIDE;
    dismod->keytranstable[0x4A] = TDISKEY_KP_MINUS;
    dismod->keytranstable[0x5E] = TDISKEY_KP_PLUS;
    dismod->keytranstable[0x43] = TDISKEY_KP_ENTER;

    dismod->keytranstable[0x3E] = TDISKEY_KP8;
    dismod->keytranstable[0x2D] = TDISKEY_KP4;
    dismod->keytranstable[0x2E] = TDISKEY_KP5;
    dismod->keytranstable[0x2F] = TDISKEY_KP6;
    dismod->keytranstable[0x1E] = TDISKEY_KP2;

    dismod->keytranstable[0x66] = TDISKEY_OEM1; // left AMIGA
    dismod->keytranstable[0x67] = TDISKEY_OEM2; // right AMIGA
    dismod->keytranstable[0x5F] = TDISKEY_OEM3; // HELP
}


