# Microsoft Developer Studio Generated NMAKE File, Based on vic.dsp
!IF "$(CFG)" == ""
CFG=vic - Win32 Debug IPv6 Musica
!MESSAGE No configuration specified. Defaulting to vic - Win32 Debug IPv6 Musica.
!ENDIF 

!IF "$(CFG)" != "vic - Win32 Release" && "$(CFG)" != "vic - Win32 Debug IPv6" && "$(CFG)" != "vic - Win32 Debug IPv6 Musica" && "$(CFG)" != "vic - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vic.mak" CFG="vic - Win32 Debug IPv6 Musica"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vic - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vic - Win32 Debug IPv6" (based on "Win32 (x86) Application")
!MESSAGE "vic - Win32 Debug IPv6 Musica" (based on "Win32 (x86) Application")
!MESSAGE "vic - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vic - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\vic.exe" "$(OUTDIR)\vic.bsc"

!ELSE 

ALL : "tcl2c - Win32 Release" "tcl2cpp - Win32 Release" "ppmtolut - Win32 Release" "mkhuff - Win32 Release" "mkcube - Win32 Release" "mkbv - Win32 Release" "histtolut - Win32 Release" "H263v2 Enc_tmnx - Win32 Release" "H263v2 Dec_tmndec - Win32 Release" "tklib - Win32 Release" "tcllib - Win32 Release" "common - Win32 Release" "$(OUTDIR)\vic.exe" "$(OUTDIR)\vic.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"common - Win32 ReleaseCLEAN" "tcllib - Win32 ReleaseCLEAN" "tklib - Win32 ReleaseCLEAN" "H263v2 Dec_tmndec - Win32 ReleaseCLEAN" "H263v2 Enc_tmnx - Win32 ReleaseCLEAN" "histtolut - Win32 ReleaseCLEAN" "mkbv - Win32 ReleaseCLEAN" "mkcube - Win32 ReleaseCLEAN" "mkhuff - Win32 ReleaseCLEAN" "ppmtolut - Win32 ReleaseCLEAN" "tcl2cpp - Win32 ReleaseCLEAN" "tcl2c - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\assistor-list.obj"
	-@erase "$(INTDIR)\assistor-list.sbr"
	-@erase "$(INTDIR)\bitIn.obj"
	-@erase "$(INTDIR)\bitIn.sbr"
	-@erase "$(INTDIR)\bitOut.obj"
	-@erase "$(INTDIR)\bitOut.sbr"
	-@erase "$(INTDIR)\block.obj"
	-@erase "$(INTDIR)\block.sbr"
	-@erase "$(INTDIR)\bv.obj"
	-@erase "$(INTDIR)\bv.sbr"
	-@erase "$(INTDIR)\cellb_tables.obj"
	-@erase "$(INTDIR)\cellb_tables.sbr"
	-@erase "$(INTDIR)\cf-confbus.obj"
	-@erase "$(INTDIR)\cf-confbus.sbr"
	-@erase "$(INTDIR)\cf-main.obj"
	-@erase "$(INTDIR)\cf-main.sbr"
	-@erase "$(INTDIR)\cf-network.obj"
	-@erase "$(INTDIR)\cf-network.sbr"
	-@erase "$(INTDIR)\cf-tm.obj"
	-@erase "$(INTDIR)\cf-tm.sbr"
	-@erase "$(INTDIR)\cf-util.obj"
	-@erase "$(INTDIR)\cf-util.sbr"
	-@erase "$(INTDIR)\cm0.obj"
	-@erase "$(INTDIR)\cm0.sbr"
	-@erase "$(INTDIR)\cm1.obj"
	-@erase "$(INTDIR)\cm1.sbr"
	-@erase "$(INTDIR)\code.obj"
	-@erase "$(INTDIR)\code.sbr"
	-@erase "$(INTDIR)\color-dither.obj"
	-@erase "$(INTDIR)\color-dither.sbr"
	-@erase "$(INTDIR)\color-ed.obj"
	-@erase "$(INTDIR)\color-ed.sbr"
	-@erase "$(INTDIR)\color-gray.obj"
	-@erase "$(INTDIR)\color-gray.sbr"
	-@erase "$(INTDIR)\color-hi.obj"
	-@erase "$(INTDIR)\color-hi.sbr"
	-@erase "$(INTDIR)\color-hist.obj"
	-@erase "$(INTDIR)\color-hist.sbr"
	-@erase "$(INTDIR)\color-mono.obj"
	-@erase "$(INTDIR)\color-mono.sbr"
	-@erase "$(INTDIR)\color-pseudo.obj"
	-@erase "$(INTDIR)\color-pseudo.sbr"
	-@erase "$(INTDIR)\color-quant.obj"
	-@erase "$(INTDIR)\color-quant.sbr"
	-@erase "$(INTDIR)\color-true.obj"
	-@erase "$(INTDIR)\color-true.sbr"
	-@erase "$(INTDIR)\color-yuv.obj"
	-@erase "$(INTDIR)\color-yuv.sbr"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\color.sbr"
	-@erase "$(INTDIR)\communicator.obj"
	-@erase "$(INTDIR)\communicator.sbr"
	-@erase "$(INTDIR)\compositor.obj"
	-@erase "$(INTDIR)\compositor.sbr"
	-@erase "$(INTDIR)\confbus.obj"
	-@erase "$(INTDIR)\confbus.sbr"
	-@erase "$(INTDIR)\crypt-des.obj"
	-@erase "$(INTDIR)\crypt-des.sbr"
	-@erase "$(INTDIR)\crypt-dull.obj"
	-@erase "$(INTDIR)\crypt-dull.sbr"
	-@erase "$(INTDIR)\crypt.obj"
	-@erase "$(INTDIR)\crypt.sbr"
	-@erase "$(INTDIR)\dct.obj"
	-@erase "$(INTDIR)\dct.sbr"
	-@erase "$(INTDIR)\decoder-bvc.obj"
	-@erase "$(INTDIR)\decoder-bvc.sbr"
	-@erase "$(INTDIR)\decoder-cellb.obj"
	-@erase "$(INTDIR)\decoder-cellb.sbr"
	-@erase "$(INTDIR)\decoder-h261.obj"
	-@erase "$(INTDIR)\decoder-h261.sbr"
	-@erase "$(INTDIR)\decoder-h261v1.obj"
	-@erase "$(INTDIR)\decoder-h261v1.sbr"
	-@erase "$(INTDIR)\decoder-h263.obj"
	-@erase "$(INTDIR)\decoder-h263.sbr"
	-@erase "$(INTDIR)\decoder-h263v2.obj"
	-@erase "$(INTDIR)\decoder-h263v2.sbr"
	-@erase "$(INTDIR)\decoder-jpeg.obj"
	-@erase "$(INTDIR)\decoder-jpeg.sbr"
	-@erase "$(INTDIR)\decoder-nv.obj"
	-@erase "$(INTDIR)\decoder-nv.sbr"
	-@erase "$(INTDIR)\decoder-pvh.obj"
	-@erase "$(INTDIR)\decoder-pvh.sbr"
	-@erase "$(INTDIR)\decoder-raw.obj"
	-@erase "$(INTDIR)\decoder-raw.sbr"
	-@erase "$(INTDIR)\decoder.obj"
	-@erase "$(INTDIR)\decoder.sbr"
	-@erase "$(INTDIR)\device.obj"
	-@erase "$(INTDIR)\device.sbr"
	-@erase "$(INTDIR)\encoder-bvc.obj"
	-@erase "$(INTDIR)\encoder-bvc.sbr"
	-@erase "$(INTDIR)\encoder-cellb.obj"
	-@erase "$(INTDIR)\encoder-cellb.sbr"
	-@erase "$(INTDIR)\encoder-h261.obj"
	-@erase "$(INTDIR)\encoder-h261.sbr"
	-@erase "$(INTDIR)\encoder-h263.obj"
	-@erase "$(INTDIR)\encoder-h263.sbr"
	-@erase "$(INTDIR)\encoder-h263v2.obj"
	-@erase "$(INTDIR)\encoder-h263v2.sbr"
	-@erase "$(INTDIR)\encoder-jpeg.obj"
	-@erase "$(INTDIR)\encoder-jpeg.sbr"
	-@erase "$(INTDIR)\encoder-nv.obj"
	-@erase "$(INTDIR)\encoder-nv.sbr"
	-@erase "$(INTDIR)\encoder-pvh.obj"
	-@erase "$(INTDIR)\encoder-pvh.sbr"
	-@erase "$(INTDIR)\encoder-raw.obj"
	-@erase "$(INTDIR)\encoder-raw.sbr"
	-@erase "$(INTDIR)\entry.obj"
	-@erase "$(INTDIR)\entry.sbr"
	-@erase "$(INTDIR)\fdct.obj"
	-@erase "$(INTDIR)\fdct.sbr"
	-@erase "$(INTDIR)\framer-jpeg.obj"
	-@erase "$(INTDIR)\framer-jpeg.sbr"
	-@erase "$(INTDIR)\getblk.obj"
	-@erase "$(INTDIR)\getblk.sbr"
	-@erase "$(INTDIR)\getgob.obj"
	-@erase "$(INTDIR)\getgob.sbr"
	-@erase "$(INTDIR)\gethdr.obj"
	-@erase "$(INTDIR)\gethdr.sbr"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt.sbr"
	-@erase "$(INTDIR)\getvlc.obj"
	-@erase "$(INTDIR)\getvlc.sbr"
	-@erase "$(INTDIR)\grabber-still.obj"
	-@erase "$(INTDIR)\grabber-still.sbr"
	-@erase "$(INTDIR)\grabber-win32.obj"
	-@erase "$(INTDIR)\grabber-win32.sbr"
	-@erase "$(INTDIR)\grabber.obj"
	-@erase "$(INTDIR)\grabber.sbr"
	-@erase "$(INTDIR)\group-ipc.obj"
	-@erase "$(INTDIR)\group-ipc.sbr"
	-@erase "$(INTDIR)\h263dec.obj"
	-@erase "$(INTDIR)\h263dec.sbr"
	-@erase "$(INTDIR)\h263enc.obj"
	-@erase "$(INTDIR)\h263enc.sbr"
	-@erase "$(INTDIR)\h263mux.obj"
	-@erase "$(INTDIR)\h263mux.sbr"
	-@erase "$(INTDIR)\h263rtp.obj"
	-@erase "$(INTDIR)\h263rtp.sbr"
	-@erase "$(INTDIR)\huffcode.obj"
	-@erase "$(INTDIR)\huffcode.sbr"
	-@erase "$(INTDIR)\idctdec.obj"
	-@erase "$(INTDIR)\idctdec.sbr"
	-@erase "$(INTDIR)\idctenc.obj"
	-@erase "$(INTDIR)\idctenc.sbr"
	-@erase "$(INTDIR)\idlecallback.obj"
	-@erase "$(INTDIR)\idlecallback.sbr"
	-@erase "$(INTDIR)\inet.obj"
	-@erase "$(INTDIR)\inet.sbr"
	-@erase "$(INTDIR)\inet6.obj"
	-@erase "$(INTDIR)\inet6.sbr"
	-@erase "$(INTDIR)\input.obj"
	-@erase "$(INTDIR)\input.sbr"
	-@erase "$(INTDIR)\iohandler.obj"
	-@erase "$(INTDIR)\iohandler.sbr"
	-@erase "$(INTDIR)\jpeg.obj"
	-@erase "$(INTDIR)\jpeg.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\mbus_engine.obj"
	-@erase "$(INTDIR)\mbus_engine.sbr"
	-@erase "$(INTDIR)\mbus_handler.obj"
	-@erase "$(INTDIR)\mbus_handler.sbr"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5c.sbr"
	-@erase "$(INTDIR)\media-timer.obj"
	-@erase "$(INTDIR)\media-timer.sbr"
	-@erase "$(INTDIR)\module.obj"
	-@erase "$(INTDIR)\module.sbr"
	-@erase "$(INTDIR)\motion.obj"
	-@erase "$(INTDIR)\motion.sbr"
	-@erase "$(INTDIR)\net-addr.obj"
	-@erase "$(INTDIR)\net-addr.sbr"
	-@erase "$(INTDIR)\net-ip.obj"
	-@erase "$(INTDIR)\net-ip.sbr"
	-@erase "$(INTDIR)\net-ipv6.obj"
	-@erase "$(INTDIR)\net-ipv6.sbr"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\net.sbr"
	-@erase "$(INTDIR)\p64.obj"
	-@erase "$(INTDIR)\p64.sbr"
	-@erase "$(INTDIR)\pktbuf-rtp.obj"
	-@erase "$(INTDIR)\pktbuf-rtp.sbr"
	-@erase "$(INTDIR)\pktbuf.obj"
	-@erase "$(INTDIR)\pktbuf.sbr"
	-@erase "$(INTDIR)\pkttbl.obj"
	-@erase "$(INTDIR)\pkttbl.sbr"
	-@erase "$(INTDIR)\pvh-huff.obj"
	-@erase "$(INTDIR)\pvh-huff.sbr"
	-@erase "$(INTDIR)\random.obj"
	-@erase "$(INTDIR)\random.sbr"
	-@erase "$(INTDIR)\rate-variable.obj"
	-@erase "$(INTDIR)\rate-variable.sbr"
	-@erase "$(INTDIR)\recon.obj"
	-@erase "$(INTDIR)\recon.sbr"
	-@erase "$(INTDIR)\reconh263.obj"
	-@erase "$(INTDIR)\reconh263.sbr"
	-@erase "$(INTDIR)\renderer-window.obj"
	-@erase "$(INTDIR)\renderer-window.sbr"
	-@erase "$(INTDIR)\renderer.obj"
	-@erase "$(INTDIR)\renderer.sbr"
	-@erase "$(INTDIR)\rgb-converter.obj"
	-@erase "$(INTDIR)\rgb-converter.sbr"
	-@erase "$(INTDIR)\sac.obj"
	-@erase "$(INTDIR)\sac.sbr"
	-@erase "$(INTDIR)\session.obj"
	-@erase "$(INTDIR)\session.sbr"
	-@erase "$(INTDIR)\source.obj"
	-@erase "$(INTDIR)\source.sbr"
	-@erase "$(INTDIR)\strtol.obj"
	-@erase "$(INTDIR)\strtol.sbr"
	-@erase "$(INTDIR)\strtoul.obj"
	-@erase "$(INTDIR)\strtoul.sbr"
	-@erase "$(INTDIR)\Tcl.obj"
	-@erase "$(INTDIR)\Tcl.sbr"
	-@erase "$(INTDIR)\Tcl2.obj"
	-@erase "$(INTDIR)\Tcl2.sbr"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\timer.sbr"
	-@erase "$(INTDIR)\tkerror.obj"
	-@erase "$(INTDIR)\tkerror.sbr"
	-@erase "$(INTDIR)\tkStripchart.obj"
	-@erase "$(INTDIR)\tkStripchart.sbr"
	-@erase "$(INTDIR)\tkWinColor.obj"
	-@erase "$(INTDIR)\tkWinColor.sbr"
	-@erase "$(INTDIR)\transcoder-jpeg.obj"
	-@erase "$(INTDIR)\transcoder-jpeg.sbr"
	-@erase "$(INTDIR)\transmitter.obj"
	-@erase "$(INTDIR)\transmitter.sbr"
	-@erase "$(INTDIR)\ui-ctrlmenu.obj"
	-@erase "$(INTDIR)\ui-ctrlmenu.sbr"
	-@erase "$(INTDIR)\ui-extout.obj"
	-@erase "$(INTDIR)\ui-extout.sbr"
	-@erase "$(INTDIR)\ui-grabber.obj"
	-@erase "$(INTDIR)\ui-grabber.sbr"
	-@erase "$(INTDIR)\ui-help.obj"
	-@erase "$(INTDIR)\ui-help.sbr"
	-@erase "$(INTDIR)\ui-main.obj"
	-@erase "$(INTDIR)\ui-main.sbr"
	-@erase "$(INTDIR)\ui-relate.obj"
	-@erase "$(INTDIR)\ui-relate.sbr"
	-@erase "$(INTDIR)\ui-resource.obj"
	-@erase "$(INTDIR)\ui-resource.sbr"
	-@erase "$(INTDIR)\ui-srclist.obj"
	-@erase "$(INTDIR)\ui-srclist.sbr"
	-@erase "$(INTDIR)\ui-stats.obj"
	-@erase "$(INTDIR)\ui-stats.sbr"
	-@erase "$(INTDIR)\ui-switcher.obj"
	-@erase "$(INTDIR)\ui-switcher.sbr"
	-@erase "$(INTDIR)\ui-util.obj"
	-@erase "$(INTDIR)\ui-util.sbr"
	-@erase "$(INTDIR)\ui-win32.obj"
	-@erase "$(INTDIR)\ui-win32.sbr"
	-@erase "$(INTDIR)\ui-windows.obj"
	-@erase "$(INTDIR)\ui-windows.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(INTDIR)\vw.obj"
	-@erase "$(INTDIR)\vw.sbr"
	-@erase "$(INTDIR)\win32.obj"
	-@erase "$(INTDIR)\win32.sbr"
	-@erase "$(INTDIR)\win32X.obj"
	-@erase "$(INTDIR)\win32X.sbr"
	-@erase "$(OUTDIR)\vic.bsc"
	-@erase "$(OUTDIR)\vic.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

