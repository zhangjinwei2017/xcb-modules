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
#include <math.h>
#include <xcb/macros.h>
#include <xcb/mem.h>
#include "brent.h"
#include "fd.h"

/* FIXME */
double fd_euro_call(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps) {
	return 0.0;
}

/* FIXME */
double fd_euro_put(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps) {
	return 0.0;
}

/* FIXME */
double fd_amer_call(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps) {
	/* M needs to be even */
	int M = ssteps + ssteps % 2;
	int N = tsteps;
	double ds = 2.0 * spot / M;
	double dt = expiry / N;
	double vv = vol * vol;
	double *prices, res;
	gsl_vector *diag, *e, *f, *b, *x;
	int i, step;

	if ((prices = ALLOC((M + 1) * sizeof (double))) == NULL)
		return 0.0;
	for (i = 0; i <= M; ++i)
		prices[i] = (i + 1) * ds;
	/* tridiagonal systems */
	diag = gsl_vector_alloc(M + 1);
	gsl_vector_set(diag, 0, 1.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(diag, i, 1.0 + dt * (vv * (i + 1) * (i + 1) + r));
	gsl_vector_set(diag, M, 1.0);
	e = gsl_vector_alloc(M);
	gsl_vector_set(e, 0, 0.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(e, i, 0.5 * (i + 1) * dt * (-r + d - vv * (i + 1)));
	f = gsl_vector_alloc(M);
	for (i = 0; i < M - 1; ++i)
		gsl_vector_set(f, i, 0.5 * (i + 2) * dt * (r - d - vv * (i + 2)));
	gsl_vector_set(f, M - 1, 0.0);
	b = gsl_vector_alloc(M + 1);
	for (i = 0; i <= M; ++i)
		gsl_vector_set(b, i, MAX(0.0, prices[i] - strike));
	x = gsl_vector_alloc(M + 1);
	gsl_linalg_solve_tridiag(diag, e, f, b, x);
	for (step = N - 1; step > 0; --step) {
		gsl_vector_memcpy(b, x);
		gsl_linalg_solve_tridiag(diag, e, f, b, x);
		for (i = 1; i < M; ++i)
			gsl_vector_set(x, i, MAX(gsl_vector_get(x, i), prices[i] - strike));
	}
	res = gsl_vector_get(x, M / 2 - 1);
	gsl_vector_free(x);
	gsl_vector_free(b);
	gsl_vector_free(f);
	gsl_vector_free(e);
	gsl_vector_free(diag);
	FREE(prices);
	return res;
}

/* FIXME */
double fd_amer_put(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps) {
	/* M needs to be even */
	int M = ssteps + ssteps % 2;
	int N = tsteps;
	double ds = 2.0 * spot / M;
	double dt = expiry / N;
	double vv = vol * vol;
	double *prices, res;
	gsl_vector *diag, *e, *f, *b, *x;
	int i, step;

	if ((prices = ALLOC((M + 1) * sizeof (double))) == NULL)
		return 0.0;
	for (i = 0; i <= M; ++i)
		prices[i] = (i + 1) * ds;
	/* tridiagonal systems */
	diag = gsl_vector_alloc(M + 1);
	gsl_vector_set(diag, 0, 1.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(diag, i, 1.0 + dt * (vv * (i + 1) * (i + 1) + r));
	gsl_vector_set(diag, M, 1.0);
	e = gsl_vector_alloc(M);
	gsl_vector_set(e, 0, 0.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(e, i, 0.5 * (i + 1) * dt * (-r + d - vv * (i + 1)));
	f = gsl_vector_alloc(M);
	for (i = 0; i < M - 1; ++i)
		gsl_vector_set(f, i, 0.5 * (i + 2) * dt * (r - d - vv * (i + 2)));
	gsl_vector_set(f, M - 1, 0.0);
	b = gsl_vector_alloc(M + 1);
	for (i = 0; i <= M; ++i)
		gsl_vector_set(b, i, MAX(0.0, strike - prices[i]));
	x = gsl_vector_alloc(M + 1);
	gsl_linalg_solve_tridiag(diag, e, f, b, x);
	for (step = N - 1; step > 0; --step) {
		gsl_vector_memcpy(b, x);
		gsl_linalg_solve_tridiag(diag, e, f, b, x);
		for (i = 1; i < M; ++i)
			gsl_vector_set(x, i, MAX(gsl_vector_get(x, i), strike - prices[i]));
	}
	res = gsl_vector_get(x, M / 2 - 1);
	gsl_vector_free(x);
	gsl_vector_free(b);
	gsl_vector_free(f);
	gsl_vector_free(e);
	gsl_vector_free(diag);
	FREE(prices);
	return res;
}

/* FIXME */
void fd_amer_call_greeks(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps, double *delta, double *gamma, double *theta, double *vega, double *rho) {
	/* M needs to be even */
	int M = ssteps + ssteps % 2;
	int N = tsteps;
	double ds = 2.0 * spot / M;
	double dt = expiry / N;
	double vv = vol * vol;
	double *prices;
	gsl_vector *diag, *e, *f, *b, *x;
	int i, step;
	double f12, f11, f10, f00;

	if ((prices = ALLOC((M + 1) * sizeof (double))) == NULL)
		return;
	for (i = 0; i <= M; ++i)
		prices[i] = (i + 1) * ds;
	/* tridiagonal systems */
	diag = gsl_vector_alloc(M + 1);
	gsl_vector_set(diag, 0, 1.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(diag, i, 1.0 + dt * (vv * (i + 1) * (i + 1) + r));
	gsl_vector_set(diag, M, 1.0);
	e = gsl_vector_alloc(M);
	gsl_vector_set(e, 0, 0.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(e, i, 0.5 * (i + 1) * dt * (-r + d - vv * (i + 1)));
	f = gsl_vector_alloc(M);
	for (i = 0; i < M - 1; ++i)
		gsl_vector_set(f, i, 0.5 * (i + 2) * dt * (r - d - vv * (i + 2)));
	gsl_vector_set(f, M - 1, 0.0);
	b = gsl_vector_alloc(M + 1);
	for (i = 0; i <= M; ++i)
		gsl_vector_set(b, i, MAX(0.0, prices[i] - strike));
	x = gsl_vector_alloc(M + 1);
	gsl_linalg_solve_tridiag(diag, e, f, b, x);
	for (step = N - 1; step > 1; --step) {
		gsl_vector_memcpy(b, x);
		gsl_linalg_solve_tridiag(diag, e, f, b, x);
		for (i = 1; i < M; ++i)
			gsl_vector_set(x, i, MAX(gsl_vector_get(x, i), prices[i] - strike));
	}
	f12 = gsl_vector_get(x, M / 2);
	f11 = gsl_vector_get(x, M / 2 - 1);
	f10 = gsl_vector_get(x, M / 2 - 2);
	gsl_vector_memcpy(b, x);
	gsl_linalg_solve_tridiag(diag, e, f, b, x);
	for (i = 1; i < M; ++i)
		gsl_vector_set(x, i, MAX(gsl_vector_get(x, i), prices[i] - strike));
	f00 = gsl_vector_get(x, M / 2 - 1);
	*delta = (f12 - f10) / (2 * ds);
	*gamma = (f12 - 2 * f11 + f10) / (ds * ds);
	*theta = (f11 - f00) / dt;
	*vega  = (fd_amer_call(spot, strike, r, d, 1.002 * vol, expiry, ssteps, tsteps) - f00) / (0.002 * vol);
	*rho   = (fd_amer_call(spot, strike, 1.02 * r, 1.02 * d, vol, expiry, ssteps, tsteps) - f00) / (0.02 * r);
	gsl_vector_free(x);
	gsl_vector_free(b);
	gsl_vector_free(f);
	gsl_vector_free(e);
	gsl_vector_free(diag);
	FREE(prices);
}

/* FIXME */
void fd_amer_put_greeks(double spot, double strike, double r, double d, double vol, double expiry,
	int ssteps, int tsteps, double *delta, double *gamma, double *theta, double *vega, double *rho) {
	/* M needs to be even */
	int M = ssteps + ssteps % 2;
	int N = tsteps;
	double ds = 2.0 * spot / M;
	double dt = expiry / N;
	double vv = vol * vol;
	double *prices;
	gsl_vector *diag, *e, *f, *b, *x;
	int i, step;
	double f12, f11, f10, f00;

	if ((prices = ALLOC((M + 1) * sizeof (double))) == NULL)
		return;
	for (i = 0; i <= M; ++i)
		prices[i] = (i + 1) * ds;
	/* tridiagonal systems */
	diag = gsl_vector_alloc(M + 1);
	gsl_vector_set(diag, 0, 1.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(diag, i, 1.0 + dt * (vv * (i + 1) * (i + 1) + r));
	gsl_vector_set(diag, M, 1.0);
	e = gsl_vector_alloc(M);
	gsl_vector_set(e, 0, 0.0);
	for (i = 1; i < M; ++i)
		gsl_vector_set(e, i, 0.5 * (i + 1) * dt * (-r + d - vv * (i + 1)));
	f = gsl_vector_alloc(M);
	for (i = 0; i < M - 1; ++i)
		gsl_vector_set(f, i, 0.5 * (i + 2) * dt * (r - d - vv * (i + 1)));
	gsl_vector_set(f, M - 1, 0.0);
	b = gsl_vector_alloc(M + 1);
	for (i = 0; i <= M; ++i)
		gsl_vector_set(b, i, MAX(0.0, strike - prices[i]));
	x = gsl_vector_alloc(M + 1);
	gsl_linalg_solve_tridiag(diag, e, f, b, x);
	for (step = N - 1; step > 1; --step) {
		gsl_vector_memcpy(b, x);
		gsl_linalg_solve_tridiag(diag, e, f, b, x);
		for (i = 1; i < M; ++i)
			gsl_vector_set(x, i, MAX(gsl_vector_get(x, i), strike - prices[i]));
	}
	f12 = gsl_vector_get(x, M / 2);
	f11 = gsl_vector_get(x, M / 2 - 1);
	f10 = gsl_vector_get(x, M / 2 - 2);
	gsl_vector_memcpy(b, x);
	gsl_linalg_solve_tridiag(diag, e, f, b, x);
	for (i = 1; i < M; ++i)
		gsl_vector_set(x, i, MAX(gsl_vector_get(x, i), strike - prices[i]));
	f00 = gsl_vector_get(x, M / 2 - 1);
	*delta = (f12 - f10) / (2 * ds);
	*gamma = (f12 - 2 * f11 + f10) / (ds * ds);
	*theta = (f11 - f00) / dt;
	*vega  = (fd_amer_put(spot, strike, r, d, 1.002 * vol, expiry, ssteps, tsteps) - f00) / (0.002 * vol);
	*rho   = (fd_amer_put(spot, strike, 1.02 * r, 1.02 * d, vol, expiry, ssteps, tsteps) - f00) / (0.02 * r);
	gsl_vector_free(x);
	gsl_vector_free(b);
	gsl_vector_free(f);
	gsl_vector_free(e);
	gsl_vector_free(diag);
	FREE(prices);
}

/* FIXME */
double impv_fd(double spot, double strike, double r, double d, double expiry, int ssteps, int tsteps,
	double price, int type) {
	double low = 0.000001, high = 0.3, ce;

	/* FIXME */
	if (type != AMER_CALL && type != AMER_PUT)
		return NAN;
	ce = type == AMER_CALL ? fd_amer_call(spot, strike, r, d, high, expiry, ssteps, tsteps) :
		fd_amer_put(spot, strike, r, d, high, expiry, ssteps, tsteps);
	while (ce < price) {
		high *= 2.0;
		if (high > 1e10)
			return NAN;
		ce = type == AMER_CALL ? fd_amer_call(spot, strike, r, d, high, expiry, ssteps, tsteps) :
			fd_amer_put(spot, strike, r, d, high, expiry, ssteps, tsteps);
	}
	return type == AMER_CALL
		? brent(low, high, price, NULL, NULL, fd_amer_call, spot, strike, r, d, expiry, ssteps, tsteps)
		: brent(low, high, price, NULL, NULL, fd_amer_put,  spot, strike, r, d, expiry, ssteps, tsteps);
}

