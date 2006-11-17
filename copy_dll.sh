#!/bin/sh
echo "copying avcodec avutil swscale dll into win32/lib"
cp ffmpeg/libavcodec/*.lib win32/lib
cp ffmpeg/libavcodec/*.dll win32/lib
cp ffmpeg/libavutil/*.lib win32/lib
cp ffmpeg/libavutil/*.dll win32/lib
cp ffmpeg/libswscale/*.lib win32/lib
cp ffmpeg/libswscale/*.dll win32/lib
cp x264/*.lib win32/lib
cp x264/*.dll win32/lib
echo "copy ffmpeg and x264 header into win32"
cp -r ffmpeg/include/ffmpeg win32
cp x264/x264.h win32 
