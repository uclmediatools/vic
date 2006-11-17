Win32 build instruction written by Barz Hsu

# Make FFmpeg DLL under Mingw 
* check out ffmpeg
svn co -r 7110 svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg

./configure  --prefix=/tmp/ffmpeg --enable-mingw32 --enable-memalign-hack --e
nable-swscaler --enable-gpl --enable-shared --disable-static

# Make X264 DLL under Mingw
* check out x264
svn co -r 600 svn://svn.videolan.org/x264/trunk x264 
./configure --enable-shared

* patch Makefile for generating DLL
add the 3-6 lines in SONAME targe

1: $(SONAME): .depend $(OBJS) $(OBJASM)
2:        $(CC) -shared -o $@ $(OBJS) $(OBJASM) -Wl,-soname,$(SONAME) $(LDFLAGS)
3: ifeq ($(SYS),MINGW)
4:        $(CC) -shared -Wl,--output-def,libx264.def -o libx264.dll $(OBJS) $(OBJA
SM)
5:        lib /machine:i386 /def:libx264.def
6: endif

# Copy DLL and LIB files to win32/lib
./copy_dll.sh

~                                                                                                                                           
~                                                                                                                                           
~                                                                                                                                           
~                                                                                                                                           
~                          
~
~                                                                               
~                                                                               
~                                                                               
~                                                                               
~                    
