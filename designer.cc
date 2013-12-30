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

static float transfer_function(float x) {
	return pow(10, (x <= 1500 ? 2 : 2 * log2(x/750)) / -20);
}

static void compute_response(float *result_interleaved, const vector<float> &xs) {
	float response[1024] = {0};
	float result_memory[1024];
	DSPSplitComplex result = {result_memory, result_memory + 512};
	for(int i=0;i<xs.size()/4;++i) {
		float a = xs[2*i] * (1 - xs[2*i+1]);
		float b = xs[2*i+1];
		response[0] += a;
		for(int i=1;i<1024;++i) {
			response[i] = b*response[i-1];
		}
	}
	vDSP_ctoz((DSPComplex *)response, 2, &result, 1, 512);
	vDSP_fft_zrip(fft_context, &result, 1, 10, FFT_FORWARD);
	vDSP_ztoc(&result, 1, (DSPComplex *)result_interleaved, 2, 512);
	vDSP_polar(result_interleaved, 2, result_interleaved, 2, 512);
	memset(response, 0, sizeof(response));
	for(int i=0;i<xs.size()/4;++i) {
		float higha0 = (xs[xs.size()/2+2*i]) * ((1+xs[xs.size()/2+2*i+1])/2);
		float higha1 = -higha0;
		float b = xs[xs.size()/2+2*i+1];
		response[0] += higha0;
		response[1] += higha1 + higha0 * b;
		for(int i=2;i<1024;++i) {
			response[i] = b*response[i-1];
		}
	}
	vDSP_ctoz((DSPComplex *)response, 2, &result, 1, 512);
	vDSP_fft_zrip(fft_context, &result, 1, 10, FFT_FORWARD);
	vDSP_ztoc(&result, 1, (DSPComplex *)(result_interleaved+1024), 2, 512);
	vDSP_polar(result_interleaved+1024, 2, result_interleaved+1024, 2, 512);
}

static float compute_error(const vector<float> &xs) {
	float result_interleaved[2048];
	compute_response(result_interleaved, xs);
	float error = 0;
	for(int i=0;i<512;++i) {
		float freq = round(i * 44100) / 1024.;
		float mono_resp = (1 + 0.5*result_interleaved[2*i]) * 0.5*result_interleaved[2*i+1024];
		float xfeed_resp = 0.5*result_interleaved[2*i];
		float mono_err = mono_resp - 1;
		float xfeed_err = (xfeed_resp - transfer_function(freq)) / transfer_function(freq);
		error += (mono_err*mono_err) + (xfeed_err*xfeed_err);
	}
	return error / 1024.;
}

int main(int argc, char *argv[]) {
	fft_context = vDSP_create_fftsetup(10, FFT_RADIX2);
	vector<float> xs = {};
	float err = compute_error(xs);
	while(err > 0.1 && xs.size() < 4) {
		xs.push_back(1);
		xs.push_back(0);
		xs.push_back(1);
		xs.push_back(0);
		vector<float> slope(xs.size());
		float mu = 0.2;
		const float delta = 0.00001;
		err = compute_error(xs);
		while(true) {
			if(err < 0.005 || mu < 1. / (1 << 24))
				break;
			vector<float> new_xs(xs);
			for(int i=0;i<new_xs.size();++i) {
				new_xs[i] += delta;
				//new_xs[i] = i % 2 ? (new_xs[i] < 0 ? 0 : (new_xs[i] > 1 ? 1 : new_xs[i])) : new_xs[i];
				slope[i] = (compute_error(new_xs) - err) / delta;
				new_xs[i] = xs[i];
			}
			for(int i=0;i<new_xs.size();++i)  {
				new_xs[i] -= slope[i] * mu;
				//new_xs[i] = i % 2 ? (new_xs[i] < 0 ? 0 : (new_xs[i] > 1 ? 1 : new_xs[i])) : new_xs[i];
			}
			float new_err = compute_error(new_xs);
			if(new_err < err) {
				xs = move(new_xs);
				err = new_err;
			} else {
				mu /= 2;
			}
		}
		cerr << "Size: " << xs.size()/4 << ", error: " << err << endl;
	}
	float result_interleaved[2048];
	compute_response(result_interleaved, xs);
	for(int i=0;i<512;++i) {
		float freq = round(i * 44100) / 1024.;
		cout << freq << '\t' << 20*log10(transfer_function(freq)) << '\t' << freq << '\t' << 20*log10(0.5*result_interleaved[2*i]) << '\t' << freq << '\t' << 20*log10((1 + 0.5*result_interleaved[2*i])*0.5*result_interleaved[2*i+1024]) << '\n';
	}
	for(float x: xs) {
		cerr << x << endl;
	}
	vDSP_destroy_fftsetup(fft_context);
}
