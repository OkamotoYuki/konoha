/****************************************************************************
 * Copyright (c) 2012, the Konoha project authors. All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************/

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "http_log.h"
#include "ap_config.h"
#include "apr_strings.h"
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_VERSION
#include "apache_glue.h"

static void Request_init(CTX, kObject *po, void *conf)
{
	(void)_ctx;
	((kRequest*)po)->r = (request_rec *) conf;
}

static void Request_free(CTX, kObject *po)
{
	(void)_ctx;
((kRequest*)po)->r = NULL;
}

static void kapacheshare_setup(CTX, struct kmodshare_t *def, int newctx) {}
static void kapacheshare_reftrace(CTX, struct kmodshare_t *baseh) {}
static void kapacheshare_free(CTX, struct kmodshare_t *baseh)
{
	KFREE(baseh, sizeof(kapacheshare_t));
}


static kbool_t apache_initPackage(CTX, kKonohaSpace *ks, int argc, const char**args, kline_t pline)
{
	static KDEFINE_CLASS Def = {
		STRUCTNAME(Request),
		.init = Request_init,
		.free = Request_free,
	};

	kapacheshare_t *base = (kapacheshare_t*)KCALLOC(sizeof(kapacheshare_t), 1);
	base->h.name     = "apache";
	base->h.setup    = kapacheshare_setup;
	base->h.reftrace = kapacheshare_reftrace;
	base->h.free     = kapacheshare_free;
	Konoha_setModule(MOD_APACHE, &base->h, pline);
	base->cRequest = Konoha_addClassDef(ks->packid, ks->packdom, NULL, &Def, 0);
	KDEFINE_INT_CONST IntData[] = {
#define DEFINE_KEYWORD(KW) {#KW, TY_Int, KW}
		{"APACHE_OK", TY_Int, OK},
		{"APLOG_EMERG", TY_Int, APLOG_EMERG},
		{"APLOG_ALERT", TY_Int, APLOG_ALERT},
		{"APLOG_CRIT", TY_Int, APLOG_CRIT},
		{"APLOG_ERR", TY_Int, APLOG_ERR},
		{"APLOG_WARNING", TY_Int, APLOG_WARNING},
		{"APLOG_NOTICE", TY_Int, APLOG_NOTICE},
		{"APLOG_INFO", TY_Int, APLOG_INFO},
		{"APLOG_DEBUG", TY_Int, APLOG_DEBUG},
		{NULL, 0, 0}
	};
	kKonohaSpace_loadConstData(ks, IntData, 0);
	return true;
}

static kbool_t apache_setupPackage(CTX, kKonohaSpace *ks, kline_t pline)
{
	return true;
}

static kbool_t apache_initKonohaSpace(CTX,  kKonohaSpace *ks, kline_t pline)
{
	return true;
}

static kbool_t apache_setupKonohaSpace(CTX, kKonohaSpace *ks, kline_t pline)
{
	return true;
}

KDEFINE_PACKAGE* apache_init(void)
{
	static KDEFINE_PACKAGE d = {
		KPACKNAME("apache", "1.0"),
		.initPackage = apache_initPackage,
		.setupPackage = apache_setupPackage,
		.initKonohaSpace = apache_initKonohaSpace,
		.setupKonohaSpace = apache_setupKonohaSpace,
	};
	return &d;
}

