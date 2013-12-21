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

static const float kernel_48k[] = {
	6.2015253e-17, -0.0039839461, 0.036636684, -0.085970297, -0.028689241, -0.011710714,
	-0.063327722, 0.018163972, -0.0176876, -0.057560664, 0.15262495, -0.2834028, 0.5123921,
	-0.22858866, 0.26745284, -0.17582662, -0.10386075, 0.0080126934, -0.27700529, -0.038427804,
	-0.082708158, -0.27534547, 0.41420299, -0.039613277, -0.028892104, -1.0715905, 0.29819348,
	-0.21958114, -0.068755209, 0.23113751, -0.015563318, 0.15876542, 0.21984984, 0.089894362,
	0.32944447, -0.071118779, -0.31842265, -0.0066262791
};

static const float kernel_44k[] = {
	-2.7136378e-17, -0.018747337, 0.021715589, -0.058580723, -0.085795112, 0.015333511,
	-0.014421395, -0.093146324, 0.038963363, 0.095251292, -0.35964823, 0.63512981, -0.33304206,
	0.20017704, -0.018507227, -0.30435652, -0.067316547, -0.040500347, -0.35523808, -0.10085122,
	0.33388123, -0.30160415, 0.22832309, -1.0727667, 0.13886796, 0.20863526, -0.086053632,
	0.19333611, 0.276041, 0.02887051, 0.14308633, 0.26545551, -0.12701088, -0.39847958, -0.037688017
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
