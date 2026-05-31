
# NMake Makefile for dxwnd (converted from dxwnd.vs2008.vcproj)

!IF "$(CFG)" == ""
CFG=Release
!ENDIF

CPP = cl.exe
LINK = link.exe
RC = rc.exe

INCS = /I../Include /I. /I../Include/mfc
LIBS_COMMON = detours.lib shlwapi.lib legacy_stdio_definitions.lib
LIBPATH = ../libs

!IF "$(CFG)" == "Debug"
OUTDIR = Debug
TARGET = Debug\dxwnd.exe
CFLAGS = /nologo /MTd /W3 /EHa /Zi /Od $(INCS) /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\dxwnd.pdb" /c
LFLAGS = /nologo /subsystem:windows /incremental:yes /debug /machine:I386 /out:$(TARGET) /pdb:$(OUTDIR)\dxwnd.pdb /DYNAMICBASE /LIBPATH:$(LIBPATH)
LIBS = $(LIBS_COMMON) ../src/Debug/dxwnd.lib
RCFLAGS=/l 0x409 /d "_DEBUG" /i.. /i../include/mfc
!ELSE
OUTDIR = Release
TARGET = Release\dxwnd.exe
CFLAGS=/nologo /MT /W3 /EHa /O2 /Zi $(INCS) /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\dxwnd.pdb" /c
LFLAGS=/nologo /subsystem:windows /incremental:no /machine:I386 /DEBUG /out:$(TARGET) /pdb:$(OUTDIR)\dxwnd.pdb /DYNAMICBASE /LIBPATH:$(LIBPATH)
LIBS = $(LIBS_COMMON) ../src/Release/dxwnd.lib
RCFLAGS=/l 0x409 /d "NDEBUG" /i.. /i../include/mfc
!ENDIF

OBJS = \
	$(OUTDIR)\associate.obj \
	$(OUTDIR)\C3DEffectsDlg.obj \
	$(OUTDIR)\CAboutDlg.obj \
	$(OUTDIR)\CDChargerDialog.obj \
	$(OUTDIR)\CDPlayerDialog.obj \
	$(OUTDIR)\CDragEdit.obj \
	$(OUTDIR)\CGlobalSettings.obj \
	$(OUTDIR)\CGlobalStartUp.obj \
	$(OUTDIR)\clearshim.obj \
	$(OUTDIR)\CoolUtils.obj \
	$(OUTDIR)\CppUACSelfElevation.obj \
	$(OUTDIR)\DesktopDialog.obj \
	$(OUTDIR)\dib.obj \
	$(OUTDIR)\dxelevate.obj \
	$(OUTDIR)\dxTabConfCtrl.obj \
	$(OUTDIR)\dxTabCtrl.obj \
	$(OUTDIR)\dxwndhost.obj \
	$(OUTDIR)\dxwndhostDoc.obj \
	$(OUTDIR)\dxwndhostView.obj \
	$(OUTDIR)\findwindow.obj \
	$(OUTDIR)\FolderDlg.obj \
	$(OUTDIR)\GameIcons.obj \
	$(OUTDIR)\GammaCtrlDialog.obj \
	$(OUTDIR)\GammaRampDialog.obj \
	$(OUTDIR)\getfname.obj \
	$(OUTDIR)\InjectDebug.obj \
	$(OUTDIR)\InjectDetours.obj \
	$(OUTDIR)\InjectHook.obj \
	$(OUTDIR)\InjectProxy.obj \
	$(OUTDIR)\InjectSuspended.obj \
	$(OUTDIR)\KillProc.obj \
	$(OUTDIR)\KillProcTree.obj \
	$(OUTDIR)\MainFrm.obj \
	$(OUTDIR)\MessageBox.obj \
	$(OUTDIR)\MousePositionDialog.obj \
	$(OUTDIR)\OutTrace.obj \
	$(OUTDIR)\PaletteDialog.obj \
	$(OUTDIR)\pedump.obj \
	$(OUTDIR)\PictureEx.obj \
	$(OUTDIR)\queryWDDM.obj \
	$(OUTDIR)\safedisk.obj \
	$(OUTDIR)\SaveIcon.obj \
	$(OUTDIR)\ShimsDialog.obj \
	$(OUTDIR)\ShowHelp.obj \
	$(OUTDIR)\SpecialEdit.obj \
	$(OUTDIR)\StatusDialog.obj \
	$(OUTDIR)\StdAfx.obj \
	$(OUTDIR)\syscursors.obj \
	$(OUTDIR)\SystemTray.obj \
	$(OUTDIR)\Tab3DDialog.obj \
	$(OUTDIR)\TabCDA.obj \
	$(OUTDIR)\TabCompat.obj \
	$(OUTDIR)\TabConfDefaults.obj \
	$(OUTDIR)\TabConfFlags.obj \
	$(OUTDIR)\TabConfKeys.obj \
	$(OUTDIR)\TabDebug.obj \
	$(OUTDIR)\TabDirect3D.obj \
	$(OUTDIR)\TabDirect3D2.obj \
	$(OUTDIR)\TabDirectX.obj \
	$(OUTDIR)\TabDirectX2.obj \
	$(OUTDIR)\TabGDI.obj \
	$(OUTDIR)\TabHook.obj \
	$(OUTDIR)\TabHW.obj \
	$(OUTDIR)\TabInput.obj \
	$(OUTDIR)\TabIO.obj \
	$(OUTDIR)\TabLocale.obj \
	$(OUTDIR)\TabLogs.obj \
	$(OUTDIR)\TabMouse.obj \
	$(OUTDIR)\TabMsgs.obj \
	$(OUTDIR)\TabNotes.obj \
	$(OUTDIR)\TabOpenGL.obj \
	$(OUTDIR)\TabProgram.obj \
	$(OUTDIR)\TabRegistry.obj \
	$(OUTDIR)\TabSDL.obj \
	$(OUTDIR)\TabSound.obj \
	$(OUTDIR)\TabSysLibs.obj \
	$(OUTDIR)\TabTiming.obj \
	$(OUTDIR)\TabTweaks.obj \
	$(OUTDIR)\TabWDDM.obj \
	$(OUTDIR)\TabWindow.obj \
	$(OUTDIR)\TabWine.obj \
	$(OUTDIR)\TargetDlg.obj \
	$(OUTDIR)\TimeSliderDlg.obj \
	$(OUTDIR)\ViewFlagsDialog.obj \
	$(OUTDIR)\VJoyDialog.obj \
	$(OUTDIR)\dxwndhost.res

all: $(OUTDIR) $(TARGET)

$(OUTDIR):
	-mkdir $(OUTDIR)

$(TARGET): $(OBJS)
	$(LINK) $(LFLAGS) $(OBJS) $(LIBS)

.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) $<

clean:
	-rmdir /s /q $(OUTDIR)

$(OUTDIR)\dxwndhost.res: dxwndhost.rc
	$(RC) $(RCFLAGS) /fo$(OUTDIR)\dxwndhost.res dxwndhost.rc

$(OUTDIR)\dxwndhost.obj: dxwndhost.cpp
$(OUTDIR)\MainFrm.obj: MainFrm.cpp

