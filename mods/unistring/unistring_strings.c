
/*
**	$Id: unistring_strings.c,v 1.28 2005/09/13 02:42:42 tmueller Exp $
**	mods/unistring/unistring_strings.c - Strings storage class
**
**	Written by Timm S. Mueller <tmueller at neoscientists.org>
**	See copyright notice in teklib/COPYRIGHT
**
**	TODO:	str_encodeutf8 should also reject illegal UCS characters
*/

#include <tek/debug.h>
#include <tek/teklib.h>
#include "unistring_mod.h"

/*****************************************************************************/

static TINT setpos(TAHEAD * arr, TINT pos)
{
	return __array_seek(arr, &arr->tah_Cursor,
		pos - arr->tah_Cursor.tac_AbsPos);
}

/*****************************************************************************/

LOCAL TAHEAD *_str_valid(TMOD_US * mod, TUString idx)
{
	if (idx >= 0 && idx < mod->us_NumTotal) {
		TAHEAD *arr = mod->us_Array[idx];
		if (arr->tah_Flags & TDSTRF_VALID)
			return arr;
	}
	return TNULL;
}

/*****************************************************************************/

static TAPTR preparechar(TMOD_US * mod, TAHEAD * arr, TUINT8 * temp, TWCHAR c)
{
	switch (arr->tah_ElementSize) {
		case 0:
			if (c < 0x80) {
				temp[0] = c;
				return temp;
			}

			if (c < 0x100) {
				temp[0] = c;
				arr->tah_Flags |= TDSTRF_8BIT_PRESENT;
				return temp;
			}

			if (c < 0x10000) {
				if (_array_setsize(mod, arr, 1) < 0)
					return TNULL;
				arr->tah_Flags |= TDSTRF_UCS2_PRESENT;
			}

			/* fallthrough */

		case 1:
			if (c < 0x10000) {
				*((TUINT16 *) temp) = c;
				return temp;
			}

			if (_array_setsize(mod, arr, 2) < 0)
				return TNULL;
			arr->tah_Flags |= TDSTRF_UCS4_PRESENT;

			/* fallthrough */

		case 2:
			*((TWCHAR *) temp) = c;
			return temp;
	}

	return TNULL;
}

/*****************************************************************************/

static TVOID _str_insert_utf8(TMOD_US * mod, TAHEAD * arr, TWCHAR c)
{
	TUINT8 d;

	if (c < 0x80) {
		d = c;
		_array_ins(mod, arr, &d);
	} else if (c < 0x800) {
		d = 0xc0 | (TUINT8) (c >> 6);
		_array_ins(mod, arr, &d);
		d = 0x80 | (TUINT8) (c & 0x3f);
		_array_ins(mod, arr, &d);
	} else if (c < 0x10000) {
		d = 0xe0 | (c >> 12);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 6) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | (c & 0x3f);
		_array_ins(mod, arr, &d);
	} else if (c < 0x200000) {
		d = 0xf0 | (c >> 18);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 12) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 6) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | (c & 0x3f);
		_array_ins(mod, arr, &d);
	} else if (c < 0x4000000) {
		d = 0xf8 | (c >> 24);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 18) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 12) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 6) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | (c & 0x3f);
		_array_ins(mod, arr, &d);
	} else {
		d = 0xfc | (c >> 30);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 24) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 18) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 12) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | ((c >> 6) & 0x3f);
		_array_ins(mod, arr, &d);
		d = 0x80 | (c & 0x3f);
		_array_ins(mod, arr, &d);
	}
}

/*****************************************************************************/
/* 
**	idx = str_alloc(mod, initstr)
*/

EXPORT TUString str_alloc(TMOD_US * mod, TUINT8 * cstr)
{
	TUString s = array_alloc(mod, 0);
	if (s >= 0) {
		if (cstr) {
			if (str_insertstrn(mod, s, 0, cstr, 0x7fffffff, TASIZE_8BIT) < 0) {
				array_free(mod, s);
				s = TINVALID_STRING;
			}
		}
	}
	return s;
}

