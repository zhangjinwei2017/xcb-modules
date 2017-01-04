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
#include <xcb/utils.h>
#include <xcb/basics.h>
#include "lagrange.h"

/* FIXME */
struct scp {
	double	strike, cvol, pvol, cvol2, pvol2, cvol3, pvol3;
	int	flag;
	dstr	suffix;
};
struct sd {
	char	*sep;
	dlist_t	dlist;
};

/* FIXME */
static char *app = "vintp";
static char *desc = "Volatility Interpolation";
static char *fmt = "VINTP,timestamp,contract,strike1,vol,vol2,vol3,strike2,vol,vol2,vol3,...,"
	"striken,vol,vol2,vol3";
static table_t spots;
static struct msgs *vintp_msgs;
static struct config *cfg;
static const char *inmsg = "impv_msgs";

static void scpfree(void *value) {
	struct scp *scp = (struct scp *)value;

	dstr_free(scp->suffix);
	FREE(scp);
}

static inline void load_config(void) {
	/* FIXME */
	if ((cfg = config_load("/etc/xcb/vintp.conf"))) {
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
							"category '%s' of vintp.conf", var->name, cat);
					var = var->next;
				}
			} else if (!strcasecmp(cat, "expiries")) {
				var = variable_browse(cfg, cat);
				while (var) {
					const char *p, *q;

					p = strrchr(var->name, 'C');
					if (p && p != var->name && p != var->name + strlen(var->name) - 1 &&
						((*(p - 1) == '-' && *(p + 1) == '-') ||
						(isdigit(*(p - 1)) && isdigit(*(p + 1))))) {
						dstr spotname;
						double strike;
						struct sd *sd;
						struct scp *scp;

						q = var->name + strlen(var->name) - 1;
						while (isdigit(*q))
							--q;
						if (!strncasecmp(var->name, "SH", 2) ||
							!strncasecmp(var->name, "SZ", 2)) {
							spotname = dstr_new_len(var->name, q - var->name);
							strike   = atof(q + 1) / 1000;
						} else {
							spotname = *(p - 1) == '-'
								? dstr_new_len(var->name, p - var->name - 1)
								: dstr_new_len(var->name, p - var->name);
							strike   = atof(q + 1);
						}
						if ((sd = table_get_value(spots, spotname)) == NULL) {
							if (NEW(scp)) {
								scp->strike = strike;
								scp->cvol   = scp->pvol  = NAN;
								scp->cvol2  = scp->pvol2 = NAN;
								scp->cvol3  = scp->pvol3 = NAN;
								scp->flag   = 0;
								scp->suffix = *(p - 1) == '-'
									? dstr_new(p + 2) : dstr_new(p + 1);
								if (NEW(sd)) {
									sd->sep   = *(p - 1) == '-'
										? "-" : "";
									sd->dlist = dlist_new(NULL, scpfree);
									dlist_insert_tail(sd->dlist, scp);
									table_insert(spots, spotname, sd);
								} else
									scpfree(scp);
							} else
								dstr_free(spotname);
						} else {
							dlist_iter_t iter = dlist_iter_new(sd->dlist,
										DLIST_START_HEAD);
							dlist_node_t node;

							while ((node = dlist_next(iter))) {
								scp = (struct scp *)dlist_node_value(node);
								if (scp->strike > strike ||
									fabs(scp->strike - strike) <= 0.000001)
									break;
							}
							dlist_iter_free(&iter);
							if (node == NULL || scp->strike > strike) {
								if (NEW(scp)) {
									scp->strike = strike;
									scp->cvol   = scp->pvol  = NAN;
									scp->cvol2  = scp->pvol2 = NAN;
									scp->cvol3  = scp->pvol3 = NAN;
									scp->flag   = 0;
									scp->suffix = *(p - 1) == '-'
										? dstr_new(p + 2)
										: dstr_new(p + 1);
								}
								if (node == NULL)
									dlist_insert_tail(sd->dlist, scp);
								else
									dlist_insert(sd->dlist, node, scp, 0);
							}
							dstr_free(spotname);
						}
					}
					var = var->next;
				}
			}
			cat = category_browse(cfg, cat);
		}
	}
}

