#
# Copyright (c) 1993-1994 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the names of the copyright holders nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# @(#) $Header$ (LBL)
#

ZIP = c:\progra~1\winzip\winzip32

# Uncomment this line to compile an optimized (with no debug symbols)
# version of the program.
NODEBUG=1

APPVER=4.0
TOOLS32 =  d:\progra~1\micros~1\vc98
#VC5: TOOLS32 =  d:\progra~1\devstudio\vc
cc32 = $(TOOLS32)\bin\cl -I$(TOOLS32)\include
#rc32 = d:\progra~1\micros~1\common\msdev98\bin\rc
rc32 = rc
link32 = link

TARGETOS=WIN95
!include <ntwin32.mak>


ALL = vic histtolut
all:	vic

.SUFFIXES : .cc

.cc.obj:
	$(cc32) $(cdebug) $(cflags) $(cvars) $(CFLAGS) -Fo$@ -FR$*.sbr -Tp $<

.c.obj:
	$(cc32) $(cdebug) $(cflags) $(cvars) $(CFLAGS) -Fo$@ -FR$*.sbr $<

ED_YBITS = 4

TK_DIR  = ../tk-8.0
TCL_DIR = ../tcl-8.0

INCLUDE_TK = -I$(TK_DIR)/win -I$(TK_DIR)/generic -I$(TK_DIR)/compat
INCLUDE_TCL = -I$(TCL_DIR)/win -I$(TCL_DIR)/generic
INCLUDE_X11 = -I$(TK_DIR)/xlib
INCLUDE_MISC = -Itmndec -Itmn-x

GENERICDIR	= $(TK_DIR)\generic
RCDIR		= $(TK_DIR)\win\rc

LIB_TK = ../tk-8.0/win/debug/tklib.lib
LIB_TCL = ../tcl-8.0/win/debug/tcllib.lib

MKDEP = \mksnt\sh .\mkdep

BSC32= bscmake
BSC32_FLAGS=/nologo /n

LIBRARY_TK = $(TK_DIR)/library
LIBRARY_TCL = $(TCL_DIR)/library
TK_LIBRARY_FILES = \
		$(LIBRARY_TCL)/init.tcl \
		$(LIBRARY_TK)/tk.tcl \
		$(LIBRARY_TK)/button.tcl \
		$(LIBRARY_TK)/dialog.tcl \
		$(LIBRARY_TK)/entry.tcl \
		$(LIBRARY_TK)/focus.tcl \
		$(LIBRARY_TK)/listbox.tcl \
		$(LIBRARY_TK)/menu.tcl \
		$(LIBRARY_TK)/palette.tcl \
		$(LIBRARY_TK)/scale.tcl \
		$(LIBRARY_TK)/tearoff.tcl \
		$(LIBRARY_TK)/text.tcl \
		$(LIBRARY_TK)/optMenu.tcl $(LIBRARY_TK)/scrlbar.tcl

LIB_H263=  tmndec\libh263.lib tmn-x\libh263coder.lib
LIB_GRABBER = vfw32.lib
INCLUDE_GRABBER = 
OBJ_GRABBER = grabber-win32.obj
SRC_GRABBER = $(OBJ_GRABBER:.obj=.cc) 
OBJ_XIL = 
LIBS = $(LIB_H263) $(LIB_GRABBER) $(LIB_TK) $(LIB_TCL) $(guilibs) winmm.lib
INCLUDES = $(INCLUDE_MISC) $(INCLUDE_GRABBER) $(INCLUDE_TK) $(INCLUDE_TCL) \
	$(INCLUDE_X11) $(MD_INC) -I./jpeg -I./p64 -I. -Ih263
DEFINE = -DED_YBITS=$(ED_YBITS) -DSIGRET=void -DSIGARGS=int -D_Windows 
#-DWIN32_LEAN_AND_MEAN -D_i386_ 
BFLAGS = $(DEFINE) $(INCLUDES)
CFLAGS = $(CCOPT) $(BFLAGS)