vic=rc.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /D "NDEBUG" /D "_WINDOWS" /D "SASR" /D "WIN32" /D "HAVE_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vic.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cf-confbus.sbr" \
	"$(INTDIR)\cf-main.sbr" \
	"$(INTDIR)\cf-network.sbr" \
	"$(INTDIR)\cf-tm.sbr" \
	"$(INTDIR)\cf-util.sbr" \
	"$(INTDIR)\entry.sbr" \
	"$(INTDIR)\tkerror.sbr" \
	"$(INTDIR)\ui-ctrlmenu.sbr" \
	"$(INTDIR)\ui-extout.sbr" \
	"$(INTDIR)\ui-grabber.sbr" \
	"$(INTDIR)\ui-help.sbr" \
	"$(INTDIR)\ui-main.sbr" \
	"$(INTDIR)\ui-relate.sbr" \
	"$(INTDIR)\ui-resource.sbr" \
	"$(INTDIR)\ui-srclist.sbr" \
	"$(INTDIR)\ui-stats.sbr" \
	"$(INTDIR)\ui-switcher.sbr" \
	"$(INTDIR)\ui-util.sbr" \
	"$(INTDIR)\ui-win32.sbr" \
	"$(INTDIR)\ui-windows.sbr" \
	"$(INTDIR)\communicator.sbr" \
	"$(INTDIR)\confbus.sbr" \
	"$(INTDIR)\crypt-des.sbr" \
	"$(INTDIR)\crypt-dull.sbr" \
	"$(INTDIR)\crypt.sbr" \
	"$(INTDIR)\group-ipc.sbr" \
	"$(INTDIR)\inet.sbr" \
	"$(INTDIR)\inet6.sbr" \
	"$(INTDIR)\mbus_engine.sbr" \
	"$(INTDIR)\mbus_handler.sbr" \
	"$(INTDIR)\net-addr.sbr" \
	"$(INTDIR)\net-ip.sbr" \
	"$(INTDIR)\net-ipv6.sbr" \
	"$(INTDIR)\net.sbr" \
	"$(INTDIR)\pktbuf.sbr" \
	"$(INTDIR)\pkttbl.sbr" \
	"$(INTDIR)\bitIn.sbr" \
	"$(INTDIR)\bitOut.sbr" \
	"$(INTDIR)\block.sbr" \
	"$(INTDIR)\code.sbr" \
	"$(INTDIR)\fdct.sbr" \
	"$(INTDIR)\getblk.sbr" \
	"$(INTDIR)\getgob.sbr" \
	"$(INTDIR)\gethdr.sbr" \
	"$(INTDIR)\getvlc.sbr" \
	"$(INTDIR)\h263dec.sbr" \
	"$(INTDIR)\h263enc.sbr" \
	"$(INTDIR)\h263mux.sbr" \
	"$(INTDIR)\h263rtp.sbr" \
	"$(INTDIR)\idctdec.sbr" \
	"$(INTDIR)\idctenc.sbr" \
	"$(INTDIR)\input.sbr" \
	"$(INTDIR)\motion.sbr" \
	"$(INTDIR)\recon.sbr" \
	"$(INTDIR)\reconh263.sbr" \
	"$(INTDIR)\sac.sbr" \
	"$(INTDIR)\bv.sbr" \
	"$(INTDIR)\cellb_tables.sbr" \
	"$(INTDIR)\compositor.sbr" \
	"$(INTDIR)\dct.sbr" \
	"$(INTDIR)\decoder-bvc.sbr" \
	"$(INTDIR)\decoder-cellb.sbr" \
	"$(INTDIR)\decoder-h261.sbr" \
	"$(INTDIR)\decoder-h261v1.sbr" \
	"$(INTDIR)\decoder-h263.sbr" \
	"$(INTDIR)\decoder-h263v2.sbr" \
	"$(INTDIR)\decoder-jpeg.sbr" \
	"$(INTDIR)\decoder-nv.sbr" \
	"$(INTDIR)\decoder-pvh.sbr" \
	"$(INTDIR)\decoder-raw.sbr" \
	"$(INTDIR)\decoder.sbr" \
	"$(INTDIR)\encoder-bvc.sbr" \
	"$(INTDIR)\encoder-cellb.sbr" \
	"$(INTDIR)\encoder-h261.sbr" \
	"$(INTDIR)\encoder-h263.sbr" \
	"$(INTDIR)\encoder-h263v2.sbr" \
	"$(INTDIR)\encoder-jpeg.sbr" \
	"$(INTDIR)\encoder-nv.sbr" \
	"$(INTDIR)\encoder-pvh.sbr" \
	"$(INTDIR)\encoder-raw.sbr" \
	"$(INTDIR)\framer-jpeg.sbr" \
	"$(INTDIR)\huffcode.sbr" \
	"$(INTDIR)\jpeg.sbr" \
	"$(INTDIR)\p64.sbr" \
	"$(INTDIR)\pvh-huff.sbr" \
	"$(INTDIR)\transcoder-jpeg.sbr" \
	"$(INTDIR)\cm0.sbr" \
	"$(INTDIR)\cm1.sbr" \
	"$(INTDIR)\color-dither.sbr" \
	"$(INTDIR)\color-ed.sbr" \
	"$(INTDIR)\color-gray.sbr" \
	"$(INTDIR)\color-hi.sbr" \
	"$(INTDIR)\color-hist.sbr" \
	"$(INTDIR)\color-mono.sbr" \
	"$(INTDIR)\color-pseudo.sbr" \
	"$(INTDIR)\color-quant.sbr" \
	"$(INTDIR)\color-true.sbr" \
	"$(INTDIR)\color-yuv.sbr" \
	"$(INTDIR)\color.sbr" \
	"$(INTDIR)\renderer-window.sbr" \
	"$(INTDIR)\renderer.sbr" \
	"$(INTDIR)\rgb-converter.sbr" \
	"$(INTDIR)\vw.sbr" \
	"$(INTDIR)\pktbuf-rtp.sbr" \
	"$(INTDIR)\session.sbr" \
	"$(INTDIR)\source.sbr" \
	"$(INTDIR)\transmitter.sbr" \
	"$(INTDIR)\win32.sbr" \
	"$(INTDIR)\win32X.sbr" \
	"$(INTDIR)\getopt.sbr" \
	"$(INTDIR)\idlecallback.sbr" \
	"$(INTDIR)\iohandler.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\md5c.sbr" \
	"$(INTDIR)\media-timer.sbr" \
	"$(INTDIR)\module.sbr" \
	"$(INTDIR)\random.sbr" \
	"$(INTDIR)\rate-variable.sbr" \
	"$(INTDIR)\strtol.sbr" \
	"$(INTDIR)\strtoul.sbr" \
	"$(INTDIR)\Tcl.sbr" \
	"$(INTDIR)\Tcl2.sbr" \
	"$(INTDIR)\timer.sbr" \
	"$(INTDIR)\tkStripchart.sbr" \
	"$(INTDIR)\tkWinColor.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\assistor-list.sbr" \
	"$(INTDIR)\device.sbr" \
	"$(INTDIR)\grabber-still.sbr" \
	"$(INTDIR)\grabber-win32.sbr" \
	"$(INTDIR)\grabber.sbr"

"$(OUTDIR)\vic.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=tklib.lib tcllib.lib uclmm.lib libh263.lib libh263coder.lib wsock32.lib Ws2_32.lib winmm.lib kernel32.lib user32.lib gdi32.lib shell32.lib vfw32.lib advapi32.lib comdlg32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\vic.pdb" /machine:I386 /out:"$(OUTDIR)\vic.exe" /libpath:"..\common\Release" /libpath:"..\tcl-8.0\win\Release" /libpath:"..\tk-8.0\win\Release" /libpath:"codec\tmndec" /libpath:"codec\tmn-x" 
LINK32_OBJS= \
	"$(INTDIR)\cf-confbus.obj" \
	"$(INTDIR)\cf-main.obj" \
	"$(INTDIR)\cf-network.obj" \
	"$(INTDIR)\cf-tm.obj" \
	"$(INTDIR)\cf-util.obj" \
	"$(INTDIR)\entry.obj" \
	"$(INTDIR)\tkerror.obj" \
	"$(INTDIR)\ui-ctrlmenu.obj" \
	"$(INTDIR)\ui-extout.obj" \
	"$(INTDIR)\ui-grabber.obj" \
	"$(INTDIR)\ui-help.obj" \
	"$(INTDIR)\ui-main.obj" \
	"$(INTDIR)\ui-relate.obj" \
	"$(INTDIR)\ui-resource.obj" \
	"$(INTDIR)\ui-srclist.obj" \
	"$(INTDIR)\ui-stats.obj" \
	"$(INTDIR)\ui-switcher.obj" \
	"$(INTDIR)\ui-util.obj" \
	"$(INTDIR)\ui-win32.obj" \
	"$(INTDIR)\ui-windows.obj" \
	"$(INTDIR)\communicator.obj" \
	"$(INTDIR)\confbus.obj" \
	"$(INTDIR)\crypt-des.obj" \
	"$(INTDIR)\crypt-dull.obj" \
	"$(INTDIR)\crypt.obj" \
	"$(INTDIR)\group-ipc.obj" \
	"$(INTDIR)\inet.obj" \
	"$(INTDIR)\inet6.obj" \
	"$(INTDIR)\mbus_engine.obj" \
	"$(INTDIR)\mbus_handler.obj" \
	"$(INTDIR)\net-addr.obj" \
	"$(INTDIR)\net-ip.obj" \
	"$(INTDIR)\net-ipv6.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\pktbuf.obj" \
	"$(INTDIR)\pkttbl.obj" \
	"$(INTDIR)\bitIn.obj" \
	"$(INTDIR)\bitOut.obj" \
	"$(INTDIR)\block.obj" \
	"$(INTDIR)\code.obj" \
	"$(INTDIR)\fdct.obj" \
	"$(INTDIR)\getblk.obj" \
	"$(INTDIR)\getgob.obj" \
	"$(INTDIR)\gethdr.obj" \
	"$(INTDIR)\getvlc.obj" \
	"$(INTDIR)\h263dec.obj" \
	"$(INTDIR)\h263enc.obj" \
	"$(INTDIR)\h263mux.obj" \
	"$(INTDIR)\h263rtp.obj" \
	"$(INTDIR)\idctdec.obj" \
	"$(INTDIR)\idctenc.obj" \
	"$(INTDIR)\input.obj" \
	"$(INTDIR)\motion.obj" \
	"$(INTDIR)\recon.obj" \
	"$(INTDIR)\reconh263.obj" \
	"$(INTDIR)\sac.obj" \
	"$(INTDIR)\bv.obj" \
	"$(INTDIR)\cellb_tables.obj" \
	"$(INTDIR)\compositor.obj" \
	"$(INTDIR)\dct.obj" \
	"$(INTDIR)\decoder-bvc.obj" \
	"$(INTDIR)\decoder-cellb.obj" \
	"$(INTDIR)\decoder-h261.obj" \
	"$(INTDIR)\decoder-h261v1.obj" \
	"$(INTDIR)\decoder-h263.obj" \
	"$(INTDIR)\decoder-h263v2.obj" \
	"$(INTDIR)\decoder-jpeg.obj" \
	"$(INTDIR)\decoder-nv.obj" \
	"$(INTDIR)\decoder-pvh.obj" \
	"$(INTDIR)\decoder-raw.obj" \
	"$(INTDIR)\decoder.obj" \
	"$(INTDIR)\encoder-bvc.obj" \
	"$(INTDIR)\encoder-cellb.obj" \
	"$(INTDIR)\encoder-h261.obj" \
	"$(INTDIR)\encoder-h263.obj" \
	"$(INTDIR)\encoder-h263v2.obj" \
	"$(INTDIR)\encoder-jpeg.obj" \
	"$(INTDIR)\encoder-nv.obj" \
	"$(INTDIR)\encoder-pvh.obj" \
	"$(INTDIR)\encoder-raw.obj" \
	"$(INTDIR)\framer-jpeg.obj" \
	"$(INTDIR)\huffcode.obj" \
	"$(INTDIR)\jpeg.obj" \
	"$(INTDIR)\p64.obj" \
	"$(INTDIR)\pvh-huff.obj" \
	"$(INTDIR)\transcoder-jpeg.obj" \
	"$(INTDIR)\cm0.obj" \
	"$(INTDIR)\cm1.obj" \
	"$(INTDIR)\color-dither.obj" \
	"$(INTDIR)\color-ed.obj" \
	"$(INTDIR)\color-gray.obj" \
	"$(INTDIR)\color-hi.obj" \
	"$(INTDIR)\color-hist.obj" \
	"$(INTDIR)\color-mono.obj" \
	"$(INTDIR)\color-pseudo.obj" \
	"$(INTDIR)\color-quant.obj" \
	"$(INTDIR)\color-true.obj" \
	"$(INTDIR)\color-yuv.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\renderer-window.obj" \
	"$(INTDIR)\renderer.obj" \
	"$(INTDIR)\rgb-converter.obj" \
	"$(INTDIR)\vw.obj" \
	"$(INTDIR)\pktbuf-rtp.obj" \
	"$(INTDIR)\session.obj" \
	"$(INTDIR)\source.obj" \
	"$(INTDIR)\transmitter.obj" \
	"$(INTDIR)\win32.obj" \
	"$(INTDIR)\win32X.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\idlecallback.obj" \
	"$(INTDIR)\iohandler.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\media-timer.obj" \
	"$(INTDIR)\module.obj" \
	"$(INTDIR)\random.obj" \
	"$(INTDIR)\rate-variable.obj" \
	"$(INTDIR)\strtol.obj" \
	"$(INTDIR)\strtoul.obj" \
	"$(INTDIR)\Tcl.obj" \
	"$(INTDIR)\Tcl2.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\tkStripchart.obj" \
	"$(INTDIR)\tkWinColor.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\assistor-list.obj" \
	"$(INTDIR)\device.obj" \
	"$(INTDIR)\grabber-still.obj" \
	"$(INTDIR)\grabber-win32.obj" \
	"$(INTDIR)\grabber.obj" \
	"..\tk-8.0\win\tk.res" \
	"..\common\Release\uclmm.lib" \
	"..\tcl-8.0\win\Release\tcllib.lib" \
	"..\tk-8.0\win\Release\tklib.lib" \
	".\codec\tmndec\libh263.lib" \
	".\codec\tmn-x\libh263coder.lib"

"$(OUTDIR)\vic.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

OUTDIR=.\Debug_IPv6
INTDIR=.\Debug_IPv6
# Begin Custom Macros
OutDir=.\Debug_IPv6
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\vic.exe" "$(OUTDIR)\vic.bsc"

!ELSE 

ALL : "tklib - Win32 Debug IPv6" "tcllib - Win32 Debug IPv6" "common - Win32 Debug IPv6" "$(OUTDIR)\vic.exe" "$(OUTDIR)\vic.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"common - Win32 Debug IPv6CLEAN" "tcllib - Win32 Debug IPv6CLEAN" "tklib - Win32 Debug IPv6CLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\assistor-list.obj"
	-@erase "$(INTDIR)\assistor-list.sbr"
	-@erase "$(INTDIR)\bitIn.obj"
	-@erase "$(INTDIR)\bitIn.sbr"
	-@erase "$(INTDIR)\bitOut.obj"
	-@erase "$(INTDIR)\bitOut.sbr"
	-@erase "$(INTDIR)\block.obj"
	-@erase "$(INTDIR)\block.sbr"
	-@erase "$(INTDIR)\bv.obj"
	-@erase "$(INTDIR)\bv.sbr"
	-@erase "$(INTDIR)\cellb_tables.obj"
	-@erase "$(INTDIR)\cellb_tables.sbr"
	-@erase "$(INTDIR)\cf-confbus.obj"
	-@erase "$(INTDIR)\cf-confbus.sbr"
	-@erase "$(INTDIR)\cf-main.obj"
	-@erase "$(INTDIR)\cf-main.sbr"
	-@erase "$(INTDIR)\cf-network.obj"
	-@erase "$(INTDIR)\cf-network.sbr"
	-@erase "$(INTDIR)\cf-tm.obj"
	-@erase "$(INTDIR)\cf-tm.sbr"
	-@erase "$(INTDIR)\cf-util.obj"
	-@erase "$(INTDIR)\cf-util.sbr"
	-@erase "$(INTDIR)\cm0.obj"
	-@erase "$(INTDIR)\cm0.sbr"
	-@erase "$(INTDIR)\cm1.obj"
	-@erase "$(INTDIR)\cm1.sbr"
	-@erase "$(INTDIR)\code.obj"
	-@erase "$(INTDIR)\code.sbr"
	-@erase "$(INTDIR)\color-dither.obj"
	-@erase "$(INTDIR)\color-dither.sbr"
	-@erase "$(INTDIR)\color-ed.obj"
	-@erase "$(INTDIR)\color-ed.sbr"
	-@erase "$(INTDIR)\color-gray.obj"
	-@erase "$(INTDIR)\color-gray.sbr"
	-@erase "$(INTDIR)\color-hi.obj"
	-@erase "$(INTDIR)\color-hi.sbr"
	-@erase "$(INTDIR)\color-hist.obj"
	-@erase "$(INTDIR)\color-hist.sbr"
	-@erase "$(INTDIR)\color-mono.obj"
	-@erase "$(INTDIR)\color-mono.sbr"
	-@erase "$(INTDIR)\color-pseudo.obj"
	-@erase "$(INTDIR)\color-pseudo.sbr"
	-@erase "$(INTDIR)\color-quant.obj"
	-@erase "$(INTDIR)\color-quant.sbr"
	-@erase "$(INTDIR)\color-true.obj"
	-@erase "$(INTDIR)\color-true.sbr"
	-@erase "$(INTDIR)\color-yuv.obj"
	-@erase "$(INTDIR)\color-yuv.sbr"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\color.sbr"
	-@erase "$(INTDIR)\communicator.obj"
	-@erase "$(INTDIR)\communicator.sbr"
	-@erase "$(INTDIR)\compositor.obj"
	-@erase "$(INTDIR)\compositor.sbr"
	-@erase "$(INTDIR)\confbus.obj"
	-@erase "$(INTDIR)\confbus.sbr"
	-@erase "$(INTDIR)\crypt-des.obj"
	-@erase "$(INTDIR)\crypt-des.sbr"
	-@erase "$(INTDIR)\crypt-dull.obj"
	-@erase "$(INTDIR)\crypt-dull.sbr"
	-@erase "$(INTDIR)\crypt.obj"
	-@erase "$(INTDIR)\crypt.sbr"
	-@erase "$(INTDIR)\dct.obj"
	-@erase "$(INTDIR)\dct.sbr"
	-@erase "$(INTDIR)\decoder-bvc.obj"
	-@erase "$(INTDIR)\decoder-bvc.sbr"
	-@erase "$(INTDIR)\decoder-cellb.obj"
	-@erase "$(INTDIR)\decoder-cellb.sbr"
	-@erase "$(INTDIR)\decoder-h261.obj"
	-@erase "$(INTDIR)\decoder-h261.sbr"
	-@erase "$(INTDIR)\decoder-h261v1.obj"
	-@erase "$(INTDIR)\decoder-h261v1.sbr"
	-@erase "$(INTDIR)\decoder-h263.obj"
	-@erase "$(INTDIR)\decoder-h263.sbr"
	-@erase "$(INTDIR)\decoder-h263v2.obj"
	-@erase "$(INTDIR)\decoder-h263v2.sbr"
	-@erase "$(INTDIR)\decoder-jpeg.obj"
	-@erase "$(INTDIR)\decoder-jpeg.sbr"
	-@erase "$(INTDIR)\decoder-nv.obj"
	-@erase "$(INTDIR)\decoder-nv.sbr"
	-@erase "$(INTDIR)\decoder-pvh.obj"
	-@erase "$(INTDIR)\decoder-pvh.sbr"
	-@erase "$(INTDIR)\decoder-raw.obj"
	-@erase "$(INTDIR)\decoder-raw.sbr"
	-@erase "$(INTDIR)\decoder.obj"
	-@erase "$(INTDIR)\decoder.sbr"
	-@erase "$(INTDIR)\device.obj"
	-@erase "$(INTDIR)\device.sbr"
	-@erase "$(INTDIR)\encoder-bvc.obj"
	-@erase "$(INTDIR)\encoder-bvc.sbr"
	-@erase "$(INTDIR)\encoder-cellb.obj"
	-@erase "$(INTDIR)\encoder-cellb.sbr"
	-@erase "$(INTDIR)\encoder-h261.obj"
	-@erase "$(INTDIR)\encoder-h261.sbr"
	-@erase "$(INTDIR)\encoder-h263.obj"
	-@erase "$(INTDIR)\encoder-h263.sbr"
	-@erase "$(INTDIR)\encoder-h263v2.obj"
	-@erase "$(INTDIR)\encoder-h263v2.sbr"
	-@erase "$(INTDIR)\encoder-jpeg.obj"
	-@erase "$(INTDIR)\encoder-jpeg.sbr"
	-@erase "$(INTDIR)\encoder-nv.obj"
	-@erase "$(INTDIR)\encoder-nv.sbr"
	-@erase "$(INTDIR)\encoder-pvh.obj"
	-@erase "$(INTDIR)\encoder-pvh.sbr"
	-@erase "$(INTDIR)\encoder-raw.obj"
	-@erase "$(INTDIR)\encoder-raw.sbr"
	-@erase "$(INTDIR)\entry.obj"
	-@erase "$(INTDIR)\entry.sbr"
	-@erase "$(INTDIR)\fdct.obj"
	-@erase "$(INTDIR)\fdct.sbr"
	-@erase "$(INTDIR)\framer-jpeg.obj"
	-@erase "$(INTDIR)\framer-jpeg.sbr"
	-@erase "$(INTDIR)\getblk.obj"
	-@erase "$(INTDIR)\getblk.sbr"
	-@erase "$(INTDIR)\getgob.obj"
	-@erase "$(INTDIR)\getgob.sbr"
	-@erase "$(INTDIR)\gethdr.obj"
	-@erase "$(INTDIR)\gethdr.sbr"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt.sbr"
	-@erase "$(INTDIR)\getvlc.obj"
	-@erase "$(INTDIR)\getvlc.sbr"
	-@erase "$(INTDIR)\grabber-still.obj"
	-@erase "$(INTDIR)\grabber-still.sbr"
	-@erase "$(INTDIR)\grabber-win32.obj"
	-@erase "$(INTDIR)\grabber-win32.sbr"
	-@erase "$(INTDIR)\grabber.obj"
	-@erase "$(INTDIR)\grabber.sbr"
	-@erase "$(INTDIR)\group-ipc.obj"
	-@erase "$(INTDIR)\group-ipc.sbr"
	-@erase "$(INTDIR)\h263dec.obj"
	-@erase "$(INTDIR)\h263dec.sbr"
	-@erase "$(INTDIR)\h263enc.obj"
	-@erase "$(INTDIR)\h263enc.sbr"
	-@erase "$(INTDIR)\h263mux.obj"
	-@erase "$(INTDIR)\h263mux.sbr"
	-@erase "$(INTDIR)\h263rtp.obj"
	-@erase "$(INTDIR)\h263rtp.sbr"
	-@erase "$(INTDIR)\huffcode.obj"
	-@erase "$(INTDIR)\huffcode.sbr"
	-@erase "$(INTDIR)\idctdec.obj"
	-@erase "$(INTDIR)\idctdec.sbr"
	-@erase "$(INTDIR)\idctenc.obj"
	-@erase "$(INTDIR)\idctenc.sbr"
	-@erase "$(INTDIR)\idlecallback.obj"
	-@erase "$(INTDIR)\idlecallback.sbr"
	-@erase "$(INTDIR)\inet.obj"
	-@erase "$(INTDIR)\inet.sbr"
	-@erase "$(INTDIR)\inet6.obj"
	-@erase "$(INTDIR)\inet6.sbr"
	-@erase "$(INTDIR)\input.obj"
	-@erase "$(INTDIR)\input.sbr"
	-@erase "$(INTDIR)\iohandler.obj"
	-@erase "$(INTDIR)\iohandler.sbr"
	-@erase "$(INTDIR)\jpeg.obj"
	-@erase "$(INTDIR)\jpeg.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\mbus_engine.obj"
	-@erase "$(INTDIR)\mbus_engine.sbr"
	-@erase "$(INTDIR)\mbus_handler.obj"
	-@erase "$(INTDIR)\mbus_handler.sbr"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5c.sbr"
	-@erase "$(INTDIR)\media-timer.obj"
	-@erase "$(INTDIR)\media-timer.sbr"
	-@erase "$(INTDIR)\module.obj"
	-@erase "$(INTDIR)\module.sbr"
	-@erase "$(INTDIR)\motion.obj"
	-@erase "$(INTDIR)\motion.sbr"
	-@erase "$(INTDIR)\net-addr.obj"
	-@erase "$(INTDIR)\net-addr.sbr"
	-@erase "$(INTDIR)\net-ip.obj"
	-@erase "$(INTDIR)\net-ip.sbr"
	-@erase "$(INTDIR)\net-ipv6.obj"
	-@erase "$(INTDIR)\net-ipv6.sbr"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\net.sbr"
	-@erase "$(INTDIR)\p64.obj"
	-@erase "$(INTDIR)\p64.sbr"
	-@erase "$(INTDIR)\pktbuf-rtp.obj"
	-@erase "$(INTDIR)\pktbuf-rtp.sbr"
	-@erase "$(INTDIR)\pktbuf.obj"
	-@erase "$(INTDIR)\pktbuf.sbr"
	-@erase "$(INTDIR)\pkttbl.obj"
	-@erase "$(INTDIR)\pkttbl.sbr"
	-@erase "$(INTDIR)\pvh-huff.obj"
	-@erase "$(INTDIR)\pvh-huff.sbr"
	-@erase "$(INTDIR)\random.obj"
	-@erase "$(INTDIR)\random.sbr"
	-@erase "$(INTDIR)\rate-variable.obj"
	-@erase "$(INTDIR)\rate-variable.sbr"
	-@erase "$(INTDIR)\recon.obj"
	-@erase "$(INTDIR)\recon.sbr"
	-@erase "$(INTDIR)\reconh263.obj"
	-@erase "$(INTDIR)\reconh263.sbr"
	-@erase "$(INTDIR)\renderer-window.obj"
	-@erase "$(INTDIR)\renderer-window.sbr"
	-@erase "$(INTDIR)\renderer.obj"
	-@erase "$(INTDIR)\renderer.sbr"
	-@erase "$(INTDIR)\rgb-converter.obj"
	-@erase "$(INTDIR)\rgb-converter.sbr"
	-@erase "$(INTDIR)\sac.obj"
	-@erase "$(INTDIR)\sac.sbr"
	-@erase "$(INTDIR)\session.obj"
	-@erase "$(INTDIR)\session.sbr"
	-@erase "$(INTDIR)\source.obj"
	-@erase "$(INTDIR)\source.sbr"
	-@erase "$(INTDIR)\strtol.obj"
	-@erase "$(INTDIR)\strtol.sbr"
	-@erase "$(INTDIR)\strtoul.obj"
	-@erase "$(INTDIR)\strtoul.sbr"
	-@erase "$(INTDIR)\Tcl.obj"
	-@erase "$(INTDIR)\Tcl.sbr"
	-@erase "$(INTDIR)\Tcl2.obj"
	-@erase "$(INTDIR)\Tcl2.sbr"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\timer.sbr"
	-@erase "$(INTDIR)\tkerror.obj"
	-@erase "$(INTDIR)\tkerror.sbr"
	-@erase "$(INTDIR)\tkStripchart.obj"
	-@erase "$(INTDIR)\tkStripchart.sbr"
	-@erase "$(INTDIR)\tkWinColor.obj"
	-@erase "$(INTDIR)\tkWinColor.sbr"
	-@erase "$(INTDIR)\transcoder-jpeg.obj"
	-@erase "$(INTDIR)\transcoder-jpeg.sbr"
	-@erase "$(INTDIR)\transmitter.obj"
	-@erase "$(INTDIR)\transmitter.sbr"
	-@erase "$(INTDIR)\ui-ctrlmenu.obj"
	-@erase "$(INTDIR)\ui-ctrlmenu.sbr"
	-@erase "$(INTDIR)\ui-extout.obj"
	-@erase "$(INTDIR)\ui-extout.sbr"
	-@erase "$(INTDIR)\ui-grabber.obj"
	-@erase "$(INTDIR)\ui-grabber.sbr"
	-@erase "$(INTDIR)\ui-help.obj"
	-@erase "$(INTDIR)\ui-help.sbr"
	-@erase "$(INTDIR)\ui-main.obj"
	-@erase "$(INTDIR)\ui-main.sbr"
	-@erase "$(INTDIR)\ui-relate.obj"
	-@erase "$(INTDIR)\ui-relate.sbr"
	-@erase "$(INTDIR)\ui-resource.obj"
	-@erase "$(INTDIR)\ui-resource.sbr"
	-@erase "$(INTDIR)\ui-srclist.obj"
	-@erase "$(INTDIR)\ui-srclist.sbr"
	-@erase "$(INTDIR)\ui-stats.obj"
	-@erase "$(INTDIR)\ui-stats.sbr"
	-@erase "$(INTDIR)\ui-switcher.obj"
	-@erase "$(INTDIR)\ui-switcher.sbr"
	-@erase "$(INTDIR)\ui-util.obj"
	-@erase "$(INTDIR)\ui-util.sbr"
	-@erase "$(INTDIR)\ui-win32.obj"
	-@erase "$(INTDIR)\ui-win32.sbr"
	-@erase "$(INTDIR)\ui-windows.obj"
	-@erase "$(INTDIR)\ui-windows.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(INTDIR)\vw.obj"
	-@erase "$(INTDIR)\vw.sbr"
	-@erase "$(INTDIR)\win32.obj"
	-@erase "$(INTDIR)\win32.sbr"
	-@erase "$(INTDIR)\win32X.obj"
	-@erase "$(INTDIR)\win32X.sbr"
	-@erase "$(OUTDIR)\vic.bsc"
	-@erase "$(OUTDIR)\vic.exe"
	-@erase "$(OUTDIR)\vic.ilk"
	-@erase "$(OUTDIR)\vic.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

