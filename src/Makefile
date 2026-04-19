
# NMake Makefile for dxwnd (converted from dxwnd.vs2008.vcproj)

!IF "$(CFG)" == ""
CFG=Release
!ENDIF

CC = cl.exe
LINK = link.exe
RC = rc.exe
MT = mt.exe

INCS = /I. /IInclude /I../Include
LIBS_COMMON = libMinHook.x86.lib detours.lib version.lib OpenGl32.lib advapi32.lib user32.lib kernel32.lib gdi32.lib shell32.lib legacy_stdio_definitions.lib
LIBPATH = ../libs

!IF "$(CFG)" == "Debug"
OUTDIR = Debug
TARGET = Debug\dxwnd.dll
CFLAGS = /nologo /W3 /EHa /MTd /Zi /Od $(INCS) /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "DXWND_EXPORTS" /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\dxwnd.pdb"
LFLAGS = /nologo /DLL /DEBUG /INCREMENTAL:YES /DEF:dxwnd.def /OUT:$(TARGET) /IMPLIB:$(OUTDIR)\dxwnd.lib /PDB:$(OUTDIR)\dxwnd.pdb /LIBPATH:$(LIBPATH)
LIBS = $(LIBS_COMMON)
!ELSE
OUTDIR = Release
TARGET = Release\dxwnd.dll
CFLAGS = /nologo /W3 /EHa /MT /Zi /O2 $(INCS) /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "DXWND_EXPORTS" /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\dxwnd.pdb"
LFLAGS = /nologo /DLL /DEBUG /INCREMENTAL:NO /DEF:dxwnd.def /OUT:$(TARGET) /IMPLIB:$(OUTDIR)\dxwnd.lib /PDB:$(OUTDIR)\dxwnd.pdb /LIBPATH:$(LIBPATH)
LIBS = $(LIBS_COMMON)
!ENDIF

