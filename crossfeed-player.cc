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
static float scale = 1;

static void event_handler(struct PlayerEvent *evt) {
	struct control_msg *msg;
	switch(evt->type) {
	case PlayerEvent::PLAYER_RENDER:
		crossfeed_filter_inplace_noninterleaved(&crossfeed, evt->left, evt->right, evt->size);
		for(unsigned int i=0;i<evt->size;++i) {
			evt->left[i] *= scale;
			evt->right[i] *= scale;
		}
		break;
	case PlayerEvent::PLAYER_DONE:
		msg = (struct control_msg *)message_queue_message_alloc_blocking(&mq);
		msg->type = control_msg::MSG_FILEDONE;
		message_queue_write(&mq, msg);
		break;
	}
}

int main(int argc, char *argv[]) {
	int res = EXIT_FAILURE;
	message_queue_init(&mq, sizeof(struct control_msg), 2);
	crossfeed_init(&crossfeed);
	if(argc < 2) {
		fprintf(stderr, "Usage: %s [-g dBFS] /foo/bar\n", argc == 1 ? argv[0] : "crossfeed-player");
		goto done;
	}
	struct Player player;
	if(CAInitPlayer(&player, &event_handler)) {
		fprintf(stderr, "Error initializing audio output\n");
		goto done;
	}
	for(int i=1;i<argc;++i) {
		if(strcmp(argv[i], "-g") == 0) {
			if(++i > argc)
				break;
			float scale_db = atof(argv[i]);
			scale = pow(10, scale_db/20);
			fprintf(stderr, "Set gain to %f (%fdBFS)\n", scale, scale_db);
			continue;
		}
		fprintf(stderr, "Playing `%s'...\n", argv[i]);
		if(CAPlayFile(&player, argv[i])) {
			fprintf(stderr, "Error playing `%s'\n", argv[i]);
			continue;
		}
		message_queue_message_free(&mq, message_queue_read(&mq));
		CAStopPlayback(&player);
	}
	res = EXIT_SUCCESS;
close_player:
	CADestroyPlayer(&player);
done:
	message_queue_destroy(&mq);
	return res;
}
