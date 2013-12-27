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
	9.09067e-21, 0, 2.2681e-07, 0, -5.33741e-07, 0, 2.43678e-06, 0, -1.74938e-06, 0, 8.01894e-06, 0,
	 -3.03924e-06, 0, 1.8471e-05, 0, -3.79016e-06, 0, 3.55831e-05, 0, -3.47074e-06, 0, 6.13814e-05,
	 0, -1.78117e-06, 0, 9.80102e-05, 0, 1.11276e-06, 0, 0.000147533, 0, 4.20909e-06, 0,
	 0.00021164, 0, 5.19175e-06, 0, 0.000291256, 0, -2.39687e-07, 0, 0.000386083, 0, -1.91854e-05,
	 0, 0.000493983, 0, -6.26508e-05, -2.60746e-21, 0.000610464, -2.06738e-08, -0.000147022,
	 8.93772e-08, 0.000727985, -4.20655e-07, -0.000295953, -5.42481e-08, 0.000835235, -1.91513e-06,
	 -0.000543392, -1.16614e-06, 0.000916547, -5.40316e-06, -0.000938161, -4.13028e-06,
	 0.000951216, -1.19469e-05, -0.00155205, -9.94155e-06, 0.000913153, -2.27101e-05, -0.00249437,
	 -1.96185e-05, 0.000771332, -3.88405e-05, -0.003939, -3.4041e-05, 0.000491258, -6.12654e-05,
	 -0.00616617, -5.37151e-05, 1.04917e-05, -9.03945e-05, -0.00945994, -7.84247e-05, -0.00159166,
	 -0.000125739, -0.00811519, -0.000106802, -0.0567622, -0.000165419, 0.314512, -0.000135812,
	 -0.614312, -0.000205614, -0.190102, -0.000160163, 0.0840841, -0.000239958, 0.180259,
	 -0.000171728, 0.174123, -0.00025895, 0.165522, -0.00015894, 0.133346, -0.000249362, 0.120255,
	 -0.000106295, 0.0946442, -0.000193798, 0.0865369, 6.08538e-06, 0.0679219, -7.03231e-05,
	 0.0636857, 0.000202736, 0.0498708, 0.000147708, 0.0479883, 0.000512914, 0.0373528, 0.00049177,
	 0.0368344, 0.000970662, 0.0283692, 0.000998036, 0.0286286, 0.00161467, 0.0217139, 0.00170728,
	 0.0224068, 0.00248819, 0.0166522, 0.00266486, 0.0175749, 0.0036391, 0.0127262, 0.00392089,
	 0.0137563, 0.00512042, 0.00964089, 0.00553102, 0.0107044, 0.00699137, 0.00720025, 0.007558,
	 0.00825192, 0.00931982, 0.00526829, 0.0100748, 0.00628082, 0.0121862, 0.00374672, 0.0131701,
	 0.00470362, 0.0156907, 0.00256124, 0.0169561, 0.00345278, 0.0199638, 0.00165278, 0.0215834,
	 0.00247349, 0.0251838, 0.00097263, 0.0272604, 0.00171974, 0.031602, 0.000479113, 0.034284,
	 0.00115188, 0.0395769, 0.000136142, 0.043074, 0.000735158, 0.0496036, -8.78812e-05, 0.0541786,
	 0.000439066, 0.0622573, -0.000220553, 0.0680645, 0.000237122, 0.0776273, -0.000285461,
	 0.0837375, 0.000106465, 0.0920331, -0.000302534, 0.0910706, 2.79315e-05, 0.0799347,
	 -0.00028807, 0.0306699, -1.41029e-05, -0.0814418, -0.00025499, -0.199422, -3.205e-05,
	 0.0715177, -0.00021302, -0.0151182, -3.53241e-05, -0.0079912, -0.000169119, -0.00335517,
	 -3.07713e-05, -0.00575163, -0.000127874, -0.0023135, -2.30321e-05, -0.00382604, -9.19622e-05,
	 -0.00157624, -1.50167e-05, -0.00257855, -6.25834e-05, -0.0010261, -8.30666e-06, -0.00173363,
	 -3.99141e-05, -0.000623975, -3.53327e-06, -0.0011455, -2.3437e-05, -0.000337022, -7.23207e-07,
	 -0.000733089, -1.22568e-05, -0.000139265, 4.75998e-07, -0.00044635, -5.33111e-06,
	 -1.00052e-05, 5.99016e-07, -0.000251361, -1.6131e-06, 6.76078e-05, 2.4337e-07, -0.000123508,
	 -1.48386e-07, 0.000107447, -4.83423e-21, -4.41462e-05, 0, 0.000120902, 0, 1.09464e-06, 0,
	 0.000117126, 0, 2.32893e-05, 0, 0.000103258, 0, 3.08101e-05, 0, 8.46363e-05, 0, 2.98018e-05,
	 0, 6.50026e-05, 0, 2.45809e-05, 0, 4.67762e-05, 0, 1.79914e-05, 0, 3.1318e-05, 0, 1.1746e-05,
	 0, 1.91972e-05, 0, 6.71937e-06, 0, 1.04323e-05, 0, 3.20953e-06, 0, 4.69396e-06, 0,
	 1.13907e-06, 0, 1.46191e-06, 0, 2.07949e-07, 0, 1.37534e-07, 0, -1.87068e-21
};

