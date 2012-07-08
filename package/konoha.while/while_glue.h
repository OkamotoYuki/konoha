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

#ifndef WHILE_GLUE_H_
#define WHILE_GLUE_H_

// --------------------------------------------------------------------------

static	kbool_t while_initPackage(CTX, kNameSpace *ks, int argc, const char**args, kline_t pline)
{
	return true;
}

static kbool_t while_setupPackage(CTX, kNameSpace *ks, kline_t pline)
{
	return true;
}

// --------------------------------------------------------------------------

static KMETHOD StmtTyCheck_while(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_StmtTyCheck(stmt, gma);
	DBG_P("while statement .. ");
	int ret = false;
	if(SUGAR Stmt_tyCheckExpr(_ctx, stmt, KW_ExprPattern, gma, TY_Boolean, 0)) {
		kBlock *bk = kStmt_block(stmt, KW_BlockPattern, K_NULLBLOCK);
		ret = SUGAR Block_tyCheckAll(_ctx, bk, gma);
		kStmt_typed(stmt, LOOP);
	}
	RETURNb_(ret);
}

static KMETHOD StmtTyCheck_for(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_StmtTyCheck(stmt, gma);
	DBG_P("for statement .. ");
	int ret = false;
	if(SUGAR Stmt_tyCheckExpr(_ctx, stmt, KW_ExprPattern, gma, TY_Boolean, 0)) {
		kBlock *bk = kStmt_block(stmt, KW_BlockPattern, K_NULLBLOCK);
		ret = SUGAR Block_tyCheckAll(_ctx, bk, gma);
		kStmt_typed(stmt, LOOP);
	}
	RETURNb_(ret);
}

static inline kStmt* kStmt_getParentNULL(kStmt *stmt)
{
	return stmt->parentNULL->parentNULL;
}

static KMETHOD StmtTyCheck_break(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_StmtTyCheck(stmt, gma);
	kStmt *p = stmt;
	while((p = kStmt_getParentNULL(p)) != NULL) {
		if(FLAG_is(p->syn->flag, SYNFLAG_StmtJumpSkip)) {
			kObject_setObject(stmt, stmt->syn->kw, p);
			kStmt_typed(stmt, JUMP);
			RETURNb_(true);
		}
	}
	SUGAR Stmt_p(_ctx, stmt, NULL, ERR_, "break statement not within a loop");
	RETURNb_(false);
}

static KMETHOD StmtTyCheck_continue(CTX, ksfp_t *sfp _RIX)
{
	USING_SUGAR;
	VAR_StmtTyCheck(stmt, gma);
	DBG_P("continue statement .. ");
	kStmt *p = stmt;
	while((p = kStmt_getParentNULL(p)) != NULL) {
		if(FLAG_is(p->syn->flag, SYNFLAG_StmtJumpAhead)) {
			kObject_setObject(stmt, stmt->syn->kw, p);
			kStmt_typed(stmt, JUMP);
			RETURNb_(true);
		}
	}
	SUGAR Stmt_p(_ctx, stmt, NULL, ERR_, "continue statement not within a loop");
	RETURNb_((false));
}

#define _LOOP .flag = (SYNFLAG_StmtJumpAhead|SYNFLAG_StmtJumpSkip)

static kbool_t while_initNameSpace(CTX,  kNameSpace *ks, kline_t pline)
{
	USING_SUGAR;
	KDEFINE_SYNTAX SYNTAX[] = {
		{ .kw = SYM_("while"), _LOOP, StmtTyCheck_(while), .rule = "\"while\" \"(\" $expr \")\" $block", },
		{ .kw = SYM_("break"), StmtTyCheck_(break), .rule = "\"break\" [ $USYMBOL ]", },
		{ .kw = SYM_("continue"), StmtTyCheck_(continue), .rule = "\"continue\" [ $USYMBOL ]", },
		{ .kw = SYM_("for"), _LOOP, StmtTyCheck_(for), .rule = "\"for\" \"(\" var: $block \";\" $expr \";\" each: $block \")\" $block", },
		{ .kw = KW_END, },
	};
	SUGAR NameSpace_defineSyntax(_ctx, ks, SYNTAX);
	return true;
}

static kbool_t while_setupNameSpace(CTX, kNameSpace *ks, kline_t pline)
{
	return true;
}


#endif /* WHILE_GLUE_H_ */
