CFLAGS=-O4
LDFLAGS=-framework CoreFoundation -framework AudioUnit -framework AudioToolbox

crossfeed-player: crossfeed-player.o message_queue.o crossfeed.o cautil.o
crossfeed-player.o: crossfeed-player.c message_queue.h crossfeed.h cautil.h
message_queue.o: message_queue.c message_queue.h
crossfeed.o: crossfeed.c crossfeed.h
cautil.o: cautil.c cautil.h
