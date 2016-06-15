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
#include "brent.h"
#include "binomial.h"

/* FIXME */
double bi_euro_call(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	return 0.0;
}

/* FIXME */
double bi_euro_put(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	return 0.0;
}

/* FIXME */
double bi_amer_call(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	/* up movement */
	double up = exp(vol * sqrt(dt));
	double uu = up * up;
	/* down movement */
	double dn = 1.0 / up;
	double p_up = (exp((r - d) * dt) - dn) / (up - dn);
	double p_dn = 1.0 - p_up;
	double *prices, *call_values, res;
	int i, step;

	if ((prices = ALLOC((steps + 1) * sizeof (double))) == NULL)
		return 0.0;
	if ((call_values = ALLOC((steps + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return 0.0;
	}
	prices[0] = spot * pow(dn, steps);
	call_values[0] = MAX(0.0, prices[0] - strike);
	for (i = 1; i <= steps; ++i) {
		/* price of underlying */
		prices[i] = uu * prices[i - 1];
		/* value of corresponding call */
		call_values[i] = MAX(0.0, prices[i] - strike);
	}
	for (step = steps - 1; step >= 0; --step)
		for (i = 0; i <= step; ++i) {
			call_values[i] = Rinv * (p_up * call_values[i + 1] + p_dn * call_values[i]);
			prices[i] = dn * prices[i + 1];
			call_values[i] = MAX(call_values[i], prices[i] - strike);
		}
	res = call_values[0];
	FREE(call_values);
	FREE(prices);
	return res;
}

/* FIXME */
double bi_amer_put(double spot, double strike, double r, double d, double vol, double expiry, int steps) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	/* up movement */
	double up = exp(vol * sqrt(dt));
	double uu = up * up;
	/* down movement */
	double dn = 1.0 / up;
	double p_up = (exp((r - d) * dt) - dn) / (up - dn);
	double p_dn = 1.0 - p_up;
	double *prices, *put_values, res;
	int i, step;

	if ((prices = ALLOC((steps + 1) * sizeof (double))) == NULL)
		return 0.0;
	if ((put_values = ALLOC((steps + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return 0.0;
	}
	prices[0] = spot * pow(dn, steps);
	put_values[0] = MAX(0.0, strike - prices[0]);
	for (i = 1; i <= steps; ++i) {
		/* price of underlying */
		prices[i] = uu * prices[i - 1];
		/* value of corresponding put */
		put_values[i] = MAX(0.0, strike - prices[i]);
	}
	for (step = steps - 1; step >= 0; --step)
		for (i = 0; i <= step; ++i) {
			put_values[i] = Rinv * (p_up * put_values[i + 1] + p_dn * put_values[i]);
			prices[i] = dn * prices[i + 1];
			put_values[i] = MAX(put_values[i], strike - prices[i]);
		}
	res = put_values[0];
	FREE(put_values);
	FREE(prices);
	return res;
}

/* FIXME */
void bi_amer_call_greeks(double spot, double strike, double r, double d, double vol, double expiry, int steps,
	double *delta, double *gamma, double *theta, double *vega, double *rho) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	/* up movement */
	double up = exp(vol * sqrt(dt));
	double uu = up * up;
	/* down movement */
	double dn = 1.0 / up;
	double p_up = (exp((r - d) * dt) - dn) / (up - dn);
	double p_dn = 1.0 - p_up;
	double *prices, *call_values;
	int i, step;
	double f22, f21, f20, f11, f10, f00;

	if ((prices = ALLOC((steps + 1) * sizeof (double))) == NULL)
		return;
	if ((call_values = ALLOC((steps + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return;
	}
	prices[0] = spot * pow(dn, steps);
	call_values[0] = MAX(0.0, prices[0] - strike);
	for (i = 1; i <= steps; ++i) {
		/* price of underlying */
		prices[i] = uu * prices[i - 1];
		/* value of corresponding call */
		call_values[i] = MAX(0.0, prices[i] - strike);
	}
	for (step = steps - 1; step >= 2; --step)
		for (i = 0; i <= step; ++i) {
			call_values[i] = Rinv * (p_up * call_values[i + 1] + p_dn * call_values[i]);
			prices[i] = dn * prices[i + 1];
			call_values[i] = MAX(call_values[i], prices[i] - strike);
		}
	f22 = call_values[2];
	f21 = call_values[1];
	f20 = call_values[0];
	for (i = 0; i <= 1; ++i) {
		call_values[i] = Rinv * (p_up * call_values[i + 1] + p_dn * call_values[i]);
		prices[i] = dn * prices[i + 1];
		call_values[i] = MAX(call_values[i], prices[i] - strike);
	}
	f11 = call_values[1];
	f10 = call_values[0];
	call_values[0] = Rinv * (p_up * call_values[1] + p_dn * call_values[0]);
	prices[0] = dn * prices[1];
	call_values[0] = MAX(call_values[0], prices[0] - strike);
	f00 = call_values[0];
	*delta = (f11 - f10) / (spot * up - spot * dn);
	*gamma = ((f22 - f21) / (spot * uu - spot) - (f21 - f20) / (spot - spot * dn * dn)) /
		(0.5 * (spot * uu - spot * dn * dn));
	*theta = (f21 - f00) / (2 * dt);
	*vega  = (bi_amer_call(spot, strike, r, d, vol + 0.02, expiry, steps) - f00) / 0.02;
	*rho   = (bi_amer_call(spot, strike, r + 0.05, d + 0.05, vol, expiry, steps) - f00) / 0.05;
	FREE(call_values);
	FREE(prices);
}

/* FIXME */
void bi_amer_put_greeks(double spot, double strike, double r, double d, double vol, double expiry, int steps,
	double *delta, double *gamma, double *theta, double *vega, double *rho) {
	double dt = expiry / steps;
	/* interest rate for each step */
	double R = exp(r * dt);
	/* inverse of interest rate */
	double Rinv = 1.0 / R;
	/* up movement */
	double up = exp(vol * sqrt(dt));
	double uu = up * up;
	/* down movement */
	double dn = 1.0 / up;
	double p_up = (exp((r - d) * dt) - dn) / (up - dn);
	double p_dn = 1.0 - p_up;
	double *prices, *put_values;
	int i, step;
	double f22, f21, f20, f11, f10, f00;

	if ((prices = ALLOC((steps + 1) * sizeof (double))) == NULL)
		return;
	if ((put_values = ALLOC((steps + 1) * sizeof (double))) == NULL) {
		FREE(prices);
		return;
	}
	prices[0] = spot * pow(dn, steps);
	put_values[0] = MAX(0.0, strike - prices[0]);
	for (i = 1; i <= steps; ++i) {
		/* price of underlying */
		prices[i] = uu * prices[i - 1];
		/* value of corresponding put */
		put_values[i] = MAX(0.0, strike - prices[i]);
	}
	for (step = steps - 1; step >= 2; --step)
		for (i = 0; i <= step; ++i) {
			put_values[i] = Rinv * (p_up * put_values[i + 1] + p_dn * put_values[i]);
			prices[i] = dn * prices[i + 1];
			put_values[i] = MAX(put_values[i], strike - prices[i]);
		}
	f22 = put_values[2];
	f21 = put_values[1];
	f20 = put_values[0];
	for (i = 0; i <= 1; ++i) {
		put_values[i] = Rinv * (p_up * put_values[i + 1] + p_dn * put_values[i]);
		prices[i] = dn * prices[i + 1];
		put_values[i] = MAX(put_values[i], strike - prices[i]);
	}
	f11 = put_values[1];
	f10 = put_values[0];
	put_values[0] = Rinv * (p_up * put_values[1] + p_dn * put_values[0]);
	prices[0] = dn * prices[1];
	put_values[0] = MAX(put_values[0], strike - prices[0]);
	f00 = put_values[0];
	*delta = (f11 - f10) / (spot * up - spot * dn);
	*gamma = ((f22 - f21) / (spot * uu - spot) - (f21 - f20) / (spot - spot * dn * dn)) /
		(0.5 * (spot * uu - spot * dn * dn));
	*theta = (f21 - f00) / (2 * dt);
	*vega  = (bi_amer_put(spot, strike, r, d, vol + 0.02, expiry, steps) - f00) / 0.02;
	*rho   = (bi_amer_put(spot, strike, r + 0.05, d + 0.05, vol, expiry, steps) - f00) / 0.05;
	FREE(put_values);
	FREE(prices);
}

/* FIXME */
double impv_bi(double spot, double strike, double r, double d, double expiry, int steps,
	double price, int type) {
	double low = 0.000001, high = 0.3, ce;

	/* FIXME */
	if (type != AMER_CALL && type != AMER_PUT)
		return NAN;
	ce = type == AMER_CALL ? bi_amer_call(spot, strike, r, d, high, expiry, steps) :
		bi_amer_put(spot, strike, r, d, high, expiry, steps);
	while (ce < price) {
		high *= 2.0;
		if (high > 1e10)
			return NAN;
		ce = type == AMER_CALL ? bi_amer_call(spot, strike, r, d, high, expiry, steps) :
			bi_amer_put(spot, strike, r, d, high, expiry, steps);
	}
	return type == AMER_CALL
		? brent(low, high, price, NULL, bi_amer_call, NULL, spot, strike, r, d, expiry, 0, steps)
		: brent(low, high, price, NULL, bi_amer_put,  NULL, spot, strike, r, d, expiry, 0, steps);
}

