# Microsoft Developer Studio Project File - Name="H263v2 Enc_tmnx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=H263v2 Enc_tmnx - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "H263v2 Enc_tmnx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "H263v2 Enc_tmnx.mak" CFG="H263v2 Enc_tmnx - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "H263v2 Enc_tmnx - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "H263v2 Enc_tmnx - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "H263v2 Enc_tmnx - Win32 Release"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f "H263v2 Enc_tmnx.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "H263v2 Enc_tmnx.exe"
# PROP BASE Bsc_Name "H263v2 Enc_tmnx.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake /f "makefile.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "libh263coder.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "H263v2 Enc_tmnx - Win32 Debug"

# PROP BASE Use_MFC
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f "H263v2 Enc_tmnx.mak""
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "H263v2 Enc_tmnx.exe"
# PROP BASE Bsc_Name "H263v2 Enc_tmnx.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "nmake /f "makefile.mak""
# PROP Rebuild_Opt "/a"
# PROP Target_File "libh263coder.lib"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "H263v2 Enc_tmnx - Win32 Release"
# Name "H263v2 Enc_tmnx - Win32 Debug"

!IF  "$(CFG)" == "H263v2 Enc_tmnx - Win32 Release"

!ELSEIF  "$(CFG)" == "H263v2 Enc_tmnx - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\coder.c
# End Source File
# Begin Source File

SOURCE=.\countbit.c
# End Source File
# Begin Source File

SOURCE=.\dct.c
# End Source File
# Begin Source File

SOURCE=.\filter.c
# End Source File
# Begin Source File

SOURCE=.\intra_pred.c
# End Source File
# Begin Source File

SOURCE=.\io.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\malloc_debug.c
# End Source File
# Begin Source File

SOURCE=.\mot_est.c
# End Source File
# Begin Source File

SOURCE=.\pred.c
# End Source File
# Begin Source File

SOURCE=.\putbits.c
# End Source File
# Begin Source File

SOURCE=.\putvlc.c
# End Source File
# Begin Source File

SOURCE=.\quant.c
# End Source File
# Begin Source File

SOURCE=.\ratectrl.c
# End Source File
# Begin Source File

SOURCE=.\sac.c
# End Source File
# Begin Source File

SOURCE=.\snr.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\h263coder.h
# End Source File
# Begin Source File

SOURCE=.\indices.h
# End Source File
# Begin Source File

SOURCE=.\macros.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\putvlc.h
# End Source File
# Begin Source File

SOURCE=.\sactbls.h
# End Source File
# Begin Source File

SOURCE=.\sim.h
# End Source File
# Begin Source File

SOURCE=.\vlc.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
