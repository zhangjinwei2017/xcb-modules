/*
 * Copyright (c) 2013-2015, Dalian Futures Information Technology Co., Ltd.
 *
 * Bo Wang     <futurewb at dce dot com dot cn>
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
#include <xcb/dlist.h>
#include <xcb/table.h>
#include <xcb/dstr.h>
#include <xcb/logger.h>
#include <xcb/config.h>
#include <xcb/module.h>
#include <xcb/utilities.h>
#include <xcb/basics.h>

/* FIXME */
struct sd {
	double	hsv;
	double	spot;
	double	expiry;
	char	*sep;
	dlist_t	dlist;
};

/* FIXME */
static char *app = "hsv";
static char *desc = "Historical Volatility";
static char *fmt = "HSV,timestamp,contract,hsv,spotprice";
static table_t spots;
static struct msgs *hsv_msgs;
static struct config *cfg;
static double r = 0.033;

static void vfree(void *value) {
	dstr_free((dstr)value);
}

static inline void load_config(void) {
	/* FIXME */
	if ((cfg = config_load("/etc/xcb/hsv.conf"))) {
		char *cat = category_browse(cfg, NULL);
		struct variable *var;
		struct sd *sd;

		while (cat) {
			if (!strcasecmp(cat, "general")) {
				var = variable_browse(cfg, cat);
				while (var) {
					if (!strcasecmp(var->name, "rate")) {
						if (strcmp(var->value, ""))
							r = atof(var->value);
					} else
						xcb_log(XCB_LOG_WARNING, "Unknown variable '%s' in "
							"category '%s' of impv.conf", var->name, cat);
					var = var->next;
				}
			} else if (!strcasecmp(cat, "expiries")) {
				var = variable_browse(cfg, cat);
				while (var) {
					char *p;

					if ((p = strrchr(var->name, 'C')) == NULL)
						p = strrchr(var->name, 'P');
					if (p && p != var->name && p != var->name + strlen(var->name) - 1 &&
						((*(p - 1) == '-' && *(p + 1) == '-') ||
						(isdigit(*(p - 1)) && isdigit(*(p + 1))))) {
						dstr spotname, strike;

						spotname = *(p - 1) == '-'
							? dstr_new_len(var->name, p - var->name - 1)
							: dstr_new_len(var->name, p - var->name);
						strike   = *(p + 1) == '-'
							? dstr_new_len(p + 2, var->name + strlen(var->name) - p - 2)
							: dstr_new_len(p + 1, var->name + strlen(var->name) - p - 1);
						if ((sd = table_get_value(spots, spotname)) == NULL) {
							if (NEW(sd)) {
								sd->hsv    = NAN;
								sd->spot   = NAN;
								sd->expiry = diffnow(atoi(var->value)) / 252.0;
								sd->sep    = *(p - 1) == '-' ? "-" : "";
								sd->dlist  = dlist_new(cmpstr, vfree);
								dlist_insert_tail(sd->dlist, strike);
								table_insert(spots, spotname, sd);
							} else {
								dstr_free(strike);
								dstr_free(spotname);
							}
						} else {
							dlist_node_t node;

							sd->expiry = diffnow(atoi(var->value)) / 252.0;
							sd->sep    = *(p - 1) == '-' ? "-" : "";
							if ((node = dlist_find(sd->dlist, strike)) == NULL)
								dlist_insert_sort(sd->dlist, strike);
							else
								dstr_free(strike);
							dstr_free(spotname);
						}
					}
					var = var->next;
				}
			/* FIXME */
			} else if (!strcasecmp(cat, "hsves")) {
				var = variable_browse(cfg, cat);
				while (var) {
					if ((sd = table_get_value(spots, var->name)) == NULL) {
						if (NEW(sd)) {
							sd->hsv    = atof(var->value);
							sd->spot   = NAN;
							sd->expiry = NAN;
							sd->sep    = NULL;
							sd->dlist  = dlist_new(cmpstr, vfree);
							table_insert(spots, dstr_new(var->name), sd);
						}
					} else
						sd->hsv = atof(var->value);
					var = var->next;
				}
			}
			cat = category_browse(cfg, cat);
		}
	}
}

