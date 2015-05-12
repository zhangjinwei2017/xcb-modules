/*
 * Copyright (c) 2013-2015, Dalian Futures Information Technology Co., Ltd.
 *
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

#include <math.h>
#include "bs.h"
#include "baw.h"
#include "binomial.h"
#include "trinomial.h"
#include "impv.h"

double impv_bs(double spot, double strike, double r, double d, double expiry,
	double price, int type) {
	double low = 0.000001, high = 0.3, ce;
	int niters = 0;

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
	while (++niters < 500) {
		double vol = 0.5 * (low + high);

		ce = type == EURO_CALL ? bs_call(spot, strike, r, d, vol, expiry) :
			bs_put(spot, strike, r, d, vol, expiry);
		if (fabs(ce - price) <= 0.000001)
			return vol;
		if (ce < price)
			low  = vol;
		else
			high = vol;
	}
	return NAN;
}

double impv_baw(double spot, double strike, double r, double d, double expiry,
	double price, int type) {
	double low = 0.000001, high = 0.3, ce;
	int niters = 0;

	/* FIXME */
	if (type != AMER_CALL && type != AMER_PUT)
		return NAN;
	ce = type == AMER_CALL ? baw_call(spot, strike, r, d, high, expiry) :
		baw_put(spot, strike, r, d, high, expiry);
	while (ce < price) {
		high *= 2.0;
		if (high > 1e10)
			return NAN;
		ce = type == AMER_CALL ? baw_call(spot, strike, r, d, high, expiry) :
			baw_put(spot, strike, r, d, high, expiry);
	}
	while (++niters < 500) {
		double vol = 0.5 * (low + high);

		ce = type == AMER_CALL ? baw_call(spot, strike, r, d, vol, expiry) :
			baw_put(spot, strike, r, d, vol, expiry);
		if (fabs(ce - price) <= 0.000001)
			return vol;
		if (ce < price)
			low  = vol;
		else
			high = vol;
	}
	return NAN;
}

double impv_binomial(double spot, double strike, double r, double d, double expiry, int steps,
	double price, int type) {
	double low = 0.000001, high = 0.3, ce;
	int niters = 0;

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
	while (++niters < 500) {
		double vol = 0.5 * (low + high);

		ce = type == AMER_CALL ? bi_amer_call(spot, strike, r, d, vol, expiry, steps) :
			bi_amer_put(spot, strike, r, d, vol, expiry, steps);
		if (fabs(ce - price) <= 0.000001)
			return vol;
		if (ce < price)
			low  = vol;
		else
			high = vol;
	}
	return NAN;
}

double impv_trinomial(double spot, double strike, double r, double d, double expiry, int steps,
	double price, int type) {
	double low = 0.000001, high = 0.3, ce;
	int niters = 0;

	/* FIXME */
	if (type != AMER_CALL && type != AMER_PUT)
		return NAN;
	ce = type == AMER_CALL ? tri_amer_call(spot, strike, r, d, high, expiry, steps) :
		tri_amer_put(spot, strike, r, d, high, expiry, steps);
	while (ce < price) {
		high *= 2.0;
		if (high > 1e10)
			return NAN;
		ce = type == AMER_CALL ? tri_amer_call(spot, strike, r, d, high, expiry, steps) :
			tri_amer_put(spot, strike, r, d, high, expiry, steps);
	}
	while (++niters < 500) {
		double vol = 0.5 * (low + high);

		ce = type == AMER_CALL ? tri_amer_call(spot, strike, r, d, vol, expiry, steps) :
			tri_amer_put(spot, strike, r, d, vol, expiry, steps);
		if (fabs(ce - price) <= 0.000001)
			return vol;
		if (ce < price)
			low  = vol;
		else
			high = vol;
	}
	return NAN;
}