OBJS = \
    $(OUTDIR)\advapi.obj \
    $(OUTDIR)\asyncdc.obj \
    $(OUTDIR)\avifile.obj \
    $(OUTDIR)\binkw32.obj \
    $(OUTDIR)\bmplib.obj \
    $(OUTDIR)\charset.obj \
    $(OUTDIR)\clearshim.obj \
    $(OUTDIR)\comctl32.obj \
    $(OUTDIR)\comdlg32.obj \
    $(OUTDIR)\comvjoy.obj \
    $(OUTDIR)\comvsound.obj \
    $(OUTDIR)\cpulimit.obj \
    $(OUTDIR)\createproc.obj \
    $(OUTDIR)\d3d9blit.obj \
    $(OUTDIR)\d3dtexture.obj \
    $(OUTDIR)\d3dumddi.obj \
    $(OUTDIR)\d3dxof.obj \
    $(OUTDIR)\ddblit.obj \
    $(OUTDIR)\ddcolor2alpha.obj \
    $(OUTDIR)\ddghost.obj \
    $(OUTDIR)\ddraw.obj \
    $(OUTDIR)\ddraw.createsurface.obj \
    $(OUTDIR)\ddraw.ddrawex.obj \
    $(OUTDIR)\ddshot.obj \
    $(OUTDIR)\ddtexture.obj \
    $(OUTDIR)\dibdump.obj \
    $(OUTDIR)\dibemu.obj \
    $(OUTDIR)\dinput.obj \
    $(OUTDIR)\dlgstretch.obj \
    $(OUTDIR)\dplayx.obj \
    $(OUTDIR)\dshow.basefilter.obj \
    $(OUTDIR)\dshow.basicaudio.obj \
    $(OUTDIR)\dshow.basicvideo.obj \
    $(OUTDIR)\dshow.obj \
    $(OUTDIR)\dshow.createdevenum.obj \
    $(OUTDIR)\dshow.enumfilters.obj \
    $(OUTDIR)\dshow.enumpins.obj \
    $(OUTDIR)\dshow.filtergraph.obj \
    $(OUTDIR)\dshow.filtergraph2.obj \
    $(OUTDIR)\dshow.graphbuilder.obj \
    $(OUTDIR)\dshow.mediacontrol.obj \
    $(OUTDIR)\dshow.mediaeventex.obj \
    $(OUTDIR)\dshow.mediaeventsink.obj \
    $(OUTDIR)\dshow.mediafilter.obj \
    $(OUTDIR)\dshow.openprogress.obj \
    $(OUTDIR)\dshow.videowindow.obj \
    $(OUTDIR)\dshow.VMRImagePresenter.obj \
    $(OUTDIR)\dshow.VMRImagePresenterConfig.obj \
    $(OUTDIR)\dshow.VMRSurfaceAllocator.obj \
    $(OUTDIR)\dshow.VMRSurfaceAllocatorNotify.obj \
    $(OUTDIR)\dshow.VMRWindowlessControl.obj \
    $(OUTDIR)\dsound.obj \
    $(OUTDIR)\dumphbmp.obj \
    $(OUTDIR)\dumptext.obj \
    $(OUTDIR)\dwdisasm.obj \
    $(OUTDIR)\dwflip.obj \
    $(OUTDIR)\dwhide.obj \
    $(OUTDIR)\dwmapi.obj \
    $(OUTDIR)\dxdiaghook.obj \
    $(OUTDIR)\dxemublt.obj \
    $(OUTDIR)\dxhelper.obj \
    $(OUTDIR)\dxhook.obj \
    $(OUTDIR)\dxjoystick.obj \
    $(OUTDIR)\dxmapping.obj \
    $(OUTDIR)\dxpatcher.obj \
    $(OUTDIR)\dxwchildwstack.obj \
    $(OUTDIR)\dxwcore.obj \
    $(OUTDIR)\dxwnd.obj \
    $(OUTDIR)\dxwsstack.obj \
    $(OUTDIR)\dxwwstack.obj \
    $(OUTDIR)\ear.obj \
    $(OUTDIR)\emu3dnow.obj \
    $(OUTDIR)\emulateddc.obj \
    $(OUTDIR)\excepthandler.obj \
    $(OUTDIR)\fixwinproc.obj \
    $(OUTDIR)\flash.obj \
    $(OUTDIR)\flipoverlay.obj \
    $(OUTDIR)\fontstack.obj \
    $(OUTDIR)\forceD3d9On12.obj \
    $(OUTDIR)\fourcc.obj \
    $(OUTDIR)\fthook.obj \
    $(OUTDIR)\gdi32.obj \
    $(OUTDIR)\gdiblt.obj \
    $(OUTDIR)\glide.obj \
    $(OUTDIR)\guids.obj \
    $(OUTDIR)\hd3d.obj \
    $(OUTDIR)\hd3d11.obj \
    $(OUTDIR)\hd3d7.obj \
    $(OUTDIR)\hd3d8.obj \
    $(OUTDIR)\hd3d9.obj \
    $(OUTDIR)\hdsetup.obj \
    $(OUTDIR)\hdxgi.obj \
    $(OUTDIR)\heap.obj \
    $(OUTDIR)\helpmsgs.obj \
    $(OUTDIR)\hexdump.obj \
    $(OUTDIR)\hidedm.obj \
    $(OUTDIR)\hint.obj \
    $(OUTDIR)\hooktable.obj \
    $(OUTDIR)\hotpatch.obj \
    $(OUTDIR)\hTable.obj \
    $(OUTDIR)\iatpatch.obj \
    $(OUTDIR)\imagehlp.obj \
    $(OUTDIR)\imelib.obj \
    $(OUTDIR)\Inject.obj \
    $(OUTDIR)\int3hdlr.obj \
    $(OUTDIR)\kernel32.obj \
    $(OUTDIR)\locale.obj \
    $(OUTDIR)\localeinfo.obj \
    $(OUTDIR)\lz32.obj \
    $(OUTDIR)\mciplayer.obj \
    $(OUTDIR)\mididump.obj \
    $(OUTDIR)\mincaps.obj \
    $(OUTDIR)\mmedia.obj \
    $(OUTDIR)\msghook.obj \
    $(OUTDIR)\msims32.obj \
    $(OUTDIR)\mss32.obj \
    $(OUTDIR)\msvcrt.obj \
    $(OUTDIR)\msvfw.obj \
    $(OUTDIR)\ntdll.obj \
    $(OUTDIR)\oglblt.obj \
    $(OUTDIR)\ogltexture.obj \
    $(OUTDIR)\ole32.obj \
    $(OUTDIR)\opengl.obj \
    $(OUTDIR)\patcher.obj \
    $(OUTDIR)\queryint.obj \
    $(OUTDIR)\s3tc.obj \
    $(OUTDIR)\savegraph.obj \
    $(OUTDIR)\sblit.obj \
    $(OUTDIR)\sdl.obj \
    $(OUTDIR)\sdl2blt.obj \
    $(OUTDIR)\sdlblt.obj \
    $(OUTDIR)\sdump.obj \
    $(OUTDIR)\sdump17.obj \
    $(OUTDIR)\sdump8.obj \
    $(OUTDIR)\sdump9.obj \
    $(OUTDIR)\shareddc.obj \
    $(OUTDIR)\shell32.obj \
    $(OUTDIR)\smack.obj \
    $(OUTDIR)\supmodes.obj \
    $(OUTDIR)\syscursors.obj \
    $(OUTDIR)\toggle_fs.obj \
    $(OUTDIR)\user32.obj \
    $(OUTDIR)\user32.winhelp.obj \
    $(OUTDIR)\uxtheme.obj \
    $(OUTDIR)\vbasic.obj \
    $(OUTDIR)\version.obj \
    $(OUTDIR)\vfilesys.obj \
    $(OUTDIR)\wail32.obj \
    $(OUTDIR)\winexec.obj \
    $(OUTDIR)\wing32.obj \
    $(OUTDIR)\winmm.obj \
    $(OUTDIR)\winmm.mcicommand.obj \
    $(OUTDIR)\winmm.mcistring.obj \
    $(OUTDIR)\winmm.mciwrapper.obj \
    $(OUTDIR)\winproc.obj \
    $(OUTDIR)\winshot.obj \
    $(OUTDIR)\wintrust.obj \
    $(OUTDIR)\wlocale.obj \
    $(OUTDIR)\xinput.obj \
    $(OUTDIR)\dxwnd.res

all: $(OUTDIR) $(TARGET)

$(OUTDIR):
    -mkdir $(OUTDIR)

$(TARGET): $(OBJS)
    $(LINK) $(LFLAGS) $(OBJS) $(LIBS)
    $(MT) -nologo -manifest dxwnd.dll.manifest -outputresource:"$(TARGET);#2"

.cpp{$(OUTDIR)}.obj:
    $(CC) $(CFLAGS) -c $<

$(OUTDIR)\dxwnd.res: dxwnd.rc
    $(RC) /i$(INCS) /fo"$(OUTDIR)\dxwnd.res" dxwnd.rc

clean:
    -rmdir /s /q $(OUTDIR)
