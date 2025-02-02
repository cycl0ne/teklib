/*      this header file contains all function prototypes for a
	display host module, the function "Dismod_InitMod()"
	for initializing the module pointers, and the functions
	mod_open and mod_close
*/

/* module prototypes */
static TCALLBACK TMOD_DISMOD *mod_open(TMOD_DISMOD *mod, TAPTR selftask, TTAGITEM *tags);
static TCALLBACK TVOID mod_close(TMOD_DISMOD *dismod, TAPTR selftask);

TMODAPI TBOOL dismod_create                    (TMOD_DISMOD *dismod,TSTRPTR title,TINT x, TINT y, TINT w, TINT h, TINT d, TUINT flags);
TMODAPI TVOID dismod_destroy                   (TMOD_DISMOD *dismod);
TMODAPI TVOID dismod_getproperties             (TMOD_DISMOD *dismod,TDISPROPS *props);
TMODAPI TINT dismod_getmodelist                (TMOD_DISMOD *dismod,TDISMODE **modelist);
TMODAPI TVOID dismod_waitmsg                   (TMOD_DISMOD *dismod);
TMODAPI TBOOL dismod_getmsg                                     (TMOD_DISMOD *dismod,TDISMSG *dismsg);
TMODAPI TVOID dismod_setattrs                                   (TMOD_DISMOD *dismod,TTAGITEM *tags);
TMODAPI TVOID dismod_flush                             (TMOD_DISMOD *dismod);
TMODAPI TVOID dismod_getcaps                                    (TMOD_DISMOD *dismod,TDISCAPS *caps);
TMODAPI TBOOL dismod_allocpen                  (TMOD_DISMOD *dismod,TDISPEN *pen);
TMODAPI TVOID dismod_freepen                   (TMOD_DISMOD *dismod,TDISPEN *pen);
TMODAPI TVOID dismod_setdpen                   (TMOD_DISMOD *dismod,TDISPEN *pen);
TMODAPI TBOOL dismod_setpalette                (TMOD_DISMOD *dismod,TIMGARGBCOLOR *pal,TINT sp,TINT sd,TINT numentries);
TMODAPI TBOOL dismod_allocbitmap               (TMOD_DISMOD *dismod,TDISBITMAP *bitmap,TINT width,TINT height,TINT flags);
TMODAPI TVOID dismod_freebitmap                (TMOD_DISMOD *dismod,TDISBITMAP *bitmap);

TMODAPI TVOID dismod_describe_dis              (TMOD_DISMOD *dismod,TDISDESCRIPTOR *desc);
TMODAPI TVOID dismod_describe_bm               (TMOD_DISMOD *dismod,TDISBITMAP *bm,TDISDESCRIPTOR *desc);
TMODAPI TBOOL dismod_lock_dis                  (TMOD_DISMOD *dismod,TIMGPICTURE *img);
TMODAPI TBOOL dismod_lock_bm                   (TMOD_DISMOD *dismod,TDISBITMAP *bm,TIMGPICTURE *img);
TMODAPI TVOID dismod_unlock_dis                (TMOD_DISMOD *dismod);
TMODAPI TVOID dismod_unlock_bm                 (TMOD_DISMOD *dismod,TDISBITMAP *bm);
TMODAPI TBOOL dismod_begin_dis                 (TMOD_DISMOD *dismod);
TMODAPI TBOOL dismod_begin_bm                  (TMOD_DISMOD *dismod,TDISBITMAP *bm);
TMODAPI TVOID dismod_end_dis                   (TMOD_DISMOD *dismod);
TMODAPI TVOID dismod_end_bm                    (TMOD_DISMOD *dismod,TDISBITMAP *bm);

TMODAPI TVOID dismod_blit                      (TMOD_DISMOD *dismod,TDISBITMAP *bm,TDBLITOPS *bops);

TMODAPI TVOID dismod_textout_dis               (TMOD_DISMOD *dismod,TINT8 *text,TINT row,TINT column);
TMODAPI TVOID dismod_textout_bm                (TMOD_DISMOD *dismod,TINT8 *text,TINT row,TINT column);

TMODAPI TVOID dismod_putimage_dis              (TMOD_DISMOD *dismod,TIMGPICTURE *img,TDISRECT *src,TDISRECT *dst);
TMODAPI TVOID dismod_putimage_bm               (TMOD_DISMOD *dismod,TDISBITMAP *bm,TIMGPICTURE *img,TDISRECT *src,TDISRECT *dst);
TMODAPI TVOID dismod_putscaleimage_dis (TMOD_DISMOD *dismod,TIMGPICTURE *img,TDISRECT *src,TDISRECT *dst);
TMODAPI TVOID dismod_putscaleimage_bm  (TMOD_DISMOD *dismod,TDISBITMAP *bm,TIMGPICTURE *img,TDISRECT *src,TDISRECT *dst);