/*****************************************************************************/
/* 
**	len/err = str_insert(mod, str, pos, wchar)
*/

LOCAL TINT _str_insert(TMOD_US * mod, TAHEAD * arr, TINT pos, TWCHAR c)
{
	if (pos < 0)
		pos = arr->tah_Length + 1 + pos;
	if (setpos(arr, pos) == pos) {
		TUINT8 temp[4];
		if (preparechar(mod, arr, temp, c)) {
			if (_array_ins(mod, arr, temp) >= 0) {
				return arr->tah_Length;
			}
		}
	}
	return -1;
}

EXPORT TINT str_insert(TMOD_US * mod, TUString idx, TINT pos, TWCHAR c)
{
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr && c >= 0)
		return _str_insert(mod, arr, pos, c);
	return -1;
}

/*****************************************************************************/
/* 
**	char/err = str_remove(mod, str, pos)
*/

EXPORT TWCHAR str_remove(TMOD_US * mod, TUString idx, TINT pos)
{
	TWCHAR oldc = TINVALID_WCHAR;
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr) {
		if (pos < 0)
			pos = arr->tah_Length + 1 + pos;
		if (setpos(arr, pos) == pos) {
			TINT8 temp[4];
			if (_array_rem(mod, arr, temp)) {
				switch (arr->tah_ElementSize) {
					case 0:
						oldc = temp[0];
						break;
					case 1:
						oldc = *((TUINT16 *) temp);
						break;
					case 2:
						oldc = *((TINT *) temp);
						break;
				}
			}
		}
	}
	return oldc;
}

/*****************************************************************************/
/* 
**	ptr = str_map(mod, str, offs, len, mode)
**	return a pointer to a linear range of elements
*/

EXPORT TAPTR
str_map(TMOD_US * mod, TUString idx, TINT offs, TINT len, TUINT mode)
{
	TAPTR ptr = TNULL;
	if (idx >= 0 && idx < mod->us_NumTotal && len >= -1) {
		TAHEAD *arr = mod->us_Array[idx];
		TUINT flags = arr->tah_Flags;

		if (len == -1)
			len = arr->tah_Length;
		if (flags & TDSTRF_VALID) {
			switch (mode) {
				default:
					break;

				case TASIZE_7BIT:
					if (flags & TDSTRF_8BIT_PRESENT)
						break;

					/* fallthrough */

				case TASIZE_8BIT:
					if (flags & 0x18)
						break;
					if (_array_setsize(mod, arr, 0) < 0)
						break;
					ptr = _array_maplinear(mod, arr, offs, len);
					if (ptr) {
						/* from now on, we must assume that there
						 ** might be 8bit chars in the string */
						arr->tah_Flags |= TDSTRF_8BIT_PRESENT;
					}
					break;

				case TASIZE_16BIT:
					if (flags & 0x10)
						break;
					if (_array_setsize(mod, arr, 1) < 0)
						break;
					ptr = _array_maplinear(mod, arr, offs, len);
					break;

				case TASIZE_32BIT:
					if (_array_setsize(mod, arr, 2) < 0)
						break;
					ptr = _array_maplinear(mod, arr, offs, len);
					break;
			}
		}
	}
	return ptr;
}

/*****************************************************************************/
/* 
**	error = str_render(mod, str, ptr, offs, len, mode)
**	render a copy of a string into user-supplied memory. error:
**		0 success
**		-1 invalid arguments, invalid string
**		-2 information loss
*/

EXPORT TINT
str_render(TMOD_US * mod, TUString idx, TAPTR ptr, TINT ofs, TINT len,
	TUINT mode)
{
	TINT error = -1;
	if (idx >= 0 && idx < mod->us_NumTotal && ptr && len > 0) {
		TAHEAD *arr = mod->us_Array[idx];
		TUINT flags = arr->tah_Flags;
		if (flags & TDSTRF_VALID) {
			error = -2;			/* loss of information */
			switch (mode) {
				default:
					error = -1;	/* illegal arguments */
					break;

				case TASIZE_7BIT:
					if (flags & TDSTRF_8BIT_PRESENT)
						break;

				case TASIZE_8BIT:
					if (flags & 0x18)
						break;
					goto render;

				case TASIZE_16BIT:
					if (flags & 0x10)
						break;

					/* fallthrough */

				case TASIZE_32BIT:

				  render:
					error = _array_render(mod, arr, ptr, ofs, len, mode);
			}
		}
	}
	return error;
}

