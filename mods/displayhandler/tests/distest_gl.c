/*
	Example for displayhandler
	--------------------------

	this example shows how to open a simple display and some rendering inside
	this display and a attached bitmap
*/

#include <stdio.h>
#include <stdlib.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/util.h>
#include <tek/proto/io.h>
#include <tek/proto/time.h>
#include <tek/proto/displayhandler.h>
#include <tek/proto/datatypehandler.h>
#include <tek/proto/imgproc.h>

#ifdef TSYS_DARWIN
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/gl.h>
	#include <OpenGL/glext.h>
	#include <OpenGL/glu.h>
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif

static THNDL *clrpen=TNULL, *textpen=TNULL, *redpen=TNULL, *grnpen=TNULL, *bluepen=TNULL;
static TAPTR execbase=TNULL,utilbase=TNULL,dhbase=TNULL,imgpbase=TNULL,timebase=TNULL,treq=TNULL;

/* function prototypes */
TVOID shutup(TSTRPTR errormessage);

THNDL* CreateDisplayBitmap(THNDL *theDisplay, TINT w, TINT h, TBOOL ckey, TBOOL blend);
TIMGPICTURE* CreateIMGPBitmap(TINT w, TINT h);
TVOID RenderToDisplay(THNDL *theDisplay, TINT w, TINT h);
TVOID ShowIMGPBitmap(THNDL *theDisplay, TIMGPICTURE *pic, TINT x, TINT y);
TVOID ShowDisplayBitmap(THNDL *theDisplay, THNDL *theBitmap, TINT x, TINT y, TBOOL ckey, TBOOL blend);

TVOID RenderDirectBitmap(THNDL *theBitmap);

TVOID RenderDirectOpenGl(THNDL *theDisplay);

/**************************************************************************
	TEKMain
 **************************************************************************/