TMODAPI TVOID dismod_fill_dis                  (TMOD_DISMOD *dismod);
TMODAPI TVOID dismod_fill_bm                   (TMOD_DISMOD *dismod);
TMODAPI TVOID dismod_plot_dis                  (TMOD_DISMOD *dismod,TINT x,TINT y);
TMODAPI TVOID dismod_plot_bm                   (TMOD_DISMOD *dismod,TINT x,TINT y);
TMODAPI TVOID dismod_line_dis                  (TMOD_DISMOD *dismod,TINT sx,TINT sy,TINT dx,TINT dy);
TMODAPI TVOID dismod_line_bm                   (TMOD_DISMOD *dismod,TINT sx,TINT sy,TINT dx,TINT dy);
TMODAPI TVOID dismod_box_dis                   (TMOD_DISMOD *dismod,TINT sx,TINT sy,TINT w,TINT h);
TMODAPI TVOID dismod_box_bm                    (TMOD_DISMOD *dismod,TINT sx,TINT sy,TINT w,TINT h);
TMODAPI TVOID dismod_boxf_dis                  (TMOD_DISMOD *dismod,TINT sx,TINT sy,TINT w,TINT h);
TMODAPI TVOID dismod_boxf_bm                   (TMOD_DISMOD *dismod,TINT sx,TINT sy,TINT w,TINT h);
TMODAPI TVOID dismod_poly_dis                  (TMOD_DISMOD *dismod,TINT numpoints,TINT *points);
TMODAPI TVOID dismod_poly_bm                   (TMOD_DISMOD *dismod,TINT numpoints,TINT *points);
TMODAPI TVOID dismod_polyf_dis                 (TMOD_DISMOD *dismod,TINT numpoints,TINT *points);
TMODAPI TVOID dismod_polyf_bm                  (TMOD_DISMOD *dismod,TINT numpoints,TINT *points);
TMODAPI TVOID dismod_ellipse_dis               (TMOD_DISMOD *dismod,TINT x,TINT y,TINT rx,TINT ry);
TMODAPI TVOID dismod_ellipse_bm                (TMOD_DISMOD *dismod,TINT x,TINT y,TINT rx,TINT ry);
TMODAPI TVOID dismod_ellipsef_dis              (TMOD_DISMOD *dismod,TINT x,TINT y,TINT rx,TINT ry);
TMODAPI TVOID dismod_ellipsef_bm                   (TMOD_DISMOD *dismod,TINT x,TINT y,TINT rx,TINT ry);
TMODAPI TVOID dismod_movepixels_dis            (TMOD_DISMOD *dismod,TINT sx,TINT sy,TINT dx,TINT dy, TINT w, TINT h);
TMODAPI TVOID dismod_movepixels_bm             (TMOD_DISMOD *dismod,TINT sx,TINT sy,TINT dx,TINT dy, TINT w, TINT h);


/**************************************************************************
	tek_init
 **************************************************************************/
