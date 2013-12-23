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
	-1.99794e-19,0,-3.38556e-06,0,1.51975e-05,0,-3.34012e-05,0,6.81922e-05,0,-0.000103601,0,
	0.00017481,0,-0.000231418,0,0.000360201,0,-0.000446112,0,0.000665305,0,-0.000798187,0,
	0.00116012,0,-0.00137959,0,0.00197425,0,-0.00237337,0,0.00337813,0,-0.00419513,0,0.0060381,0,
	-0.00798174,0,0.0119872,0,-0.0175728,0,0.0285943,0,-0.0466785,0,0.0777295,-2.79864e-20,-0.11652,
	-4.71559e-07,0.137225,1.92777e-06,-0.0744788,-4.55151e-06,-0.100132,7.75667e-06,0.228673,
	-1.36772e-05,-0.0554812,1.76459e-05,-0.187273,-2.92016e-05,-0.0352108,3.18912e-05,0.201198,
	-5.28443e-05,0.153781,5.09546e-05,0.0248635,-8.6663e-05,-0.151825,7.55198e-05,-0.181247,
	-0.000132986,-0.182836,0.000106589,-0.0851509,-0.00019427,-0.0331407,0.000145611,0.0584256,
	-0.000272896,0.0776506,0.000194686,0.12832,-0.000370892,0.113272,0.000256842,0.13741,
	-0.000489585,0.106132,0.000336369,0.11992,-0.000629164,0.0846621,0.000439151,0.0964775,
	-0.000788195,0.0625165,0.000573184,0.0753332,-0.000963134,0.0442157,0.000748971,0.0585541,
	-0.00114777,0.0303541,0.000980097,0.0458823,-0.00133269,0.0202495,0.0012837,0.0364518,
	-0.00150476,0.0130015,0.00168106,0.029406,-0.0016465,0.00783332,0.0021983,0.0240661,-0.00173558,
	0.00415409,0.00286726,0.0199361,-0.00174393,0.00154012,0.00372666,0.01667,-0.00163656,
	-0.000306238,0.00482409,0.0140281,-0.00136998,-0.00159224,0.00621883,0.0118495,-0.00088901,
	-0.00246441,0.00798691,0.0100236,-0.000121976,-0.00302391,0.0102289,0.00847491,0.00102796,
	-0.00334603,0.013084,0.00715145,0.00269751,-0.00348588,0.0167532,0.00601663,0.00509001,
	-0.00348495,0.0215418,0.00504189,0.00852371,-0.00337749,0.0279361,0.00420757,0.0135239,
	-0.00319134,0.0367524,0.00349547,0.0210028,-0.00294896,0.0494284,0.00289,0.0326194,-0.00266988,
	0.0686098,0.00237809,0.0514635,-0.00237141,0.0991994,0.00194756,0.0828659,-0.00206761,0.148752,
	0.00158711,0.130048,-0.00176941,0.210263,0.00128621,0.128338,-0.00148639,0.0816554,0.00103617,
	-0.387042,-0.00122494,-0.00546887,0.000828689,0.0835148,-0.000989684,-0.0731005,0.000656985,
	0.0365672,-0.000783106,-0.0356078,0.000515319,0.019321,-0.000605857,-0.0218309,0.000398536,
	0.0124943,-0.000457197,-0.0148494,0.000302716,0.00890183,-0.000335381,-0.0106343,0.000224612,
	0.00668522,-0.000237939,-0.00783035,0.000161497,0.00517586,-0.000161905,-0.00585322,0.000111185,
	0.00407807,-0.000104293,-0.004408,7.19473e-05,0.00324223,-6.2103e-05,-0.0033279,4.22881e-05,
	0.00258528,-3.26608e-05,-0.00251024,2.10795e-05,0.00205783,-1.36594e-05,-0.00188715,7.45808e-06,
	0.00162883,-3.23878e-06,-0.00141123,8.19302e-07,0.00127774,4.50481e-20,-0.00104793,0,
	0.000990266,0,-0.000771369,0,0.000755855,0,-0.000561674,0,0.000566267,0,-0.000403489,0,
	0.000414692,0,-0.000284858,0,0.000295306,0,-0.000196519,0,0.000202979,0,-0.000131318,0,
	0.000133168,0,-8.37892e-05,0,8.18815e-05,0,-4.98091e-05,0,4.56573e-05,0,-2.63209e-05,0,
	2.15723e-05,0,-1.11175e-05,0,7.23353e-06,0,-2.67207e-06,0,7.53649e-07,0,3.77532e-20
};