TTASKENTRY TVOID TEKMain(TAPTR task)
{
    THNDL *theDisplay=TNULL;
    THNDL *theBitmap=TNULL;
	TIMGPICTURE *pic=TNULL;
    TDISMSG msg;
    TDISKEY *key;
    TINT bm_w=0,bm_h=0,bm_x,bm_y,xdir,ydir,xstep,ystep;
	TINT pic_w=0,pic_h=0;
	TTIME atime,btime;
	TINT frametime=0;
	TBOOL sizechange=TTRUE;
	TINT8 textbuf[128];
    TBOOL abort=TFALSE;
    TBOOL redraw=TFALSE;
	TBOOL active=TTRUE;

	const GLubyte *glvendor = TNULL, *glrenderer = TNULL, *glversion = TNULL;

    // initialize needed modules
	execbase = TGetExecBase(task);
	utilbase = TExecOpenModule(execbase, "util", 0, TNULL);
	imgpbase = TExecOpenModule(execbase, "imgproc", 0, TNULL);
	dhbase = TExecOpenModule(execbase, "displayhandler", 0, TNULL);
	timebase = TExecOpenModule(execbase, "time", 0, TNULL);
	treq = TTimeAllocTimeRequest(timebase, TNULL);

	if(utilbase && imgpbase && dhbase && timebase)
	{
		TINT width,height,depth;
		TBOOL fullscreen,dblbuf,resize,vsync,ckey,blend;
		TTAG args[11];
		TSTRPTR *argv;
		TAPTR arghandle;

		TSTRPTR tmpl = "?=help/S,-w=WIDTH/N,-h=HEIGHT/N,-d=DEPTH/N,-f=FULLSCREEN/S,-dbl=DOUBLEBUFFER/S,-r=RESIZE/S,-v=VSYNC/S,-c=CKEY/S,-b=BLEND/S";

		// set default options
		width=640;
		height=480;
		depth=32;
		fullscreen=TFALSE;
		dblbuf=TFALSE;
		resize=TFALSE;
		vsync=TFALSE;
		ckey=TFALSE;
		blend=TFALSE;

		// set default options for commandline args
		args[0]=(TTAG)TFALSE;
		args[1]=(TTAG)&width;
		args[2]=(TTAG)&height;
		args[3]=(TTAG)&depth;
		args[4]=(TTAG)fullscreen,
		args[5]=(TTAG)dblbuf,
		args[6]=(TTAG)resize,
		args[7]=(TTAG)vsync,
		args[8]=(TTAG)ckey,
		args[9]=(TTAG)blend,

		argv = TUtilGetArgV(utilbase);
		arghandle = TUtilParseArgV(utilbase, tmpl, argv+1, args);

		if(arghandle && !args[0])
		{
			width           = *(TINT*)args[1];
			height          = *(TINT*)args[2];
			depth           = *(TINT*)args[3];
			fullscreen      = (TBOOL)args[4];
			dblbuf          = (TBOOL)args[5];
			resize          = (TBOOL)args[6];
			vsync           = (TBOOL)args[7];
			ckey            = (TBOOL)args[8];
			blend           = (TBOOL)args[9];
		}
		else
		{
			printf("%s\n",TUtilStrChr(utilbase,tmpl,',')+1);
			TDestroy(arghandle);
			shutup(TNULL);
			return;
		}

		if(arghandle)
			TDestroy(arghandle);

		if(depth!=16 && depth!=24 && depth!=32)
		{
			shutup("color depth must be 8, 16, 24 or 32 bit!");
			return;
		}

		if(depth<16)
		{
			shutup("for opengl color depth must be at least 16 bit!");
			return;
		}

		if(width<320)
		{
			shutup("minimum allowed width 320!");
			return;
		}

		if(height<200)
		{
			shutup("minimum allowed height 200!");
			return;
		}

		printf("\nDisplaytest settings:\n");
		printf("---------------------\n");
		printf("\tDimension:    %d x %d x %d\n",width,height,depth);
		printf("\tFullscreen:   %s\n",fullscreen? "yes":"no");
		printf("\tDoublebuffer: %s\n",dblbuf? "yes":"no");
		printf("\tResize:       %s\n",resize? "yes":"no");
		printf("\tVSync:        %s\n",vsync? "yes":"no");

		// use simplecreateview for the display

		theDisplay=TDisSimpleCreateView(dhbase,"teklib display",width,height,depth,TTRUE,fullscreen,dblbuf,resize);
		if(!theDisplay)
		{
		    shutup("Couldn't open display!!!");
			return;
		}
		else
		{
			TTAGITEM displaytags[2];

			displaytags[0].tti_Tag=TDISTAG_VSYNCHINT;               displaytags[0].tti_Value=(TTAG)vsync;
			displaytags[1].tti_Tag=TTAG_DONE;

			TDisSetAttrs(dhbase,theDisplay,displaytags);

			clrpen=TDisAllocPen(dhbase,theDisplay,0x00000000);
			redpen=TDisAllocPen(dhbase,theDisplay,0x00e02020);
			grnpen=TDisAllocPen(dhbase,theDisplay,0x0020e020);
			bluepen=TDisAllocPen(dhbase,theDisplay,0x002020e0);
			textpen=TDisAllocPen(dhbase,theDisplay,0x00ffffff);

			// get strings describing the opengl subsystem
			if(TDisBegin(dhbase,theDisplay))
			{
				glvendor = glGetString(GL_VENDOR);
				glrenderer = glGetString(GL_RENDERER);
				glversion = glGetString(GL_VERSION);
				TDisEnd(dhbase);
			}

			// set variable for bitmap movement
			bm_x = 0;
			bm_y = 0;
			xstep = 1;
			xdir = 1;
			ystep = 1;
			ydir = -1;

			// mainloop
			while(!abort)
			{
				if(active)
				{
					// move the bitmap
					bm_x += xstep*xdir;
					if ( bm_x < 0 )
					{
						bm_x = 0;
						xdir = 1;
					}
					else if( bm_x > width - bm_w )
					{
						bm_x = width - bm_w - 1;
						xdir = -1;
					}

					bm_y += ystep*ydir;
					if ( bm_y < 0 )
					{
						bm_y = 0;
						ydir = 1;
					}
					else if( bm_y > height - bm_h )
					{
						bm_y = height - bm_h - 1;
						ydir = -1;
					}

					if(sizechange)
					{
						// prepare a bitmap for blitting
						bm_w=width/3;
						bm_h=height/3;
						if(theBitmap) TDestroy(theBitmap);
						theBitmap=CreateDisplayBitmap(theDisplay,bm_w,bm_h,ckey,blend);
						// and render something direct to the bitmap
						if(theBitmap)
							RenderDirectBitmap(theBitmap);

						// prepare a offscreen bitmap
						pic_w=width/4;
						pic_h=height/4;
						if(pic) TImgFreeBitmap(imgpbase,pic);
						pic=CreateIMGPBitmap(pic_w,pic_h);

						sizechange=TFALSE;
					}
				}

				if(active || redraw)
				{
					TTimeQueryTime(timebase, treq, &atime);
					RenderToDisplay(theDisplay,width,height);

					if(pic)
						ShowIMGPBitmap(theDisplay,pic,0,height-pic_h);

					if(theBitmap)
						ShowDisplayBitmap(theDisplay,theBitmap,bm_x,bm_y,ckey,blend);

					RenderDirectOpenGl(theDisplay);

					// print some white text
					if(TDisBegin(dhbase,theDisplay))
					{
						TDisSetDPen(dhbase,textpen);
						TDisTextout(dhbase,"press ESC to quit",0,0);

						sprintf(textbuf,"redraw: %d  ",frametime);
						TDisTextout(dhbase,textbuf,1,0);

						sprintf(textbuf,"GL_Vendor:");
						TDisTextout(dhbase,textbuf,3,0);
						sprintf(textbuf,"%s",glvendor);
						TDisTextout(dhbase,textbuf,3,15);
						
						sprintf(textbuf,"GL_Renderer:");
						TDisTextout(dhbase,textbuf,4,0);
						sprintf(textbuf,"%s",glrenderer);
						TDisTextout(dhbase,textbuf,4,15);
						
						sprintf(textbuf,"GL_Version:");
						TDisTextout(dhbase,textbuf,5,0);
						sprintf(textbuf,"%s",glversion);
						TDisTextout(dhbase,textbuf,5,15);

						TDisEnd(dhbase);
					}

					// and make all things visible
					TDisFlush(dhbase,theDisplay);

					TTimeQueryTime(timebase, treq, &btime);
					TTimeSubTime(timebase, &btime, &atime);
					frametime = (TINT)(((TFLOAT) btime.ttm_USec / 1000000 + btime.ttm_Sec) * 1000);

					redraw=TFALSE;
				}

				if(!active)
					TDisWaitMsg(dhbase,theDisplay,TNULL);

				// process all pending messages
				while((TDisGetMsg(dhbase,theDisplay,&msg)))
				{
					switch(msg.code)
					{
						case TDISMSG_REDRAW:
							if(!active) redraw=TTRUE;
						break;

						case TDISMSG_RESIZE:
						{
							TDISRECT *drect=(TDISRECT*)msg.data;
							width=drect->width;
							height=drect->height;
							sizechange=TTRUE;
						}
						break;

						case TDISMSG_CLOSE:
							abort=TTRUE;
						break;

						case TDISMSG_KEYDOWN:
							key=(TDISKEY*)msg.data;
							if(key->code==TDISKEY_ESCAPE)
								abort=TTRUE;
						break;

						case TDISMSG_KEYUP:
						break;

						case TDISMSG_ACTIVATED:
							active=TTRUE;
						break;

						case TDISMSG_DEACTIVATED: case TDISMSG_ICONIC:
							active=TFALSE;
						break;
					}
				}
			}
			// clean up
			if(pic) TImgFreeBitmap(imgpbase,pic);
			if(theBitmap) TDestroy(theBitmap);
			if(redpen) TDestroy(redpen);
			if(grnpen) TDestroy(grnpen);
			if(bluepen) TDestroy(bluepen);
			if(clrpen) TDestroy(clrpen);
			if(textpen) TDestroy(textpen);
			TDestroy(theDisplay);
		}
	}
    shutup(TNULL);
}

