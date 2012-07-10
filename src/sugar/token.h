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

/* ************************************************************************ */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */

static int parseINDENT(CTX, struct _kToken *tk, tenv_t *tenv, int pos)
{
	int ch, indent = 0;
	while((ch = tenv->source[pos++]) != 0) {
		if(ch == '\t') { indent += tenv->indent_tab; }
		else if(ch == ' ') { indent += 1; }
		break;
	}
	if(IS_NOTNULL(tk)) {
		kToken_setUnresolved(tk, true);  // to avoid indent within tree tokens
		tk->kw = TK_INDENT;
		tk->indent = 0; /* indent FIXME: Debug/Parser/LineNumber.k (Failed) */
	}
	return pos-1;
}

static int parseNL(CTX, struct _kToken *tk, tenv_t *tenv, int pos)
{
	tenv->uline += 1;
	return parseINDENT(_ctx, tk, tenv, pos+1);
}

static int parseNUM(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	int ch, pos = tok_start, dot = 0;
	const char *ts = tenv->source;
	while((ch = ts[pos++]) != 0) {
		if(ch == '_') continue; // nothing
		if(ch == '.') {
			if(!isdigit(ts[pos])) {
				break;
			}
			dot++;
			continue;
		}
		if((ch == 'e' || ch == 'E') && (ts[pos] == '+' || ts[pos] =='-')) {
			pos++;
			continue;
		}
		if(!isalnum(ch)) break;
	}
	if(IS_NOTNULL(tk)) {
		KSETv(tk->text, new_kString(ts + tok_start, (pos-1)-tok_start, SPOL_ASCII));
		tk->kw = (dot == 0) ? TK_INT : TK_FLOAT;
	}
	return pos - 1;  // next
}

/**static kbool_t isLowerCaseSymbol(const char *t)
{
	while(t[0] != 0) {
		if(islower(t[0])) return true;
		if(t[0] == '_') {
			t++; continue;
		}
		return false;
	}
	return true;
}**/

static kbool_t isUpperCaseSymbol(const char *t)
{
	while(t[0] != 0) {
		if(isupper(t[0])) return true;
		if(t[0] == '_') {
			t++; continue;
		}
		break;
	}
	return false;
}

static void Token_setSymbolText(CTX, struct _kToken *tk, const char *t, size_t len)
{
	if(IS_NOTNULL(tk)) {
		ksymbol_t kw = ksymbolA(t, len, SYM_NONAME);
		kToken_setUnresolved(tk, true);
		if(kw == SYM_UNMASK(kw)) {
			KSETv(tk->text, SYM_s(kw));
		}
		else {
			KSETv(tk->text, new_kString(t, len, SPOL_ASCII));
		}
		tk->kw = TK_SYMBOL;
	}
}

static int parseSYMBOL(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	int ch, pos = tok_start;
	const char *ts = tenv->source;
	while((ch = ts[pos++]) != 0) {
		if(ch == '_' || isalnum(ch)) continue; // nothing
		break;
	}
	Token_setSymbolText(_ctx, tk, ts + tok_start, (pos-1)-tok_start);
	return pos - 1;  // next
}

static int parseOP1(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	Token_setSymbolText(_ctx, tk,  tenv->source + tok_start, 1);
	return tok_start+1;
}

static int parseOP(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	int ch, pos = tok_start;
	while((ch = tenv->source[pos++]) != 0) {
		if(isalnum(ch)) break;
		switch(ch) {
			case '<': case '>': case '@': case '$': case '#':
			case '+': case '-': case '*': case '%': case '/':
			case '=': case '&': case '?': case ':': case '.':
			case '^': case '!': case '~': case '|':
			continue;
		}
		break;
	}
	Token_setSymbolText(_ctx, tk, tenv->source + tok_start, (pos-1)-tok_start);
	return pos-1;
}

static int parseLINE(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	int ch, pos = tok_start;
	while((ch = tenv->source[pos++]) != 0) {
		if(ch == '\n') break;
	}
	return pos-1;/*EOF*/
}

static int parseCOMMENT(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	int ch, prev = 0, level = 1, pos = tok_start + 2;
	/*@#nnnn is line number */
	if(tenv->source[pos] == '@' && tenv->source[pos+1] == '#' && isdigit(tenv->source[pos+2])) {
		tenv->uline >>= (sizeof(kshort_t)*8);
		tenv->uline = (tenv->uline<<(sizeof(kshort_t)*8))  | (kshort_t)strtoll(tenv->source + pos + 2, NULL, 10);
	}
	while((ch = tenv->source[pos++]) != 0) {
		if(ch == '\n') {
			tenv->uline += 1;
		}
		if(prev == '*' && ch == '/') {
			level--;
			if(level == 0) return pos;
		}else if(prev == '/' && ch == '*') {
			level++;
		}
		prev = ch;
	}
	if(IS_NOTNULL(tk)) {
		Token_pERR(_ctx, tk, "must close with */");
	}
	return pos-1;/*EOF*/
}

