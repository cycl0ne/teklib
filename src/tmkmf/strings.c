
#include "global.h"
#include <tek/inline/exec.h>
#include <tek/inline/util.h>

TBOOL dInitStrings(struct dStrings *dstr)
{
	dstr->dStrings = TNULL;
	dstr->dStrLens = TNULL;
	dstr->mmu = MMU;
	dstr->dNumStrings = TNULL;
	return TTRUE;
}

void dExitStrings(struct dStrings *dstr)
{
	TINT i;
	for (i = 0; i < dstr->dNumStrings; ++i)
		TFree(dstr->dStrings[i]);
	TFree(dstr->dStrings);
	TFree(dstr->dStrLens);
}

TINT dAllocString(struct dStrings *dstr, TSTRPTR istr)
{
	TINT id, ilen;
	if (!dstr->dStrings)
	{
		dstr->dStrings = TAlloc(dstr->mmu, sizeof(TSTRPTR *));
		dstr->dStrLens = TAlloc(dstr->mmu, sizeof(TINT *));
		if (!dstr->dStrings || !dstr->dStrLens)
		{
			TFree(dstr->dStrings);
			TFree(dstr->dStrLens);
			dstr->dStrings = TNULL;
			dstr->dStrLens = TNULL;
			return -1;
		}
		dstr->dNumStrings = 1;
		id = 0;
	}
	else
	{
		TSTRPTR *ns;
		TINT *nl;
		for (id = 0; id < dstr->dNumStrings; ++id)
			if (dstr->dStrLens[id] < 0)
				goto found;
		ns = TRealloc(dstr->dStrings, sizeof(TSTRPTR *) * (dstr->dNumStrings + 1));
		nl = TRealloc(dstr->dStrLens, sizeof(TINT *) * (dstr->dNumStrings + 1));
		if (!ns || !nl)
		{
			TFree(nl);
			TFree(ns);
			return -1;
		}
		dstr->dStrings = ns;
		dstr->dStrLens = nl;
		id = dstr->dNumStrings++;
	}
found:
	ilen = TStrLen(istr);
	dstr->dStrLens[id] = ilen;
	ilen = ilen ? ilen + 1 : 64;
	dstr->dStrings[id] = TAlloc(dstr->mmu, ilen);
	if (!dstr->dStrings[id]) return -1;
	if (istr) TStrCpy(dstr->dStrings[id], istr);
	return id;
}

void dFreeString(struct dStrings *dstr, TINT id)
{
	if (id >= 0)
	{
		TFree(dstr->dStrings[id]);
		dstr->dStrings[id] = TNULL;
		dstr->dStrLens[id] = -1;
	}
}

TINT dLengthString(struct dStrings *dstr, TINT s)
{
	if (s >= 0 && dstr->dStrings[s])
		return dstr->dStrLens[s];
	return -1;
}

TINT dGetCharString(struct dStrings *dstr, TINT s, TINT pos)
{
	if (s < 0) return -1;
	if (dstr->dStrLens[s] < 0) return -1;
	if (dstr->dStrings[s] == TNULL) return -1;
	if (pos < 0) pos = dstr->dStrLens[s] + 1 + pos;
	if (pos < 0) return -1;
	if (pos >= dstr->dStrLens[s]) return -1;
	return dstr->dStrings[s][pos];
}

TINT dSetCharString(struct dStrings *dstr, TINT s, TINT pos, TINT c)
{
	TINT len;
	if (s < 0) return -1;
	if (dstr->dStrings[s] == TNULL) return -1;
	len = dstr->dStrLens[s];
	if (len < 0) return -1;
	if (pos < 0) pos = len + 1 + pos;
	if (pos < 0) return -1;
	if (pos == len)
	{
		if (pos == TGetSize(dstr->dStrings[s]))
		{
			TSTRPTR ns = TRealloc(dstr->dStrings[s], len + 64);
			if (ns == TNULL)
			{
				TFree(dstr->dStrings[s]);
				dstr->dStrings[s] = TNULL;
				return -1;
			}
			dstr->dStrings[s] = ns;
		}
		dstr->dStrLens[s] = ++len;
	}
	dstr->dStrings[s][pos] = c;
	return len;
}