TUINT Dismod_InitMod(TAPTR selftask, TMOD_DISMOD *mod, TUINT16 version, TTAGITEM *tags)
{
	if (!mod)
	{
		if (version != 0xffff)                                  /* first call */
		{
			if (version <= MOD_VERSION)                     /* version check */
			{
				return sizeof(TMOD_DISMOD);    /* return module positive size */
			}
		}
		else                                                                    /* second call */
		{
			return sizeof(TAPTR) * 90;                      /* return module negative size */
		}
	}
	else                                                                            /* third call */
	{
		mod->module.tmd_Version = MOD_VERSION;
		mod->module.tmd_Revision = MOD_REVISION;

		/* this module has instances. place instance
		** open/close functions into the module structure. */
		mod->module.tmd_OpenFunc = (TMODOPENFUNC) mod_open;
		mod->module.tmd_CloseFunc = (TMODCLOSEFUNC) mod_close;

		((TAPTR *) mod)[-1 ] = (TAPTR) dismod_create;
		((TAPTR *) mod)[-2 ] = (TAPTR) dismod_destroy;
		((TAPTR *) mod)[-3 ] = (TAPTR) dismod_getproperties;
		((TAPTR *) mod)[-4 ] = (TAPTR) dismod_getmodelist;
		((TAPTR *) mod)[-5 ] = (TAPTR) dismod_waitmsg;
		((TAPTR *) mod)[-6 ] = (TAPTR) dismod_getmsg;

		((TAPTR *) mod)[-7 ] = (TAPTR) dismod_setattrs;
		((TAPTR *) mod)[-9 ] = (TAPTR) dismod_flush;
		((TAPTR *) mod)[-10] = (TAPTR) dismod_getcaps;

		((TAPTR *) mod)[-20] = (TAPTR) dismod_allocpen;
		((TAPTR *) mod)[-21] = (TAPTR) dismod_freepen;
		((TAPTR *) mod)[-22] = (TAPTR) dismod_setdpen;
		((TAPTR *) mod)[-23] = (TAPTR) dismod_setpalette;
		((TAPTR *) mod)[-24] = (TAPTR) dismod_allocbitmap;
		((TAPTR *) mod)[-25] = (TAPTR) dismod_freebitmap;

		((TAPTR *) mod)[-30] = (TAPTR) dismod_describe_dis;
		((TAPTR *) mod)[-31] = (TAPTR) dismod_describe_bm;
		((TAPTR *) mod)[-32] = (TAPTR) dismod_lock_dis;
		((TAPTR *) mod)[-33] = (TAPTR) dismod_lock_bm;
		((TAPTR *) mod)[-34] = (TAPTR) dismod_unlock_dis;
		((TAPTR *) mod)[-35] = (TAPTR) dismod_unlock_bm;
		((TAPTR *) mod)[-36] = (TAPTR) dismod_begin_dis;
		((TAPTR *) mod)[-37] = (TAPTR) dismod_begin_bm;
		((TAPTR *) mod)[-38] = (TAPTR) dismod_end_dis;
		((TAPTR *) mod)[-39] = (TAPTR) dismod_end_bm;

		((TAPTR *) mod)[-40] = (TAPTR) dismod_blit;

		((TAPTR *) mod)[-50] = (TAPTR) dismod_textout_dis;
		((TAPTR *) mod)[-51] = (TAPTR) dismod_textout_bm;

		((TAPTR *) mod)[-61] = (TAPTR) dismod_putimage_dis;
		((TAPTR *) mod)[-62] = (TAPTR) dismod_putimage_bm;
		((TAPTR *) mod)[-63] = (TAPTR) dismod_putscaleimage_dis;
		((TAPTR *) mod)[-64] = (TAPTR) dismod_putscaleimage_bm;

		((TAPTR *) mod)[-70] = (TAPTR) dismod_fill_dis;
		((TAPTR *) mod)[-71] = (TAPTR) dismod_fill_bm;
		((TAPTR *) mod)[-72] = (TAPTR) dismod_plot_dis;
		((TAPTR *) mod)[-73] = (TAPTR) dismod_plot_bm;
		((TAPTR *) mod)[-74] = (TAPTR) dismod_line_dis;
		((TAPTR *) mod)[-75] = (TAPTR) dismod_line_bm;
		((TAPTR *) mod)[-76] = (TAPTR) dismod_box_dis;
		((TAPTR *) mod)[-77] = (TAPTR) dismod_box_bm;
		((TAPTR *) mod)[-78] = (TAPTR) dismod_boxf_dis;
		((TAPTR *) mod)[-79] = (TAPTR) dismod_boxf_bm;
		((TAPTR *) mod)[-80] = (TAPTR) dismod_poly_dis;
		((TAPTR *) mod)[-81] = (TAPTR) dismod_poly_bm;
		((TAPTR *) mod)[-82] = (TAPTR) dismod_polyf_dis;
		((TAPTR *) mod)[-83] = (TAPTR) dismod_polyf_bm;
		((TAPTR *) mod)[-84] = (TAPTR) dismod_ellipse_dis;
		((TAPTR *) mod)[-85] = (TAPTR) dismod_ellipse_bm;
		((TAPTR *) mod)[-86] = (TAPTR) dismod_ellipsef_dis;
		((TAPTR *) mod)[-87] = (TAPTR) dismod_ellipsef_bm;
		((TAPTR *) mod)[-88] = (TAPTR) dismod_movepixels_dis;
		((TAPTR *) mod)[-89] = (TAPTR) dismod_movepixels_bm;

		return TTRUE;
	}
	return 0;
}

/**************************************************************************
	open instance
 **************************************************************************/
static TCALLBACK TMOD_DISMOD *mod_open(TMOD_DISMOD *dismod, TAPTR selftask, TTAGITEM *tags)
{
	dismod = TNewInstance(dismod, dismod->module.tmd_PosSize, dismod->module.tmd_NegSize);
	if (!dismod) return TNULL;

	dismod->exec = TGetExecBase(dismod);
	dismod->util = TExecOpenModule(dismod->exec, "util", 0, TNULL);
	dismod->imgp = TExecOpenModule(dismod->exec, "imgproc", 0, TNULL);

	if(!Dismod_ReadProperties(dismod))
	{
		TExecCloseModule(dismod->exec, dismod->imgp);
		TExecCloseModule(dismod->exec, dismod->util);
		TFreeInstance(dismod);
		return TNULL;
	}

	return dismod;
}

/**************************************************************************
	close instance
 **************************************************************************/
static TCALLBACK TVOID mod_close(TMOD_DISMOD *dismod, TAPTR selftask)
{
	dismod_destroy(dismod);
	TExecCloseModule(dismod->exec, dismod->imgp);
	TExecCloseModule(dismod->exec, dismod->util);
	TFreeInstance(dismod);
}
