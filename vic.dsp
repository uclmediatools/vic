# Microsoft Developer Studio Project File - Name="vic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=vic - Win32 Debug IPv6 MSR
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vic.mak" CFG="vic - Win32 Debug IPv6 MSR"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vic - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "vic - Win32 Debug IPv6 Musica" (based on "Win32 (x86) Application")
!MESSAGE "vic - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "vic - Win32 Debug IPv6 Win2000" (based on "Win32 (x86) Application")
!MESSAGE "vic - Win32 Debug IPv6 MSR" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
vic=rc.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "codec" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "codec\mpeg" /I "..\common\src" /I "..\tk-8.0\win" /I "..\tcl-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\generic" /D "VIC" /D "NDEBUG" /D "_WINDOWS" /D "SASR" /D "WIN32" /D "HAVE_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR /YX /FD /c
# SUBTRACT CPP /X
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 tklib.lib tcllib.lib uclmm.lib wsock32.lib Ws2_32.lib winmm.lib kernel32.lib user32.lib gdi32.lib shell32.lib vfw32.lib advapi32.lib comdlg32.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\common\src\Release" /libpath:"..\tcl-8.0\win\Release" /libpath:"..\tk-8.0\win\Release" /libpath:"codec\tmndec" /libpath:"codec\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vic___Win32_Debug_IPv6_Musica"
# PROP BASE Intermediate_Dir "vic___Win32_Debug_IPv6_Musica"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_IPv6_Musica"
# PROP Intermediate_Dir "Debug_IPv6_Musica"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
vic=rc.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I "\src\common" /I "\src\tcl-8.0\generic" /I "\src\tk-8.0\generic" /I "\src\tk-8.0\xlib" /I "\src\tk-8.0\xlib\X11" /I "\DDK\inc" /I "\src\msripv6-1.2\inc" /D "_WINDOWS" /D "DEBUG" /D "SASR" /D "WIN32" /D "_DEBUG" /D "HAVE_IPv6" /D "NEED_INET_PTON" /D "NEED_INET_NTOP" /FR /YX /FD /I ../common /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I ".\\" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "codec" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "codec\mpeg" /I "..\common\src" /I "..\tk-8.0\win" /I "..\tcl-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\generic" /I "..\MUSICA\WINSOCK6" /D "_WINDOWS" /D "DEBUG" /D "SASR" /D "_DEBUG" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "MUSICA_IPV6" /D "_POSIX" /D "_WINNT" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR /YX /FD /I ../common /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tklib.lib tcllib.lib wship6.lib uclmm.lib winmm.lib wsock32.lib Ws2_32.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"\src\tcl-8.0\win\Debug_IPv6" /libpath:"\src\tk-8.0\win\Debug_IPv6" /libpath:"\src\msripv6-1.2\wship6\obj\i386\free" /libpath:"\src\common\Debug"
# SUBTRACT BASE LINK32 /pdb:none /incremental:no /map /force
# ADD LINK32 tklib.lib tcllib.lib uclmm.lib lib44bsd.lib Resolv.lib wsock32.lib Ws2_32.lib winmm.lib kernel32.lib user32.lib gdi32.lib shell32.lib vfw32.lib advapi32.lib comdlg32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"..\common\Debug" /libpath:"codec\tmndec" /libpath:"codec\tmn-x" /libpath:"..\MUSICA\WINSOCK6"
# SUBTRACT LINK32 /pdb:none /incremental:no /map /force

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
vic=rc.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "codec" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "codec\mpeg" /I "..\common\src" /I "..\tk-8.0\win" /I "..\tcl-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\generic" /I "mst_wm\incls" /D "DEBUG" /D "_DEBUG" /D "DEBUG_MEM" /D "WIN32" /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /D ED_YBITS=4 /D "VIC" /D "USE_TIME" /D "WINDOWS" /Fr /YX /FD /I ../common /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 tklib.lib tcllib.lib wsock32.lib Ws2_32.lib winmm.lib kernel32.lib user32.lib gdi32.lib shell32.lib vfw32.lib advapi32.lib comdlg32.lib uclmm.lib /nologo /subsystem:windows /debug /machine:I386 /libpath:"..\common\src\Debug" /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"codec\tmndec" /libpath:"codec\tmn-x"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vic___Win32_Debug_IPv6_Win2000"
# PROP BASE Intermediate_Dir "vic___Win32_Debug_IPv6_Win2000"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_IPv6_Win2000"
# PROP Intermediate_Dir "Debug_IPv6_Win2000"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
vic=rc.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common\src" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "..\IPv6Kit\inc" /I "g:\DDK\inc" /D "DEBUG" /D "_DEBUG" /D "DEBUG_MEM" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR /YX /FD /I ../common /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I ".\\" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "codec" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "codec\mpeg" /I "..\common\src" /I "..\tk-8.0\win" /I "..\tcl-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\generic" /I "..\IPv6Kit\inc" /I "mst_wm\incls" /D "DEBUG" /D "_DEBUG" /D "DEBUG_MEM" /D "WIN32" /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /D ED_YBITS=4 /D "VIC" /D "USE_TIME" /D "WINDOWS" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "NEED_INET_PTON" /D "WIN2K_IPV6" /FR /YX /FD /I ../common /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tklib.lib tcllib.lib uclmm.lib winmm.lib wsock32.lib Ws2_32.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libh263.lib libh263coder.lib vfw32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"..\common\src\Debug_IPv6" /libpath:"codec\tmndec" /libpath:"codec\tmn-x" /libpath:"..\IPv6Kit\lib"
# SUBTRACT BASE LINK32 /pdb:none /incremental:no /map /force
# ADD LINK32 tklib.lib tcllib.lib uclmm.lib winmm.lib wsock32.lib Ws2_32.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib vfw32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"..\common\src\Debug_IPv6" /libpath:"codec\tmndec" /libpath:"codec\tmn-x"
# SUBTRACT LINK32 /pdb:none /incremental:no /map /force

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vic___Win32_Debug_IPv6_MSR"
# PROP BASE Intermediate_Dir "vic___Win32_Debug_IPv6_MSR"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_IPv6_MSR"
# PROP Intermediate_Dir "Debug_IPv6_MSR"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
vic=rc.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I ".\\" /I "codec" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "..\common\src" /I "..\tk-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\win" /I "..\tcl-8.0\generic" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "..\IPv6Kit\inc" /I "g:\DDK\inc" /D "DEBUG" /D "_DEBUG" /D "DEBUG_MEM" /D "HAVE_IPV6" /D "NEED_INET_NTOP" /D "WIN32" /D "NEED_INET_PTON" /D ED_YBITS=4 /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /FR /YX /FD /I ../common /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I ".\\" /I "render" /I "rtp" /I "net" /I "win32" /I "video" /I "codec" /I "codec\tmndec" /I "codec\tmn-x" /I "codec\jpeg" /I "codec\p64" /I "codec\h263" /I "codec\mpeg" /I "..\common\src" /I "..\tk-8.0\win" /I "..\tcl-8.0\win" /I "..\tk-8.0\generic" /I "..\tk-8.0\xlib" /I "..\tcl-8.0\generic" /I "\DDK\inc" /I "..\msripv6\ms-1.4\msripv6-1.4-src\inc" /I "mst_wm\incls" /D "DEBUG" /D "_DEBUG" /D "DEBUG_MEM" /D "WIN32" /D SIGRET=void /D SIGARGS=int /D NLAYER=8 /D ED_YBITS=4 /D "VIC" /D "USE_TIME" /D "WINDOWS" /D "HAVE_IPV6" /D "HAVE_INET_PTON" /D "HAVE_INET_NTOP" /FR /YX /FD /I ../common /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 tklib.lib tcllib.lib uclmm.lib winmm.lib wsock32.lib Ws2_32.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libh263.lib libh263coder.lib vfw32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"..\common\src\Debug_IPv6" /libpath:"codec\tmndec" /libpath:"codec\tmn-x" /libpath:"..\IPv6Kit\lib"
# SUBTRACT BASE LINK32 /pdb:none /incremental:no /map /force
# ADD LINK32 tklib.lib tcllib.lib uclmm.lib winmm.lib wsock32.lib Ws2_32.lib msacm32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib vfw32.lib wship6.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /pdbtype:sept /libpath:"..\tcl-8.0\win\Debug" /libpath:"..\tk-8.0\win\Debug" /libpath:"..\common\src\Debug_IPv6" /libpath:"codec\tmndec" /libpath:"codec\tmn-x" /libpath:"..\IPv6Kit-1.4-NT4\lib"
# SUBTRACT LINK32 /pdb:none /map /force

