#include <stdlib.h>
#include <stdio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include "message_queue.h"

struct control_msg {
	int id;
	enum {MSG_FILEDONE} type;
};

static struct message_queue mq;

static OSStatus render_callback(void *data, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData) {
	uintptr_t end = (uintptr_t)data;
	if(inTimeStamp->mSampleTime >= end) {
		struct control_msg *msg = message_queue_message_alloc_blocking(&mq);
		msg->type = MSG_FILEDONE;
		message_queue_write(&mq, msg);
	}
	return 0;
}

int main(int argc, char *argv[]) {
	int res = EXIT_FAILURE;
	message_queue_init(&mq, sizeof(struct control_msg), 2);
	if(argc < 2) {
		fprintf(stderr, "Usage: %s /foo/bar\n", argc == 1 ? argv[0] : "crossfeed-player");
		goto done;
	}
	AudioFileID audioFile;
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (UInt8*)argv[1], strlen(argv[1]), false);
	if(!url) {
		fprintf(stderr, "Failed to translate `%s' to URL\n", argv[1]);
		goto done;
	}
	if(AudioFileOpenURL(url, kAudioFileReadPermission, 0, &audioFile)) {
		fprintf(stderr, "Failed to open `%s'\n", argv[1]);
		goto release_url;
	}
	AudioStreamBasicDescription fileFormat;
	UInt32 propsize = sizeof(AudioStreamBasicDescription);
	if(AudioFileGetProperty(audioFile, kAudioFilePropertyDataFormat, &propsize, &fileFormat)) {
		fprintf(stderr, "Failed to get number of channels for `%s'\n", argv[1]);
		goto close_file;
	}
	AUGraph graph;
	if(NewAUGraph(&graph)) {
		fprintf(stderr, "Failed to create audio graph\n");
		goto close_file;
	}
	AudioComponentDescription cd = {
		.componentType = kAudioUnitType_Output,
		.componentSubType = kAudioUnitSubType_DefaultOutput,
		.componentManufacturer = kAudioUnitManufacturer_Apple
	};
	AUNode outputNode;
	if(AUGraphAddNode(graph, &cd, &outputNode)) {
		fprintf(stderr, "Failed to open default sound output device\n");
		goto close_graph;
	}
	AUNode fileNode;
	cd.componentType = kAudioUnitType_Generator;
	cd.componentSubType = kAudioUnitSubType_AudioFilePlayer;
	if(AUGraphAddNode(graph, &cd, &fileNode)) {
		fprintf(stderr, "Failed to open AudioFilePlayer\n");
		goto close_graph;
	}
	if(AUGraphOpen(graph)) {
		fprintf(stderr, "Failed to open audio graph\n");
		goto close_graph;
	}
	AudioUnit fileAU;
	if(AUGraphNodeInfo(graph, fileNode, NULL, &fileAU)) {
		fprintf(stderr, "Failed to get AudioFilePlayer info\n");
		goto close_graph;
	}
	AudioStreamBasicDescription outputFormat;
	propsize = sizeof(AudioStreamBasicDescription);
	if(AudioUnitGetProperty(fileAU, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outputFormat, &propsize)) {
		fprintf(stderr, "Failed to get AudioFilePlayer stream format\n");
		goto close_graph;
	}
	outputFormat.mFormatFlags = (outputFormat.mFormatFlags & ~(kAudioFormatFlagIsSignedInteger)) | kLinearPCMFormatFlagIsFloat;
	outputFormat.mSampleRate = 96000.;
	outputFormat.mChannelsPerFrame = fileFormat.mChannelsPerFrame;
	if(AudioUnitSetProperty(fileAU, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &outputFormat, sizeof(outputFormat))) {
		fprintf(stderr, "Failed to set AudioFilePlayer stream format\n");
		goto close_graph;
	}
	if(AudioUnitSetProperty(fileAU, kAudioUnitProperty_ScheduledFileIDs, kAudioUnitScope_Global, 0, &audioFile, sizeof(audioFile))) {
		fprintf(stderr, "Failed to setup file for playback\n");
		goto close_graph;
	}
	if(AUGraphConnectNodeInput(graph, fileNode, 0, outputNode, 0)) {
		fprintf(stderr, "Failed to setup audio graph connections\n");
		goto close_graph;
	}
	if(AUGraphInitialize(graph)) {
		fprintf(stderr, "Failed to initialize audio graph\n");
		goto close_graph;
	}
	usleep(10000);
	UInt64 packets;
	propsize = sizeof(packets);
	if(AudioFileGetProperty(audioFile, kAudioFilePropertyAudioDataPacketCount, &propsize, &packets)) {
		fprintf(stderr, "Failed to determine file length\n");
		goto uninit_graph;
	}
	if(AudioUnitAddRenderNotify(fileAU, render_callback, (void *)(uintptr_t)(packets * fileFormat.mFramesPerPacket * 96000/(fileFormat.mSampleRate)))) {
		fprintf(stderr, "Failed to add render callback\n");
		goto uninit_graph;
	}
	Float64 fileDuration = (packets * fileFormat.mFramesPerPacket) / fileFormat.mSampleRate;
	ScheduledAudioFileRegion rgn = {
		.mTimeStamp = {
			.mFlags = kAudioTimeStampSampleTimeValid,
			.mSampleTime = 0
		},
		.mCompletionProc = NULL,
		.mAudioFile = audioFile,
		.mLoopCount = 0,
		.mStartFrame = 0,
		.mFramesToPlay = (UInt32)(packets * fileFormat.mFramesPerPacket)
	};
	if(AudioUnitSetProperty(fileAU, kAudioUnitProperty_ScheduledFileRegion, kAudioUnitScope_Global, 0, &rgn, sizeof(rgn))) {
		fprintf(stderr, "Failed to schedule file for playback\n");
		goto uninit_graph;
	}
	UInt32 defaultVal = 0;
	if(AudioUnitSetProperty(fileAU, kAudioUnitProperty_ScheduledFilePrime, kAudioUnitScope_Global, 0, &defaultVal, sizeof(defaultVal))) {
		fprintf(stderr, "Failed to schedule file for playback\n");
		goto uninit_graph;
	}
	AudioTimeStamp startTime = {
		.mFlags = kAudioTimeStampSampleTimeValid,
		.mSampleTime = -1
	};
	if(AudioUnitSetProperty(fileAU, kAudioUnitProperty_ScheduleStartTimeStamp, kAudioUnitScope_Global, 0, &startTime, sizeof(startTime))) {
		fprintf(stderr, "Failed to schedule file for playback\n");
		goto uninit_graph;
	}
	if(AUGraphStart(graph)) {
		fprintf(stderr, "Failed to start playback\n");
		goto uninit_graph;
	}
	message_queue_message_free(&mq, message_queue_read(&mq));
	res = EXIT_SUCCESS;
	AUGraphStop(graph);
uninit_graph:
	AUGraphUninitialize(graph);
close_graph:
	AUGraphClose(graph);
close_file:
	AudioFileClose(audioFile);
release_url:
	CFRelease(url);
done:
	message_queue_destroy(&mq);
	return res;
}
