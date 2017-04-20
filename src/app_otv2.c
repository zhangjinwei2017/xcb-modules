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

#include <xcb/fmacros.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <xcb/macros.h>
#include <xcb/dstr.h>
#include <xcb/logger.h>
#include <xcb/config.h>
#include <xcb/module.h>
#include <xcb/basics.h>
#include "binomial.h"

/* FIXME */
static char *app = "otv2";
static char *desc = "Option Theoretical Value (Binomial)";
static char *fmt = "OTV2,timestamp,contract,otv,otv2,otv3";
static struct config *cfg;
static const char *inmsg = "vsml_msgs";

static inline void load_config(void) {
	/* FIXME */
	if ((cfg = config_load("/etc/xcb/otv2.conf"))) {
		char *cat = category_browse(cfg, NULL);

		while (cat) {
			if (!strcasecmp(cat, "general")) {
				struct variable *var = variable_browse(cfg, cat);

				while (var) {
					if (!strcasecmp(var->name, "inmsg")) {
						if (strcmp(var->value, ""))
							inmsg = var->value;
					} else
						xcb_log(XCB_LOG_WARNING, "Unknown variable '%s' in "
							"category '%s' of otv2.conf", var->name, cat);
					var = var->next;
				}
			}
			cat = category_browse(cfg, cat);
		}
	}
}

static int otv2_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	dstr *fields = NULL;
	int nfield = 0;
	time_t t;
	struct tm lt;
	int msec, i, steps;
	char *spotname, *sep;
	double spot, r, expiry;
	char datestr[64];
	NOT_USED(data2);

	fields = dstr_split_len(msg->data, strlen(msg->data), ",", 1, &nfield);
	/* FIXME */
	if (nfield < 14) {
		xcb_log(XCB_LOG_WARNING, "Message '%s' garbled", msg->data);
		goto end;
	}
	t        = (time_t)atoi(fields[1]);
	msec     = atoi(fields[2]);
	spotname = fields[3];
	spot     = atof(fields[nfield - 6]);
	r        = atof(fields[nfield - 5]);
	expiry   = atof(fields[nfield - 4]);
	sep      = fields[nfield - 3];
	steps    = atoi(fields[nfield - 1]);
	strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
	for (i = 4; i < nfield - 6; i += 4) {
		double strike, call, put;
		char res[512];

		strike = atof(fields[i]);
		call   = !strcasecmp(fields[i + 1], "nan") || atof(fields[i + 1]) < 0.0
			? NAN : bi_amer_call(spot, strike, r, r, atof(fields[i + 1]), expiry, steps);
		put    = !strcasecmp(fields[i + 1], "nan") || atof(fields[i + 1]) < 0.0
			? NAN : bi_amer_put (spot, strike, r, r, atof(fields[i + 1]), expiry, steps);
		snprintf(res, sizeof res, "OTV2,%s.%03d,%s%sC%s%s|%f,%f,%f",
			datestr,
			msec,
			spotname,
			sep,
			sep,
			fields[i],
			/* FIXME */
			call,
			!strcasecmp(fields[i + 2], "nan") || atof(fields[i + 2]) < 0.0
				? NAN
				: (fabs(atof(fields[i + 2]) - atof(fields[i + 1])) <= 0.000001
					? call
					: bi_amer_call(spot, strike, r, r, atof(fields[i + 2]),
						expiry, steps)),
			!strcasecmp(fields[i + 3], "nan") || atof(fields[i + 3]) < 0.0
				? NAN
				: (fabs(atof(fields[i + 3]) - atof(fields[i + 1])) <= 0.000001
					? call
					: bi_amer_call(spot, strike, r, r, atof(fields[i + 3]),
						expiry, steps)));
		out2rmp(res);
		snprintf(res, sizeof res, "OTV2,%s.%03d,%s%sP%s%s|%f,%f,%f",
			datestr,
			msec,
			spotname,
			sep,
			sep,
			fields[i],
			/* FIXME */
			put,
			!strcasecmp(fields[i + 2], "nan") || atof(fields[i + 2]) < 0.0
				? NAN
				: (fabs(atof(fields[i + 2]) - atof(fields[i + 1])) <= 0.000001
					? put
					: bi_amer_put (spot, strike, r, r, atof(fields[i + 2]),
						expiry, steps)),
			!strcasecmp(fields[i + 3], "nan") || atof(fields[i + 3]) < 0.0
				? NAN
				: (fabs(atof(fields[i + 3]) - atof(fields[i + 1])) <= 0.000001
					? put
					: bi_amer_put (spot, strike, r, r, atof(fields[i + 3]),
						expiry, steps)));
		out2rmp(res);
	}

end:
	dstr_free_tokens(fields, nfield);
	return 0;
}

static int load_module(void) {
	int res;

	load_config();
	if ((res = msgs_hook_name(inmsg, otv2_exec, NULL)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue '%s' not found", inmsg);
		return MODULE_LOAD_FAILURE;
	}
	return register_application(app, otv2_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	msgs_unhook_name(inmsg, otv2_exec);
	return unregister_application(app);
}

static int reload_module(void) {
	int res;

	msgs_unhook_name(inmsg, otv2_exec);
	load_config();
	if ((res = msgs_hook_name(inmsg, otv2_exec, NULL)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue '%s' not found", inmsg);
		return MODULE_LOAD_FAILURE;
	}
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Option Theoretical Value (Binomial) Application");

