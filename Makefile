
#
#	TEKlib world makefile
#
#	1. build standalone makefile generator
#	2. generate makefiles
#	3. build sourcetree
#
###########################################################################

help:
	@echo "============================================================================="
	@echo "==  PLATFORM TARGETS"
	@echo "==     amiga[_release|_install|_debug|_clean] ....... AmigaOS, SAS/C"
	@echo "==     fbsd[_release|_install|_debug|_clean] ........ FreeBSD, GCC"
	@echo "==     linux[_release|_install|_debug|_clean] ....... GNU/Linux, GCC"
	@echo "==     morphos_gcc[_release|_install|_debug|_clean] . MorphOS, GCC"
	@echo "==     ps2_gcc[_release|_install|_debug|_clean] ..... Playstation2, GCC cross"
	@echo "==     win32[_release|_install|_debug|_clean] ....... Windows 2K/NT/XP, MSVC6"
	@echo "==     win32_gcc[_release|_install|_debug|_clean] ... Windows 2K/NT/XP, MingW"
	@echo "==  SPECIAL TARGETS"
	@echo "==     clean ................ Note: Unix-like shell environment expected"
	@echo "==     distclean ............ Note: Unix-like shell environment expected"
	@echo "==  NOTES"
	@echo "==     After the generation of makefiles, to locally build a (sub-)project,"
	@echo "==     you can invoke from here (as well as from any project directory):"
	@echo "==     make -f build/tmk_platform_compiler[_release|...]"
	@echo "============================================================================="

all install: help

###########################################################################

TMKMF = build/bin/tmkmf
LIBDIR = build/lib

COMMONOBJS = $(LIBDIR)/init.o $(LIBDIR)/teklib.o $(LIBDIR)/hal_mod.o \
	$(LIBDIR)/exec_mod.o $(LIBDIR)/time_mod.o $(LIBDIR)/hash_mod.o $(LIBDIR)/util_mod.o \
	$(LIBDIR)/unistring_mod.o $(LIBDIR)/io_master.o $(LIBDIR)/hal.o $(LIBDIR)/io_default.o $(LIBDIR)/host.o

###########################################################################
#	Posix/GNU

GCP = $(CC) -I. -DTSYS_POSIX
GAR = ar r
GRANLIB = ranlib
GRM = rm

$(LIBDIR)/libposix.a:
	-mkdir -p build/lib build/bin
	$(GCP) boot/init.c -c -o $(LIBDIR)/init.o
	$(GCP) boot/teklib.c -c -o $(LIBDIR)/teklib.o
	$(GCP) -Imods/hal mods/hal/hal_mod.c -c -o $(LIBDIR)/hal_mod.o
	$(GCP) -Imods/exec mods/exec/exec_all.c -c -o $(LIBDIR)/exec_mod.o
	$(GCP) -Imods/time mods/time/time_mod.c -c -o $(LIBDIR)/time_mod.o
	$(GCP) -Imods/hash mods/hash/hash_mod.c -c -o $(LIBDIR)/hash_mod.o
	$(GCP) -Imods/util mods/util/util_all.c -c -o $(LIBDIR)/util_mod.o
	$(GCP) -Imods/unistring mods/unistring/unistring_all.c -c -o $(LIBDIR)/unistring_mod.o
	$(GCP) -Imods/io/master mods/io/master/io_all.c -c -o $(LIBDIR)/io_master.o
	$(GCP) -Imods/hal mods/hal/posix/hal.c -c -o $(LIBDIR)/hal.o
	$(GCP) -Imods/io/default/posix mods/io/default/posix/iohnd_default.c -c -o $(LIBDIR)/io_default.o
	$(GCP) -Iboot boot/unix/host.c -c -o $(LIBDIR)/host.o
	$(GAR) $@ $(COMMONOBJS)
	$(GRANLIB) $@
	$(GRM) $(COMMONOBJS)

$(LIBDIR)/tmkmf_posix.o:
	$(GCP) -Iapps/tmkmf apps/tmkmf/main.c -c -o $@

#	linux

$(TMKMF)_linux: $(LIBDIR)/libposix.a $(LIBDIR)/tmkmf_posix.o
	$(GCP) $(LIBDIR)/tmkmf_posix.o -L$(LIBDIR) -lposix -lpthread -ldl -lm -o $@

build/tmk_linux_gcc: $(TMKMF)_linux
	$(TMKMF)_linux tmkmakefile RECURSE CONTEXT linux_gcc
	@echo "================ MAKEFILES HAVE BEEN GENERATED ===================="

build/tmk_linux_gcc_release: $(TMKMF)_linux
	$(TMKMF)_linux tmkmakefile RECURSE CONTEXT linux_gcc_release
	@echo "========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ==========="

