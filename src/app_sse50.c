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
#include <strings.h>
#include <time.h>
#include <xcb/macros.h>
#include <xcb/mem.h>
#include <xcb/table.h>
#include <xcb/logger.h>
#include <xcb/config.h>
#include <xcb/module.h>
#include <xcb/utils.h>
#include <xcb/basics.h>

/* FIXME */
struct wp {
	double	weight, price;
};

/* FIXME */
static char *app = "sse50";
static char *desc = "SSE50 Index";
static char *fmt = "SSE50,timestamp,sse50";
static table_t contracts;
static struct config *cfg;
static double divisor = NAN;

static inline void load_config(void) {
	/* FIXME */
	if ((cfg = config_load("/etc/xcb/sse50.conf"))) {
		char *cat = category_browse(cfg, NULL);
		struct variable *var;

		while (cat) {
			if (!strcasecmp(cat, "weights")) {
				var = variable_browse(cfg, cat);
				while (var) {
					struct wp *wp;

					if (NEW(wp)) {
						wp->weight = atof(var->value);
						wp->price  = NAN;
						table_insert(contracts, var->name, wp);
					}
					var = var->next;
				}
			} else if (!strcasecmp(cat, "divisor"))
				if ((var = variable_browse(cfg, cat)) && strcmp(var->value, ""))
					divisor = atof(var->value);
			cat = category_browse(cfg, cat);
		}
	}
}

static int sse50_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	Quote *quote = (Quote *)msg->data;
	struct wp *wp;
	NOT_USED(data2);

	if (isnan(divisor)) {
		xcb_log(XCB_LOG_WARNING, "No valid value for divisor");
		goto end;
	}
	table_lock(contracts);
	if ((wp = table_get_value(contracts, quote->thyquote.m_cHYDM)) &&
		(isnan(wp->price) || fabs(wp->price - quote->thyquote.m_dZXJ) > 0.000001)) {
		table_iter_t iter = table_iter_new(contracts);
		table_node_t node;
		double sum = 0.0;
		time_t t = (time_t)quote->thyquote.m_nTime;
		struct tm lt;
		char datestr[64], res[512];

		wp->price = quote->thyquote.m_dZXJ;
		while ((node = table_next(iter))) {
			wp = table_node_value(node);
			if (isnan(wp->price)) {
				xcb_log(XCB_LOG_WARNING, "No price info for '%s'", table_node_key(node));
				table_iter_free(&iter);
				table_unlock(contracts);
				goto end;
			}
			sum += wp->weight * wp->price;
		}
		table_iter_free(&iter);
		strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
		snprintf(res, sizeof res, "SSE50,%s.%03d|%f",
			datestr,
			quote->m_nMSec,
			(sum / divisor) * 1000);
		out2rmp(res);
	}
	table_unlock(contracts);

end:
	return 0;
}

static void vfree(void *value) {
	struct wp *wp = (struct wp *)value;

	FREE(wp);
}

static int load_module(void) {
	contracts = table_new(cmpstr, hashmurmur2, NULL, vfree);
	load_config();
	if (msgs_hook(&default_msgs, sse50_exec, NULL) == -1)
		return MODULE_LOAD_FAILURE;
	return register_application(app, sse50_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	msgs_unhook(&default_msgs, sse50_exec);
	config_destroy(cfg);
	table_free(&contracts);
	return unregister_application(app);
}

static int reload_module(void) {
	msgs_unhook(&default_msgs, sse50_exec);
	config_destroy(cfg);
	table_clear(contracts);
	load_config();
	if (msgs_hook(&default_msgs, sse50_exec, NULL) == -1)
		return MODULE_LOAD_FAILURE;
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "SSE50 Index Application");