/**************************************************************************
	shutup
 **************************************************************************/
TVOID shutup(TSTRPTR errormessage)
{
    if(treq) TTimeFreeTimeRequest(timebase,treq);
	if(timebase) TExecCloseModule(execbase,timebase);
	if(imgpbase) TExecCloseModule(execbase,imgpbase);
	if(dhbase) TExecCloseModule(execbase,dhbase);
	if(utilbase) TExecCloseModule(execbase,utilbase);

	if(errormessage) printf("\n\tError: %s\n\n",errormessage);
}

/**************************************************************************
	CreateDisplayBitmap
	-------------------

	this allocates a bitmap from the display and renders some stuff inside.
	the bitmap is device dependent, and with most displaymodules the best
	solution for fast blits.
 **************************************************************************/
THNDL* CreateDisplayBitmap(THNDL *theDisplay, TINT w, TINT h, TBOOL ckey, TBOOL blend)
{
	if(w && h)
	{
		THNDL *theBitmap;
		TINT flags=0;
		
		if(ckey)
			flags|=TDISCF_CKEYBLIT;

		if(blend)
			flags|=TDISCF_CALPHABLIT;

		theBitmap=TDisAllocBitmap(dhbase,theDisplay,w,h,flags);
		if(theBitmap)
		{
			// start rendering inside the bitmap
			if(TDisBegin(dhbase,theBitmap))
			{
				TINT sw1=w/20;
				TINT sh1=h/20;
				TINT sw2=w/10;
				TINT sh2=h/10;
				TINT sw3=w/8;
				TINT sh3=h/8;

				if(sw1 && sh1 && sw2 && sh2 && sw3 && sh3)
				{
					// clear to white
					TDisSetDPen(dhbase,textpen);
					TDisFill(dhbase);

					// draw a blue circle
					TDisSetDPen(dhbase,bluepen);
					TDisEllipsef(dhbase,w/2,h/2,w/2-sw1,h/2-sh1);

					// a red
					TDisSetDPen(dhbase,redpen);
					TDisEllipsef(dhbase,w/2,h/2,w/2-sw2,h/2-sh2);

					// and a smaller white
					TDisSetDPen(dhbase,textpen);
					TDisEllipsef(dhbase,w/2,h/2,w/3-sw3,h/3-sh3);

					// a red box at left bottom
					TDisSetDPen(dhbase,redpen);
					TDisBoxf(dhbase,1,h-sh3-1,sw3,sh3);
				}

				TDisTextout(dhbase,"a cool bitmap",0,0);

				// finish rendering
				TDisEnd(dhbase);
				return theBitmap;
			}
			TDestroy(theBitmap);
		}
	}
	return TNULL;
}

