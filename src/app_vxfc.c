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
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <xcb/macros.h>
#include <xcb/mem.h>
#include <xcb/dlist.h>
#include <xcb/table.h>
#include <xcb/dstr.h>
#include <xcb/logger.h>
#include <xcb/config.h>
#include <xcb/module.h>
#include <xcb/module.h>
#include <xcb/utilities.h>
#include <xcb/basics.h>

/* FIXME */
struct scp {
	double	strike, cbid, cask, pbid, pask;
	int	expiry;
};

/* FIXME */
static char *app = "vxfc";
static char *desc = "Volatility Index FC";
static char *fmt = "VXFC,timestamp,vxfc";
static table_t spots;
static struct config *cfg;
static double r = 0.033;
static const char *contract1, *contract2;
static double T1 = NAN, sigma1 = NAN, T2 = NAN, sigma2 = NAN;

static void scpfree(void *value) {
	FREE(value);
}

static inline void load_config(void) {
	/* FIXME */
	if ((cfg = config_load("/etc/xcb/vxfc.conf"))) {
		char *cat = category_browse(cfg, NULL);
		struct variable *var;

		while (cat) {
			if (!strcasecmp(cat, "general")) {
				struct variable *var = variable_browse(cfg, cat);

				while (var) {
					if (!strcasecmp(var->name, "rate")) {
						if (strcmp(var->value, ""))
							r = atof(var->value);
					} else if (!strcasecmp(var->name, "contract1")) {
						if (strcmp(var->value, ""))
							contract1 = var->value;
					} else if (!strcasecmp(var->name, "contract2")) {
						if (strcmp(var->value, ""))
							contract2 = var->value;
					} else
						xcb_log(XCB_LOG_WARNING, "Unknown variable '%s' in "
							"category '%s' of vxfc.conf", var->name, cat);
					var = var->next;
				}
			} else if (!strcasecmp(cat, "expiries")) {
				var = variable_browse(cfg, cat);
				while (var) {
					char *p;

					if (memcmp(contract1, var->name, strlen(contract1)) &&
						memcmp(contract2, var->name, strlen(contract2)))
						goto end;
					if ((p = strrchr(var->name, 'C')) == NULL)
						p = strrchr(var->name, 'P');
					if (p && p != var->name && p != var->name + strlen(var->name) - 1 &&
						((*(p - 1) == '-' && *(p + 1) == '-') ||
						(isdigit(*(p - 1)) && isdigit(*(p + 1))))) {
						dstr spotname, strike;
						dlist_t dlist;
						struct scp *scp;

						spotname = *(p - 1) == '-'
							? dstr_new_len(var->name, p - var->name - 1)
							: dstr_new_len(var->name, p - var->name);
						strike   = *(p + 1) == '-'
							? dstr_new_len(p + 2, var->name + strlen(var->name) - p - 2)
							: dstr_new_len(p + 1, var->name + strlen(var->name) - p - 1);
						if ((dlist = table_get_value(spots, spotname)) == NULL) {
							if (NEW(scp)) {
								scp->strike = atof(strike);
								scp->cbid   = scp->cask = NAN;
								scp->pbid   = scp->pask = NAN;
								scp->expiry = atoi(var->value);
								dlist = dlist_new(NULL, scpfree);
								dlist_insert_tail(dlist, scp);
								table_insert(spots, spotname, dlist);
							} else
								dstr_free(spotname);
						} else {
							dlist_iter_t iter = dlist_iter_new(dlist, DLIST_START_HEAD);
							dlist_node_t node;

							while ((node = dlist_next(iter))) {
								scp = (struct scp *)dlist_node_value(node);
								if (scp->strike > atof(strike) ||
									fabs(scp->strike - atof(strike)) <= 0.000001)
									break;
							}
							dlist_iter_free(&iter);
							if (node == NULL || scp->strike > atof(strike)) {
								if (NEW(scp)) {
									scp->strike = atof(strike);
									scp->cbid   = scp->cask = NAN;
									scp->pbid   = scp->pask = NAN;
									scp->expiry = atoi(var->value);
								}
								if (node == NULL)
									dlist_insert_tail(dlist, scp);
								else
									dlist_insert(dlist, node, scp, 0);
							}
							dstr_free(spotname);
						}
						dstr_free(strike);
					}

end:
					var = var->next;
				}
			}
			cat = category_browse(cfg, cat);
		}
	}
}

/* FIXME */
static double compute_T(int expiry) {
	time_t t = time(NULL);
	struct tm le;

	localtime_r(&t, &le);
	le.tm_mday = expiry % 100;
	le.tm_mon  = expiry / 100 % 100 - 1;
	le.tm_year = expiry / 10000 - 1900;
	return ((24 - le.tm_hour - 1) * 60 + le.tm_min +
		difftime(mktime(&le), t) / 60.0 - 1440 + 900) / 525600;
}