/*****************************************************************************/
/* 
**	length/err = str_set(mod, idx, pos, char)
**	set a character, or append it (pos < 0)
*/

EXPORT TINT str_set(TMOD_US * mod, TUString idx, TINT pos, TWCHAR c)
{
	TINT length = -1;
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr && c >= 0) {
		TUINT8 temp[4];
		TAPTR data = preparechar(mod, arr, temp, c);
		if (data) {
			TINT newpos;
			if (pos < 0)
				pos = arr->tah_Length + 1 + pos;
			newpos = setpos(arr, pos);
			if (newpos == arr->tah_Length) {
				if (_array_ins(mod, arr, data) >= 0) {
					length = arr->tah_Length;
				}
			} else if (newpos >= 0) {
				_array_set(arr, data);
				length = arr->tah_Length;
			}
		}
	}
	return length;
}

/*****************************************************************************/
/* 
**	char/err = str_get(mod, idx, pos)
*/

static TWCHAR __str_get(TACURSOR * cursor, TINT elementsize)
{
	TUINT8 temp[16];
	__array_get(cursor, elementsize, temp);
	switch (elementsize) {
		case 0:
			return temp[0];
		case 1:
			return *((TUINT16 *) temp);
		case 2:
			return *((TINT *) temp);
	}
	return TINVALID_WCHAR;
}

LOCAL TWCHAR _str_get(TAHEAD * arr, TINT pos)
{
	TACURSOR *cursor = &arr->tah_Cursor;
	if (pos < 0 || pos >= arr->tah_Length) {
		return TINVALID_WCHAR;
	} else if (pos == 0) {
		_array_seek(arr, cursor, 1, 0);
	} else {
		__array_seek(arr, cursor, pos - cursor->tac_AbsPos);
	}
	return __str_get(cursor, arr->tah_ElementSize);
}

EXPORT TWCHAR str_get(TMOD_US * mod, TUString idx, TINT pos)
{
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr) {
		if (pos < 0)
			pos = arr->tah_Length + 1 + pos;
		return _str_get(arr, pos);
	}
	return TINVALID_WCHAR;
}

/*****************************************************************************/
/* 
**	len/err = str_insertstrn(mod, str, pos, data, len, type)
**	insert a string
*/

EXPORT TINT
str_insertstrn(TMOD_US * mod, TUString idx, TINT pos, TAPTR instr,
	TINT len, TUINT mode)
{
	TINT newlen = -1;
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr && instr) {
		if (pos < 0)
			pos = arr->tah_Length + 1 + pos;
		if (setpos(arr, pos) == pos) {
			TUINT8 temp[4];
			TWCHAR c;
			TINT i;

			switch (mode) {
				default:
					return -1;

				case TASIZE_8BIT:
				{
					TUINT8 *p = instr;
					for (i = 0; i < len && (c = *p++); ++i) {
						if (preparechar(mod, arr, temp, c) == TFALSE ||
							_array_ins(mod, arr, temp) < 0) {
							return -1;
						}
					}
					break;
				}

				case TASIZE_16BIT:
				{
					TUINT16 *p = instr;
					for (i = 0; i < len && (c = *p++); ++i) {
						if (preparechar(mod, arr, temp, c) == TFALSE ||
							_array_ins(mod, arr, temp) < 0) {
							return -1;
						}
					}
					break;
				}

				case TASIZE_32BIT:
				{
					TWCHAR *p = instr;
					for (i = 0; i < len && (c = *p++); ++i) {
						if (preparechar(mod, arr, temp, c) == TFALSE ||
							_array_ins(mod, arr, temp) < 0) {
							return -1;
						}
					}
					break;
				}
			}

			newlen = arr->tah_Length;
		}
	}
	return newlen;
}

