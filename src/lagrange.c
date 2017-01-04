/*
 * Copyright (c) 2013-2017, Dalian Futures Information Technology Co., Ltd.
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

#include "lagrange.h"

double quadratic(double strike, double strike1, double strike2, double strike3,
	double vol1, double vol2, double vol3) {
	return vol1 * (strike - strike2) * (strike - strike3) / (strike1 - strike2) / (strike1 - strike3) +
		vol2 * (strike - strike1) * (strike - strike3) / (strike2 - strike1) / (strike2 - strike3) +
		vol3 * (strike - strike1) * (strike - strike2) / (strike3 - strike1) / (strike3 - strike2);
}

double cubic(double strike, double strike1, double strike2, double strike3, double strike4,
	double vol1, double vol2, double vol3, double vol4) {
	return vol1 * (strike - strike2) * (strike - strike3) * (strike - strike4) /
		(strike1 - strike2) / (strike1 - strike3) / (strike1 - strike4) +
		vol2 * (strike - strike1) * (strike - strike3) * (strike - strike4) /
		(strike2 - strike1) / (strike2 - strike3) / (strike2 - strike4) +
		vol3 * (strike - strike1) * (strike - strike2) * (strike - strike4) /
		(strike3 - strike1) / (strike3 - strike2) / (strike3 - strike4) +
		vol4 * (strike - strike1) * (strike - strike2) * (strike - strike3) /
		(strike4 - strike1) / (strike4 - strike2) / (strike4 - strike3);
}

