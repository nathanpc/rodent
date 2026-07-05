# Microsoft Developer Studio Project File - Name="Rodent" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Rodent - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Rodent.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Rodent.mak" CFG="Rodent - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Rodent - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Rodent - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Rodent - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../gopher" /I "../shims" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "WIN32_LEAN_AND_MEAN" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib shlwapi.lib comctl32.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Rodent - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../gopher" /I "../shims" /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "WIN32_LEAN_AND_MEAN" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib shlwapi.lib comctl32.lib /nologo /entry:"wWinMainCRTStartup" /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Rodent - Win32 Release"
# Name "Rodent - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\src\Rodent.cpp
# End Source File
# Begin Source File

SOURCE=..\src\stdafx.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\src\Rodent.h
# End Source File
# Begin Source File

SOURCE=..\src\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\src\targetver.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Group "Icons"

# PROP Default_Filter "ico"
# Begin Source File

SOURCE=..\icons\arrow_left_blue.ico
# End Source File
# Begin Source File

SOURCE=..\icons\arrow_right_blue.ico
# End Source File
# Begin Source File

SOURCE=..\icons\arrow_up_blue.ico
# End Source File
# Begin Source File

SOURCE=..\icons\blank.ico
# End Source File
# Begin Source File

SOURCE=..\icons\document_certificate.ico
# End Source File
# Begin Source File

SOURCE=..\icons\document_text.ico
# End Source File
# Begin Source File

SOURCE=..\icons\earth.ico
# End Source File
# Begin Source File

SOURCE=..\icons\error.ico
# End Source File
# Begin Source File

SOURCE=..\icons\folder.ico
# End Source File
# Begin Source File

SOURCE=..\icons\laptop.ico
# End Source File
# Begin Source File

SOURCE=..\icons\loudspeaker.ico
# End Source File
# Begin Source File

SOURCE=..\icons\media_play_green.ico
# End Source File
# Begin Source File

SOURCE=..\icons\movie.ico
# End Source File
# Begin Source File

SOURCE=..\icons\photo_scenery.ico
# End Source File
# Begin Source File

SOURCE=..\icons\refresh.ico
# End Source File
# Begin Source File

SOURCE=..\icons\stop.ico
# End Source File
# Begin Source File

SOURCE=..\icons\unknown.ico
# End Source File
# Begin Source File

SOURCE=..\icons\view.ico
# End Source File
# Begin Source File

SOURCE=..\icons\window_gear.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Rodent.ico
# End Source File
# Begin Source File

SOURCE=.\Rodent.rc
# End Source File
# Begin Source File

SOURCE=..\src\SharedResources.h
# End Source File
# Begin Source File

SOURCE=.\small.ico
# End Source File
# End Group
# Begin Group "Utilities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\Utilities\DialogWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Utilities\DialogWindow.h
# End Source File
# Begin Source File

SOURCE=..\src\Utilities\MsgBoxes.cpp
# End Source File
# Begin Source File

SOURCE=..\src\Utilities\MsgBoxes.h
# End Source File
# Begin Source File

SOURCE=..\src\Utilities\WindowUtilities.c
# End Source File
# Begin Source File

SOURCE=..\src\Utilities\WindowUtilities.h
# End Source File
# End Group
# Begin Group "Windows and Dialogs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\AboutDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\src\AboutDialog.h
# End Source File
# Begin Source File

SOURCE=..\src\DownloadDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\src\DownloadDialog.h
# End Source File
# Begin Source File

SOURCE=..\src\MainWindow.cpp
# End Source File
# Begin Source File

SOURCE=..\src\MainWindow.h
# End Source File
# End Group
# Begin Group "Gopher"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\gopher\gopher.c
# End Source File
# Begin Source File

SOURCE=..\..\gopher\gopher.h
# End Source File
# Begin Source File

SOURCE=..\src\GopherWrapper.cpp
# End Source File
# Begin Source File

SOURCE=..\src\GopherWrapper.h
# End Source File
# End Group
# End Target
# End Project
