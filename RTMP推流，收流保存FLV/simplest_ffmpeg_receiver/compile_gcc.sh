#! /bin/sh
#��򵥵Ļ���FFmpeg��������������RTMP��----�����б���
#Simplest FFmpeg Receiver (Receive RTMP)----Compile in Shell 
#
#������ Lei Xiaohua
#leixiaohua1020@126.com
#�й���ý��ѧ/���ֵ��Ӽ���
#Communication University of China / Digital TV Technology
#http://blog.csdn.net/leixiaohua1020
#
#compile
gcc simplest_ffmpeg_receiver.cpp -g -o simplest_ffmpeg_receiver.out \
-I /usr/local/include -L /usr/local/lib -lavformat -lavcodec -lavutil
