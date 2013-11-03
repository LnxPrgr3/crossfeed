#include "crossfeed.h"
#include <string.h>

static const float kernel[] = {
	-6.5949324e-17, 0.0045014457, 0.022524569, -0.025003152, -0.020022731, 0.040223219,
	-0.013063066, 0.0086236969, -0.0093082022, -0.024329932, 0.046240062, 0.011692866,
	-0.057115946, 0.011906438, 0.026917025, -0.013178191, 0.0023361482, -0.025915308,
	-0.010047952, 0.073893473, -0.065088294, -0.081742078, 0.14480254, -0.055060681, 0.89285111,
	-0.078405745, 0.22987437, -0.091824301, -0.13483715, 0.14268893, -0.034489024, 0.030484864,
	0.004442533, -0.060682625, 0.042746112, 0.12932804, -0.0093505234, 0.11326276, 0.015986128,
	-0.03860902, 0.021401245, 0.028922923, -0.056020558, 0.18060984, -0.16011734, -0.064057432,
	0.36784136, -0.47685066, -0.1238166, -0.72417462, 0.41136181, -0.069534779, -0.26199466,
	0.38073397, -0.1991304, -0.0089984555, 0.039526168, -0.022302471, -0.071558997, 0.19194767,
	-0.31636775, 0.037015077, -0.073966466, 0.00022724067, 0.0020957794, 0.049722925, -0.12743083,
	0.069876105, -0.028591951, -0.001548495, 0.0077972054, -0.022507614, -0.020718653,
	-0.0012374669
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
	for(unsigned int i=0;i<74;++i) {
		oside += filter->side[(filter->pos + 74 - i) % 74] * kernel[i];
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
