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

#ifndef BAW_INCLUDED
#define BAW_INCLUDED

#include "impv.h"

/* FIXME: exported functions */
extern double baw_call(double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_put(double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_call_delta(double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_put_delta (double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_call_gamma(double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_put_gamma (double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_call_theta(double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_put_theta (double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_call_vega (double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_put_vega  (double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_call_rho  (double spot, double strike, double r, double d, double vol, double expiry);
extern double baw_put_rho   (double spot, double strike, double r, double d, double vol, double expiry);
extern double impv_baw(double spot, double strike, double r, double d, double expiry, double price, int type);

#endif /* BAW_INCLUDED */