#
# Remember to add a dependency if you add any tcl sources here.
#
TCL_VIC = ui-ctrlmenu.tcl ui-main.tcl ui-resource.tcl ui-relate.tcl\
	ui-srclist.tcl ui-stats.tcl ui-util.tcl ui-windows.tcl \
	ui-switcher.tcl ui-extout.tcl ui-grabber.tcl ui-win32.tcl \
	cf-main.tcl cf-tm.tcl cf-confbus.tcl cf-network.tcl cf-util.tcl \
	tkerror.tcl entry.tcl

#
# These modules are broken in libg++.  Rather than fix libg++
# every time I install it on a given system, just make them
# here (these sources came from tcl's compat directory)
#
BROKEN_OBJ = strtol.obj strtoul.obj
COLOR_OBJS = color-true.obj color-pseudo.obj color-dither.obj color-ed.obj \
	color-hi.obj color-quant.obj color-gray.obj color-mono.obj color-hist.obj
QFDES_OBJS = qfDES.obj qfDES_key.obj qfDES_memory.obj

H263_OBJS = h263/h263rtp.obj h263/h263dec.obj h263/bitIn.obj h263/input.obj h263/getgob.obj \
			h263/reconh263.obj h263/recon.obj h263/getvlc.obj h263/getblk.obj h263/h263enc.obj \
			h263/motion.obj h263/block.obj h263/bitOut.obj h263/h263mux.obj h263/idctdec.obj \
			h263/fdct.obj h263/code.obj h263/gethdr.obj h263/idctenc.obj h263/sac.obj

OBJ0 =	getopt.obj win32.obj win32X.obj
OBJ1 =	inet.obj cellb_tables.obj tkStripchart.obj md5c.obj random.obj
OBJ2 =	main.obj net.obj net-ip.obj net-ipv6.obj source.obj \
	iohandler.obj timer.obj idlecallback.obj media-timer.obj \
	session.obj net-addr.obj inet6.obj \
	decoder.obj decoder-jpeg.obj decoder-nv.obj decoder-raw.obj \
	decoder-h261.obj decoder-h261v1.obj decoder-bvc.obj  \
	decoder-cellb.obj mbus_engine.obj mbus.obj \
	device.obj grabber.obj vw.obj Tcl.obj Tcl2.obj module.obj transmitter.obj \
	encoder-nv.obj encoder-cellb.obj encoder-h261.obj encoder-jpeg.obj encoder-raw.obj \
	encoder-bvc.obj encoder-h263v2.obj decoder-h263v2.obj assistor-list.obj \
#	H263 codec:
	encoder-h263.obj decoder-h263.obj $(H263_OBJS) \
	transcoder-jpeg.obj framer-jpeg.obj \
	group-ipc.obj confbus.obj renderer.obj renderer-window.obj \
	color.obj $(COLOR_OBJS) $(QFDES_OBJS) \
	rgb-converter.obj jpeg.obj p64.obj dct.obj \
	compositor.obj rate-variable.obj crypt.obj crypt-des.obj \
	grabber-still.obj tkWinColor.obj  
OBJ3 =	cm0.obj cm1.obj huffcode.obj version.obj bv.obj \
	$(TCL_VIC:.tcl=.obj) tcltk.obj
SRC =	$(OBJ1:.obj=.c) $(OBJ2:.obj=.cc) $(BROKEN_OBJ:.obj=.c) \
	$(RTIP_OBJ:.obj=.c) $(SRC_GRABBER) $(OBJ_XIL:.obj=.cc)
OBJ =	$(OBJ0) $(OBJ1) $(OBJ2) $(OBJ3) $(BROKEN_OBJ) $(RTIP_OBJ) $(LIB_H263)

SBR =	$(OBJ:.obj=.sbr) $(OBJ_GRABBER:.obj=.sbr)

OBJ_VDD = $(OBJ0) vdd.obj p64.obj p64dump.obj \
	module.obj renderer.obj renderer-window.obj color.obj \
	$(COLOR_OBJS) \
	Tcl.obj Tcl2.obj vw.obj cm0.obj cm1.obj \
	dct.obj huffcode.obj bv.obj iohandler.obj timer.obj \
	ui-vdd.obj tk.obj