/*****************************************************************************/
/* 
**	len/err = str_insertutf8str(mod, str, pos, utf8str)
**	insert an utf8-encoded string
*/

EXPORT TINT
str_insertutf8str(TMOD_US * mod, TUString idx, TINT pos, TUINT8 * utf8str)
{
	TINT newlen = -1;
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr && utf8str) {
		if (pos < 0)
			pos = arr->tah_Length + 1 + pos;
		if (setpos(arr, pos) == pos) {
			TUINT8 temp[4];
			TUINT8 c;
			TWCHAR d = 0;
			TINT expc = 0, j = 0;

			while ((c = *utf8str++)) {
				if (c < 0x80) {
					if (expc)
						return -1;
					d = c;
				} else if (c < 0xc0) {
					if (expc == 0)
						return -1;
					d <<= 6;
					d |= c & 0x3f;
					if (--expc)
						continue;
				} else {
					if (expc)
						return -1;
					if (c < 0xe0) {
						d = c & 31;
						expc = 1;
					} else if (c < 0xf0) {
						d = c & 15;
						expc = 2;
					} else if (c < 0xf8) {
						d = c & 7;
						expc = 3;
					} else if (c < 0xfc) {
						d = c & 3;
						expc = 4;
					} else if (c < 0xfe) {
						d = c & 1;
						expc = 5;
					} else
						return -1;

					j = expc;
					continue;
				}

				/* catch overlong encodings */
				switch (j) {
					case 5:
						if (d < 0x4000000)
							return -1;
						break;
					case 4:
						if (d < 0x200000)
							return -1;
						break;
					case 3:
						if (d < 0x10000)
							return -1;
						break;
					case 2:
						if (d < 0x800)
							return -1;
						/* catch illegal codes */
						if ((d >= 0xd800 && d < 0xe000) ||
							d == 0xfffe || d == 0xffff)
							return -1;
						break;
					case 1:
						if (d < 0x80)
							return -1;
				}

				if (preparechar(mod, arr, temp, d) == TFALSE ||
					_array_ins(mod, arr, temp) < 0) {
					return -1;
				}

				j = 0;
			}

			if (expc)
				return -1;

			newlen = arr->tah_Length;
		}
	}
	return newlen;
}

/*****************************************************************************/
/* 
**	str = str_encodeutf8(mod, str)
**	create an UTF-8 encoded copy of a string
*/

EXPORT TINT str_encodeutf8(TMOD_US * mod, TUString idx)
{
	TUString newidx = -1;
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr) {
		newidx = array_alloc(mod, 0);
		if (newidx >= 0) {
			TAHEAD *newarr = mod->us_Array[newidx];
			TINT i;

			newarr->tah_Flags |= TDSTRF_8BIT_PRESENT;

			for (i = 0; i < arr->tah_Length; ++i) {
				_str_insert_utf8(mod, newarr, _str_get(arr, i));
			}

			if (!(newarr->tah_Flags & TDSTRF_VALID)) {
				array_free(mod, newidx);
				newidx = -1;
			}
		}
	}
	return newidx;
}

/*****************************************************************************/
/* 
**	result = str_ncmp(mod, s1, s2, pos1, pos2, maxlen)
**	compare arrays, case-sensitive, starting at a specified
**	position, with a maximum length. Either or both strings may be
**	invalid. an invalid string is "less than" a valid string.
*/

