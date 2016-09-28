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

#include <stdio.h>
#include "utils.h"
#include "baw.h"
#include "binomial.h"
#include "trinomial.h"
#include "fd.h"
#include "lsm.h"

int main(int argc, char **argv) {
	double spot = 2698.00, strike = 2650.00, r = 0.1, d = 0.1;
	double call = 252.00, put = 289.00, expiry = diffday(20160928, 20170807);

	printf("%f\n", impv_baw(spot, strike, r, d, expiry, call, AMER_CALL));
	printf("%f\n", impv_baw(spot, strike, r, d, expiry, put,  AMER_PUT));
	printf("%f\n", impv_bi (spot, strike, r, d, expiry, 1024, call, AMER_CALL));
	printf("%f\n", impv_bi (spot, strike, r, d, expiry, 1024, put,  AMER_PUT));
	printf("%f\n", impv_tri(spot, strike, r, d, expiry, 1024, call, AMER_CALL));
	printf("%f\n", impv_tri(spot, strike, r, d, expiry, 1024, put,  AMER_PUT));
	printf("%f\n", impv_fd (spot, strike, r, d, expiry, 100, 1024, call, AMER_CALL));
	printf("%f\n", impv_fd (spot, strike, r, d, expiry, 100, 1024, put,  AMER_PUT));
	printf("%f\n", impv_lsm(spot, strike, r, d, expiry, 10000, 100, call, AMER_CALL));
	printf("%f\n", impv_lsm(spot, strike, r, d, expiry, 10000, 100, put,  AMER_PUT));
	return 0;
}

