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

#ifndef IMPV_INCLUDED
#define IMPV_INCLUDED

/* FIXME */
#define EURO_CALL 0
#define EURO_PUT  1
#define AMER_CALL 2
#define AMER_PUT  3

/* FIXME: exported functions */
extern double impv_bs(double spot, double strike, double r, double d, double expiry,
		double price, int type);
extern double impv_baw(double spot, double strike, double r, double d, double expiry,
		double price, int type);
extern double impv_binomial(double spot, double strike, double r, double d, double expiry, int steps,
		double price, int type);
extern double impv_trinomial(double spot, double strike, double r, double d, double expiry, int steps,
		double price, int type);

#endif /* IMPV_INCLUDED */

