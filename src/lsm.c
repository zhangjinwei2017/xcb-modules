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

#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_statistics.h>
#include <math.h>
#include <time.h>
#include <xcb/macros.h>
#include "brent.h"
#include "lsm.h"

/* FIXME */
gsl_matrix *randn(int m, int n, short antithetic, short moment_matching,
		unsigned long seed)
{
	double data[m * n];
	double var = 0.0, mean = 0.0;
	gsl_matrix *a = gsl_matrix_alloc(m, n);

	const gsl_rng_type *T = gsl_rng_default;
	gsl_rng *r;
	int i, j, index = 0;
	gsl_rng_default_seed = seed;
	r = gsl_rng_alloc(T);
	if (antithetic)
	{
		int m2 = (m % 2 == 0)? m / 2 : m / 2 + 1;
		for (i = 0; i < m2; ++i)
			for (j = 0; j < n; ++j)
			{
				gsl_matrix_set(a, i, j, gsl_ran_gaussian(r, 1.0));
				data[index++] = gsl_matrix_get(a, i, j);
				if (i + m2 < m)
				{
					gsl_matrix_set(a, i + m2, j, -1 * gsl_matrix_get(a, i, j));
					data[index++] = gsl_matrix_get(a, i + m2, j);
				}
			}
	}
	else
	{
		for (i = 0; i < m; ++i)
			for (j = 0; j < n; ++j)
			{
				gsl_matrix_set(a, i, j, gsl_ran_gaussian(r, 1.0));
				data[index++] = gsl_matrix_get(a, i, j);
			}
	}
	if (moment_matching)
	{
		mean = gsl_stats_mean(data, 1, index);
		var = gsl_stats_variance(data, 1, index);
		for (i = 0; i < m; i++)
			for (j = 0; j < n; j++)
				gsl_matrix_set(a, i, j, (gsl_matrix_get(a, i, j) - mean)/var);
	}
	gsl_rng_free(r);
	return a;
}

gsl_matrix* pinv(gsl_matrix* A)
{
	if (A->size1 < A->size2)
	{
		gsl_matrix *A_trans = gsl_matrix_alloc(A->size2, A->size1);
		gsl_matrix *pinv_res = gsl_matrix_alloc(A->size2, A->size1);
		gsl_matrix *pinv_trans_res = gsl_matrix_alloc(A->size1, A->size2);
		gsl_matrix_transpose_memcpy(A_trans, A);
		pinv_trans_res = pinv(A_trans);
		gsl_matrix_transpose_memcpy(pinv_res, pinv_trans_res);
		gsl_matrix_free(A_trans);
		gsl_matrix_free(pinv_trans_res);
		return pinv_res;
	}

	int i,j,k;
	double temp;
	gsl_matrix *V = gsl_matrix_alloc(A->size2, A->size2);
	gsl_vector *S = gsl_vector_alloc(A->size2);
	gsl_matrix *s = gsl_matrix_alloc(A->size2, 1);
	gsl_matrix *s_mat = gsl_matrix_alloc(A->size2, A->size2);
	gsl_vector *work = gsl_vector_alloc(A->size2);

	gsl_linalg_SV_decomp(A, V, S, work);

	for (i = 0; i < s->size1; i++)
		gsl_matrix_set(s, i, 0, (double)1 / gsl_vector_get(S, i));

	for (i = 0; i < s_mat->size1; i++)
		for (j = 0; j < s_mat->size2; j++)
			if (i != j)
				gsl_matrix_set(s_mat, i, j, 0.0);
			else
				gsl_matrix_set(s_mat, i, j, gsl_matrix_get(s, i, 0));
	gsl_matrix *X = gsl_matrix_alloc(A->size2, A->size1);
	for (i = 0; i < V->size1; i++)
		for (j = 0; j < V->size2; j++)
			gsl_matrix_set(V, i, j, gsl_matrix_get(V, i, j) * gsl_matrix_get(s, j, 0));

	for (i = 0; i < X->size1; i++)
		for (j = 0; j < X->size2; j++)
		{
			temp = 0.0;
			for (k = 0; k < V->size1; k++)
				temp += gsl_matrix_get(V, i, k) * gsl_matrix_get(A, j, k);
			gsl_matrix_set(X, i, j, temp);
		}
	gsl_matrix_free(V);
	gsl_vector_free(S);
	gsl_matrix_free(s);
	gsl_matrix_free(s_mat);
	gsl_vector_free(work);
	return X;
}