SBR_VDD = $(OBJ_VDD:.obj=.sbr)

OBJ_H261PLAY = $(OBJ0) h261_play.obj p64.obj p64dump.obj \
	module.obj renderer.obj renderer-window.obj color.obj \
	$(COLOR_OBJS) \
	Tcl.obj Tcl2.obj vw.obj cm0.obj cm1.obj \
	dct.obj huffcode.obj bv.obj iohandler.obj random.obj timer.obj \
	ui-h261_play.obj tk.obj

SBR_H261PLAY = $(OBJ_H261PLAY:.obj=.sbr)

OBJ_JPEGPLAY = $(OBJ0) jpeg_play.obj jpeg/jpeg.obj \
	renderer.obj renderer-window.obj color.obj \
	$(COLOR_OBJS) \
	Tcl.obj Tcl2.obj vw.obj cm0.obj cm1.obj \
	dct.obj huffcode.obj jpeg_play_tcl.obj bv.obj iohandler.obj timer.obj \
	ui-jpeg_play.obj tk.obj

SBR_JPEGPLAY = $(OBJ_JPEGPLAY:.obj=.sbr)

LIB_CB = 
OBJ_CB = $(OBJ0) cbAppInit.obj cb.obj confbus.obj group-ipc.obj iohandler.obj \
	net.obj net-ip.obj crypt.obj crypt-des.obj communicator.obj \
	ppm.obj Tcl.obj Tcl2.obj inet.obj md5c.obj

SBR_CB = $(OBJ_CB:.obj=.sbr)

OBJ_H261DUMP = $(OBJ0) h261_dump.obj p64.obj p64dump.obj huffcode.obj dct.obj bv.obj

SBR_H261DUMP = $(OBJ_H261DUMP:.obj=.sbr)

#vic: vic.exe vic.bsc
vic: vic.exe

vic.exe: $(OBJ) $(OBJ_GRABBER) tk.res
	$(link) $(ldebug) $(guilflags) -out:$@ tk.res $(OBJ) $(OBJ_GRABBER) $(LIBS)

vic.bsc : $(OUTDIR)  $(SBR)
    $(BSC32) @<<
  $(BSC32_FLAGS) /o$@ $(SBR)
<<

histtolut: histtolut.exe

histtolut.exe: histtolut.obj convex_hull.obj getopt.obj
	$(cc32) /nologo -o $@ $(CFLAGS) histtolut.obj convex_hull.obj getopt.obj

vdd: vdd.exe vdd.bsc

vdd.exe: $(OBJ_VDD)
	$(cc32) /nologo -o $@ $(CFLAGS) $(LDFLAGS) $(OBJ_VDD) $(LIBS)

vdd.bsc : $(OUTDIR)  $(SBR_VDD)
    $(BSC32) @<<
  $(BSC32_FLAGS) /o$@ $(SBR_VDD)
<<

cb_wish: cb_wish.exe cb_wish.bsc

cb_wish.exe: $(OBJ_CB)
	$(cc32) /nologo -o $@ $(CFLAGS) $(LDFLAGS) $(OBJ_CB) $(LIB_CB)

cb_wish.bsc : $(OUTDIR)  $(SBR_CB)
    $(BSC32) @<<
  $(BSC32_FLAGS) /o$@ $(SBR_CB)
<<

jpeg_play: jpeg_play.exe jpeg_play.bsc

jpeg_play.exe: $(OBJ_JPEGPLAY)
	$(cc32) /nologo $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ_JPEGPLAY) $(LIBS)

jpeg_play.bsc : $(OUTDIR)  $(SBR_JPEGPLAY)
    $(BSC32) @<<
  $(BSC32_FLAGS) /o$@ $(SBR_JPEGPLAY)
<<

h261_play: h261_play.exe h261_play.bsc

h261_play.exe: $(OBJ_H261PLAY)
	$(link) $(ldebug) $(guilflags) -out:$@ $(OBJ_H261PLAY) $(LIBS)

h261_play.bsc : $(OUTDIR)  $(SBR_H261PLAY)
    $(BSC32) @<<
  $(BSC32_FLAGS) /o$@ $(SBR_H261PLAY)