build/tmk_linux_gcc_debug: $(TMKMF)_linux
	$(TMKMF)_linux tmkmakefile RECURSE CONTEXT linux_gcc_debug
	@echo "========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============"

linux: build/tmk_linux_gcc
	@$(MAKE) -s -f build/tmk_linux_gcc all

linux_libs: build/tmk_linux_gcc
	@$(MAKE) -s -f build/tmk_linux_gcc libs

linux_release: build/tmk_linux_gcc_release
	@$(MAKE) -s -f build/tmk_linux_gcc_release all

linux_debug: build/tmk_linux_gcc_debug
	@$(MAKE) -s -f build/tmk_linux_gcc_debug all

linux_install: build/tmk_linux_gcc_release
	@$(MAKE) -s -f build/tmk_linux_gcc_release all install

linux_clean: build/tmk_linux_gcc
	@$(MAKE) -s -f build/tmk_linux_gcc clean

#	freebsd

$(TMKMF)_fbsd: $(LIBDIR)/libposix.a $(LIBDIR)/tmkmf_posix.o
	$(GCP) $(LIBDIR)/tmkmf_posix.o -L$(LIBDIR) -lposix -pthread -export-dynamic -lm -o $@

build/tmk_fbsd_gcc: $(TMKMF)_fbsd
	$(TMKMF)_fbsd tmkmakefile RECURSE CONTEXT fbsd_gcc
	@echo "================ MAKEFILES HAVE BEEN GENERATED ===================="

build/tmk_fbsd_gcc_release: $(TMKMF)_fbsd
	$(TMKMF)_fbsd tmkmakefile RECURSE CONTEXT fbsd_gcc_release
	@echo "========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ==========="

build/tmk_fbsd_gcc_debug: $(TMKMF)_fbsd
	$(TMKMF)_fbsd tmkmakefile RECURSE CONTEXT fbsd_gcc_debug
	@echo "========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============"

fbsd: build/tmk_fbsd_gcc
	@$(MAKE) -s -f build/tmk_fbsd_gcc all

fbsd_libs: build/tmk_fbsd_gcc
	@$(MAKE) -s -f build/tmk_fbsd_gcc libs

fbsd_release: build/tmk_fbsd_gcc_release
	@$(MAKE) -s -f build/tmk_fbsd_gcc_release all

fbsd_debug: build/tmk_fbsd_gcc_debug
	@$(MAKE) -s -f build/tmk_fbsd_gcc_debug all

fbsd_install: build/tmk_fbsd_gcc_release
	@$(MAKE) -s -f build/tmk_fbsd_gcc_release all install

fbsd_clean: build/tmk_fbsd_gcc
	@$(MAKE) -s -f build/tmk_fbsd_gcc clean

#	ps2 (cross on Linux)

build/tmk_ps2_gcc: $(TMKMF)_linux
	$(TMKMF)_linux tmkmakefile RECURSE CONTEXT ps2_gcc
	@echo "================ MAKEFILES HAVE BEEN GENERATED ===================="

build/tmk_ps2_gcc_release: $(TMKMF)_linux
	$(TMKMF)_linux tmkmakefile RECURSE CONTEXT ps2_gcc_release
	@echo "========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ==========="

build/tmk_ps2_gcc_debug: $(TMKMF)_linux
	$(TMKMF)_linux tmkmakefile RECURSE CONTEXT ps2_gcc_debug
	@echo "========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============"

ps2_gcc: build/tmk_ps2_gcc
	@$(MAKE) -s -f build/tmk_ps2_gcc all

ps2_gcc_libs: build/tmk_ps2_gcc
	@$(MAKE) -s -f build/tmk_ps2_gcc libs

ps2_gcc_release: build/tmk_ps2_gcc_release
	@$(MAKE) -s -f build/tmk_ps2_gcc_release all

ps2_gcc_debug: build/tmk_ps2_gcc_debug
	@$(MAKE) -s -f build/tmk_ps2_gcc_debug all

ps2_gcc_install: build/tmk_ps2_gcc_release
	@$(MAKE) -s -f build/tmk_ps2_gcc_release all install

ps2_gcc_clean: build/tmk_ps2_gcc
	@$(MAKE) -s -f build/tmk_ps2_gcc clean

###########################################################################
#	Amiga/SASC

SASC = sc resopt nover noicons cpu=68030 math=68882 params=r data=fo prec=mixed strsect=code strmer nochkabort nostkchk idir= def=TSYS_AMIGA
AAR = oml
ARM = delete quiet
ALIBS = lib:scm881nb.lib lib:scnb.lib lib:debug.lib lib:amiga.lib
ALD = slink quiet noicons define smallcode
AMAKE = smake

