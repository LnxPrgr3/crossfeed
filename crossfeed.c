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

#include "crossfeed.h"
#include <string.h>

static const float kernel_96k[] = {
	1, -0.0091856609, -0.0095313762, -0.0094959013, -0.0095133384, -0.0091595304, -0.0089518565,
	-0.0084889918, -0.0083923074, -0.0082735254, -0.008871099, -0.0097754859, -0.011849761,
	-0.014595947, -0.019015331, -0.02440621, -0.031957076, -0.040552728, -0.051731966, -0.063484813,
	-0.078258299, -0.091407124, -0.10916878, -0.099287143, -0.00034654326
};

static const float kernel_48k[] = {
	1, -0.018384673, -0.018457898, -0.016591364, -0.016537887, -0.017895616, -0.027158923,
	-0.04339092, -0.07543667, -0.11570891, -0.17908852, -0.19217771, -0.00065829085
};

static const float kernel_44k[] = {
	1, -0.020730974, -0.019022247, -0.018487588, -0.017449792, -0.024426898, -0.038292188,
	-0.071077453, -0.11469807, -0.1874774, -0.20722037, -0.00073042441
};

int crossfeed_init(crossfeed_t *filter, int samplerate) {
	memset(filter, 0, sizeof(crossfeed_t));
	switch(samplerate) {
	case 44100:
		filter->filter = kernel_44k;
		filter->delay = 0;
		filter->len = sizeof(kernel_44k)/sizeof(float);
		break;
	case 48000:
		filter->filter = kernel_48k;
		filter->delay = 0;
		filter->len = sizeof(kernel_48k)/sizeof(float);
		break;
	case 96000:
		filter->filter = kernel_96k;
		filter->delay = 0;
		filter->len = sizeof(kernel_96k)/sizeof(float);
		break;
	default:
		return -1;
	}
	return 0;
}

static inline void crossfeed_process_sample(crossfeed_t *filter, float left, float right,
                                            float *oleft, float *oright) {
	float mid = (left + right) / 2;
	float side = (left - right) / 2;
	float oside = 0;
	filter->mid[(filter->pos + filter->delay) % filter->len] = mid;
	filter->side[filter->pos] = side;
	if(!filter->bypass) {
		for(unsigned int i=0;i<filter->len;++i) {
			oside += filter->side[(filter->pos + filter->len - i) % filter->len] * filter->filter[i];
		}
	} else {
		oside = filter->side[(filter->pos + filter->len - filter->delay) % filter->len];
	}
	*oleft = filter->mid[filter->pos] + oside;
	*oright = filter->mid[filter->pos] - oside;
	filter->pos = (filter->pos + 1) % filter->len;
}

void crossfeed_filter(crossfeed_t *filter, float *input, float *output, unsigned int size) {
	for(unsigned int i=0;i<size;++i) {
		crossfeed_process_sample(filter, input[i*2], input[i*2+1], &output[i*2],
		                         &output[i*2+1]);
	}
}

void crossfeed_filter_inplace_noninterleaved(crossfeed_t *filter, float *left, float *right,
                                             unsigned int size) {
	for(unsigned int i=0;i<size;++i) {
		crossfeed_process_sample(filter, left[i], right[i], &left[i],
		                         &right[i]);
	}
}
