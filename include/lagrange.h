/*
 * Copyright (c) 2013-2016, Dalian Futures Information Technology Co., Ltd.
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

#ifndef LAGRANGE_INCLUDED
#define LAGRANGE_INCLUDED

/* FIXME: exported functions */
extern double quadratic(double strike, double strike1, double strike2, double strike3,
		double vol1, double vol2, double vol3);
extern double cubic(double strike, double strike1, double strike2, double strike3, double strike4,
		double vol1, double vol2, double vol3, double vol4);

#endif /* LAGRANGE_INCLUDED */

