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

static const float kernel[] = {
	3.0587726e-17, -0.0056460388, 0.051022124, -0.0094165057, -0.021792317, 0.028376782,
	-0.028174553, 0.015822908, 0.010473754, -0.085965388, -0.053120401, -0.023303129, -0.027494108,
	0.011219176, -0.015117921, -0.030191232, 0.064110525, 0.022067929, -0.034555659, 0.0035550345,
	-0.062312953, -0.022548446, 0.15813033, -0.27111578, 0.61055827, -0.28841469, 0.21637601,
	-0.040384188, -0.088467337, 0.057584431, -0.064588994, 0.069902532, 0.11767644, -0.12683418,
	-0.13344301, -0.088813566, -0.12666656, -0.092350014, -0.18703273, -0.098055974, 0.24780546,
	0.13467719, -0.091019586, 0.03432107, -0.19244523, 0.0040873936, 0.6536935, -0.31949449,
	0.096892789, -0.72856224, 0.16797681, -0.45377222, -0.30855018, 0.40489513, -0.10493416,
	-0.088810273, -0.020470267, -0.15007421, -0.046331245, 0.017920788, -0.32838127, 0.17092752,
	0.21841994, 0.17452505, 0.23787333, 0.14964744, -0.052751489, 0.076283768, -0.041450847,
	0.070162848, 0.13571241, -0.031656493, -0.05358373, -0.010663124
};

void crossfeed_init(crossfeed_t *filter) {
	memset(filter, 0, sizeof(crossfeed_t));
}

static inline void crossfeed_process_sample(crossfeed_t *filter, float left, float right,
                                            float *oleft, float *oright) {
	float mid = (left + right) / 2;
	float side = (left - right) / 2;
	float oside = 0;
	filter->mid[(filter->pos + 24) % 74] = mid;
	filter->side[filter->pos] = side;
	if(!filter->bypass) {
		for(unsigned int i=0;i<74;++i) {
			oside += filter->side[(filter->pos + 74 - i) % 74] * kernel[i];
		}
	} else {
		oside = filter->side[(filter->pos + 74 - 24) % 74];
	}
	*oleft = filter->mid[filter->pos] + oside;
	*oright = filter->mid[filter->pos] - oside;
	filter->pos = (filter->pos + 1) % 74;
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