!ENDIF 

# Begin Target

# Name "vic - Win32 Release"
# Name "vic - Win32 Debug IPv6 Musica"
# Name "vic - Win32 Debug"
# Name "vic - Win32 Debug IPv6 Win2000"
# Name "vic - Win32 Debug IPv6 MSR"
# Begin Group "Tcl/Tk Scripts"

# PROP Default_Filter ".tcl"
# Begin Group "Tcl/Tk C++ generated files"

# PROP Default_Filter ".cpp"
# Begin Source File

SOURCE=".\tcl\generated\cf-confbus.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\cf-main.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\cf-network.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\cf-tm.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\cf-util.cpp"
# End Source File
# Begin Source File

SOURCE=.\tcl\generated\entry.cpp
# End Source File
# Begin Source File

SOURCE=.\tcl\generated\tkerror.cpp
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-ctrlmenu.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-extout.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-grabber.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-help.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-main.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-relate.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-resource.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-srclist.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-stats.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-switcher.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-util.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-win32.cpp"
# End Source File
# Begin Source File

SOURCE=".\tcl\generated\ui-windows.cpp"
# End Source File
# End Group
# Begin Source File

SOURCE=".\tcl\cf-confbus.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\cf-confbus.tcl"
InputName=cf-confbus

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\cf-main.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\cf-main.tcl"
InputName=cf-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\cf-network.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\cf-network.tcl"
InputName=cf-network

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\cf-tm.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\cf-tm.tcl"
InputName=cf-tm

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\cf-util.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\cf-util.tcl"
InputName=cf-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tcl\entry.tcl

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=.\tcl\entry.tcl
InputName=entry

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\tcl\entry.tcl
InputName=entry

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=.\tcl\entry.tcl
InputName=entry

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=.\tcl\entry.tcl
InputName=entry

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=.\tcl\entry.tcl
InputName=entry

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tcl\tkerror.tcl

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=.\tcl\tkerror.tcl
InputName=tkerror

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=.\tcl\tkerror.tcl
InputName=tkerror

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=.\tcl\tkerror.tcl
InputName=tkerror

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=.\tcl\tkerror.tcl
InputName=tkerror

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=.\tcl\tkerror.tcl
InputName=tkerror

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-ctrlmenu.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-ctrlmenu.tcl"
InputName=ui-ctrlmenu

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-extout.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-extout.tcl"
InputName=ui-extout

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-grabber.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-grabber.tcl"
InputName=ui-grabber

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-help.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-help.tcl"
InputName=ui-help

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-main.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-main.tcl"
InputName=ui-main

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-relate.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-relate.tcl"
InputName=ui-relate

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-resource.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-resource.tcl"
InputName=ui-resource

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-srclist.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-srclist.tcl"
InputName=ui-srclist

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-stats.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-stats.tcl"
InputName=ui-stats

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-switcher.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-switcher.tcl"
InputName=ui-switcher

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-util.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-util.tcl"
InputName=ui-util

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-win32.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-win32.tcl"
InputName=ui-win32

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\tcl\ui-windows.tcl"