vic=rc.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "..\IPv6Kit\inc" /I "g:\DDK\inc" /D "DEBUG" /D "_DEBUG" /D "DEBUG_MEM" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vic.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cf-confbus.sbr" \
	"$(INTDIR)\cf-main.sbr" \
	"$(INTDIR)\cf-network.sbr" \
	"$(INTDIR)\cf-tm.sbr" \
	"$(INTDIR)\cf-util.sbr" \
	"$(INTDIR)\entry.sbr" \
	"$(INTDIR)\tkerror.sbr" \
	"$(INTDIR)\ui-ctrlmenu.sbr" \
	"$(INTDIR)\ui-extout.sbr" \
	"$(INTDIR)\ui-grabber.sbr" \
	"$(INTDIR)\ui-help.sbr" \
	"$(INTDIR)\ui-main.sbr" \
	"$(INTDIR)\ui-relate.sbr" \
	"$(INTDIR)\ui-resource.sbr" \
	"$(INTDIR)\ui-srclist.sbr" \
	"$(INTDIR)\ui-stats.sbr" \
	"$(INTDIR)\ui-switcher.sbr" \
	"$(INTDIR)\ui-util.sbr" \
	"$(INTDIR)\ui-win32.sbr" \
	"$(INTDIR)\ui-windows.sbr" \
	"$(INTDIR)\communicator.sbr" \
	"$(INTDIR)\confbus.sbr" \
	"$(INTDIR)\crypt-des.sbr" \
	"$(INTDIR)\crypt-dull.sbr" \
	"$(INTDIR)\crypt.sbr" \
	"$(INTDIR)\group-ipc.sbr" \
	"$(INTDIR)\inet.sbr" \
	"$(INTDIR)\inet6.sbr" \
	"$(INTDIR)\mbus_engine.sbr" \
	"$(INTDIR)\mbus_handler.sbr" \
	"$(INTDIR)\net-addr.sbr" \
	"$(INTDIR)\net-ip.sbr" \
	"$(INTDIR)\net-ipv6.sbr" \
	"$(INTDIR)\net.sbr" \
	"$(INTDIR)\pktbuf.sbr" \
	"$(INTDIR)\pkttbl.sbr" \
	"$(INTDIR)\bitIn.sbr" \
	"$(INTDIR)\bitOut.sbr" \
	"$(INTDIR)\block.sbr" \
	"$(INTDIR)\code.sbr" \
	"$(INTDIR)\fdct.sbr" \
	"$(INTDIR)\getblk.sbr" \
	"$(INTDIR)\getgob.sbr" \
	"$(INTDIR)\gethdr.sbr" \
	"$(INTDIR)\getvlc.sbr" \
	"$(INTDIR)\h263dec.sbr" \
	"$(INTDIR)\h263enc.sbr" \
	"$(INTDIR)\h263mux.sbr" \
	"$(INTDIR)\h263rtp.sbr" \
	"$(INTDIR)\idctdec.sbr" \
	"$(INTDIR)\idctenc.sbr" \
	"$(INTDIR)\input.sbr" \
	"$(INTDIR)\motion.sbr" \
	"$(INTDIR)\recon.sbr" \
	"$(INTDIR)\reconh263.sbr" \
	"$(INTDIR)\sac.sbr" \
	"$(INTDIR)\bv.sbr" \
	"$(INTDIR)\cellb_tables.sbr" \
	"$(INTDIR)\compositor.sbr" \
	"$(INTDIR)\dct.sbr" \
	"$(INTDIR)\decoder-bvc.sbr" \
	"$(INTDIR)\decoder-cellb.sbr" \
	"$(INTDIR)\decoder-h261.sbr" \
	"$(INTDIR)\decoder-h261v1.sbr" \
	"$(INTDIR)\decoder-h263.sbr" \
	"$(INTDIR)\decoder-h263v2.sbr" \
	"$(INTDIR)\decoder-jpeg.sbr" \
	"$(INTDIR)\decoder-nv.sbr" \
	"$(INTDIR)\decoder-pvh.sbr" \
	"$(INTDIR)\decoder-raw.sbr" \
	"$(INTDIR)\decoder.sbr" \
	"$(INTDIR)\encoder-bvc.sbr" \
	"$(INTDIR)\encoder-cellb.sbr" \
	"$(INTDIR)\encoder-h261.sbr" \
	"$(INTDIR)\encoder-h263.sbr" \
	"$(INTDIR)\encoder-h263v2.sbr" \
	"$(INTDIR)\encoder-jpeg.sbr" \
	"$(INTDIR)\encoder-nv.sbr" \
	"$(INTDIR)\encoder-pvh.sbr" \
	"$(INTDIR)\encoder-raw.sbr" \
	"$(INTDIR)\framer-jpeg.sbr" \
	"$(INTDIR)\huffcode.sbr" \
	"$(INTDIR)\jpeg.sbr" \
	"$(INTDIR)\p64.sbr" \
	"$(INTDIR)\pvh-huff.sbr" \
	"$(INTDIR)\transcoder-jpeg.sbr" \
	"$(INTDIR)\cm0.sbr" \
	"$(INTDIR)\cm1.sbr" \
	"$(INTDIR)\color-dither.sbr" \
	"$(INTDIR)\color-ed.sbr" \
	"$(INTDIR)\color-gray.sbr" \
	"$(INTDIR)\color-hi.sbr" \
	"$(INTDIR)\color-hist.sbr" \
	"$(INTDIR)\color-mono.sbr" \
	"$(INTDIR)\color-pseudo.sbr" \
	"$(INTDIR)\color-quant.sbr" \
	"$(INTDIR)\color-true.sbr" \
	"$(INTDIR)\color-yuv.sbr" \
	"$(INTDIR)\color.sbr" \
	"$(INTDIR)\renderer-window.sbr" \
	"$(INTDIR)\renderer.sbr" \
	"$(INTDIR)\rgb-converter.sbr" \
	"$(INTDIR)\vw.sbr" \
	"$(INTDIR)\pktbuf-rtp.sbr" \
	"$(INTDIR)\session.sbr" \
	"$(INTDIR)\source.sbr" \
	"$(INTDIR)\transmitter.sbr" \
	"$(INTDIR)\win32.sbr" \
	"$(INTDIR)\win32X.sbr" \
	"$(INTDIR)\getopt.sbr" \
	"$(INTDIR)\idlecallback.sbr" \
	"$(INTDIR)\iohandler.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\md5c.sbr" \
	"$(INTDIR)\media-timer.sbr" \
	"$(INTDIR)\module.sbr" \
	"$(INTDIR)\random.sbr" \
	"$(INTDIR)\rate-variable.sbr" \
	"$(INTDIR)\strtol.sbr" \
	"$(INTDIR)\strtoul.sbr" \
	"$(INTDIR)\Tcl.sbr" \
	"$(INTDIR)\Tcl2.sbr" \
	"$(INTDIR)\timer.sbr" \
	"$(INTDIR)\tkStripchart.sbr" \
	"$(INTDIR)\tkWinColor.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\assistor-list.sbr" \
	"$(INTDIR)\device.sbr" \
	"$(INTDIR)\grabber-still.sbr" \
	"$(INTDIR)\grabber-win32.sbr" \
	"$(INTDIR)\grabber.sbr"

"$(OUTDIR)\vic.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=tklib.lib tcllib.lib wship6.lib uclmm.lib winmm.lib wsock32.lib Ws2_32.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libh263.lib libh263coder.lib vfw32.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\vic.pdb" /debug /machine:I386 /out:"$(OUTDIR)\vic.exe" /pdbtype:sept /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"..\common\Debug_IPv6" /libpath:"codec\tmndec" /libpath:"codec\tmn-x" /libpath:"..\IPv6Kit\lib" 
LINK32_OBJS= \
	"$(INTDIR)\cf-confbus.obj" \
	"$(INTDIR)\cf-main.obj" \
	"$(INTDIR)\cf-network.obj" \
	"$(INTDIR)\cf-tm.obj" \
	"$(INTDIR)\cf-util.obj" \
	"$(INTDIR)\entry.obj" \
	"$(INTDIR)\tkerror.obj" \
	"$(INTDIR)\ui-ctrlmenu.obj" \
	"$(INTDIR)\ui-extout.obj" \
	"$(INTDIR)\ui-grabber.obj" \
	"$(INTDIR)\ui-help.obj" \
	"$(INTDIR)\ui-main.obj" \
	"$(INTDIR)\ui-relate.obj" \
	"$(INTDIR)\ui-resource.obj" \
	"$(INTDIR)\ui-srclist.obj" \
	"$(INTDIR)\ui-stats.obj" \
	"$(INTDIR)\ui-switcher.obj" \
	"$(INTDIR)\ui-util.obj" \
	"$(INTDIR)\ui-win32.obj" \
	"$(INTDIR)\ui-windows.obj" \
	"$(INTDIR)\communicator.obj" \
	"$(INTDIR)\confbus.obj" \
	"$(INTDIR)\crypt-des.obj" \
	"$(INTDIR)\crypt-dull.obj" \
	"$(INTDIR)\crypt.obj" \
	"$(INTDIR)\group-ipc.obj" \
	"$(INTDIR)\inet.obj" \
	"$(INTDIR)\inet6.obj" \
	"$(INTDIR)\mbus_engine.obj" \
	"$(INTDIR)\mbus_handler.obj" \
	"$(INTDIR)\net-addr.obj" \
	"$(INTDIR)\net-ip.obj" \
	"$(INTDIR)\net-ipv6.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\pktbuf.obj" \
	"$(INTDIR)\pkttbl.obj" \
	"$(INTDIR)\bitIn.obj" \
	"$(INTDIR)\bitOut.obj" \
	"$(INTDIR)\block.obj" \
	"$(INTDIR)\code.obj" \
	"$(INTDIR)\fdct.obj" \
	"$(INTDIR)\getblk.obj" \
	"$(INTDIR)\getgob.obj" \
	"$(INTDIR)\gethdr.obj" \
	"$(INTDIR)\getvlc.obj" \
	"$(INTDIR)\h263dec.obj" \
	"$(INTDIR)\h263enc.obj" \
	"$(INTDIR)\h263mux.obj" \
	"$(INTDIR)\h263rtp.obj" \
	"$(INTDIR)\idctdec.obj" \
	"$(INTDIR)\idctenc.obj" \
	"$(INTDIR)\input.obj" \
	"$(INTDIR)\motion.obj" \
	"$(INTDIR)\recon.obj" \
	"$(INTDIR)\reconh263.obj" \
	"$(INTDIR)\sac.obj" \
	"$(INTDIR)\bv.obj" \
	"$(INTDIR)\cellb_tables.obj" \
	"$(INTDIR)\compositor.obj" \
	"$(INTDIR)\dct.obj" \
	"$(INTDIR)\decoder-bvc.obj" \
	"$(INTDIR)\decoder-cellb.obj" \
	"$(INTDIR)\decoder-h261.obj" \
	"$(INTDIR)\decoder-h261v1.obj" \
	"$(INTDIR)\decoder-h263.obj" \
	"$(INTDIR)\decoder-h263v2.obj" \
	"$(INTDIR)\decoder-jpeg.obj" \
	"$(INTDIR)\decoder-nv.obj" \
	"$(INTDIR)\decoder-pvh.obj" \
	"$(INTDIR)\decoder-raw.obj" \
	"$(INTDIR)\decoder.obj" \
	"$(INTDIR)\encoder-bvc.obj" \
	"$(INTDIR)\encoder-cellb.obj" \
	"$(INTDIR)\encoder-h261.obj" \
	"$(INTDIR)\encoder-h263.obj" \
	"$(INTDIR)\encoder-h263v2.obj" \
	"$(INTDIR)\encoder-jpeg.obj" \
	"$(INTDIR)\encoder-nv.obj" \
	"$(INTDIR)\encoder-pvh.obj" \
	"$(INTDIR)\encoder-raw.obj" \
	"$(INTDIR)\framer-jpeg.obj" \
	"$(INTDIR)\huffcode.obj" \
	"$(INTDIR)\jpeg.obj" \
	"$(INTDIR)\p64.obj" \
	"$(INTDIR)\pvh-huff.obj" \
	"$(INTDIR)\transcoder-jpeg.obj" \
	"$(INTDIR)\cm0.obj" \
	"$(INTDIR)\cm1.obj" \
	"$(INTDIR)\color-dither.obj" \
	"$(INTDIR)\color-ed.obj" \
	"$(INTDIR)\color-gray.obj" \
	"$(INTDIR)\color-hi.obj" \
	"$(INTDIR)\color-hist.obj" \
	"$(INTDIR)\color-mono.obj" \
	"$(INTDIR)\color-pseudo.obj" \
	"$(INTDIR)\color-quant.obj" \
	"$(INTDIR)\color-true.obj" \
	"$(INTDIR)\color-yuv.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\renderer-window.obj" \
	"$(INTDIR)\renderer.obj" \
	"$(INTDIR)\rgb-converter.obj" \
	"$(INTDIR)\vw.obj" \
	"$(INTDIR)\pktbuf-rtp.obj" \
	"$(INTDIR)\session.obj" \
	"$(INTDIR)\source.obj" \
	"$(INTDIR)\transmitter.obj" \
	"$(INTDIR)\win32.obj" \
	"$(INTDIR)\win32X.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\idlecallback.obj" \
	"$(INTDIR)\iohandler.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\media-timer.obj" \
	"$(INTDIR)\module.obj" \
	"$(INTDIR)\random.obj" \
	"$(INTDIR)\rate-variable.obj" \
	"$(INTDIR)\strtol.obj" \
	"$(INTDIR)\strtoul.obj" \
	"$(INTDIR)\Tcl.obj" \
	"$(INTDIR)\Tcl2.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\tkStripchart.obj" \
	"$(INTDIR)\tkWinColor.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\assistor-list.obj" \
	"$(INTDIR)\device.obj" \
	"$(INTDIR)\grabber-still.obj" \
	"$(INTDIR)\grabber-win32.obj" \
	"$(INTDIR)\grabber.obj" \
	"..\tk-8.0\win\tk.res" \
	"..\common\Debug_IPv6\uclmm.lib" \
	"..\tcl-8.0\win\Debug_IPv6\tcllib.lib" \
	"..\tk-8.0\win\Debug_IPv6\tklib.lib"

