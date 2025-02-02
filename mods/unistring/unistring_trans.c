
/*
**	$Id: unistring_trans.c,v 1.4 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_trans.c - Transformations
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include "unistring_mod.h"

/*****************************************************************************/

static const TINT16
ConversionTable[] =
{
	/* reversible small <-> capital */
	0,	0x0061, /* LATIN SMALL LETTER A */                     0x0041, /* LATIN CAPITAL LETTER A */
	0,	0x0062, /* LATIN SMALL LETTER B */                     0x0042, /* LATIN CAPITAL LETTER B */
	0,	0x0063, /* LATIN SMALL LETTER C */                     0x0043, /* LATIN CAPITAL LETTER C */
	0,	0x0064, /* LATIN SMALL LETTER D */                     0x0044, /* LATIN CAPITAL LETTER D */
	0,	0x0065, /* LATIN SMALL LETTER E */                     0x0045, /* LATIN CAPITAL LETTER E */
	0,	0x0066, /* LATIN SMALL LETTER F */                     0x0046, /* LATIN CAPITAL LETTER F */
	0,	0x0067, /* LATIN SMALL LETTER G */                     0x0047, /* LATIN CAPITAL LETTER G */
	0,	0x0068, /* LATIN SMALL LETTER H */                     0x0048, /* LATIN CAPITAL LETTER H */
	0,	0x0069, /* LATIN SMALL LETTER I */                     0x0049, /* LATIN CAPITAL LETTER I */
	0,	0x006A, /* LATIN SMALL LETTER J */                     0x004A, /* LATIN CAPITAL LETTER J */
	0,	0x006B, /* LATIN SMALL LETTER K */                     0x004B, /* LATIN CAPITAL LETTER K */
	0,	0x006C, /* LATIN SMALL LETTER L */                     0x004C, /* LATIN CAPITAL LETTER L */
	0,	0x006D, /* LATIN SMALL LETTER M */                     0x004D, /* LATIN CAPITAL LETTER M */
	0,	0x006E, /* LATIN SMALL LETTER N */                     0x004E, /* LATIN CAPITAL LETTER N */
	0,	0x006F, /* LATIN SMALL LETTER O */                     0x004F, /* LATIN CAPITAL LETTER O */
	0,	0x0070, /* LATIN SMALL LETTER P */                     0x0050, /* LATIN CAPITAL LETTER P */
	0,	0x0071, /* LATIN SMALL LETTER Q */                     0x0051, /* LATIN CAPITAL LETTER Q */
	0,	0x0072, /* LATIN SMALL LETTER R */                     0x0052, /* LATIN CAPITAL LETTER R */
	0,	0x0073, /* LATIN SMALL LETTER S */                     0x0053, /* LATIN CAPITAL LETTER S */
	0,	0x0074, /* LATIN SMALL LETTER T */                     0x0054, /* LATIN CAPITAL LETTER T */
	0,	0x0075, /* LATIN SMALL LETTER U */                     0x0055, /* LATIN CAPITAL LETTER U */
	0,	0x0076, /* LATIN SMALL LETTER V */                     0x0056, /* LATIN CAPITAL LETTER V */
	0,	0x0077, /* LATIN SMALL LETTER W */                     0x0057, /* LATIN CAPITAL LETTER W */
	0,	0x0078, /* LATIN SMALL LETTER X */                     0x0058, /* LATIN CAPITAL LETTER X */
	0,	0x0079, /* LATIN SMALL LETTER Y */                     0x0059, /* LATIN CAPITAL LETTER Y */
	0,	0x007A, /* LATIN SMALL LETTER Z */                     0x005A, /* LATIN CAPITAL LETTER Z */
	0,	0x00E0, /* LATIN SMALL LETTER A WITH GRAVE */          0x00C0, /* LATIN CAPITAL LETTER A WITH GRAVE */
	0,	0x00E1, /* LATIN SMALL LETTER A WITH ACUTE */          0x00C1, /* LATIN CAPITAL LETTER A WITH ACUTE */
	0,	0x00E2, /* LATIN SMALL LETTER A WITH CIRCUMFLEX */     0x00C2, /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
	0,	0x00E3, /* LATIN SMALL LETTER A WITH TILDE */          0x00C3, /* LATIN CAPITAL LETTER A WITH TILDE */
	0,	0x00E4, /* LATIN SMALL LETTER A WITH DIAERESIS */      0x00C4, /* LATIN CAPITAL LETTER A WITH DIAERESIS */
	0,	0x00E5, /* LATIN SMALL LETTER A WITH RING ABOVE */     0x00C5, /* LATIN CAPITAL LETTER A WITH RING ABOVE */
	0,	0x00E6, /* LATIN SMALL LETTER AE */                    0x00C6, /* LATIN CAPITAL LETTER AE */
	0,	0x00E7, /* LATIN SMALL LETTER C WITH CEDILLA */        0x00C7, /* LATIN CAPITAL LETTER C WITH CEDILLA */
	0,	0x00E8, /* LATIN SMALL LETTER E WITH GRAVE */          0x00C8, /* LATIN CAPITAL LETTER E WITH GRAVE */
	0,	0x00E9, /* LATIN SMALL LETTER E WITH ACUTE */          0x00C9, /* LATIN CAPITAL LETTER E WITH ACUTE */
	0,	0x00EA, /* LATIN SMALL LETTER E WITH CIRCUMFLEX */     0x00CA, /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
	0,	0x00EB, /* LATIN SMALL LETTER E WITH DIAERESIS */      0x00CB, /* LATIN CAPITAL LETTER E WITH DIAERESIS */
	0,	0x00EC, /* LATIN SMALL LETTER I WITH GRAVE */          0x00CC, /* LATIN CAPITAL LETTER I WITH GRAVE */
	0,	0x00ED, /* LATIN SMALL LETTER I WITH ACUTE */          0x00CD, /* LATIN CAPITAL LETTER I WITH ACUTE */
	0,	0x00EE, /* LATIN SMALL LETTER I WITH CIRCUMFLEX */     0x00CE, /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
	0,	0x00EF, /* LATIN SMALL LETTER I WITH DIAERESIS */      0x00CF, /* LATIN CAPITAL LETTER I WITH DIAERESIS */
	0,	0x00F0, /* LATIN SMALL LETTER ETH */                   0x00D0, /* LATIN CAPITAL LETTER ETH */
	0,	0x00F1, /* LATIN SMALL LETTER N WITH TILDE */          0x00D1, /* LATIN CAPITAL LETTER N WITH TILDE */
	0,	0x00F2, /* LATIN SMALL LETTER O WITH GRAVE */          0x00D2, /* LATIN CAPITAL LETTER O WITH GRAVE */
	0,	0x00F3, /* LATIN SMALL LETTER O WITH ACUTE */          0x00D3, /* LATIN CAPITAL LETTER O WITH ACUTE */
	0,	0x00F4, /* LATIN SMALL LETTER O WITH CIRCUMFLEX */     0x00D4, /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
	0,	0x00F5, /* LATIN SMALL LETTER O WITH TILDE */          0x00D5, /* LATIN CAPITAL LETTER O WITH TILDE */
	0,	0x00F6, /* LATIN SMALL LETTER O WITH DIAERESIS */      0x00D6, /* LATIN CAPITAL LETTER O WITH DIAERESIS */
	0,	0x00F8, /* LATIN SMALL LETTER O WITH STROKE */         0x00D8, /* LATIN CAPITAL LETTER O WITH STROKE */
	0,	0x00F9, /* LATIN SMALL LETTER U WITH GRAVE */          0x00D9, /* LATIN CAPITAL LETTER U WITH GRAVE */
	0,	0x00FA, /* LATIN SMALL LETTER U WITH ACUTE */          0x00DA, /* LATIN CAPITAL LETTER U WITH ACUTE */
	0,	0x00FB, /* LATIN SMALL LETTER U WITH CIRCUMFLEX */     0x00DB, /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
	0,	0x00FC, /* LATIN SMALL LETTER U WITH DIAERESIS */      0x00DC, /* LATIN CAPITAL LETTER U WITH DIAERESIS */
	0,	0x00FD, /* LATIN SMALL LETTER Y WITH ACUTE */          0x00DD, /* LATIN CAPITAL LETTER Y WITH ACUTE */
	0,	0x00FE, /* LATIN SMALL LETTER THORN */                 0x00DE, /* LATIN CAPITAL LETTER THORN */
	0,	0x00FF, /* LATIN SMALL LETTER Y WITH DIAERESIS */      0x0178, /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
	0,	0x0101, /* LATIN SMALL LETTER A WITH MACRON */         0x0100, /* LATIN CAPITAL LETTER A WITH MACRON */
	0,	0x0103, /* LATIN SMALL LETTER A WITH BREVE */          0x0102, /* LATIN CAPITAL LETTER A WITH BREVE */
	0,	0x0105, /* LATIN SMALL LETTER A WITH OGONEK */         0x0104, /* LATIN CAPITAL LETTER A WITH OGONEK */
	0,	0x0107, /* LATIN SMALL LETTER C WITH ACUTE */          0x0106, /* LATIN CAPITAL LETTER C WITH ACUTE */
	0,	0x0109, /* LATIN SMALL LETTER C WITH CIRCUMFLEX */     0x0108, /* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
	0,	0x010B, /* LATIN SMALL LETTER C WITH DOT ABOVE */      0x010A, /* LATIN CAPITAL LETTER C WITH DOT ABOVE */
	0,	0x010D, /* LATIN SMALL LETTER C WITH CARON */          0x010C, /* LATIN CAPITAL LETTER C WITH CARON */
	0,	0x010F, /* LATIN SMALL LETTER D WITH CARON */          0x010E, /* LATIN CAPITAL LETTER D WITH CARON */
	0,	0x0111, /* LATIN SMALL LETTER D WITH STROKE */         0x0110, /* LATIN CAPITAL LETTER D WITH STROKE */
	0,	0x0113, /* LATIN SMALL LETTER E WITH MACRON */         0x0112, /* LATIN CAPITAL LETTER E WITH MACRON */
	0,	0x0115, /* LATIN SMALL LETTER E WITH BREVE */          0x0114, /* LATIN CAPITAL LETTER E WITH BREVE */
	0,	0x0117, /* LATIN SMALL LETTER E WITH DOT ABOVE */      0x0116, /* LATIN CAPITAL LETTER E WITH DOT ABOVE */
	0,	0x0119, /* LATIN SMALL LETTER E WITH OGONEK */         0x0118, /* LATIN CAPITAL LETTER E WITH OGONEK */
	0,	0x011B, /* LATIN SMALL LETTER E WITH CARON */          0x011A, /* LATIN CAPITAL LETTER E WITH CARON */
	0,	0x011D, /* LATIN SMALL LETTER G WITH CIRCUMFLEX */     0x011C, /* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
	0,	0x011F, /* LATIN SMALL LETTER G WITH BREVE */          0x011E, /* LATIN CAPITAL LETTER G WITH BREVE */
	0,	0x0121, /* LATIN SMALL LETTER G WITH DOT ABOVE */      0x0120, /* LATIN CAPITAL LETTER G WITH DOT ABOVE */
	0,	0x0123, /* LATIN SMALL LETTER G WITH CEDILLA */        0x0122, /* LATIN CAPITAL LETTER G WITH CEDILLA */
	0,	0x0125, /* LATIN SMALL LETTER H WITH CIRCUMFLEX */     0x0124, /* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
	0,	0x0127, /* LATIN SMALL LETTER H WITH STROKE */         0x0126, /* LATIN CAPITAL LETTER H WITH STROKE */
	0,	0x0129, /* LATIN SMALL LETTER I WITH TILDE */          0x0128, /* LATIN CAPITAL LETTER I WITH TILDE */
	0,	0x012B, /* LATIN SMALL LETTER I WITH MACRON */         0x012A, /* LATIN CAPITAL LETTER I WITH MACRON */
	0,	0x012D, /* LATIN SMALL LETTER I WITH BREVE */          0x012C, /* LATIN CAPITAL LETTER I WITH BREVE */
	0,	0x012F, /* LATIN SMALL LETTER I WITH OGONEK */         0x012E, /* LATIN CAPITAL LETTER I WITH OGONEK */
	0,	0x0133, /* LATIN SMALL LIGATURE IJ */                  0x0132, /* LATIN CAPITAL LIGATURE IJ */
	0,	0x0135, /* LATIN SMALL LETTER J WITH CIRCUMFLEX */     0x0134, /* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
	0,	0x0137, /* LATIN SMALL LETTER K WITH CEDILLA */        0x0136, /* LATIN CAPITAL LETTER K WITH CEDILLA */
	0,	0x013A, /* LATIN SMALL LETTER L WITH ACUTE */          0x0139, /* LATIN CAPITAL LETTER L WITH ACUTE */
	0,	0x013C, /* LATIN SMALL LETTER L WITH CEDILLA */        0x013B, /* LATIN CAPITAL LETTER L WITH CEDILLA */
	0,	0x013E, /* LATIN SMALL LETTER L WITH CARON */          0x013D, /* LATIN CAPITAL LETTER L WITH CARON */
	0,	0x0140, /* LATIN SMALL LETTER L WITH MIDDLE DOT */     0x013F, /* LATIN CAPITAL LETTER L WITH MIDDLE DOT */
	0,	0x0142, /* LATIN SMALL LETTER L WITH STROKE */         0x0141, /* LATIN CAPITAL LETTER L WITH STROKE */
	0,	0x0144, /* LATIN SMALL LETTER N WITH ACUTE */          0x0143, /* LATIN CAPITAL LETTER N WITH ACUTE */
	0,	0x0146, /* LATIN SMALL LETTER N WITH CEDILLA */        0x0145, /* LATIN CAPITAL LETTER N WITH CEDILLA */
	0,	0x0148, /* LATIN SMALL LETTER N WITH CARON */          0x0147, /* LATIN CAPITAL LETTER N WITH CARON */
	0,	0x014B, /* LATIN SMALL LETTER ENG */                   0x014A, /* LATIN CAPITAL LETTER ENG */
	0,	0x014D, /* LATIN SMALL LETTER O WITH MACRON */         0x014C, /* LATIN CAPITAL LETTER O WITH MACRON */
	0,	0x014F, /* LATIN SMALL LETTER O WITH BREVE */          0x014E, /* LATIN CAPITAL LETTER O WITH BREVE */
	0,	0x0151, /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */   0x0150, /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
	0,	0x0153, /* LATIN SMALL LIGATURE OE */                  0x0152, /* LATIN CAPITAL LIGATURE OE */
	0,	0x0155, /* LATIN SMALL LETTER R WITH ACUTE */          0x0154, /* LATIN CAPITAL LETTER R WITH ACUTE */
	0,	0x0157, /* LATIN SMALL LETTER R WITH CEDILLA */        0x0156, /* LATIN CAPITAL LETTER R WITH CEDILLA */
	0,	0x0159, /* LATIN SMALL LETTER R WITH CARON */          0x0158, /* LATIN CAPITAL LETTER R WITH CARON */
	0,	0x015B, /* LATIN SMALL LETTER S WITH ACUTE */          0x015A, /* LATIN CAPITAL LETTER S WITH ACUTE */
	0,	0x015D, /* LATIN SMALL LETTER S WITH CIRCUMFLEX */     0x015C, /* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
	0,	0x015F, /* LATIN SMALL LETTER S WITH CEDILLA */        0x015E, /* LATIN CAPITAL LETTER S WITH CEDILLA */
	0,	0x0161, /* LATIN SMALL LETTER S WITH CARON */          0x0160, /* LATIN CAPITAL LETTER S WITH CARON */
	0,	0x0163, /* LATIN SMALL LETTER T WITH CEDILLA */        0x0162, /* LATIN CAPITAL LETTER T WITH CEDILLA */
	0,	0x0165, /* LATIN SMALL LETTER T WITH CARON */          0x0164, /* LATIN CAPITAL LETTER T WITH CARON */
	0,	0x0167, /* LATIN SMALL LETTER T WITH STROKE */         0x0166, /* LATIN CAPITAL LETTER T WITH STROKE */
	0,	0x0169, /* LATIN SMALL LETTER U WITH TILDE */          0x0168, /* LATIN CAPITAL LETTER U WITH TILDE */
	0,	0x016B, /* LATIN SMALL LETTER U WITH MACRON */         0x016A, /* LATIN CAPITAL LETTER U WITH MACRON */
	0,	0x016D, /* LATIN SMALL LETTER U WITH BREVE */          0x016C, /* LATIN CAPITAL LETTER U WITH BREVE */
	0,	0x016F, /* LATIN SMALL LETTER U WITH RING ABOVE */     0x016E, /* LATIN CAPITAL LETTER U WITH RING ABOVE */
	0,	0x0171, /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */   0x0170, /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
	0,	0x0173, /* LATIN SMALL LETTER U WITH OGONEK */         0x0172, /* LATIN CAPITAL LETTER U WITH OGONEK */
	0,	0x0175, /* LATIN SMALL LETTER W WITH CIRCUMFLEX */     0x0174, /* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
	0,	0x0177, /* LATIN SMALL LETTER Y WITH CIRCUMFLEX */     0x0176, /* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
	0,	0x017A, /* LATIN SMALL LETTER Z WITH ACUTE */          0x0179, /* LATIN CAPITAL LETTER Z WITH ACUTE */
	0,	0x017C, /* LATIN SMALL LETTER Z WITH DOT ABOVE */      0x017B, /* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
	0,	0x017E, /* LATIN SMALL LETTER Z WITH CARON */          0x017D, /* LATIN CAPITAL LETTER Z WITH CARON */

	/* irreversible small -> capital */
	1,	0x0131, /* LATIN SMALL LETTER DOTLESS I */             0x0049, /* LATIN CAPITAL LETTER I */
	1,	0x017F, /* LATIN SMALL LETTER LONG S */                0x0053, /* LATIN CAPITAL LETTER S */

	/* irreversible small <- capital */
	2,	0x0069, /* LATIN SMALL LETTER I */                     0x0130, /* LATIN CAPITAL LETTER I WITH DOT ABOVE */

	/* irreversible small -> capital, multiple characters (null-terminated) */
	3,	0x00DF, /* LATIN SMALL LETTER SHARP S */               0x0053, 0x0053, 0, /* 2 LATIN CAPITAL LETTER S */

	/* undefined, no case conversion */
	4,	0x0138,	/* LATIN SMALL LETTER KRA */

		
	/* control, formatting */
	7,	0x007f,

	/* control, formatting (ranges) */
	8,	0x0000, 0x001f,
	8,	0x200c, 0x200f,
	8,	0x0080, 0x009f,

	/* spaces */
	5,	0x0009,	/* TAB */
	5,	0x000b,	/* VERTICAL TAB */
	5,	0x000c,	/* FF */
	5,	0x000d,	/* CR */
	5,	0x0020,	/* SPACE */
	5,	0x00a0,	/* NO-BREAK SPACE */

	/* spaces (ranges) */
	6,	0x2000, 0x200b,

	/* end. */
	-1
};