$(LIBDIR)/amiga.lib:
	-makedir >NIL: build/lib build/bin
	$(SASC) boot/init.c objname $(LIBDIR)/init.o
	$(SASC) boot/teklib.c objname $(LIBDIR)/teklib.o
	$(SASC) idir=mods/hal mods/hal/hal_mod.c objname $(LIBDIR)/hal_mod.o
	$(SASC) idir=mods/exec mods/exec/exec_all.c objname $(LIBDIR)/exec_mod.o
	$(SASC) idir=mods/time mods/time/time_mod.c objname $(LIBDIR)/time_mod.o
	$(SASC) idir=mods/hash mods/hash/hash_mod.c objname $(LIBDIR)/hash_mod.o
	$(SASC) idir=mods/util mods/util/util_all.c objname $(LIBDIR)/util_mod.o
	$(SASC) idir=mods/unistring mods/unistring/unistring_all.c objname $(LIBDIR)/unistring_mod.o
	$(SASC) idir=mods/io/master mods/io/master/io_all.c objname $(LIBDIR)/io_master.o
	$(SASC) idir=mods/hal mods/hal/amiga/hal.c objname $(LIBDIR)/hal.o
	$(SASC) idir=mods/io/default/amiga mods/io/default/amiga/iohnd_default.c objname $(LIBDIR)/io_default.o
	$(SASC) idir=boot/ boot/amiga/host.c objname $(LIBDIR)/host.o
	$(AAR) $@ R $(COMMONOBJS)
	$(ARM) $(COMMONOBJS)

$(LIBDIR)/tmkmf_amiga.o:
	$(SASC) idir=apps/tmkmf apps/tmkmf/main.c objname $@

$(TMKMF)_amiga: $(LIBDIR)/amiga.lib $(LIBDIR)/tmkmf_amiga.o
	@$(ALD) FROM lib:c.o $(LIBDIR)/tmkmf_amiga.o LIB $(LIBDIR)/amiga.lib $(ALIBS) TO $@

build/tmk_amiga_sasc: $(TMKMF)_amiga
	$(TMKMF)_amiga tmkmakefile RECURSE CONTEXT amiga_sasc BUILDDIR build
	@echo "================ MAKEFILES HAVE BEEN GENERATED ===================="

build/tmk_amiga_sasc_release: $(TMKMF)_amiga
	$(TMKMF)_amiga tmkmakefile RECURSE CONTEXT amiga_sasc_release BUILDDIR build
	@echo "========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ==========="

build/tmk_amiga_sasc_debug: $(TMKMF)_amiga
	$(TMKMF)_amiga tmkmakefile RECURSE CONTEXT amiga_sasc_debug BUILDDIR build
	@echo "========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============"

amiga: build/tmk_amiga_sasc
	@$(AMAKE) -s -f build/tmk_amiga_sasc all

amiga_libs: build/tmk_amiga_sasc
	@$(AMAKE) -s -f build/tmk_amiga_sasc libs

amiga_release: build/tmk_amiga_sasc_release
	@$(AMAKE) -s -f build/tmk_amiga_sasc_release all

amiga_debug: build/tmk_amiga_sasc_debug
	@$(AMAKE) -s -f build/tmk_amiga_sasc_debug all

amiga_install: build/tmk_amiga_sasc_release
	@$(AMAKE) -s -f build/tmk_amiga_sasc_release all install

amiga_clean: build/tmk_amiga_sasc
	@$(AMAKE) -s -f build/tmk_amiga_sasc clean

#	Amiga/gcc

AGCC = gcc -I. -DTSYS_AMIGA -noixemul
AGAR = ar r
AGRANLIB = ranlib

$(LIBDIR)/libamigcc.a:
	-mkdir -p build/lib build/bin
	$(AGCC) boot/init.c -c -o $(LIBDIR)/init.o
	$(AGCC) boot/teklib.c -c -o $(LIBDIR)/teklib.o
	$(AGCC) -Imods/hal mods/hal/hal_mod.c -c -o $(LIBDIR)/hal_mod.o
	$(AGCC) -Imods/exec mods/exec/exec_all.c -c -o $(LIBDIR)/exec_mod.o
	$(AGCC) -Imods/time mods/time/time_mod.c -c -o $(LIBDIR)/time_mod.o
	$(AGCC) -Imods/hash mods/hash/hash_mod.c -c -o $(LIBDIR)/hash_mod.o
	$(AGCC) -Imods/util mods/util/util_all.c -c -o $(LIBDIR)/util_mod.o
	$(AGCC) -Imods/unistring mods/unistring/unistring_all.c -c -o $(LIBDIR)/unistring_mod.o
	$(AGCC) -Imods/io/master mods/io/master/io_all.c -c -o $(LIBDIR)/io_master.o
	$(AGCC) -Imods/hal mods/hal/amiga/hal.c -c -o $(LIBDIR)/hal.o
	$(AGCC) -Imods/io/default/amiga mods/io/default/amiga/iohnd_default.c -c -o $(LIBDIR)/io_default.o
	$(AGCC) -Iboot boot/amiga/host.c -c -o $(LIBDIR)/host.o
	$(AGAR) $@ $(COMMONOBJS)
	$(AGRANLIB) $@
	$(ARM) $(COMMONOBJS)

