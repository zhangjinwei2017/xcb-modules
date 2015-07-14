/*
 * Copyright (C) 2014 Bernt Arne Oedegaard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * revised by wb, xiaoyem
 */

#include <gsl/gsl_linalg.h>
#include <gsl/gsl_vector.h>
#include <xcb/macros.h>
#include <xcb/mem.h>
#include "fd.h"

/* FIXME */
double fd_euro_call(double spot, double strike, double r, double d, double vol,
	double expiry, int ssteps, int tsteps) {
	return 0.0;
}

/* FIXME */
double fd_euro_put(double spot, double strike, double r, double d, double vol,
	double expiry, int ssteps, int tsteps) {
	return 0.0;
}

/* FIXME */
double fd_amer_call(double spot, double strike, double r, double d, double vol,
	double expiry, int ssteps, int tsteps) {
	/* M needs to be even */
	int M = ssteps + ssteps % 2;
	int N = tsteps;
	double ds = 2.0 * spot / M;
	double dt = expiry / N;
	double vv = vol * vol;
	double *prices, *x, res;
	gsl_vector *diag, *e, *f, *B;
	gsl_vector_view X;
	int i, step;

	if ((prices = ALLOC((M + 1) * sizeof (double))) == NULL)
		return 0.0;
	if ((x = ALLOC((M + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return 0.0;
	}
	for (i = 0; i <= M; ++i)
		prices[i] = i * ds;
	/* tridiagonal systems */
	diag = gsl_vector_alloc(M + 1);
	gsl_vector_set(diag, 0, 1.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(diag, i, 1.0 + dt * (vv * i * i + r));
	gsl_vector_set(diag, M, 1.0);
	e = gsl_vector_alloc(M);
	gsl_vector_set(e, 0, 0.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(e, i, 0.5 * i * dt * (-r + d - vv * i));
	f = gsl_vector_alloc(M);
	for (i = 0; i < M - 1; ++i)
		gsl_vector_set(f, i, 0.5 * i * dt * (r - d - vv * i));
	gsl_vector_set(f, M - 1, 0.0);
	B = gsl_vector_alloc(M + 1);
	for (i = 0; i <= M; ++i)
		gsl_vector_set(B, i, MAX(0.0, prices[i] - strike));
	X = gsl_vector_view_array(x, M + 1);
	gsl_linalg_solve_tridiag(diag, e, f, B, &X.vector);
	for (step = N - 1; step > 0; --step) {
		gsl_vector_memcpy(B, &X.vector);
		gsl_linalg_solve_tridiag(diag, e, f, B, &X.vector);
		for (i = 1; i < M; ++i)
			x[i] = MAX(x[i], prices[i] - strike);
	}
	res = x[M / 2];
	gsl_vector_free(B);
	gsl_vector_free(f);
	gsl_vector_free(e);
	gsl_vector_free(diag);
	FREE(x);
	FREE(prices);
	return res;
}

/* FIXME */
double fd_amer_put(double spot, double strike, double r, double d, double vol,
	double expiry, int ssteps, int tsteps) {
	/* M needs to be even */
	int M = ssteps + ssteps % 2;
	int N = tsteps;
	double ds = 2.0 * spot / M;
	double dt = expiry / N;
	double vv = vol * vol;
	double *prices, *x, res;
	gsl_vector *diag, *e, *f, *B;
	gsl_vector_view X;
	int i, step;

	if ((prices = ALLOC((M + 1) * sizeof (double))) == NULL)
		return 0.0;
	if ((x = ALLOC((M + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return 0.0;
	}
	for (i = 0; i <= M; ++i)
		prices[i] = i * ds;
	/* tridiagonal systems */
	diag = gsl_vector_alloc(M + 1);
	gsl_vector_set(diag, 0, 1.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(diag, i, 1.0 + dt * (vv * i * i + r));
	gsl_vector_set(diag, M, 1.0);
	e = gsl_vector_alloc(M);
	gsl_vector_set(e, 0, 0.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(e, i, 0.5 * i * dt * (-r + d - vv * i));
	f = gsl_vector_alloc(M);
	for (i = 0; i < M - 1; ++i)
		gsl_vector_set(f, i, 0.5 * i * dt * (r - d - vv * i));
	gsl_vector_set(f, M - 1, 0.0);
	B = gsl_vector_alloc(M + 1);
	for (i = 0; i <= M; ++i)
		gsl_vector_set(B, i, MAX(0.0, strike - prices[i]));
	X = gsl_vector_view_array(x, M + 1);
	gsl_linalg_solve_tridiag(diag, e, f, B, &X.vector);
	for (step = N - 1; step > 0; --step) {
		gsl_vector_memcpy(B, &X.vector);
		gsl_linalg_solve_tridiag(diag, e, f, B, &X.vector);
		for (i = 1; i < M; ++i)
			x[i] = MAX(x[i], strike - prices[i]);
	}
	res = x[M / 2];
	gsl_vector_free(B);
	gsl_vector_free(f);
	gsl_vector_free(e);
	gsl_vector_free(diag);
	FREE(x);
	FREE(prices);
	return res;
}

