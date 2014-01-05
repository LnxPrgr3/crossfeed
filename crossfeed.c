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
	9.05152000e-01, -7.12136000e-02, -3.91498000e-02, -2.89690000e-02, -2.15189000e-02,
	-1.71734000e-02, -1.39221000e-02, -1.13950000e-02, -9.67162000e-03, -7.95421000e-03,
	-6.92881000e-03, -5.58999000e-03, -4.53400000e-03, -1.08068000e-03, -1.57271000e-03,
	-1.09544000e-03, -1.87322000e-03, -1.10872000e-03, -1.64669000e-03, -6.62193000e-04,
	-9.07612000e-04, 4.15401000e-04, 8.27322000e-04, 3.10003000e-03, 7.02294000e-03,
	-3.13349140e-01, -9.23396974e-02, -3.88605068e-02, -2.88940417e-02, -1.70492514e-02,
	-1.44157576e-02, -8.73466591e-03, -8.07778915e-03, -4.46547320e-03, -4.62496094e-03,
	-1.97296737e-03, -2.57395321e-03, -6.00360115e-04, 3.57396716e-03, 3.84352853e-03,
	2.92043543e-03, 2.59248053e-03, 2.02799202e-03, 1.86368478e-03, 1.36542116e-03, 1.17843865e-03,
	5.89547154e-04, 1.58647655e-04, -8.45715000e-04, -2.66189670e-03, -1.00050043e-03,
	-5.58272309e-04, -4.32105496e-04, -3.27153344e-04, -2.83017818e-04, -2.32894728e-04,
	-2.10916338e-04, -1.75771142e-04, -1.59619221e-04, -1.25991522e-04, -1.06000128e-04,
	-6.19181016e-05, -1.06144013e-05
};

static const float kernel_48k[] = {
	8.68683000e-01, -8.47392000e-02, -3.85314000e-02, -2.62095000e-02, -1.63222000e-02,
	-1.22217000e-02, -7.29694000e-03, -4.54808000e-03, 6.64516000e-04, 5.45668000e-04,
	1.27027000e-03, 3.50661000e-03, 1.06110000e-02, -3.74660372e-01, -9.53704376e-02,
	-3.36139872e-02, -2.31058774e-02, -1.05480262e-02, -8.12235532e-03, -2.83043835e-03,
	-3.05196618e-03, 4.42931056e-03, 2.13729271e-03, 7.26001004e-04, -1.03171371e-03,
	-4.85908871e-03, -1.80205494e-03, -9.66361635e-04, -7.33541473e-04, -4.71786039e-04,
	-3.49177429e-04, -2.14919607e-04, -1.36118568e-04, -1.04553579e-06
};

static const float kernel_44k[] = {
	8.61947000e-01, -8.46179000e-02, -4.07772000e-02, -2.37515000e-02, -1.80934000e-02,
	-9.18115000e-03, -8.56252000e-03, 5.45115000e-04, -2.39949000e-03, 3.37018000e-03,
	1.81437000e-04, 1.29068000e-02, -3.81658926e-01, -9.55876045e-02, -3.12767062e-02,
	-2.23735404e-02, -8.20664016e-03, -7.62852422e-03, -9.70449992e-04, 9.24487089e-04,
	4.26695972e-03, 5.76634130e-04, 4.22985197e-04, -5.44158043e-03, -1.94976407e-03,
	-9.52879810e-04, -7.35752446e-04, -4.47167004e-04, -3.35820289e-04, -1.82063843e-04,
	-7.04588665e-05
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
