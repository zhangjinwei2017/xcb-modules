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

#include <time.h>
#include "utils.h"

/* FIXME */
int diffday(int startday, int endday) {
	time_t t = time(NULL);
	struct tm ls, le;
	int res;

	localtime_r(&t, &ls);
	localtime_r(&t, &le);
	ls.tm_mday = startday % 100;
	ls.tm_mon  = startday / 100 % 100 - 1;
	ls.tm_year = startday / 10000 - 1900;
	le.tm_mday = endday   % 100;
	le.tm_mon  = endday   / 100 % 100 - 1;
	le.tm_year = endday   / 10000 - 1900;
	res = difftime(mktime(&le), mktime(&ls)) / (24 * 60 * 60);
	/* return res / 7 * 5 + ((rem = res % 7) == 6 ? 5 : rem) + 1; */
	return res + 1;
}

/* FIXME */
int diffnow(int endday) {
	time_t t = time(NULL);
	struct tm le;
	int res;

	localtime_r(&t, &le);
	le.tm_mday = endday % 100;
	le.tm_mon  = endday / 100 % 100 - 1;
	le.tm_year = endday / 10000 - 1900;
	res = difftime(mktime(&le), t) / (24 * 60 * 60);
	/* return res / 7 * 5 + ((rem = res % 7) == 6 ? 5 : rem) + 1; */
	return res + 1;
}

