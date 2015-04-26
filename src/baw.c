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
 * revised by xiaoyem
 */

#include <math.h>
#include "norms.h"
#include "bs.h"
#include "baw.h"

/* FIXME */
double baw_call(double spot, double strike, double r, double d, double vol, double expiry) {
	double M = 2.0 * r / (vol * vol);
	double N = 2.0 * (r - d) / (vol * vol);
	double K = 1.0 - exp(-r * expiry);
	double q2 = 0.5 * (-(N - 1) + sqrt((N - 1) * (N - 1) + 4 * M / K));
	double q2inf = 0.5 *(-(N - 1) + sqrt((N - 1) * (N - 1) + 4 * M));
	double Ssinf = strike / (1.0 - 1.0 / q2inf);
	double stddev = vol * sqrt(expiry);
	double h2 = -((r - d) * expiry + 2.0 * stddev) * (strike / (Ssinf - strike));
	double S0 = strike + (Ssinf - strike) * (1.0 - exp(h2));
	double g = 1.0;
	double gprime = 1.0;
	int niters = 0;
	double Si = S0;
	double Ss;
	double ca, ce;

	while (fabs(g) > 0.000001 && fabs(gprime) > 0.000001 && ++niters < 500 && Si > 0.0) {
		double ce = bs_call(Si, strike, r, d, vol, expiry);
		double d1 = (log(Si / strike) + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;

		g = (1.0 - 1.0 / q2) * Si - strike - ce +
			(1.0 / q2) * Si * exp(-d * expiry) * cum_norm(d1);
		gprime = (1.0 - 1.0 / q2) * (1.0 - exp(-d * expiry)) * cum_norm(d1) +
			(1.0 / q2) * exp(-d * expiry) * cum_norm(d1) * (1.0 / stddev);
		Si -= g/gprime;
	}
	Ss = fabs(g) > 0.000001 ? S0 : Si;
	ce = bs_call(spot, strike, r, d, vol, expiry);
	if (spot >= Ss)
		ca = spot - strike;
	else {
		double d1 = (log(Ss / strike) + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;
		double A2 = (Ss / q2) * (1.0 - exp(-d * expiry) * cum_norm(d1));

		ca = ce + A2 * pow(spot / Ss, q2);
	}
	return ca > ce ? ca : ce;
}

/* FIXME */
double baw_put(double spot, double strike, double r, double d, double vol, double expiry) {
	double M = 2.0 * r / (vol * vol);
	double N = 2.0 * (r - d) / (vol * vol);
	double K = 1.0 - exp(-r * expiry);
	double q1 = 0.5 * (-(N - 1) - sqrt((N - 1) * (N - 1) + 4 * M / K));
	double q1inf = 0.5 *(-(N - 1) - sqrt((N - 1) * (N - 1) + 4 * M));
	double Ssinf = strike / (1.0 - 1.0 / q1inf);
	double stddev = vol * sqrt(expiry);
	double h1 = ((r - d) * expiry - 2.0 * stddev) * (strike / (strike - Ssinf));
	double S0 = Ssinf + (strike - Ssinf) * exp(h1);
	double g = 1.0;
	double gprime = 1.0;
	int niters = 0;
	double Si = S0;
	double Ss;
	double pa, pe;

	while (fabs(g) > 0.000001 && fabs(gprime) > 0.000001 && ++niters < 500 && Si > 0.0) {
		double pe = bs_put(Si, strike, r, d, vol, expiry);
		double d1 = (log(Si / strike) + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;

		g = strike - Si - pe +
			(1.0 / q1) * Si * (1.0 - exp(-d * expiry)) * cum_norm(-d1);
		gprime = (1.0 / q1 - 1.0) * (1.0 - exp(-d * expiry)) * cum_norm(-d1) +
			(1.0 / q1) * exp(-d * expiry) * cum_norm(-d1) * (1.0 / stddev);
		Si -= g/gprime;
	}
	Ss = fabs(g) > 0.000001 ? S0 : Si;
	pe = bs_put(spot, strike, r, d, vol, expiry);
	if (spot <= Ss)
		pa = strike - spot;
	else {
		double d1 = (log(Ss / strike) + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;
		double A1 = (-Ss / q1) * (1.0 - exp(-d * expiry) * cum_norm(-d1));

		pa = pe + A1 * pow(spot / Ss, q1);
	}
	return pa > pe ? pa : pe;
}

/* FIXME */
inline double baw_call_delta(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_call(1.001 * spot, strike, r, d, vol, expiry) -
		baw_call(0.999 * spot, strike, r, d, vol, expiry)) / (0.002 * spot);
}

/* FIXME */
inline double baw_put_delta(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_put(1.001 * spot, strike, r, d, vol, expiry) -
		baw_put(0.999 * spot, strike, r, d, vol, expiry)) / (0.002 * spot);
}

/* FIXME */
inline double baw_call_gamma(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_call_delta(1.01 * spot, strike, r, d, vol, expiry) -
		baw_call_delta(0.99 * spot, strike, r, d, vol, expiry)) / (0.02 * spot);
}

/* FIXME */
inline double baw_put_gamma(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_put_delta(1.01 * spot, strike, r, d, vol, expiry) -
		baw_put_delta(0.99 * spot, strike, r, d, vol, expiry)) / (0.02 * spot);
}

/* FIXME */
inline double baw_call_theta(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_call(spot, strike, r, d, vol, 1.01 * expiry) -
		baw_call(spot, strike, r, d, vol, 0.99 * expiry)) / (0.02 * expiry);
}

/* FIXME */
inline double baw_put_theta(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_put(spot, strike, r, d, vol, 1.01 * expiry) -
		baw_put(spot, strike, r, d, vol, 0.99 * expiry)) / (0.02 * expiry);
}

/* FIXME */
inline double baw_call_vega(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_call(spot, strike, r, d, 1.001 * vol, expiry) -
		baw_call(spot, strike, r, d, 0.999 * vol, expiry)) / (0.002 * vol);
}

/* FIXME */
inline double baw_put_vega(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_put(spot, strike, r, d, 1.001 * vol, expiry) -
		baw_put(spot, strike, r, d, 0.999 * vol, expiry)) / (0.002 * vol);
}

/* FIXME */
inline double baw_call_rho(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_call(spot, strike, 1.01 * r, d, vol, expiry) -
		baw_call(spot, strike, 0.99 * r, d, vol, expiry)) / (0.02 * r);
}

/* FIXME */
inline double baw_put_rho(double spot, double strike, double r, double d, double vol, double expiry) {
	return (baw_put(spot, strike, 1.01 * r, d, vol, expiry) -
		baw_put(spot, strike, 0.99 * r, d, vol, expiry)) / (0.02 * r);
}