$(LIBDIR)/tmkmf_amiga_gcc.o:
	$(AGCC) -Iapps/tmkmf apps/tmkmf/main.c -c -o $@

$(TMKMF)_amiga_gcc: $(LIBDIR)/libamigcc.a $(LIBDIR)/tmkmf_amiga_gcc.o
	$(AGCC) -noixemul $(LIBDIR)/tmkmf_amiga_gcc.o -o $@ -L$(LIBDIR) -lamigcc -lnix -lamiga -ldebug -lm

build/tmk_amiga_gcc: $(TMKMF)_amiga_gcc
	$(TMKMF)_amiga_gcc tmkmakefile RECURSE CONTEXT amiga_gcc
	@echo '================ MAKEFILES HAVE BEEN GENERATED ===================='

build/tmk_amiga_gcc_release: $(TMKMF)_amiga_gcc
	$(TMKMF)_amiga_gcc tmkmakefile RECURSE CONTEXT amiga_gcc_release
	@echo '========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ==========='

build/tmk_amiga_gcc_debug: $(TMKMF)_amiga_gcc
	$(TMKMF)_amiga_gcc tmkmakefile RECURSE CONTEXT amiga_gcc_debug
	@echo '========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============'

amiga_gcc: build/tmk_amiga_gcc
	@$(MAKE) -s -f build/tmk_amiga_gcc all

amiga_gcc_libs: build/tmk_amiga_gcc
	@$(MAKE) -s -f build/tmk_amiga_gcc libs

amiga_gcc_release: build/tmk_amiga_gcc_release
	@$(MAKE) -s -f build/tmk_amiga_gcc_release all

amiga_gcc_debug: build/tmk_amiga_gcc_debug
	@$(MAKE) -s -f build/tmk_amiga_gcc_debug all

amiga_gcc_install: build/tmk_amiga_gcc_release
	@$(MAKE) -s -f build/tmk_amiga_gcc_release all install

amiga_gcc_clean: build/tmk_amiga_gcc
	@$(MAKE) -s -f build/tmk_amiga_gcc clean


###########################################################################
#	Darwin

GCD = $(CC) -I. -DTSYS_DARWIN -no-cpp-precomp

$(LIBDIR)/libdarwin.a:
	-mkdir -p build/lib build/bin
	$(GCD) boot/init.c -c -o $(LIBDIR)/init.o
	$(GCD) boot/teklib.c -c -o $(LIBDIR)/teklib.o
	$(GCD) -Imods/hal mods/hal/hal_mod.c -c -o $(LIBDIR)/hal_mod.o
	$(GCD) -Imods/exec mods/exec/exec_all.c -c -o $(LIBDIR)/exec_mod.o
	$(GCD) -Imods/time mods/time/time_mod.c -c -o $(LIBDIR)/time_mod.o
	$(GCD) -Imods/hash mods/hash/hash_mod.c -c -o $(LIBDIR)/hash_mod.o
	$(GCD) -Imods/util mods/util/util_all.c -c -o $(LIBDIR)/util_mod.o
	$(GCD) -Imods/unistring mods/unistring/unistring_all.c -c -o $(LIBDIR)/unistring_mod.o
	$(GCD) -Imods/io/master mods/io/master/io_all.c -c -o $(LIBDIR)/io_master.o
	$(GCD) -Imods/hal mods/hal/darwin/hal.c -c -o $(LIBDIR)/hal.o
	$(GCD) -Imods/io/default/posix mods/io/default/posix/iohnd_default.c -c -o $(LIBDIR)/io_default.o
	$(GCD) -Iboot boot/darwin/host.c -c -o $(LIBDIR)/host.o
	$(GAR) $@ $(COMMONOBJS)
	$(GRANLIB) $@
	$(GRM) $(COMMONOBJS)

$(LIBDIR)/tmkmf_darwin.o:
	$(GCD) -Iapps/tmkmf apps/tmkmf/main.c -c -o $@

$(TMKMF)_darwin: $(LIBDIR)/libdarwin.a $(LIBDIR)/tmkmf_darwin.o
	$(GCD) $(LIBDIR)/tmkmf_darwin.o -L$(LIBDIR) -ldarwin -lpthread -lSystem -o $@

build/tmk_darwin_gcc: $(TMKMF)_darwin
	$(TMKMF)_darwin tmkmakefile RECURSE CONTEXT darwin_gcc
	@echo "================ MAKEFILES HAVE BEEN GENERATED ===================="

