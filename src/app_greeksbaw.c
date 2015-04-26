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

#include <xcb/fmacros.h>
#include <math.h>
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
#include "baw.h"

/* FIXME */
static char *app = "greeksbaw";
static char *desc = "Delta, Gamma, Theta, Vega, and Rho (BAW)";
static char *fmt = "GREEKSBAW,timestamp,contract,lastprice,delta,gamma,theta,vega,rho,"
	"bidprice1,delta2,gamma2,theta2,vega2,rho2,askprice1,delta3,gamma3,theta3,vega3,rho3";

static int greeksbaw_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	dstr *fields = NULL;
	int nfield = 0;
	time_t t;
	char *contract, *type;
	double spot, strike, r, vol, vol2, vol3, expiry;
	NOT_USED(data2);

	fields = dstr_split_len(msg->data, strlen(msg->data), ",", 1, &nfield);
	/* FIXME */
	if (nfield != 19) {
		xcb_log(XCB_LOG_WARNING, "Message '%s' garbled", msg->data);
		goto end;
	}
	t        = (time_t)atoi(fields[1]);
	contract = fields[3];
	type     = fields[12];
	spot     = atof(fields[10]);
	strike   = atof(fields[13]);
	r        = atof(fields[14]);
	vol      = atof(fields[5]);
	vol2     = atof(fields[7]);
	vol3     = atof(fields[9]);
	expiry   = atof(fields[15]);
	if (!isnan(vol) || !isnan(vol2) || !isnan(vol3)) {
		double delta, gamma, theta, vega, rho;
		double delta2, gamma2, theta2, vega2, rho2;
		double delta3, gamma3, theta3, vega3, rho3;
		struct tm lt;
		char datestr[64], res[512];

		if (isnan(vol))
			delta = gamma = theta = vega = rho = NAN;
		else {
			if (!strcasecmp(type, "C")) {
				delta = baw_call_delta(spot, strike, r, r, vol, expiry);
				gamma = baw_call_gamma(spot, strike, r, r, vol, expiry);
				theta = baw_call_theta(spot, strike, r, r, vol, expiry);
				vega  = baw_call_vega (spot, strike, r, r, vol, expiry);
				rho   = baw_call_rho  (spot, strike, r, r, vol, expiry);
			} else {
				delta = baw_put_delta (spot, strike, r, r, vol, expiry);
				gamma = baw_put_gamma (spot, strike, r, r, vol, expiry);
				theta = baw_put_theta (spot, strike, r, r, vol, expiry);
				vega  = baw_put_vega  (spot, strike, r, r, vol, expiry);
				rho   = baw_put_rho   (spot, strike, r, r, vol, expiry);
			}
		}
		if (isnan(vol2))
			delta2 = gamma2 = theta2 = vega2 = rho2 = NAN;
		else if (fabs(vol2 - vol) <= 0.000001) {
			delta2 = delta;
			gamma2 = gamma;
			theta2 = theta;
			vega2  = vega;
			rho2   = rho;
		} else {
			if (!strcasecmp(type, "C")) {
				delta2 = baw_call_delta(spot, strike, r, r, vol2, expiry);
				gamma2 = baw_call_gamma(spot, strike, r, r, vol2, expiry);
				theta2 = baw_call_theta(spot, strike, r, r, vol2, expiry);
				vega2  = baw_call_vega (spot, strike, r, r, vol2, expiry);
				rho2   = baw_call_rho  (spot, strike, r, r, vol2, expiry);
			} else {
				delta2 = baw_put_delta (spot, strike, r, r, vol2, expiry);
				gamma2 = baw_put_gamma (spot, strike, r, r, vol2, expiry);
				theta2 = baw_put_theta (spot, strike, r, r, vol2, expiry);
				vega2  = baw_put_vega  (spot, strike, r, r, vol2, expiry);
				rho2   = baw_put_rho   (spot, strike, r, r, vol2, expiry);
			}
		}
		if (isnan(vol3))
			delta3 = gamma3 = theta3 = vega3 = rho3 = NAN;
		else if (fabs(vol3 - vol) <= 0.000001) {
			delta3 = delta;
			gamma3 = gamma;
			theta3 = theta;
			vega3  = vega;
			rho3   = rho;
		} else {
			if (!strcasecmp(type, "C")) {
				delta3 = baw_call_delta(spot, strike, r, r, vol3, expiry);
				gamma3 = baw_call_gamma(spot, strike, r, r, vol3, expiry);
				theta3 = baw_call_theta(spot, strike, r, r, vol3, expiry);
				vega3  = baw_call_vega (spot, strike, r, r, vol3, expiry);
				rho3   = baw_call_rho  (spot, strike, r, r, vol3, expiry);
			} else {
				delta3 = baw_put_delta (spot, strike, r, r, vol3, expiry);
				gamma3 = baw_put_gamma (spot, strike, r, r, vol3, expiry);
				theta3 = baw_put_theta (spot, strike, r, r, vol3, expiry);
				vega3  = baw_put_vega  (spot, strike, r, r, vol3, expiry);
				rho3   = baw_put_rho   (spot, strike, r, r, vol3, expiry);
			}
		}
		strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
		snprintf(res, sizeof res, "GREEKSBAW,%s.%03d,%s|%.2f,%f,%f,%f,%f,%f,%.2f,%f,%f,%f,%f,%f,%.2f,%f,%f,%f,%f,%f",
			datestr,
			atoi(fields[2]),
			contract,
			atof(fields[4]),
			delta,
			gamma,
			theta,
			vega,
			rho,
			atof(fields[6]),
			delta2,
			gamma2,
			theta2,
			vega2,
			rho2,
			atof(fields[8]),
			delta3,
			gamma3,
			theta3,
			vega3,
			rho3);
		out2rmp(res);
	}

end:
	dstr_free_tokens(fields, nfield);
	return 0;
}

static int load_module(void) {
	int res;

	if ((res = msgs_hook_name("impvbaw_msgs", greeksbaw_exec, NULL)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue 'impvbaw_msgs' not found");
		return MODULE_LOAD_FAILURE;
	}
	return register_application(app, greeksbaw_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	msgs_unhook_name("impvbaw_msgs", greeksbaw_exec);
	return unregister_application(app);
}

static int reload_module(void) {
	/* FIXME */
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Greeks (BAW) Application");

