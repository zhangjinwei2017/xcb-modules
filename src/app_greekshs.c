/*
 * Copyright (c) 2013-2017, Dalian Futures Information Technology Co., Ltd.
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

#include <xcb/fmacros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <xcb/macros.h>
#include <xcb/mem.h>
#include <xcb/dstr.h>
#include <xcb/logger.h>
#include <xcb/module.h>
#include <xcb/basics.h>
#include "bs.h"
#include "baw.h"

/* FIXME */
static char *app = "greekshs";
static char *desc = "Delta, Gamma, Theta, Vega, and Rho (Historical)";
static char *fmt = "GREEKSHS,timestamp,contract,lastprice,delta,gamma,theta,vega,rho";

static int greekshs_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	dstr *fields = NULL;
	int nfield = 0;
	time_t t;
	struct tm lt;
	int msec, i;
	dstr spotname, sep;
	double spot, r, expiry;
	char datestr[64];
	NOT_USED(data2);

	fields = dstr_split_len(msg->data, strlen(msg->data), ",", 1, &nfield);
	/* FIXME */
	if (nfield < 10) {
		xcb_log(XCB_LOG_WARNING, "Message '%s' garbled", msg->data);
		goto end;
	}
	t        = (time_t)atoi(fields[1]);
	msec     = atoi(fields[2]);
	spotname = dstr_new(fields[3]);
	spot     = atof(fields[nfield - 4]);
	r        = atof(fields[nfield - 3]);
	expiry   = atof(fields[nfield - 2]);
	sep      = dstr_new(fields[nfield - 1]);
	strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
	for (i = 4; i < nfield - 4; i += 2) {
		char res[512];

		snprintf(res, sizeof res, "GREEKSHS,%s.%03d,%s%sC%s%s|0.00,%f,%f,%f,%f,%f",
			datestr,
			msec,
			spotname,
			sep,
			sep,
			fields[i],
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_call_delta (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_call_delta(spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry),
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_call_gamma (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_call_gamma(spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry),
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_call_theta (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_call_theta(spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry),
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_call_vega  (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_call_vega (spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry),
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_call_rho   (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_call_rho  (spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry));
		out2rmp(res);
		snprintf(res, sizeof res, "GREEKSHS,%s.%03d,%s%sP%s%s|0.00,%f,%f,%f,%f,%f",
			datestr,
			msec,
			spotname,
			sep,
			sep,
			fields[i],
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_put_delta  (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_put_delta (spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry),
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_put_gamma  (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_put_gamma (spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry),
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_put_theta  (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_put_theta (spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry),
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_put_vega   (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_put_vega  (spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry),
			!strncasecmp(spotname, "IO", 2) || !strncasecmp(spotname, "HO", 2)
				? bs_put_rho    (spot, atof(fields[i]), r, 0, atof(fields[i + 1]), expiry)
				: baw_put_rho   (spot, atof(fields[i]), r, r, atof(fields[i + 1]), expiry));
		out2rmp(res);
	}
	dstr_free(sep);
	dstr_free(spotname);

end:
	dstr_free_tokens(fields, nfield);
	return 0;
}

static int load_module(void) {
	int res;

	if ((res = msgs_hook_name("hsv_msgs", greekshs_exec, NULL)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue 'hsv_msgs' not found");
		return MODULE_LOAD_FAILURE;
	}
	return register_application(app, greekshs_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	msgs_unhook_name("hsv_msgs", greekshs_exec);
	return unregister_application(app);
}

static int reload_module(void) {
	/* FIXME */
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Greeks (Historical) Application");