!IF  "$(CFG)" == "vic - Win32 Release"

# Begin Custom Build
InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Ignore_Default_Tool 1
# Begin Custom Build
InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# Begin Custom Build
InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# Begin Custom Build
InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# Begin Custom Build
InputPath=".\tcl\ui-windows.tcl"
InputName=ui-windows

"tcl\generated\$(InputName).cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	tcl\tcl2cpp\tcl2cpp 2 $(InputPath)  > tcl\generated\$(InputName).cpp

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "net"

# PROP Default_Filter ""
# Begin Group "Net Header files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\net\crypt.h
# End Source File
# Begin Source File

SOURCE=".\net\group-ipc.h"
# End Source File
# Begin Source File

SOURCE=.\net\inet.h
# End Source File
# Begin Source File

SOURCE=.\net\inet6.h
# End Source File
# Begin Source File

SOURCE=.\net\mbus_engine.h
# End Source File
# Begin Source File

SOURCE=.\net\mbus_handler.h
# End Source File
# Begin Source File

SOURCE=".\net\net-addr.h"
# End Source File
# Begin Source File

SOURCE=.\net\net.h
# End Source File
# Begin Source File

SOURCE=.\net\pktbuf.h
# End Source File
# Begin Source File

SOURCE=.\net\pkttbl.h
# End Source File
# Begin Source File