/* FIXME */
double lsm_amer_call(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps) {
	double dt = expiry / tsteps, res = 0.0;
//	gsl_matrix *a = randn(ssteps, tsteps);
	gsl_matrix *a = randn(ssteps, tsteps, 1, 1, time(NULL));
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
			gsl_vector *tau, *y, *x, *residual, *cf;
			gsl_matrix_view X, CF;

			R        = gsl_matrix_alloc(k, 3);
			R2       = gsl_matrix_alloc(k, 3);
			tau      = gsl_vector_alloc(3);
			y        = gsl_vector_alloc(k);
			x        = gsl_vector_alloc(3);
			residual = gsl_vector_alloc(k);
			cf       = gsl_vector_alloc(k);
			X        = gsl_matrix_view_vector(x,  3, 1);
			CF       = gsl_matrix_view_vector(cf, k, 1);
			k = 0;
			for (i = 0; i < ssteps; ++i)
				if (gsl_matrix_get(s, i, j) > strike) {
					double tmp = gsl_matrix_get(s, i, j) / spot;

					gsl_matrix_set(R, k, 0, 1);
					gsl_matrix_set(R, k, 1, 1 - tmp);
					gsl_matrix_set(R, k, 2, 1 - 2 * tmp - tmp * tmp / 2);
					gsl_vector_set(y, k++, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
				}
/*
			gsl_matrix_memcpy(R2, R);
			gsl_linalg_QR_decomp(R2, tau);
			gsl_linalg_QR_lssolve(R2, tau, y, x, residual);
			gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, R, &X.matrix, 0.0, &CF.matrix);
*/
			gsl_matrix* R_pinv = gsl_matrix_alloc(R->size2, R->size1);
			gsl_matrix* temp_R = gsl_matrix_alloc(R->size1, R->size2);
			gsl_matrix_memcpy(temp_R, R);
			R_pinv = pinv(temp_R);
			double temp = 0.0;
			int jj = 0;
			for (i = 0; i < x->size; i++)
			{
				temp = 0.0;
				for (jj = 0; jj < R->size1; jj++)
					temp += gsl_matrix_get(R_pinv, i, jj) * gsl_vector_get(y, jj);
				gsl_vector_set(x, i, temp);
			}
			for (i = 0; i < cf->size; i++)
			{
				temp = 0.0;
				for (jj = 0; jj < R->size2; jj++)
					temp += gsl_matrix_get(R, i, jj) * gsl_vector_get(x, jj);
				gsl_vector_set(cf, i, temp);
			}

			k = 0;
			for (i = 0; i < ssteps; ++i)
				if (gsl_matrix_get(s, i, j) > strike &&
					MAX(0.0, gsl_matrix_get(s, i, j) - strike) > gsl_vector_get(cf, k++))
					gsl_matrix_set(c, i, j, MAX(0.0, gsl_matrix_get(s, i, j) - strike));
				else
					gsl_matrix_set(c, i, j, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
			gsl_vector_free(cf);
			gsl_vector_free(residual);
			gsl_vector_free(x);
			gsl_vector_free(y);
			gsl_vector_free(tau);
			gsl_matrix_free(R2);
			gsl_matrix_free(R_pinv);
			gsl_matrix_free(temp_R);
//			gsl_matrix_free(R2);
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
//	gsl_matrix *a = randn(ssteps, tsteps);
	gsl_matrix *a = randn(ssteps, tsteps, 1, 1, time(NULL));
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
			gsl_vector *tau, *y, *x, *residual, *cf;
			gsl_matrix_view X, CF;

			R        = gsl_matrix_alloc(k, 3);
			R2       = gsl_matrix_alloc(k, 3);
			tau      = gsl_vector_alloc(3);
			y        = gsl_vector_alloc(k);
			x        = gsl_vector_alloc(3);
			residual = gsl_vector_alloc(k);
			cf       = gsl_vector_alloc(k);
			X        = gsl_matrix_view_vector(x,  3, 1);
			CF       = gsl_matrix_view_vector(cf, k, 1);
			k = 0;
			for (i = 0; i < ssteps; ++i)
				if (gsl_matrix_get(s, i, j) < strike) {
					double tmp = gsl_matrix_get(s, i, j) / spot;

					gsl_matrix_set(R, k, 0, 1);
					gsl_matrix_set(R, k, 1, 1 - tmp);
					gsl_matrix_set(R, k, 2, 1 - 2 * tmp - tmp * tmp / 2);
					gsl_vector_set(y, k++, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
				}
/*
			gsl_matrix_memcpy(R2, R);
			gsl_linalg_QR_decomp(R2, tau);
			gsl_linalg_QR_lssolve(R2, tau, y, x, residual);
			gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, R, &X.matrix, 0.0, &CF.matrix);
*/
			gsl_matrix* R_pinv = gsl_matrix_alloc(R->size2, R->size1);
			gsl_matrix* temp_R = gsl_matrix_alloc(R->size1, R->size2);
			gsl_matrix_memcpy(temp_R, R);
			R_pinv = pinv(temp_R);
			double temp = 0.0;
			int jj = 0;
			for (i = 0; i < x->size; i++)
			{
				temp = 0.0;
				for (jj = 0; jj < R->size1; jj++)
					temp += gsl_matrix_get(R_pinv, i, jj) * gsl_vector_get(y, jj);
				gsl_vector_set(x, i, temp);
			}

			for (i = 0; i < cf->size; i++)
			{
				temp = 0.0;
				for (jj = 0; jj < R->size2; jj++)
					temp += gsl_matrix_get(R, i, jj) * gsl_vector_get(x, jj);
				gsl_vector_set(cf, i, temp);
			}

			k = 0;
			for (i = 0; i < ssteps; ++i)
				if (gsl_matrix_get(s, i, j) < strike &&
					MAX(0.0, strike - gsl_matrix_get(s, i, j)) > gsl_vector_get(cf, k++))
					gsl_matrix_set(c, i, j, MAX(0.0, strike - gsl_matrix_get(s, i, j)));
				else
					gsl_matrix_set(c, i, j, gsl_matrix_get(c, i, j + 1) * exp(-r * dt));
			gsl_vector_free(cf);
			gsl_vector_free(residual);
			gsl_vector_free(x);
			gsl_vector_free(y);
			gsl_vector_free(tau);
//			gsl_matrix_free(R2);
			gsl_matrix_free(R_pinv);
			gsl_matrix_free(temp_R);
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

