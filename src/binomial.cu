/*
 * Copyright (c) 2013-2016, Dalian Futures Information Technology Co., Ltd.
 *
 * Bo Wang
 * Xiaoye Meng <mengxiaoye at dce dot com dot cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cuda_runtime.h>
#include "impv.h"
#include "brent.h"

/* FIXME */
#define MAX_STEPS   4096
#define CACHE_SIZE  256
#define TIME_STEPS  16
#define CACHE_DELTA (2 * TIME_STEPS)
#define CACHE_STEP  (CACHE_SIZE - CACHE_DELTA)

/* FIXME */
static __device__ double d_pricebuf[MAX_STEPS + 16];
static __device__ double  d_callbuf[MAX_STEPS + 16];
static __device__ double   d_putbuf[MAX_STEPS + 16];

/* FIXME */
static __global__ void bi_amer_call(double spot, double strike, double vdt, double pu, double pd,
	int steps, double *d_callval) {
	__shared__ double pricea[CACHE_SIZE + 1];
	__shared__ double priceb[CACHE_SIZE + 1];
	__shared__ double  calla[CACHE_SIZE + 1];
	__shared__ double  callb[CACHE_SIZE + 1];
	const int tid = threadIdx.x;
	int i, base;

	for (i = tid; i <= steps; i += CACHE_SIZE) {
		d_pricebuf[i] = spot * exp(vdt * (2.0 * i - steps));
		d_callbuf[i]  = spot * exp(vdt * (2.0 * i - steps)) - strike;
		d_callbuf[i]  = d_callbuf[i] > 0.0 ? d_callbuf[i] : 0.0;
	}
	for (i = steps; i > 0; i -= CACHE_DELTA)
		for (base = 0; base < i; base += CACHE_STEP) {
			int start = min(CACHE_SIZE - 1, i - base);
			int end   = start - CACHE_DELTA;
			int k;

			__syncthreads();
			if (tid <= start) {
				pricea[tid] = d_pricebuf[base + tid];
				calla[tid]  = d_callbuf[base + tid];
			}
			for (k = start - 1; k >= end;) {
				double callval;

				__syncthreads();
				priceb[tid] = exp(-vdt) * pricea[tid + 1];
				callval = pu * calla[tid + 1] + pd * calla[tid];
				callb[tid] = callval > (priceb[tid] - strike)
					? callval : (priceb[tid] - strike);
				k--;
				__syncthreads();
				pricea[tid] = exp(-vdt) * priceb[tid + 1];
				callval = pu * callb[tid + 1] + pd * callb[tid];
				calla[tid] = callval > (pricea[tid] - strike)
					? callval : (pricea[tid] - strike);
				k--;
			}
			__syncthreads();
			if (tid <= end) {
				d_pricebuf[base + tid] = pricea[tid];
				d_callbuf[base + tid]  = calla[tid];
			}
		}
	if (threadIdx.x == 0)
		*d_callval = calla[0];
}

/* FIXME */
static __global__ void bi_amer_put(double spot, double strike, double vdt, double pu, double pd,
	int steps, double *d_putval) {
	__shared__ double pricea[CACHE_SIZE + 1];
	__shared__ double priceb[CACHE_SIZE + 1];
	__shared__ double   puta[CACHE_SIZE + 1];
	__shared__ double   putb[CACHE_SIZE + 1];
	const int tid = threadIdx.x;
	int i, base;

	for (i = tid; i <= steps; i += CACHE_SIZE) {
		d_pricebuf[i] = spot * exp(vdt * (2.0 * i - steps));
		d_putbuf[i]   = strike - spot * exp(vdt * (2.0 * i - steps));
		d_putbuf[i]   = d_putbuf[i] > 0.0 ? d_putbuf[i] : 0.0;
	}
	for (i = steps; i > 0; i -= CACHE_DELTA)
		for (base = 0; base < i; base += CACHE_STEP) {
			int start = min(CACHE_SIZE - 1, i - base);
			int end   = start - CACHE_DELTA;
			int k;

			__syncthreads();
			if (tid <= start) {
				pricea[tid] = d_pricebuf[base + tid];
				puta[tid]   = d_putbuf[base + tid];
			}
			for (k = start - 1; k >= end;) {
				double putval;

				__syncthreads();
				priceb[tid] = exp(-vdt) * pricea[tid + 1];
				putval = pu * puta[tid + 1] + pd * puta[tid];
				putb[tid] = putval > (strike - priceb[tid])
					? putval : (strike - priceb[tid]);
				k--;
				__syncthreads();
				pricea[tid] = exp(-vdt) * priceb[tid + 1];
				putval = pu * putb[tid + 1] + pd * putb[tid];
				puta[tid] = putval > (strike - pricea[tid])
					? putval : (strike - pricea[tid]);
				k--;
			}
			__syncthreads();
			if (tid <= end) {
				d_pricebuf[base + tid] = pricea[tid];
				d_putbuf[base + tid]   = puta[tid];
			}
		}
	if (threadIdx.x == 0)
		*d_putval = puta[0];
}

/* FIXME */
double bi_cuda_amer_call(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	double vdt = vol * sqrt(dt);
	/* up movement */
	double up = exp(vdt);
	/* down movement */
	double dn = 1.0 / up;
	double p_up = (exp((r - d) * dt) - dn) / (up - dn);
	double p_dn = 1.0 - p_up;
	double *d_callval, res = 0.0;

	cudaMalloc((void **)&d_callval, sizeof (double));
	bi_amer_call<<<1, CACHE_SIZE>>>(spot, strike, vdt, Rinv * p_up, Rinv * p_dn, steps, d_callval);
	cudaMemcpy(&res, d_callval, sizeof (double), cudaMemcpyDeviceToHost);
	cudaFree(d_callval);
	return res;
}

/* FIXME */
double bi_cuda_amer_put(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	double vdt = vol * sqrt(dt);
	/* up movement */
	double up = exp(vdt);
	/* down movement */
	double dn = 1.0 / up;
	double p_up = (exp((r - d) * dt) - dn) / (up - dn);
	double p_dn = 1.0 - p_up;
	double *d_putval, res = 0.0;

	cudaMalloc((void **)&d_putval, sizeof (double));
	bi_amer_put<<<1, CACHE_SIZE>>>(spot, strike, vdt, Rinv * p_up, Rinv * p_dn, steps, d_putval);
	cudaMemcpy(&res, d_putval, sizeof (double), cudaMemcpyDeviceToHost);
	cudaFree(d_putval);
	return res;
}

/* FIXME */
double impv_bi_cuda(double spot, double strike, double r, double d, double expiry, int steps,
	double price, int type) {
	double low = 0.000001, high = 0.3, ce;

	/* FIXME */
	if (type != AMER_CALL && type != AMER_PUT)
		return NAN;
	ce = type == AMER_CALL ? bi_cuda_amer_call(spot, strike, r, d, high, expiry, steps) :
		bi_cuda_amer_put(spot, strike, r, d, high, expiry, steps);
	while (ce < price) {
		high *= 2.0;
		if (high > 1e10)
			return NAN;
		ce = type == AMER_CALL ? bi_cuda_amer_call(spot, strike, r, d, high, expiry, steps) :
			bi_cuda_amer_put(spot, strike, r, d, high, expiry, steps);
	}
	return type == AMER_CALL
		? brent(low, high, price, NULL, bi_cuda_amer_call, NULL, spot, strike, r, d, expiry, 0, steps)
		: brent(low, high, price, NULL, bi_cuda_amer_put,  NULL, spot, strike, r, d, expiry, 0, steps);
}