static const float kernel_48k[] = {
	-1.45019e-19, 0, -9.86267e-06, 0, 4.81157e-05, 0, -9.58106e-05, 0, 0.000231164, 0, -0.000300186,
	 0, 0.000636493, 0, -0.000695002, 0, 0.00139558, 0, -0.00142194, 0, 0.00268829, 0, -0.00275143,
	 0, 0.0047547, 2.65755e-20, -0.00519895, 1.85733e-06, 0.00795119, -9.91472e-06, -0.009781,
	 1.3943e-05, 0.0129725, -5.21481e-05, -0.0187232, 3.06613e-05, 0.021767, -0.000151601,
	 -0.0382016, 4.62258e-05, 0.0429427, -0.000336012, -0.0979512, 6.66409e-05, 0.179406,
	 -0.000620108, -0.542705, 0.000130314, -0.0253268, -0.000977163, 0.46559, 0.000339698,
	 0.200649, -0.00130089, 0.253065, 0.000898529, 0.0954927, -0.00136642, 0.144039, 0.00214474,
	 0.045751, -0.000801966, 0.0899948, 0.00456929, 0.0214168, 0.000920503, 0.0589418, 0.00881922,
	 0.00841873, 0.00447886, 0.0391164, 0.0157005, 0.00124897, 0.0107031, 0.0257415, 0.0262264,
	 -0.00251333, 0.0206446, 0.0165762, 0.0418101, -0.00413166, 0.0358275, 0.010369, 0.0648136,
	 -0.00440204, 0.0589034, 0.00628545, 0.0998847, -0.0038973, 0.0947842, 0.00369899, 0.155884,
	 -0.00304263, 0.144984, 0.00212337, 0.218557, -0.0021337, -0.00281997, 0.00119339, -0.208922,
	 -0.00134629, 0.0443423, 0.000653388, -0.0360726, -0.000755302, 0.00855298, 0.000339563,
	 -0.0150584, -0.000364168, 0.00438842, 0.000156405, -0.0074828, -0.000137642, 0.00290322,
	 5.3175e-05, -0.00380278, -2.9478e-05, 0.00202304, 5.80063e-06, -0.00188836, 9.06814e-20,
	 0.00136548, 0, -0.000900596, 0, 0.000855395, 0, -0.000410607, 0, 0.000481337, 0, -0.000178202,
	 0, 0.000232589, 0, -7.10962e-05, 0, 8.74341e-05, 0, -2.2254e-05, 0, 1.83869e-05, 0,
	 -2.37552e-06, 0, -5.45984e-20
};

