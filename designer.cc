/*
 * Copyright (c) 2017 Jeremy Pepper
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
#include "filter_designer.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <unistd.h>
using namespace std;
using namespace fdesign;

int SR = -1;
int FDELAY;
int TDELAY;
#define OVERSAMPLE 5

typedef float fp;

static void compute_ideal_gabor_response(fp *response, const fp *crossfeed, int crossfeed_delay, gabor_context<fp> &dgt) {
	fp signal[dgt.N];
	fp samechan[dgt.N*((crossfeed_delay*2+2))/dgt.window_spacing];
	fp crosschan[dgt.N*((crossfeed_delay*2+2))/dgt.window_spacing];
	array_clear(signal, dgt.N);
	signal[0] = 1;
	dgt.dgt(samechan, signal, (crossfeed_delay*2+2));
	for(int i=0;i<FDELAY;++i) {
		signal[i+1] = crossfeed[FDELAY-i-1];
	}
	signal[0] = 0;
	dgt.dgt(crosschan, signal, (crossfeed_delay*2+2));
	for(int i=0;i<dgt.N*((crossfeed_delay*2+2))/dgt.window_spacing;++i) {
		response[i] = samechan[i] + crosschan[i];
	}
}

static void usage(int argc, char *argv[]) {
	fprintf(stderr, "Usage: %s -s <samplerate>\n", argc ? argv[0] : "designer");
	exit(-1);
}

static void parseopts(int argc, char *argv[]) {
	int ch;
	while((ch = getopt(argc, argv, "s:")) != -1) {
		switch(ch) {
		case 's':
			SR = atoi(optarg);
			FDELAY = round((40.*SR)/96000.);
			TDELAY = round((22.*SR)/96000.);
			break;
		case '?':
		default:
			usage(argc, argv);
		}
	}
	if(SR < 0)
		usage(argc, argv);
}

int main(int argc, char *argv[]) {
	ios_base::sync_with_stdio(false);
	fp transfer_fn[257];
	fp filter[512];
	fp last_error = 1;
	parseopts(argc, argv);
	int last_N = 0;
	// 250: 1
	// 1000: 4
	// 5000: 6
	// 10000: 11
	transfer_function_sample(transfer_fn, [](fp x) {
		//return pow(10, (x <= 250 ? 1 : 1 + 1.5*log2(x/250) + (x > 5000 ? 5*log2(x/5000) : 0)) / -20);
		//fp db = ((x-1000.)*(x-5000.)*(x-10000.)*1)/((250.-1000.)*(250.-5000.)*(250.-10000.)) + ((x-250.)*(x-5000.)*(x-10000.)*4)/((1000.-250.)*(1000.-5000.)*(1000.-10000.)) + ((x-250.)*(x-1000.)*(x-10000.)*6)/((5000.-250.)*(5000.-1000.)*(5000.-10000.)) + ((x-250.)*(x-1000.)*(x-5000.)*11)/((10000.-250.)*(10000.-1000.)*(10000.-5000.));
		//return pow(10, db / -20);
		//fp val = exp(-(x+260)/200)-.00004388*x+0.7;
		//return val > 1 ? 1 : val < 0.1 ? 0.1 : val;
		//fp val = -4/(1+exp(-x/250))-x*0.0006;
		fp val = -10/(1+exp(-x/250))+6-x*0.0006;
		return pow(10, val / 20);
	}, 256, SR);
	auto res = filter_create(filter, transfer_fn, 512, [&](fp error, int N) {
		if(N > last_N || error / last_error < 0.99) {
			cout << "N " << N << ", error: " << error << "           \r" << flush;
			last_N = N;
			last_error = error;
		}
		return error < 0.001*0.001 || N > FDELAY;
	});
	cout << endl;
	fp fixer[512] = {1, 0};
#if 0
	fp target[(512*2*(FDELAY+1))/OVERSAMPLE];
	gabor_context<fp> dgt(512, FDELAY+1, OVERSAMPLE);
	compute_ideal_gabor_response(target, filter, FDELAY, dgt);
	last_N = 0;
	last_error = 1;
	auto res2 = filter_create(fixer, FDELAY, [&](const fp *fixer) {
		fp combined[512] = {0};
		fp resp[(512*2*(FDELAY+1))/OVERSAMPLE];
		fp error = 0;
		for(int i=0;i<2*FDELAY+1;++i) {
			combined[i] = fixer[i];
		}
		for(int i=0;i<FDELAY;++i) {
			combined[i+1] += filter[FDELAY-i-1];
		}
		dgt.dgt(resp, combined, 2*(FDELAY+1));
		for(int i=0;i<(512*2*(FDELAY+1))/OVERSAMPLE;++i) {
			fp err = resp[i] - target[i];
			error += err*err;
		}
		return error / (512*2*(FDELAY+1));
	}, [&](fp error, int N) {
		if(N > last_N || error / last_error < 0.99) {
			cout << "N " << N << ", error: " << error << "           \r" << flush;
			last_N = N;
			last_error = error;
		}
		return false;
		//return error < 0.01*0.01;
	});
#endif
	cout << endl;
	cout << setprecision(numeric_limits<float>::digits10+2);
	filter[FDELAY] = 0;
	for(int i=0;i<TDELAY;++i) {
		int idx = i+FDELAY-TDELAY;
		cout << fixer[i] - (idx > 0 ? filter[FDELAY-idx-1] : 0) << ' ';
	}
	/*
	for(int i=0;i<2*FDELAY+1;++i) {
		cout << fixer[i] - (i > FDELAY ? filter[i-FDELAY-1] : 0) << ' ';
	}*/
	cout << endl;
}
