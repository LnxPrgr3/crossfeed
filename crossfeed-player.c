#include <stdlib.h>
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include "cautil.h"
#include "message_queue.h"
#include "crossfeed.h"

struct control_msg {
	int id;
	enum {MSG_FILEDONE} type;
};

static struct message_queue mq;
static crossfeed_t crossfeed;

static void event_handler(struct PlayerEvent *evt) {
	struct control_msg *msg;
	switch(evt->type) {
	case PLAYER_RENDER:
		crossfeed_filter_inplace_noninterleaved(&crossfeed, evt->left, evt->right, evt->size);
		break;
	case PLAYER_DONE:
		msg = message_queue_message_alloc_blocking(&mq);
		msg->type = MSG_FILEDONE;
		message_queue_write(&mq, msg);
		break;
	}
}

int main(int argc, char *argv[]) {
	int res = EXIT_FAILURE;
	message_queue_init(&mq, sizeof(struct control_msg), 2);
	crossfeed_init(&crossfeed);
	if(argc < 2) {
		fprintf(stderr, "Usage: %s /foo/bar\n", argc == 1 ? argv[0] : "crossfeed-player");
		goto done;
	}
	struct Player player;
	if(CAInitPlayer(&player, &event_handler)) {
		fprintf(stderr, "Error initializing audio output\n");
		goto done;
	}
	if(CAPlayFile(&player, argv[1])) {
		fprintf(stderr, "Error playing file\n");
		goto close_player;
	}
	message_queue_message_free(&mq, message_queue_read(&mq));
	CAStopPlayback(&player);
	res = EXIT_SUCCESS;
close_player:
	CADestroyPlayer(&player);
done:
	message_queue_destroy(&mq);
	return res;
}
