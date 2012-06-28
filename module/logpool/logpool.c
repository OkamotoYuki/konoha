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

#include "konoha2/konoha2.h"
#include "konoha2/logger.h"
#include "konoha2/konoha2_local.h"
#include <logpool/logpool.h>

/* ************************************************************************ */

typedef struct ctxlogpool_t {
	kmodlocal_t h;
	logpool_t *logpool;
} ctxlogpool_t;

typedef struct kmodlogpool_t {
	kmodshare_t h;
	char trace[16];
} kmodlogpool_t;

#define kmodlogpool ((kmodlogpool_t*)_ctx->modshare[MOD_logger])
#define ctxlogpool  ((ctxlogpool_t*)_ctx->modlocal[MOD_logger])

static uintptr_t logpool_Ktrace(CTX, klogconf_t *logconf, ...)
{
	if(TFLAG_is(int, logconf->policy, LOGPOL_INIT)) {
		TFLAG_set(int,logconf->policy,LOGPOL_INIT,0);
	}
	va_list ap;
	va_start(ap, logconf);
	logpool_record_ap(ctxlogpool->logpool, logconf, 0, kmodlogpool->trace, ap);
	va_end(ap);
	return 0;// FIXME reference to log
}

static void ctxlogpool_reftrace(CTX, struct kmodlocal_t *baseh)
{
}

static void ctxlogpool_free(CTX, struct kmodlocal_t *baseh)
{
	ctxlogpool_t *base = (ctxlogpool_t*)baseh;
	logpool_close(base->logpool);
	KFREE(base, sizeof(ctxlogpool_t));
}

static void kmodlogpool_setup(CTX, struct kmodshare_t *def, int newctx)
{
	if(newctx) {
#define DEFAULT_SERVER "127.0.0.1"
		char *serverinfo = getenv("LOGPOOL_SERVER");
		char host[128] = {0};
		int  port = 14801;
		if (serverinfo) {
			char *pos;
			if ((pos = strchr(serverinfo, ':')) != NULL) {
				port = strtol(pos+1, NULL, 10);
				memcpy(host, serverinfo, pos - serverinfo);
			}
		} else {
			memcpy(host, DEFAULT_SERVER, strlen(DEFAULT_SERVER));
		}
		ctxlogpool_t *base = (ctxlogpool_t*)calloc(sizeof(ctxlogpool_t), 1);
		base->h.reftrace = ctxlogpool_reftrace;
		base->h.free     = ctxlogpool_free;
		base->logpool    = logpool_open_trace(NULL, host, port);
		_ctx->modlocal[MOD_logger] = (kmodlocal_t*)base;
	}
}

static void kmodlogpool_reftrace(CTX, struct kmodshare_t *baseh)
{
}

void MODLOGGER_free(CTX, kcontext_t *ctx)
{
	if(IS_ROOTCTX(ctx)) {
		logpool_close(ctxlogpool->logpool);
		free(ctxlogpool/*, sizeof(ctxlogpool_t)*/);
		free(kmodlogpool/*, sizeof(kmodshare_t)*/);
		logpool_global_exit();
	}
}

void MODLOGGER_init(CTX, kcontext_t *ctx)
{
	logpool_global_init(LOGPOOL_TRACE);
	kmodlogpool_t *base = (kmodlogpool_t*)calloc(sizeof(kmodlogpool_t), 1);
	base->h.name     = "verbose";
	base->h.setup    = kmodlogpool_setup;
	base->h.reftrace = kmodlogpool_reftrace;
	unsigned i = 0, ch = 0;
	unsigned t = rand();
	ch = t % 26;
	base->trace[i] = 'A' + ch;
	for(i = 1; i < 16; i++) {
		t = t / 36;
		if (t == 0)
			t = rand();
		ch = t % 36;
		base->trace[i] = (ch < 10) ? '0' + ch : 'A' + (ch - 10);
	}
	if (IS_ROOTCTX(ctx)) {
		kmodlogpool_setup(_ctx, (kmodshare_t*)base, 1);
	}
	Konoha_setModule(MOD_logger, (kmodshare_t*)base, 0);
	((struct _klib2*)_ctx->lib2)->Ktrace = logpool_Ktrace;
}