/**************************************************************************
	CreateIMGPBitmap
	-------------------

	this allocates a bitmap with the imgproc module and renders some stuff
	inside. the bitmap is device independent, and so a good way to store
	image data in custom formats.
 **************************************************************************/
TIMGPICTURE* CreateIMGPBitmap(TINT w,TINT h)
{
	if(w && h)
	{
		TIMGPICTURE *pic=TImgAllocBitmap(imgpbase,w,h,IMGFMT_A8R8G8B8);
		if(pic)
		{
			TINT sw1=w/16;
			TINT sh1=h/16;
			TINT sw2=w/8;
			TINT sh2=h/8;

			if(sw1 && sh1 && sw2 && sh2)
			{
				TIMGARGBCOLOR col;

				col.a=0;		col.r=255;		col.g=0;		col.b=0;
				TImgBoxf(imgpbase,pic,0,0,w,h,&col);

				col.a=0;		col.r=0;		col.g=255;		col.b=0;
				TImgBoxf(imgpbase,pic,sw1,sh1,w-sw1*2,h-sh1*2,&col);

				col.a=0;		col.r=0;		col.g=0;		col.b=255;
				TImgBoxf(imgpbase,pic,sw2,sh2,w-sw2*2,h-sh2*2,&col);

				col.a=0;		col.r=32;		col.g=96;		col.b=48;
				TImgBoxf(imgpbase,pic,0,0,sw2,sh2,&col);

				col.a=0;		col.r=96;		col.g=32;		col.b=48;
				TImgBoxf(imgpbase,pic,w-sw2,h-sh2,sw2,sh2,&col);
			}
			return pic;
		}
	}
	return TNULL;
}

/**************************************************************************
	RenderToDisplay
	---------------

	here is some rendering to the display with standard render functions.
	if possible, these functions are utilizing the render facilities of the
	underlaying operating system.
 **************************************************************************/
TVOID RenderToDisplay(THNDL *theDisplay, TINT w, TINT h)
{
	if(w && h)
	{
		if(TDisBegin(dhbase,theDisplay))
		{
			TINT poldat[8];

			TINT bx=w/16;
			TINT by=h/16;
			TINT bw=w/3;
			TINT bh=h/3;

			TINT ex=w/2;
			TINT ey=h-h/4;
			TINT erx=w/4;
			TINT ery=h/5;

			// clear display to black
			TDisSetDPen(dhbase,clrpen);
			TDisFill(dhbase);

			// draw a solid red box
			if(bw && bh)
			{
				TDisSetDPen(dhbase,redpen);
				TDisBoxf(dhbase,bx,by,bw,bh);
			}

			// draw a solid green ellipse
			if(erx && ery)
			{
				TDisSetDPen(dhbase,grnpen);
				TDisEllipsef(dhbase,ex,ey,erx,ery);
			}

			// draw a solid blue triangle
			TDisSetDPen(dhbase,bluepen);
			poldat[0]=w-bx-bw/2;    poldat[1]=by;
			poldat[2]=w-bx-bw;              poldat[3]=by+bh;
			poldat[4]=w-bx;                 poldat[5]=by+bh;
			TDisPolyf(dhbase,3,poldat);

			// finish rendering
			TDisEnd(dhbase);
		}
	}
}

