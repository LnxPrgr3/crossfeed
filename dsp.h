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
#ifndef DSP_H
#define DSP_H
#include <cmath>
#include <memory>
#include <Accelerate/Accelerate.h>

namespace fdesign {
	template <typename value_type>
	class fft_context {
	private:
		void *context;
	public:
		const int N, logN;
		fft_context(int N);
		~fft_context();
		void fft(value_type *result, const value_type *source);
		void polar(value_type *magnitudes, value_type *source);
	};

	template <>
	fft_context<float>::fft_context(int N) : N(N), logN((int)std::round(std::log2(N))) {
		context = vDSP_create_fftsetup(logN, FFT_RADIX2);
	}

	template <>
	fft_context<float>::~fft_context() {
		vDSP_destroy_fftsetup((FFTSetup)context);
	}

	template <>
	void fft_context<float>::fft(float *result, const float *source) {
		float response_memory[N];
		float scale = 0.5;
		DSPSplitComplex response = {response_memory, response_memory + N/2};
		vDSP_ctoz((DSPComplex *)source, 2, &response, 1, N/2);
		vDSP_fft_zrip((FFTSetup)context, &response, 1, logN, FFT_FORWARD);
		vDSP_ztoc(&response, 1, (DSPComplex *)result, 2, N/2);
		vDSP_vsmul(result, 1, &scale, result, 1, N);
	}

	template <>
	void fft_context<float>::polar(float *magnitudes, float *source) {
		vDSP_polar(magnitudes, 2, source, 2, N/2);
	}

	template <>
	fft_context<double>::fft_context(int N) : N(N), logN((int)std::round(std::log2(N))) {
		context = vDSP_create_fftsetupD(logN, FFT_RADIX2);
	}

	template <>
	fft_context<double>::~fft_context() {
		vDSP_destroy_fftsetupD((FFTSetupD)context);
	}

	template <>
	void fft_context<double>::fft(double *result, const double *source) {
		double response_memory[N];
		double scale = 0.5;
		DSPDoubleSplitComplex response = {response_memory, response_memory + N/2};
		vDSP_ctozD((DSPDoubleComplex *)source, 2, &response, 1, N/2);
		vDSP_fft_zripD((FFTSetupD)context, &response, 1, logN, FFT_FORWARD);
		vDSP_ztocD(&response, 1, (DSPDoubleComplex *)result, 2, N/2);
		vDSP_vsmulD(result, 1, &scale, result, 1, N);
	}

	template <>
	void fft_context<double>::polar(double *magnitudes, double *source) {
		vDSP_polarD(magnitudes, 2, source, 2, N/2);
	}

	template <typename value_type>
	void array_clear(value_type *array, int N);

	template <>
	void array_clear<float>(float *array, int N) {
		vDSP_vclr(array, 1, N);
	}

	template <>
	void array_clear<double>(double *array, int N) {
		vDSP_vclrD(array, 1, N);
	}

	template <typename value_type>
	void filter_apply(value_type *result, const value_type *signal, int signal_len, const value_type *filter, int filter_len) {
		int result_len = signal_len+filter_len-1;
		array_clear(result, result_len);
		for(int i=0;i<signal_len;++i) {
			for(int j=0;j<filter_len;++j) {
				result[i+j] += signal[i]*filter[j];
			}
		}
	}

	template <typename value_type>
	class gabor_context : public fft_context<value_type> {
	private:
		std::unique_ptr<value_type[]> window;
	public:
		const int time_period;
		const int window_len;
		const int window_spacing;
		gabor_context(int N, int time_period, int oversample) : fft_context<value_type>(N), time_period(time_period), window_len(6*time_period+1), window_spacing(time_period / oversample) {
			window.reset(new value_type[window_len]);
			array_clear(window.get(), window_len);
			int T = time_period/2;
			T *= T;
			value_type total = 0;
			for(int t=0;t<window_len;++t) {
				int n = t - window_len/2;
				window[t] = /* (1/sqrt(2*M_PI*T))* */exp((-(n*n))/(2.*T));
			}
		}
		void dgt(value_type *result, const value_type *source, int source_len) {
			value_type windowed[this->N];
			for(int t=0;t*window_spacing < source_len;++t) {
				for(int i=0;i<this->N;++i) {
					int wpos = (window_len/2)-((t*window_spacing)+i);
					value_type in = i+t*window_spacing < source_len ? source[i+t*window_spacing] : 0;
					value_type win = wpos > 0 && wpos < window_len ? window[wpos] : 0;
					windowed[i] = in*win;
				}
				this->fft(&result[t*this->N], windowed);
			}
		}
	};
}

#endif
