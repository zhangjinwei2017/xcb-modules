/*
 * Copyright (c) 2013-2016, Dalian Futures Information Technology Co., Ltd.
 *
 * Bo Wang
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
#include "brent.h"

double brent(double a, double b, double price,
	double func (double spot, double strike, double r, double d, double vol, double expiry),
	double func2(double spot, double strike, double r, double d, double vol, double expiry, int steps),
	double spot, double strike, double r, double d, double expiry, int steps) {
	double fa, fb, c, fc, s, fs, dd;
	int mflag, niters = 0;

	fa = func ? func(spot, strike, r, d, a, expiry) : func2(spot, strike, r, d, a, expiry, steps);
	fb = func ? func(spot, strike, r, d, b, expiry) : func2(spot, strike, r, d, b, expiry, steps);
	/* root is not bracketed */
	if ((fa - price) * (fb - price) >= 0.0)
		return NAN;
	/* swap */
	if (fabs(fa) < fabs(fb)) {
		double t = a, ft = fa;

		a  = b;
		b  = t;
		fa = fb;
		fb = ft;
	}
	c = a, fc = fa;
	mflag = 1;
	while (++niters < 500) {
		/* convergence */
		if (fabs(fb - price) <= 0.000001 || fabs(b - a) <= 0.00001)
			return b;
		if (fabs(fa - fc) > 0.000001 && fabs(fb - fc) > 0.000001)
			/* inverse quadratic interpolation */
			s = a * fb * fc / ((fa - fb) * (fa - fc)) +
				b * fa * fc / ((fb - fa) * (fb - fc)) +
				c * fa * fb / ((fc - fa) * (fc - fb));
		else
			/* secant method */
			s = b - fb * (b - a) / (fb - fa);
		if ((s < 0.25 * (3.0 * a + b) || s > b) ||
			( mflag && fabs(s - b) >= 0.5 * fabs(b - c)) ||
			(!mflag && fabs(s - b) >= 0.5 * fabs(c - dd)) ||
			( mflag && fabs(b - c) < 0.00001) ||
			(!mflag && fabs(c - dd) < 0.00001)) {
			/* bisection method */
			s = 0.5 * (a + b);
			mflag = 1;
		} else
			mflag = 0;
		fs = func ? func(spot, strike, r, d, s, expiry) : func2(spot, strike, r, d, s, expiry, steps);
		dd = c;
		c = b, fc = fb;
		if ((fa - price) * (fs - price) < 0.0) {
			b  = s;
			fb = fs;
		} else {
			a  = s;
			fa = fs;
		}
		/* swap */
		if (fabs(fa) < fabs(fb)) {
			double t = a, ft = fa;

			a  = b;
			b  = t;
			fa = fb;
			fb = ft;
		}
	}
	return NAN;
}

