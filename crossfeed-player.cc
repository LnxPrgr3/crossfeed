/*
 * Copyright (c) 2013 Jeremy Pepper
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of crossfeed nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
	if(crossfeed_init(&crossfeed, player.samplerate)) {
		fprintf(stderr, "Filter not available for %dHz\n", player.samplerate);
		goto destroy_player;
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
destroy_player:
	CADestroyPlayer(&player);
done:
	return data;
}

int main(int argc, char *argv[]) {
	int res = EXIT_FAILURE;
	bool running = true;
	bool shuffle = false;
	message_queue_init(&mq, sizeof(struct control_msg), 2);
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
