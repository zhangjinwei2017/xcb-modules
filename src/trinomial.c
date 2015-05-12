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

#include <math.h>
#include <xcb/macros.h>
#include <xcb/mem.h>
#include "trinomial.h"

/* FIXME */
double tri_euro_call(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	return 0.0;
}

/* FIXME */
double tri_euro_put(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	return 0.0;
}

/* FIXME */
double tri_amer_call(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	double vv = vol * vol;
	/* up movement */
	double up = exp(vol * sqrt(3.0 * dt));
	/* down movement */
	double dn = 1.0 / up;
	double p_up = 1.0 / 6.0 + sqrt(dt / (12.0 * vv)) * (r - d - 0.5 * vv);
	double p_md = 2.0 / 3.0;
	double p_dn = 1.0 / 6.0 - sqrt(dt / (12.0 * vv)) * (r - d - 0.5 * vv);
	double *prices, *call_values, res;
	int i, step;

	if ((prices = ALLOC((2 * steps + 1) * sizeof (double))) == NULL)
		return 0.0;
	if ((call_values = ALLOC((2 * steps + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return 0.0;
	}
	prices[0] = spot * pow(dn, steps);
	call_values[0] = MAX(0.0, prices[0] - strike);
	for (i = 1; i <= 2 * steps; ++i) {
		/* price of underlying */
		prices[i] = up * prices[i - 1];
		/* value of corresponding call */
		call_values[i] = MAX(0.0, prices[i] - strike);
	}
	for (step = steps - 1; step >= 0; --step)
		for (i = 0; i <= 2 * step; ++i) {
			call_values[i] = Rinv * (p_up * call_values[i + 2] + p_md * call_values[i + 1] +
				p_dn * call_values[i]);
			call_values[i] = MAX(call_values[i], prices[i + steps - step] - strike);
		}
	res = call_values[0];
	FREE(call_values);
	FREE(prices);
	return res;
}

/* FIXME */
double tri_amer_put(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	double vv = vol * vol;
	/* up movement */
	double up = exp(vol * sqrt(3.0 * dt));
	/* down movement */
	double dn = 1.0 / up;
	double p_up = 1.0 / 6.0 + sqrt(dt / (12.0 * vv)) * (r - d - 0.5 * vv);
	double p_md = 2.0 / 3.0;
	double p_dn = 1.0 / 6.0 - sqrt(dt / (12.0 * vv)) * (r - d - 0.5 * vv);
	double *prices, *put_values, res;
	int i, step;

	if ((prices = ALLOC((2 * steps + 1) * sizeof (double))) == NULL)
		return 0.0;
	if ((put_values = ALLOC((2 * steps + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return 0.0;
	}
	prices[0] = spot * pow(dn, steps);
	put_values[0] = MAX(0.0, strike - prices[0]);
	for (i = 1; i <= 2 * steps; ++i) {
		/* price of underlying */
		prices[i] = up * prices[i - 1];
		/* value of corresponding put */
		put_values[i] = MAX(0.0, strike - prices[i]);
	}
	for (step = steps - 1; step >= 0; --step)
		for (i = 0; i <= 2 * step; ++i) {
			put_values[i] = Rinv * (p_up * put_values[i + 2] + p_md * put_values[i + 1] +
				p_dn * put_values[i]);
			put_values[i] = MAX(put_values[i], strike - prices[i + steps - step]);
		}
	res = put_values[0];
	FREE(put_values);
	FREE(prices);
	return res;
}

/* FIXME */
void tri_amer_call_greeks(double spot, double strike, double r, double d, double vol, double expiry, int steps,
	double *delta, double *gamma, double *theta, double *vega, double *rho) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	double vv = vol * vol;
	/* up movement */
	double up = exp(vol * sqrt(3.0 * dt));
	/* down movement */
	double dn = 1.0 / up;
	double p_up = 1.0 / 6.0 + sqrt(dt / (12.0 * vv)) * (r - d - 0.5 * vv);
	double p_md = 2.0 / 3.0;
	double p_dn = 1.0 / 6.0 - sqrt(dt / (12.0 * vv)) * (r - d - 0.5 * vv);
	double *prices, *call_values;
	int i, step;
	double f12, f11, f10, f00;

	if ((prices = ALLOC((2 * steps + 1) * sizeof (double))) == NULL)
		return;
	if ((call_values = ALLOC((2 * steps + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return;
	}
	prices[0] = spot * pow(dn, steps);
	call_values[0] = MAX(0.0, prices[0] - strike);
	for (i = 1; i <= 2 * steps; ++i) {
		/* price of underlying */
		prices[i] = up * prices[i - 1];
		/* value of corresponding call */
		call_values[i] = MAX(0.0, prices[i] - strike);
	}
	for (step = steps - 1; step >= 1; --step)
		for (i = 0; i <= 2 * step; ++i) {
			call_values[i] = Rinv * (p_up * call_values[i + 2] + p_md * call_values[i + 1] +
				p_dn * call_values[i]);
			call_values[i] = MAX(call_values[i], prices[i + steps - step] - strike);
		}
	f12 = call_values[2];
	f11 = call_values[1];
	f10 = call_values[0];
	call_values[0] = Rinv * (p_up * call_values[2] + p_md * call_values[1] + p_dn * call_values[0]);
	call_values[0] = MAX(call_values[0], spot - strike);
	f00 = call_values[0];
	*delta = (f12 - f10) / (spot * up - spot * dn);
	*gamma = ((f12 - f11) / (spot * up - spot) - (f11 - f10) / (spot - spot * dn)) /
		(0.5 * (spot * up - spot * dn));
	*theta = (f11 - f00) / dt;
	*vega  = (tri_amer_call(spot, strike, r, d, vol + 0.02, expiry, steps) - f00) / 0.02;
	*rho   = (tri_amer_call(spot, strike, r + 0.05, d + 0.05, vol, expiry, steps) - f00) / 0.05;
	FREE(call_values);
	FREE(prices);
}

/* FIXME */
void tri_amer_put_greeks(double spot, double strike, double r, double d, double vol, double expiry, int steps,
	double *delta, double *gamma, double *theta, double *vega, double *rho) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	double vv = vol * vol;
	/* up movement */
	double up = exp(vol * sqrt(3.0 * dt));
	/* down movement */
	double dn = 1.0 / up;
	double p_up = 1.0 / 6.0 + sqrt(dt / (12.0 * vv)) * (r - d - 0.5 * vv);
	double p_md = 2.0 / 3.0;
	double p_dn = 1.0 / 6.0 - sqrt(dt / (12.0 * vv)) * (r - d - 0.5 * vv);
	double *prices, *put_values;
	int i, step;
	double f12, f11, f10, f00;

	if ((prices = ALLOC((2 * steps + 1) * sizeof (double))) == NULL)
		return;
	if ((put_values = ALLOC((2 * steps + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return;
	}
	prices[0] = spot * pow(dn, steps);
	put_values[0] = MAX(0.0, strike - prices[0]);
	for (i = 1; i <= 2 * steps; ++i) {
		/* price of underlying */
		prices[i] = up * prices[i - 1];
		/* value of corresponding put */
		put_values[i] = MAX(0.0, strike - prices[i]);
	}
	for (step = steps - 1; step >= 1; --step)
		for (i = 0; i <= 2 * step; ++i) {
			put_values[i] = Rinv * (p_up * put_values[i + 2] + p_md * put_values[i + 1] +
				p_dn * put_values[i]);
			put_values[i] = MAX(put_values[i], strike - prices[i + steps - step]);
		}
	f12 = put_values[2];
	f11 = put_values[1];
	f10 = put_values[0];
	put_values[0] = Rinv * (p_up * put_values[2] + p_md * put_values[1] + p_dn * put_values[0]);
	put_values[0] = MAX(put_values[0], strike - spot);
	f00 = put_values[0];
	*delta = (f12 - f10) / (spot * up - spot * dn);
	*gamma = ((f12 - f11) / (spot * up - spot) - (f11 - f10) / (spot - spot * dn)) /
		(0.5 * (spot * up - spot * dn));
	*theta = (f11 - f00) / dt;
	*vega  = (tri_amer_put(spot, strike, r, d, vol + 0.02, expiry, steps) - f00) / 0.02;
	*rho   = (tri_amer_put(spot, strike, r + 0.05, d + 0.05, vol, expiry, steps) - f00) / 0.05;
	FREE(put_values);
	FREE(prices);
}

