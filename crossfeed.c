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
	-4.9117538e-17, -8.7516346e-05, 0.00079945743, -0.0018196403, -0.0013150165, -0.0053273141,
	-0.00019307568, 0.012711092, 0.028703772, 0.031438619, -0.05142916, 0.08934696, 0.069599554,
	-0.078820422, 0.098732702, 0.087771811, 0.069281362, 0.0071823173, -0.11584523, -0.054059815,
	-0.075850263, 0.10499027, -0.03773731, 0.12588364, 0.59971637, 0.079907477, -0.013934914,
	0.12390097, -0.12513193, -0.056923307, -0.20932582, 0.021347078, 0.14282791, 0.23368703,
	0.19144253, -0.23383677, 0.32316995, 0.25361747, -0.19848318, 0.22031236, 0.21342027,
	0.12635995, -0.014269463, -0.19632295, -0.1059119, -0.10456789, 0.092574336, -0.0083819032,
	0.078643344, -0.74921393, 0.11697309, -0.034292173, 0.11101205, -0.087618321, -0.035696872,
	-0.12973954, 0.017204605, 0.088497236, 0.10286717, 0.091760695, -0.086641118, 0.11649897,
	0.064329952, -0.041086931, 0.048295598, 0.035708219, 0.020751882, -0.0030160618, -0.012678616,
	-0.0066379653, -0.0032804748, 0.0015182693, 0.00049011619, 5.1157844e-05
};

static const float kernel_48k[] = {
	-3.2359345e-17, 0.00082109403, -0.0075924527, 0.011327101, 0.0044941618, -0.022952406,
	-0.010943388, -0.00057246088, 0.099134311, -0.24252507, 0.10219692, 0.19819005, 0.9499318,
	0.18954262, 0.10371193, -0.29921561, 0.18358216, 0.036954589, -0.10728865, -0.0095369611,
	0.021457382, 0.15497215, -0.3099443, 0.11324875, 0.20265557, -0.48978409, 0.18358231,
	0.08808434, -0.24378306, 0.13002266, -0.0045062029, -0.034341604, -0.00040533632,
	-0.0036935641, 0.0170469, -0.015050028, 0.00061125011, 0.001167611
};

static const float kernel_44k[] = {
	-3.8828242e-17, 0.001674715, -0.0053713704, -0.021496296, 0.044228066, -0.027272897, 0.094330154,
	-0.14820284, -0.057946861, 0.18637878, 0.22356145, 0.81390303, 0.18950203, 0.24311683,
	-0.14548977, -0.24569078, 0.30518913, -0.12530276, 0.22883382, -0.26587284, -0.082381584,
	0.23484258, 0.22289547, -0.58032656, 0.19660917, 0.20513113, -0.085549667, -0.14047281,
	0.14128554, -0.044993144, 0.047974352, -0.026897335, -0.0043590888, 0.0069963727, 0.0018284065
};

int crossfeed_init(crossfeed_t *filter, int samplerate) {
	memset(filter, 0, sizeof(crossfeed_t));
	switch(samplerate) {
	case 44100:
		filter->filter = kernel_44k;
		filter->delay = 11;
		filter->len = 35;
		break;
	case 48000:
		filter->filter = kernel_48k;
		filter->delay = 12;
		filter->len = 38;
		break;
	case 96000:
		filter->filter = kernel_96k;
		filter->delay = 24;
		filter->len = 74;
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