<<

h261_dump: h261_dump.exe h261_dump.bsc

h261_dump.exe: $(OBJ_H261DUMP)
	$(cc32) /nologo -o $@ $(CFLAGS) $(OBJ_H261DUMP)

h261_dump.bsc : $(OUTDIR)  $(SBR_H261DUMP)
    $(BSC32) @<<
  $(BSC32_FLAGS) /o$@ $(SBR_H261DUMP)
<<

h261tortp.exe: h261tortp.cc
	$(cc32) /nologo -o $@ $(CFLAGS) h261tortp.cc

#
# pass-1 embedded tcl files
#
tcltk.obj: $(TK_LIBRARY_FILES) tcl2c++.exe
	-del $@
	.\tcl2c++ 1 $(TK_LIBRARY_FILES) > tcltk.cc
	$(cc32) /Zm500 $(cdebug) $(cflags) $(cvars) -o $@ -FR$*.sbr -c $(CFLAGS) -Tp $(@:.obj=).cc
	del $(@:.obj=).cc

#
# pass-2 embedded tcl files
#
$(TCL_VIC:.tcl=.obj) ui-vdd.obj ui-h261_play.obj: tcl2c++.exe
	-del $@
	.\tcl2c++ 2 $(@:.obj=).tcl > $(@:.obj=).cc
	$(cc32) $(cdebug) $(cflags) $(cvars) -o $@ -c $(CFLAGS) -FR$*.sbr -Tp $(@:.obj=).cc
	del $(@:.obj=).cc

ui-ctrlmenu.obj: ui-ctrlmenu.tcl
ui-extout.obj: ui-extout.tcl
ui-grabber.obj: ui-grabber.tcl
ui-main.obj: ui-main.tcl
ui-resource.obj: ui-resource.tcl
ui-srclist.obj: ui-srclist.tcl
ui-stats.obj: ui-stats.tcl
ui-switcher.obj: ui-switcher.tcl
ui-util.obj: ui-util.tcl
ui-windows.obj: ui-windows.tcl
ui-win32.obj: ui-win32.tcl
cf-main.obj: cf-main.tcl
cf-util.obj: cf-util.tcl
cf-network.obj: cf-network.tcl
cf-tm.obj: cf-tm.tcl
cf-confbus.obj: cf-confbus.tcl
tkerror.obj: tkerror.tcl
entry.obj: entry.tcl
ui-vdd.obj: ui-vdd.tcl
ui-h261_play.obj: ui-h261_play.tcl
ui-relate.obj: ui-relate.tcl

mkversion.exe: mkversion.obj
	-del $@
	$(cc32) /nologo -o $@ $(CFLAGS) mkversion.obj

version.c: mkversion.exe VERSION
	-del version.c
	.\mkversion < VERSION > version.c

version.mak: mkversion.exe VERSION
	-del version.mak
	.\mkversion VICVERSION=%%s < VERSION > version.mak

cm0.obj: cm0.c
cm0.c: rgb-cube.ppm jv-cube-128.ppm ppmtolut.exe
	-del $@
	.\ppmtolut -n cube rgb-cube.ppm >> $@
	.\ppmtolut -n jv_cube jv-cube-128.ppm >> $@

cm1.obj: cm1.c
cm1.c: yuv-map.ppm cm170.ppm ppmtolut.exe
	-del $@
	.\ppmtolut -n quant cm170.ppm >> $@
	.\ppmtolut -Y $(ED_YBITS) -n ed -e yuv-map.ppm >> $@

mkhuff.obj: p64/mkhuff.cc
	$(cc32) $(cdebug) $(cflags) $(cvars) $(CFLAGS) -o $@ -FR$*.sbr -Tp p64/mkhuff.cc

mkhuff.exe: mkhuff.obj getopt.obj
	-del $@
	$(cc32) /nologo -o $@ $(CFLAGS) mkhuff.obj getopt.obj

huffcode.c: mkhuff.exe
	-del $@
	.\mkhuff -e > huffcode.c

mkbv.exe: mkbv.obj
	-del $@
	$(cc32) /nologo -o $@ $(CFLAGS) $*.obj