"$(OUTDIR)\vic.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

OUTDIR=.\Debug_IPv6_Musica
INTDIR=.\Debug_IPv6_Musica
# Begin Custom Macros
OutDir=.\Debug_IPv6_Musica
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\vic.exe" "$(OUTDIR)\vic.bsc"

!ELSE 

ALL : "common - Win32 Debug IPv6 Musica" "$(OUTDIR)\vic.exe" "$(OUTDIR)\vic.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"common - Win32 Debug IPv6 MusicaCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\assistor-list.obj"
	-@erase "$(INTDIR)\assistor-list.sbr"
	-@erase "$(INTDIR)\bitIn.obj"
	-@erase "$(INTDIR)\bitIn.sbr"
	-@erase "$(INTDIR)\bitOut.obj"
	-@erase "$(INTDIR)\bitOut.sbr"
	-@erase "$(INTDIR)\block.obj"
	-@erase "$(INTDIR)\block.sbr"
	-@erase "$(INTDIR)\bv.obj"
	-@erase "$(INTDIR)\bv.sbr"
	-@erase "$(INTDIR)\cellb_tables.obj"
	-@erase "$(INTDIR)\cellb_tables.sbr"
	-@erase "$(INTDIR)\cf-confbus.obj"
	-@erase "$(INTDIR)\cf-confbus.sbr"
	-@erase "$(INTDIR)\cf-main.obj"
	-@erase "$(INTDIR)\cf-main.sbr"
	-@erase "$(INTDIR)\cf-network.obj"
	-@erase "$(INTDIR)\cf-network.sbr"
	-@erase "$(INTDIR)\cf-tm.obj"
	-@erase "$(INTDIR)\cf-tm.sbr"
	-@erase "$(INTDIR)\cf-util.obj"
	-@erase "$(INTDIR)\cf-util.sbr"
	-@erase "$(INTDIR)\cm0.obj"
	-@erase "$(INTDIR)\cm0.sbr"
	-@erase "$(INTDIR)\cm1.obj"
	-@erase "$(INTDIR)\cm1.sbr"
	-@erase "$(INTDIR)\code.obj"
	-@erase "$(INTDIR)\code.sbr"
	-@erase "$(INTDIR)\color-dither.obj"
	-@erase "$(INTDIR)\color-dither.sbr"
	-@erase "$(INTDIR)\color-ed.obj"
	-@erase "$(INTDIR)\color-ed.sbr"
	-@erase "$(INTDIR)\color-gray.obj"
	-@erase "$(INTDIR)\color-gray.sbr"
	-@erase "$(INTDIR)\color-hi.obj"
	-@erase "$(INTDIR)\color-hi.sbr"
	-@erase "$(INTDIR)\color-hist.obj"
	-@erase "$(INTDIR)\color-hist.sbr"
	-@erase "$(INTDIR)\color-mono.obj"
	-@erase "$(INTDIR)\color-mono.sbr"
	-@erase "$(INTDIR)\color-pseudo.obj"
	-@erase "$(INTDIR)\color-pseudo.sbr"
	-@erase "$(INTDIR)\color-quant.obj"
	-@erase "$(INTDIR)\color-quant.sbr"
	-@erase "$(INTDIR)\color-true.obj"
	-@erase "$(INTDIR)\color-true.sbr"
	-@erase "$(INTDIR)\color-yuv.obj"
	-@erase "$(INTDIR)\color-yuv.sbr"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\color.sbr"
	-@erase "$(INTDIR)\communicator.obj"
	-@erase "$(INTDIR)\communicator.sbr"
	-@erase "$(INTDIR)\compositor.obj"
	-@erase "$(INTDIR)\compositor.sbr"
	-@erase "$(INTDIR)\confbus.obj"
	-@erase "$(INTDIR)\confbus.sbr"
	-@erase "$(INTDIR)\crypt-des.obj"
	-@erase "$(INTDIR)\crypt-des.sbr"
	-@erase "$(INTDIR)\crypt-dull.obj"
	-@erase "$(INTDIR)\crypt-dull.sbr"
	-@erase "$(INTDIR)\crypt.obj"
	-@erase "$(INTDIR)\crypt.sbr"
	-@erase "$(INTDIR)\dct.obj"
	-@erase "$(INTDIR)\dct.sbr"
	-@erase "$(INTDIR)\decoder-bvc.obj"
	-@erase "$(INTDIR)\decoder-bvc.sbr"
	-@erase "$(INTDIR)\decoder-cellb.obj"
	-@erase "$(INTDIR)\decoder-cellb.sbr"
	-@erase "$(INTDIR)\decoder-h261.obj"
	-@erase "$(INTDIR)\decoder-h261.sbr"
	-@erase "$(INTDIR)\decoder-h261v1.obj"
	-@erase "$(INTDIR)\decoder-h261v1.sbr"
	-@erase "$(INTDIR)\decoder-h263.obj"
	-@erase "$(INTDIR)\decoder-h263.sbr"
	-@erase "$(INTDIR)\decoder-h263v2.obj"
	-@erase "$(INTDIR)\decoder-h263v2.sbr"
	-@erase "$(INTDIR)\decoder-jpeg.obj"
	-@erase "$(INTDIR)\decoder-jpeg.sbr"
	-@erase "$(INTDIR)\decoder-nv.obj"
	-@erase "$(INTDIR)\decoder-nv.sbr"
	-@erase "$(INTDIR)\decoder-pvh.obj"
	-@erase "$(INTDIR)\decoder-pvh.sbr"
	-@erase "$(INTDIR)\decoder-raw.obj"
	-@erase "$(INTDIR)\decoder-raw.sbr"
	-@erase "$(INTDIR)\decoder.obj"
	-@erase "$(INTDIR)\decoder.sbr"
	-@erase "$(INTDIR)\device.obj"
	-@erase "$(INTDIR)\device.sbr"
	-@erase "$(INTDIR)\encoder-bvc.obj"
	-@erase "$(INTDIR)\encoder-bvc.sbr"
	-@erase "$(INTDIR)\encoder-cellb.obj"
	-@erase "$(INTDIR)\encoder-cellb.sbr"
	-@erase "$(INTDIR)\encoder-h261.obj"
	-@erase "$(INTDIR)\encoder-h261.sbr"
	-@erase "$(INTDIR)\encoder-h263.obj"
	-@erase "$(INTDIR)\encoder-h263.sbr"
	-@erase "$(INTDIR)\encoder-h263v2.obj"
	-@erase "$(INTDIR)\encoder-h263v2.sbr"
	-@erase "$(INTDIR)\encoder-jpeg.obj"
	-@erase "$(INTDIR)\encoder-jpeg.sbr"
	-@erase "$(INTDIR)\encoder-nv.obj"
	-@erase "$(INTDIR)\encoder-nv.sbr"
	-@erase "$(INTDIR)\encoder-pvh.obj"
	-@erase "$(INTDIR)\encoder-pvh.sbr"
	-@erase "$(INTDIR)\encoder-raw.obj"
	-@erase "$(INTDIR)\encoder-raw.sbr"
	-@erase "$(INTDIR)\entry.obj"
	-@erase "$(INTDIR)\entry.sbr"
	-@erase "$(INTDIR)\fdct.obj"
	-@erase "$(INTDIR)\fdct.sbr"
	-@erase "$(INTDIR)\framer-jpeg.obj"
	-@erase "$(INTDIR)\framer-jpeg.sbr"
	-@erase "$(INTDIR)\getblk.obj"
	-@erase "$(INTDIR)\getblk.sbr"
	-@erase "$(INTDIR)\getgob.obj"
	-@erase "$(INTDIR)\getgob.sbr"
	-@erase "$(INTDIR)\gethdr.obj"
	-@erase "$(INTDIR)\gethdr.sbr"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt.sbr"
	-@erase "$(INTDIR)\getvlc.obj"
	-@erase "$(INTDIR)\getvlc.sbr"
	-@erase "$(INTDIR)\grabber-still.obj"
	-@erase "$(INTDIR)\grabber-still.sbr"
	-@erase "$(INTDIR)\grabber-win32.obj"
	-@erase "$(INTDIR)\grabber-win32.sbr"
	-@erase "$(INTDIR)\grabber.obj"
	-@erase "$(INTDIR)\grabber.sbr"
	-@erase "$(INTDIR)\group-ipc.obj"
	-@erase "$(INTDIR)\group-ipc.sbr"
	-@erase "$(INTDIR)\h263dec.obj"
	-@erase "$(INTDIR)\h263dec.sbr"
	-@erase "$(INTDIR)\h263enc.obj"
	-@erase "$(INTDIR)\h263enc.sbr"
	-@erase "$(INTDIR)\h263mux.obj"
	-@erase "$(INTDIR)\h263mux.sbr"
	-@erase "$(INTDIR)\h263rtp.obj"
	-@erase "$(INTDIR)\h263rtp.sbr"
	-@erase "$(INTDIR)\huffcode.obj"
	-@erase "$(INTDIR)\huffcode.sbr"
	-@erase "$(INTDIR)\idctdec.obj"
	-@erase "$(INTDIR)\idctdec.sbr"
	-@erase "$(INTDIR)\idctenc.obj"
	-@erase "$(INTDIR)\idctenc.sbr"
	-@erase "$(INTDIR)\idlecallback.obj"
	-@erase "$(INTDIR)\idlecallback.sbr"
	-@erase "$(INTDIR)\inet.obj"
	-@erase "$(INTDIR)\inet.sbr"
	-@erase "$(INTDIR)\inet6.obj"
	-@erase "$(INTDIR)\inet6.sbr"
	-@erase "$(INTDIR)\input.obj"
	-@erase "$(INTDIR)\input.sbr"
	-@erase "$(INTDIR)\iohandler.obj"
	-@erase "$(INTDIR)\iohandler.sbr"
	-@erase "$(INTDIR)\jpeg.obj"
	-@erase "$(INTDIR)\jpeg.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\mbus_engine.obj"
	-@erase "$(INTDIR)\mbus_engine.sbr"
	-@erase "$(INTDIR)\mbus_handler.obj"
	-@erase "$(INTDIR)\mbus_handler.sbr"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5c.sbr"
	-@erase "$(INTDIR)\media-timer.obj"
	-@erase "$(INTDIR)\media-timer.sbr"
	-@erase "$(INTDIR)\module.obj"
	-@erase "$(INTDIR)\module.sbr"
	-@erase "$(INTDIR)\motion.obj"
	-@erase "$(INTDIR)\motion.sbr"
	-@erase "$(INTDIR)\net-addr.obj"
	-@erase "$(INTDIR)\net-addr.sbr"
	-@erase "$(INTDIR)\net-ip.obj"
	-@erase "$(INTDIR)\net-ip.sbr"
	-@erase "$(INTDIR)\net-ipv6.obj"
	-@erase "$(INTDIR)\net-ipv6.sbr"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\net.sbr"
	-@erase "$(INTDIR)\p64.obj"
	-@erase "$(INTDIR)\p64.sbr"
	-@erase "$(INTDIR)\pktbuf-rtp.obj"
	-@erase "$(INTDIR)\pktbuf-rtp.sbr"
	-@erase "$(INTDIR)\pktbuf.obj"
	-@erase "$(INTDIR)\pktbuf.sbr"
	-@erase "$(INTDIR)\pkttbl.obj"
	-@erase "$(INTDIR)\pkttbl.sbr"
	-@erase "$(INTDIR)\pvh-huff.obj"
	-@erase "$(INTDIR)\pvh-huff.sbr"
	-@erase "$(INTDIR)\random.obj"
	-@erase "$(INTDIR)\random.sbr"
	-@erase "$(INTDIR)\rate-variable.obj"
	-@erase "$(INTDIR)\rate-variable.sbr"
	-@erase "$(INTDIR)\recon.obj"
	-@erase "$(INTDIR)\recon.sbr"
	-@erase "$(INTDIR)\reconh263.obj"
	-@erase "$(INTDIR)\reconh263.sbr"
	-@erase "$(INTDIR)\renderer-window.obj"
	-@erase "$(INTDIR)\renderer-window.sbr"
	-@erase "$(INTDIR)\renderer.obj"
	-@erase "$(INTDIR)\renderer.sbr"
	-@erase "$(INTDIR)\rgb-converter.obj"
	-@erase "$(INTDIR)\rgb-converter.sbr"
	-@erase "$(INTDIR)\sac.obj"
	-@erase "$(INTDIR)\sac.sbr"
	-@erase "$(INTDIR)\session.obj"
	-@erase "$(INTDIR)\session.sbr"
	-@erase "$(INTDIR)\source.obj"
	-@erase "$(INTDIR)\source.sbr"
	-@erase "$(INTDIR)\strtol.obj"
	-@erase "$(INTDIR)\strtol.sbr"
	-@erase "$(INTDIR)\strtoul.obj"
	-@erase "$(INTDIR)\strtoul.sbr"
	-@erase "$(INTDIR)\Tcl.obj"
	-@erase "$(INTDIR)\Tcl.sbr"
	-@erase "$(INTDIR)\Tcl2.obj"
	-@erase "$(INTDIR)\Tcl2.sbr"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\timer.sbr"
	-@erase "$(INTDIR)\tkerror.obj"
	-@erase "$(INTDIR)\tkerror.sbr"
	-@erase "$(INTDIR)\tkStripchart.obj"
	-@erase "$(INTDIR)\tkStripchart.sbr"
	-@erase "$(INTDIR)\tkWinColor.obj"
	-@erase "$(INTDIR)\tkWinColor.sbr"
	-@erase "$(INTDIR)\transcoder-jpeg.obj"
	-@erase "$(INTDIR)\transcoder-jpeg.sbr"
	-@erase "$(INTDIR)\transmitter.obj"
	-@erase "$(INTDIR)\transmitter.sbr"
	-@erase "$(INTDIR)\ui-ctrlmenu.obj"
	-@erase "$(INTDIR)\ui-ctrlmenu.sbr"
	-@erase "$(INTDIR)\ui-extout.obj"
	-@erase "$(INTDIR)\ui-extout.sbr"
	-@erase "$(INTDIR)\ui-grabber.obj"
	-@erase "$(INTDIR)\ui-grabber.sbr"
	-@erase "$(INTDIR)\ui-help.obj"
	-@erase "$(INTDIR)\ui-help.sbr"
	-@erase "$(INTDIR)\ui-main.obj"
	-@erase "$(INTDIR)\ui-main.sbr"
	-@erase "$(INTDIR)\ui-relate.obj"
	-@erase "$(INTDIR)\ui-relate.sbr"
	-@erase "$(INTDIR)\ui-resource.obj"
	-@erase "$(INTDIR)\ui-resource.sbr"
	-@erase "$(INTDIR)\ui-srclist.obj"
	-@erase "$(INTDIR)\ui-srclist.sbr"
	-@erase "$(INTDIR)\ui-stats.obj"
	-@erase "$(INTDIR)\ui-stats.sbr"
	-@erase "$(INTDIR)\ui-switcher.obj"
	-@erase "$(INTDIR)\ui-switcher.sbr"
	-@erase "$(INTDIR)\ui-util.obj"
	-@erase "$(INTDIR)\ui-util.sbr"
	-@erase "$(INTDIR)\ui-win32.obj"
	-@erase "$(INTDIR)\ui-win32.sbr"
	-@erase "$(INTDIR)\ui-windows.obj"
	-@erase "$(INTDIR)\ui-windows.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(INTDIR)\vw.obj"
	-@erase "$(INTDIR)\vw.sbr"
	-@erase "$(INTDIR)\win32.obj"
	-@erase "$(INTDIR)\win32.sbr"
	-@erase "$(INTDIR)\win32X.obj"
	-@erase "$(INTDIR)\win32X.sbr"
	-@erase "$(OUTDIR)\vic.bsc"
	-@erase "$(OUTDIR)\vic.exe"
	-@erase "$(OUTDIR)\vic.ilk"
	-@erase "$(OUTDIR)\vic.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

