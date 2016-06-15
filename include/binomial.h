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

#ifndef BINOMIAL_INCLUDED
#define BINOMIAL_INCLUDED

#include "impv.h"

/* FIXME: exported functions */
extern double bi_euro_call(double spot, double strike, double r, double d, double vol,
		double expiry, int steps);
extern double bi_euro_put (double spot, double strike, double r, double d, double vol,
		double expiry, int steps);
extern double bi_amer_call(double spot, double strike, double r, double d, double vol,
		double expiry, int steps);
extern double bi_amer_put (double spot, double strike, double r, double d, double vol,
		double expiry, int steps);
extern void   bi_amer_call_greeks(double spot, double strike, double r, double d, double vol, double expiry,
		int steps, double *delta, double *gamma, double *theta, double *vega, double *rho);
extern void   bi_amer_put_greeks (double spot, double strike, double r, double d, double vol, double expiry,
		int steps, double *delta, double *gamma, double *theta, double *vega, double *rho);
extern double impv_bi(double spot, double strike, double r, double d, double expiry, int steps,
		double price, int type);

#endif /* BINOMIAL_INCLUDED */