SOURCE=.\net\rt.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\net\communicator.cpp
# End Source File
# Begin Source File

SOURCE=.\net\confbus.cpp
# End Source File
# Begin Source File

SOURCE=".\net\crypt-des.cpp"
# End Source File
# Begin Source File

SOURCE=".\net\crypt-dull.cpp"
# End Source File
# Begin Source File

SOURCE=.\net\crypt.cpp
# End Source File
# Begin Source File

SOURCE=".\net\group-ipc.cpp"
# End Source File
# Begin Source File

SOURCE=.\net\inet.c
# End Source File
# Begin Source File

SOURCE=.\net\inet6.c

!IF  "$(CFG)" == "vic - Win32 Release"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# ADD CPP /D "NEED_ADDRINFO_H"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\net\mbus_engine.cpp
# End Source File
# Begin Source File

SOURCE=.\net\mbus_handler.cpp
# End Source File
# Begin Source File

SOURCE=".\net\net-addr.cpp"
# End Source File
# Begin Source File

SOURCE=".\net\net-ip.cpp"
# End Source File
# Begin Source File

SOURCE=".\net\net-ipv6.cpp"
# End Source File
# Begin Source File

SOURCE=.\net\net.cpp
# End Source File
# Begin Source File

SOURCE=.\net\pktbuf.cpp
# End Source File
# Begin Source File

SOURCE=.\net\pkttbl.cpp
# End Source File
# End Group
# Begin Group "codec"

# PROP Default_Filter ""
# Begin Group "H263 Erlangen codec"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "H263 Erlangen Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\codec\h263\bitIn.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\bitOut.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\code.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\common.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\config.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\DataTypes.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\defs.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\error.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\getvlc.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\h263.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_cbpb.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_cbpyI.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_cbpyP.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_esclevel.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_escrun.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_intradc.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_mcbpcI.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_mcbpcP.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_modb.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_mvd.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\H263_tcoef.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\h263decoder.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\h263encoder.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\indices.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\Motion.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\sactbls.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\structs.h
# End Source File
# Begin Source File

SOURCE=.\codec\h263\Util.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\codec\h263\bitIn.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\bitOut.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\block.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\code.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\fdct.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\getblk.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\getgob.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\gethdr.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\getvlc.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\h263dec.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\h263enc.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\h263mux.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\h263rtp.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\idctdec.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\idctenc.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\input.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\motion.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\recon.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\reconh263.c
# End Source File
# Begin Source File

SOURCE=.\codec\h263\sac.c
# End Source File
# End Group
# Begin Group "General codec Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\codec\dct.h
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-h263.h"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-jpeg.h"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-mpeg.h"
# End Source File
# Begin Source File

SOURCE=.\codec\decoder.h
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-h263.h"
# End Source File
# Begin Source File

SOURCE=".\codec\framer-h261.h"
# End Source File
# Begin Source File

SOURCE=.\codec\tmn\h263coder.h
# End Source File
# Begin Source File

SOURCE=.\codec\mpeg2par.h
# End Source File
# Begin Source File

SOURCE=".\codec\parser-mpeg.h"
# End Source File
# Begin Source File

SOURCE=".\codec\pvh-huff.h"
# End Source File
# Begin Source File

SOURCE=.\codec\pvh.h
# End Source File
# End Group
# Begin Group "tmn(v2) H.263+ codec"

# PROP Default_Filter ""
# Begin Group "Decoder (tmndec)"