vic=rc.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "..\MUSICA\WINSOCK6" /D "_WINDOWS" /D "DEBUG" /D "SASR" /D "_DEBUG" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "MUSICA_IPV6" /D "_POSIX" /D "_WINNT" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vic.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cf-confbus.sbr" \
	"$(INTDIR)\cf-main.sbr" \
	"$(INTDIR)\cf-network.sbr" \
	"$(INTDIR)\cf-tm.sbr" \
	"$(INTDIR)\cf-util.sbr" \
	"$(INTDIR)\entry.sbr" \
	"$(INTDIR)\tkerror.sbr" \
	"$(INTDIR)\ui-ctrlmenu.sbr" \
	"$(INTDIR)\ui-extout.sbr" \
	"$(INTDIR)\ui-grabber.sbr" \
	"$(INTDIR)\ui-help.sbr" \
	"$(INTDIR)\ui-main.sbr" \
	"$(INTDIR)\ui-relate.sbr" \
	"$(INTDIR)\ui-resource.sbr" \
	"$(INTDIR)\ui-srclist.sbr" \
	"$(INTDIR)\ui-stats.sbr" \
	"$(INTDIR)\ui-switcher.sbr" \
	"$(INTDIR)\ui-util.sbr" \
	"$(INTDIR)\ui-win32.sbr" \
	"$(INTDIR)\ui-windows.sbr" \
	"$(INTDIR)\communicator.sbr" \
	"$(INTDIR)\confbus.sbr" \
	"$(INTDIR)\crypt-des.sbr" \
	"$(INTDIR)\crypt-dull.sbr" \
	"$(INTDIR)\crypt.sbr" \
	"$(INTDIR)\group-ipc.sbr" \
	"$(INTDIR)\inet.sbr" \
	"$(INTDIR)\inet6.sbr" \
	"$(INTDIR)\mbus_engine.sbr" \
	"$(INTDIR)\mbus_handler.sbr" \
	"$(INTDIR)\net-addr.sbr" \
	"$(INTDIR)\net-ip.sbr" \
	"$(INTDIR)\net-ipv6.sbr" \
	"$(INTDIR)\net.sbr" \
	"$(INTDIR)\pktbuf.sbr" \
	"$(INTDIR)\pkttbl.sbr" \
	"$(INTDIR)\bitIn.sbr" \
	"$(INTDIR)\bitOut.sbr" \
	"$(INTDIR)\block.sbr" \
	"$(INTDIR)\code.sbr" \
	"$(INTDIR)\fdct.sbr" \
	"$(INTDIR)\getblk.sbr" \
	"$(INTDIR)\getgob.sbr" \
	"$(INTDIR)\gethdr.sbr" \
	"$(INTDIR)\getvlc.sbr" \
	"$(INTDIR)\h263dec.sbr" \
	"$(INTDIR)\h263enc.sbr" \
	"$(INTDIR)\h263mux.sbr" \
	"$(INTDIR)\h263rtp.sbr" \
	"$(INTDIR)\idctdec.sbr" \
	"$(INTDIR)\idctenc.sbr" \
	"$(INTDIR)\input.sbr" \
	"$(INTDIR)\motion.sbr" \
	"$(INTDIR)\recon.sbr" \
	"$(INTDIR)\reconh263.sbr" \
	"$(INTDIR)\sac.sbr" \
	"$(INTDIR)\bv.sbr" \
	"$(INTDIR)\cellb_tables.sbr" \
	"$(INTDIR)\compositor.sbr" \
	"$(INTDIR)\dct.sbr" \
	"$(INTDIR)\decoder-bvc.sbr" \
	"$(INTDIR)\decoder-cellb.sbr" \
	"$(INTDIR)\decoder-h261.sbr" \
	"$(INTDIR)\decoder-h261v1.sbr" \
	"$(INTDIR)\decoder-h263.sbr" \
	"$(INTDIR)\decoder-h263v2.sbr" \
	"$(INTDIR)\decoder-jpeg.sbr" \
	"$(INTDIR)\decoder-nv.sbr" \
	"$(INTDIR)\decoder-pvh.sbr" \
	"$(INTDIR)\decoder-raw.sbr" \
	"$(INTDIR)\decoder.sbr" \
	"$(INTDIR)\encoder-bvc.sbr" \
	"$(INTDIR)\encoder-cellb.sbr" \
	"$(INTDIR)\encoder-h261.sbr" \
	"$(INTDIR)\encoder-h263.sbr" \
	"$(INTDIR)\encoder-h263v2.sbr" \
	"$(INTDIR)\encoder-jpeg.sbr" \
	"$(INTDIR)\encoder-nv.sbr" \
	"$(INTDIR)\encoder-pvh.sbr" \
	"$(INTDIR)\encoder-raw.sbr" \
	"$(INTDIR)\framer-jpeg.sbr" \
	"$(INTDIR)\huffcode.sbr" \
	"$(INTDIR)\jpeg.sbr" \
	"$(INTDIR)\p64.sbr" \
	"$(INTDIR)\pvh-huff.sbr" \
	"$(INTDIR)\transcoder-jpeg.sbr" \
	"$(INTDIR)\cm0.sbr" \
	"$(INTDIR)\cm1.sbr" \
	"$(INTDIR)\color-dither.sbr" \
	"$(INTDIR)\color-ed.sbr" \
	"$(INTDIR)\color-gray.sbr" \
	"$(INTDIR)\color-hi.sbr" \
	"$(INTDIR)\color-hist.sbr" \
	"$(INTDIR)\color-mono.sbr" \
	"$(INTDIR)\color-pseudo.sbr" \
	"$(INTDIR)\color-quant.sbr" \
	"$(INTDIR)\color-true.sbr" \
	"$(INTDIR)\color-yuv.sbr" \
	"$(INTDIR)\color.sbr" \
	"$(INTDIR)\renderer-window.sbr" \
	"$(INTDIR)\renderer.sbr" \
	"$(INTDIR)\rgb-converter.sbr" \
	"$(INTDIR)\vw.sbr" \
	"$(INTDIR)\pktbuf-rtp.sbr" \
	"$(INTDIR)\session.sbr" \
	"$(INTDIR)\source.sbr" \
	"$(INTDIR)\transmitter.sbr" \
	"$(INTDIR)\win32.sbr" \
	"$(INTDIR)\win32X.sbr" \
	"$(INTDIR)\getopt.sbr" \
	"$(INTDIR)\idlecallback.sbr" \
	"$(INTDIR)\iohandler.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\md5c.sbr" \
	"$(INTDIR)\media-timer.sbr" \
	"$(INTDIR)\module.sbr" \
	"$(INTDIR)\random.sbr" \
	"$(INTDIR)\rate-variable.sbr" \
	"$(INTDIR)\strtol.sbr" \
	"$(INTDIR)\strtoul.sbr" \
	"$(INTDIR)\Tcl.sbr" \
	"$(INTDIR)\Tcl2.sbr" \
	"$(INTDIR)\timer.sbr" \
	"$(INTDIR)\tkStripchart.sbr" \
	"$(INTDIR)\tkWinColor.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\assistor-list.sbr" \
	"$(INTDIR)\device.sbr" \
	"$(INTDIR)\grabber-still.sbr" \
	"$(INTDIR)\grabber-win32.sbr" \
	"$(INTDIR)\grabber.sbr"

"$(OUTDIR)\vic.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=tklib.lib tcllib.lib uclmm.lib libh263.lib libh263coder.lib lib44bsd.lib Resolv.lib wsock32.lib Ws2_32.lib winmm.lib kernel32.lib user32.lib gdi32.lib shell32.lib vfw32.lib advapi32.lib comdlg32.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\vic.pdb" /debug /machine:I386 /out:"$(OUTDIR)\vic.exe" /pdbtype:sept /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"..\common\Debug" /libpath:"codec\tmndec" /libpath:"codec\tmn-x" /libpath:"..\MUSICA\WINSOCK6" 
LINK32_OBJS= \
	"$(INTDIR)\cf-confbus.obj" \
	"$(INTDIR)\cf-main.obj" \
	"$(INTDIR)\cf-network.obj" \
	"$(INTDIR)\cf-tm.obj" \
	"$(INTDIR)\cf-util.obj" \
	"$(INTDIR)\entry.obj" \
	"$(INTDIR)\tkerror.obj" \
	"$(INTDIR)\ui-ctrlmenu.obj" \
	"$(INTDIR)\ui-extout.obj" \
	"$(INTDIR)\ui-grabber.obj" \
	"$(INTDIR)\ui-help.obj" \
	"$(INTDIR)\ui-main.obj" \
	"$(INTDIR)\ui-relate.obj" \
	"$(INTDIR)\ui-resource.obj" \
	"$(INTDIR)\ui-srclist.obj" \
	"$(INTDIR)\ui-stats.obj" \
	"$(INTDIR)\ui-switcher.obj" \
	"$(INTDIR)\ui-util.obj" \
	"$(INTDIR)\ui-win32.obj" \
	"$(INTDIR)\ui-windows.obj" \
	"$(INTDIR)\communicator.obj" \
	"$(INTDIR)\confbus.obj" \
	"$(INTDIR)\crypt-des.obj" \
	"$(INTDIR)\crypt-dull.obj" \
	"$(INTDIR)\crypt.obj" \
	"$(INTDIR)\group-ipc.obj" \
	"$(INTDIR)\inet.obj" \
	"$(INTDIR)\inet6.obj" \
	"$(INTDIR)\mbus_engine.obj" \
	"$(INTDIR)\mbus_handler.obj" \
	"$(INTDIR)\net-addr.obj" \
	"$(INTDIR)\net-ip.obj" \
	"$(INTDIR)\net-ipv6.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\pktbuf.obj" \
	"$(INTDIR)\pkttbl.obj" \
	"$(INTDIR)\bitIn.obj" \
	"$(INTDIR)\bitOut.obj" \
	"$(INTDIR)\block.obj" \
	"$(INTDIR)\code.obj" \
	"$(INTDIR)\fdct.obj" \
	"$(INTDIR)\getblk.obj" \
	"$(INTDIR)\getgob.obj" \
	"$(INTDIR)\gethdr.obj" \
	"$(INTDIR)\getvlc.obj" \
	"$(INTDIR)\h263dec.obj" \
	"$(INTDIR)\h263enc.obj" \
	"$(INTDIR)\h263mux.obj" \
	"$(INTDIR)\h263rtp.obj" \
	"$(INTDIR)\idctdec.obj" \
	"$(INTDIR)\idctenc.obj" \
	"$(INTDIR)\input.obj" \
	"$(INTDIR)\motion.obj" \
	"$(INTDIR)\recon.obj" \
	"$(INTDIR)\reconh263.obj" \
	"$(INTDIR)\sac.obj" \
	"$(INTDIR)\bv.obj" \
	"$(INTDIR)\cellb_tables.obj" \
	"$(INTDIR)\compositor.obj" \
	"$(INTDIR)\dct.obj" \
	"$(INTDIR)\decoder-bvc.obj" \
	"$(INTDIR)\decoder-cellb.obj" \
	"$(INTDIR)\decoder-h261.obj" \
	"$(INTDIR)\decoder-h261v1.obj" \
	"$(INTDIR)\decoder-h263.obj" \
	"$(INTDIR)\decoder-h263v2.obj" \
	"$(INTDIR)\decoder-jpeg.obj" \
	"$(INTDIR)\decoder-nv.obj" \
	"$(INTDIR)\decoder-pvh.obj" \
	"$(INTDIR)\decoder-raw.obj" \
	"$(INTDIR)\decoder.obj" \
	"$(INTDIR)\encoder-bvc.obj" \
	"$(INTDIR)\encoder-cellb.obj" \
	"$(INTDIR)\encoder-h261.obj" \
	"$(INTDIR)\encoder-h263.obj" \
	"$(INTDIR)\encoder-h263v2.obj" \
	"$(INTDIR)\encoder-jpeg.obj" \
	"$(INTDIR)\encoder-nv.obj" \
	"$(INTDIR)\encoder-pvh.obj" \
	"$(INTDIR)\encoder-raw.obj" \
	"$(INTDIR)\framer-jpeg.obj" \
	"$(INTDIR)\huffcode.obj" \
	"$(INTDIR)\jpeg.obj" \
	"$(INTDIR)\p64.obj" \
	"$(INTDIR)\pvh-huff.obj" \
	"$(INTDIR)\transcoder-jpeg.obj" \
	"$(INTDIR)\cm0.obj" \
	"$(INTDIR)\cm1.obj" \
	"$(INTDIR)\color-dither.obj" \
	"$(INTDIR)\color-ed.obj" \
	"$(INTDIR)\color-gray.obj" \
	"$(INTDIR)\color-hi.obj" \
	"$(INTDIR)\color-hist.obj" \
	"$(INTDIR)\color-mono.obj" \
	"$(INTDIR)\color-pseudo.obj" \
	"$(INTDIR)\color-quant.obj" \
	"$(INTDIR)\color-true.obj" \
	"$(INTDIR)\color-yuv.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\renderer-window.obj" \
	"$(INTDIR)\renderer.obj" \
	"$(INTDIR)\rgb-converter.obj" \
	"$(INTDIR)\vw.obj" \
	"$(INTDIR)\pktbuf-rtp.obj" \
	"$(INTDIR)\session.obj" \
	"$(INTDIR)\source.obj" \
	"$(INTDIR)\transmitter.obj" \
	"$(INTDIR)\win32.obj" \
	"$(INTDIR)\win32X.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\idlecallback.obj" \
	"$(INTDIR)\iohandler.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\media-timer.obj" \
	"$(INTDIR)\module.obj" \
	"$(INTDIR)\random.obj" \
	"$(INTDIR)\rate-variable.obj" \
	"$(INTDIR)\strtol.obj" \
	"$(INTDIR)\strtoul.obj" \
	"$(INTDIR)\Tcl.obj" \
	"$(INTDIR)\Tcl2.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\tkStripchart.obj" \
	"$(INTDIR)\tkWinColor.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\assistor-list.obj" \
	"$(INTDIR)\device.obj" \
	"$(INTDIR)\grabber-still.obj" \
	"$(INTDIR)\grabber-win32.obj" \
	"$(INTDIR)\grabber.obj" \
	"..\tk-8.0\win\tk.res" \
	"..\common\Debug\uclmm.lib"

"$(OUTDIR)\vic.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\vic.exe" "$(OUTDIR)\vic.bsc"

!ELSE 

