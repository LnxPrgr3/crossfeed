#include "crossfeed.h"
#include <string.h>

static const float kernel[] = {
	-2.4521847e-17, -0.0010487922, 0.031420387, -0.023124512, -0.021316921, 0.035805073,
	-0.028186101, 0.0043962691, 0.010263441, -0.032269828, 0.034994692, 0.048420284, 0.010202188,
	0.035241261, 0.0010797267, -0.040379547, 0.035441346, 0.0035936108, -0.013609189, 0.054569002,
	-0.031671565, -0.042685658, 0.136527, -0.23503315, 0.67110574, -0.24441561, 0.222657,
	-0.048926685, -0.09018442, 0.091514431, -0.06073438, 0.0055678668, 0.031606793,-0.10052362,
	0.040101938, 0.17329268, 0.13219273, 0.14953232, -0.02228196, -0.12755704, 0.058534652,
	0.0059564393, -0.080707997, 0.10680597, -0.16086607, -0.066193663, 0.46951896, -0.53276598,
	-0.11736935, -0.77559763, 0.34767476, -0.19575806, -0.22076076, 0.37094438, -0.16833755,
	-0.076737724, 0.069217324, -0.009715274, 0.079999663, 0.15501672, -0.38176361, -0.10035028,
	-0.12699345, -0.085814379, 0.05361025, 0.061291013, -0.074593984, 0.10920279,-0.014919096,
	0.0030028627, 0.046673447, -0.023521114, -0.021248439, -0.00096126326
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
