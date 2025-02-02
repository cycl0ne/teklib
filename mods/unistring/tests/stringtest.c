
#include <stdio.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/proto/exec.h>
#include <tek/proto/unistring.h>

#define TArrayPrint(mod,a)	(*(((TMODCALL TVOID(**)(TAPTR,TUString))(mod))[-3]))(mod,a)

/*****************************************************************************/

TAPTR TUStrBase;
TAPTR TExecBase;

static TVOID
printrange(TUString s, TINT start, TINT len)
{
	TUINT8 buf[1024];
	TINT i;

#if 1
	if (TUStrRenderArray(TUStrBase, s, buf, start, len) == 0)
	{
		buf[len] = 0;
	}
#else
	TUINT8 *p;
	p = TArrayMap(TUStrBase, s, start, len);
	if (p)
	{
		TExecCopyMem(TExecBase, p, buf, len);
		buf[len] = 0;
	}
#endif

	printf("s:%03d l:%03d ", start, len);

#if 0
	for (i = 0; i < len; ++i)
	{
		if (buf[i] < 0x100)
		{
			printf("%c", buf[i]);
		}
		else
		{
			printf("\\%03d", buf[i]);
		}
	}
#endif

	for (i = 0; i < len; ++i)
	{
		TWCHAR c = TUStrGetCharString(TUStrBase, s, i);
		if (c < 0x80)
		{
			printf("%c", c);
		}
		else
		{
			printf("\\%03d", c);
		}
	}

	printf("\n");
}

static TVOID
printstring(TUString s)
{
	printrange(s, 0, TUStrLengthString(TUStrBase, s));
}


static TVOID
testaddpart(TSTRPTR path, TSTRPTR part)
{
	TUString a, b;
	TINT res;
	a = TUStrAllocString(TUStrBase, path);
	b = TUStrAllocString(TUStrBase, part);
	
	res = TUStrAddPartString(TUStrBase, a, b);

	printf("--------------------------------------------------------------------\n");	
	printf("%s + %s result: %s\n", path, part, res ? "okay" : "error");
	if (res) printstring(a);
	TUStrFreeString(TUStrBase, a);
	TUStrFreeString(TUStrBase, b);
}


