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
#include <sys/time.h>
#include <time.h>
#include <xcb/macros.h>
#include <xcb/mem.h>
#include <xcb/table.h>
#include <xcb/dstr.h>
#include <xcb/logger.h>
#include <xcb/config.h>
#include <xcb/module.h>
#include <xcb/utilities.h>
#include <xcb/basics.h>
#include "impv.h"

/* FIXME */
struct prices {
	float	prelast, prebid1, preask1, prespot;
};

/* FIXME */
static char *app = "impv2";
static char *desc = "Implied Volatility (Binomial)";
static char *fmt = "IMPV2,timestamp,contract,lastprice,vol,bidprice1,vol2,askprice1,vol3,spotprice";
static table_t spots;
static table_t optns;
static table_t expiries;
static struct msgs *impv2_msgs;
static struct config *cfg;
static double r = 0.033;
static int steps = 4096;

static inline void load_config(void) {
	/* FIXME */
	if ((cfg = config_load("/etc/xcb/impv2.conf"))) {
		char *cat = category_browse(cfg, NULL);
		struct variable *var;

		while (cat) {
			if (!strcasecmp(cat, "general")) {
				var = variable_browse(cfg, cat);
				while (var) {
					if (!strcasecmp(var->name, "rate")) {
						if (strcmp(var->value, ""))
							r = atof(var->value);
					} else if (!strcasecmp(var->name, "steps")) {
						if (strcmp(var->value, ""))
							steps = atoi(var->value);
					} else
						xcb_log(XCB_LOG_WARNING, "Unknown variable '%s' in "
							"category '%s' of impv2.conf", var->name, cat);
					var = var->next;
				}
			} else if (!strcasecmp(cat, "expiries")) {
				table_node_t node;

				var = variable_browse(cfg, cat);
				while (var) {
					/* FIXME */
					if ((node = table_insert_raw(expiries, var->name)))
						table_set_double(node, diffnow(atoi(var->value)) / 252.0);
					var = var->next;
				}
			}
			cat = category_browse(cfg, cat);
		}
	}
}

