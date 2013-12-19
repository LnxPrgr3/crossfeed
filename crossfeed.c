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
	-5.669545e-19, -0.00076195586, 0.016427275, -0.015503742, -0.0063618617, 0.025785673,
	-0.023885079, 0.02833947, 0.010300335, -0.040894017, 0.0096008433, 0.022971002, -0.011706693,
	0.025878062, -0.017593864, -0.035775889, 0.038184103, 0.013770352, -0.029385436,0.022894062,
	-0.039710201, -0.0060594608, 0.17330474, -0.19427294, 0.67796922, -0.27050647, 0.19732359,
	-0.066463038, -0.099627368, 0.082552381, -0.047839411, 0.042512897, 0.0465695, -0.13621607,
	-0.032034356, 0.073727876, 0.025100961, 0.050125636, -0.07547199, -0.13223848, 0.079970837,
	0.03734991, -0.070256263, 0.060846779, -0.12020037, -0.0040066987, 0.53763312, -0.4398841,
	-0.11437872, -0.95361435, 0.21339208, -0.32299316, -0.25641668, 0.38536364, -0.081158735,
	-0.1193809, 0.041342393, -0.073460318, 0.095952459, 0.22021261, -0.19240645, 0.011648804,
	0.02140441, -0.01793671, 0.065583043, 0.036210332, -0.064252943, 0.061707105, -0.0068526925,
	-0.01026434, 0.021303993, -0.017861687, -0.014283513, -0.00044980977
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
