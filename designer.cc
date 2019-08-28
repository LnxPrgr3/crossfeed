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
	for(int i=0;i<crossfeed_delay;++i) {
		signal[i+FDELAY] = crossfeed[i];
	}
	for(int i = 0; i < FDELAY; ++i)
		signal[i] = 0;
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

static fp square(fp x) {
	return x*x;
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
		fp val = 4*(exp(-x/3000+1)-3)+3*exp(-square(x-4600)/14000000);
		return pow(10, val / 20);
	}, 256, SR);
	auto res = filter_create(filter, transfer_fn, 512, [&](fp error, int N) {
		if(N > last_N || error / last_error < 0.99) {
			cout << "N " << N << ", error: " << error << "           \r" << flush;
			last_N = N;
			last_error = error;
		}
		return error < 0.001*0.001;
	});
	cout << endl;
	fp fixer[512] = {1, 0};
	cout << endl;
	cout << setprecision(numeric_limits<float>::digits10+2);
	filter[FDELAY] = 0;
	for(int i=0;i<res.second+FDELAY;++i) {
		fp impulse = i == 0 ? 1 : 0;
		cout << (impulse - (i - FDELAY < 0 ? 0 : filter[i-FDELAY])) << ' ';
	}
	cout << endl;
}
