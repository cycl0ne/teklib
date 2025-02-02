
/*
**	$Id: playdt.c,v 1.8 2005/10/07 14:17:05 tmueller Exp $
**	Datatype audio player
*/

#include <stdio.h>
#include <tek/teklib.h>
#include <tek/inline/exec.h>
#include <tek/inline/util.h>
#include <tek/inline/io.h>
#include <tek/proto/datatypehandler.h>
#include <tek/mod/audio.h>

#define ARG_TEMPLATE	"-f=FROM/A/M,-l=LOOP/S,-i=INFO/S,-nse=NOSONGEND/S,-h=HELP/S"
enum { ARG_FROM, ARG_LOOP, ARG_INFO, ARG_NOSONGEND, ARG_HELP, ARG_NUM };

TAPTR TExecBase;
TAPTR TUtilBase;
TAPTR TDthBase;
TAPTR TIOBase;
TTAG args[ARG_NUM];

static TSTRPTR 
maketekname(TSTRPTR filename)
{
	TSTRPTR tekname = TNULL;
	TINT len = TMakeName(filename, TNULL, 0, TPPF_HOST2TEK, TNULL);
	if (len >= 0)
	{
		tekname = TAlloc(TNULL, len + 1);
		if (tekname)
		{
			TMakeName(filename, tekname, len + 1, TPPF_HOST2TEK, TNULL);
		}		
	}
	return tekname;	
}

static TVOID 
play(TAPTR dtobj, TTAGITEM *dttags, TUINT format, TINT rate)
{
	struct TAudIORequest *ioreq1, *ioreq2, *ioreqt;
	TTAGITEM audiotags[3];
	TUINT prefformats[2];
	TUINT prefrates[2];
	
	prefformats[0] = format;
	prefformats[1] = 0xffffffff;

	prefrates[0] = rate;
	prefrates[1] = 0xffffffff;
	
	audiotags[0].tti_Tag = TADIOT_PrefFormats;
	audiotags[0].tti_Value = (TTAG) prefformats;
	audiotags[1].tti_Tag = TADIOT_PrefRates;
	audiotags[1].tti_Value = (TTAG) prefrates;
	audiotags[2].tti_Tag = TTAG_DONE;

	ioreq1 = TOpenModule("audio", 0, audiotags);
	if (ioreq1)
	{
		TINT ioreqsize = TGetSize(ioreq1);
		ioreq2 = TAllocMsg(ioreqsize);
		if (ioreq2)
		{
			TAPTR replyport = TCreatePort(TNULL);
			if (replyport)
			{
				TINT numsmp = (TINT) TGetTag(ioreq1->audio_Tags, TADIOT_FragmentSize, (TTAG) 1024);
				TINT numchn = TADIO_GETNUMCHANNELS(format);
				TINT numbpc = TADIO_GETBYTESPERCHAN(format);
				TINT numbps = numchn * numbpc;

				ioreq1->audio_Std.io_Length = numbps * numsmp;
				ioreq1->audio_Std.io_Req.io_Command = TIOCMD_WRITE;
				ioreq1->audio_Std.io_Req.io_ReplyPort = replyport;
				ioreq1->audio_Format = format;
				ioreq1->audio_Rate = rate;

				TCopyMem(ioreq1, ioreq2, ioreqsize);

				ioreq1->audio_Std.io_Data = TAlloc(TNULL, numbps * numsmp);
				ioreq2->audio_Std.io_Data = TAlloc(TNULL, numbps * numsmp);
				
				if (ioreq1->audio_Std.io_Data && ioreq2->audio_Std.io_Data)
				{
					TTAGITEM dtdotags[3], restarttags[2];
					TBOOL abort = TFALSE;

					dtdotags[0].tti_Tag = TDOTAG_GETDATA;
					dtdotags[1].tti_Tag = TDOTAG_NUMSAMPLES;
					dtdotags[1].tti_Value = (TTAG) numsmp;
					dtdotags[2].tti_Tag = TTAG_DONE;
					restarttags[0].tti_Tag = TDOTAG_RESTART;
					restarttags[0].tti_Value = TTRUE;
					restarttags[1].tti_Tag = TTAG_DONE;
				
					ioreq1->audio_Link = TNULL;
					TPutIO((struct TIORequest *) ioreq1);
									
					ioreq2->audio_Link = ioreq1;
					TPutIO((struct TIORequest *) ioreq2);

					while (!abort)
					{
						TUINT8 *adr = ioreq1->audio_Std.io_Data;
						TINT smpleft = numsmp;

						TWaitIO((struct TIORequest *) ioreq1);
						
						dtdotags[0].tti_Value = (TTAG) adr;

						while (smpleft > 0)
						{
							smpleft -= TDthDoMethod(TDthBase, dtobj, dtdotags);							
							if (TGetTag(dttags, TDTAG_END_OF_STREAM, TFALSE))
							{
								if (!args[ARG_NOSONGEND] || 
									!TDthDoMethod(TDthBase, dtobj, restarttags))
								{
									abort = TTRUE;
									break;
								}
							}
						}
							
						ioreq1->audio_Link = ioreq2;
						TPutIO((struct TIORequest *) ioreq1);
						
						ioreqt = ioreq1; ioreq1 = ioreq2; ioreq2 = ioreqt;
					}

					TAbortIO((struct TIORequest *) ioreq1);
					TAbortIO((struct TIORequest *) ioreq2);
					TWaitIO((struct TIORequest *) ioreq1);
					TWaitIO((struct TIORequest *) ioreq2);
				}
				
				TFree(ioreq2->audio_Std.io_Data);
				TFree(ioreq1->audio_Std.io_Data);

				TDestroy(replyport);
			}
			TFree(ioreq2);
		}

		TCloseModule(ioreq1->audio_Std.io_Req.io_Device);
		TFree(ioreq1);
	}
	else printf("*** could not open audio device\n");
}

