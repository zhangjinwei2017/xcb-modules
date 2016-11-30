/*
 * Copyright (c) 2013-2016, Dalian Futures Information Technology Co., Ltd.
 *
 * Gaohang Wu  <wugaohang at dce dot com dot cn>
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

#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_vector.h>
#include <math.h>
#include <time.h>
#include <xcb/macros.h>
#include "brent.h"
#include "lsm.h"

/* FIXME */
static gsl_matrix *randn(int m, int n, int antithetic, int moment_matching) {
	const gsl_rng_type *T = gsl_rng_default;
	gsl_rng *r;
	int i, j, k = 0;
	gsl_matrix *a = gsl_matrix_alloc(m, n);
	double data[m * n];

	gsl_rng_default_seed = time(NULL);
	r = gsl_rng_alloc(T);
	if (antithetic) {
		int m2 = m % 2 ? m / 2 + 1 : m / 2;

		for (i = 0; i < m2; ++i)
			for (j = 0; j < n; ++j) {
				gsl_matrix_set(a, i, j, gsl_ran_gaussian(r, 1.0));
				data[k++] = gsl_matrix_get(a, i, j);
				if (i + m2 < m) {
					gsl_matrix_set(a, i + m2, j, -gsl_matrix_get(a, i, j));
					data[k++] = gsl_matrix_get(a, i + m2, j);
				}
			}
	} else
		for (i = 0; i < m; ++i)
			for (j = 0; j < n; ++j) {
				gsl_matrix_set(a, i, j, gsl_ran_gaussian(r, 1.0));
				data[k++] = gsl_matrix_get(a, i, j);
			}
	if (moment_matching) {
		double mean = gsl_stats_mean(data, 1, k);
		double var  = gsl_stats_variance(data, 1, k);

		for (i = 0; i < m; ++i)
			for (j = 0; j < n; ++j)
				gsl_matrix_set(a, i, j, (gsl_matrix_get(a, i, j) - mean) / var);
	}
	gsl_rng_free(r);
	return a;
}

/* FIXME: adapted from MathWorks, pinv.m */
static gsl_matrix *pinv(gsl_matrix *R) {
	gsl_matrix *A, *V, *X;
	gsl_vector *S, *work;
	int i, j;

	if (R->size1 < R->size2) {
		A = gsl_matrix_alloc(R->size2, R->size1);
		gsl_matrix_transpose_memcpy(A, R);
	} else {
		A = gsl_matrix_alloc(R->size1, R->size2);
		gsl_matrix_memcpy(A, R);
	}
	V    = gsl_matrix_alloc(A->size2, A->size2);
	S    = gsl_vector_alloc(A->size2);
	work = gsl_vector_alloc(A->size2);
	X    = gsl_matrix_alloc(A->size2, A->size1);
	gsl_linalg_SV_decomp(A, V, S, work);
	for (i = 0; i < A->size2; ++i)
		for (j = 0; j < A->size1; ++j) {
			int k;
			double sum = 0.0;

			for (k = 0; k < A->size2; ++k)
				sum += gsl_matrix_get(V, i, k) / gsl_vector_get(S, k) *
					gsl_matrix_get(A, j, k);
			gsl_matrix_set(X, i, j, sum);
		}
	gsl_vector_free(work);
	gsl_vector_free(S);
	gsl_matrix_free(V);
	if (R->size1 < R->size2) {
		gsl_matrix_transpose_memcpy(A, X);
		gsl_matrix_free(X);
		return A;
	} else {
		gsl_matrix_free(A);
		return X;
	}
}