ALL : "tcl2c - Win32 Debug" "tcl2cpp - Win32 Debug" "ppmtolut - Win32 Debug" "mkhuff - Win32 Debug" "mkcube - Win32 Debug" "mkbv - Win32 Debug" "histtolut - Win32 Debug" "H263v2 Enc_tmnx - Win32 Debug" "H263v2 Dec_tmndec - Win32 Debug" "tklib - Win32 Debug" "tcllib - Win32 Debug" "common - Win32 Debug" "$(OUTDIR)\vic.exe" "$(OUTDIR)\vic.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"common - Win32 DebugCLEAN" "tcllib - Win32 DebugCLEAN" "tklib - Win32 DebugCLEAN" "H263v2 Dec_tmndec - Win32 DebugCLEAN" "H263v2 Enc_tmnx - Win32 DebugCLEAN" "histtolut - Win32 DebugCLEAN" "mkbv - Win32 DebugCLEAN" "mkcube - Win32 DebugCLEAN" "mkhuff - Win32 DebugCLEAN" "ppmtolut - Win32 DebugCLEAN" "tcl2cpp - Win32 DebugCLEAN" "tcl2c - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\assistor-list.obj"
	-@erase "$(INTDIR)\assistor-list.sbr"
	-@erase "$(INTDIR)\bitIn.obj"
	-@erase "$(INTDIR)\bitIn.sbr"
	-@erase "$(INTDIR)\bitOut.obj"
	-@erase "$(INTDIR)\bitOut.sbr"
	-@erase "$(INTDIR)\block.obj"
	-@erase "$(INTDIR)\block.sbr"
	-@erase "$(INTDIR)\bv.obj"
	-@erase "$(INTDIR)\bv.sbr"
	-@erase "$(INTDIR)\cellb_tables.obj"
	-@erase "$(INTDIR)\cellb_tables.sbr"
	-@erase "$(INTDIR)\cf-confbus.obj"
	-@erase "$(INTDIR)\cf-confbus.sbr"
	-@erase "$(INTDIR)\cf-main.obj"
	-@erase "$(INTDIR)\cf-main.sbr"
	-@erase "$(INTDIR)\cf-network.obj"
	-@erase "$(INTDIR)\cf-network.sbr"
	-@erase "$(INTDIR)\cf-tm.obj"
	-@erase "$(INTDIR)\cf-tm.sbr"
	-@erase "$(INTDIR)\cf-util.obj"
	-@erase "$(INTDIR)\cf-util.sbr"
	-@erase "$(INTDIR)\cm0.obj"
	-@erase "$(INTDIR)\cm0.sbr"
	-@erase "$(INTDIR)\cm1.obj"
	-@erase "$(INTDIR)\cm1.sbr"
	-@erase "$(INTDIR)\code.obj"
	-@erase "$(INTDIR)\code.sbr"
	-@erase "$(INTDIR)\color-dither.obj"
	-@erase "$(INTDIR)\color-dither.sbr"
	-@erase "$(INTDIR)\color-ed.obj"
	-@erase "$(INTDIR)\color-ed.sbr"
	-@erase "$(INTDIR)\color-gray.obj"
	-@erase "$(INTDIR)\color-gray.sbr"
	-@erase "$(INTDIR)\color-hi.obj"
	-@erase "$(INTDIR)\color-hi.sbr"
	-@erase "$(INTDIR)\color-hist.obj"
	-@erase "$(INTDIR)\color-hist.sbr"
	-@erase "$(INTDIR)\color-mono.obj"
	-@erase "$(INTDIR)\color-mono.sbr"
	-@erase "$(INTDIR)\color-pseudo.obj"
	-@erase "$(INTDIR)\color-pseudo.sbr"
	-@erase "$(INTDIR)\color-quant.obj"
	-@erase "$(INTDIR)\color-quant.sbr"
	-@erase "$(INTDIR)\color-true.obj"
	-@erase "$(INTDIR)\color-true.sbr"
	-@erase "$(INTDIR)\color-yuv.obj"
	-@erase "$(INTDIR)\color-yuv.sbr"
	-@erase "$(INTDIR)\color.obj"
	-@erase "$(INTDIR)\color.sbr"
	-@erase "$(INTDIR)\communicator.obj"
	-@erase "$(INTDIR)\communicator.sbr"
	-@erase "$(INTDIR)\compositor.obj"
	-@erase "$(INTDIR)\compositor.sbr"
	-@erase "$(INTDIR)\confbus.obj"
	-@erase "$(INTDIR)\confbus.sbr"
	-@erase "$(INTDIR)\crypt-des.obj"
	-@erase "$(INTDIR)\crypt-des.sbr"
	-@erase "$(INTDIR)\crypt-dull.obj"
	-@erase "$(INTDIR)\crypt-dull.sbr"
	-@erase "$(INTDIR)\crypt.obj"
	-@erase "$(INTDIR)\crypt.sbr"
	-@erase "$(INTDIR)\dct.obj"
	-@erase "$(INTDIR)\dct.sbr"
	-@erase "$(INTDIR)\decoder-bvc.obj"
	-@erase "$(INTDIR)\decoder-bvc.sbr"
	-@erase "$(INTDIR)\decoder-cellb.obj"
	-@erase "$(INTDIR)\decoder-cellb.sbr"
	-@erase "$(INTDIR)\decoder-h261.obj"
	-@erase "$(INTDIR)\decoder-h261.sbr"
	-@erase "$(INTDIR)\decoder-h261v1.obj"
	-@erase "$(INTDIR)\decoder-h261v1.sbr"
	-@erase "$(INTDIR)\decoder-h263.obj"
	-@erase "$(INTDIR)\decoder-h263.sbr"
	-@erase "$(INTDIR)\decoder-h263v2.obj"
	-@erase "$(INTDIR)\decoder-h263v2.sbr"
	-@erase "$(INTDIR)\decoder-jpeg.obj"
	-@erase "$(INTDIR)\decoder-jpeg.sbr"
	-@erase "$(INTDIR)\decoder-nv.obj"
	-@erase "$(INTDIR)\decoder-nv.sbr"
	-@erase "$(INTDIR)\decoder-pvh.obj"
	-@erase "$(INTDIR)\decoder-pvh.sbr"
	-@erase "$(INTDIR)\decoder-raw.obj"
	-@erase "$(INTDIR)\decoder-raw.sbr"
	-@erase "$(INTDIR)\decoder.obj"
	-@erase "$(INTDIR)\decoder.sbr"
	-@erase "$(INTDIR)\device.obj"
	-@erase "$(INTDIR)\device.sbr"
	-@erase "$(INTDIR)\encoder-bvc.obj"
	-@erase "$(INTDIR)\encoder-bvc.sbr"
	-@erase "$(INTDIR)\encoder-cellb.obj"
	-@erase "$(INTDIR)\encoder-cellb.sbr"
	-@erase "$(INTDIR)\encoder-h261.obj"
	-@erase "$(INTDIR)\encoder-h261.sbr"
	-@erase "$(INTDIR)\encoder-h263.obj"
	-@erase "$(INTDIR)\encoder-h263.sbr"
	-@erase "$(INTDIR)\encoder-h263v2.obj"
	-@erase "$(INTDIR)\encoder-h263v2.sbr"
	-@erase "$(INTDIR)\encoder-jpeg.obj"
	-@erase "$(INTDIR)\encoder-jpeg.sbr"
	-@erase "$(INTDIR)\encoder-nv.obj"
	-@erase "$(INTDIR)\encoder-nv.sbr"
	-@erase "$(INTDIR)\encoder-pvh.obj"
	-@erase "$(INTDIR)\encoder-pvh.sbr"
	-@erase "$(INTDIR)\encoder-raw.obj"
	-@erase "$(INTDIR)\encoder-raw.sbr"
	-@erase "$(INTDIR)\entry.obj"
	-@erase "$(INTDIR)\entry.sbr"
	-@erase "$(INTDIR)\fdct.obj"
	-@erase "$(INTDIR)\fdct.sbr"
	-@erase "$(INTDIR)\framer-jpeg.obj"
	-@erase "$(INTDIR)\framer-jpeg.sbr"
	-@erase "$(INTDIR)\getblk.obj"
	-@erase "$(INTDIR)\getblk.sbr"
	-@erase "$(INTDIR)\getgob.obj"
	-@erase "$(INTDIR)\getgob.sbr"
	-@erase "$(INTDIR)\gethdr.obj"
	-@erase "$(INTDIR)\gethdr.sbr"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt.sbr"
	-@erase "$(INTDIR)\getvlc.obj"
	-@erase "$(INTDIR)\getvlc.sbr"
	-@erase "$(INTDIR)\grabber-still.obj"
	-@erase "$(INTDIR)\grabber-still.sbr"
	-@erase "$(INTDIR)\grabber-win32.obj"
	-@erase "$(INTDIR)\grabber-win32.sbr"
	-@erase "$(INTDIR)\grabber.obj"
	-@erase "$(INTDIR)\grabber.sbr"
	-@erase "$(INTDIR)\group-ipc.obj"
	-@erase "$(INTDIR)\group-ipc.sbr"
	-@erase "$(INTDIR)\h263dec.obj"
	-@erase "$(INTDIR)\h263dec.sbr"
	-@erase "$(INTDIR)\h263enc.obj"
	-@erase "$(INTDIR)\h263enc.sbr"
	-@erase "$(INTDIR)\h263mux.obj"
	-@erase "$(INTDIR)\h263mux.sbr"
	-@erase "$(INTDIR)\h263rtp.obj"
	-@erase "$(INTDIR)\h263rtp.sbr"
	-@erase "$(INTDIR)\huffcode.obj"
	-@erase "$(INTDIR)\huffcode.sbr"
	-@erase "$(INTDIR)\idctdec.obj"
	-@erase "$(INTDIR)\idctdec.sbr"
	-@erase "$(INTDIR)\idctenc.obj"
	-@erase "$(INTDIR)\idctenc.sbr"
	-@erase "$(INTDIR)\idlecallback.obj"
	-@erase "$(INTDIR)\idlecallback.sbr"
	-@erase "$(INTDIR)\inet.obj"
	-@erase "$(INTDIR)\inet.sbr"
	-@erase "$(INTDIR)\inet6.obj"
	-@erase "$(INTDIR)\inet6.sbr"
	-@erase "$(INTDIR)\input.obj"
	-@erase "$(INTDIR)\input.sbr"
	-@erase "$(INTDIR)\iohandler.obj"
	-@erase "$(INTDIR)\iohandler.sbr"
	-@erase "$(INTDIR)\jpeg.obj"
	-@erase "$(INTDIR)\jpeg.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\mbus_engine.obj"
	-@erase "$(INTDIR)\mbus_engine.sbr"
	-@erase "$(INTDIR)\mbus_handler.obj"
	-@erase "$(INTDIR)\mbus_handler.sbr"
	-@erase "$(INTDIR)\md5c.obj"
	-@erase "$(INTDIR)\md5c.sbr"
	-@erase "$(INTDIR)\media-timer.obj"
	-@erase "$(INTDIR)\media-timer.sbr"
	-@erase "$(INTDIR)\module.obj"
	-@erase "$(INTDIR)\module.sbr"
	-@erase "$(INTDIR)\motion.obj"
	-@erase "$(INTDIR)\motion.sbr"
	-@erase "$(INTDIR)\net-addr.obj"
	-@erase "$(INTDIR)\net-addr.sbr"
	-@erase "$(INTDIR)\net-ip.obj"
	-@erase "$(INTDIR)\net-ip.sbr"
	-@erase "$(INTDIR)\net-ipv6.obj"
	-@erase "$(INTDIR)\net-ipv6.sbr"
	-@erase "$(INTDIR)\net.obj"
	-@erase "$(INTDIR)\net.sbr"
	-@erase "$(INTDIR)\p64.obj"
	-@erase "$(INTDIR)\p64.sbr"
	-@erase "$(INTDIR)\pktbuf-rtp.obj"
	-@erase "$(INTDIR)\pktbuf-rtp.sbr"
	-@erase "$(INTDIR)\pktbuf.obj"
	-@erase "$(INTDIR)\pktbuf.sbr"
	-@erase "$(INTDIR)\pkttbl.obj"
	-@erase "$(INTDIR)\pkttbl.sbr"
	-@erase "$(INTDIR)\pvh-huff.obj"
	-@erase "$(INTDIR)\pvh-huff.sbr"
	-@erase "$(INTDIR)\random.obj"
	-@erase "$(INTDIR)\random.sbr"
	-@erase "$(INTDIR)\rate-variable.obj"
	-@erase "$(INTDIR)\rate-variable.sbr"
	-@erase "$(INTDIR)\recon.obj"
	-@erase "$(INTDIR)\recon.sbr"
	-@erase "$(INTDIR)\reconh263.obj"
	-@erase "$(INTDIR)\reconh263.sbr"
	-@erase "$(INTDIR)\renderer-window.obj"
	-@erase "$(INTDIR)\renderer-window.sbr"
	-@erase "$(INTDIR)\renderer.obj"
	-@erase "$(INTDIR)\renderer.sbr"
	-@erase "$(INTDIR)\rgb-converter.obj"
	-@erase "$(INTDIR)\rgb-converter.sbr"
	-@erase "$(INTDIR)\sac.obj"
	-@erase "$(INTDIR)\sac.sbr"
	-@erase "$(INTDIR)\session.obj"
	-@erase "$(INTDIR)\session.sbr"
	-@erase "$(INTDIR)\source.obj"
	-@erase "$(INTDIR)\source.sbr"
	-@erase "$(INTDIR)\strtol.obj"
	-@erase "$(INTDIR)\strtol.sbr"
	-@erase "$(INTDIR)\strtoul.obj"
	-@erase "$(INTDIR)\strtoul.sbr"
	-@erase "$(INTDIR)\Tcl.obj"
	-@erase "$(INTDIR)\Tcl.sbr"
	-@erase "$(INTDIR)\Tcl2.obj"
	-@erase "$(INTDIR)\Tcl2.sbr"
	-@erase "$(INTDIR)\timer.obj"
	-@erase "$(INTDIR)\timer.sbr"
	-@erase "$(INTDIR)\tkerror.obj"
	-@erase "$(INTDIR)\tkerror.sbr"
	-@erase "$(INTDIR)\tkStripchart.obj"
	-@erase "$(INTDIR)\tkStripchart.sbr"
	-@erase "$(INTDIR)\tkWinColor.obj"
	-@erase "$(INTDIR)\tkWinColor.sbr"
	-@erase "$(INTDIR)\transcoder-jpeg.obj"
	-@erase "$(INTDIR)\transcoder-jpeg.sbr"
	-@erase "$(INTDIR)\transmitter.obj"
	-@erase "$(INTDIR)\transmitter.sbr"
	-@erase "$(INTDIR)\ui-ctrlmenu.obj"
	-@erase "$(INTDIR)\ui-ctrlmenu.sbr"
	-@erase "$(INTDIR)\ui-extout.obj"
	-@erase "$(INTDIR)\ui-extout.sbr"
	-@erase "$(INTDIR)\ui-grabber.obj"
	-@erase "$(INTDIR)\ui-grabber.sbr"
	-@erase "$(INTDIR)\ui-help.obj"
	-@erase "$(INTDIR)\ui-help.sbr"
	-@erase "$(INTDIR)\ui-main.obj"
	-@erase "$(INTDIR)\ui-main.sbr"
	-@erase "$(INTDIR)\ui-relate.obj"
	-@erase "$(INTDIR)\ui-relate.sbr"
	-@erase "$(INTDIR)\ui-resource.obj"
	-@erase "$(INTDIR)\ui-resource.sbr"
	-@erase "$(INTDIR)\ui-srclist.obj"
	-@erase "$(INTDIR)\ui-srclist.sbr"
	-@erase "$(INTDIR)\ui-stats.obj"
	-@erase "$(INTDIR)\ui-stats.sbr"
	-@erase "$(INTDIR)\ui-switcher.obj"
	-@erase "$(INTDIR)\ui-switcher.sbr"
	-@erase "$(INTDIR)\ui-util.obj"
	-@erase "$(INTDIR)\ui-util.sbr"
	-@erase "$(INTDIR)\ui-win32.obj"
	-@erase "$(INTDIR)\ui-win32.sbr"
	-@erase "$(INTDIR)\ui-windows.obj"
	-@erase "$(INTDIR)\ui-windows.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\version.obj"
	-@erase "$(INTDIR)\version.sbr"
	-@erase "$(INTDIR)\vw.obj"
	-@erase "$(INTDIR)\vw.sbr"
	-@erase "$(INTDIR)\win32.obj"
	-@erase "$(INTDIR)\win32.sbr"
	-@erase "$(INTDIR)\win32X.obj"
	-@erase "$(INTDIR)\win32X.sbr"
	-@erase "$(OUTDIR)\vic.bsc"
	-@erase "$(OUTDIR)\vic.exe"
	-@erase "$(OUTDIR)\vic.ilk"
	-@erase "$(OUTDIR)\vic.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

vic=rc.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /D "_WINDOWS" /D "DEBUG" /D "SASR" /D "_DEBUG" /D "_WIN95" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vic.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cf-confbus.sbr" \
	"$(INTDIR)\cf-main.sbr" \
	"$(INTDIR)\cf-network.sbr" \
	"$(INTDIR)\cf-tm.sbr" \
	"$(INTDIR)\cf-util.sbr" \
	"$(INTDIR)\entry.sbr" \
	"$(INTDIR)\tkerror.sbr" \
	"$(INTDIR)\ui-ctrlmenu.sbr" \
	"$(INTDIR)\ui-extout.sbr" \
	"$(INTDIR)\ui-grabber.sbr" \
	"$(INTDIR)\ui-help.sbr" \
	"$(INTDIR)\ui-main.sbr" \
	"$(INTDIR)\ui-relate.sbr" \
	"$(INTDIR)\ui-resource.sbr" \
	"$(INTDIR)\ui-srclist.sbr" \
	"$(INTDIR)\ui-stats.sbr" \
	"$(INTDIR)\ui-switcher.sbr" \
	"$(INTDIR)\ui-util.sbr" \
	"$(INTDIR)\ui-win32.sbr" \
	"$(INTDIR)\ui-windows.sbr" \
	"$(INTDIR)\communicator.sbr" \
	"$(INTDIR)\confbus.sbr" \
	"$(INTDIR)\crypt-des.sbr" \
	"$(INTDIR)\crypt-dull.sbr" \
	"$(INTDIR)\crypt.sbr" \
	"$(INTDIR)\group-ipc.sbr" \
	"$(INTDIR)\inet.sbr" \
	"$(INTDIR)\inet6.sbr" \
	"$(INTDIR)\mbus_engine.sbr" \
	"$(INTDIR)\mbus_handler.sbr" \
	"$(INTDIR)\net-addr.sbr" \
	"$(INTDIR)\net-ip.sbr" \
	"$(INTDIR)\net-ipv6.sbr" \
	"$(INTDIR)\net.sbr" \
	"$(INTDIR)\pktbuf.sbr" \
	"$(INTDIR)\pkttbl.sbr" \
	"$(INTDIR)\bitIn.sbr" \
	"$(INTDIR)\bitOut.sbr" \
	"$(INTDIR)\block.sbr" \
	"$(INTDIR)\code.sbr" \
	"$(INTDIR)\fdct.sbr" \
	"$(INTDIR)\getblk.sbr" \
	"$(INTDIR)\getgob.sbr" \
	"$(INTDIR)\gethdr.sbr" \
	"$(INTDIR)\getvlc.sbr" \
	"$(INTDIR)\h263dec.sbr" \
	"$(INTDIR)\h263enc.sbr" \
	"$(INTDIR)\h263mux.sbr" \
	"$(INTDIR)\h263rtp.sbr" \
	"$(INTDIR)\idctdec.sbr" \
	"$(INTDIR)\idctenc.sbr" \
	"$(INTDIR)\input.sbr" \
	"$(INTDIR)\motion.sbr" \
	"$(INTDIR)\recon.sbr" \
	"$(INTDIR)\reconh263.sbr" \
	"$(INTDIR)\sac.sbr" \
	"$(INTDIR)\bv.sbr" \
	"$(INTDIR)\cellb_tables.sbr" \
	"$(INTDIR)\compositor.sbr" \
	"$(INTDIR)\dct.sbr" \
	"$(INTDIR)\decoder-bvc.sbr" \
	"$(INTDIR)\decoder-cellb.sbr" \
	"$(INTDIR)\decoder-h261.sbr" \
	"$(INTDIR)\decoder-h261v1.sbr" \
	"$(INTDIR)\decoder-h263.sbr" \
	"$(INTDIR)\decoder-h263v2.sbr" \
	"$(INTDIR)\decoder-jpeg.sbr" \
	"$(INTDIR)\decoder-nv.sbr" \
	"$(INTDIR)\decoder-pvh.sbr" \
	"$(INTDIR)\decoder-raw.sbr" \
	"$(INTDIR)\decoder.sbr" \
	"$(INTDIR)\encoder-bvc.sbr" \
	"$(INTDIR)\encoder-cellb.sbr" \
	"$(INTDIR)\encoder-h261.sbr" \
	"$(INTDIR)\encoder-h263.sbr" \
	"$(INTDIR)\encoder-h263v2.sbr" \
	"$(INTDIR)\encoder-jpeg.sbr" \
	"$(INTDIR)\encoder-nv.sbr" \
	"$(INTDIR)\encoder-pvh.sbr" \
	"$(INTDIR)\encoder-raw.sbr" \
	"$(INTDIR)\framer-jpeg.sbr" \
	"$(INTDIR)\huffcode.sbr" \
	"$(INTDIR)\jpeg.sbr" \
	"$(INTDIR)\p64.sbr" \
	"$(INTDIR)\pvh-huff.sbr" \
	"$(INTDIR)\transcoder-jpeg.sbr" \
	"$(INTDIR)\cm0.sbr" \
	"$(INTDIR)\cm1.sbr" \
	"$(INTDIR)\color-dither.sbr" \
	"$(INTDIR)\color-ed.sbr" \
	"$(INTDIR)\color-gray.sbr" \
	"$(INTDIR)\color-hi.sbr" \
	"$(INTDIR)\color-hist.sbr" \
	"$(INTDIR)\color-mono.sbr" \
	"$(INTDIR)\color-pseudo.sbr" \
	"$(INTDIR)\color-quant.sbr" \
	"$(INTDIR)\color-true.sbr" \
	"$(INTDIR)\color-yuv.sbr" \
	"$(INTDIR)\color.sbr" \
	"$(INTDIR)\renderer-window.sbr" \
	"$(INTDIR)\renderer.sbr" \
	"$(INTDIR)\rgb-converter.sbr" \
	"$(INTDIR)\vw.sbr" \
	"$(INTDIR)\pktbuf-rtp.sbr" \
	"$(INTDIR)\session.sbr" \
	"$(INTDIR)\source.sbr" \
	"$(INTDIR)\transmitter.sbr" \
	"$(INTDIR)\win32.sbr" \
	"$(INTDIR)\win32X.sbr" \
	"$(INTDIR)\getopt.sbr" \
	"$(INTDIR)\idlecallback.sbr" \
	"$(INTDIR)\iohandler.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\md5c.sbr" \
	"$(INTDIR)\media-timer.sbr" \
	"$(INTDIR)\module.sbr" \
	"$(INTDIR)\random.sbr" \
	"$(INTDIR)\rate-variable.sbr" \
	"$(INTDIR)\strtol.sbr" \
	"$(INTDIR)\strtoul.sbr" \
	"$(INTDIR)\Tcl.sbr" \
	"$(INTDIR)\Tcl2.sbr" \
	"$(INTDIR)\timer.sbr" \
	"$(INTDIR)\tkStripchart.sbr" \
	"$(INTDIR)\tkWinColor.sbr" \
	"$(INTDIR)\version.sbr" \
	"$(INTDIR)\assistor-list.sbr" \
	"$(INTDIR)\device.sbr" \
	"$(INTDIR)\grabber-still.sbr" \
	"$(INTDIR)\grabber-win32.sbr" \
	"$(INTDIR)\grabber.sbr"

"$(OUTDIR)\vic.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=tklib.lib tcllib.lib libh263.lib libh263coder.lib wsock32.lib Ws2_32.lib winmm.lib kernel32.lib user32.lib gdi32.lib shell32.lib vfw32.lib advapi32.lib comdlg32.lib uclmm.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\vic.pdb" /debug /machine:I386 /out:"$(OUTDIR)\vic.exe" /libpath:"..\common\Debug" /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"codec\tmndec" /libpath:"codec\tmn-x" 
LINK32_OBJS= \
	"$(INTDIR)\cf-confbus.obj" \
	"$(INTDIR)\cf-main.obj" \
	"$(INTDIR)\cf-network.obj" \
	"$(INTDIR)\cf-tm.obj" \
	"$(INTDIR)\cf-util.obj" \
	"$(INTDIR)\entry.obj" \
	"$(INTDIR)\tkerror.obj" \
	"$(INTDIR)\ui-ctrlmenu.obj" \
	"$(INTDIR)\ui-extout.obj" \
	"$(INTDIR)\ui-grabber.obj" \
	"$(INTDIR)\ui-help.obj" \
	"$(INTDIR)\ui-main.obj" \
	"$(INTDIR)\ui-relate.obj" \
	"$(INTDIR)\ui-resource.obj" \
	"$(INTDIR)\ui-srclist.obj" \
	"$(INTDIR)\ui-stats.obj" \
	"$(INTDIR)\ui-switcher.obj" \
	"$(INTDIR)\ui-util.obj" \
	"$(INTDIR)\ui-win32.obj" \
	"$(INTDIR)\ui-windows.obj" \
	"$(INTDIR)\communicator.obj" \
	"$(INTDIR)\confbus.obj" \
	"$(INTDIR)\crypt-des.obj" \
	"$(INTDIR)\crypt-dull.obj" \
	"$(INTDIR)\crypt.obj" \
	"$(INTDIR)\group-ipc.obj" \
	"$(INTDIR)\inet.obj" \
	"$(INTDIR)\inet6.obj" \
	"$(INTDIR)\mbus_engine.obj" \
	"$(INTDIR)\mbus_handler.obj" \
	"$(INTDIR)\net-addr.obj" \
	"$(INTDIR)\net-ip.obj" \
	"$(INTDIR)\net-ipv6.obj" \
	"$(INTDIR)\net.obj" \
	"$(INTDIR)\pktbuf.obj" \
	"$(INTDIR)\pkttbl.obj" \
	"$(INTDIR)\bitIn.obj" \
	"$(INTDIR)\bitOut.obj" \
	"$(INTDIR)\block.obj" \
	"$(INTDIR)\code.obj" \
	"$(INTDIR)\fdct.obj" \
	"$(INTDIR)\getblk.obj" \
	"$(INTDIR)\getgob.obj" \
	"$(INTDIR)\gethdr.obj" \
	"$(INTDIR)\getvlc.obj" \
	"$(INTDIR)\h263dec.obj" \
	"$(INTDIR)\h263enc.obj" \
	"$(INTDIR)\h263mux.obj" \
	"$(INTDIR)\h263rtp.obj" \
	"$(INTDIR)\idctdec.obj" \
	"$(INTDIR)\idctenc.obj" \
	"$(INTDIR)\input.obj" \
	"$(INTDIR)\motion.obj" \
	"$(INTDIR)\recon.obj" \
	"$(INTDIR)\reconh263.obj" \
	"$(INTDIR)\sac.obj" \
	"$(INTDIR)\bv.obj" \
	"$(INTDIR)\cellb_tables.obj" \
	"$(INTDIR)\compositor.obj" \
	"$(INTDIR)\dct.obj" \
	"$(INTDIR)\decoder-bvc.obj" \
	"$(INTDIR)\decoder-cellb.obj" \
	"$(INTDIR)\decoder-h261.obj" \
	"$(INTDIR)\decoder-h261v1.obj" \
	"$(INTDIR)\decoder-h263.obj" \
	"$(INTDIR)\decoder-h263v2.obj" \
	"$(INTDIR)\decoder-jpeg.obj" \
	"$(INTDIR)\decoder-nv.obj" \
	"$(INTDIR)\decoder-pvh.obj" \
	"$(INTDIR)\decoder-raw.obj" \
	"$(INTDIR)\decoder.obj" \
	"$(INTDIR)\encoder-bvc.obj" \
	"$(INTDIR)\encoder-cellb.obj" \
	"$(INTDIR)\encoder-h261.obj" \
	"$(INTDIR)\encoder-h263.obj" \
	"$(INTDIR)\encoder-h263v2.obj" \
	"$(INTDIR)\encoder-jpeg.obj" \
	"$(INTDIR)\encoder-nv.obj" \
	"$(INTDIR)\encoder-pvh.obj" \
	"$(INTDIR)\encoder-raw.obj" \
	"$(INTDIR)\framer-jpeg.obj" \
	"$(INTDIR)\huffcode.obj" \
	"$(INTDIR)\jpeg.obj" \
	"$(INTDIR)\p64.obj" \
	"$(INTDIR)\pvh-huff.obj" \
	"$(INTDIR)\transcoder-jpeg.obj" \
	"$(INTDIR)\cm0.obj" \
	"$(INTDIR)\cm1.obj" \
	"$(INTDIR)\color-dither.obj" \
	"$(INTDIR)\color-ed.obj" \
	"$(INTDIR)\color-gray.obj" \
	"$(INTDIR)\color-hi.obj" \
	"$(INTDIR)\color-hist.obj" \
	"$(INTDIR)\color-mono.obj" \
	"$(INTDIR)\color-pseudo.obj" \
	"$(INTDIR)\color-quant.obj" \
	"$(INTDIR)\color-true.obj" \
	"$(INTDIR)\color-yuv.obj" \
	"$(INTDIR)\color.obj" \
	"$(INTDIR)\renderer-window.obj" \
	"$(INTDIR)\renderer.obj" \
	"$(INTDIR)\rgb-converter.obj" \
	"$(INTDIR)\vw.obj" \
	"$(INTDIR)\pktbuf-rtp.obj" \
	"$(INTDIR)\session.obj" \
	"$(INTDIR)\source.obj" \
	"$(INTDIR)\transmitter.obj" \
	"$(INTDIR)\win32.obj" \
	"$(INTDIR)\win32X.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\idlecallback.obj" \
	"$(INTDIR)\iohandler.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\md5c.obj" \
	"$(INTDIR)\media-timer.obj" \
	"$(INTDIR)\module.obj" \
	"$(INTDIR)\random.obj" \
	"$(INTDIR)\rate-variable.obj" \
	"$(INTDIR)\strtol.obj" \
	"$(INTDIR)\strtoul.obj" \
	"$(INTDIR)\Tcl.obj" \
	"$(INTDIR)\Tcl2.obj" \
	"$(INTDIR)\timer.obj" \
	"$(INTDIR)\tkStripchart.obj" \
	"$(INTDIR)\tkWinColor.obj" \
	"$(INTDIR)\version.obj" \
	"$(INTDIR)\assistor-list.obj" \
	"$(INTDIR)\device.obj" \
	"$(INTDIR)\grabber-still.obj" \
	"$(INTDIR)\grabber-win32.obj" \
	"$(INTDIR)\grabber.obj" \
	"..\tk-8.0\win\tk.res" \
	"..\common\Debug\uclmm.lib" \
	"..\tcl-8.0\win\Debug\tcllib.lib" \
	"..\tk-8.0\win\Debug\tklib.lib" \
	".\codec\tmndec\libh263.lib" \
	".\codec\tmn-x\libh263coder.lib"