static int impv2_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	Quote *quote = (Quote *)msg->data;
	struct msgs *out = (struct msgs *)data2;
	dstr contract;
	float last;
	char *p;
	table_node_t node;

	contract = dstr_new(quote->thyquote.m_cHYDM);
	if (!strcmp(contract, "") || (fabs(quote->thyquote.m_dZXJ) <= 0.000001 &&
		fabs(quote->thyquote.m_dMRJG1) <= 0.000001 &&
		fabs(quote->thyquote.m_dMCJG1) <= 0.000001)) {
		xcb_log(XCB_LOG_WARNING, "Invalid quote: '%d,%d,%s,%.2f,%.2f,%.2f'",
			quote->thyquote.m_nTime,
			quote->m_nMSec,
			contract,
			quote->thyquote.m_dZXJ,
			quote->thyquote.m_dMRJG1,
			quote->thyquote.m_dMCJG1);
		goto end;
	}
	/* FIXME */
	if (!strncasecmp(contract, "IO", 2) || !strncasecmp(contract, "HO", 2) ||
		!strncasecmp(contract, "00", 2) || !strncasecmp(contract, "60", 2))
		goto end;
	last = quote->thyquote.m_dZXJ;
	if ((p = strrchr(contract, 'C')) == NULL)
		p = strrchr(contract, 'P');
	/* FIXME: option quote */
	if (p && p != contract && p != contract + dstr_length(contract) - 1 &&
		((*(p - 1) == '-' && *(p + 1) == '-') || (isdigit(*(p - 1)) && isdigit(*(p + 1))))) {
		dstr spotname, type, strike;
		float spot;
		struct prices *prices;
		double expiry, vol, vol2, vol3;
		struct tm lt;
		char *res;

		spotname = *(p - 1) == '-' ? dstr_new_len(contract, p - contract - 1) :
			dstr_new_len(contract, p - contract);
		type     = dstr_new_len(p, 1);
		strike   = *(p + 1) == '-' ? dstr_new_len(p + 2, contract + dstr_length(contract) - p - 2) :
			dstr_new_len(p + 1, contract + dstr_length(contract) - p - 1);
		table_rwlock_rdlock(spots);
		if ((node = table_find(spots, spotname)) == NULL) {
			table_rwlock_unlock(spots);
			dstr_free(strike);
			dstr_free(type);
			dstr_free(spotname);
			goto end;
		}
		spot = table_node_float(node);
		table_rwlock_unlock(spots);
		if (fabs(spot) <= 0.000001) {
			xcb_log(XCB_LOG_WARNING, "The price of spot '%s' be zero", spotname);
			dstr_free(strike);
			dstr_free(type);
			dstr_free(spotname);
			goto end;
		}
		table_lock(optns);
		if ((node = table_find(optns, contract)) == NULL) {
			if (NEW(prices)) {
				prices->prelast = last;
				prices->prebid1 = quote->thyquote.m_dMRJG1;
				prices->preask1 = quote->thyquote.m_dMCJG1;
				prices->prespot = spot;
				table_insert(optns, dstr_new(contract), prices);
			} else
				xcb_log(XCB_LOG_WARNING, "Error allocating memory for prices");
		} else {
			prices = (struct prices *)table_node_value(node);

			if (fabs(prices->prelast - last) <= 0.000001 &&
				fabs(prices->prebid1 - quote->thyquote.m_dMRJG1) <= 0.000001 &&
				fabs(prices->preask1 - quote->thyquote.m_dMCJG1) <= 0.000001 &&
				fabs(prices->prespot - spot) <= 0.000001) {
				table_unlock(optns);
				dstr_free(strike);
				dstr_free(type);
				dstr_free(spotname);
				goto end;
			} else {
				prices->prelast = last;
				prices->prebid1 = quote->thyquote.m_dMRJG1;
				prices->preask1 = quote->thyquote.m_dMCJG1;
				prices->prespot = spot;
			}
		}
		table_unlock(optns);
		if ((node = table_find(expiries, contract)))
			expiry = table_node_double(node);
		else {
			char *month;
			struct timeval tv;
			int diff;

			xcb_log(XCB_LOG_WARNING, "No exact expiry date for '%s'", contract);
			month = spotname + dstr_length(spotname) - 2;
			gettimeofday(&tv, NULL);
			if ((diff = atoi(month) - localtime_r(&tv.tv_sec, &lt)->tm_mon - 1) < 0)
				diff += 12;
			if (diff == 0)
				diff = 1;
			expiry = diff / 12.0;
		}
		/* FIXME: last price */
		if (fabs(last) <= 0.000001)
			vol = NAN;
		else
			vol = impv_binomial(spot, atof(strike), r, r, expiry, steps, last,
				!strcasecmp(type, "C") ? AMER_CALL : AMER_PUT);
		/* FIXME: bid price 1 */
		if (fabs(quote->thyquote.m_dMRJG1) <= 0.000001)
			vol2 = NAN;
		else if (fabs(quote->thyquote.m_dMRJG1 - last) <= 0.000001)
			vol2 = vol;
		else
			vol2 = impv_binomial(spot, atof(strike), r, r, expiry, steps, quote->thyquote.m_dMRJG1,
				!strcasecmp(type, "C") ? AMER_CALL : AMER_PUT);
		/* FIXME: ask price 1 */
		if (fabs(quote->thyquote.m_dMCJG1) <= 0.000001)
			vol3 = NAN;
		else if (fabs(quote->thyquote.m_dMCJG1 - last) <= 0.000001)
			vol3 = vol;
		else
			vol3 = impv_binomial(spot, atof(strike), r, r, expiry, steps, quote->thyquote.m_dMCJG1,
				!strcasecmp(type, "C") ? AMER_CALL : AMER_PUT);
		if ((res = ALLOC(512))) {
			time_t t = (time_t)quote->thyquote.m_nTime;
			char datestr[64];

			strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
			snprintf(res, 512, "IMPV2,%s.%03d,%s|%.2f,%f,%.2f,%f,%.2f,%f,%.2f",
				datestr,
				quote->m_nMSec,
				contract,
				last,
				vol,
				quote->thyquote.m_dMRJG1,
				vol2,
				quote->thyquote.m_dMCJG1,
				vol3,
				spot);
			out2rmp(res);
			snprintf(res, 512, "IMPV2,%d,%d,%s,%.2f,%f,%.2f,%f,%.2f,%f,%.2f,%s,%s,%s,%f,%f,%d,0,%d",
				quote->thyquote.m_nTime,
				quote->m_nMSec,
				contract,
				last,
				vol,
				quote->thyquote.m_dMRJG1,
				vol2,
				quote->thyquote.m_dMCJG1,
				vol3,
				spot,
				spotname,
				type,
				strike,
				r,
				expiry,
				quote->thyquote.m_nCJSL,
				steps);
			if (out2msgs(res, out) == -1)
				FREE(res);
		} else
			xcb_log(XCB_LOG_WARNING, "Error allocating memory for result");
		dstr_free(strike);
		dstr_free(type);
		dstr_free(spotname);
	/* future quote */
	} else {
		table_rwlock_wrlock(spots);
		if ((node = table_find(spots, contract)) == NULL) {
			if ((node = table_insert_raw(spots, contract)))
				table_set_float(node, last);
		} else {
			table_set_float(node, last);
			dstr_free(contract);
		}
		table_rwlock_unlock(spots);
		return 0;
	}

end:
	dstr_free(contract);
	return 0;
}

static void kfree(const void *key) {
	dstr_free((dstr)key);
}

static void vfree(void *value) {
	FREE(value);
}

static int load_module(void) {
	spots    = table_new(cmpstr, hashmurmur2, kfree, NULL);
	optns    = table_new(cmpstr, hashmurmur2, kfree, vfree);
	expiries = table_new(cmpstr, hashmurmur2, NULL,  NULL);
	load_config();
	if (msgs_init(&impv2_msgs, "impv2_msgs", mod_info->self) == -1)
		return MODULE_LOAD_FAILURE;
	if (start_msgs(impv2_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	if (msgs_hook(&default_msgs, impv2_exec, impv2_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	return register_application(app, impv2_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	if (check_msgs(impv2_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	msgs_unhook(&default_msgs, impv2_exec);
	stop_msgs(impv2_msgs);
	msgs_free(impv2_msgs);
	config_destroy(cfg);
	table_free(&expiries);
	table_free(&optns);
	table_free(&spots);
	return unregister_application(app);
}

static int reload_module(void) {
	msgs_unhook(&default_msgs, impv2_exec);
	config_destroy(cfg);
	table_clear(expiries);
	table_clear(optns);
	table_clear(spots);
	load_config();
	if (msgs_hook(&default_msgs, impv2_exec, impv2_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Implied Volatility (Binomial) Application");