static int hsv_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	Quote *quote = (Quote *)msg->data;
	struct msgs *out = (struct msgs *)data2;
	char *contract, *p;

	contract = quote->thyquote.m_cHYDM;
	if (!strcmp(contract, "") || fabs(quote->thyquote.m_dZXJ) <= 0.000001) {
		xcb_log(XCB_LOG_WARNING, "Invalid quote: '%d,%d,%s,%.2f,%.2f,%.2f'",
			quote->thyquote.m_nTime,
			quote->m_nMSec,
			contract,
			quote->thyquote.m_dZXJ,
			quote->thyquote.m_dMRJG1,
			quote->thyquote.m_dMCJG1);
		goto end;
	}
	if ((p = strrchr(contract, 'C')) == NULL)
		p = strrchr(contract, 'P');
	if (p && p != contract && p != contract + strlen(contract) - 1 &&
		((*(p - 1) == '-' && *(p + 1) == '-') || (isdigit(*(p - 1)) && isdigit(*(p + 1))))) {
		/* do nothing */
	} else {
		table_node_t tnode;

		table_lock(spots);
		if ((tnode = table_find(spots, contract))) {
			struct sd *sd = (struct sd *)table_node_value(tnode);

			if (!isnan(sd->hsv) && (isnan(sd->spot) ||
				fabs(quote->thyquote.m_dZXJ - sd->spot) > 0.000001)) {
				char *res;

				sd->spot = quote->thyquote.m_dZXJ;
				if ((res = ALLOC(2048))) {
					time_t t = (time_t)quote->thyquote.m_nTime;
					struct tm lt;
					char datestr[64];
					int off;
					dlist_iter_t iter = dlist_iter_new(sd->dlist, DLIST_START_HEAD);
					dlist_node_t node;

					strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
					snprintf(res, 2048, "HSV,%s.%03d,%s|%f,%.2f",
						datestr,
						quote->m_nMSec,
						contract,
						sd->hsv,
						sd->spot);
					out2rmp(res);
					off = snprintf(res, 2048, "HSV,%d,%d,%s,",
						quote->thyquote.m_nTime,
						quote->m_nMSec,
						contract);
					while ((node = dlist_next(iter)))
						off += snprintf(res + off, 2048 - off, "%s,%f,",
							(dstr)dlist_node_value(node),
							sd->hsv);
					dlist_iter_free(&iter);
					snprintf(res + off, 2048 - off, "%.2f,%f,%f,%s",
						sd->spot,
						r,
						sd->expiry,
						sd->sep);
					if (out2msgs(res, out) == -1)
						FREE(res);
				}
			}
		}
		table_unlock(spots);
	}

end:
	return 0;
}

static void kfree(const void *key) {
	dstr_free((dstr)key);
}

static void sdfree(void *value) {
	struct sd *sd = (struct sd *)value;

	dlist_free(&sd->dlist);
	FREE(sd);
}

static int load_module(void) {
	spots = table_new(cmpstr, hashmurmur2, kfree, sdfree);
	load_config();
	if (msgs_init(&hsv_msgs, "hsv_msgs", mod_info->self) == -1)
		return MODULE_LOAD_FAILURE;
	if (start_msgs(hsv_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	if (msgs_hook(&default_msgs, hsv_exec, hsv_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	return register_application(app, hsv_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	if (check_msgs(hsv_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	msgs_unhook(&default_msgs, hsv_exec);
	stop_msgs(hsv_msgs);
	msgs_free(hsv_msgs);
	config_destroy(cfg);
	table_free(&spots);
	return unregister_application(app);
}

static int reload_module(void) {
	msgs_unhook(&default_msgs, hsv_exec);
	config_destroy(cfg);
	table_clear(spots);
	load_config();
	if (msgs_hook(&default_msgs, hsv_exec, hsv_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Historical Volatility Application");