TTASKENTRY TVOID 
TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUStrBase = TExecOpenModule(TExecBase, "unistring", 0, TNULL);
	if (TUStrBase)
	{
		TUString s1, s2, s3, s4, s5;
		TINT i;
		TAPTR ptr;

		s1 = TUStrAllocString(TUStrBase, TNULL);

		for (i = 0; i < 26; ++i)
		{
			TUStrSetCharString(TUStrBase, s1, i, 'a' + i);
		}

		for (i = 0; i < 10; ++i)
		{
			TUStrInsCharString(TUStrBase, s1, 25, '0');
		}
		
		s5 = TUStrDupString(TUStrBase, s1,0,-1);

		TUStrInsertStrNString(TUStrBase, s1, 35, "hallo hähö", 10, TASIZE_8BIT);
		TUStrSetCharString(TUStrBase, s1, -1, 0);
		printstring(s1);
		
		s2 = TUStrEncodeUTF8String(TUStrBase, s1);
		printstring(s2);
		
		s3 = TUStrAllocString(TUStrBase, TNULL);
		ptr = TUStrMapString(TUStrBase, s2, 0, TUStrLengthString(TUStrBase, s2), TASIZE_8BIT);
		if (TUStrInsertUTF8String(TUStrBase, s3, 0, ptr) < 0) printf("failed\n");
		printstring(s3);


		s4 = TUStrAllocString(TUStrBase, TNULL);
		TUStrInsertUTF8String(TUStrBase, s4, 0, "AnfÃ¼hrungszeichen");
		TUStrInsertStrNString(TUStrBase, s4, 17, "Anführungszeichen", 20, TASIZE_8BIT);
		printstring(s4);

		/* malformed UTF-8 tests: */
		if (TUStrInsertUTF8String(TUStrBase, s4, 0, "€¿€¿€¿") < 0) printf("UTF-8 test1 passed\n");
		if (TUStrInsertUTF8String(TUStrBase, s4, 0, "À Á Â Ã Ä Å Æ Ç È É Ê Ë Ì Í Î Ï ") < 0) printf("UTF-8 test2 passed\n");
		if (TUStrInsertUTF8String(TUStrBase, s4, 0, "ø€€€¯") < 0) printf("UTF-8 test3 passed\n");
		if (TUStrInsertUTF8String(TUStrBase, s4, 0, "üƒ¿¿¿¿") < 0) printf("UTF-8 test4 passed\n");
		if (TUStrInsertUTF8String(TUStrBase, s4, 0, "à€") < 0) printf("UTF-8 test5 passed\n");
		if (TUStrInsertUTF8String(TUStrBase, s4, 0, "þþÿÿ") < 0) printf("UTF-8 test6 passed\n");

		if (TUStrInsertUTF8String(TUStrBase, s4, 0, "ï¿¿") < 0) printf("UTF-8 test7 passed\n");
		if (TUStrInsertUTF8String(TUStrBase, s4, 0, "í­¿í°€") < 0) printf("UTF-8 test8 passed\n");


		TUStrFreeString(TUStrBase, s1);
		TUStrFreeString(TUStrBase, s2);
		TUStrFreeString(TUStrBase, s3);
		TUStrFreeString(TUStrBase, s4);
		
		printf("-----------------------------------------------------------------\n");

		s1 = TUStrAllocString(TUStrBase, TNULL);
		TUStrInsertStrNString(TUStrBase, s1, 0, "tim", 3, TASIZE_8BIT);
		s2 = TUStrAllocString(TUStrBase, TNULL);
		TUStrInsertStrNString(TUStrBase, s2, 0, "simm", 4, TASIZE_8BIT);


		printstring(s1);
		printstring(s2);
		i = TUStrCmpNString(TUStrBase, s1, s2, 1, 1, 1);
		if (i == 0) printf("s1 = s2\n");
		else if (i > 0) printf("s1 > s2\n");
		else printf("s1 < s2\n");
		
		

		TUStrFreeString(TUStrBase, s1);
		TUStrFreeString(TUStrBase, s2);
		
		printf("-----------------------------------------------------------------\n");

		printstring(s5);
		printf("crop: %d\n", TUStrCropString(TUStrBase, s5, 1, 15));
		TUStrSetCharString(TUStrBase, s5, -1, 'h');
		printstring(s5);
		TUStrFreeString(TUStrBase, s5);
		

		printf("-----------------------------------------------------------------\n");
		{
			TUString s = TUStrAllocString(TUStrBase, "Wäßrige Lösungen");
			printstring(s);
			TUStrTransformString(TUStrBase, s, 0, -1, TSTRF_UPPER);
			printstring(s);
			TUStrFreeString(TUStrBase, s);			
		}
	
		printf("-----------------------------------------------------------------\n");
		{
			TINT res;
			TUString pat = TUStrAllocString(TUStrBase, 
				"~(skfh01_(wp|bi|shi|khp)[0-9]_(k|a)(xx|ug|eg|01|02)[0-9][a-z]_[a-z][a-z]#?([a-z0-9]|%).(jpg|png))");

			TUString str1 = TUStrAllocString(TUStrBase, "skfh01_bi5_kug1u_hmfs5030.foo");
			TUString str2 = TUStrAllocString(TUStrBase, "skfh01_bi5_kug1u_hmfs5030.jpg");

			printstring(pat);

			res = TUStrTokenizeString(TUStrBase, pat, 0);
			printf("parsepattern: %d - len: %d\n", res, TUStrLengthString(TUStrBase, pat));
			/*printstring(pat);*/

			printstring(str1);
			res = TUStrMatchString(TUStrBase, pat, str1);
			printf("match: %d\n", res);

			printstring(str2);
			res = TUStrMatchString(TUStrBase, pat, str2);
			printf("match: %d\n", res);

			TUStrFreeString(TUStrBase, pat);
			TUStrFreeString(TUStrBase, str1);
			TUStrFreeString(TUStrBase, str2);
		}

	#if 0
	#ifdef TDEBUG
	#ifdef TSYS_POSIX
		printf("-----------------------------------------------------------------\n");
		{
			TUString s = TUStrAllocString(TUStrBase, TNULL);
			TINT i;
			
			for (i = 0; i < 16; ++i)
			{
				TUStrSetCharString(TUStrBase, s, i, 'a'+i);
				TArrayPrint(TUStrBase, s);
			}

			for (i = 0; i < 20; ++i)
			{
				TArraySeek(TUStrBase, s, 1, 0);
				TArraySeek(TUStrBase, s, 1, i);
				TArrayPrint(TUStrBase, s);
			}

			for (i = 0; i < 26; ++i)
			{
				TUStrSetCharString(TUStrBase, s, i, 'a'+i);
				TArrayPrint(TUStrBase, s);
			}

			for (i = 0; i < 10; ++i)
			{
				TStringRemChar(TUStrBase, s, 10);
				TArrayPrint(TUStrBase, s);
			}

			for (i = 0; i < 20; ++i)
			{
				TArraySeek(TUStrBase, s, 1, 0);
				TArraySeek(TUStrBase, s, 1, i);
				TArrayPrint(TUStrBase, s);
			}

			for (i = 19; i >= 0; --i)
			{
				TArraySeek(TUStrBase, s, 0, -1);
				TArrayPrint(TUStrBase, s);
			}
			
			for (i = 0; i < 10; ++i)
			{
				TUStrInsCharString(TUStrBase, s, 10+i, '0'+i);
				TArrayPrint(TUStrBase, s);
			}

			TArraySeek(TUStrBase, s, -1, 0);
			TArrayPrint(TUStrBase, s);
			for (i = 29; i >= 0; --i)
			{
				TArraySeek(TUStrBase, s, 0, -1);
				TArrayPrint(TUStrBase, s);
			}

			TUStrFreeString(TUStrBase, s);
		}
	#endif
	#endif
	#endif


		testaddpart("foo/bar/", "/bla/fasel");		// -> /foo/bla/fasel (ok)
		testaddpart("foo/bar/", "//bla");			// -> bla (ok)
		testaddpart("foo/", "//");					// -> / (ok)
		testaddpart("foo/", "/");					// -> "" (ok)
		testaddpart("foo/bar", "//bla//fasel");		// -> fasel (ok)
		testaddpart("foo/bar", "/bla///fasel");		// -> fasel (ok)
		testaddpart("foo/bar/", "bla///fasel");		// -> foo/fasel (ok)

	#if 0
		printf("-----------------------------------------------------------------\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, "foo/bar/");
			b = TUStrAllocString(TUStrBase, "/bla/fasel");	// -> /foo/bla/fasel ok
			TUStrAddPartString(TUStrBase, a, b);
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}
		
		printf("-----------------------------------------------------------------\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, "foo/bar/");
			b = TUStrAllocString(TUStrBase, "//bla");		// -> bla ok
			TUStrAddPartString(TUStrBase, a, b);
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}
			
		printf("-----------------------------------------------------------------\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, "foo/");
			b = TUStrAllocString(TUStrBase, "//");			// -> / ok
			TUStrAddPartString(TUStrBase, a, b);
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}

		printf("-----------------------------------------------------------------\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, "foo/");
			b = TUStrAllocString(TUStrBase, "/");			// -> () ok
			TUStrAddPartString(TUStrBase, a, b);
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}
		
		printf("-----------------------------------------------------------------\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, "foo/bar");
			b = TUStrAllocString(TUStrBase, "//bla//fasel");	// -> fasel
			TUStrAddPartString(TUStrBase, a, b);
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}

		printf("-----------------------------------------------------------------\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, "abc");
			b = TUStrAllocString(TUStrBase, "bar:");				// -> bar:
			printf("result: %s\n", TUStrAddPartString(TUStrBase, a, b) ? "okay" : "error");
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}
		
		printf("-----------------------------------------------------------------\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, "foo:");
			b = TUStrAllocString(TUStrBase, "/bla///fasel");	// -> foo://fasel
			printf("result: %s\n", TUStrAddPartString(TUStrBase, a, b) ? "okay" : "error");
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}
		


		printf("-----------------------------------------------------------------\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, "foo");
			b = TUStrAllocString(TUStrBase, "/");			// -> () ok
			printf("result: %s\n", TUStrAddPartString(TUStrBase, a, b) ? "okay" : "error");
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}



		printf("----------------------------------------------------------------?\n");
		{
			TUString a, b;
			
			a = TUStrAllocString(TUStrBase, "x:foo///bla");
			b = TUStrAllocString(TUStrBase, "");				// -> x:/bla
			printf("result: %s\n", TUStrAddPartString(TUStrBase, a, b) ? "okay" : "error");
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}

		printf("----------------------------------------------------------------?\n");
		{
			TUString a, b;
			
			a = TUStrAllocString(TUStrBase, "x:foo///bla//");
			b = TUStrAllocString(TUStrBase, "");				// -> x:/
			printf("result: %s\n", TUStrAddPartString(TUStrBase, a, b) ? "okay" : "error");
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}


		printf("----------------------------------------------------------------!\n");
		{
			TUString a, b;

			a = TUStrAllocString(TUStrBase, ":");
			b = TUStrAllocString(TUStrBase, "/");				// -> error
			printf("result: %s\n", TUStrAddPartString(TUStrBase, a, b) ? "okay" : "error");
			printstring(a);
			TUStrFreeString(TUStrBase, a);
			TUStrFreeString(TUStrBase, b);
		}
	#endif

		printf("--------------------------------------------------------------------\n");
		{
			TUINT8 str[] = "abcABCæþßÐÇØ123.;+ \t\r\b";
			TSTRPTR s = str;
			TWCHAR c;
			printf("        alph alnm ctrl graf lowr uppr prnt pnct spac tlow tupp\n");
			while ((c = *s++))
			{
				printf("%03d %c :  %s    %s    %s    %s    %s    %s    %s    %s    %s    %c    %c\n",
					c,
					(TUStrIsCntrl(TUStrBase, c) || TUStrIsSpace(TUStrBase, c)) ? 32 : c,
					TUStrIsAlpha(TUStrBase, c) ? "X" : " ",
					TUStrIsAlnum(TUStrBase, c) ? "X" : " ",
					TUStrIsCntrl(TUStrBase, c) ? "X" : " ",
					TUStrIsGraph(TUStrBase, c) ? "X" : " ",
					TUStrIsLower(TUStrBase, c) ? "X" : " ",
					TUStrIsUpper(TUStrBase, c) ? "X" : " ",
					TUStrIsPrint(TUStrBase, c) ? "X" : " ",
					TUStrIsPunct(TUStrBase, c) ? "X" : " ",
					TUStrIsSpace(TUStrBase, c) ? "X" : " ",
					TUStrIsGraph(TUStrBase, c) ? TUStrToLower(TUStrBase, c) : 32,
					TUStrIsGraph(TUStrBase, c) ? TUStrToUpper(TUStrBase, c) : 32);
			}
		}		

		printf("--------------------------------------------------------------------\n");
		{
			TUString s = TUStrAllocString(TUStrBase,
				"Content-Disposition: form-data; name=\"hello\" ");
			TUString p = TUStrAllocString(TUStrBase,
				"(%a+)%-[Dd]isposition:[ ]*[Ff]orm%-[Dd]ata;[ ]*.*[Nn]ame=\"?(%a[_%w]+)\"?");
			TUString c1 = TINVALID_STRING;
			TUString c2 = TINVALID_STRING;
			TINT spos, epos;
			TTAGITEM ftags[5];

			ftags[0].tti_Tag = TUStrFind_MatchBegin;
			ftags[0].tti_Value = (TTAG) &spos; 
			ftags[1].tti_Tag = TUStrFind_MatchEnd;
			ftags[1].tti_Value = (TTAG) &epos; 
			ftags[2].tti_Tag = TUStrFind_CaptureString;
			ftags[2].tti_Value = (TTAG) &c1; 
			ftags[3].tti_Tag = TUStrFind_CaptureString;
			ftags[3].tti_Value = (TTAG) &c2; 
			ftags[4].tti_Tag = TTAG_DONE;
			
			if (TUStrFindPatString(TUStrBase, s, p, 0, -1, ftags) >= 0)
			{
				printf("match found from %d to %d\n", spos, epos);
				printstring(c1);
				printstring(c2);
			}
			
			TUStrFreeString(TUStrBase, s);
			TUStrFreeString(TUStrBase, p);
			TUStrFreeString(TUStrBase, c1);
			TUStrFreeString(TUStrBase, c2);
		}
				
		TExecCloseModule(TExecBase, TUStrBase);
		printf("all done\n");
	}
	else
	{
		printf("*** Module open failed\n");
	}
}