build/tmk_darwin_gcc_release: $(TMKMF)_darwin
	$(TMKMF)_darwin tmkmakefile RECURSE CONTEXT darwin_gcc_release
	@echo "========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ==========="

build/tmk_darwin_gcc_debug: $(TMKMF)_darwin
	$(TMKMF)_darwin tmkmakefile RECURSE CONTEXT darwin_gcc_debug
	@echo "========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============"

darwin: build/tmk_darwin_gcc
	@$(MAKE) -s -f build/tmk_darwin_gcc all
	@echo "(PACKAGING MODULES)"
	@cp -Rf bin/darwin/mod/* lib/darwin/tek.framework/mod/
	@echo "(PACKAGING HEADERS)"
	@cp -Rf tek/* lib/darwin/tek.framework/Headers/
	@find lib/darwin/tek.framework -name "CVS" -print | xargs rm -r

darwin_libs: build/tmk_darwin_gcc
	@$(MAKE) -s -f build/tmk_darwin_gcc libs

darwin_release: build/tmk_darwin_gcc_release
	@$(MAKE) -s -f build/tmk_darwin_gcc_release all
	@echo "(PACKAGING MODULES)"
	@cp -Rf bin/darwin/mod/* lib/darwin/tek.framework/mod/
	@echo "(PACKAGING HEADERS)"
	@cp -Rf tek/* lib/darwin/tek.framework/Headers/
	@find lib/darwin/tek.framework -name "CVS" -print | xargs rm -r

darwin_debug: build/tmk_darwin_gcc_debug
	@$(MAKE) -s -f build/tmk_darwin_gcc_debug all

darwin_install: darwin_release
	@echo "Please enter your password when requested."
	sudo cp -rp lib/darwin/tek.framework /Library/Frameworks/

darwin_clean: build/tmk_darwin_gcc
	@$(MAKE) -s -f build/tmk_darwin_gcc clean

###########################################################################
#	Win32-vcpp

VCPP = cl /I. /nologo /noBool /D "WIN32" /D "TEKLIB" /D "TSYS_WIN32" /MLd
WAR = link -lib /nologo
WRM = erase /S
WLD = link /nologo /nodefaultlib /subsystem:console
WINLIBS = msvcrt.lib kernel32.lib user32.lib shell32.lib advapi32.lib libc.lib
 
$(LIBDIR)/win32.lib:
	-mkdir build\lib build\bin
	$(VCPP) /c boot/init.c /Fo$(LIBDIR)/init.o
	$(VCPP) /c boot/teklib.c /Fo$(LIBDIR)/teklib.o
	$(VCPP) /Imods/hal /c mods/hal/hal_mod.c /Fo$(LIBDIR)/hal_mod.o
	$(VCPP) /Imods/exec /c mods/exec/exec_all.c /Fo$(LIBDIR)/exec_mod.o
	$(VCPP) /Imods/time /c mods/time/time_mod.c /Fo$(LIBDIR)/time_mod.o
	$(VCPP) /Imods/hash /c mods/hash/hash_mod.c /Fo$(LIBDIR)/hash_mod.o
	$(VCPP) /Imods/util /c mods/util/util_all.c /Fo$(LIBDIR)/util_mod.o
	$(VCPP) /Imods/unistring /c mods/unistring/unistring_all.c /Fo$(LIBDIR)/unistring_mod.o
	$(VCPP) /Imods/io/master /c mods/io/master/io_all.c /Fo$(LIBDIR)/io_master.o
	$(VCPP) /Imods/hal /c mods/hal/win32/hal.c /Fo$(LIBDIR)/hal.o
	$(VCPP) /Imods/io/default/win32 /c mods/io/default/win32/iohnd_default.c /Fo$(LIBDIR)/io_default.o
	$(VCPP) /Iboot /c boot/win32/host.c /Fo$(LIBDIR)/host.o
	$(WAR) $(COMMONOBJS) /out:"$@"
	$(WRM) "$(LIBDIR)\*.o"

$(LIBDIR)/tmkmf_win32.o:
	$(VCPP) /Iapps/tmkmf /c apps/tmkmf/main.c /Fo$@

$(TMKMF)_win32.exe: $(LIBDIR)/win32.lib $(LIBDIR)/tmkmf_win32.o
	$(WLD) $(LIBDIR)/win32.lib $(WINLIBS) $(LIBDIR)/tmkmf_win32.o /out:$@

build/tmk_win32_vcpp: $(TMKMF)_win32.exe
	build\bin\tmkmf_win32 tmkmakefile RECURSE CONTEXT win32_vcpp
	@echo ================ MAKEFILES HAVE BEEN GENERATED ====================

build/tmk_win32_vcpp_release: $(TMKMF)_win32.exe
	build\bin\tmkmf_win32 tmkmakefile RECURSE CONTEXT win32_vcpp_release
	@echo ========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ===========

build/tmk_win32_vcpp_debug: $(TMKMF)_win32.exe
	build\bin\tmkmf_win32 tmkmakefile RECURSE CONTEXT win32_vcpp_debug
	@echo ========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============

win32: build/tmk_win32_vcpp
	@$(MAKE) /nologo /s /f build/tmk_win32_vcpp all

win32_libs: build/tmk_win32_vcpp
	@$(MAKE) /nologo /s /f build/tmk_win32_vcpp libs

win32_release: build/tmk_win32_vcpp_release
	@$(MAKE) /nologo /s /f build/tmk_win32_vcpp_release all

win32_debug: build/tmk_win32_vcpp_debug
	@$(MAKE) /nologo /s /f build/tmk_win32_vcpp_debug all

win32_install: build/tmk_win32_vcpp_release
	@$(MAKE) /nologo /s /f build/tmk_win32_vcpp_release all
	-mkdir "%CommonProgramFiles%\tek\mod"
	copy /Y "bin\win32\mod\*.dll" "%CommonProgramFiles%\tek\mod"

win32_clean: build/tmk_win32_vcpp
	@$(MAKE) /nologo /s /f build/tmk_win32_vcpp clean

#	Win32 mingw-gcc

MGWMAKE = make.exe

MGWCC = gcc.exe -I. -mno-cygwin -DTSYS_WIN32
MGWAR = ar.exe r
MGWRANLIB = ranlib.exe
MGWINLIBS = -lmsvcrt -lkernel32 -luser32 -lshell32 -ladvapi32

MKDIR = mkdir -p
RM = rm 

WMKDIR = cmd \\\\/S \\\\E:ON \\\\/C mkdir 
WCP    = cmd \\\\/S \\\\/C copy  


$(LIBDIR)/libwin32.a:
	-$(MKDIR) build/lib build/bin
	$(MGWCC) -c boot/init.c -o$(LIBDIR)/init.o
	$(MGWCC) -c boot/teklib.c -o$(LIBDIR)/teklib.o
	$(MGWCC) -Imods/hal -c mods/hal/hal_mod.c -o $(LIBDIR)/hal_mod.o
	$(MGWCC) -Imods/exec -c mods/exec/exec_all.c -o $(LIBDIR)/exec_mod.o
	$(MGWCC) -Imods/time -c mods/time/time_mod.c -o $(LIBDIR)/time_mod.o
	$(MGWCC) -Imods/hash -c mods/hash/hash_mod.c -o $(LIBDIR)/hash_mod.o
	$(MGWCC) -Imods/util -c mods/util/util_all.c -o $(LIBDIR)/util_mod.o
	$(MGWCC) -Imods/unistring -c mods/unistring/unistring_all.c -o $(LIBDIR)/unistring_mod.o
	$(MGWCC) -Imods/io/master -c mods/io/master/io_all.c -o $(LIBDIR)/io_master.o
	$(MGWCC) -Imods/hal -c mods/hal/win32/hal.c -o $(LIBDIR)/hal.o
	$(MGWCC) -Imods/io/default/win32 -c mods/io/default/win32/iohnd_default.c -o$(LIBDIR)/io_default.o
	-$(MGWCC) -Iboot -c boot/win32/host.c -o$(LIBDIR)/host.o
	-$(MGWAR) $(LIBDIR)/libwin32.a $(COMMONOBJS)
	-$(RM) $(LIBDIR)/*.o

$(LIBDIR)/tmkmf_win32_gcc.o:
	$(MGWCC) -Iapps/tmkmf -c apps/tmkmf/main.c -o $@

$(TMKMF)_win32_gcc.exe: $(LIBDIR)/libwin32.a $(LIBDIR)/tmkmf_win32_gcc.o
	$(MGWCC) $(MGWINLIBS) $(LIBDIR)/tmkmf_win32_gcc.o -L$(LIBDIR) -lwin32 -o $@

build/tmk_win32_gcc: $(TMKMF)_win32_gcc.exe
	./build/bin/tmkmf_win32_gcc.exe tmkmakefile RECURSE CONTEXT win32_gcc
	@echo "================ MAKEFILES HAVE BEEN GENERATED ===================="

build/tmk_win32_gcc_release: $(TMKMF)_win32_gcc.exe
	./build/bin/tmkmf_win32_gcc.exe tmkmakefile RECURSE CONTEXT win32_gcc_release
	@echo "========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ==========="

build/tmk_win32_gcc_debug: $(TMKMF)_win32_gcc.exe
	./build/bin/tmkmf_win32_gcc.exe tmkmakefile RECURSE CONTEXT win32_gcc_debug
	@echo "========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ==========+="

win32_gcc: build/tmk_win32_gcc
	@$(MGWMAKE) -s -f build/tmk_win32_gcc all

win32_libs_gcc: build/tmk_win32_gcc
	@$(MGWMAKE) -s -f build/tmk_win32_gcc libs

win32_gcc_release: build/tmk_win32_gcc_release
	@$(MGWMAKE) -s -f build/tmk_win32_gcc_release all

win32_gcc_debug: build/tmk_win32_gcc_debug
	@$(MGWMAKE) -s -f build/tmk_win32_gcc_debug all

win32_gcc_install: build/tmk_win32_gcc_release
	@$(MGWMAKE) -s -f build/tmk_win32_gcc_release all 
	-$(WMKDIR) "$(COMMONPROGRAMFILES)\tek\mod"
	@$(WCP) "bin\win32\mod\*.dll" "$(COMMONPROGRAMFILES)\tek\mod"

win32_gcc_clean: build/tmk_win32_gcc
	@$(MGWMAKE) -s -f build/tmk_win32_gcc clean


###########################################################################
#	MorphOS/gcc

MGCC = ppc-morphos-gcc -noixemul -I. -DTEKLIB -DTSYS_MORPHOS -DUSE_INLINE_STDARG
MGAR = ppc-morphos-ar r
MGRANLIB = ppc-morphos-ranlib

$(LIBDIR)/libmorphos.a:
	-mkdir -p build/lib build/bin
	$(MGCC) boot/init.c -c -o $(LIBDIR)/init.o
	$(MGCC) boot/teklib.c -c -o $(LIBDIR)/teklib.o
	$(MGCC) -Imods/hal mods/hal/hal_mod.c -c -o $(LIBDIR)/hal_mod.o
	$(MGCC) -Imods/exec mods/exec/exec_all.c -c -o $(LIBDIR)/exec_mod.o
	$(MGCC) -Imods/time mods/time/time_mod.c -c -o $(LIBDIR)/time_mod.o
	$(MGCC) -Imods/hash mods/hash/hash_mod.c -c -o $(LIBDIR)/hash_mod.o
	$(MGCC) -Imods/util mods/util/util_all.c -c -o $(LIBDIR)/util_mod.o
	$(MGCC) -Imods/unistring mods/unistring/unistring_all.c -c -o $(LIBDIR)/unistring_mod.o
	$(MGCC) -Imods/io/master mods/io/master/io_all.c -c -o $(LIBDIR)/io_master.o
	$(MGCC) -Imods/hal mods/hal/amiga/hal.c -c -o $(LIBDIR)/hal.o
	$(MGCC) -Imods/io/default/amiga mods/io/default/amiga/iohnd_default.c -c -o $(LIBDIR)/io_default.o
	$(MGCC) -Iboot boot/amiga/host.c -c -o $(LIBDIR)/host.o
	$(MGAR) $@ $(COMMONOBJS)
	$(MGRANLIB) $@
	$(ARM) $(COMMONOBJS)

$(LIBDIR)/tmkmf_morphos_gcc.o: 
	$(MGCC) -Iapps/tmkmf apps/tmkmf/main.c -c -o $@

$(TMKMF)_morphos_gcc: $(LIBDIR)/libmorphos.a $(LIBDIR)/tmkmf_morphos_gcc.o
	$(MGCC) $(LIBDIR)/tmkmf_morphos_gcc.o -o $@ -L$(LIBDIR) -lmorphos -labox -laboxstubs -lc -lm -lmath

build/tmk_morphos_gcc: $(TMKMF)_morphos_gcc
	$(TMKMF)_morphos_gcc tmkmakefile RECURSE CONTEXT morphos_gcc
	@echo "================ MAKEFILES HAVE BEEN GENERATED ===================="

build/tmk_morphos_gcc_release: $(TMKMF)_morphos_gcc
	$(TMKMF)_morphos_gcc tmkmakefile RECURSE CONTEXT morphos_gcc_release
	@echo "========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ==========="

build/tmk_morphos_gcc_debug: $(TMKMF)_morphos_gcc
	$(TMKMF)_morphos_gcc tmkmakefile RECURSE CONTEXT morphos_gcc_debug
	@echo "========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============"

morphos_gcc: build/tmk_morphos_gcc
	@$(MAKE) -s -f build/tmk_morphos_gcc all

morphos_gcc_libs: build/tmk_morphos_gcc
	@$(MAKE) -s -f build/tmk_morphos_gcc libs

morphos_gcc_release: build/tmk_morphos_gcc_release
	@$(MAKE) -s -f build/tmk_morphos_gcc_release all

morphos_gcc_debug: build/tmk_morphos_gcc_debug
	@$(MAKE) -s -f build/tmk_morphos_gcc_debug all

morphos_gcc_install: build/tmk_morphos_gcc_release
	@$(MAKE) -s -f build/tmk_morphos_gcc_release all install

morphos_gcc_clean: build/tmk_morphos_gcc
	@$(MAKE) -s -f build/tmk_morphos_gcc clean

###########################################################################
#	intent

VPCC = vpcc -I. -DTSYS_INTENT -Wall -O2
VPAR = vpar rc
VPRM = rm -f
VPRANLIB = @echo -- ranlib
VPAS = asm -ng

$(LIBDIR)/libintent.a:
	-mkdir -p build/lib build/bin
	$(VPAS) mods/hal/intent/thread.asm
	$(VPCC) boot/init.c -c -o $(LIBDIR)/init.o
	$(VPCC) boot/teklib.c -c -o $(LIBDIR)/teklib.o
	$(VPCC) -Imods/hal mods/hal/hal_mod.c -c -o $(LIBDIR)/hal_mod.o
	$(VPCC) -Imods/exec mods/exec/exec_all.c -c -o $(LIBDIR)/exec_mod.o
	$(VPCC) -Imods/time mods/time/time_mod.c -c -o $(LIBDIR)/time_mod.o
	$(VPCC) -Imods/hash mods/hash/hash_mod.c -c -o $(LIBDIR)/hash_mod.o
	$(VPCC) -Imods/util mods/util/util_all.c -c -o $(LIBDIR)/util_mod.o
	$(VPCC) -Imods/unistring mods/unistring/unistring_all.c -c -o $(LIBDIR)/unistring_mod.o
	$(VPCC) -Imods/io/master mods/io/master/io_all.c -c -o $(LIBDIR)/io_master.o
	$(VPCC) -Imods/io/default/posix mods/io/default/posix/iohnd_default.c -c -o $(LIBDIR)/io_default.o
	$(VPCC) -Imods/hal mods/hal/intent/hal.c -c -o $(LIBDIR)/hal.o
	$(VPCC) -Iboot boot/intent/host.c -c -o $(LIBDIR)/host.o
	$(VPAR) $@ $(COMMONOBJS)
	$(VPRANLIB) $@
	$(VPRM) $(COMMONOBJS)

$(LIBDIR)/tmkmf_intent.o:
	$(VPCC) -Iapps/tmkmf apps/tmkmf/main.c -c -o $@

$(TMKMF)_intent: $(LIBDIR)/libintent.a $(LIBDIR)/tmkmf_intent.o
	$(VPCC) $(LIBDIR)/tmkmf_intent.o -L$(LIBDIR) -lintent -lsys -o $@

build/tmk_intent_vpcc: $(TMKMF)_intent
	$(TMKMF)_intent tmkmakefile RECURSE CONTEXT intent_vpcc
	@echo {================ MAKEFILES HAVE BEEN GENERATED ====================}

build/tmk_intent_vpcc_release: $(TMKMF)_intent
	$(TMKMF)_intent tmkmakefile RECURSE CONTEXT intent_vpcc_release
	@echo {========= MAKEFILES (RELEASE BUILD) HAVE BEEN GENERATED ===========}

build/tmk_intent_vpcc_debug: $(TMKMF)_intent
	$(TMKMF)_intent tmkmakefile RECURSE CONTEXT intent_vpcc_debug
	@echo {========== MAKEFILES (DEBUG BUILD) HAVE BEEN GENERATED ============}

intent: build/tmk_intent_vpcc
	@$(MAKE) -s -f build/tmk_intent_vpcc all

intent_libs: build/tmk_intent_vpcc
	@$(MAKE) -s -f build/tmk_intent_vpcc libs

intent_release: build/tmk_intent_vpcc_release
	@$(MAKE) -s -f build/tmk_intent_vpcc_release all

intent_debug: build/tmk_intent_vpcc_release
	@$(MAKE) -s -f build/tmk_intent_vpcc_release all

intent_install: build/tmk_intent_vpcc_release
	@$(MAKE) -s -f build/tmk_intent_vpcc_release all install

intent_clean: build/tmk_intent_vpcc
	@$(MAKE) -s -f build/tmk_intent_vpcc clean

###########################################################################
#	Special builds

clean:
	-find . \( -name '*.[oa]' -or -name '*.lib' -or -name '*.pdb' \) -type f -print | xargs rm -f
	-find bin/[a-z]*/ -type f -print | xargs rm -f

#	-find . -name "tek.framework" -print | xargs rm -r

distclean: clean
	-find . \( -name Debug -or -name Release \) -type d -print | xargs rm -Rf
	-find . \( -name '*.plg' -o -name '*.ncb' -o -name '*.opt' -o -name '*.lnk' \
		-o -name 'tmk_*_*' -o -name 'temp_smk.tmp' \) -type f -print | xargs rm -f
	rm -f build/bin/tmkmf_*