# PROP Default_Filter ""
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\codec\tmndec\config.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\getvlc.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\global.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\indices.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\libh263.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\sactbls.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\tmndec.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\win.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\codec\tmndec\display.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\dither.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\getbits.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\getblk.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\gethdr.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\getpic.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\getvlc.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\idct.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\idctref.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\recon.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\sac.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\store.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\tmndec.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\win.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\yuv2rgb.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\codec\tmndec\yuvrgb24.c

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmndec"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmndec"

!ENDIF 

# End Source File
# End Group
# Begin Group "Encoder (tmn-x)"

# PROP Default_Filter ""
# Begin Group "Header Files No. 1"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\config.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\h263coder.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\indices.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\macros.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\main.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\putvlc.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sactbls.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sim.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vlc.h

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=".\codec\tmn-x\coder.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\countbit.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\dct.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\filter.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\intra_pred.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\io.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\main.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\malloc_debug.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\mot_est.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\pred.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\putbits.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\putvlc.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\quant.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\ratectrl.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\sac.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\codec\tmn-x\snr.c"

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Intermediate_Dir "Release\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

# PROP Intermediate_Dir "Debug_IPv6_Musica\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Intermediate_Dir "Debug\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

# PROP Intermediate_Dir "Debug_IPv6_Win2000\tmn-x"

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

# PROP Intermediate_Dir "Debug_IPv6_MSR\tmn-x"

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Source File

SOURCE=.\codec\bv.c
# End Source File
# Begin Source File

SOURCE=.\codec\cellb_tables.c
# End Source File
# Begin Source File

SOURCE=.\codec\compositor.cpp
# End Source File
# Begin Source File

SOURCE=.\codec\dct.cpp
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-bvc.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-cellb.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-h261.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-h261v1.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-h263.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-h263v2.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-jpeg.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-nv.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-pvh.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\decoder-raw.cpp"
# End Source File
# Begin Source File

SOURCE=.\codec\decoder.cpp
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-bvc.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-cellb.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-h261.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-h263.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-h263v2.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-jpeg.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-nv.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-pvh.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\encoder-raw.cpp"
# End Source File
# Begin Source File

SOURCE=".\codec\framer-jpeg.cpp"
# End Source File
# Begin Source File

SOURCE=.\codec\huffcode.c
# End Source File
# Begin Source File

SOURCE=.\codec\jpeg\jpeg.cpp
# End Source File
# Begin Source File

SOURCE=.\codec\p64\p64.cpp
# End Source File
# Begin Source File

SOURCE=".\codec\pvh-huff.c"
# End Source File
# Begin Source File

SOURCE=".\codec\transcoder-jpeg.cpp"
# End Source File
# End Group
# Begin Group "render"

# PROP Default_Filter ""
# Begin Group "Render Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=".\render\color-hist.h"
# End Source File
# Begin Source File

SOURCE=".\render\color-pseudo.h"
# End Source File
# Begin Source File

SOURCE=.\render\color.h
# End Source File
# Begin Source File

SOURCE=".\render\renderer-window.h"
# End Source File
# Begin Source File

SOURCE=.\render\renderer.h
# End Source File
# Begin Source File

SOURCE=".\render\rgb-converter.h"
# End Source File
# Begin Source File

SOURCE=.\render\vw.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\render\cm0.c
# End Source File
# Begin Source File

SOURCE=.\render\cm1.c
# End Source File
# Begin Source File

SOURCE=".\render\color-dither.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-ed.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-gray.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-hi.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-hist.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-mono.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-pseudo.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-quant.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-true.cpp"
# End Source File
# Begin Source File

SOURCE=".\render\color-yuv.cpp"
# End Source File
# Begin Source File

SOURCE=.\render\color.cpp
# End Source File
# Begin Source File

SOURCE=".\render\renderer-window.cpp"
# End Source File
# Begin Source File

SOURCE=.\render\renderer.cpp
# End Source File
# Begin Source File

SOURCE=".\render\rgb-converter.cpp"
# End Source File
# Begin Source File

SOURCE=.\render\vw.cpp
# End Source File
# End Group
# Begin Group "rtp Source Session"

# PROP Default_Filter ""
# Begin Group "rtp Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=".\rtp\ntp-time.h"
# End Source File
# Begin Source File

