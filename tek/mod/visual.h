
#ifndef _TEK_MOD_VISUAL_H
#define _TEK_MOD_VISUAL_H

/*
**	$Id: visual.h,v 1.3 2005/09/13 02:45:09 tmueller Exp $
**	teklib/tek/mod/visual.h - Visual module definitions
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/exec.h>
#include <tek/mod/time.h>

/*****************************************************************************/
/*
**	Types
*/

typedef TAPTR TVPEN;				/*	a "pen" for rendering  */

/*****************************************************************************/
/*
**	Tags for creation and attachment
*/

#define TVISTAGS_					(TTAG_USER + 0x600)
#define TVisual_PixWidth			(TVISTAGS_ + 0)
#define TVisual_PixHeight			(TVISTAGS_ + 1)
#define TVisual_TextWidth			(TVISTAGS_ + 2)
#define TVisual_TextHeight			(TVISTAGS_ + 3)
#define TVisual_FontWidth			(TVISTAGS_ + 4)
#define TVisual_FontHeight			(TVISTAGS_ + 5)
#define TVisual_Title				(TVISTAGS_ + 6)
#define TVisual_Attach				(TVISTAGS_ + 7)

/*****************************************************************************/
/*
**	Input message
*/

typedef struct TInputMessage
{
	struct TNode timsg_Node;	/* Node header */
	TTIME timsg_TimeStamp;		/* Timestamp */
	TUINT timsg_Type;			/* Message type */
	TUINT timsg_Code;			/* Message code */
	TUINT timsg_Qualifier;		/* Keyboard qualifiers */
	TINT timsg_MouseX;			/* Mouse position */
	TINT timsg_MouseY;

	/* more fields may follow in the future. Do not use
	** sizeof(TIMSG). If necessary use TExecGetSize(imsg) */

} TIMSG;

/*****************************************************************************/
/*
**	Input types
*/

#define TITYPE_NONE				0x00000000
#define TITYPE_ALL				0xffffffff

#define TITYPE_CLOSE			0x00000001		/* Closed */
#define TITYPE_FOCUS			0x00000002		/* Focused/unfocused */
#define TITYPE_NEWSIZE			0x00000004		/* Resized */
#define TITYPE_REFRESH			0x00000008		/* Needs refreshing */
#define TITYPE_MOUSEOVER		0x00000010		/* Mouse moved in/out */

#define	TITYPE_COOKEDKEY		0x00000100		/* "Cooked" keystroke */
#define TITYPE_MOUSEMOVE		0x00000200		/* Mouse movement */
#define TITYPE_MOUSEBUTTON		0x00000400		/* Mouse button */

/*****************************************************************************/
/*
**	Mouse button codes
**	Styleguide notes: Some mice have only one or two buttons.
**	usage of the middle and right buttons without alternatives
**	is disencouraged.
*/

#define TMBCODE_LEFTDOWN		0x00000001
#define TMBCODE_LEFTUP			0x00000002
#define TMBCODE_RIGHTDOWN		0x00000004
#define TMBCODE_RIGHTUP			0x00000008
#define TMBCODE_MIDDLEDOWN		0x00000010
#define TMBCODE_MIDDLEUP		0x00000020

/*****************************************************************************/
/*
**	Keycodes
*/

#define TKEYC_NONE				0x00000000		/* may still be a qualifier */

/*
**	Function key codes
**	Styleguide notes: Some keyboards have only 10 F-Keys.
**	Hardcoded key bindings should never rely on F11 or F12
**	without offering some kind of alternative.
*/

#define TKEYC_F1				0x00000100
#define TKEYC_F2				0x00000101
#define TKEYC_F3				0x00000102
#define TKEYC_F4				0x00000103
#define TKEYC_F5				0x00000104
#define TKEYC_F6				0x00000105
#define TKEYC_F7				0x00000106
#define TKEYC_F8				0x00000107
#define TKEYC_F9				0x00000108
#define TKEYC_F10				0x00000109
#define TKEYC_F11				0x0000010a
#define TKEYC_F12				0x0000010b

/*
**	Cursor key codes
*/

#define	TKEYC_CRSRLEFT			0x00000200
#define	TKEYC_CRSRRIGHT			0x00000201
#define	TKEYC_CRSRUP			0x00000202
#define	TKEYC_CRSRDOWN			0x00000203

/*
**	Special key codes
*/

#define TKEYC_ESC				0x00000300	/* escape key */
#define TKEYC_DEL				0x00000301	/* del key */
#define TKEYC_BCKSPC			0x00000302	/* backspace key */
#define TKEYC_TAB				0x00000303	/* tab key */
#define TKEYC_ENTER				0x00000304	/* return/enter */

/*
**	Proprietary key codes
**	Styleguide notes: Whenever your application binds actions
**	to a key from this section, you should offer at least one
**	non-proprietary alternative
*/

#define TKEYC_HELP				0x00000400	/* help key (amiga) */
#define TKEYC_INSERT			0x00000401	/* insert key (pc) */
#define TKEYC_OVERWRITE			0x00000402	/* overwrite key (pc) */
#define	TKEYC_PAGEUP			0x00000403	/* page up (pc) */
#define	TKEYC_PAGEDOWN			0x00000404	/* page down (pc) */
#define TKEYC_POSONE			0x00000405	/* position one key (pc) */
#define TKEYC_POSEND			0x00000406	/* position end key (pc) */
#define TKEYC_PRINT				0x00000407	/* print key (pc) */
#define TKEYC_SCROLL			0x00000408	/* scroll down (pc) */
#define TKEYC_PAUSE				0x00000409	/* pause key (pc) */

/*
**	Keyboard qualifiers
**	Styleguide notes: Your application's default or hardcoded key
**	bindings should never rely on TKEYQ_PROP alone. Some keyboards
**	do not have a numblock, so usage of the TKEYQ_NUMBLOCK qualifier
**	without an alternative is disencouraged, too. Also note that some
**	keyboards don't have a right control key.
*/

#define	TKEYQ_NONE				0x00000000	/* no qualifier */
#define TKEYQ_LSHIFT			0x00000001
#define TKEYQ_RSHIFT			0x00000002
#define TKEYQ_LCTRL				0x00000004
#define TKEYQ_RCTRL				0x00000008
#define TKEYQ_LALT				0x00000010
#define TKEYQ_RALT				0x00000020
#define TKEYQ_LPROP				0x00000040	/* proprietary qualifier */
#define TKEYQ_RPROP				0x00000080
#define TKEYQ_NUMBLOCK			0x00000100	/* numkeypad is a qualifier */

/*	Combinations: Use e.g. if (msg->timsg_Qualifier & TKEYQ_SHIFT)
**	to find out whether any SHIFT key was held */

#define TKEYQ_SHIFT				(TKEYQ_LSHIFT|TKEYQ_RSHIFT)
#define TKEYQ_CTRL				(TKEYQ_LCTRL|TKEYQ_RCTRL)
#define TKEYQ_ALT				(TKEYQ_LALT|TKEYQ_RALT)
#define TKEYQ_PROP				(TKEYQ_LPROP|TKEYQ_RPROP)

/*****************************************************************************/
/*
**	Revision History
**	$Log: visual.h,v $
**	Revision 1.3  2005/09/13 02:45:09  tmueller
**	updated copyright reference
**	
**	Revision 1.2  2004/01/13 02:27:26  tmueller
**	Some structure fields renamed, TITYPE_KEY is now TITYPE_COOKEDKEY
**	
*/

#endif