/* FIXME */
double lsm_amer_call(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps) {
	double dt = expiry / tsteps, res = 0.0;
	gsl_matrix *a = randn(ssteps, tsteps, 1, 1);
	gsl_matrix *s = gsl_matrix_alloc(ssteps, tsteps + 1);
	gsl_matrix *c = gsl_matrix_alloc(ssteps, tsteps + 1);
	int i, j;

	for (i = 0; i < ssteps; ++i) {
		gsl_matrix_set(s, i, 0, spot);
		gsl_matrix_set(c, i, 0, 0.0);
	}
	for (i = 0; i < ssteps; ++i)
		for (j = 1; j <= tsteps; ++j) {
			gsl_matrix_set(s, i, j, gsl_matrix_get(s, i, j - 1) *
				exp((r - d - vol * vol / 2) * dt + vol *
					sqrt(dt) * gsl_matrix_get(a, i, j - 1)));
			j != tsteps ? gsl_matrix_set(c, i, j, 0.0) :
				gsl_matrix_set(c, i, j, MAX(0.0, gsl_matrix_get(s, i, j) - strike));
		}
	for (j = tsteps - 1; j >= 1; --j) {
		int k = 0;

		for (i = 0; i < ssteps; ++i)
			if (gsl_matrix_get(s, i, j) > strike)
				++k;
		/* FIXME */
		if (k > 0) {
			gsl_matrix *R, *R2;
			gsl_vector *y, *x, *cf;
			double sum;

			R  = gsl_matrix_alloc(k, 3);
			y  = gsl_vector_alloc(k);
			x  = gsl_vector_alloc(3);
			cf = gsl_vector_alloc(k);
			k = 0;
			for (i = 0; i < ssteps; ++i)
				if (gsl_matrix_get(s, i, j) > strike) {
					double tmp = gsl_matrix_get(s, i, j) / spot;

					gsl_matrix_set(R, k, 0, 1);
					gsl_matrix_set(R, k, 1, 1 - tmp);
					gsl_matrix_set(R, k, 2, 1 - 2 * tmp - tmp * tmp / 2);
					gsl_vector_set(y, k++, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
				}
			R2 = pinv(R);
			for (i = 0; i < x->size; ++i) {
				sum = 0.0;
				for (k = 0; k < R2->size2; ++k)
					sum += gsl_matrix_get(R2, i, k) * gsl_vector_get(y, k);
				gsl_vector_set(x, i, sum);
			}
			for (i = 0; i < cf->size; ++i) {
				sum = 0.0;
				for (k = 0; k < R->size2; ++k)
					sum += gsl_matrix_get(R, i, k) * gsl_vector_get(x, k);
				gsl_vector_set(cf, i, sum);
			}
			k = 0;
			for (i = 0; i < ssteps; ++i)
				if (gsl_matrix_get(s, i, j) > strike &&
					MAX(0.0, gsl_matrix_get(s, i, j) - strike) > gsl_vector_get(cf, k++))
					gsl_matrix_set(c, i, j, MAX(0.0, gsl_matrix_get(s, i, j) - strike));
				else
					gsl_matrix_set(c, i, j, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
			gsl_vector_free(cf);
			gsl_vector_free(x);
			gsl_vector_free(y);
			gsl_matrix_free(R2);
			gsl_matrix_free(R);
		} else
			for (i = 0; i < ssteps; ++i)
				gsl_matrix_set(c, i, j, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
	}
	for (i = 0; i < ssteps; ++i)
		res += gsl_matrix_get(c, i, 1);
	res = res / ssteps * exp(-r * dt);
	gsl_matrix_free(c);
	gsl_matrix_free(s);
	gsl_matrix_free(a);
	return res;
}

/* FIXME */
double lsm_amer_put(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps) {
	double dt = expiry / tsteps, res = 0.0;
	gsl_matrix *a = randn(ssteps, tsteps, 1, 1);
	gsl_matrix *s = gsl_matrix_alloc(ssteps, tsteps + 1);
	gsl_matrix *c = gsl_matrix_alloc(ssteps, tsteps + 1);
	int i, j;

	for (i = 0; i < ssteps; ++i) {
		gsl_matrix_set(s, i, 0, spot);
		gsl_matrix_set(c, i, 0, 0.0);
	}
	for (i = 0; i < ssteps; ++i)
		for (j = 1; j <= tsteps; ++j) {
			gsl_matrix_set(s, i, j, gsl_matrix_get(s, i, j - 1) *
				exp((r - d - vol * vol / 2) * dt + vol *
					sqrt(dt) * gsl_matrix_get(a, i, j - 1)));
			j != tsteps ? gsl_matrix_set(c, i, j, 0.0) :
				gsl_matrix_set(c, i, j, MAX(0.0, strike - gsl_matrix_get(s, i, j)));
		}
	for (j = tsteps - 1; j >= 1; --j) {
		int k = 0;

		for (i = 0; i < ssteps; ++i)
			if (gsl_matrix_get(s, i, j) < strike)
				++k;
		/* FIXME */
		if (k > 0) {
			gsl_matrix *R, *R2;
			gsl_vector *y, *x, *cf;
			double sum;

			R  = gsl_matrix_alloc(k, 3);
			R2 = gsl_matrix_alloc(k, 3);
			y  = gsl_vector_alloc(k);
			x  = gsl_vector_alloc(3);
			cf = gsl_vector_alloc(k);
			k = 0;
			for (i = 0; i < ssteps; ++i)
				if (gsl_matrix_get(s, i, j) < strike) {
					double tmp = gsl_matrix_get(s, i, j) / spot;

					gsl_matrix_set(R, k, 0, 1);
					gsl_matrix_set(R, k, 1, 1 - tmp);
					gsl_matrix_set(R, k, 2, 1 - 2 * tmp - tmp * tmp / 2);
					gsl_vector_set(y, k++, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
				}
			R2 = pinv(R);
			for (i = 0; i < x->size; ++i) {
				sum = 0.0;
				for (k = 0; k < R2->size2; ++k)
					sum += gsl_matrix_get(R2, i, k) * gsl_vector_get(y, k);
				gsl_vector_set(x, i, sum);
			}
			for (i = 0; i < cf->size; ++i) {
				sum = 0.0;
				for (k = 0; k < R->size2; ++k)
					sum += gsl_matrix_get(R, i, k) * gsl_vector_get(x, k);
				gsl_vector_set(cf, i, sum);
			}
			k = 0;
			for (i = 0; i < ssteps; ++i)
				if (gsl_matrix_get(s, i, j) < strike &&
					MAX(0.0, strike - gsl_matrix_get(s, i, j)) > gsl_vector_get(cf, k++))
					gsl_matrix_set(c, i, j, MAX(0.0, strike - gsl_matrix_get(s, i, j)));
				else
					gsl_matrix_set(c, i, j, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
			gsl_vector_free(cf);
			gsl_vector_free(x);
			gsl_vector_free(y);
			gsl_matrix_free(R2);
			gsl_matrix_free(R);
		} else
			for (i = 0; i < ssteps; ++i)
				gsl_matrix_set(c, i, j, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
	}
	for (i = 0; i < ssteps; ++i)
		res += gsl_matrix_get(c, i, 1);
	res = res / ssteps * exp(-r * dt);
	gsl_matrix_free(c);
	gsl_matrix_free(s);
	gsl_matrix_free(a);
	return res;
}

/* FIXME */
double impv_lsm(double spot, double strike, double r, double d, double expiry, int ssteps, int tsteps,
	double price, int type) {
	double low = 0.000001, high = 0.3, ce;

	/* FIXME */
	if (type != AMER_CALL && type != AMER_PUT)
		return NAN;
	ce = type == AMER_CALL ? lsm_amer_call(spot, strike, r, d, high, expiry, ssteps, tsteps) :
		lsm_amer_put(spot, strike, r, d, high, expiry, ssteps, tsteps);
	while (ce < price) {
		high *= 2.0;
		if (high > 1e10)
			return NAN;
		ce = type == AMER_CALL ? lsm_amer_call(spot, strike, r, d, high, expiry, ssteps, tsteps) :
			lsm_amer_put(spot, strike, r, d, high, expiry, ssteps, tsteps);
	}
	return type == AMER_CALL
		? brent(low, high, price, NULL, NULL, lsm_amer_call, spot, strike, r, d, expiry, ssteps, tsteps)
		: brent(low, high, price, NULL, NULL, lsm_amer_put,  spot, strike, r, d, expiry, ssteps, tsteps);
}