EXPORT TINT
str_ncmp(TMOD_US * mod, TUString s1, TUString s2, TINT pos1, TINT pos2,
	TINT maxlen)
{
	TAHEAD *a1 = _str_valid(mod, s1);
	TAHEAD *a2 = _str_valid(mod, s2);
	if (a1) {
		if (a2) {
			TWCHAR t1, t2, c1, c2;

			if (pos1 < 0)
				pos1 = a1->tah_Length + 1 + pos1;
			if (pos2 < 0)
				pos2 = a2->tah_Length + 1 + pos2;

			t1 = _str_get(a1, pos1);
			t2 = _str_get(a2, pos2);

			if (maxlen < 0) {
				do {
					if ((c1 = t1) >= 0)
						t1 = _str_get(a1, pos1++);
					if ((c2 = t2) >= 0)
						t2 = _str_get(a2, pos2++);
					if (c1 < 0 || c2 < 0)
						break;

				} while (c1 == c2);
			} else if (maxlen > 0) {
				do {
					if ((c1 = t1) >= 0)
						t1 = _str_get(a1, pos1++);
					if ((c2 = t2) >= 0)
						t2 = _str_get(a2, pos2++);
					if (c1 < 0 || c2 < 0)
						break;

				} while (maxlen-- && c1 == c2);
			} else
				return 0;

			return ((TINT) c1 - (TINT) c2);
		}
		return 1;
	}
	if (a2)
		return -1;
	return 0;
}

/*****************************************************************************/
/* 
**	newlength = str_crop(mod, idx, pos, len)
**	crop a string to the given range
*/

EXPORT TINT str_crop(TMOD_US * mod, TUString idx, TINT startpos, TINT len)
{
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr && len >= -1) {
		TINT endpos;
		if (startpos < 0) {
			startpos = arr->tah_Length + 1 + startpos;
			if (startpos < 0)
				startpos = 0;
		}
		endpos = startpos + len;
		if (len == -1)
			endpos = arr->tah_Length;

		if (endpos < arr->tah_Length) {
			setpos(arr, endpos);
			array_trunc(mod, idx);
		}

		if (startpos > 0) {
			TINT i;
			setpos(arr, 0);
			for (i = 0; i < startpos; ++i) {
				_array_rem(mod, arr, TNULL);
			}
		}

		return arr->tah_Length;
	}
	return -1;
}

/*****************************************************************************/
/* 
**	newlen/err = str_insertdstr(mod, str1, pos1, str2, pos2, maxlen)
**	insert a dynamic string
*/

EXPORT TINT
str_insertdstr(TMOD_US * mod, TUString idx1, TINT pos1, TUString idx2,
	TINT pos2, TINT maxlen)
{
	TINT newlen = -1;
	TAHEAD *arr1 = _str_valid(mod, idx1);
	TAHEAD *arr2 = _str_valid(mod, idx2);
	if (arr1 && arr2) {
		if (pos1 < 0)
			pos1 = arr1->tah_Length + 1 + pos1;

		if (pos2 < 0) {
			pos2 = arr2->tah_Length + 1 + pos2;
			if (pos2 < 0)
				pos2 = 0;
		}

		if (maxlen < 0)
			maxlen = arr2->tah_Length + 1 + maxlen - pos2;
		if (maxlen <= 0)
			return -1;

		if (setpos(arr1, pos1) == pos1) {
			TUINT8 temp[4];
			TWCHAR c;

			if (maxlen < 0) {
				while ((c = _str_get(arr2, pos2) >= 0)) {
					if (preparechar(mod, arr1, temp, c) == TFALSE ||
						_array_ins(mod, arr1, temp) < 0) {
						return -1;
					}
					pos2++;
				}
			} else {
				for (; maxlen && (c = _str_get(arr2, pos2)) >= 0; --maxlen) {
					if (preparechar(mod, arr1, temp, c) == TFALSE ||
						_array_ins(mod, arr1, temp) < 0) {
						return -1;
					}
					pos2++;
				}
			}

			newlen = arr1->tah_Length;
		}
	}
	return newlen;
}

/*****************************************************************************/
/* 
**	newlength = str_transform(mod, idx, startpos, len, mode)
**	modes:	TSTRF_UPPER:
**			TSTRF_UPPER_NOLOSS:
**			TSTRF_LOWER:
**			TSTRF_LOWER_NOLOSS:
**	return:	newlength - if valid
**			-1 - out of memory
**			-2 - loss of information
*/

