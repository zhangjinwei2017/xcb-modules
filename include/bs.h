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

#ifndef BS_INCLUDED
#define BS_INCLUDED

/* FIXME: exported functions */
extern double bs_call(double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_put (double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_call_delta(double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_put_delta (double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_call_gamma(double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_put_gamma (double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_call_theta(double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_put_theta (double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_call_vega (double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_put_vega  (double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_call_rho  (double spot, double strike, double r, double d, double vol, double expiry);
extern double bs_put_rho   (double spot, double strike, double r, double d, double vol, double expiry);

#endif /* BS_INCLUDED */

