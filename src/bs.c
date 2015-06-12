/*
 * Copyright (c) 2002 Mark Joshi
 *
 * Permission to use, copy, modify, distribute and sell this software for any
 * purpose is hereby granted without fee, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and this
 * permission notice appear in supporting documentation. Mark Joshi makes no
 * representations about the suitability of this software for any purpose. It is
 * provided "as is" without express or implied warranty.
 */

/*
 * revised by xiaoyem
 */

#include <math.h>
#include "norms.h"
#include "brent.h"
#include "bs.h"

double bs_call(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;
	double d2 = d1 - stddev;

	return spot * exp(-d * expiry) * cum_norm(d1) - strike * exp(-r * expiry) * cum_norm(d2);
}

double bs_put(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;
	double d2 = d1 - stddev;

	return strike * exp(-r * expiry) * cum_norm(-d2) - spot * exp(-d * expiry) * cum_norm(-d1);
}

double bs_call_delta(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;

	return exp(-d * expiry) * cum_norm(d1);
}

double bs_put_delta(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;

	return exp(-d * expiry) *(cum_norm(d1) - 1);
}

double bs_call_gamma(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;

	return exp(-d * expiry) * norm_density(d1) / (spot * vol * sqrt(expiry));
}

inline double bs_put_gamma(double spot, double strike, double r, double d, double vol, double expiry) {
	return bs_call_gamma(spot, strike, r, d, vol, expiry);
}

double bs_call_theta(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;
	double d2 = d1 - stddev;

	return d * spot * exp(-d * expiry) * cum_norm(d1) -
		spot * exp(-d * expiry) * vol * norm_density(d1) / (2 * sqrt(expiry)) -
		r * strike * exp(-r * expiry) * cum_norm(d2);
}

double bs_put_theta(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;
	double d2 = d1 - stddev;

	return r * strike * exp(-r * expiry) * cum_norm(-d2) -
		d * spot * exp(-d * expiry) * cum_norm(-d1) -
		spot * exp(-d * expiry) * vol * norm_density(d1) / (2 * sqrt(expiry));
}

double bs_call_vega(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;

	return spot * exp(-d * expiry) * sqrt(expiry) * norm_density(d1);
}

inline double bs_put_vega(double spot, double strike, double r, double d, double vol, double expiry) {
	return bs_call_vega(spot, strike, r, d, vol, expiry);
}

double bs_call_rho(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;
	double d2 = d1 - stddev;

	return strike * expiry * exp(-r * expiry) * cum_norm(d2);
}

double bs_put_rho(double spot, double strike, double r, double d, double vol, double expiry) {
	double moneyness = log(spot / strike);
	double stddev = vol * sqrt(expiry);
	double d1 = (moneyness + (r - d) * expiry + 0.5 * stddev * stddev) / stddev;
	double d2 = d1 - stddev;

	return -strike * expiry * exp(-r * expiry) * cum_norm(-d2);
}

double impv_bs(double spot, double strike, double r, double d, double expiry, double price, int type) {
	double low = 0.000001, high = 0.3, ce;

	if (type != EURO_CALL && type != EURO_PUT)
		return NAN;
	/*
	else if (type == EURO_CALL && price < 0.99 * (spot * exp(-d * expiry) - strike * exp(-r * expiry)))
		return NAN;
	else if (type == EURO_PUT  && price < 0.99 * (strike * exp(-r * expiry) - spot * exp(-d * expiry)))
		return NAN;
	*/
	ce = type == EURO_CALL ? bs_call(spot, strike, r, d, high, expiry) :
		bs_put(spot, strike, r, d, high, expiry);
	while (ce < price) {
		high *= 2.0;
		if (high > 1e10)
			return NAN;
		ce = type == EURO_CALL ? bs_call(spot, strike, r, d, high, expiry) :
			bs_put(spot, strike, r, d, high, expiry);
	}
	return type == EURO_CALL ? brent(low, high, price, bs_call, spot, strike, r, d, expiry) :
		brent(low, high, price, bs_put, spot, strike, r, d, expiry);
}

