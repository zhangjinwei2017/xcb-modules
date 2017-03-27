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
#include "trinomial.h"

/* FIXME */
static char *app = "greeks3";
static char *desc = "Delta, Gamma, Theta, Vega, and Rho (Trinomial)";
static char *fmt = "GREEKS3,timestamp,contract,lastprice,delta,gamma,theta,vega,rho,"
	"bidprice1,delta2,gamma2,theta2,vega2,rho2,askprice1,delta3,gamma3,theta3,vega3,rho3,"
	"avgprice,delta4,gamma4,theta4,vega4,rho4";

static int greeks3_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	dstr *fields = NULL;
	int nfield = 0;
	time_t t;
	char *contract, *type;
	double spot, strike, r, vol, vol2, vol3, vol4, expiry;
	int steps;
	NOT_USED(data2);

	fields = dstr_split_len(msg->data, strlen(msg->data), ",", 1, &nfield);
	/* FIXME */
	if (nfield != 21) {
		xcb_log(XCB_LOG_WARNING, "Message '%s' garbled", msg->data);
		goto end;
	}
	t        = (time_t)atoi(fields[1]);
	contract = fields[3];
	type     = fields[14];
	spot     = atof(fields[12]);
	strike   = atof(fields[15]);
	r        = atof(fields[16]);
	vol      = atof(fields[5]);
	vol2     = atof(fields[7]);
	vol3     = atof(fields[9]);
	vol4     = atof(fields[11]);
	expiry   = atof(fields[17]);
	steps    = atoi(fields[20]);
	if (!isnan(vol) || !isnan(vol2) || !isnan(vol3) || !isnan(vol4)) {
		double delta, gamma, theta, vega, rho;
		double delta2, gamma2, theta2, vega2, rho2;
		double delta3, gamma3, theta3, vega3, rho3;
		double delta4, gamma4, theta4, vega4, rho4;
		struct tm lt;
		char datestr[64], res[512];

		if (isnan(vol))
			delta = gamma = theta = vega = rho = NAN;
		else {
			if (!strcasecmp(type, "C"))
				tri_amer_call_greeks(spot, strike, r, r, vol, expiry, steps,
					&delta, &gamma, &theta, &vega, &rho);
			else
				tri_amer_put_greeks (spot, strike, r, r, vol, expiry, steps,
					&delta, &gamma, &theta, &vega, &rho);
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
			if (!strcasecmp(type, "C"))
				tri_amer_call_greeks(spot, strike, r, r, vol2, expiry, steps,
					&delta2, &gamma2, &theta2, &vega2, &rho2);
			else
				tri_amer_put_greeks (spot, strike, r, r, vol2, expiry, steps,
					&delta2, &gamma2, &theta2, &vega2, &rho2);
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
			if (!strcasecmp(type, "C"))
				tri_amer_call_greeks(spot, strike, r, r, vol3, expiry, steps,
					&delta3, &gamma3, &theta3, &vega3, &rho3);
			else
				tri_amer_put_greeks (spot, strike, r, r, vol3, expiry, steps,
					&delta3, &gamma3, &theta3, &vega3, &rho3);
		}
		if (isnan(vol4))
			delta4 = gamma4 = theta4 = vega4 = rho4 = NAN;
		else if (fabs(vol4 - vol) <= 0.000001) {
			delta4 = delta;
			gamma4 = gamma;
			theta4 = theta;
			vega4  = vega;
			rho4   = rho;
		} else {
			if (!strcasecmp(type, "C"))
				tri_amer_call_greeks(spot, strike, r, r, vol4, expiry, steps,
					&delta4, &gamma4, &theta4, &vega4, &rho4);
			else
				tri_amer_put_greeks (spot, strike, r, r, vol4, expiry, steps,
					&delta4, &gamma4, &theta4, &vega4, &rho4);
		}
		strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
		snprintf(res, sizeof res, "GREEKS3,%s.%03d,%s|%.4f,%f,%f,%f,%f,%f,%.4f,%f,%f,%f,%f,%f,"
			"%.4f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
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
			rho3,
			atof(fields[10]),
			delta4,
			gamma4,
			theta4,
			vega4,
			rho4);
		out2rmp(res);
	}

end:
	dstr_free_tokens(fields, nfield);
	return 0;
}

static int load_module(void) {
	int res;

	if ((res = msgs_hook_name("impv3_msgs", greeks3_exec, NULL)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue 'impv3_msgs' not found");
		return MODULE_LOAD_FAILURE;
	}
	return register_application(app, greeks3_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	msgs_unhook_name("impv3_msgs", greeks3_exec);
	return unregister_application(app);
}

static int reload_module(void) {
	/* FIXME */
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Greeks (Trinomial) Application");