static int vxfc_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	Quote *quote = (Quote *)msg->data;
	dstr contract;
	int flag;
	char *p;
	NOT_USED(data2);

	contract = dstr_new(quote->thyquote.m_cHYDM);
	if (!memcmp(contract1, contract, strlen(contract1)))
		flag = 1;
	else if (!memcmp(contract2, contract, strlen(contract2)))
		flag = 2;
	else
		goto end;
	if ((p = strrchr(contract, 'C')) == NULL)
		p = strrchr(contract, 'P');
	/* FIXME: option quote */
	if (p && p != contract && p != contract + dstr_length(contract) - 1 &&
		((*(p - 1) == '-' && *(p + 1) == '-') || (isdigit(*(p - 1)) && isdigit(*(p + 1))))) {
		dstr spotname, type, strike;
		dlist_t dlist;
		struct scp *scp;

		spotname = *(p - 1) == '-' ? dstr_new_len(contract, p - contract - 1) :
			dstr_new_len(contract, p - contract);
		type     = dstr_new_len(p, 1);
		strike   = *(p + 1) == '-' ? dstr_new_len(p + 2, contract + dstr_length(contract) - p - 2) :
			dstr_new_len(p + 1, contract + dstr_length(contract) - p - 1);
		table_lock(spots);
		if ((dlist = table_get_value(spots, spotname)) == NULL) {
			/* can't happen */
			if (NEW(scp) == NULL) {
				xcb_log(XCB_LOG_WARNING, "Error allocating memory for scp");
				dstr_free(spotname);
				goto err;
			}
			scp->strike = atof(strike);
			if (!strcasecmp(type, "C")) {
				scp->cbid = quote->thyquote.m_dMRJG1;
				scp->cask = quote->thyquote.m_dMCJG1;
				scp->pbid = NAN;
				scp->pask = NAN;
			} else {
				scp->cbid = NAN;
				scp->cask = NAN;
				scp->pbid = quote->thyquote.m_dMRJG1;
				scp->pask = quote->thyquote.m_dMCJG1;
			}
			dlist = dlist_new(NULL, scpfree);
			dlist_insert_tail(dlist, scp);
			table_insert(spots, spotname, dlist);
		} else {
			dlist_iter_t iter = dlist_iter_new(dlist, DLIST_START_HEAD);
			dlist_node_t node, node0 = NULL, prev, next;
			double mindiff = NAN, K0 = NAN, T, F, deltaK, QK, sum;
			struct scp *scp2, *scp3;

			dstr_free(spotname);
			while ((node = dlist_next(iter))) {
				scp = (struct scp *)dlist_node_value(node);
				if (scp->strike > atof(strike) || fabs(scp->strike - atof(strike)) <= 0.000001)
					break;
			}
			if (node && fabs(scp->strike - atof(strike)) <= 0.000001) {
				if (!strcasecmp(type, "C")) {
					scp->cbid = quote->thyquote.m_dMRJG1;
					scp->cask = quote->thyquote.m_dMCJG1;
				} else {
					scp->pbid = quote->thyquote.m_dMRJG1;
					scp->pask = quote->thyquote.m_dMCJG1;
				}
			} else {
				/* can't happen */
				if (NEW(scp) == NULL) {
					xcb_log(XCB_LOG_WARNING, "Error allocating memory for scp");
					goto err;
				}
				scp->strike = atof(strike);
				if (!strcasecmp(type, "C")) {
					scp->cbid = quote->thyquote.m_dMRJG1;
					scp->cask = quote->thyquote.m_dMCJG1;
					scp->pbid = NAN;
					scp->pask = NAN;
				} else {
					scp->cbid = NAN;
					scp->cask = NAN;
					scp->pbid = quote->thyquote.m_dMRJG1;
					scp->pask = quote->thyquote.m_dMCJG1;
				}
				if (node == NULL)
					dlist_insert_tail(dlist, scp);
				else
					dlist_insert(dlist, node, scp, 0);
			}
			/* determine F */
			dlist_iter_rewind_head(iter, dlist);
			while ((node = dlist_next(iter))) {
				scp = (struct scp *)dlist_node_value(node);
				if (!isnan(scp->cbid) && !isnan(scp->cask) &&
					!isnan(scp->pbid) && !isnan(scp->pask)) {
					double diff = (scp->cbid + scp->cask) / 2 -
						(scp->pbid + scp->pask) / 2;

					if (isnan(mindiff) || fabs(mindiff) > fabs(diff)) {
						mindiff = diff;
						K0      = scp->strike;
						node0   = node;
					}
				}
			}
			dlist_iter_free(&iter);
			if (isnan(mindiff))
				goto err;
			T = compute_T(scp->expiry);
			F = K0 + exp(r * T) * mindiff;
			/* determine K0 */
			while (K0 >= F) {
				if ((node0 = dlist_node_prev(node0)) == NULL)
					goto err;
				scp = (struct scp *)dlist_node_value(node0);
				K0  = scp->strike;
			}
			while (K0 < F) {
				if ((node = dlist_node_next(node0)) == NULL)
					break;
				scp = (struct scp *)dlist_node_value(node);
				if (scp->strike < F) {
					K0    = scp->strike;
					node0 = node;
				} else
					break;
			}
			/* select both the put and call with strike price K0 */
			prev = dlist_node_prev(node0);
			next = dlist_node_next(node0);
			scp  = (struct scp *)dlist_node_value(node0);
			scp2 = (struct scp *)dlist_node_value(prev);
			scp3 = (struct scp *)dlist_node_value(next);
			/* FIXME */
			if (prev == NULL && next == NULL)
				goto err;
			else if (prev == NULL)
				deltaK = scp3->strike - K0;
			else if (next == NULL)
				deltaK = K0 - scp2->strike;
			else
				deltaK = (scp3->strike - scp2->strike) / 2.0;
			QK = (scp->cbid + scp->cask + scp->pbid + scp->pask) / 4.0;
			sum = deltaK * exp(r * T) * QK / (K0 * K0);
			/* select out-of-the-money put options with strike prices < K0 */
			node = node0;
			while ((node = dlist_node_prev(node))) {
				prev = dlist_node_prev(node);
				next = dlist_node_next(node);
				scp  = (struct scp *)dlist_node_value(node);
				scp2 = (struct scp *)dlist_node_value(prev);
				scp3 = (struct scp *)dlist_node_value(next);
				if (isnan(scp->pbid) || isnan(scp->pask))
					continue;
				if (fabs(scp->pbid) <= 0.000001 && scp2 && fabs(scp2->pbid) <= 0.000001)
					break;
				if (fabs(scp->pbid) <= 0.000001)
					continue;
				deltaK = prev ? (scp3->strike - scp2->strike) / 2.0 :
					scp3->strike - scp->strike;
				QK = (scp->pbid + scp->pask) / 2.0;
				sum += deltaK * exp(r * T) * QK / (scp->strike * scp->strike);
			}
			/* select out-of-the-money call options with strike prices > K0 */
			node = node0;
			while ((node = dlist_node_next(node))) {
				prev = dlist_node_prev(node);
				next = dlist_node_next(node);
				scp  = (struct scp *)dlist_node_value(node);
				scp2 = (struct scp *)dlist_node_value(prev);
				scp3 = (struct scp *)dlist_node_value(next);
				if (isnan(scp->cbid) || isnan(scp->cask))
					continue;
				if (fabs(scp->cbid) <= 0.000001 && scp3 && fabs(scp3->cbid) <= 0.000001)
					break;
				if (fabs(scp->cbid) <= 0.000001)
					continue;
				deltaK = next ? (scp3->strike - scp2->strike) / 2.0 :
					scp->strike - scp2->strike;
				QK = (scp->cbid + scp->cask) / 2.0;
				sum += deltaK * exp(r * T) * QK / (scp->strike * scp->strike);
			}
			if (flag == 1) {
				T1     = T;
				sigma1 = 2 * sum / T - (F / K0 - 1) * (F / K0 - 1) / T;
			} else {
				T2     = T;
				sigma2 = 2 * sum / T - (F / K0 - 1) * (F / K0 - 1) / T;
			}
			if (!isnan(sigma1) && !isnan(sigma2) && !isnan(T1) && !isnan(T2)) {
				time_t t = (time_t)quote->thyquote.m_nTime;
				struct tm lt;
				char datestr[64], res[512];
				double vxfc;

				vxfc = 100 * sqrt(T1 * sigma1 * ((T2 * 525600 - 43200) /
					(T2 * 525600 - T1 * 525600)) + T2 * sigma2 * ((43200 - T1 * 525600) /
					(T2 * 525600 - T1 * 525600)) * 525600 / 43200);
				strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
				snprintf(res, sizeof res, "VXFC,%s.%03d|%f",
					datestr,
					quote->m_nMSec,
					vxfc);
				out2rmp(res);
			}
		}

err:
		table_unlock(spots);
		dstr_free(type);
		dstr_free(strike);
	}

end:
	dstr_free(contract);
	return 0;
}

static void kfree(const void *key) {
	dstr_free((dstr)key);
}

static void vfree(void *value) {
	dlist_t dlist = (dlist_t)value;

	dlist_free(&dlist);
}

static int load_module(void) {
	spots = table_new(cmpstr, hashmurmur2, kfree, vfree);
	load_config();
	if (msgs_hook(&default_msgs, vxfc_exec, NULL) == -1)
		return MODULE_LOAD_FAILURE;
	return register_application(app, vxfc_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	msgs_unhook(&default_msgs, vxfc_exec);
	config_destroy(cfg);
	table_free(&spots);
	return unregister_application(app);
}

static int reload_module(void) {
	msgs_unhook(&default_msgs, vxfc_exec);
	config_destroy(cfg);
	table_clear(spots);
	load_config();
	if (msgs_hook(&default_msgs, vxfc_exec, NULL) == -1)
		return MODULE_LOAD_FAILURE;
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Volatility Index FC Application");