TTASKENTRY TVOID 
TEKMain(TAPTR task)
{
	TExecBase = TGetExecBase(task);
	TUtilBase = TOpenModule("util", 0, TNULL);
	TIOBase = TOpenModule("io", 0, TNULL);
	TDthBase = TOpenModule("datatypehandler", 0, TNULL);
	if (TUtilBase && TIOBase && TDthBase)
	{
		TSTRPTR *argv = TGetArgV();
		TAPTR arghandle;
		
		args[ARG_FROM] = TNULL;
		args[ARG_INFO] = (TTAG) TFALSE;
		args[ARG_LOOP] = (TTAG) TFALSE;
		args[ARG_NOSONGEND] = (TTAG) TFALSE;
		args[ARG_HELP] = (TTAG) TFALSE;
		
		arghandle = TParseArgV(ARG_TEMPLATE, argv + 1, args);
		if (!arghandle || args[ARG_HELP])
		{
			printf("TEKlib datatype audio player\n");
			printf("-f=FROM/A/M      : Audio file(s)\n");
			printf("-l=LOOP/S        : Loop playlist\n");
			printf("-i=INFO/S        : Show format info\n");
			printf("-nse=NOSONGEND/S : Disable song end detection\n");
			printf("-h=HELP/S        : This help\n");
		}
		else
		{
			do
			{
				TSTRPTR *fnames = (TSTRPTR *) args[ARG_FROM];
				TSTRPTR filename;
				while ((filename = *fnames++))
				{
					printf("%s\n", filename);	
					filename = maketekname(filename);
					if (filename)
					{
						TAPTR fh = TOpenFile(filename, TFMODE_READONLY, TNULL);
						if (fh)
						{
							TTAGITEM opentags[2];
							TAPTR dtobj;
							
							opentags[0].tti_Tag = TDOPENTAG_FILEHANDLE;
							opentags[0].tti_Value = (TTAG)fh;
							opentags[1].tti_Tag = TTAG_DONE;
							
							dtobj = TDthOpen(TDthBase, opentags);					
							if (dtobj)
							{
								TTAGITEM *dttags = TDthGetAttrs(TDthBase, dtobj);
								if ((TINT) TGetTag(dttags, TDTAG_CLASS, (TTAG) DTCLASS_NONE) == DTCLASS_SOUND)
								{
									TUINT format, rate, chans;
									TSTRPTR fmtname;
									
									format = (TUINT) TGetTag(dttags, TDTAG_SOUND_FORMAT, (TTAG) TADIOFMT_INVALID);
									rate = (TUINT) TGetTag(dttags, TDTAG_SOUND_RATE, (TTAG) 0);
									chans = (TUINT) TGetTag(dttags, TDTAG_SOUND_CHANNELS, (TTAG) 0);
									fmtname = (TSTRPTR) TGetTag(dttags, TDTAG_FULLNAME, (TTAG) 0);
							
									if (args[ARG_INFO])
									{
										printf("Media format: %s\n", fmtname);
										printf("Audio format: %08x\n", format);
										printf("Channels:     %d\n", chans);
										printf("Rate:         %d\n", rate);
									}

									play(dtobj, dttags, format, rate);
									
								} else printf("*** not a soundclass datatype\n");
		
								TDestroy(dtobj);
		
							} else printf("*** fileformat not recognized\n");
		
							TCloseFile(fh);
		
						} else printf("*** could not open %s\n", (TSTRPTR) args[ARG_FROM]);
				
						TFree(filename);
					}
					else printf("*** invalid name %s (or out of memory)\n", (TSTRPTR) args[ARG_FROM]);
				}
			} while (args[ARG_LOOP]);
		}
		TDestroy(arghandle);
	}
	else printf("*** could not open required modules\n");

	TCloseModule(TDthBase);
	TCloseModule(TIOBase);
	TCloseModule(TUtilBase);
}

