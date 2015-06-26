/*
 * Copyright (c) 2013-2015, Dalian Futures Information Technology Co., Ltd.
 *
 * Guodong Zhang
 * Xiaoye Meng   <mengxiaoye at dce dot com dot cn>
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
#include <strings.h>
#include <time.h>
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
struct scp {
	double	strike, cvol, pvol, cvol2, pvol2, cvol3, pvol3;
};
struct pd {
	double	prevxo, prevxo2, prevxo3;
	char	*sep;
	dlist_t	dlist;
};

/* FIXME */
static char *app = "vxo";
static char *desc = "Volatility Index Old";
static char *fmt = "VXO,timestamp,contract,vxo,vxo2,vxo3";
static table_t spots;
static struct msgs *vxo_msgs;
static struct config *cfg;
static const char *inmsg = "impv_msgs";

static void scpfree(void *value) {
	FREE(value);
}

static inline void load_config(void) {
	/* FIXME */
	if ((cfg = config_load("/etc/xcb/vxo.conf"))) {
		char *cat = category_browse(cfg, NULL);
		struct variable *var;

		while (cat) {
			if (!strcasecmp(cat, "general")) {
				struct variable *var = variable_browse(cfg, cat);

				while (var) {
					if (!strcasecmp(var->name, "inmsg")) {
						if (strcmp(var->value, ""))
							inmsg = var->value;
					} else
						xcb_log(XCB_LOG_WARNING, "Unknown variable '%s' in "
							"category '%s' of vxo.conf", var->name, cat);
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
						struct pd *pd;
						struct scp *scp;

						spotname = *(p - 1) == '-'
							? dstr_new_len(var->name, p - var->name - 1)
							: dstr_new_len(var->name, p - var->name);
						strike   = *(p + 1) == '-'
							? dstr_new_len(p + 2, var->name + strlen(var->name) - p - 2)
							: dstr_new_len(p + 1, var->name + strlen(var->name) - p - 1);
						if ((pd = table_get_value(spots, spotname)) == NULL) {
							if (NEW(scp)) {
								scp->strike = atof(strike);
								scp->cvol   = scp->pvol  = NAN;
								scp->cvol2  = scp->pvol2 = NAN;
								scp->cvol3  = scp->pvol3 = NAN;
								if (NEW(pd)) {
									pd->prevxo = pd->prevxo2 = pd->prevxo3 = NAN;
									pd->sep    = *(p - 1) == '-' ? "-" : "";
									pd->dlist  = dlist_new(NULL, scpfree);
									dlist_insert_tail(pd->dlist, scp);
									table_insert(spots, spotname, pd);
								} else
									FREE(scp);
							} else
								dstr_free(spotname);
						} else {
							dlist_iter_t iter = dlist_iter_new(pd->dlist, DLIST_START_HEAD);
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
									scp->cvol   = scp->pvol  = NAN;
									scp->cvol2  = scp->pvol2 = NAN;
									scp->cvol3  = scp->pvol3 = NAN;
								}
								if (node == NULL)
									dlist_insert_tail(pd->dlist, scp);
								else
									dlist_insert(pd->dlist, node, scp, 0);
							}
							dstr_free(spotname);
						}
						dstr_free(strike);
					}
					var = var->next;
				}
			}
			cat = category_browse(cfg, cat);
		}
	}
}

