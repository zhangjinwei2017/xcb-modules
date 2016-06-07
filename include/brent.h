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

#ifndef BRENT_INCLUDED
#define BRENT_INCLUDED

/* FIXME: exported functions */
extern double brent(double a, double b, double price,
		double func (double spot, double strike, double r, double d, double vol, double expiry),
		double func2(double spot, double strike, double r, double d, double vol, double expiry, int steps),
		double spot, double strike, double r, double d, double expiry, int steps);

#endif /* BRENT_INCLUDED */