"$(OUTDIR)\vic.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("vic.dep")
!INCLUDE "vic.dep"
!ELSE 
!MESSAGE Warning: cannot find "vic.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "vic - Win32 Release" || "$(CFG)" == "vic - Win32 Debug IPv6" || "$(CFG)" == "vic - Win32 Debug IPv6 Musica" || "$(CFG)" == "vic - Win32 Debug"
SOURCE=".\tcl\generated\cf-confbus.cpp"

"$(INTDIR)\cf-confbus.obj"	"$(INTDIR)\cf-confbus.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\cf-main.cpp"

"$(INTDIR)\cf-main.obj"	"$(INTDIR)\cf-main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\cf-network.cpp"

"$(INTDIR)\cf-network.obj"	"$(INTDIR)\cf-network.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\cf-tm.cpp"

"$(INTDIR)\cf-tm.obj"	"$(INTDIR)\cf-tm.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\cf-util.cpp"

"$(INTDIR)\cf-util.obj"	"$(INTDIR)\cf-util.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\tcl\generated\entry.cpp

"$(INTDIR)\entry.obj"	"$(INTDIR)\entry.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\tcl\generated\tkerror.cpp

"$(INTDIR)\tkerror.obj"	"$(INTDIR)\tkerror.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-ctrlmenu.cpp"

"$(INTDIR)\ui-ctrlmenu.obj"	"$(INTDIR)\ui-ctrlmenu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-extout.cpp"

"$(INTDIR)\ui-extout.obj"	"$(INTDIR)\ui-extout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-grabber.cpp"

"$(INTDIR)\ui-grabber.obj"	"$(INTDIR)\ui-grabber.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-help.cpp"

"$(INTDIR)\ui-help.obj"	"$(INTDIR)\ui-help.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-main.cpp"

"$(INTDIR)\ui-main.obj"	"$(INTDIR)\ui-main.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-relate.cpp"

"$(INTDIR)\ui-relate.obj"	"$(INTDIR)\ui-relate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-resource.cpp"

"$(INTDIR)\ui-resource.obj"	"$(INTDIR)\ui-resource.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-srclist.cpp"

"$(INTDIR)\ui-srclist.obj"	"$(INTDIR)\ui-srclist.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-stats.cpp"

"$(INTDIR)\ui-stats.obj"	"$(INTDIR)\ui-stats.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-switcher.cpp"

"$(INTDIR)\ui-switcher.obj"	"$(INTDIR)\ui-switcher.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-util.cpp"

"$(INTDIR)\ui-util.obj"	"$(INTDIR)\ui-util.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-win32.cpp"

"$(INTDIR)\ui-win32.obj"	"$(INTDIR)\ui-win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\generated\ui-windows.cpp"

"$(INTDIR)\ui-windows.obj"	"$(INTDIR)\ui-windows.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\tcl\cf-confbus.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

".\tcl\generated\cf-confbus.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

".\tcl\generated\cf-confbus.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

".\tcl\generated\cf-confbus.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

".\tcl\generated\cf-confbus.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\cf-main.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

".\tcl\generated\cf-main.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

".\tcl\generated\cf-main.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

".\tcl\generated\cf-main.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

".\tcl\generated\cf-main.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\cf-network.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

".\tcl\generated\cf-network.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

".\tcl\generated\cf-network.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

".\tcl\generated\cf-network.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

".\tcl\generated\cf-network.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\cf-tm.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

".\tcl\generated\cf-tm.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

".\tcl\generated\cf-tm.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

".\tcl\generated\cf-tm.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

".\tcl\generated\cf-tm.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\cf-util.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

".\tcl\generated\cf-util.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

".\tcl\generated\cf-util.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

".\tcl\generated\cf-util.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

".\tcl\generated\cf-util.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=.\tcl\entry.tcl

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=.\tcl\entry.tcl
InputName=entry

".\tcl\generated\entry.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=.\tcl\entry.tcl
InputName=entry

".\tcl\generated\entry.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=.\tcl\entry.tcl
InputName=entry

".\tcl\generated\entry.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=.\tcl\entry.tcl
InputName=entry

".\tcl\generated\entry.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=.\tcl\tkerror.tcl

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=.\tcl\tkerror.tcl
InputName=tkerror

".\tcl\generated\tkerror.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=.\tcl\tkerror.tcl
InputName=tkerror

".\tcl\generated\tkerror.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=.\tcl\tkerror.tcl
InputName=tkerror

".\tcl\generated\tkerror.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=.\tcl\tkerror.tcl
InputName=tkerror

".\tcl\generated\tkerror.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-ctrlmenu.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

".\tcl\generated\ui-ctrlmenu.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

".\tcl\generated\ui-ctrlmenu.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

".\tcl\generated\ui-ctrlmenu.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

".\tcl\generated\ui-ctrlmenu.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-extout.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

".\tcl\generated\ui-extout.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

".\tcl\generated\ui-extout.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

".\tcl\generated\ui-extout.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

".\tcl\generated\ui-extout.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-grabber.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

".\tcl\generated\ui-grabber.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

".\tcl\generated\ui-grabber.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

".\tcl\generated\ui-grabber.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

".\tcl\generated\ui-grabber.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-help.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

".\tcl\generated\ui-help.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

".\tcl\generated\ui-help.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

".\tcl\generated\ui-help.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

".\tcl\generated\ui-help.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-main.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

".\tcl\generated\ui-main.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

".\tcl\generated\ui-main.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

".\tcl\generated\ui-main.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

".\tcl\generated\ui-main.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-relate.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

".\tcl\generated\ui-relate.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

".\tcl\generated\ui-relate.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

".\tcl\generated\ui-relate.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

".\tcl\generated\ui-relate.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-resource.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

".\tcl\generated\ui-resource.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

".\tcl\generated\ui-resource.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

".\tcl\generated\ui-resource.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

".\tcl\generated\ui-resource.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-srclist.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

".\tcl\generated\ui-srclist.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

".\tcl\generated\ui-srclist.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

".\tcl\generated\ui-srclist.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

".\tcl\generated\ui-srclist.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-stats.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

".\tcl\generated\ui-stats.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

".\tcl\generated\ui-stats.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

".\tcl\generated\ui-stats.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

".\tcl\generated\ui-stats.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-switcher.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

".\tcl\generated\ui-switcher.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

".\tcl\generated\ui-switcher.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

".\tcl\generated\ui-switcher.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

".\tcl\generated\ui-switcher.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-util.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

".\tcl\generated\ui-util.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

".\tcl\generated\ui-util.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

".\tcl\generated\ui-util.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

".\tcl\generated\ui-util.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-win32.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

".\tcl\generated\ui-win32.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

".\tcl\generated\ui-win32.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

".\tcl\generated\ui-win32.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

".\tcl\generated\ui-win32.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=".\tcl\ui-windows.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

".\tcl\generated\ui-windows.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

".\tcl\generated\ui-windows.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

".\tcl\generated\ui-windows.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

".\tcl\generated\ui-windows.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp
<< 
	

!ENDIF 

SOURCE=.\net\communicator.cpp

"$(INTDIR)\communicator.obj"	"$(INTDIR)\communicator.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\net\confbus.cpp

"$(INTDIR)\confbus.obj"	"$(INTDIR)\confbus.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\net\crypt-des.cpp"

"$(INTDIR)\crypt-des.obj"	"$(INTDIR)\crypt-des.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\net\crypt-dull.cpp"

"$(INTDIR)\crypt-dull.obj"	"$(INTDIR)\crypt-dull.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\net\crypt.cpp

"$(INTDIR)\crypt.obj"	"$(INTDIR)\crypt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\net\group-ipc.cpp"

"$(INTDIR)\group-ipc.obj"	"$(INTDIR)\group-ipc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\net\inet.c

"$(INTDIR)\inet.obj"	"$(INTDIR)\inet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\net\inet6.c

!IF  "$(CFG)" == "vic - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /D "NDEBUG" /D "_WINDOWS" /D "SASR" /D "WIN32" /D "HAVE_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\inet6.obj"	"$(INTDIR)\inet6.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /Zi /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "..\IPv6Kit\inc" /I "g:\DDK\inc" /D "DEBUG" /D "_DEBUG" /D "DEBUG_MEM" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 

"$(INTDIR)\inet6.obj"	"$(INTDIR)\inet6.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /Zi /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "..\MUSICA\WINSOCK6" /D "_WINDOWS" /D "DEBUG" /D "SASR" /D "_DEBUG" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "MUSICA_IPV6" /D "_POSIX" /D "_WINNT" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /D "NEED_ADDRINFO_H" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 

"$(INTDIR)\inet6.obj"	"$(INTDIR)\inet6.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /D "_WINDOWS" /D "DEBUG" /D "SASR" /D "_DEBUG" /D "_WIN95" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 

"$(INTDIR)\inet6.obj"	"$(INTDIR)\inet6.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\net\mbus_engine.cpp

"$(INTDIR)\mbus_engine.obj"	"$(INTDIR)\mbus_engine.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\net\mbus_handler.cpp

"$(INTDIR)\mbus_handler.obj"	"$(INTDIR)\mbus_handler.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\net\net-addr.cpp"

"$(INTDIR)\net-addr.obj"	"$(INTDIR)\net-addr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\net\net-ip.cpp"

!IF  "$(CFG)" == "vic - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /D "NDEBUG" /D "_WINDOWS" /D "SASR" /D "WIN32" /D "HAVE_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\net-ip.obj"	"$(INTDIR)\net-ip.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /Zi /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "..\IPv6Kit\inc" /I "g:\DDK\inc" /D "DEBUG" /D "_DEBUG" /D "DEBUG_MEM" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 

"$(INTDIR)\net-ip.obj"	"$(INTDIR)\net-ip.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /Zi /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "..\MUSICA\WINSOCK6" /D "_WINDOWS" /D "DEBUG" /D "SASR" /D "_DEBUG" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "MUSICA_IPV6" /D "_POSIX" /D "_WINNT" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 

"$(INTDIR)\net-ip.obj"	"$(INTDIR)\net-ip.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /ZI /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /D "_WINDOWS" /D "DEBUG" /D "SASR" /D "_DEBUG" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /Fr"$(INTDIR)\\" /Fp"$(INTDIR)\vic.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /I ../common /c 

"$(INTDIR)\net-ip.obj"	"$(INTDIR)\net-ip.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=".\net\net-ipv6.cpp"

"$(INTDIR)\net-ipv6.obj"	"$(INTDIR)\net-ipv6.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\net\net.cpp

"$(INTDIR)\net.obj"	"$(INTDIR)\net.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\net\pktbuf.cpp

"$(INTDIR)\pktbuf.obj"	"$(INTDIR)\pktbuf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\net\pkttbl.cpp

"$(INTDIR)\pkttbl.obj"	"$(INTDIR)\pkttbl.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\bitIn.c

"$(INTDIR)\bitIn.obj"	"$(INTDIR)\bitIn.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\bitOut.c

"$(INTDIR)\bitOut.obj"	"$(INTDIR)\bitOut.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\block.c

"$(INTDIR)\block.obj"	"$(INTDIR)\block.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\code.c

"$(INTDIR)\code.obj"	"$(INTDIR)\code.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\fdct.c

"$(INTDIR)\fdct.obj"	"$(INTDIR)\fdct.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\getblk.c

"$(INTDIR)\getblk.obj"	"$(INTDIR)\getblk.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\getgob.c

"$(INTDIR)\getgob.obj"	"$(INTDIR)\getgob.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\gethdr.c

"$(INTDIR)\gethdr.obj"	"$(INTDIR)\gethdr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\getvlc.c

"$(INTDIR)\getvlc.obj"	"$(INTDIR)\getvlc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\h263dec.c

"$(INTDIR)\h263dec.obj"	"$(INTDIR)\h263dec.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\h263enc.c

"$(INTDIR)\h263enc.obj"	"$(INTDIR)\h263enc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\h263mux.c

"$(INTDIR)\h263mux.obj"	"$(INTDIR)\h263mux.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\h263rtp.c

"$(INTDIR)\h263rtp.obj"	"$(INTDIR)\h263rtp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\idctdec.c

"$(INTDIR)\idctdec.obj"	"$(INTDIR)\idctdec.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\idctenc.c

"$(INTDIR)\idctenc.obj"	"$(INTDIR)\idctenc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\input.c

"$(INTDIR)\input.obj"	"$(INTDIR)\input.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\motion.c

"$(INTDIR)\motion.obj"	"$(INTDIR)\motion.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\recon.c

"$(INTDIR)\recon.obj"	"$(INTDIR)\recon.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\reconh263.c

"$(INTDIR)\reconh263.obj"	"$(INTDIR)\reconh263.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\h263\sac.c

"$(INTDIR)\sac.obj"	"$(INTDIR)\sac.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\bv.c

"$(INTDIR)\bv.obj"	"$(INTDIR)\bv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\cellb_tables.c

"$(INTDIR)\cellb_tables.obj"	"$(INTDIR)\cellb_tables.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\compositor.cpp

"$(INTDIR)\compositor.obj"	"$(INTDIR)\compositor.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\dct.cpp

"$(INTDIR)\dct.obj"	"$(INTDIR)\dct.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-bvc.cpp"

"$(INTDIR)\decoder-bvc.obj"	"$(INTDIR)\decoder-bvc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-cellb.cpp"

"$(INTDIR)\decoder-cellb.obj"	"$(INTDIR)\decoder-cellb.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-h261.cpp"

"$(INTDIR)\decoder-h261.obj"	"$(INTDIR)\decoder-h261.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-h261v1.cpp"

"$(INTDIR)\decoder-h261v1.obj"	"$(INTDIR)\decoder-h261v1.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-h263.cpp"

"$(INTDIR)\decoder-h263.obj"	"$(INTDIR)\decoder-h263.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-h263v2.cpp"

"$(INTDIR)\decoder-h263v2.obj"	"$(INTDIR)\decoder-h263v2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-jpeg.cpp"

"$(INTDIR)\decoder-jpeg.obj"	"$(INTDIR)\decoder-jpeg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-nv.cpp"

"$(INTDIR)\decoder-nv.obj"	"$(INTDIR)\decoder-nv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-pvh.cpp"

"$(INTDIR)\decoder-pvh.obj"	"$(INTDIR)\decoder-pvh.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\decoder-raw.cpp"

"$(INTDIR)\decoder-raw.obj"	"$(INTDIR)\decoder-raw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\decoder.cpp

"$(INTDIR)\decoder.obj"	"$(INTDIR)\decoder.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-bvc.cpp"

"$(INTDIR)\encoder-bvc.obj"	"$(INTDIR)\encoder-bvc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-cellb.cpp"

"$(INTDIR)\encoder-cellb.obj"	"$(INTDIR)\encoder-cellb.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-h261.cpp"

"$(INTDIR)\encoder-h261.obj"	"$(INTDIR)\encoder-h261.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-h263.cpp"

"$(INTDIR)\encoder-h263.obj"	"$(INTDIR)\encoder-h263.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-h263v2.cpp"

"$(INTDIR)\encoder-h263v2.obj"	"$(INTDIR)\encoder-h263v2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-jpeg.cpp"

"$(INTDIR)\encoder-jpeg.obj"	"$(INTDIR)\encoder-jpeg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-nv.cpp"

"$(INTDIR)\encoder-nv.obj"	"$(INTDIR)\encoder-nv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-pvh.cpp"

"$(INTDIR)\encoder-pvh.obj"	"$(INTDIR)\encoder-pvh.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\encoder-raw.cpp"

"$(INTDIR)\encoder-raw.obj"	"$(INTDIR)\encoder-raw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\framer-jpeg.cpp"

"$(INTDIR)\framer-jpeg.obj"	"$(INTDIR)\framer-jpeg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\huffcode.c

"$(INTDIR)\huffcode.obj"	"$(INTDIR)\huffcode.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\jpeg\jpeg.cpp

"$(INTDIR)\jpeg.obj"	"$(INTDIR)\jpeg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\codec\p64\p64.cpp

"$(INTDIR)\p64.obj"	"$(INTDIR)\p64.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\pvh-huff.c"

"$(INTDIR)\pvh-huff.obj"	"$(INTDIR)\pvh-huff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\codec\transcoder-jpeg.cpp"

"$(INTDIR)\transcoder-jpeg.obj"	"$(INTDIR)\transcoder-jpeg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\cm0.c

"$(INTDIR)\cm0.obj"	"$(INTDIR)\cm0.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\cm1.c

"$(INTDIR)\cm1.obj"	"$(INTDIR)\cm1.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-dither.cpp"

"$(INTDIR)\color-dither.obj"	"$(INTDIR)\color-dither.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-ed.cpp"

"$(INTDIR)\color-ed.obj"	"$(INTDIR)\color-ed.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-gray.cpp"

"$(INTDIR)\color-gray.obj"	"$(INTDIR)\color-gray.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-hi.cpp"

