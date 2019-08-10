echo cc easywsclient.cpp
g++ -w -O2 -c easywsclient.cpp -o easywsclient.o -std=c++11 -Wall -static

echo cc md5.c
g++ -w -O2 -c md5.c -o md5.o -std=c++11 -Wall -static

echo cc huyadanmu.cpp
g++ -w -O2 -c huyadanmu.cpp -o huyadanmu.o -std=c++11 -Wall -static -I/usr/local/include 

echo link hydm.exe
g++  md5.o huyadanmu.o easywsclient.o -o hydm.exe `pkg-config --cflags --libs --static libcjson ` -lws2_32 -static

echo strip hydm.exe
strip hydm.exe