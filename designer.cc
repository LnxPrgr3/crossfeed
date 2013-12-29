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

static void compute_response(float *result_interleaved, float x) {
	float a = transfer_function(0) * (1 - x);
	float b = x;
	float response[1024] = {0};
	float result_memory[1024];
	DSPSplitComplex result = {result_memory, result_memory + 512};
	response[0] = a;
	for(int i=1;i<1024;++i) {
		response[i] = b*response[i-1];
	}
	vDSP_ctoz((DSPComplex *)response, 2, &result, 1, 512);
	vDSP_fft_zrip(fft_context, &result, 1, 10, FFT_FORWARD);
	vDSP_ztoc(&result, 1, (DSPComplex *)result_interleaved, 2, 512);
	vDSP_polar(result_interleaved, 2, result_interleaved, 2, 512);
	float higha0 = (1/transfer_function(0)) * ((1+x)/2);
	float higha1 = -higha0;
	response[0] = higha0;
	response[1] = higha1 + higha0 * b;
	for(int i=2;i<1024;++i) {
		response[i] = b*response[i-1];
	}
	vDSP_ctoz((DSPComplex *)response, 2, &result, 1, 512);
	vDSP_fft_zrip(fft_context, &result, 1, 10, FFT_FORWARD);
	vDSP_ztoc(&result, 1, (DSPComplex *)(result_interleaved+1024), 2, 512);
	vDSP_polar(result_interleaved+1024, 2, result_interleaved+1024, 2, 512);
}

static float compute_error(float x) {
	float result_interleaved[2048];
	compute_response(result_interleaved, x);
	float error = 0;
	for(int i=0;i<512;++i) {
		float freq = round(i * 44100) / 1024.;
		float mono_resp = 0.5*result_interleaved[2*i] + 0.5*result_interleaved[2*i+1024];
		float xfeed_resp = 0.5*result_interleaved[2*i+1024] / 0.5*result_interleaved[2*i];
		float mono_err = mono_resp - 1;
		float xfeed_err = (0.5*result_interleaved[2*i] - transfer_function(freq)) / transfer_function(freq);
		error += (mono_err*mono_err) + (xfeed_err*xfeed_err);
	}
	return error / 1024.;
}

static float find_lowest_error(float start, float end) {
	if((end - start) < 1/(float)(1 << 24))
		return start;
	float start_error = compute_error(start);
	float end_error = compute_error(end);
	return start_error < end_error ? find_lowest_error(start, (end-start)/2+start) : find_lowest_error((end-start)/2+start, end);
}

int main(int argc, char *argv[]) {
	fft_context = vDSP_create_fftsetup(12, FFT_RADIX2);
	float b = 0, err = compute_error(0);
	for(float i=0;i<1;i += 1/(float)(1 << 16)) {
		float new_err = compute_error(i);
		if(new_err < err) {
			err = new_err;
			b = i;
		}
		cerr << i*100 << "%, err = " << err << "        \r" << flush;
	}
	b = find_lowest_error(b-1/(float)(1 << 16), b+1/(float)(1 << 16));
	//float b = find_lowest_error(0, 1);
	float result_interleaved[2048];
	compute_response(result_interleaved, b);
	for(int i=0;i<512;++i) {
		float freq = round(i * 44100) / 1024.;
		cout << freq << '\t' << 20*log10(transfer_function(freq)) << '\t' << freq << '\t' << 20*log10(0.5*result_interleaved[2*i]) << '\t' << freq << '\t' << 20*log10(0.5*result_interleaved[2*i+1024]) << '\n';
	}
	cerr << "a = " << transfer_function(0) * (1 - b) << ", b = " << b << endl;
	vDSP_destroy_fftsetup(fft_context);
}
