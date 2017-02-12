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
#ifndef FILTER_DESIGNER_H
#define FILTER_DESIGNER_H
#include "dsp.h"
#include <cstring>
#include <iostream>

namespace fdesign {
	// Sample transfer function
	//
	// This samples fn at N+1 frequencies from 0Hz to samplerate/2.
	template <typename value_type, typename F>
	void transfer_function_sample(value_type *result, const F& fn, int N, int samplerate) {
		for(int i=0;i<=N;++i) {
			result[i] = fn(i*samplerate/(float)(2*N));
		}
	}

	// Compute filter's deviation from desired transfer function
	//
	// This returns the mean square difference between the transfer function and
	// the filter's measured frequency response. Filter must contain fft.N
	// elements, and the transfer function must contain fft.N/2+1 elements.
	template <typename value_type>
	value_type transfer_function_error(value_type *filter, const value_type *transfer_fn, fft_context<value_type> &fft) {
		value_type response[fft.N];
		value_type error = 0;
		value_type err;
		fft.fft(response, filter);
		fft.polar(response, response);
		for(int i=0;i<fft.N/2;++i) {
			err = transfer_fn[i] - response[2*i];
			error += err*err;
		}
		err = transfer_fn[fft.N/2] - response[1];
		error += err*err;
		return error / (fft.N/2+1);
	}

	template <typename value_type>
	bool dummy_progress_callback(value_type error, int N) {
		return false;
	}

	// Create filter minimizing error returned by compute_error
	//
	// Filter will be at N points long. If progress_callback is provided, it will
	// be called for every design iteration and given the computed error and
	// current filter length. progress_callback returning true causes filter_create
	// to return immediately with the current design.
	//
	// This returns a pair containing the filter's measured error and length.
	template <typename value_type, typename F, typename CB>
	std::pair<value_type, int> filter_create(value_type *filter, int N, const F& compute_error, const CB &progress_callback = dummy_progress_callback<value_type>) {
		std::pair<value_type, int> rv(0, 0);
		value_type weight[N];
		value_type slope[N];
		value_type &err = rv.first;
		int &_N = rv.second;
		const value_type delta = 0.00001;
		array_clear(filter, N);
		filter[0] = 1;
		err = compute_error(filter);
		for(_N=2;_N<N;++_N) {
			value_type t =(2*std::pow(10, std::numeric_limits<value_type>::epsilon()/20))/(8.69*N);
			for(int i=0;i<_N;++i) {
				weight[i] = std::exp(-i*t);
			}
			value_type mu = err;
			value_type new_err;
			while(true) {
				if(err < std::numeric_limits<value_type>::epsilon() || mu < std::numeric_limits<value_type>::epsilon())
					break;
				value_type new_filter[N];
				std::memcpy(new_filter, filter, sizeof(value_type)*N);
				for(int i=0;i<_N;++i) {
					new_filter[i] += delta;
					slope[i] = (compute_error(new_filter) - err) / delta;
					new_filter[i] = filter[i];
				}
				for(int i=0;i<_N;++i) {
					new_filter[i] -= slope[i] * weight[i] * mu;
				}
				new_err = compute_error(new_filter);
				if(new_err < err) {
					std::memcpy(filter, new_filter, sizeof(value_type)*N);
					err = new_err;
				} else {
					mu /= 2;
				}
				if(progress_callback(err, _N))
					return rv;
			}
		}
		return rv;
	}

	// Create filter minimizing mean square deviation from transfer_fn
	//
	// Filter will be at N points long. If progress_callback is provided, it will
	// be called for every design iteration and given the computed error and
	// current filter length. progress_callback returning true causes filter_create
	// to return immediately with the current design.
	//
	// Provided transfer_fn must contain N/2+1 elements spaced evenly from 0Hz to
	// the Nyquist frequency.
	//
	// This returns a pair containing the filter's measured error and length.
	template <typename value_type, typename CB>
	std::pair<value_type, int> filter_create(value_type *filter, const value_type *transfer_fn, int N, const CB &progress_callback = dummy_progress_callback<value_type>) {
		fft_context<value_type> fft(N);
		return filter_create(filter, fft.N, [&](value_type *filter) {
			return transfer_function_error(filter, transfer_fn, fft);
		}, progress_callback);
	}
}

#endif