static const float kernel_44k[] = {
	-1.07273e-19, 0, -8.33008e-06, 0, 4.22359e-05, 0, -7.92288e-05, 0, 0.000209849, 0, -0.000244748,
	 0, 0.000594735, 0, -0.000567201, 0, 0.0013304, 0, -0.00118781, 0, 0.00258489, 0, -0.00240942,
	 1.8865e-20, 0.00455509, 1.69101e-06, -0.00486224, -8.98529e-06, 0.00750974, 1.10051e-05,
	 -0.00988377, -5.13101e-05, 0.0120476, 1.70745e-05, -0.0206704, -0.000157944, 0.0203976,
	 7.70192e-06, -0.0472273, -0.000361324, 0.0442553, -1.1883e-05, -0.0640458, -0.000668402,
	 -0.559691, 1.66161e-05, 0.469563, -0.00101279, 0.276615, 0.00025813, 0.278373, -0.00119311,
	 0.127532, 0.00104362, 0.147099, -0.000815225, 0.0609495, 0.00291621, 0.0865488, 0.000741769,
	 0.0299313, 0.00664984, 0.0536754, 0.00432879, 0.0136496, 0.0132499, 0.0336066, 0.0110245,
	 0.00465162, 0.023991, 0.020665, 0.0222103, -0.000157587, 0.0406299, 0.0122695, 0.0399091,
	 -0.00236017, 0.0661059, 0.00697199, 0.0677889, -0.00295711, 0.106385, 0.00378914, 0.113239,
	 -0.00266673, 0.172577, 0.00198752, 0.177322, -0.00200437, 0.2008, 0.00102427, -0.207886,
	 -0.00130256, -0.0463071, 0.000526927, 0.00927798, -0.000733328, -0.0214309, 0.000267482,
	 0.0033519, -0.000348235, -0.0095395, 0.000124657, 0.00225406, -0.000127928, -0.00449576,
	 4.37188e-05, 0.00173594, -2.64247e-05, -0.0020629, 4.94131e-06, 0.0012735, 6.81904e-20,
	 -0.000892126, 0, 0.000840537, 0, -0.000361657, 0, 0.000484682, 0, -0.000139861, 0,
	 0.000234647, 0, -5.21585e-05, 0, 8.6802e-05, 0, -1.65091e-05, 0, 1.77138e-05, 0, -1.88417e-06,
	 0, -4.39881e-20
};

int crossfeed_init(crossfeed_t *filter, int samplerate) {
	memset(filter, 0, sizeof(crossfeed_t));
	switch(samplerate) {
	case 96000:
		filter->filter = kernel_96k;
		filter->delay = 99;
		filter->len = 147;
		break;
	case 48000:
		filter->filter = kernel_48k;
		filter->delay = 49;
		filter->len = 73;
	case 44100:
		filter->filter = kernel_44k;
		filter->delay = 46;
		filter->len = 68;
	default:
		return -1;
	}
	return 0;
}

static inline void crossfeed_process_sample(crossfeed_t *filter, float left, float right,
                                            float *oleft, float *oright) {
	float tleft = (left - right) / 2;
	float mid = (left + right) / 2;
	float tright = (right - left) / 2;
	float toleft = 0, toright = 0;
	filter->left[filter->pos] = tleft;
	filter->mid[(filter->pos + filter->delay) % filter->len] = mid;
	filter->right[filter->pos] = tright;
	if(!filter->bypass) {
		for(unsigned int i=0;i<filter->len;++i) {
			toleft += filter->left[(filter->pos + filter->len - i) % filter->len] * filter->filter[2*i];
			toleft += filter->right[(filter->pos + filter->len - i) % filter->len] * filter->filter[2*i+1];
			toright += filter->right[(filter->pos + filter->len - i) % filter->len] * filter->filter[2*i];
			toright += filter->left[(filter->pos + filter->len - i) % filter->len] * filter->filter[2*i+1];
		}
	} else {
		toleft = filter->left[(filter->pos + filter->len - filter->delay) % filter->len];
		toright = filter->right[(filter->pos + filter->len - filter->delay) % filter->len];
	}
	*oleft = filter->mid[filter->pos] + 0.5*(toleft - toright);
	*oright = filter->mid[filter->pos] - 0.5*(toleft - toright);
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
