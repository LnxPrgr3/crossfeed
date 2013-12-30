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
#include <math.h>

int crossfeed_init(crossfeed_t *filter, float samplerate) {
	memset(filter, 0, sizeof(crossfeed_t));
	if(samplerate < 32000 || samplerate > 192000)
		return -1;
	filter->b = 0.287681 / (samplerate / 44100);
	//filter->b = 0.5 / (samplerate / 96000);
	filter->a = 0.613331 * (1 - filter->b);
	filter->higbb = 0.995784 / (samplerate / 44100);
	filter->higha0 = 0.688758 * ((1 + filter->b)/2);
	filter->higha1 = -filter->higha0;
	filter->delay = round((samplerate * 250) / 1000000);
	return 0;
}

static inline void crossfeed_process_sample(crossfeed_t *filter, float left, float right,
                                            float *oleft, float *oright) {
	float mid = (left + right) / 2;
	float side = (left - right) / 2;
	float feedback = filter->feedback;
	filter->feedback = (filter->side[filter->pos] * filter->a) + (filter->feedback * filter->b);
	filter->highfeedback = ((side - filter->feedback) * filter->higha0 + (filter->side[(filter->pos + filter->delay + 1) % filter->delay] - feedback) * filter->higha1) + (filter->highfeedback * filter->b);
	filter->side[filter->pos] = side;
	if(!filter->bypass) {
		*oleft = mid + side + filter->highfeedback;
		*oright = mid - (side + filter->highfeedback);
	} else {
		*oleft = mid + side;
		*oright = mid - side;
	}
	filter->pos = (filter->pos + 1) % filter->delay;
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