TAPTR dMapString(struct dStrings *dstr, TINT s, TINT spos, TINT slen)
{
	TINT len;
	if (s < 0) return TNULL;
	len = dstr->dStrLens[s];
	if (len < 0) return TNULL;
	if (spos < 0) spos = len + 1 + spos;
	if (spos < 0) return TNULL;
	return dstr->dStrings[s] + spos;
}

TINT dCmpNString(struct dStrings *dstr, TINT id1, TINT id2, TINT p1, TINT p2, TINT len)
{
	if (id1 >= 0)
	{
		if (id2 >= 0)
		{
			TINT c1, c2;
			do
			{
				c1 = dGetCharString(dstr, id1, p1);
				if (c1 >= 0) p1++;
				c2 = dGetCharString(dstr, id2, p2);
				if (c2 >= 0) p2++;
			} while (c1 >= 0 && c2 >= 0 && c1 == c2 && len--);
			return ((TINT) c1 - (TINT) c2);
		}
		return 1;
	}
	if (id2 >= 0) return -1;
	return 0;
}

TINT dFindString(struct dStrings *dstr, TINT s, TINT p, TINT pos, TINT len)
{
	TINT epos, dlen;
	TINT c, d;
	TINT x = 0;
	TINT fp;

	dlen = dLengthString(dstr, p);
	if (dlen < 0) return -1;
	if (pos < 0) pos = dstr->dStrLens[s] + 1 + pos;
	if (pos < 0) return -1;
	if (len < 0) len = dstr->dStrLens[s] + 1 + len;
	epos = pos + len;

	d = dGetCharString(dstr, p, 0);
	fp = -1;
	while (pos < epos)
	{
		c = dGetCharString(dstr, s, pos);
		if (c < 0) break;

		if (c != d)
		{
			x = 0;
			fp = pos;
			d = dGetCharString(dstr, p, x);
		}

		if (c == d)
		{
			if (++x == dlen)
				return fp + 1;
			d = dGetCharString(dstr, p, x);
		}
		pos++;
	}

	return -1;
}

TINT dInsCharString(struct dStrings *dstr, TINT s, TINT pos, TINT c)
{
	TINT len;
	if (s < 0) return -1;
	len = dstr->dStrLens[s];
	if (len < 0) return -1;
	if (pos < 0) pos = len + 1 + pos;
	if (pos < 0) return -1;

	dSetCharString(dstr, s, -1, 0);
	if (pos < len)
	{
		TINT i;
		for (i = len; i > pos; --i)
			dSetCharString(dstr, s, i, dGetCharString(dstr, s, i - 1));
	}
	dSetCharString(dstr, s, pos, c);
	return dLengthString(dstr, s);
}

TINT dInsertStrNString(struct dStrings *dstr, TINT s, TINT pos, TSTRPTR data, TINT len)
{
	TINT i;
	TINT l2 = TStrLen(data);

	if (len == -1)
		len = l2;
	else
		len = TMIN(len, l2);

 	if (pos < 0) pos = dstr->dStrLens[s] + 1 + pos;
 	if (pos < 0) return -1;

	for (i = pos; i < pos + len; ++i)
		dInsCharString(dstr, s, i, *data++);

	return dLengthString(dstr, s);
}

TINT dInsertString(struct dStrings *dstr, TINT d, TINT dpos, TINT s, TINT spos, TINT len)
{
	TINT i;

 	if (dpos < 0) dpos = dstr->dStrLens[d] + 1 + dpos;
 	if (dpos < 0) return -1;

 	if (spos < 0) spos = dstr->dStrLens[s] + 1 + spos;
 	if (spos < 0) return -1;

	if (len == -1) len = dstr->dStrLens[s] - spos;

 	for (i = 0; i < len; ++i)
		dInsCharString(dstr, d, i + dpos, dGetCharString(dstr, s, i + spos));

	return dLengthString(dstr, d);
}

TINT dDupString(struct dStrings *dstr, TINT s, TINT pos, TINT len)
{
	TINT n = dAllocString(dstr, TNULL);
	if (n >= 0)
	{
		TINT i;
		len = TMIN(len, dstr->dStrLens[s]);
		for (i = 0; i < len; ++i)
			dInsCharString(dstr, n, i, dGetCharString(dstr, s, pos + i));
		return n;
	}
	return -1;
}