static int parseSLASH(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	const char *ts = tenv->source + tok_start;
	if(ts[1] == '/') {
		return parseLINE(_ctx, tk, tenv, tok_start);
	}
	if(ts[1] == '*') {
		return parseCOMMENT(_ctx, tk, tenv, tok_start);
	}
	return parseOP(_ctx, tk, tenv, tok_start);
}

static int parseDoubleQuotedText(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	kwb_t wb;
	kwb_init(&(_ctx->stack->cwb), &wb);
	int ch, prev = '"', pos = tok_start + 1, next;
	while((ch = tenv->source[pos++]) != 0) {
		if(ch == '\n') {
			break;
		}
		if(ch == '"' && prev != '\\') {
			if(IS_NOTNULL(tk)) {
				size_t length = kwb_bytesize(&wb);
				KSETv(tk->text, new_kString(kwb_top(&wb, 1), length, 0));
				tk->kw = TK_TEXT;
			}
			kwb_free(&wb);
			return pos;
		}
		if(ch == '\\' && (next = tenv->source[pos]) != 0) {
			switch (next) {
			case 'n':  ch = '\n'; pos++; break;
			case 't':  ch = '\t'; pos++; break;
			case 'r':  ch = '\r'; pos++; break;
			case '\\': ch = '\\'; pos++; break;
			case '"':  ch = '\"'; pos++; break;
			}
		}
		prev = ch;
		kwb_putc(&wb, ch);
	}
	if(IS_NOTNULL(tk)) {
		Token_pERR(_ctx, tk, "must close with \"");
	}
	kwb_free(&wb);
	return pos-1;
}

static int parseSKIP(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	return tok_start+1;
}

static int parseUndefinedToken(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	if(IS_NOTNULL(tk)) {
		Token_pERR(_ctx, tk, "undefined token character: %c (ascii=%x)", tenv->source[tok_start], tenv->source[tok_start]);
	}
	while(tenv->source[++tok_start] != 0);
	return tok_start;
}

static int parseLazyBlock(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start);

static const CFuncTokenize MiniKonohaTokenMatrix[] = {
#define _NULL      0
	parseSKIP,
#define _UNDEF     1
	parseSKIP,
#define _DIGIT     2
	parseNUM,
#define _UALPHA    3
	parseSYMBOL,
#define _LALPHA    4
	parseSYMBOL,
#define _MULTI     5
	parseUndefinedToken,
#define _NL        6
	parseNL,
#define _TAB       7
	parseSKIP,
#define _SP_       8
	parseSKIP,
#define _LPAR      9
	parseOP1,
#define _RPAR      10
	parseOP1,
#define _LSQ       11
	parseOP1,
#define _RSQ       12
	parseOP1,
#define _LBR       13
	parseLazyBlock,
#define _RBR       14
	parseOP1,
#define _LT        15
	parseOP,
#define _GT        16
	parseOP,
#define _QUOTE     17
	parseUndefinedToken,
#define _DQUOTE    18
	parseDoubleQuotedText,
#define _BKQUOTE   19
	parseUndefinedToken,
#define _OKIDOKI   20
	parseOP,
#define _SHARP     21
	parseOP,
#define _DOLLAR    22
	parseOP,
#define _PER       23
	parseOP,
#define _AND       24
	parseOP,
#define _STAR      25
	parseOP,
#define _PLUS      26
	parseOP,
#define _COMMA     27
	parseOP1,
#define _MINUS     28
	parseOP,
#define _DOT       29
	parseOP,
#define _SLASH     30
	parseSLASH,
#define _COLON     31
	parseOP,
#define _SEMICOLON 32
	parseOP1,
#define _EQ        33
	parseOP,
#define _QUESTION  34
	parseOP,
#define _AT_       35
	parseOP1,
#define _VAR       36
	parseOP,
#define _CHILDER   37
	parseOP,
#define _BKSLASH   38
	parseUndefinedToken,
#define _HAT       39
	parseOP,
#define _UNDER     40
	parseSYMBOL,
#define KCHAR_MAX  41
};

