#ifndef CAUTIL_H
#define CAUTIL_H
#include <AudioToolbox/AudioToolbox.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PlayerEvent {
	struct Player *player;
	enum {
		PLAYER_RENDER = 'rndr',
		PLAYER_DONE = 'done'
	} type;
	float *left, *right;
	unsigned int size;
};

typedef void (*PlayerEventHandler)(struct PlayerEvent *);

struct Player {
	AUGraph graph;
	AUNode outputNode;
	AUNode fileNode;
	AudioUnit fileAU;
	AudioFileID audioFile;
	uintptr_t samples;
	int playing;
	PlayerEventHandler handleEvent;
};


OSStatus CAInitPlayer(struct Player *player, PlayerEventHandler eventHandler);
OSStatus CAPlayFile(struct Player *player, const char *path);
void CAStopPlayback(struct Player *player);
void CADestroyPlayer(struct Player *player);

#ifdef __cplusplus
}
#endif

#endif
