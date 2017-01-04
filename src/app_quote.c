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

#include <stdio.h>
#include <time.h>
#include <xcb/macros.h>
#include <xcb/module.h>
#include <xcb/basics.h>

/* FIXME */
static char *app = "quote";
static char *desc = "Quotes";
static char *fmt = "QUOTE,timestamp,contract,exchange,presettlement,settlement,average,preclose,close,"
	"open,preopenint,openint,last,volume,turnover,upperlimit,lowerlimit,highest,lowest,predelta,delta,"
	"bidprice1,askprice1,bidvolume1,askvolume1,bidprice2,askprice2,bidvolume2,askvolume2,bidprice3,"
	"askprice3,bidvolume3,askvolume3,bidprice4,askprice4,bidvolume4,askvolume4,bidprice5,askprice5,"
	"bidvolume5,askvolume5";

static int quote_exec(void *data, void *data2) {
	RAII_VAR(struct msg *, msg, (struct msg *)data, msg_decr);
	Quote *quote = (Quote *)msg->data;
	time_t t = (time_t)quote->thyquote.m_nTime;
	struct tm lt;
	char datestr[64], res[2048];
	NOT_USED(data2);

	strftime(datestr, sizeof datestr, "%F %T", localtime_r(&t, &lt));
	snprintf(res, sizeof res, "QUOTE,%s.%03d,%s|%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%.2f,%d,%.2f,"
		"%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%.2f,%.2f,%d,%d,%.2f,%.2f,%d,%d,%.2f,%.2f,"
		"%d,%d,%.2f,%.2f,%d,%d",
		datestr,
		quote->m_nMSec,
		quote->thyquote.m_cHYDM,
		quote->thyquote.m_cJYS,
		quote->thyquote.m_dZJSJ,
		quote->thyquote.m_dJJSJ,
		quote->thyquote.m_dCJJJ,
		quote->thyquote.m_dZSP,
		quote->thyquote.m_dJSP,
		quote->thyquote.m_dJKP,
		quote->thyquote.m_nZCCL,
		quote->thyquote.m_nCCL,
		quote->thyquote.m_dZXJ,
		quote->thyquote.m_nCJSL,
		quote->thyquote.m_dCJJE,
		quote->thyquote.m_dZGBJ,
		quote->thyquote.m_dZDBJ,
		quote->thyquote.m_dZGJ,
		quote->thyquote.m_dZDJ,
		quote->thyquote.m_dZXSD,
		quote->thyquote.m_dJXSD,
		quote->thyquote.m_dMRJG1,
		quote->thyquote.m_dMCJG1,
		quote->thyquote.m_nMRSL1,
		quote->thyquote.m_nMCSL1,
		quote->thyquote.m_dMRJG2,
		quote->thyquote.m_dMCJG2,
		quote->thyquote.m_nMRSL2,
		quote->thyquote.m_nMCSL2,
		quote->thyquote.m_dMRJG3,
		quote->thyquote.m_dMCJG3,
		quote->thyquote.m_nMRSL3,
		quote->thyquote.m_nMCSL3,
		quote->thyquote.m_dMRJG4,
		quote->thyquote.m_dMCJG4,
		quote->thyquote.m_nMRSL4,
		quote->thyquote.m_nMCSL4,
		quote->thyquote.m_dMRJG5,
		quote->thyquote.m_dMCJG5,
		quote->thyquote.m_nMRSL5,
		quote->thyquote.m_nMCSL5);
	out2rmp(res);
	return 0;
}

static int load_module(void) {
	if (msgs_hook(&default_msgs, quote_exec, NULL) == -1)
		return MODULE_LOAD_FAILURE;
	return register_application(app, quote_exec, desc, fmt, mod_info->self);
}

static int unload_module(void) {
	msgs_unhook(&default_msgs, quote_exec);
	return unregister_application(app);
}

static int reload_module(void) {
	return MODULE_LOAD_SUCCESS;
}

MODULE_INFO(load_module, unload_module, reload_module, "Quotes Application");