static const char cMatrix[128] = {
	0/*nul*/, 1/*soh*/, 1/*stx*/, 1/*etx*/, 1/*eot*/, 1/*enq*/, 1/*ack*/, 1/*bel*/,
	1/*bs*/,  _TAB/*ht*/, _NL/*nl*/, 1/*vt*/, 1/*np*/, 1/*cr*/, 1/*so*/, 1/*si*/,
	/*	020 dle  021 dc1  022 dc2  023 dc3  024 dc4  025 nak  026 syn  027 etb*/
	1, 1, 1, 1,     1, 1, 1, 1,
	/*	030 can  031 em   032 sub  033 esc  034 fs   035 gs   036 rs   037 us*/
	1, 1, 1, 1,     1, 1, 1, 1,
	/*040 sp   041  !   042  "   043  #   044  $   045  %   046  &   047  '*/
	_SP_, _OKIDOKI, _DQUOTE, _SHARP, _DOLLAR, _PER, _AND, _QUOTE,
	/*050  (   051  )   052  *   053  +   054  ,   055  -   056  .   057  /*/
	_LPAR, _RPAR, _STAR, _PLUS, _COMMA, _MINUS, _DOT, _SLASH,
	/*060  0   061  1   062  2   063  3   064  4   065  5   066  6   067  7 */
	_DIGIT, _DIGIT, _DIGIT, _DIGIT,  _DIGIT, _DIGIT, _DIGIT, _DIGIT,
	/*	070  8   071  9   072  :   073  ;   074  <   075  =   076  >   077  ? */
	_DIGIT, _DIGIT, _COLON, _SEMICOLON, _LT, _EQ, _GT, _QUESTION,
	/*100  @   101  A   102  B   103  C   104  D   105  E   106  F   107  G */
	_AT_, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA,
	/*110  H   111  I   112  J   113  K   114  L   115  M   116  N   117  O */
	_UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA,
	/*120  P   121  Q   122  R   123  S   124  T   125  U   126  V   127  W*/
	_UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA, _UALPHA,
	/*130  X   131  Y   132  Z   133  [   134  \   135  ]   136  ^   137  _*/
	_UALPHA, _UALPHA, _UALPHA, _LSQ, _BKSLASH, _RSQ, _HAT, _UNDER,
	/*140  `   141  a   142  b   143  c   144  d   145  e   146  f   147  g*/
	_BKQUOTE, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA,
	/*150  h   151  i   152  j   153  k   154  l   155  m   156  n   157  o*/
	_LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA,
	/*160  p   161  q   162  r   163  s   164  t   165  u   166  v   167  w*/
	_LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA, _LALPHA,
	/*170  x   171  y   172  z   173  {   174  |   175  }   176  ~   177 del*/
	_LALPHA, _LALPHA, _LALPHA, _LBR, _VAR, _RBR, _CHILDER, 1,
};

static int kchar(const char *t, int pos)
{
	int ch = t[pos];
	return (ch < 0) ? _MULTI : cMatrix[ch];
}


static int callFuncTokenize(CTX, kFunc *fo, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	// The above string operation is bad thing. Don't repeat it
	struct _kString *preparedString = (struct _kString*)tenv->preparedString;
	preparedString->text = tenv->source + tok_start;
	preparedString->bytesize = tenv->source_length - tok_start;
	BEGIN_LOCAL(lsfp, K_CALLDELTA + 2);
	KSETv(lsfp[K_CALLDELTA+0].o, fo->self);
	KSETv(lsfp[K_CALLDELTA+1].o, (kObject*)tk);
	KSETv(lsfp[K_CALLDELTA+2].s, preparedString);
	KCALL(lsfp, 0, fo->mtd, 2, knull(CT_Int));
	END_LOCAL();
	int pos = lsfp[0].ivalue;
	if(pos > tok_start) { // check new lines
		int i;
		const char *t = tenv->source;
		for(i = tok_start; i < pos; i++) {
			if(t[0] == '\n') tenv->uline += 1;
		}
	}
	return pos;
}

static int tokenizeEach(CTX, int kchar, struct _kToken* tk, tenv_t *tenv, int tok_start)
{
	int pos;
	if(tenv->func != NULL && tenv->func[kchar] != NULL) {
		kFunc *fo = tenv->func[kchar];
		if(IS_Array(fo)) {
			kArray *a = (kArray*)fo;
			int i;
			for(i = kArray_size(a); i >= 0; i--) {
				pos = callFuncTokenize(_ctx, a->funcs[i], tk, tenv, tok_start);
				if(pos > tok_start) return pos;
			}
			fo = a->funcs[0];
		}
		pos = callFuncTokenize(_ctx, fo, tk, tenv, tok_start);
		if(pos > tok_start) return pos;
	}
	pos = tenv->cfunc[kchar](_ctx, tk, tenv, tok_start);
	return pos;
}