/**************************************************************************
	ShowIMGPBitmap
	--------------

	the above created imgp bitmap will be blitted to the display. this
	requires in most cases a conversion from the color format of the
	bitmap to the color format of the display. this conversion is handled
	by the displaymodule, if there is support available from the underlaying
	operating system, or done by the displayhandler, but in both cases
	transparent to you.
	if your application settles on fast conversions, you should look into
	the caps (dis_getcaps) for the flags "canconvert...". if these are TFALSE,
	your best bet is to allocate device dependent bitmaps and preconvert
	your image data.
 **************************************************************************/
TVOID ShowIMGPBitmap(THNDL *theDisplay, TIMGPICTURE *pic, TINT x, TINT y)
{
    TTAGITEM blttags[3];

	if(TDisBegin(dhbase,theDisplay))
	{
		blttags[0].tti_Tag=TDISB_DSTX;  blttags[0].tti_Value=(TTAG)x;
		blttags[1].tti_Tag=TDISB_DSTY;  blttags[1].tti_Value=(TTAG)y;
		blttags[2].tti_Tag=TTAG_DONE;
		TDisPutImage(dhbase,pic,blttags);

		TDisEnd(dhbase);
	}
}

/**************************************************************************
	ShowDisplayBitmap
	------------------

	the above created display bitmap will be blitted to the display.
	note that a blit occurs WITHOUT lock/begin
 **************************************************************************/
TVOID ShowDisplayBitmap(THNDL *theDisplay, THNDL *theBitmap, TINT x, TINT y, TBOOL ckey, TBOOL blend)
{
    TTAGITEM blttags[5];
	TINT i=0;

	blttags[i].tti_Tag=TDISB_DSTX;  blttags[i++].tti_Value=(TTAG)x;
	blttags[i].tti_Tag=TDISB_DSTY;  blttags[i++].tti_Value=(TTAG)y;
	if(ckey)
	{
		TIMGARGBCOLOR ckcol={0,0xff,0xff,0xff};
		blttags[i].tti_Tag=TDISB_CKEY;   blttags[i++].tti_Value=(TTAG)&ckcol;
	}

	if(blend)
		blttags[i].tti_Tag=TDISB_CALPHA; blttags[i++].tti_Value=(TTAG)0xc0;

	blttags[i].tti_Tag=TTAG_DONE;

	TDisBlit(dhbase,theBitmap,blttags);
}

/**************************************************************************
	RenderBitmap
	------------

	draw some weird pixels directly into to the imgp bitmap
 **************************************************************************/
TVOID RenderDirectBitmap(THNDL *theBitmap)
{
	TIMGPICTURE lockimg;

	if(TDisLock(dhbase,theBitmap,&lockimg))
	{
		if(lockimg.data)
		{
			TINT x,y;
			TUINT8 *d=(TUINT8*)lockimg.data;
			TINT xoff=lockimg.width/4;
			TINT yoff=lockimg.height/4;
			for(y=yoff;y<lockimg.height-yoff;y++)
			{
				for(x=xoff;x<lockimg.width-xoff;x++)
				{
					d[y*lockimg.bytesperrow+x]=x;
				}
			}
		}
		TDisUnlock(dhbase);
	}
}

/**************************************************************************
	RenderOpenGl
	------------

	if the created display supports opengl, here we render something with
	standard opengl commands.
 **************************************************************************/
TVOID RenderDirectOpenGl(THNDL *theDisplay)
{
	TIMGPICTURE lockimg;
	static TFLOAT wy=0.0f;

	if(TDisLock(dhbase,theDisplay,&lockimg))
	{
		glDisable( GL_LIGHTING );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );
		glEnable( GL_BLEND );
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();

		gluPerspective(48.0f,(TFLOAT)320/(TFLOAT)240,5,2000);

		glTranslatef(0,0,-25);

		glViewport(0,0,lockimg.width,lockimg.height);

		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		glRotatef(wy,0,1,0);
		wy+=0.1f;

		glBegin(GL_TRIANGLES);

		glColor4f(1,0,0,0.5f);
		glVertex3f(0,-10,0);

		glColor4f(0,1,0,0.7f);
		glVertex3f(-10,10,0);

		glColor4f(0,0,1,0.9f);
		glVertex3f(10,10,0);

		glEnd();

		TDisUnlock(dhbase);
	}
}
