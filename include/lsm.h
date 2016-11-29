/*
 * Copyright (c) 2013-2016, Dalian Futures Information Technology Co., Ltd.
 *
 * Gaohang Wu  <wugaohang at dce dot com dot cn>
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

#ifndef LSM_INCLUDED
#define LSM_INCLUDED

#include "impv.h"

/* FIXME: exported functions */
extern double lsm_amer_call(double spot, double strike, double r, double d, double vol,
		double expiry, int ssteps, int tsteps);
extern double lsm_amer_put (double spot, double strike, double r, double d, double vol,
		double expiry, int ssteps, int tsteps);
extern double impv_lsm(double spot, double strike, double r, double d, double expiry, int ssteps, int tsteps,
		double price, int type);

#endif /* LSM_INCLUDED */