static void tokenize(CTX, tenv_t *tenv)
{
	int ch, pos = 0;
	struct _kToken *tk = new_W(Token, 0);
	tk->uline = tenv->uline;
	pos = parseINDENT(_ctx, tk, tenv, pos);
	while((ch = kchar(tenv->source, pos)) != 0) {
		if(tk->kw != 0) {
			kArray_add(tenv->list, tk);
			tk = new_W(Token, 0);
			tk->uline = tenv->uline;
		}
		int pos2 = tokenizeEach(_ctx, ch, tk, tenv, pos);
		assert(pos2 > pos);
		pos = pos2;
	}
	if(tk->kw != 0) {  // FIXME: Memory Leaks ???
		kArray_add(tenv->list, tk);
	}
}

static int parseLazyBlock(CTX, struct _kToken *tk, tenv_t *tenv, int tok_start)
{
	int ch, level = 1, pos = tok_start + 1;
	while((ch = kchar(tenv->source, pos)) != 0) {
		if(ch == _RBR/*}*/) {
			level--;
			if(level == 0) {
				if(IS_NOTNULL(tk)) {
					KSETv(tk->text, new_kString(tenv->source + tok_start + 1, ((pos-2)-(tok_start)+1), 0));
					tk->kw = TK_CODE;
				}
				return pos + 1;
			}
			pos++;
		}
		else if(ch == _LBR/*'{'*/) {
			level++; pos++;
		}
		else {
			pos = tokenizeEach(_ctx, ch, (struct _kToken*)K_NULLTOKEN, tenv, pos);
		}
	}
	if(IS_NOTNULL(tk)) {
		Token_pERR(_ctx, tk, "must close with }");
	}
	return pos-1;
}

static const CFuncTokenize *NameSpace_tokenMatrix(CTX, kNameSpace *ns)
{
	if(ns->tokenMatrix == NULL) {
		DBG_ASSERT(KCHAR_MAX * sizeof(CFuncTokenize) == sizeof(MiniKonohaTokenMatrix));
		CFuncTokenize *tokenMatrix = (CFuncTokenize*)KMALLOC(SIZEOF_TOKENMATRIX);
		if(ns->parentNULL != NULL && ns->parentNULL->tokenMatrix != NULL) {
			memcpy(tokenMatrix, ns->parentNULL->tokenMatrix, SIZEOF_TOKENMATRIX);
		}
		else {
			memcpy(tokenMatrix, MiniKonohaTokenMatrix, sizeof(MiniKonohaTokenMatrix));
			bzero(tokenMatrix + KCHAR_MAX, sizeof(MiniKonohaTokenMatrix));
		}
		((struct _kNameSpace*)ns)->tokenMatrix = (const CFuncTokenize*)tokenMatrix;
	}
	return ns->tokenMatrix;
}

static kFunc **NameSpace_tokenFuncMatrix(CTX, kNameSpace *ns)
{
	kFunc **funcMatrix = (kFunc**)NameSpace_tokenMatrix(_ctx, ns);
	return funcMatrix + KCHAR_MAX;
}

static void NameSpace_setTokenizeFunc(CTX, kNameSpace *ns, int ch, CFuncTokenize cfunc, kFunc *funcTokenize, int isAddition)
{
	int kchar = (ch < 0) ? _MULTI : cMatrix[ch];
	if(cfunc != NULL) {
		CFuncTokenize *funcMatrix = (CFuncTokenize *)NameSpace_tokenMatrix(_ctx, ns);
		funcMatrix[kchar] = cfunc;
	}
	else {
		kFunc ** funcMatrix = NameSpace_tokenFuncMatrix(_ctx, ns);
		if(funcMatrix[kchar] == NULL) {
			KINITv(funcMatrix[kchar], funcTokenize);
		}
		else {
			if(isAddition) {
				kArray *a = (kArray*)funcMatrix[kchar];
				if(!IS_Array(a)) {
					a = new_(Array, 0);
					kArray_add(a, funcMatrix[kchar]);
					KSETv(funcMatrix[kchar], (kFunc*)a);
				}
				kArray_add(a, funcTokenize);
			}
			else {
				KSETv(funcMatrix[kchar], funcTokenize);
			}
		}
	}
}

