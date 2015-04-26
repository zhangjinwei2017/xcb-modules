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

#include <math.h>
#include "norms.h"

static const double one_over_root_two_pi = 0.398942280401433;

/* unit or standard normal density */
inline double norm_density(double x) {
	return one_over_root_two_pi * exp(-x * x / 2);
}

/* normal distribution function */
double cum_norm(double x) {
	static double a[5] = {
		0.319381530,
		-0.356563782,
		1.781477937,
		-1.821255978,
		1.330274429
	};
	double res;

	if (x < -7.0)
		res = norm_density(x) / sqrt(1.0 + x * x);
	else if (x > 7.0)
		res = 1.0 - cum_norm(-x);
	else {
		double tmp = 1.0 / (1.0 + 0.2316419 * fabs(x));

		res = 1 - norm_density(x) *
			(tmp * (a[0] + tmp * (a[1] + tmp * (a[2] + tmp * (a[3] + tmp * a[4])))));
		if (x <= 0.0)
			res = 1.0 - res;
	}
	return res;
}

/* inverse normal distribution function */
double inv_cum_norm(double x) {
	static double a[4] = {
		2.50662823884,
		-18.61500062529,
		41.39119773534,
		-25.44106049637
	};
	static double b[4] = {
		-8.47351093090,
		23.08336743743,
		-21.06224101826,
		3.13082909833
	};
	static double c[9] = {
		0.3374754822726147,
		0.9761690190917186,
		0.1607979714918209,
		0.0276438810333863,
		0.0038405729373609,
		0.0003951896511919,
		0.0000321767881768,
		0.0000002888167364,
		0.0000003960315187
	};
	double u = x - 0.5;
	double res;

	/* Beasley-Springer */
	if (fabs(u) < 0.42) {
		double y = u * u;

		res = u * (((a[3] * y + a[2]) * y + a[1]) * y + a[0]) /
			((((b[3] * y + b[2]) * y + b[1]) * y + b[0]) * y + 1.0);
	/* Moro */
	} else {
		res = x;
		if (u > 0.0)
			res = 1.0 - x;
		res = log(-log(res));
		res = c[0] + res * (c[1] + res * (c[2] + res * (c[3] + res * (c[4] + res * (c[5] +
			res * (c[6] + res * (c[7] + res * c[8])))))));
		if (u < 0.0)
			res = -res;
	}
	return res;
}

