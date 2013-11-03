#include "crossfeed.h"
#include <string.h>

static const float kernel[] = {
	3.810114e-17, 0.00057074119, 0.0036383825, 0.00802249, 0.0030424956, -0.0056136339,
	-0.012746178, -0.015629187, -0.021201266, -0.011451564, -0.0024262015, 0.0023740481,
	0.001601528, 0.0048879948, 0.0012624422, 0.0095108747, 0.015015749, 0.018161977, 0.0010558863,
	-0.016349094, 0.006770506, 0.06602791, -0.0091732806, -0.17842372, 0.79500389, -0.055401541,
	0.020726213, 0.024730124, 0.013692942, -0.015625473, -0.071733445, -0.075079001, -0.076685749,
	-0.020419678, 0.0024186713, -0.031852022, -0.053926453, 0.008134813, -0.011911276,
	-0.0077756061, -0.053873725, -0.13927366, -0.10397572, 0.056234889, -0.053873774,
	0.0023132914, 0.17274453, -0.16519427, 0.30882645, -0.34567493, 0.11466106, 0.053277675,
	-0.050380893, 0.095941506, -0.039547332, 0.03295086, 0.10108547, 0.077773921, -0.016714504,
	0.086656712, -0.057295103, 0.056581859, -0.038665563, -0.0844042, -0.11988226, -0.019883214,
	-0.0076610777, 0.045880675, -0.030639699, -0.02792817, 0.0085057095, 0.017872378,
	0.0055849315, 0.0010143743
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