EXPORT TINT
str_transform(TMOD_US * mod, TUString idx, TINT startpos, TINT len,
	TUINT mode)
{
	TAHEAD *arr = _str_valid(mod, idx);
	if (arr) {
		TINT strlen = arr->tah_Length;
		if (startpos < 0) {
			startpos = arr->tah_Length + 1 + startpos;
			if (startpos < 0)
				startpos = 0;
		}
		if (startpos < strlen) {

			if (len < 0)
				len = strlen + 1 + len - startpos;
			if (len <= 0)
				return -1;

			if (startpos + len <= strlen) {
				TUINT8 temp[4];
				TINT16 *conv;
				TWCHAR c;
				TINT i;

				switch (mode) {
					default:
						return -1;

					case TSTRF_UPPER:
					case TSTRF_UPPER | TSTRF_NOLOSS:
						for (i = 0; i < len; ++i) {
							c = _str_get(arr, startpos + i);
							if (c < CASECONVRANGE
								&& (conv = mod->us_SmallToCaps[c])) {
								c = conv[2];	/* set first new char */
								_array_set(arr, preparechar(mod, arr, temp,
										c));

								switch (conv[0]) {
									case 1:	/* irreversible small to cap */
										if (mode & TSTRF_NOLOSS)
											return -2;
									case 0:	/* reversible small to cap */
										continue;
								}

								/* irreversible small to cap, multichar */
								if (mode & TSTRF_NOLOSS)
									return -2;
								conv += 3;
								while ((c = *conv++)) {
									__array_seek(arr, &arr->tah_Cursor, 1);
									if (_array_ins(mod, arr,
											preparechar(mod, arr, temp,
												c)) < 0) {
										return -1;
									}
									startpos++;
								}
							}
						}
						break;

					case TSTRF_LOWER:
					case TSTRF_LOWER | TSTRF_NOLOSS:
						for (i = 0; i < len; ++i) {
							c = _str_get(arr, startpos + i);
							if (c < CASECONVRANGE
								&& (conv = mod->us_CapsToSmall[c])) {
								c = conv[1];	/* set first new char */
								_array_set(arr, preparechar(mod, arr, temp,
										c));

								switch (conv[0]) {
									case 2:	/* irreversible small to cap */
										if (mode & TSTRF_NOLOSS)
											return -2;
									case 0:	/* reversible small to cap */
										break;
								}
							}
						}
						break;
				}

				return arr->tah_Length;
			}
		}
	}
	return -1;
}

/*****************************************************************************/
/* 
**	newidx = str_dup(mod, idx, pos, len)
*/

EXPORT TUString
str_dup(TMOD_US * mod, TUString oldidx, TINT startpos, TINT len)
{
	TAHEAD *arr = _str_valid(mod, oldidx);
	if (arr) {
		if (startpos < 0) {
			startpos = arr->tah_Length + 1 + startpos;
			if (startpos < 0)
				startpos = 0;
		}
		if (len < 0) {
			len = arr->tah_Length + 1 + len;
		}
		if (len <= 0) return -1;
		if (startpos + len <= arr->tah_Length) {
			TUString newidx = array_alloc(mod, 0);
			if (newidx >= 0) {
				if (str_insertdstr(mod, newidx, 0, oldidx, startpos,
						len) >= 0) {
					return newidx;
				}
				array_free(mod, newidx);
			}
		}
	}
	return TINVALID_STRING;
}

/*****************************************************************************/
/*
**	Revision History
**	$Log: unistring_strings.c,v $
**	Revision 1.28  2005/09/13 02:42:42  tmueller
**	updated copyright reference
**	
**	Revision 1.27  2005/09/11 06:37:44  tmueller
**	reformatted; added extended interpretations of length and positional args
**	
**	Revision 1.26  2005/09/10 12:42:56  tmueller
**	-1 is now possible length for strinsert, -n is valid pos for strinsertstrn
**	
**	Revision 1.25  2004/08/01 11:31:20  tmueller
**	str_duplinear removed
**	
**	Revision 1.24  2004/07/25 00:08:15  tmueller
**	added strfind with Lua-style pattern matching
*/