static int vxo_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	struct msgs *out = (struct msgs *)data2;
	dstr *fields = NULL;
	int nfield = 0;
	double vol, vol2, vol3, spot, strike, r, expiry;
	int sec, msec;
	dstr spotname;
	char *type;
	struct pd *pd;
	struct scp *scp;

	fields = dstr_split_len(msg->data, strlen(msg->data), ",", 1, &nfield);
	/* FIXME */
	if (nfield != 19) {
		xcb_log(XCB_LOG_WARNING, "Message '%s' garbled", msg->data);
		goto end;
	}
	vol      = !strcasecmp(fields[5], "nan") ? NAN : atof(fields[5]);
	vol2     = !strcasecmp(fields[7], "nan") ? NAN : atof(fields[7]);
	vol3     = !strcasecmp(fields[9], "nan") ? NAN : atof(fields[9]);
	if (isnan(vol) && isnan(vol2) && isnan(vol3))
		goto end;
	sec      = atoi(fields[1]);
	msec     = atoi(fields[2]);
	spot     = atof(fields[10]);
	spotname = dstr_new(fields[11]);
	type     = fields[12];
	strike   = atof(fields[13]);
	r        = atof(fields[14]);
	expiry   = atof(fields[15]);
	table_lock(spots);
	if ((pd = table_get_value(spots, spotname)) == NULL) {
		/* can't happen */
		if (NEW(scp) == NULL) {
			xcb_log(XCB_LOG_WARNING, "Error allocating memory for scp");
			table_unlock(spots);
			goto end;
		}
		scp->strike = strike;
		if (!strcasecmp(type, "C")) {
			scp->cvol  = vol;
			scp->pvol  = NAN;
			scp->cvol2 = vol2;
			scp->pvol2 = NAN;
			scp->cvol3 = vol3;
			scp->pvol3 = NAN;
		} else {
			scp->cvol  = NAN;
			scp->pvol  = vol;
			scp->cvol2 = NAN;
			scp->pvol2 = vol2;
			scp->cvol3 = NAN;
			scp->pvol3 = vol3;
		}
		if (NEW(pd) == NULL) {
			xcb_log(XCB_LOG_WARNING, "Error allocating memory for pd");
			FREE(scp);
			table_unlock(spots);
			goto end;
		}
		pd->prevxo = pd->prevxo2 = pd->prevxo3 = NAN;
		pd->sep    = strchr(fields[3], '-') ? "-" : "";
		pd->dlist  = dlist_new(NULL, scpfree);
		dlist_insert_tail(pd->dlist, scp);
		table_insert(spots, spotname, pd);
	} else {
		dlist_iter_t iter = dlist_iter_new(pd->dlist, DLIST_START_HEAD);
		dlist_node_t node;

		while ((node = dlist_next(iter))) {
			scp = (struct scp *)dlist_node_value(node);
			if (scp->strike > strike || fabs(scp->strike - strike) <= 0.000001)
				break;
		}
		if (node && fabs(scp->strike - strike) <= 0.000001) {
			if (!strcasecmp(type, "C")) {
				scp->cvol  = vol;
				scp->cvol2 = vol2;
				scp->cvol3 = vol3;
			} else {
				scp->pvol  = vol;
				scp->pvol2 = vol2;
				scp->pvol3 = vol3;
			}
		} else {
			/* can't happen */
			if (NEW(scp) == NULL) {
				xcb_log(XCB_LOG_WARNING, "Error allocating memory for scp");
				table_unlock(spots);
				goto end;
			}
			scp->strike = strike;
			if (!strcasecmp(type, "C")) {
				scp->cvol  = vol;
				scp->pvol  = NAN;
				scp->cvol2 = vol2;
				scp->pvol2 = NAN;
				scp->cvol3 = vol3;
				scp->pvol3 = NAN;
			} else {
				scp->cvol  = NAN;
				scp->pvol  = vol;
				scp->cvol2 = NAN;
				scp->pvol2 = vol2;
				scp->cvol3 = NAN;
				scp->pvol3 = vol3;
			}
			if (node == NULL)
				dlist_insert_tail(pd->dlist, scp);
			else
				dlist_insert(pd->dlist, node, scp, 0);
		}
		dlist_iter_rewind_head(iter, pd->dlist);
		while ((node = dlist_next(iter))) {
			scp = (struct scp *)dlist_node_value(node);
			if (scp->strike > spot || fabs(scp->strike - spot) <= 0.000001)
				break;
		}
		dlist_iter_free(&iter);
		if (node && dlist_node_prev(node)) {
			struct scp *scp1 = (struct scp *)dlist_node_value(node);
			struct scp *scp2 = (struct scp *)dlist_node_value(dlist_node_prev(node));
			double vxo = NAN, vxo2 = NAN, vxo3 = NAN;
			int flag1, flag2, flag3;

			if (!isnan(scp1->cvol) && !isnan(scp1->pvol) && !isnan(scp2->cvol) && !isnan(scp2->pvol))
				vxo  = ((scp1->cvol + scp1->pvol) / 2) *
					(spot - scp2->strike) / (scp1->strike - scp2->strike) +
					((scp2->cvol + scp2->pvol) / 2) *
					(scp1->strike - spot) / (scp1->strike - scp2->strike);
			if (!isnan(scp1->cvol2) && !isnan(scp1->pvol2) && !isnan(scp2->cvol2) && !isnan(scp2->pvol2))
				vxo2 = ((scp1->cvol2 + scp1->pvol2) / 2) *
					(spot - scp2->strike) / (scp1->strike - scp2->strike) +
					((scp2->cvol2 + scp2->pvol2) / 2) *
					(scp1->strike - spot) / (scp1->strike - scp2->strike);
			if (!isnan(scp1->cvol3) && !isnan(scp1->pvol3) && !isnan(scp2->cvol3) && !isnan(scp2->pvol3))
				vxo3 = ((scp1->cvol3 + scp1->pvol3) / 2) *
					(spot - scp2->strike) / (scp1->strike - scp2->strike) +
					((scp2->cvol3 + scp2->pvol3) / 2) *
					(scp1->strike - spot) / (scp1->strike - scp2->strike);
			if ((flag1 = (isnan(pd->prevxo) && !isnan(vxo)) || (!isnan(pd->prevxo) && isnan(vxo)) ||
				(!isnan(pd->prevxo) && !isnan(vxo) && fabs(pd->prevxo - vxo) > 0.000001) ? 1 : 0))
				pd->prevxo  = vxo;
			if ((flag2 = (isnan(pd->prevxo2) && !isnan(vxo2)) || (!isnan(pd->prevxo2) && isnan(vxo2)) ||
				(!isnan(pd->prevxo2) && !isnan(vxo2) && fabs(pd->prevxo2 - vxo2) > 0.000001) ? 1 : 0))
				pd->prevxo2 = vxo2;
			if ((flag3 = (isnan(pd->prevxo3) && !isnan(vxo3)) || (!isnan(pd->prevxo3) && isnan(vxo3)) ||
				(!isnan(pd->prevxo3) && !isnan(vxo3) && fabs(pd->prevxo3 - vxo3) > 0.000001) ? 1 : 0))
				pd->prevxo3 = vxo3;
			if (flag1 || flag2 || flag3) {
				char *res;

				if ((res = ALLOC(4096))) {
					time_t t = (time_t)sec;
					struct tm lt;
					char datestr[64];
					int off;

					strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
					snprintf(res, 4096, "VXO,%s.%03d,%s|%f,%f,%f",
						datestr,
						msec,
						spotname,
						vxo,
						vxo2,
						vxo3);
					out2rmp(res);
					off = snprintf(res, 4096, "VXO,%d,%d,%s,",
						sec,
						msec,
						spotname);
					iter = dlist_iter_new(pd->dlist, DLIST_START_HEAD);
					while ((node = dlist_next(iter))) {
						scp = (struct scp *)dlist_node_value(node);
						off += snprintf(res + off, 4096 - off, "%.f,%f,%f,%f,",
							scp->strike,
							vxo,
							vxo2,
							vxo3);
					}
					dlist_iter_free(&iter);
					snprintf(res + off, 4096 - off, "%.2f,%f,%f,%s",
						spot,
						r,
						expiry,
						pd->sep);
					if (out2msgs(res, out) == -1)
						FREE(res);
				}
			}
		}
		dstr_free(spotname);
	}
	table_unlock(spots);