/*****************************************************************************/

LOCAL TBOOL 
mod_initcaseconversion(TMOD_US *mod)
{
	TINT16 c, *ptr = (TINT16 *) ConversionTable;

	if ((mod->us_SmallToCaps = TExecAlloc0(TExecBase, mod->us_MMU, 
			CASECONVRANGE * sizeof(TUINT16 *))) == TNULL)
	{
		return TFALSE;
	}

	if ((mod->us_CapsToSmall = TExecAlloc0(TExecBase, mod->us_MMU, 
			CASECONVRANGE * sizeof(TUINT16 *))) == TNULL)
	{
		TExecFree(TExecBase, mod->us_SmallToCaps);
		return TFALSE;
	}

	if ((mod->us_CharInfo = TExecAlloc(TExecBase, mod->us_MMU, 
			CHARINFORANGE)) == TNULL)
	{
		TExecFree(TExecBase, mod->us_SmallToCaps);
		TExecFree(TExecBase, mod->us_CapsToSmall);
		return TFALSE;
	}

	TExecFillMem(TExecBase, mod->us_CharInfo, CHARINFORANGE, C_UNDEFINED);

	while ((c = *ptr) != -1)
	{
		switch (c)
		{
			case 0:		/* reversible */
				mod->us_SmallToCaps[ptr[1]] = ptr;
				mod->us_CapsToSmall[ptr[2]] = ptr;
				mod->us_CharInfo[ptr[1]] = C_LATIN;
				mod->us_CharInfo[ptr[2]] = C_CAPITAL;
				ptr += 3;
				break;

			case 1:		/* irreversible small -> capital */
				mod->us_SmallToCaps[ptr[1]] = ptr;
				mod->us_CharInfo[ptr[1]] = C_IRREV;
				ptr += 3;
				break;

			case 2:		/* irreversible small <- capital */
				mod->us_CapsToSmall[ptr[2]] = ptr;
				mod->us_CharInfo[ptr[2]] = C_CAPITAL | C_IRREV;
				ptr += 3;
				break;

			case 3:		/* irreversible small -> capital, multichar */
				mod->us_SmallToCaps[ptr[1]] = ptr;
				mod->us_CharInfo[ptr[1]] = C_IRREV | C_MULTICHAR;
				while ((*ptr++));
				break;
			
			case 4:		/* undefined, no conversion */
				mod->us_CharInfo[ptr[1]] = C_NOCASE;
				ptr += 2;
				break;
			
			case 5:		/* space */
				mod->us_CharInfo[ptr[1]] = C_SPACE;
				ptr += 2;
				break;

			case 6:		/* spaces (range) */
			{
				TINT i;
				for (i = ptr[1]; i <= ptr[2]; ++i)
					mod->us_CharInfo[i] = C_SPACE;
				ptr += 3;
				break;
			}

			case 7:		/* control, formatting */
				mod->us_CharInfo[ptr[1]] = C_CONTROL;
				ptr += 2;
				break;

			case 8:		/* control, formatting (range) */
			{
				TINT i;
				for (i = ptr[1]; i <= ptr[2]; ++i)
					mod->us_CharInfo[i] = C_CONTROL;
				ptr += 3;
				break;
			}
		}
	}
	
	return TTRUE;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_trans.c,v $
**	Revision 1.4  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.3  2004/07/18 20:50:24  tmueller
**	character info added
**	
**	Revision 1.2  2004/04/04 12:20:29  tmueller
**	Datatype TDIndex renamed to TUString. Docs and Prototypes adapted.
**	
**	Revision 1.1  2004/02/15 05:02:27  tmueller
**	Transformation rules for uppercase/lowercase conversions added
**	
**	Revision 1.10  2004/02/14 21:13:15  tmueller
*/
