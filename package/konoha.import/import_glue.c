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

#include <konoha2/konoha2.h>
#include <konoha2/sugar.h>
#include <stdio.h>

static KMETHOD StmtTyCheck_import(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	int ret = false;
	VAR_StmtTyCheck(stmt, gma);
	kTokenArray *tls = (kTokenArray *) kObject_getObjectNULL(stmt, KW_ToksPattern);
	if (tls == NULL) {
		RETURNb_(false);
	}
	kwb_t wb;
	kwb_init(&(_ctx->stack->cwb), &wb);
	int i = 0;
	if (i + 2 < kArray_size(tls)) {
		for (; i < kArray_size(tls)-1; i+=2) {
			/* name . */
			kToken *tk  = tls->toks[i+0];
			kToken *dot = tls->toks[i+1];
			assert(tk->kw  == TK_SYMBOL);
			assert(dot->kw == KW_DOT);
			kwb_write(&wb, S_text(tk->text), S_size(tk->text));
			kwb_putc(&wb, '.');
		}
	}
	kString *name = tls->toks[i]->text;
	kwb_write(&wb, S_text(name), S_size(name));
	kString *pkgname = new_kString(kwb_top(&wb, 1), kwb_bytesize(&wb), 0);
	kNameSpace *ks = (kNameSpace *) gma->genv->ks;
	struct _ksyntax *syn1 = (struct _ksyntax*) SYN_(ks, KW_ExprMethodCall);
	struct _kToken *tkImport = new_W(Token, 0);
	kExpr *ePKG = new_ConstValue(TY_String, pkgname);
	tkImport->kw = MN_("import");
	kExpr *expr = SUGAR new_ConsExpr(_ctx, syn1, 3,
			tkImport, new_ConstValue(O_cid(ks), ks), ePKG);
	kObject_setObject(stmt, KW_ExprPattern, expr);
	ret = SUGAR Stmt_tyCheckExpr(_ctx, stmt, KW_ExprPattern, gma, TY_Boolean, 0);
	if (ret) {
		kStmt_typed(stmt, EXPR);
	}
	RETURNb_(ret);
}

// --------------------------------------------------------------------------

static kbool_t import_initPackage(CTX, kNameSpace *ks, int argc, const char**args, kline_t pline)
{
	return true;
}

static kbool_t import_setupPackage(CTX, kNameSpace *ks, kline_t pline)
{
	return true;
}

static kbool_t import_initNameSpace(CTX, kNameSpace *ks, kline_t pline)
{
	USING_SUGAR;
	KDEFINE_SYNTAX SYNTAX[] = {
		{ .kw = SYM_("import"), .rule = "\"import\" $toks", TopStmtTyCheck_(import)},
		{ .kw = KW_END, },
	};
	SUGAR NameSpace_defineSyntax(_ctx, ks, SYNTAX);
	return true;
}

static kbool_t import_setupNameSpace(CTX, kNameSpace *ks, kline_t pline)
{
	return true;
}

KDEFINE_PACKAGE* import_init(void)
{
	static KDEFINE_PACKAGE d = {
		KPACKNAME("import", "1.0"),
		.initPackage = import_initPackage,
		.setupPackage = import_setupPackage,
		.initNameSpace = import_initNameSpace,
		.setupNameSpace = import_setupNameSpace,
	};
	return &d;
}