end:
	dstr_free_tokens(fields, nfield);
	return 0;
}

static void kfree(const void *key) {
	dstr_free((dstr)key);
}

static void vfree(void *value) {
	struct pd *pd = (struct pd *)value;

	dlist_free(&pd->dlist);
	FREE(pd);
}

static int load_module(void) {
	int res;

	spots = table_new(cmpstr, hashmurmur2, kfree, vfree);
	load_config();
	if (msgs_init(&vxo_msgs, "vxo_msgs", mod_info->self) == -1)
		return MODULE_LOAD_FAILURE;
	if (start_msgs(vxo_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	if ((res = msgs_hook_name(inmsg, vxo_exec, vxo_msgs)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue '%s' not found", inmsg);
		return MODULE_LOAD_FAILURE;
	}
	return register_application(app, vxo_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	if (check_msgs(vxo_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	msgs_unhook_name(inmsg, vxo_exec);
	stop_msgs(vxo_msgs);
	msgs_free(vxo_msgs);
	config_destroy(cfg);
	table_free(&spots);
	return unregister_application(app);
}

static int reload_module(void) {
	int res;

	msgs_unhook_name(inmsg, vxo_exec);
	config_destroy(cfg);
	table_clear(spots);
	load_config();
	if ((res = msgs_hook_name(inmsg, vxo_exec, vxo_msgs)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue '%s' not found", inmsg);
		return MODULE_LOAD_FAILURE;
	}
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Volatility Index Old Application");

