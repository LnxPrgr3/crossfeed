#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <termios.h>
#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include <vector>
#include "cautil.h"
#include "message_queue.h"
#include "crossfeed.h"

struct control_msg {
	int id;
	enum {MSG_FILEDONE, MSG_BACK, MSG_FORWARD, MSG_EXIT} type;
};

static struct message_queue mq;
static crossfeed_t crossfeed;
static float scale_db = 0;
static float scale = 1;

static std::vector<const char *> playlist;

static void set_volume(float volume) {
	scale_db = volume;
	scale = pow(10, scale_db/20);
}

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

static void *audio_threadproc(void *data) {
	struct Player player;
	std::vector<const char *>::const_iterator i = playlist.begin();
	if(CAInitPlayer(&player, &event_handler)) {
		fprintf(stderr, "Error initializing audio output\n");
		goto done;
	}
	while(i != playlist.end()) {
		fprintf(stderr, "Playing `%s'...\r\n", *i);
		if(CAPlayFile(&player, *i)) {
			fprintf(stderr, "Error playing `%s'\r\n", *i);
			++i;
			continue;
		}
		struct control_msg *msg = (struct control_msg *)message_queue_read(&mq);
		switch(msg->type) {
		case control_msg::MSG_BACK:
			if(i != playlist.begin())
				--i;
			break;
		case control_msg::MSG_FORWARD:
		case control_msg::MSG_FILEDONE:
			++i;
			break;
		case control_msg::MSG_EXIT:
			i = playlist.end();
			break;
		}
		message_queue_message_free(&mq, msg);
		CAStopPlayback(&player);
	}
	CADestroyPlayer(&player);
done:
	return data;
}

int main(int argc, char *argv[]) {
	int res = EXIT_FAILURE;
	bool running = true;
	bool shuffle = false;
	message_queue_init(&mq, sizeof(struct control_msg), 2);
	crossfeed_init(&crossfeed);
	if(argc < 2) {
		fprintf(stderr, "Usage: %s [-s] [-g dBFS] /foo/bar\n", argc == 1 ? argv[0] : "crossfeed-player");
		goto done;
	}
	for(int i=1;i<argc;++i) {
		if(strcmp(argv[i], "-g") == 0) {
			if(++i > argc)
				break;
			set_volume(atof(argv[i]));
			continue;
		}
		if(strcmp(argv[i], "-s") == 0) {
			shuffle = true;
			continue;
		}
		playlist.push_back(argv[i]);
	}
	if(shuffle) {
		srand(time(NULL));
		random_shuffle(playlist.begin(), playlist.end(), [](unsigned int n) {
			return rand() % n;
		});
	}
	pthread_t audio_thread;
	if(pthread_create(&audio_thread, NULL, &audio_threadproc, NULL)) {
		fprintf(stderr, "Error starting audio control thread\n");
		goto done;
	}
	struct termios t_orig_params, t_params;
	tcgetattr(0, &t_params);
	t_orig_params = t_params;
	cfmakeraw(&t_params);
	tcsetattr(0, TCSANOW, &t_params);
	while(running) {
		struct control_msg *msg;
		switch(getchar()) {
		case EOF:
		case 'q':
			msg = (struct control_msg *)message_queue_message_alloc_blocking(&mq);
			msg->type = control_msg::MSG_EXIT;
			message_queue_write(&mq, msg);
			running = false;
			break;
		case '<':
			msg = (struct control_msg *)message_queue_message_alloc_blocking(&mq);
			msg->type = control_msg::MSG_BACK;
			message_queue_write(&mq, msg);
			break;
		case '>':
			msg = (struct control_msg *)message_queue_message_alloc_blocking(&mq);
			msg->type = control_msg::MSG_FORWARD;
			message_queue_write(&mq, msg);
			break;
		case 'c':
			crossfeed.bypass = crossfeed.bypass ? 0 : 1;
			fprintf(stderr, "XFeed: %s    \r", crossfeed.bypass ? "Off" : "On");
			break;
		case '/':
			set_volume(scale_db - 0.5);
			fprintf(stderr, "Volume: %.1f  \r", scale_db);
			break;
		case '*':
			set_volume(scale_db + 0.5);
			fprintf(stderr, "Volume: %.1f  \r", scale_db);
			break;
		}
	}
	pthread_join(audio_thread, NULL);
	tcsetattr(0, TCSANOW, &t_orig_params);
	res = EXIT_SUCCESS;
done:
	message_queue_destroy(&mq);
	return res;
}