static int vintp_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	struct msgs *out = (struct msgs *)data2;
	dstr *fields = NULL;
	int nfield = 0;
	double vol, vol2, vol3, spot, strike, r, expiry;
	int sec, msec;
	dstr spotname;
	char *type, *p;
	struct sd *sd;
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
	if ((p = strrchr(fields[3], 'C')) == NULL)
		p = strrchr(fields[3], 'P');
	table_lock(spots);
	if ((sd = table_get_value(spots, spotname)) == NULL) {
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
		scp->suffix = *(p - 1) == '-' ? dstr_new(p + 2) : dstr_new(p + 1);
		if (!isnan(vol) || !isnan(vol2) || !isnan(vol3))
			scp->flag = 1;
		if (NEW(sd) == NULL) {
			xcb_log(XCB_LOG_WARNING, "Error allocating memory for sd");
			scpfree(scp);
			table_unlock(spots);
			goto end;
		}
		sd->sep   = strchr(fields[3], '-') ? "-" : "";
		sd->dlist = dlist_new(NULL, scpfree);
		dlist_insert_tail(sd->dlist, scp);
		table_insert(spots, spotname, sd);
	} else {
		dlist_iter_t iter = dlist_iter_new(sd->dlist, DLIST_START_HEAD);
		dlist_node_t node;

		while ((node = dlist_next(iter))) {
			scp = (struct scp *)dlist_node_value(node);
			if (scp->strike > strike || fabs(scp->strike - strike) <= 0.000001)
				break;
		}
		if (node && fabs(scp->strike - strike) <= 0.000001) {
			if (!strcasecmp(type, "C")) {
				if ((isnan(scp->cvol) && !isnan(vol)) ||
					(!isnan(scp->cvol) && !isnan(vol) &&
					fabs(scp->cvol - vol) > 0.000001)) {
					scp->cvol  = vol;
					scp->flag  = 1;
				}
				if ((isnan(scp->cvol2) && !isnan(vol2)) ||
					(!isnan(scp->cvol2) && !isnan(vol2) &&
					fabs(scp->cvol2 - vol2) > 0.000001)) {
					scp->cvol2 = vol2;
					scp->flag  = 1;
				}
				if ((isnan(scp->cvol3) && !isnan(vol3)) ||
					(!isnan(scp->cvol3) && !isnan(vol3) &&
					fabs(scp->cvol3 - vol3) > 0.000001)) {
					scp->cvol3 = vol3;
					scp->flag  = 1;
				}
			} else {
				if ((isnan(scp->pvol) && !isnan(vol)) ||
					(!isnan(scp->pvol) && !isnan(vol) &&
					fabs(scp->pvol - vol) > 0.000001)) {
					scp->pvol  = vol;
					scp->flag  = 1;
				}
				if ((isnan(scp->pvol2) && !isnan(vol2)) ||
					(!isnan(scp->pvol2) && !isnan(vol2) &&
					fabs(scp->pvol2 - vol2) > 0.000001)) {
					scp->pvol2 = vol2;
					scp->flag  = 1;
				}
				if ((isnan(scp->pvol3) && !isnan(vol3)) ||
					(!isnan(scp->pvol3) && !isnan(vol3) &&
					fabs(scp->pvol3 - vol3) > 0.000001)) {
					scp->pvol3 = vol3;
					scp->flag  = 1;
				}
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
			scp->suffix = *(p - 1) == '-' ? dstr_new(p + 2) : dstr_new(p + 1);
			if (!isnan(vol) || !isnan(vol2) || !isnan(vol3))
				scp->flag = 1;
			if (node == NULL)
				dlist_insert_tail(sd->dlist, scp);
			else
				dlist_insert(sd->dlist, node, scp, 0);
		}
		dlist_iter_rewind_head(iter, sd->dlist);
		while ((node = dlist_next(iter))) {
			scp = (struct scp *)dlist_node_value(node);
			if (scp->strike > spot || fabs(scp->strike - spot) <= 0.000001)
				break;
		}
		dlist_iter_free(&iter);
		if (node && dlist_node_prev(node) &&
			dlist_node_prev(dlist_node_prev(node)) && dlist_node_next(node)) {
			dlist_node_t node1 = dlist_head(sd->dlist);
			dlist_node_t node2 = dlist_tail(sd->dlist);
			struct scp *scp0  = (struct scp *)dlist_node_value(node);
			struct scp *scp1  = (struct scp *)dlist_node_value(dlist_node_prev(node));
			struct scp *scp12 = (struct scp *)dlist_node_value(node1);
			struct scp *scp13 = (struct scp *)dlist_node_value(node2);
			struct scp *scp22 = (struct scp *)dlist_node_value(node1);
			struct scp *scp23 = (struct scp *)dlist_node_value(node2);
			struct scp *scp32 = (struct scp *)dlist_node_value(node1);
			struct scp *scp33 = (struct scp *)dlist_node_value(node2);
			int flag1, flag2, flag3;

			/* FIXME: move right */
			while ((isnan(scp12->cvol) || isnan(scp12->pvol)) &&
				node1 != dlist_node_prev(dlist_node_prev(node))) {
				node1 = dlist_node_next(node1);
				scp12 = (struct scp *)dlist_node_value(node1);
			}
			/* FIXME: move left */
			while ((isnan(scp13->cvol) || isnan(scp13->pvol)) &&
				node2 != dlist_node_next(node)) {
				node2 = dlist_node_prev(node2);
				scp13 = (struct scp *)dlist_node_value(node2);
			}
			/* reset */
			node1 = dlist_head(sd->dlist), node2 = dlist_tail(sd->dlist);
			/* FIXME: move right */
			while ((isnan(scp22->cvol2) || isnan(scp22->pvol2)) &&
				node1 != dlist_node_prev(dlist_node_prev(node))) {
				node1 = dlist_node_next(node1);
				scp22 = (struct scp *)dlist_node_value(node1);
			}
			/* FIXME: move left */
			while ((isnan(scp23->cvol2) || isnan(scp23->pvol2)) &&
				node2 != dlist_node_next(node)) {
				node2 = dlist_node_prev(node2);
				scp23 = (struct scp *)dlist_node_value(node2);
			}
			/* reset */
			node1 = dlist_head(sd->dlist), node2 = dlist_tail(sd->dlist);
			/* FIXME: move right */
			while ((isnan(scp32->cvol3) || isnan(scp32->pvol3)) &&
				node1 != dlist_node_prev(dlist_node_prev(node))) {
				node1 = dlist_node_next(node1);
				scp32 = (struct scp *)dlist_node_value(node1);
			}
			/* FIXME: move left */
			while ((isnan(scp33->cvol3) || isnan(scp33->pvol3)) &&
				node2 != dlist_node_next(node)) {
				node2 = dlist_node_prev(node2);
				scp33 = (struct scp *)dlist_node_value(node2);
			}
			flag1 = !isnan(scp0->cvol)   && !isnan(scp0->pvol)   &&
				!isnan(scp1->cvol)   && !isnan(scp1->pvol)   &&
				!isnan(scp12->cvol)  && !isnan(scp12->pvol)  &&
				!isnan(scp13->cvol)  && !isnan(scp13->pvol)  ? 1 : 0;
			flag2 = !isnan(scp0->cvol2)  && !isnan(scp0->pvol2)  &&
				!isnan(scp1->cvol2)  && !isnan(scp1->pvol2)  &&
				!isnan(scp22->cvol2) && !isnan(scp22->pvol2) &&
				!isnan(scp23->cvol2) && !isnan(scp23->pvol2) ? 1 : 0;
			flag3 = !isnan(scp0->cvol3)  && !isnan(scp0->pvol3)  &&
				!isnan(scp1->cvol3)  && !isnan(scp1->pvol3)  &&
				!isnan(scp32->cvol3) && !isnan(scp32->pvol3) &&
				!isnan(scp33->cvol3) && !isnan(scp33->pvol3) ? 1 : 0;
			if ((scp0->flag || scp1->flag || scp12->flag || scp13->flag || scp22->flag ||
				scp23->flag || scp32->flag || scp33->flag) && (flag1 || flag2 || flag3)) {
				char *res;

				if ((res = ALLOC(4096))) {
					char tmp[4096], tmp2[4096];
					int off = 0, off2 = 0;
					time_t t = (time_t)sec;
					struct tm lt;
					char datestr[64];

					iter = dlist_iter_new(sd->dlist, DLIST_START_HEAD);
					while ((node = dlist_next(iter))) {
						scp = (struct scp *)dlist_node_value(node);
						vol = vol2 = vol3 = NAN;
						if (flag1) {
							vol = cubic(scp->strike, scp0->strike, scp1->strike,
								scp12->strike, scp13->strike,
								(scp0->cvol  + scp0->pvol)  / 2,
								(scp1->cvol  + scp1->pvol)  / 2,
								(scp12->cvol + scp12->pvol) / 2,
								(scp13->cvol + scp13->pvol) / 2);
							/* FIXME */
							if (vol < 0.0)
								vol = NAN;
						}
						if (isnan(vol)) {
							if (!isnan(scp->cvol) && !isnan(scp->pvol))
								vol = (scp->cvol + scp->pvol) / 2;
							else if (!isnan(scp->cvol))
								vol = scp->cvol;
							else if (!isnan(scp->pvol))
								vol = scp->pvol;
							else
								vol = 0.01;
						}
						off  += snprintf(tmp + off,   4096 - off,  "%f,%f,",
							scp->strike,
							vol);
						off2 += snprintf(tmp2 + off2, 4096 - off2, "%s,%f,",
							scp->suffix,
							vol);
						if (flag2) {
							vol2 = cubic(scp->strike, scp0->strike, scp1->strike,
								scp22->strike, scp23->strike,
								(scp0->cvol2  + scp0->pvol2)  / 2,
								(scp1->cvol2  + scp1->pvol2)  / 2,
								(scp22->cvol2 + scp22->pvol2) / 2,
								(scp23->cvol2 + scp23->pvol2) / 2);
							/* FIXME */
							if (vol2 < 0.0)
								vol2 = NAN;
						}
						if (isnan(vol2)) {
							if (!isnan(scp->cvol2) && !isnan(scp->pvol2))
								vol2 = (scp->cvol2 + scp->pvol2) / 2;
							else if (!isnan(scp->cvol2))
								vol2 = scp->cvol2;
							else if (!isnan(scp->pvol2))
								vol2 = scp->pvol2;
							else
								vol2 = 0.01;
						}
						off  += snprintf(tmp + off,   4096 - off,  "%f,", vol2);
						off2 += snprintf(tmp2 + off2, 4096 - off2, "%f,", vol2);
						if (flag3) {
							vol3 = cubic(scp->strike, scp0->strike, scp1->strike,
								scp32->strike, scp33->strike,
								(scp0->cvol3  + scp0->pvol3)  / 2,
								(scp1->cvol3  + scp1->pvol3)  / 2,
								(scp32->cvol3 + scp32->pvol3) / 2,
								(scp33->cvol3 + scp33->pvol3) / 2);
							/* FIXME */
							if (vol3 < 0.0)
								vol3 = NAN;
						}
						if (isnan(vol3)) {
							if (!isnan(scp->cvol3) && !isnan(scp->pvol3))
								vol3 = (scp->cvol3 + scp->pvol3) / 2;
							else if (!isnan(scp->cvol3))
								vol3 = scp->cvol3;
							else if (!isnan(scp->pvol3))
								vol3 = scp->pvol3;
							else
								vol3 = 0.01;
						}
						off  += snprintf(tmp + off,   4096 - off,  "%f,", vol3);
						off2 += snprintf(tmp2 + off2, 4096 - off2, "%f,", vol3);
					}
					dlist_iter_free(&iter);
					tmp[strlen(tmp) - 1] = '\0';
					tmp2[strlen(tmp2) - 1] = '\0';
					strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
					snprintf(res, 4096, "VINTP,%s.%03d,%s|%s",
						datestr,
						msec,
						spotname,
						tmp);
					out2rmp(res);
					/* FIXME */
					dstr_free(spotname);
					spotname = *(p - 1) == '-'
						? dstr_new_len(fields[3], p - fields[3] - 1)
						: dstr_new_len(fields[3], p - fields[3]);
					snprintf(res, 4096, "VINTP,%d,%d,%s,%s,%.2f,%f,%f,%s,%s,%s",
						sec,
						msec,
						spotname,
						tmp2,
						spot,
						r,
						expiry,
						sd->sep,
						fields[17],
						fields[18]);
					if (out2msgs(res, out) == -1)
						FREE(res);
					scp0->flag = scp1->flag = scp12->flag = scp13->flag =
						scp22->flag = scp23->flag = scp32->flag = scp33->flag = 0;
				} else
					xcb_log(XCB_LOG_WARNING, "Error allocating memory for result");
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
	struct sd *sd = (struct sd *)value;

	dlist_free(&sd->dlist);
	FREE(sd);
}

static int load_module(void) {
	int res;

	spots = table_new(cmpstr, hashmurmur2, kfree, vfree);
	load_config();
	if (msgs_init(&vintp_msgs, "vintp_msgs", mod_info->self) == -1)
		return MODULE_LOAD_FAILURE;
	if (start_msgs(vintp_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	if ((res = msgs_hook_name(inmsg, vintp_exec, vintp_msgs)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue '%s' not found", inmsg);
		return MODULE_LOAD_FAILURE;
	}
	return register_application(app, vintp_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	if (check_msgs(vintp_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	msgs_unhook_name(inmsg, vintp_exec);
	stop_msgs(vintp_msgs);
	msgs_free(vintp_msgs);
	config_destroy(cfg);
	table_free(&spots);
	return unregister_application(app);
}

static int reload_module(void) {
	int res;

	msgs_unhook_name(inmsg, vintp_exec);
	config_destroy(cfg);
	table_clear(spots);
	load_config();
	if ((res = msgs_hook_name(inmsg, vintp_exec, vintp_msgs)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue '%s' not found", inmsg);
		return MODULE_LOAD_FAILURE;
	}
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Volatility Interpolation Application");

