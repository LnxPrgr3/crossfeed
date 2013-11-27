CFLAGS=-O4
LDFLAGS=-framework CoreFoundation -framework AudioUnit -framework AudioToolbox

crossfeed-player: crossfeed-player.o message_queue.o
crossfeed-player.o: crossfeed-player.c message_queue.h
message_queue.o: message_queue.c message_queue.h