static const float kernel_48k[] = {
	-5.68637e-20,0,-4.62659e-06,0,1.80883e-05,0,-4.55346e-05,0,8.43285e-05,0,-0.000141811,0,
	0.000228233,0,-0.000317952,0,0.000497527,0,-0.000610968,0,0.000960418,0,-0.00107651,0,
	0.00170594,4.34065e-20,-0.00180225,2.2054e-06,0.00284382,-1.71214e-05,-0.0029342,1.24115e-05,
	0.0045103,-9.23738e-05,-0.00472766,1.29445e-05,0.00689598,-0.000269692,-0.00765313,-1.17249e-05,
	0.0103378,-0.00059028,-0.0126483,-4.91118e-05,0.0155823,-0.00105716,-0.0217303,-1.3982e-05,
	0.024008,-0.00157585,-0.0356909,0.000316152,0.00721597,-0.00187891,0.132731,0.00137316,
	-0.578593,-0.00145245,0.296139,0.00385793,0.275977,0.000515567,0.215499,0.00877222,0.0762697,
	0.00515417,0.0713305,0.0174242,0.0131311,0.0139107,0.0299786,0.0314413,-0.00108649,0.0285847,
	0.0160047,0.0528645,-0.0038123,0.0514852,0.00977668,0.0844348,-0.00374007,0.0858203,0.00629685,
	0.13016,-0.0030303,0.136258,0.00407652,0.195757,-0.0022634,0.208182,0.00258338,0.284936,
	-0.00159547,0.294491,0.00157716,0.36341,-0.00106125,0.279512,0.000916122,0.146798,-0.000658787,
	-0.394784,0.000498958,-0.489341,-0.000373555,0.348931,0.000248325,-0.127479,-0.000185585,
	0.0379272,0.000106533,-0.0237213,-7.32024e-05,0.0117705,3.33286e-05,-0.00868635,-1.64701e-05,
	0.00628417,3.35584e-06,-0.00358763,5.32551e-20,0.00371156,0,-0.00149648,0,0.00212631,0,
	-0.000624718,0,0.00111299,0,-0.000265186,0,0.000503699,0,-0.000111279,0,0.000177702,0,
	-3.81224e-05,0,3.50545e-05,0,-4.42842e-06,0,-9.74828e-20
};

static const float kernel_44k[] = {
	4.66117e-20,0,3.45233e-06,0,-1.64513e-05,0,3.63544e-05,0,-7.29695e-05,0,0.000123652,0,
	-0.000185948,0,0.000305354,0,-0.000381071,0,0.000643175,0,-0.000699265,0,0.00122117,3.21669e-20,
	-0.00121242,2.00598e-06,0.00214407,-1.64421e-05,-0.00205774,7.43338e-06,0.00354202,-9.61861e-05,
	-0.00350988,-1.20612e-05,0.00561179,-0.000295535,-0.0061544,-8.31444e-05,0.00880743,
	-0.000662512,-0.0114503,-0.000189046,0.014796,-0.00117701,-0.0246397,-0.000193129,0.0344374,
	-0.00165184,-0.0875568,0.00026902,0.217219,-0.00161198,-0.538102,0.00190251,0.164209,
	-0.000187218,0.376605,0.00583496,0.168465,0.00394989,0.114466,0.0136488,0.034444,0.012598,
	0.0372353,0.0273908,0.0051969,0.028057,0.0164671,0.0496625,-0.000988379,0.0533119,0.00897001,
	0.083977,-0.00205689,0.0925857,0.00536803,0.135579,-0.00190808,0.152298,0.00327842,0.21224,
	-0.00149638,0.240289,0.00196246,0.318167,-0.00107479,0.343141,0.00112596,0.384381,-0.000714088,
	0.237232,0.000609849,-0.140558,-0.000433613,-0.6904,0.000306497,0.266157,-0.000233939,
	-0.0347948,0.000138164,-0.0169639,-0.000105521,0.00845345,5.10286e-05,-0.00911899,-3.36625e-05,
	0.00577395,1.1151e-05,-0.0035848,-3.36177e-06,0.00371187,-3.16496e-20,-0.00130792,0,0.00221494,
	0,-0.000455349,0,0.00117352,0,-0.000167716,0,0.000525745,0,-7.14066e-05,0,0.000180263,0,
	-2.79405e-05,0,3.40142e-05,0,-3.71749e-06,0,-7.76982e-20
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
	float tleft = left - right;
	float mid = left + right;
	float tright = right - left;
	float toleft = 0, toright = 0;
	filter->left[filter->pos] = tleft;
	filter->mid[(filter->pos + filter->delay) % filter->len] = mid;
	filter->right[filter->pos] = tright;
	if(!filter->bypass) {
		for(unsigned int i=0;i<filter->len;++i) {
			toleft += filter->left[(filter->pos + filter->len - i) % filter->len] * filter->filter[2*i];
			toleft += filter->left[(filter->pos + filter->len - i) % filter->len] * filter->filter[2*i+1];
			toright += filter->right[(filter->pos + filter->len - i) % filter->len] * filter->filter[2*i];
			toright += filter->right[(filter->pos + filter->len - i) % filter->len] * filter->filter[2*i+1];
		}
	} else {
		toleft = filter->left[(filter->pos + filter->len - filter->delay) % filter->len];
		toright = filter->right[(filter->pos + filter->len - filter->delay) % filter->len];
	}
	*oleft = filter->mid[filter->pos] + (toleft - toright) / 2;
	*oright = filter->mid[filter->pos] - (toleft + toright) / 2;
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
