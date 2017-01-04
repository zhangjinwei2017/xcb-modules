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
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_multifit.h>
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

/* FIXME */
struct scp {
	double	strike, clast, cvol, plast, pvol, cbid, cvol2, pbid, pvol2, cask, cvol3, pask, pvol3;
	int	flag;
	dstr	suffix;
};
struct sd {
	char	*sep;
	dlist_t	dlist;
};

/* FIXME */
static char *app = "vsml";
static char *desc = "Volatility Smile";
static char *fmt = "VSML,timestamp,contract,strike1,vol,vol2,vol3,strike2,vol,vol2,vol3,...,"
	"striken,vol,vol2,vol3";
static table_t spots;
static struct msgs *vsml_msgs;
static struct config *cfg;
static const char *inmsg = "impv_msgs";

static void scpfree(void *value) {
	struct scp *scp = (struct scp *)value;

	dstr_free(scp->suffix);
	FREE(scp);
}

static inline void load_config(void) {
	/* FIXME */
	if ((cfg = config_load("/etc/xcb/vsml.conf"))) {
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
							"category '%s' of vsml.conf", var->name, cat);
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
								scp->clast  = scp->plast = NAN;
								scp->cvol   = scp->pvol  = NAN;
								scp->cbid   = scp->pbid  = NAN;
								scp->cvol2  = scp->pvol2 = NAN;
								scp->cask   = scp->pask  = NAN;
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
									scp->clast  = scp->plast = NAN;
									scp->cvol   = scp->pvol  = NAN;
									scp->cbid   = scp->pbid  = NAN;
									scp->cvol2  = scp->pvol2 = NAN;
									scp->cask   = scp->pask  = NAN;
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

static int vsml_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	struct msgs *out = (struct msgs *)data2;
	dstr *fields = NULL;
	int nfield = 0;
	double last, vol, bid, vol2, ask, vol3, spot, strike, r, expiry;
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
	last     = !strcasecmp(fields[4], "nan") ? NAN : atof(fields[4]);
	vol      = !strcasecmp(fields[5], "nan") ? NAN : atof(fields[5]);
	bid      = !strcasecmp(fields[6], "nan") ? NAN : atof(fields[6]);
	vol2     = !strcasecmp(fields[7], "nan") ? NAN : atof(fields[7]);
	ask      = !strcasecmp(fields[8], "nan") ? NAN : atof(fields[8]);
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
			scp->clast = last;
			scp->cvol  = vol;
			scp->plast = NAN;
			scp->pvol  = NAN;
			scp->cbid  = bid;
			scp->cvol2 = vol2;
			scp->pbid  = NAN;
			scp->pvol2 = NAN;
			scp->cask  = ask;
			scp->cvol3 = vol3;
			scp->pask  = NAN;
			scp->pvol3 = NAN;
		} else {
			scp->clast = NAN;
			scp->cvol  = NAN;
			scp->plast = last;
			scp->pvol  = vol;
			scp->cbid  = NAN;
			scp->cvol2 = NAN;
			scp->pbid  = bid;
			scp->pvol2 = vol2;
			scp->cask  = NAN;
			scp->cvol3 = NAN;
			scp->pask  = ask;
			scp->pvol3 = vol3;
		}
		scp->suffix = *(p - 1) == '-' ? dstr_new(p + 2) : dstr_new(p + 1);
		if (!isnan(vol2) || !isnan(vol3))
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
					scp->clast = last;
					scp->cvol  = vol;
				}
				if ((isnan(scp->cvol2) && !isnan(vol2)) ||
					(!isnan(scp->cvol2) && !isnan(vol2) &&
					fabs(scp->cvol2 - vol2) > 0.000001)) {
					scp->cbid  = bid;
					scp->cvol2 = vol2;
					scp->flag  = 1;
				}
				if ((isnan(scp->cvol3) && !isnan(vol3)) ||
					(!isnan(scp->cvol3) && !isnan(vol3) &&
					fabs(scp->cvol3 - vol3) > 0.000001)) {
					scp->cask  = ask;
					scp->cvol3 = vol3;
					scp->flag  = 1;
				}
			} else {
				if ((isnan(scp->pvol) && !isnan(vol)) ||
					(!isnan(scp->pvol) && !isnan(vol) &&
					fabs(scp->pvol - vol) > 0.000001)) {
					scp->plast = last;
					scp->pvol  = vol;
				}
				if ((isnan(scp->pvol2) && !isnan(vol2)) ||
					(!isnan(scp->pvol2) && !isnan(vol2) &&
					fabs(scp->pvol2 - vol2) > 0.000001)) {
					scp->pbid  = bid;
					scp->pvol2 = vol2;
					scp->flag  = 1;
				}
				if ((isnan(scp->pvol3) && !isnan(vol3)) ||
					(!isnan(scp->pvol3) && !isnan(vol3) &&
					fabs(scp->pvol3 - vol3) > 0.000001)) {
					scp->pask  = ask;
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
				scp->clast = last;
				scp->cvol  = vol;
				scp->plast = NAN;
				scp->pvol  = NAN;
				scp->cbid  = bid;
				scp->cvol2 = vol2;
				scp->pbid  = NAN;
				scp->pvol2 = NAN;
				scp->cask  = ask;
				scp->cvol3 = vol3;
				scp->pask  = NAN;
				scp->pvol3 = NAN;
			} else {
				scp->clast = NAN;
				scp->cvol  = NAN;
				scp->plast = last;
				scp->pvol  = vol;
				scp->cbid  = NAN;
				scp->cvol2 = NAN;
				scp->pbid  = bid;
				scp->pvol2 = vol2;
				scp->cask  = NAN;
				scp->cvol3 = NAN;
				scp->pask  = ask;
				scp->pvol3 = vol3;
			}
			scp->suffix = *(p - 1) == '-' ? dstr_new(p + 2) : dstr_new(p + 1);
			if (!isnan(vol2) || !isnan(vol3))
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
		if (node) {
			dlist_node_t prev = dlist_node_prev(node);
			dlist_node_t next = scp->strike > spot ? node : dlist_node_next(node);
			double price, preprice = NAN;
			dlist_t dlist = dlist_new(NULL, NULL);
			int flag = 0;
			size_t n;

			/* out-of-the-money puts */
			while (prev) {
				scp = (struct scp *)dlist_node_value(prev);
				if (!isnan(scp->pvol2) && !isnan(scp->pvol3)) {
					price = (scp->pbid + scp->pask) / 2;
					if (isnan(preprice) || price < preprice) {
						preprice = price;
						dlist_insert_head(dlist, scp);
						if (scp->flag)
							flag = 1;
					}
				}
				prev = dlist_node_prev(prev);
			}
			preprice = NAN;
			/* out-of-the-money calls */
			while (next) {
				scp = (struct scp *)dlist_node_value(next);
				if (!isnan(scp->cvol2) && !isnan(scp->cvol3)) {
					price = (scp->cbid + scp->cask) / 2;
					if (isnan(preprice) || price < preprice) {
						preprice = price;
						dlist_insert_tail(dlist, scp);
						if (scp->flag)
							flag = 1;
					}
				}
				next = dlist_node_next(next);
			}
			if ((n = dlist_length(dlist)) > 2 && flag) {
				double fm, chisq;
				size_t i = 0;
				gsl_matrix *X = gsl_matrix_alloc(n, 3);
				gsl_vector *y = gsl_vector_alloc(n);
				gsl_vector *c = gsl_vector_alloc(3);
				gsl_matrix *cov = gsl_matrix_alloc(3, 3);
				gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(n, 3);
				char *res;

				iter = dlist_iter_new(dlist, DLIST_START_HEAD);
				while ((node = dlist_next(iter))) {
					scp = (struct scp *)dlist_node_value(node);
					fm = log(scp->strike / spot);
					gsl_matrix_set(X, i, 0, 1.0);
					gsl_matrix_set(X, i, 1, fm);
					gsl_matrix_set(X, i, 2, fm * fm);
					if (scp->strike < spot)
						gsl_vector_set(y, i, (scp->pvol2 + scp->pvol3) / 2);
					else
						gsl_vector_set(y, i, (scp->cvol2 + scp->cvol3) / 2);
					++i;
				}
				dlist_iter_free(&iter);
				/* least squares fitting */
				gsl_multifit_linear(X, y, c, cov, &chisq, work);
				if ((res = ALLOC(4096))) {
					char tmp[4096], tmp2[4096];
					int off = 0, off2 = 0;
					time_t t = (time_t)sec;
					struct tm lt;
					char datestr[64];

					iter = dlist_iter_new(sd->dlist, DLIST_START_HEAD);
					while ((node = dlist_next(iter))) {
						gsl_vector *x = gsl_vector_alloc(3);
						double y, y_err;

						scp = (struct scp *)dlist_node_value(node);
						fm = log(scp->strike / spot);
						gsl_vector_set(x, 0, 1.0);
						gsl_vector_set(x, 1, fm);
						gsl_vector_set(x, 2, fm * fm);
						gsl_multifit_linear_est(x, c, cov, &y, &y_err);
						if (y < 0.0) {
							if (scp->strike < spot) {
								if (!isnan(scp->pvol2) && !isnan(scp->pvol3))
									y = (scp->pvol2 + scp->pvol3) / 2;
								else if (!isnan(scp->pvol))
									y = scp->pvol;
								else if (!isnan(scp->pvol2))
									y = scp->pvol2;
								else if (!isnan(scp->pvol3))
									y = scp->pvol3;
								else
									y = 0.01;
							} else {
								if (!isnan(scp->cvol2) && !isnan(scp->cvol3))
									y = (scp->cvol2 + scp->cvol3) / 2;
								else if (!isnan(scp->cvol))
									y = scp->cvol;
								else if (!isnan(scp->cvol2))
									y = scp->cvol2;
								else if (!isnan(scp->cvol3))
									y = scp->cvol3;
								else
									y = 0.01;
							}
						}
						off  += snprintf(tmp + off,   4096 - off,  "%f,%f,%f,%f,",
							scp->strike, y, y, y);
						off2 += snprintf(tmp2 + off2, 4096 - off2, "%s,%f,%f,%f,",
							scp->suffix, y, y, y);
						gsl_vector_free(x);
					}
					dlist_iter_free(&iter);
					tmp[strlen(tmp) - 1] = '\0';
					tmp2[strlen(tmp2) - 1] = '\0';
					strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
					snprintf(res, 4096, "VSML,%s.%03d,%s|%s",
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
					snprintf(res, 4096, "VSML,%d,%d,%s,%s,%.2f,%f,%f,%s,%s,%s",
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
					iter = dlist_iter_new(dlist, DLIST_START_HEAD);
					while ((node = dlist_next(iter))) {
						scp = (struct scp *)dlist_node_value(node);
						scp->flag = 0;
					}
					dlist_iter_free(&iter);
				} else
					xcb_log(XCB_LOG_WARNING, "Error allocating memory for result");
				gsl_multifit_linear_free(work);
				gsl_matrix_free(cov);
				gsl_vector_free(c);
				gsl_vector_free(y);
				gsl_matrix_free(X);
			}
			dlist_free(&dlist);
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
	if (msgs_init(&vsml_msgs, "vsml_msgs", mod_info->self) == -1)
		return MODULE_LOAD_FAILURE;
	if (start_msgs(vsml_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	if ((res = msgs_hook_name(inmsg, vsml_exec, vsml_msgs)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue '%s' not found", inmsg);
		return MODULE_LOAD_FAILURE;
	}
	return register_application(app, vsml_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	if (check_msgs(vsml_msgs) == -1)
		return MODULE_LOAD_FAILURE;
	msgs_unhook_name(inmsg, vsml_exec);
	stop_msgs(vsml_msgs);
	msgs_free(vsml_msgs);
	config_destroy(cfg);
	table_free(&spots);
	return unregister_application(app);
}

static int reload_module(void) {
	int res;

	msgs_unhook_name(inmsg, vsml_exec);
	config_destroy(cfg);
	table_clear(spots);
	load_config();
	if ((res = msgs_hook_name(inmsg, vsml_exec, vsml_msgs)) < 0) {
		if (res == -2)
			xcb_log(XCB_LOG_WARNING, "Queue '%s' not found", inmsg);
		return MODULE_LOAD_FAILURE;
	}
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Volatility Smile Application");

