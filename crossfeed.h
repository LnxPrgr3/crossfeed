#ifndef CROSSFEED_H
#define CROSSFEED_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct crossfeed_s {
	float mid[74];
	float side[74];
	unsigned char pos;
	unsigned char bypass;
} crossfeed_t;

void crossfeed_init(crossfeed_t *filter);
void crossfeed_filter(crossfeed_t *filter, float *input, float *output, unsigned int size);
void crossfeed_filter_inplace_noninterleaved(crossfeed_t *filter, float *left, float *right, unsigned int size);

#ifdef __cplusplus
}
#endif

#endif
