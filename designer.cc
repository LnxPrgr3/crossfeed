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

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <Accelerate/Accelerate.h>
using namespace std;

static FFTSetup fft_context;

struct magic {
	int samplerate;
	int delay;
	int len;
	int offset;
	int limit;
};

static float transfer_function(float x) {
	return sqrt(pow(10, (x <= 1500 ? 2 : 2 * log2(x/750)) / -20));
}

static double window_fn(int i, int N) {
	return 0.42 - 0.5 * cos((2*M_PI*i)/(N-1)) + 0.08 * cos((4*M_PI*i)/(N-1));
}

static string fn(const char *prefix, int samplerate, const char *postfix) {
	std::stringstream res;
	res << prefix << (samplerate/1000) << postfix;
	return res.str();
}

int main(int argc, char *argv[]) {
	const vector<int> rates = {96000, 48000, 44100};
	ios_base::sync_with_stdio(false);
	fft_context = vDSP_create_fftsetup(12, FFT_RADIX2);
	for(const auto rate: rates) {
		cout << rate << "Hz kernel:" << endl;
		int piece_len = round(1.88571428571428571428 * (rate/1500.) + 1);
		piece_len += 1 - piece_len % 2;
		int fft_len_log2 = ceil(log2(8*piece_len));
		int fft_len = 1 << fft_len_log2;
		int delay = (250 * rate) / 1000000.;
		float scalar = fft_len;
		float response[fft_len];
		float result_memory[fft_len];
		DSPSplitComplex result = {result_memory, result_memory + fft_len/2};
		float filter[piece_len+delay];
		float samechan[piece_len+delay];
		for(int i=0;i<fft_len/2;++i) {
			float freq = (i * rate) / (float)fft_len;
			result.realp[i] = transfer_function(freq);
			result.imagp[i] = 0;
		}
		ofstream dfile(fn("delay-", rate, "k.txt"));
		for(int i=1;i<fft_len/2;++i) {
			float freq = (i * rate) / (float)fft_len;
			float delay = (result.realp[i] - sqrt(pow(10, -2./20))) * 250;
			float phase = 2*M_PI*((delay / 1000000.) / (1. / freq));
			result.imagp[i] = result.realp[i]*sin(phase);
			result.realp[i] = result.realp[i]*cos(phase);
			dfile << freq << '\t' << phase << '\n';
		}
		{
			float freq = rate/2;
			float delay = (transfer_function(freq) - sqrt(pow(10, -2./20))) * 250;
			float phase = 2*M_PI*((delay / 1000000.) / (1. / freq));
			result.imagp[0] = transfer_function(rate/2)*cos(phase);
		}
		vDSP_fft_zrip(fft_context, &result, 1, fft_len_log2, FFT_INVERSE);
		vDSP_ztoc(&result, 1, (DSPComplex *)response, 2, fft_len/2);
		vDSP_vsdiv(response, 1, &scalar, response, 1, fft_len);
		memset(filter, 0, sizeof(float)*(piece_len+delay));
		for(int i=0;i<piece_len;++i) {
			filter[i+delay] = response[(fft_len - (piece_len / 2) + i) % fft_len] * window_fn(i, piece_len);
		}
		memset(samechan, 0, sizeof(float)*(piece_len+delay));
		for(int i=0;i<piece_len;++i) {
			samechan[i] = -filter[piece_len+delay-i-1];
		}
		samechan[piece_len/2] += 1;
		ofstream rfile(fn("filter-", rate, "k.txt"));
		for(int i=0;i<piece_len+delay;++i) {
			rfile << samechan[i] << '\n' << filter[i] << '\n';
		}
		cout << "\tComponent len: " << piece_len << endl;
		cout << "\tFFT len: " << fft_len << "(2^" << fft_len_log2 << ")" << endl;
		cout << "\tCrossfeed delay: " << delay << endl;
	}
	vDSP_destroy_fftsetup(fft_context);
}