static void NameSpace_tokenize(CTX, kNameSpace *ns, const char *source, kline_t uline, kArray *a)
{
	size_t i, pos = kArray_size(a);
	tenv_t tenv = {
		.source = source,
		.source_length = strlen(source),
		.uline  = uline,
		.list   = a,
		.indent_tab = 4,
		.cfunc   = (ns == NULL) ? MiniKonohaTokenMatrix : NameSpace_tokenMatrix(_ctx, ns),
	};
	INIT_GCSTACK();
	kString *preparedString = new_kString(tenv.source, tenv.source_length, SPOL_ASCII|SPOL_TEXT|SPOL_NOPOOL);
	PUSH_GCSTACK(preparedString);
	tenv.preparedString = preparedString;
	if(ns != NULL) {
		tenv.func = NameSpace_tokenFuncMatrix(_ctx, ns);
	}
	tokenize(_ctx, &tenv);
	RESET_GCSTACK();
	if(uline == 0) {
		for(i = pos; i < kArray_size(a); i++) {
			a->Wtoks[i]->uline = 0;
		}
	}
}

// --------------------------------------------------------------------------

static kbool_t makeSyntaxRule(CTX, kArray *tls, int s, int e, kArray *adst);
#define kToken_topch2(tt, tk) ((tk->kw == tt && (S_size((tk)->text) == 1)) ? S_text((tk)->text)[0] : 0)

static int findCloseChar(CTX, kArray *tls, int s, int e, ksymbol_t tt, int closech)
{
	int i;
	for(i = s; i < e; i++) {
		kToken *tk = tls->toks[i];
		if(kToken_topch2(tt, tk) == closech) return i;
	}
	return e;
}

static kbool_t checkNestedSyntax(CTX, kArray *tls, int *s, int e, ksymbol_t astkw, int opench, int closech)
{
	int i = *s;
	struct _kToken *tk = tls->Wtoks[i];
	int topch = kToken_topch2(tk->kw, tk);
	if(topch == opench) {
		int ne = findCloseChar(_ctx, tls, i+1, e, tk->kw, closech);
		tk->kw = astkw;
		KSETv(tk->sub, new_(TokenArray, 0));
		makeSyntaxRule(_ctx, tls, i+1, ne, tk->sub);
		*s = ne;
		return true;
	}
	return false;
}

static kbool_t makeSyntaxRule(CTX, kArray *tls, int s, int e, kArray *adst)
{
	int i;
	ksymbol_t patternKey = 0;
//	dumpTokenArray(_ctx, 0, tls, s, e);
	for(i = s; i < e; i++) {
		struct _kToken *tk = tls->Wtoks[i];
		int topch = kToken_topch(tk);
		if(tk->kw == TK_INDENT) continue;
		if(tk->kw == TK_TEXT) {
			if(checkNestedSyntax(_ctx, tls, &i, e, AST_PARENTHESIS, '(', ')') ||
				checkNestedSyntax(_ctx, tls, &i, e, AST_BRACKET, '[', ']') ||
				checkNestedSyntax(_ctx, tls, &i, e, AST_BRACE, '{', '}')) {
			}
			else {
				// FIXME: tk->tt = TK_CODE;
				tk->kw = ksymbolA(S_text(tk->text), S_size(tk->text), SYM_NEWID);
			}
			kArray_add(adst, tk);
			continue;
		}
		if(topch == '$' && i+1 < e) {
			tk = tls->Wtoks[++i];
			if(tk->kw == TK_SYMBOL) {
				tk->kw = ksymbolA(S_text(tk->text), S_size(tk->text), SYM_NEWRAW) | KW_PATTERN;
				if(patternKey == 0) patternKey = tk->kw;
				tk->patternKey = patternKey;
				patternKey = 0;
				kArray_add(adst, tk);
				continue;
			}
		}
		else if(topch == '[') {
			if(checkNestedSyntax(_ctx, tls, &i, e, AST_OPTIONAL, '[', ']')) {
				kArray_add(adst, tk);
				continue;
			}
			return false;
		}
		if(tk->kw == TK_SYMBOL && i + 1 < e && kToken_topch(tls->toks[i+1]) == ':') {
			patternKey = ksymbolA(S_text(tk->text), S_size(tk->text), SYM_NEWRAW);
			i++;
			continue;
		}
		Token_pERR(_ctx, tk, "illegal syntax rule: %s", kToken_s(tk));
		return false;
	}
	return true;
}

static void parseSyntaxRule(CTX, const char *rule, kline_t uline, kArray *a)
{
	kArray *tls = ctxsugar->tokens;
	size_t pos = kArray_size(tls);
	NameSpace_tokenize(_ctx, NULL, rule, uline, tls);
	makeSyntaxRule(_ctx, tls, pos, kArray_size(tls), a);
	kArray_clear(tls, pos);
}

/* ------------------------------------------------------------------------ */

#ifdef __cplusplus
}
#endif