SOURCE=".\rtp\pktbuf-rtp.h"
# End Source File
# Begin Source File

SOURCE=.\rtp\rtp.h
# End Source File
# Begin Source File

SOURCE=.\rtp\session.h
# End Source File
# Begin Source File

SOURCE=.\rtp\source.h
# End Source File
# Begin Source File

SOURCE=.\rtp\transmitter.h
# End Source File
# End Group
# Begin Source File

SOURCE=".\rtp\pktbuf-rtp.cpp"
# End Source File
# Begin Source File

SOURCE=.\rtp\session.cpp
# End Source File
# Begin Source File

SOURCE=.\rtp\source.cpp
# End Source File
# Begin Source File

SOURCE=.\rtp\transmitter.cpp
# End Source File
# End Group
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32\win32.c
# End Source File
# Begin Source File

SOURCE=.\win32\win32X.c
# End Source File
# End Group
# Begin Group "Vic Common"

# PROP Default_Filter ".c;.cpp"
# Begin Group "Vic Common Headers"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\config.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\getopt.c
# End Source File
# Begin Source File

SOURCE=.\idlecallback.cpp
# End Source File
# Begin Source File

SOURCE=.\iohandler.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\md5c.c
# End Source File
# Begin Source File

SOURCE=".\media-timer.cpp"
# End Source File
# Begin Source File

SOURCE=.\module.cpp
# End Source File
# Begin Source File

SOURCE=.\random.c
# End Source File
# Begin Source File

SOURCE=".\rate-variable.cpp"
# End Source File
# Begin Source File

SOURCE=.\strtol.c
# End Source File
# Begin Source File

SOURCE=.\strtoul.c
# End Source File
# Begin Source File

SOURCE=.\Tcl.cpp
# End Source File
# Begin Source File

SOURCE=.\Tcl2.cpp
# End Source File
# Begin Source File

SOURCE=.\timer.cpp
# End Source File
# Begin Source File

SOURCE=.\tkStripchart.c
# End Source File
# Begin Source File

SOURCE=.\tkWinColor.c
# End Source File
# Begin Source File

SOURCE=.\version.c
# End Source File
# End Group
# Begin Group "video"

# PROP Default_Filter ""
# Begin Group "video Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=".\video\assistor-list.h"
# End Source File
# Begin Source File

SOURCE=".\video\device-input.h"
# End Source File
# Begin Source File

SOURCE=".\video\device-output.h"
# End Source File
# Begin Source File

SOURCE=.\video\grabber.h
# End Source File
# End Group
# Begin Source File

SOURCE=".\video\assistor-list.cpp"
# End Source File
# Begin Source File

SOURCE=.\video\device.cpp
# End Source File
# Begin Source File

SOURCE=".\video\grabber-still.cpp"
# End Source File
# Begin Source File

SOURCE=".\video\grabber-win32.cpp"
# End Source File
# Begin Source File

SOURCE=.\video\grabber.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\cm170.ppm

!IF  "$(CFG)" == "vic - Win32 Release"

# PROP Ignore_Default_Tool 1
USERDEP__CM170="render\mkbv\mkbv.exe"	
# Begin Custom Build
InputPath=.\cm170.ppm

"codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	render\mkbv\mkbv > codec\bv.c

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

USERDEP__CM170="render\mkbv\mkbv.exe"	
# Begin Custom Build
InputPath=.\cm170.ppm

"codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	render\mkbv\mkbv > codec\bv.c

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__CM170="render\mkbv\mkbv.exe"	
# Begin Custom Build
InputPath=.\cm170.ppm

"codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	render\mkbv\mkbv > codec\bv.c

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

USERDEP__CM170="render\mkbv\mkbv.exe"	
# Begin Custom Build
InputPath=.\cm170.ppm

"codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	render\mkbv\mkbv > codec\bv.c

# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

USERDEP__CM170="render\mkbv\mkbv.exe"	
# Begin Custom Build
InputPath=.\cm170.ppm

"codec\bv.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	render\mkbv\mkbv > codec\bv.c

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\rgb-cube.ppm"

!IF  "$(CFG)" == "vic - Win32 Release"

USERDEP__RGB_C="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	
# Begin Custom Build
InputPath=".\rgb-cube.ppm"