"$(INTDIR)\color-hi.obj"	"$(INTDIR)\color-hi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-hist.cpp"

"$(INTDIR)\color-hist.obj"	"$(INTDIR)\color-hist.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-mono.cpp"

"$(INTDIR)\color-mono.obj"	"$(INTDIR)\color-mono.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-pseudo.cpp"

"$(INTDIR)\color-pseudo.obj"	"$(INTDIR)\color-pseudo.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-quant.cpp"

"$(INTDIR)\color-quant.obj"	"$(INTDIR)\color-quant.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-true.cpp"

"$(INTDIR)\color-true.obj"	"$(INTDIR)\color-true.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\color-yuv.cpp"

"$(INTDIR)\color-yuv.obj"	"$(INTDIR)\color-yuv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\color.cpp

"$(INTDIR)\color.obj"	"$(INTDIR)\color.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\renderer-window.cpp"

"$(INTDIR)\renderer-window.obj"	"$(INTDIR)\renderer-window.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\renderer.cpp

"$(INTDIR)\renderer.obj"	"$(INTDIR)\renderer.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\render\rgb-converter.cpp"

"$(INTDIR)\rgb-converter.obj"	"$(INTDIR)\rgb-converter.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\render\vw.cpp

"$(INTDIR)\vw.obj"	"$(INTDIR)\vw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\rtp\pktbuf-rtp.cpp"

"$(INTDIR)\pktbuf-rtp.obj"	"$(INTDIR)\pktbuf-rtp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\rtp\session.cpp

"$(INTDIR)\session.obj"	"$(INTDIR)\session.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\rtp\source.cpp

"$(INTDIR)\source.obj"	"$(INTDIR)\source.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\rtp\transmitter.cpp

"$(INTDIR)\transmitter.obj"	"$(INTDIR)\transmitter.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\win32\win32.c

"$(INTDIR)\win32.obj"	"$(INTDIR)\win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\win32\win32X.c

"$(INTDIR)\win32X.obj"	"$(INTDIR)\win32X.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\getopt.c

"$(INTDIR)\getopt.obj"	"$(INTDIR)\getopt.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\idlecallback.cpp

"$(INTDIR)\idlecallback.obj"	"$(INTDIR)\idlecallback.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\iohandler.cpp

"$(INTDIR)\iohandler.obj"	"$(INTDIR)\iohandler.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\main.cpp

"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\md5c.c

"$(INTDIR)\md5c.obj"	"$(INTDIR)\md5c.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\media-timer.cpp"

"$(INTDIR)\media-timer.obj"	"$(INTDIR)\media-timer.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\module.cpp

"$(INTDIR)\module.obj"	"$(INTDIR)\module.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\random.c

"$(INTDIR)\random.obj"	"$(INTDIR)\random.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\rate-variable.cpp"

"$(INTDIR)\rate-variable.obj"	"$(INTDIR)\rate-variable.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\strtol.c

"$(INTDIR)\strtol.obj"	"$(INTDIR)\strtol.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\strtoul.c

"$(INTDIR)\strtoul.obj"	"$(INTDIR)\strtoul.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Tcl.cpp

"$(INTDIR)\Tcl.obj"	"$(INTDIR)\Tcl.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\Tcl2.cpp

"$(INTDIR)\Tcl2.obj"	"$(INTDIR)\Tcl2.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\timer.cpp

"$(INTDIR)\timer.obj"	"$(INTDIR)\timer.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tkStripchart.c

"$(INTDIR)\tkStripchart.obj"	"$(INTDIR)\tkStripchart.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tkWinColor.c

"$(INTDIR)\tkWinColor.obj"	"$(INTDIR)\tkWinColor.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\version.c

"$(INTDIR)\version.obj"	"$(INTDIR)\version.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=".\video\assistor-list.cpp"

"$(INTDIR)\assistor-list.obj"	"$(INTDIR)\assistor-list.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\video\device.cpp

"$(INTDIR)\device.obj"	"$(INTDIR)\device.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\video\grabber-still.cpp"

"$(INTDIR)\grabber-still.obj"	"$(INTDIR)\grabber-still.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=".\video\grabber-win32.cpp"

"$(INTDIR)\grabber-win32.obj"	"$(INTDIR)\grabber-win32.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\video\grabber.cpp

"$(INTDIR)\grabber.obj"	"$(INTDIR)\grabber.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!IF  "$(CFG)" == "vic - Win32 Release"

"common - Win32 Release" : 
   cd "\src\common"
   $(MAKE) /$(MAKEFLAGS) /F .\common.mak CFG="common - Win32 Release" 
   cd "..\vic"

"common - Win32 ReleaseCLEAN" : 
   cd "\src\common"
   $(MAKE) /$(MAKEFLAGS) /F .\common.mak CFG="common - Win32 Release" RECURSE=1 CLEAN 
   cd "..\vic"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

"common - Win32 Debug IPv6" : 
   cd "\src\common"
   $(MAKE) /$(MAKEFLAGS) /F .\common.mak CFG="common - Win32 Debug IPv6" 
   cd "..\vic"

"common - Win32 Debug IPv6CLEAN" : 
   cd "\src\common"
   $(MAKE) /$(MAKEFLAGS) /F .\common.mak CFG="common - Win32 Debug IPv6" RECURSE=1 CLEAN 
   cd "..\vic"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

"common - Win32 Debug IPv6 Musica" : 
   cd "\src\common"
   $(MAKE) /$(MAKEFLAGS) /F .\common.mak CFG="common - Win32 Debug IPv6 Musica" 
   cd "..\vic"

"common - Win32 Debug IPv6 MusicaCLEAN" : 
   cd "\src\common"
   $(MAKE) /$(MAKEFLAGS) /F .\common.mak CFG="common - Win32 Debug IPv6 Musica" RECURSE=1 CLEAN 
   cd "..\vic"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"common - Win32 Debug" : 
   cd "\src\common"
   $(MAKE) /$(MAKEFLAGS) /F .\common.mak CFG="common - Win32 Debug" 
   cd "..\vic"

"common - Win32 DebugCLEAN" : 
   cd "\src\common"
   $(MAKE) /$(MAKEFLAGS) /F .\common.mak CFG="common - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\vic"

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"tcllib - Win32 Release" : 
   cd "\src\tcl-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcllib.mak" CFG="tcllib - Win32 Release" 
   cd "..\..\vic"

"tcllib - Win32 ReleaseCLEAN" : 
   cd "\src\tcl-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcllib.mak" CFG="tcllib - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\vic"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

"tcllib - Win32 Debug IPv6" : 
   cd "\src\tcl-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcllib.mak" CFG="tcllib - Win32 Debug IPv6" 
   cd "..\..\vic"

"tcllib - Win32 Debug IPv6CLEAN" : 
   cd "\src\tcl-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcllib.mak" CFG="tcllib - Win32 Debug IPv6" RECURSE=1 CLEAN 
   cd "..\..\vic"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"tcllib - Win32 Debug" : 
   cd "\src\tcl-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcllib.mak" CFG="tcllib - Win32 Debug" 
   cd "..\..\vic"

"tcllib - Win32 DebugCLEAN" : 
   cd "\src\tcl-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcllib.mak" CFG="tcllib - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\vic"

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"tklib - Win32 Release" : 
   cd "\src\tk-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tklib.mak" CFG="tklib - Win32 Release" 
   cd "..\..\vic"

"tklib - Win32 ReleaseCLEAN" : 
   cd "\src\tk-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tklib.mak" CFG="tklib - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\vic"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

"tklib - Win32 Debug IPv6" : 
   cd "\src\tk-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tklib.mak" CFG="tklib - Win32 Debug IPv6" 
   cd "..\..\vic"

"tklib - Win32 Debug IPv6CLEAN" : 
   cd "\src\tk-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tklib.mak" CFG="tklib - Win32 Debug IPv6" RECURSE=1 CLEAN 
   cd "..\..\vic"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"tklib - Win32 Debug" : 
   cd "\src\tk-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tklib.mak" CFG="tklib - Win32 Debug" 
   cd "..\..\vic"

"tklib - Win32 DebugCLEAN" : 
   cd "\src\tk-8.0\win"
   $(MAKE) /$(MAKEFLAGS) /F ".\tklib.mak" CFG="tklib - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\vic"

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"H263v2 Dec_tmndec - Win32 Release" : 
   cd ".\codec\tmndec"
   nmake /f "makefile.mak"
   cd "..\.."

"H263v2 Dec_tmndec - Win32 ReleaseCLEAN" : 
   cd ".\codec\tmndec"
   cd "..\.."

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"H263v2 Dec_tmndec - Win32 Debug" : 
   cd ".\codec\tmndec"
   nmake /f "makefile.mak"
   cd "..\.."

"H263v2 Dec_tmndec - Win32 DebugCLEAN" : 
   cd ".\codec\tmndec"
   cd "..\.."

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"H263v2 Enc_tmnx - Win32 Release" : 
   cd ".\codec\tmn-x"
   nmake /f "makefile.mak"
   cd "..\.."

"H263v2 Enc_tmnx - Win32 ReleaseCLEAN" : 
   cd ".\codec\tmn-x"
   cd "..\.."

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"H263v2 Enc_tmnx - Win32 Debug" : 
   cd ".\codec\tmn-x"
   nmake /f "makefile.mak"
   cd "..\.."

"H263v2 Enc_tmnx - Win32 DebugCLEAN" : 
   cd ".\codec\tmn-x"
   cd "..\.."

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"histtolut - Win32 Release" : 
   cd ".\render\histtolut"
   $(MAKE) /$(MAKEFLAGS) /F .\histtolut.mak CFG="histtolut - Win32 Release" 
   cd "..\.."

"histtolut - Win32 ReleaseCLEAN" : 
   cd ".\render\histtolut"
   $(MAKE) /$(MAKEFLAGS) /F .\histtolut.mak CFG="histtolut - Win32 Release" RECURSE=1 CLEAN 
   cd "..\.."

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"histtolut - Win32 Debug" : 
   cd ".\render\histtolut"
   $(MAKE) /$(MAKEFLAGS) /F .\histtolut.mak CFG="histtolut - Win32 Debug" 
   cd "..\.."

"histtolut - Win32 DebugCLEAN" : 
   cd ".\render\histtolut"
   $(MAKE) /$(MAKEFLAGS) /F .\histtolut.mak CFG="histtolut - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\.."

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"mkbv - Win32 Release" : 
   cd ".\render\mkbv"
   $(MAKE) /$(MAKEFLAGS) /F .\mkbv.mak CFG="mkbv - Win32 Release" 
   cd "..\.."

"mkbv - Win32 ReleaseCLEAN" : 
   cd ".\render\mkbv"
   $(MAKE) /$(MAKEFLAGS) /F .\mkbv.mak CFG="mkbv - Win32 Release" RECURSE=1 CLEAN 
   cd "..\.."

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"mkbv - Win32 Debug" : 
   cd ".\render\mkbv"
   $(MAKE) /$(MAKEFLAGS) /F .\mkbv.mak CFG="mkbv - Win32 Debug" 
   cd "..\.."

"mkbv - Win32 DebugCLEAN" : 
   cd ".\render\mkbv"
   $(MAKE) /$(MAKEFLAGS) /F .\mkbv.mak CFG="mkbv - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\.."

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"mkcube - Win32 Release" : 
   cd ".\render\mkcube"
   $(MAKE) /$(MAKEFLAGS) /F .\mkcube.mak CFG="mkcube - Win32 Release" 
   cd "..\.."

"mkcube - Win32 ReleaseCLEAN" : 
   cd ".\render\mkcube"
   $(MAKE) /$(MAKEFLAGS) /F .\mkcube.mak CFG="mkcube - Win32 Release" RECURSE=1 CLEAN 
   cd "..\.."

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"mkcube - Win32 Debug" : 
   cd ".\render\mkcube"
   $(MAKE) /$(MAKEFLAGS) /F .\mkcube.mak CFG="mkcube - Win32 Debug" 
   cd "..\.."

"mkcube - Win32 DebugCLEAN" : 
   cd ".\render\mkcube"
   $(MAKE) /$(MAKEFLAGS) /F .\mkcube.mak CFG="mkcube - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\.."

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"mkhuff - Win32 Release" : 
   cd ".\render\mkhuff"
   $(MAKE) /$(MAKEFLAGS) /F .\mkhuff.mak CFG="mkhuff - Win32 Release" 
   cd "..\.."

"mkhuff - Win32 ReleaseCLEAN" : 
   cd ".\render\mkhuff"
   $(MAKE) /$(MAKEFLAGS) /F .\mkhuff.mak CFG="mkhuff - Win32 Release" RECURSE=1 CLEAN 
   cd "..\.."

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"mkhuff - Win32 Debug" : 
   cd ".\render\mkhuff"
   $(MAKE) /$(MAKEFLAGS) /F .\mkhuff.mak CFG="mkhuff - Win32 Debug" 
   cd "..\.."

"mkhuff - Win32 DebugCLEAN" : 
   cd ".\render\mkhuff"
   $(MAKE) /$(MAKEFLAGS) /F .\mkhuff.mak CFG="mkhuff - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\.."

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"ppmtolut - Win32 Release" : 
   cd ".\render\ppmtolut"
   $(MAKE) /$(MAKEFLAGS) /F .\ppmtolut.mak CFG="ppmtolut - Win32 Release" 
   cd "..\.."

"ppmtolut - Win32 ReleaseCLEAN" : 
   cd ".\render\ppmtolut"
   $(MAKE) /$(MAKEFLAGS) /F .\ppmtolut.mak CFG="ppmtolut - Win32 Release" RECURSE=1 CLEAN 
   cd "..\.."

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"ppmtolut - Win32 Debug" : 
   cd ".\render\ppmtolut"
   $(MAKE) /$(MAKEFLAGS) /F .\ppmtolut.mak CFG="ppmtolut - Win32 Debug" 
   cd "..\.."

"ppmtolut - Win32 DebugCLEAN" : 
   cd ".\render\ppmtolut"
   $(MAKE) /$(MAKEFLAGS) /F .\ppmtolut.mak CFG="ppmtolut - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\.."

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"tcl2cpp - Win32 Release" : 
   cd ".\tcl\tcl2cpp"
   $(MAKE) /$(MAKEFLAGS) /F .\tcl2cpp.mak CFG="tcl2cpp - Win32 Release" 
   cd "..\.."

"tcl2cpp - Win32 ReleaseCLEAN" : 
   cd ".\tcl\tcl2cpp"
   $(MAKE) /$(MAKEFLAGS) /F .\tcl2cpp.mak CFG="tcl2cpp - Win32 Release" RECURSE=1 CLEAN 
   cd "..\.."

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"tcl2cpp - Win32 Debug" : 
   cd ".\tcl\tcl2cpp"
   $(MAKE) /$(MAKEFLAGS) /F .\tcl2cpp.mak CFG="tcl2cpp - Win32 Debug" 
   cd "..\.."

"tcl2cpp - Win32 DebugCLEAN" : 
   cd ".\tcl\tcl2cpp"
   $(MAKE) /$(MAKEFLAGS) /F .\tcl2cpp.mak CFG="tcl2cpp - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\.."

!ENDIF 

!IF  "$(CFG)" == "vic - Win32 Release"

"tcl2c - Win32 Release" : 
   cd "\src\tcl-8.0\win\tcl2c"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcl2c.mak" CFG="tcl2c - Win32 Release" 
   cd "..\..\..\vic"

"tcl2c - Win32 ReleaseCLEAN" : 
   cd "\src\tcl-8.0\win\tcl2c"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcl2c.mak" CFG="tcl2c - Win32 Release" RECURSE=1 CLEAN 
   cd "..\..\..\vic"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

"tcl2c - Win32 Debug" : 
   cd "\src\tcl-8.0\win\tcl2c"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcl2c.mak" CFG="tcl2c - Win32 Debug" 
   cd "..\..\..\vic"

"tcl2c - Win32 DebugCLEAN" : 
   cd "\src\tcl-8.0\win\tcl2c"
   $(MAKE) /$(MAKEFLAGS) /F ".\tcl2c.mak" CFG="tcl2c - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\..\..\vic"

!ENDIF 

SOURCE=.\cm170.ppm

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=.\cm170.ppm
USERDEP__CM170="render\mkbv\mkbv.exe"	

".\codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__CM170)
	<<tempfile.bat 
	@echo off 
	render\mkbv\mkbv > codec\bv.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=.\cm170.ppm
USERDEP__CM170="render\mkbv\mkbv.exe"	

".\codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__CM170)
	<<tempfile.bat 
	@echo off 
	render\mkbv\mkbv > codec\bv.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=.\cm170.ppm
USERDEP__CM170="render\mkbv\mkbv.exe"	

".\codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__CM170)
	<<tempfile.bat 
	@echo off 
	render\mkbv\mkbv > codec\bv.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=.\cm170.ppm
USERDEP__CM170="render\mkbv\mkbv.exe"	

".\codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__CM170)
	<<tempfile.bat 
	@echo off 
	render\mkbv\mkbv > codec\bv.c
<< 
	

!ENDIF 

SOURCE=".\rgb-cube.ppm"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\rgb-cube.ppm"
USERDEP__RGB_C="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	

".\rgb-cube.ppm"	".\render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__RGB_C)
	<<tempfile.bat 
	@echo off 
	render\mkcube\mkcube rgb > $(InputPath) 
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c 
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\rgb-cube.ppm"
USERDEP__RGB_C="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	

".\rgb-cube.ppm"	".\render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__RGB_C)
	<<tempfile.bat 
	@echo off 
	render\mkcube\mkcube rgb > $(InputPath) 
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c 
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\rgb-cube.ppm"
USERDEP__RGB_C="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	

".\rgb-cube.ppm"	".\render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__RGB_C)
	<<tempfile.bat 
	@echo off 
	render\mkcube\mkcube rgb > $(InputPath) 
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c 
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\rgb-cube.ppm"
USERDEP__RGB_C="render\mkcube\mkcube.exe"	"render\ppmtolut\ppmtolut.exe"	

".\rgb-cube.ppm"	".\render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__RGB_C)
	<<tempfile.bat 
	@echo off 
	render\mkcube\mkcube rgb > $(InputPath) 
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c 
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c
<< 
	

!ENDIF 

SOURCE=.\Version

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=.\Version
USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	

".\version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__VERSI)
	<<tempfile.bat 
	@echo off 
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=.\Version
USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	

".\version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__VERSI)
	<<tempfile.bat 
	@echo off 
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=.\Version
USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	

".\version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__VERSI)
	<<tempfile.bat 
	@echo off 
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=.\Version
USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	

".\version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__VERSI)
	<<tempfile.bat 
	@echo off 
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
<< 
	

!ENDIF 

SOURCE=".\yuv-map.ppm"

!IF  "$(CFG)" == "vic - Win32 Release"

InputPath=".\yuv-map.ppm"
USERDEP__YUV_M="render\mkcube\mkcube.exe"	"render\ppmtolut\ppmtolut.exe"	

".\yuv-map.ppm"	".\render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__YUV_M)
	<<tempfile.bat 
	@echo off 
	set ED_YBITS=4 
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) 
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c 
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6"

InputPath=".\yuv-map.ppm"
USERDEP__YUV_M="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	

".\yuv-map.ppm"	".\render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__YUV_M)
	<<tempfile.bat 
	@echo off 
	set ED_YBITS=4 
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) 
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c 
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

InputPath=".\yuv-map.ppm"
USERDEP__YUV_M="render\mkcube\mkcube.exe"	"render\ppmtolut\ppmtolut.exe"	

".\yuv-map.ppm"	".\render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__YUV_M)
	<<tempfile.bat 
	@echo off 
	set ED_YBITS=4 
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) 
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c 
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c
<< 
	

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

InputPath=".\yuv-map.ppm"
USERDEP__YUV_M="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	

".\yuv-map.ppm"	".\render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)" $(USERDEP__YUV_M)
	<<tempfile.bat 
	@echo off 
	set ED_YBITS=4 
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) 
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c 
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c
<< 
	

!ENDIF 


!ENDIF 