bv.c: mkbv.exe
	-del $@
	.\mkbv > $@

rgb-cube.ppm: mkcube.exe
	.\mkcube rgb > $@

yuv-map.ppm: mkcube.exe
	.\mkcube -Y $(ED_YBITS) -U 45 -V 45 yuv > $@

mkcube.exe: mkcube.obj getopt.obj
	$(cc32) /nologo -o $@ $(CFLAGS) mkcube.obj getopt.obj

ppmtolut.exe: ppmtolut.obj getopt.obj
	$(cc32) /nologo -o $@ $(CFLAGS) ppmtolut.obj getopt.obj

ppmtolut.obj: ppmtolut.c
	$(cc32) $(cflags) $(cvars) $(CFLAGS) -o $@ -FRppmtolut.sbr ppmtolut.c

p64.obj: p64/p64.cc
	$(cc32) $(cdebug) $(cflags) $(cvars) $(CFLAGS) -o $@ -FR$*.sbr -Tp p64/p64.cc

p64dump.obj: p64/p64dump.cc
	$(cc32) $(cdebug) $(cflags) $(cvars) $(CFLAGS) -o $@ -FR$*.sbr -Tp p64/p64dump.cc

jpeg.obj: jpeg/jpeg.cc
	$(cc32) $(cdebug) $(cflags) $(cvars) $(CFLAGS) -o $@ -FR$*.sbr -Tp jpeg/jpeg.cc

tk.res:
	$(rc32) -fo $@ -r -i $(GENERICDIR) $(RCDIR)/tk.rc

# qfdes doesn't like MSVC optimisations.... 
qfdes.obj: qfdes.c
	$(cc32) $(cdebug) /Od $(cflags) $(cvars) $(CFLAGS) -o $@ -FR$*.sbr -Tp qfdes.c

install: vic.exe histtolut.exe
	-mkdir "\Program Files\MBone"
	copy vic.exe "\Program Files\MBone\vic.exe"
	copy histtolut.exe "\Program Files\MBone\histtolut.exe"
	copy vic.1 "\Program Files\MBone\vic.1"

tar:	vic.exe Changes.html README version.mak
	type makefile.vc >> version.mak
	nmake -f version.mak viczip
	-del version.mak

viczip:
	-del Changes.htm
	-del Readme.txt
	copy Changes.html Changes.htm
	copy README Readme.txt
	-del vicbin-$(VICVERSION)-win95.zip
	$(ZIP) -a c:.\vicbin-$(VICVERSION)-win95.zip Readme.txt Changes.htm vic.exe
	-del Changes.htm
	-del Readme.txt

clean:
	-del *.obj
	-del *.sbr
	-del tcl2c++.exe
	-del mkbv.exe
	-del mkversion.exe
	-del bv.c
	-del mkhuff.exe
	-del mkversion.exe
	-del huffcode.c
	-del vic_tcl.c
	-del h261_play_tcl.c
	-del tmp.c
	-del vic.exe
	-del vic.bsc
	-del h261_play.exe
	-del h261_play.bsc
	-del h261_dump.exe
	-del h261_dump.bsc
	-del jpeg_play.exe
	-del jpeg_play.bsc
	-del cb_wish.exe
	-del mkcube.exe
	-del rgb-cube.ppm
	-del yuv-map.ppm
	-del cm0.c
	-del cm1.c
	-del ppmtolut.exe
	-del config.cache
	-del config.log
	-del domake.*
	-del dotar.*

realclean:
	$(MAKE) clean
	-del config.status

tags:	force
	ctags -tdwC $(SRC) *.h $(TK_DIR)/*/*.c $(TK_DIR)/*/*.h \
		$(TCL_DIR)/*/*.c $(TCL_DIR)/*/*.h

force:

depend:
	$(MKDEP) $(INCLUDES) $(DEFINE) $(SRC)

tmndec/libh263.lib:
	cd tmndec
	nmake -f makefile.mak
	cd ..

tmn-x/libh263coder.lib:
	cd tmn-x
	nmake -f makefile.mak
	cd ..
