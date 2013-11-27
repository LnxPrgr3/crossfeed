CFLAGS=-O4
CXXFLAGS=-O4 -std=c++11
LDFLAGS=-framework CoreFoundation -framework AudioUnit -framework AudioToolbox -lc++

crossfeed-player: crossfeed-player.o message_queue.o crossfeed.o cautil.o
crossfeed-player.o: crossfeed-player.cc message_queue.h crossfeed.h cautil.h
message_queue.o: message_queue.c message_queue.h
crossfeed.o: crossfeed.c crossfeed.h
cautil.o: cautil.c cautil.h
