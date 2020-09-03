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
#include <complex.h>
#include <math.h>

static const double K_0 = 32666.54325008036;
static const double K_1 = -15826.3499539613;
static const double K_2 = 1845.638326057535;

static const double p_0 = -25198.03902718557;
static const double p_1 = -20533.88090349076;
static const double p_2 = -4801.960972814431;

static complex double analog_prototype(complex double s) {
	return K_0/(s-p_0) + K_1/(s-p_1) + K_2/(s-p_2);
}

int crossfeed_init(crossfeed_t *filter, int samplerate) {
	memset(filter, 0, sizeof(crossfeed_t));

	if (samplerate < 8000 || samplerate > 352800)
		return -1;

	double pz_0 = exp(p_0*(1. / samplerate));
	double pz_1 = exp(p_1*(1. / samplerate));
	double pz_2 = exp(p_2*(1. / samplerate));

	double gain = cabs(analog_prototype(0)) /
		fabs((K_0 / (1 - pz_0) + K_1 / (1 - pz_1) + K_2 / (1 - pz_2)));

	double Kz_0 = gain * K_0;
	double Kz_1 = gain * K_1;
	double Kz_2 = gain * K_2;

	complex double f_p = cexp(2*atan(1)*I);
	double H_1 = cabs(analog_prototype(2*atan(1)*samplerate*I)) /
		cabs(Kz_0 / (1. - pz_0 / f_p) + Kz_1 / (1. - pz_1 / f_p) + Kz_2 / (1. - pz_2 / f_p));

	double c_0 = (1 + sqrt(2*H_1*H_1-1)) / 2;
	double c_1 = 1 - c_0;

	double ap_0 = (Kz_0 + Kz_1 + Kz_2)/2;
	double ap_1 = (-Kz_0*pz_1 - Kz_0*pz_2 - Kz_1*pz_0 - Kz_1*pz_2 - Kz_2*pz_0 - Kz_2*pz_1)/2;
	double ap_2 = (Kz_0*pz_1*pz_2 + Kz_1*pz_0*pz_2 + Kz_2*pz_0*pz_1)/2;

	filter->a_0 = ap_0*c_0;
	filter->a_1 = ap_0*c_1 + ap_1*c_0;
	filter->a_2 = ap_1*c_1 + ap_2*c_0;
	filter->a_3 = ap_2*c_1;

	filter->b_1 = pz_2 + pz_1 + pz_0;
	filter->b_2 = -(pz_0*pz_1 + pz_0*pz_2 + pz_1*pz_2);
	filter->b_3 = pz_0*pz_1*pz_2;

	filter->delay = round(0.0002 * samplerate);

	return 0;
}

static inline void crossfeed_process_sample(crossfeed_t *filter, float left, float right,
                                            float *oleft, float *oright) {
	const int len = 74;
	float mid = (left + right) / 2;
	float side = (left - right) / 2;
	filter->mid[filter->pos] = mid;
	filter->side[filter->pos] = side;

	float oside = filter->a_0 * filter->side[(filter->pos + len - filter->delay) % len] +
		filter->a_1 * filter->side[(filter->pos + len - filter->delay - 1) % len] +
		filter->a_2 * filter->side[(filter->pos + len - filter->delay - 2) % len] +
		filter->a_3 * filter->side[(filter->pos + len - filter->delay - 3) % len] +
		filter->b_1 * filter->o[filter->opos] +
		filter->b_2 * filter->o[(filter->opos + 3 - 1) % 3] +
		filter->b_3 * filter->o[(filter->opos + 3 - 2) % 3];

	filter->opos = (filter->opos + 1) % 3;
	filter->o[filter->opos] = oside;

	if (!filter->bypass) {
		side -= oside;
		mid += oside;
	}

	*oleft = mid + side;
	*oright = mid - side;
	filter->pos = (filter->pos + 1) % len;
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