BuildCmds= \
	render\mkcube\mkcube rgb > $(InputPath) \
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c \
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

USERDEP__RGB_C="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	
# Begin Custom Build
InputPath=".\rgb-cube.ppm"

BuildCmds= \
	render\mkcube\mkcube rgb > $(InputPath) \
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c \
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

USERDEP__RGB_C="render\mkcube\mkcube.exe"	"render\ppmtolut\ppmtolut.exe"	
# Begin Custom Build
InputPath=".\rgb-cube.ppm"

BuildCmds= \
	render\mkcube\mkcube rgb > $(InputPath) \
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c \
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

USERDEP__RGB_C="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	
# Begin Custom Build
InputPath=".\rgb-cube.ppm"

BuildCmds= \
	render\mkcube\mkcube rgb > $(InputPath) \
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c \
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

USERDEP__RGB_C="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	
# Begin Custom Build
InputPath=".\rgb-cube.ppm"

BuildCmds= \
	render\mkcube\mkcube rgb > $(InputPath) \
	render\ppmtolut\ppmtolut -n cube rgb-cube.ppm > render\cm0.c \
	render\ppmtolut\ppmtolut -n jv_cube jv-cube-128.ppm >> render\cm0.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm0.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\tk-8.0\win\tk.res"
# End Source File
# Begin Source File

SOURCE=.\Version

!IF  "$(CFG)" == "vic - Win32 Release"

USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	
# Begin Custom Build - Generating "version.c".
InputPath=.\Version

"version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	
# Begin Custom Build - Generating "version.c".
InputPath=.\Version

"version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	
# Begin Custom Build - Generating "version.c".
InputPath=.\Version

"version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	
# Begin Custom Build - Generating "version.c".
InputPath=.\Version

"version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

USERDEP__VERSI="win32\echo.txt"	"win32\set.txt"	"win32\null.txt"	
# Begin Custom Build - Generating "version.c".
InputPath=.\Version

"version.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy win32\set.txt + VERSION win32\vergen.bat 
	copy win32\vergen.bat + win32\null.txt win32\vergen.bat 
	copy win32\vergen.bat + win32\echo.txt win32\vergen.bat 
	win32\vergen.bat 
	move win32\version.c version.c 
	erase win32\version.c 
	erase win32\vergen.bat 
	
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=".\yuv-map.ppm"

!IF  "$(CFG)" == "vic - Win32 Release"

USERDEP__YUV_M="render\mkcube\mkcube.exe"	"render\ppmtolut\ppmtolut.exe"	
# Begin Custom Build
InputPath=".\yuv-map.ppm"

BuildCmds= \
	set ED_YBITS=4 \
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) \
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c \
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Musica"

USERDEP__YUV_M="render\mkcube\mkcube.exe"	"render\ppmtolut\ppmtolut.exe"	
# Begin Custom Build
InputPath=".\yuv-map.ppm"

BuildCmds= \
	set ED_YBITS=4 \
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) \
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c \
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug"

# PROP Ignore_Default_Tool 1
USERDEP__YUV_M="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	
# Begin Custom Build
InputPath=".\yuv-map.ppm"

BuildCmds= \
	set ED_YBITS=4 \
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) \
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c \
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 Win2000"

USERDEP__YUV_M="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	
# Begin Custom Build
InputPath=".\yuv-map.ppm"

BuildCmds= \
	set ED_YBITS=4 \
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) \
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c \
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "vic - Win32 Debug IPv6 MSR"

USERDEP__YUV_M="render\ppmtolut\ppmtolut.exe"	"render\mkcube\mkcube.exe"	
# Begin Custom Build
InputPath=".\yuv-map.ppm"

BuildCmds= \
	set ED_YBITS=4 \
	render\mkcube\mkcube -Y $ED_YBITS -U 45 -V 45 yuv >  $(InputPath) \
	render\ppmtolut\ppmtolut -n quant cm170.ppm > render\cm1.c \
	render\ppmtolut\ppmtolut -Y $ED_YBITS -n ed -e yuv-map.ppm >> render\cm1.c \
	

"$(InputPath)" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"render\cm1.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
