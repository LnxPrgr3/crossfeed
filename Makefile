CFLAGS=-O4
LDFLAGS=-framework CoreFoundation -framework AudioUnit -framework AudioToolbox

crossfeed-player: crossfeed-player.o
crossfeed-player.o: crossfeed-player.c
