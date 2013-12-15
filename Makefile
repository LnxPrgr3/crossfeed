CFLAGS=-O4
CXXFLAGS=-O4 -std=c++11

crossfeed-player: crossfeed-player.o message_queue.o crossfeed.o cautil.o
	$(CXX) -o crossfeed-player crossfeed-player.o message_queue.o crossfeed.o cautil.o \
	       -framework CoreFoundation -framework AudioUnit -framework AudioToolbox
crossfeed-player.o: crossfeed-player.cc message_queue.h crossfeed.h cautil.h
message_queue.o: message_queue.c message_queue.h
crossfeed.o: crossfeed.c crossfeed.h
cautil.o: cautil.c cautil.h
