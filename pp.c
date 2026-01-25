#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "config.h"
#include "cc.h"

struct macroparam {
	char *name;
	enum {
		PARAMTOK = 1<<0,  /* the parameter is used normally */
		PARAMSTR = 1<<1,  /* the parameter is used with the '#' operator */
		PARAMVAR = 1<<2,  /* the parameter is __VA_ARGS__ */
	} flags;
};

struct macroarg {
	struct token *token;
	size_t ntoken;
	/* stringized argument */
	struct token str;
};

struct macro {
	enum {
		MACROOBJ,
		MACROFUNC,
	} kind;
	char *name;
	/* whether or not this macro is ineligible for expansion */
	bool hide;
	/* parameters of function-like macro */
	struct macroparam *param;
	size_t nparam;
	/* replacement list */
	struct token *token;
	size_t ntoken;
};

struct hideset {
	struct macro *macro;
	struct hideset *next;
};

struct frame {
	struct token *token;
	size_t ntoken;
	struct macro *macro;
	struct macroarg *arg;
	void *alloc;
};

enum ppflags ppflags;

static struct array ctx;
static struct map macros;
/* number of macros currently undergoing expansion */
static size_t macrodepth;
static bool pp_inited;
static bool pp_no_stdinc;

struct incdir {
	char *path;
	int kind;
};

struct defaction {
	bool undef;
	char *name;
	char *value;
};

static struct array inc_quote;
static struct array inc_system;
static struct array inc_after;
static struct array defactions;
static struct map oncefiles;
static struct map included_once;
static unsigned long pp_counter;
static char pp_date[32];
static char pp_time[32];

static bool
hideset_contains(struct hideset *hs, struct macro *m)
{
	for (; hs; hs = hs->next) {
		if (hs->macro == m)
			return true;
	}
	return false;
}

static struct hideset *
hideset_add(struct hideset *hs, struct macro *m)
{
	struct hideset *n;

	if (!m || hideset_contains(hs, m))
		return hs;
	n = xmalloc(sizeof(*n));
	n->macro = m;
	n->next = hs;
	return n;
}

static struct hideset *
hideset_union(struct hideset *a, struct hideset *b)
{
	for (; b; b = b->next)
		a = hideset_add(a, b->macro);
	return a;
}

static bool macroequal(struct macro *, struct macro *);
static void add_default_includes(void);
static void read_line_tokens(struct array *);
static void expand_tokens(struct token *, size_t, struct array *);
static void mark_defined_operands(struct token *, size_t);
static long long ppeval(struct token *, size_t);
static char *header_from_tokens(struct token *, size_t, bool *);
static void include_file(const char *, bool, const char *, bool);
static const char *toktext(const struct token *);
static void mark_once(const char *);
static bool expand(struct token *);

static bool hideset_contains(struct hideset *, struct macro *);
static struct hideset *hideset_add(struct hideset *, struct macro *);
static struct hideset *hideset_union(struct hideset *, struct hideset *);

static void
add_incdir(struct array *a, const char *path, int kind)
{
	struct incdir *d;

	if (!path || !*path)
		return;
	d = arrayadd(a, sizeof(*d));
	d->path = strdup(path);
	if (!d->path)
		fatal("strdup:");
	d->kind = kind;
}

void
ppaddinc(const char *path, int kind)
{
	switch (kind) {
	case PPINC_QUOTE:
		add_incdir(&inc_quote, path, kind);
		break;
	case PPINC_AFTER:
		add_incdir(&inc_after, path, kind);
		break;
	default:
		add_incdir(&inc_system, path, PPINC_SYSTEM);
		break;
	}
}

void
ppnostdinc(bool on)
{
	pp_no_stdinc = on;
}

static void
defmacro(struct macro *m)
{
	struct mapkey k;
	void **entry;

	mapkey(&k, m->name, strlen(m->name));
	entry = mapput(&macros, &k);
	if (*entry && !macroequal(m, *entry))
		error(&tok.loc, "redefinition of macro '%s'", m->name);
	*entry = m;
}

static struct token *
tokensfromstr(const char *s, size_t *n)
{
	return scantokens("<command-line>", s, n);
}

void
ppdef(const char *name, const char *value)
{
	struct defaction *a;
	struct macro *m;
	struct token *toks;
	size_t ntok;

	if (!pp_inited) {
		a = arrayadd(&defactions, sizeof(*a));
		a->undef = false;
		a->name = strdup(name);
		a->value = value ? strdup(value) : NULL;
		if (!a->name || (value && !a->value))
			fatal("strdup:");
		return;
	}
	m = xmalloc(sizeof(*m));
	memset(m, 0, sizeof(*m));
	m->name = strdup(name);
	if (!m->name)
		fatal("strdup:");
	m->kind = MACROOBJ;
	m->hide = false;
	toks = tokensfromstr(value ? value : "1", &ntok);
	m->token = toks;
	m->ntoken = ntok;
	defmacro(m);
}

static void
ppdefnum(const char *name, unsigned long long v)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%llu", v);
	ppdef(name, buf);
}

static unsigned long long
umaxbits(unsigned bits)
{
	if (bits >= 64)
		return ~0ULL;
	return (1ULL << bits) - 1;
}

static void
ppdefumax(const char *name, unsigned bits, const char *suffix)
{
	char buf[64];
	unsigned long long v = umaxbits(bits);

	if (suffix)
		snprintf(buf, sizeof(buf), "%llu%s", v, suffix);
	else
		snprintf(buf, sizeof(buf), "%llu", v);
	ppdef(name, buf);
}

static void
ppdefsmax(const char *name, unsigned bits)
{
	unsigned long long umax = umaxbits(bits);
	long long smax = (long long)(umax >> 1);
	char buf[64];

	snprintf(buf, sizeof(buf), "%lld", smax);
	ppdef(name, buf);
}

static void
ppdefminfrommax(const char *name, const char *maxname)
{
	char buf[64];

	snprintf(buf, sizeof(buf), "(-%s-1)", maxname);
	ppdef(name, buf);
}

void
ppundef(const char *name)
{
	struct defaction *a;
	struct mapkey k;
	void **entry;
	struct macro *m;

	if (!pp_inited) {
		a = arrayadd(&defactions, sizeof(*a));
		a->undef = true;
		a->name = strdup(name);
		a->value = NULL;
		if (!a->name)
			fatal("strdup:");
		return;
	}
	mapkey(&k, name, strlen(name));
	entry = mapput(&macros, &k);
	m = *entry;
	if (m) {
		free(m->name);
		free(m->param);
		free(m->token);
		free(m);
		*entry = NULL;
	}
}

void
ppinit(void)
{
	time_t now;
	struct tm *tm;
	struct defaction *a;
	size_t i;

	mapinit(&macros, 64);
	mapinit(&oncefiles, 64);
	mapinit(&included_once, 64);
	pp_inited = true;

	now = time(NULL);
	tm = localtime(&now);
	if (tm) {
		strftime(pp_date, sizeof(pp_date), "%b %d %Y", tm);
		strftime(pp_time, sizeof(pp_time), "%H:%M:%S", tm);
	}

	/* basic predefined macros */
	ppdef("__STDC__", "1");
	ppdef("__STDC_VERSION__", "199901L");
	ppdef("__STDC_HOSTED__", "1");
	ppdef("__CHAR_BIT__", "8");
	ppdef("__ORDER_LITTLE_ENDIAN__", "1234");
	ppdef("__ORDER_BIG_ENDIAN__", "4321");
	ppdef("__ORDER_PDP_ENDIAN__", "3412");
	ppdef("__BYTE_ORDER__", "__ORDER_LITTLE_ENDIAN__");
	ppdef("__FLOAT_WORD_ORDER__", "__ORDER_LITTLE_ENDIAN__");

	ppdefsmax("__SCHAR_MAX__", typechar.size * 8);
	ppdefminfrommax("__SCHAR_MIN__", "__SCHAR_MAX__");
	ppdefumax("__UCHAR_MAX__", typechar.size * 8, "U");
	if (typechar.u.basic.issigned) {
		ppdef("__CHAR_MAX__", "__SCHAR_MAX__");
		ppdef("__CHAR_MIN__", "__SCHAR_MIN__");
	} else {
		ppdef("__CHAR_MAX__", "__UCHAR_MAX__");
		ppdef("__CHAR_MIN__", "0");
		ppdef("__CHAR_UNSIGNED__", "1");
	}
	ppdefsmax("__SHRT_MAX__", typeshort.size * 8);
	ppdefminfrommax("__SHRT_MIN__", "__SHRT_MAX__");
	ppdefumax("__USHRT_MAX__", typeshort.size * 8, "U");
	ppdefsmax("__INT_MAX__", typeint.size * 8);
	ppdefminfrommax("__INT_MIN__", "__INT_MAX__");
	ppdefumax("__UINT_MAX__", typeint.size * 8, "U");
	ppdefsmax("__LONG_MAX__", typelong.size * 8);
	ppdefminfrommax("__LONG_MIN__", "__LONG_MAX__");
	ppdefumax("__ULONG_MAX__", typelong.size * 8, "UL");
	ppdefsmax("__LONG_LONG_MAX__", typellong.size * 8);
	ppdefminfrommax("__LONG_LONG_MIN__", "__LONG_LONG_MAX__");
	ppdefumax("__ULLONG_MAX__", typellong.size * 8, "ULL");
	ppdefsmax("__INTMAX_MAX__", typelong.size * 8);
	ppdefminfrommax("__INTMAX_MIN__", "__INTMAX_MAX__");
	ppdefumax("__UINTMAX_MAX__", typelong.size * 8, "UL");
	ppdefsmax("__INTPTR_MAX__", typenullptr.size * 8);
	ppdefminfrommax("__INTPTR_MIN__", "__INTPTR_MAX__");
	ppdefumax("__UINTPTR_MAX__", typenullptr.size * 8, "UL");
	ppdefsmax("__PTRDIFF_MAX__", typenullptr.size * 8);
	ppdefminfrommax("__PTRDIFF_MIN__", "__PTRDIFF_MAX__");
	ppdefumax("__SIZE_MAX__", typenullptr.size * 8, "UL");
	if (targ && targ->typewchar == &typeuint) {
		ppdefumax("__WCHAR_MAX__", targ->typewchar->size * 8, "U");
		ppdef("__WCHAR_MIN__", "0");
	} else {
		ppdefsmax("__WCHAR_MAX__", targ->typewchar->size * 8);
		ppdefminfrommax("__WCHAR_MIN__", "__WCHAR_MAX__");
	}
	ppdefumax("__WINT_MAX__", typeuint.size * 8, "U");
	ppdef("__WINT_MIN__", "0");

	ppdefsmax("__INT8_MAX__", 8);
	ppdefsmax("__INT16_MAX__", 16);
	ppdefsmax("__INT32_MAX__", 32);
	ppdefsmax("__INT64_MAX__", 64);
	ppdefminfrommax("__INT8_MIN__", "__INT8_MAX__");
	ppdefminfrommax("__INT16_MIN__", "__INT16_MAX__");
	ppdefminfrommax("__INT32_MIN__", "__INT32_MAX__");
	ppdefminfrommax("__INT64_MIN__", "__INT64_MAX__");
	ppdefumax("__UINT8_MAX__", 8, "U");
	ppdefumax("__UINT16_MAX__", 16, "U");
	ppdefumax("__UINT32_MAX__", 32, "U");
	ppdefumax("__UINT64_MAX__", 64, "ULL");

	ppdefnum("__SIZEOF_SHORT__", typeshort.size);
	ppdefnum("__SIZEOF_INT__", typeint.size);
	ppdefnum("__SIZEOF_LONG__", typelong.size);
	ppdefnum("__SIZEOF_LONG_LONG__", typellong.size);
	ppdefnum("__SIZEOF_POINTER__", typenullptr.size);
	ppdefnum("__SIZEOF_SIZE_T__", typenullptr.size);
	ppdefnum("__SIZEOF_PTRDIFF_T__", typenullptr.size);
	ppdefnum("__SIZEOF_WCHAR_T__", targ->typewchar->size);
	ppdefnum("__SIZEOF_WINT_T__", typeuint.size);
	ppdefnum("__SIZEOF_FLOAT__", typefloat.size);
	ppdefnum("__SIZEOF_DOUBLE__", typedouble.size);
	ppdefnum("__SIZEOF_LONG_DOUBLE__", typeldouble.size);

	ppdef("__SIZE_TYPE__", "unsigned long");
	ppdef("__PTRDIFF_TYPE__", "long");
	ppdef("__INTPTR_TYPE__", "long");
	ppdef("__UINTPTR_TYPE__", "unsigned long");
	ppdef("__INTMAX_TYPE__", "long");
	ppdef("__UINTMAX_TYPE__", "unsigned long");
	if (targ && targ->typewchar == &typeuint)
		ppdef("__WCHAR_TYPE__", "unsigned int");
	else
		ppdef("__WCHAR_TYPE__", "int");
	ppdef("__WINT_TYPE__", "unsigned int");
	ppdef("__SIG_ATOMIC_TYPE__", "int");
	ppdef("__CHAR16_TYPE__", "unsigned short");
	ppdef("__CHAR32_TYPE__", "unsigned int");

	if (typenullptr.size == 8 && typelong.size == 8)
		ppdef("__LP64__", "1"), ppdef("_LP64", "1");

	if (targ && strcmp(targ->name, "x86_64-sysv") == 0) {
		ppdef("__x86_64__", "1");
		ppdef("__x86_64", "1");
		ppdef("__amd64__", "1");
		ppdef("__amd64", "1");
		ppdef("__ELF__", "1");
		ppdef("__linux__", "1");
		ppdef("__linux", "1");
	} else if (targ && strcmp(targ->name, "aarch64") == 0) {
		ppdef("__aarch64__", "1");
		ppdef("__AARCH64EL__", "1");
		ppdef("__ELF__", "1");
		ppdef("__linux__", "1");
		ppdef("__linux", "1");
	} else if (targ && strcmp(targ->name, "riscv64") == 0) {
		ppdef("__riscv", "1");
		ppdef("__riscv_xlen", "64");
		ppdef("__ELF__", "1");
		ppdef("__linux__", "1");
		ppdef("__linux", "1");
	}

	for (i = 0; i < defactions.len / sizeof(*a); ++i) {
		a = (struct defaction *)defactions.val + i;
		if (a->undef)
			ppundef(a->name);
		else
			ppdef(a->name, a->value);
	}
	defactions.len = 0;

	add_default_includes();

	next();
}

/* check if two macro definitions are equal, as in C11 6.10.3p2 */
static bool
macroequal(struct macro *m1, struct macro *m2)
{
	struct macroparam *p1, *p2;
	struct token *t1, *t2;

	if (m1->kind != m2->kind)
		return false;
	if (m1->kind == MACROFUNC) {
		if (m1->nparam != m2->nparam)
			return false;
		for (p1 = m1->param, p2 = m2->param; p1 < m1->param + m1->nparam; ++p1, ++p2) {
			if (strcmp(p1->name, p2->name) != 0 || p1->flags != p2->flags)
				return false;
		}
	}
	if (m1->ntoken != m2->ntoken)
		return false;
	for (t1 = m1->token, t2 = m2->token; t1 < m1->token + m1->ntoken; ++t1, ++t2) {
		if (t1->kind != t2->kind)
			return false;
		if (t1->lit && strcmp(t1->lit, t2->lit) != 0)
			return false;
	}
	return true;
}

/* find the index of a macro parameter with the given name */
static size_t
macroparam(struct macro *m, struct token *t)
{
	size_t i;

	if (t->kind == TIDENT) {
		for (i = 0; i < m->nparam; ++i) {
			if (strcmp(m->param[i].name, t->lit) == 0)
				return i;
		}
	}
	return -1;
}

/* lookup a macro by name */
static struct macro *
macroget(char *name)
{
	struct mapkey k;

	mapkey(&k, name, strlen(name));
	return mapget(&macros, &k);
}

static void
macrodone(struct macro *m, struct macroarg *arg)
{
	m->hide = false;
	if (m->kind == MACROFUNC && m->nparam > 0 && arg) {
		free(arg[0].token);
		free(arg);
	}
	--macrodepth;
}

static bool
macrovarargs(struct macro *m)
{
	return m->kind == MACROFUNC && m->nparam > 0 && m->param[m->nparam - 1].flags & PARAMVAR;
}

struct ifstate {
	bool parent;
	bool active;
	bool seen_true;
	bool seen_else;
};

static struct array ifstack;

static bool
ppactive(void)
{
	if (ifstack.len == 0)
		return true;
	return ((struct ifstate *)arraylast(&ifstack, sizeof(struct ifstate)))->active;
}

static struct ifstate *
iftop(void)
{
	if (ifstack.len == 0)
		return NULL;
	return arraylast(&ifstack, sizeof(struct ifstate));
}

static void
ifpush(bool cond)
{
	struct ifstate st = {0};

	st.parent = ppactive();
	st.active = st.parent && cond;
	st.seen_true = st.active;
	st.seen_else = false;
	arrayaddbuf(&ifstack, &st, sizeof(st));
}

static void
ifelif(bool cond)
{
	struct ifstate *st = iftop();

	if (!st)
		error(&tok.loc, "stray #elif");
	if (st->seen_else)
		error(&tok.loc, "#elif after #else");
	if (!st->parent) {
		st->active = false;
		return;
	}
	if (st->seen_true) {
		st->active = false;
		return;
	}
	st->active = cond;
	if (cond)
		st->seen_true = true;
}

static void
ifelse(void)
{
	struct ifstate *st = iftop();

	if (!st)
		error(&tok.loc, "stray #else");
	if (st->seen_else)
		error(&tok.loc, "duplicate #else");
	st->seen_else = true;
	st->active = st->parent && !st->seen_true;
	st->seen_true = true;
}

static void
ifpop(void)
{
	if (ifstack.len == 0)
		error(&tok.loc, "stray #endif");
	ifstack.len -= sizeof(struct ifstate);
}

static struct token *
framenext(struct frame *f)
{
	return f->ntoken--, f->token++;
}

/* push a new context frame */
static struct frame *
ctxpush(struct token *t, size_t n, struct macro *m, struct macroarg *arg, bool space, void *alloc)
{
	struct frame *f;

	f = arrayadd(&ctx, sizeof(*f));
	f->token = t;
	f->ntoken = n;
	f->macro = m;
	f->arg = arg;
	f->alloc = alloc;
	if (n > 0)
		t[0].space = space;
	return f;
}

/* get the next token from the context */
static struct token *
ctxnext(void)
{
	struct frame *f;

	for (f = arraylast(&ctx, sizeof(*f)); ctx.len; --f, ctx.len -= sizeof(*f)) {
		if (f->ntoken)
			break;
		if (f->alloc)
			free(f->alloc);
		if (f->macro)
			macrodone(f->macro, f->arg);
	}
	if (ctx.len == 0)
		return NULL;
	return framenext(f);
}

static void
define(void)
{
	struct token *t;
	enum tokenkind prev;
	struct macro *m;
	struct macroparam *p;
	struct array params = {0}, repl = {0};
	struct mapkey k;
	void **entry;
	size_t i;

	m = xmalloc(sizeof(*m));
	m->name = tokencheck(&tok, TIDENT, "after #define");
	m->hide = false;
	t = arrayadd(&repl, sizeof(*t));
	scan(t);
	if (t->kind == TLPAREN && !t->space) {
		m->kind = MACROFUNC;
		/* read macro parameter names */
		p = NULL;
		while (scan(&tok), tok.kind != TRPAREN) {
			if (p) {
				if (p->flags & PARAMVAR)
					tokencheck(&tok, TRPAREN, "after '...'");
				tokencheck(&tok, TCOMMA, "or ')' after macro parameter");
				scan(&tok);
			}
			p = arrayadd(&params, sizeof(*p));
			p->flags = 0;
			if (tok.kind == TELLIPSIS) {
				p->name = "__VA_ARGS__";
				p->flags |= PARAMVAR;
			} else {
				p->name = tokencheck(&tok, TIDENT, "of macro parameter name or '...'");
			}
		}
		scan(t);  /* first token in replacement list */
	} else {
		m->kind = MACROOBJ;
	}
	m->param = params.val;
	m->nparam = params.len / sizeof(m->param[0]);

	/* read macro body */
	i = macroparam(m, t);
	while (t->kind != TNEWLINE && t->kind != TEOF) {
		prev = t->kind;
		t = arrayadd(&repl, sizeof(*t));
		scan(t);
		if (t->kind == TIDENT && strcmp(t->lit, "__VA_ARGS__") == 0 && !macrovarargs(m))
			error(&t->loc, "__VA_ARGS__ can only be used in variadic function-like macros");
		if (m->kind != MACROFUNC)
			continue;
		if (i != -1)
			m->param[i].flags |= PARAMTOK;
		i = macroparam(m, t);
		if (prev == THASH) {
			tokencheck(t, TIDENT, "after '#' operator");
			if (i == -1)
				error(&t->loc, "'%s' is not a macro parameter name", t->lit);
			m->param[i].flags |= PARAMSTR;
			i = -1;
		}
	}
	m->token = repl.val;
	m->ntoken = repl.len / sizeof(*t) - 1;
	tok = *t;

	mapkey(&k, m->name, strlen(m->name));
	entry = mapput(&macros, &k);
	if (*entry && !macroequal(m, *entry))
		error(&tok.loc, "redefinition of macro '%s'", m->name);
	*entry = m;
}

static void
undef(void)
{
	char *name;
	struct mapkey k;
	void **entry;
	struct macro *m;

	name = tokencheck(&tok, TIDENT, "after #undef");
	mapkey(&k, name, strlen(name));
	entry = mapput(&macros, &k);
	m = *entry;
	if (m) {
		free(name);
		free(m->param);
		free(m->token);
		*entry = NULL;
	}
	scan(&tok);
}

static void
directive(void)
{
	struct location newloc;
	enum ppflags oldflags;
	char *name = NULL;
	bool active = ppactive();

	scan(&tok);
	if (tok.kind == TNEWLINE)
		return;  /* empty directive */
	oldflags = ppflags;
	ppflags |= PPNEWLINE;
	if (tok.kind == TNUMBER)
		goto line;  /* gcc line markers */
	name = tokencheck(&tok, TIDENT, "newline, or number after '#'");
	if (strcmp(name, "if") == 0) {
		struct array line = {0}, exp = {0};
		long long cond = 0;
		scan(&tok);
		read_line_tokens(&line);
		if (active) {
			mark_defined_operands(line.val, line.len / sizeof(struct token));
			expand_tokens(line.val, line.len / sizeof(struct token), &exp);
			cond = ppeval(exp.val, exp.len / sizeof(struct token));
		}
		ifpush(cond != 0);
		free(line.val);
		free(exp.val);
	} else if (strcmp(name, "ifdef") == 0) {
		scan(&tok);
		if (tok.kind != TIDENT)
			error(&tok.loc, "expected identifier after #ifdef");
		ifpush(active && macroget(tok.lit) != NULL);
		scan(&tok);
		while (tok.kind != TNEWLINE && tok.kind != TEOF)
			scan(&tok);
	} else if (strcmp(name, "ifndef") == 0) {
		scan(&tok);
		if (tok.kind != TIDENT)
			error(&tok.loc, "expected identifier after #ifndef");
		ifpush(active && macroget(tok.lit) == NULL);
		scan(&tok);
		while (tok.kind != TNEWLINE && tok.kind != TEOF)
			scan(&tok);
	} else if (strcmp(name, "elif") == 0) {
		struct array line = {0}, exp = {0};
		long long cond = 0;
		struct ifstate *st = iftop();
		scan(&tok);
		read_line_tokens(&line);
		if (st && st->parent && !st->seen_true) {
			mark_defined_operands(line.val, line.len / sizeof(struct token));
			expand_tokens(line.val, line.len / sizeof(struct token), &exp);
			cond = ppeval(exp.val, exp.len / sizeof(struct token));
		}
		ifelif(cond != 0);
		free(line.val);
		free(exp.val);
	} else if (strcmp(name, "else") == 0) {
		ifelse();
		scan(&tok);
		while (tok.kind != TNEWLINE && tok.kind != TEOF)
			scan(&tok);
	} else if (strcmp(name, "endif") == 0) {
		ifpop();
		scan(&tok);
		while (tok.kind != TNEWLINE && tok.kind != TEOF)
			scan(&tok);
	} else if (strcmp(name, "include") == 0 || strcmp(name, "include_next") == 0) {
		struct array line = {0}, exp = {0};
		bool angle = false;
		char *file;
		bool nextinc = strcmp(name, "include_next") == 0;

		scan(&tok);
		read_line_tokens(&line);
		if (!active) {
			free(line.val);
			goto done;
		}
		if (line.len >= sizeof(struct token)) {
			struct token *t0 = line.val;
			if (t0->kind == TSTRINGLIT || t0->kind == TLESS) {
				file = header_from_tokens(line.val, line.len / sizeof(struct token), &angle);
				free(line.val);
				if (!file)
					error(&tok.loc, "invalid #include directive");
				include_file(file, angle, tok.loc.file, nextinc);
				free(file);
				goto done;
			}
		}
		expand_tokens(line.val, line.len / sizeof(struct token), &exp);
		file = header_from_tokens(exp.val, exp.len / sizeof(struct token), &angle);
		if (!file)
			error(&tok.loc, "invalid #include directive");
		include_file(file, angle, tok.loc.file, nextinc);
		free(file);
		free(line.val);
		free(exp.val);
	} else if (strcmp(name, "define") == 0) {
		if (!active) {
			scan(&tok);
			while (tok.kind != TNEWLINE && tok.kind != TEOF)
				scan(&tok);
			goto done;
		}
		scan(&tok);
		define();
	} else if (strcmp(name, "undef") == 0) {
		if (!active) {
			scan(&tok);
			while (tok.kind != TNEWLINE && tok.kind != TEOF)
				scan(&tok);
			goto done;
		}
		scan(&tok);
		undef();
	} else if (strcmp(name, "line") == 0) {
		if (!active) {
			scan(&tok);
			while (tok.kind != TNEWLINE && tok.kind != TEOF)
				scan(&tok);
			goto done;
		}
		scan(&tok);
		tokencheck(&tok, TNUMBER, "after #line");
line:
		newloc.line = strtoull(tok.lit, NULL, 0);
		newloc.col = 1;
		scan(&tok);
		newloc.file = tok.loc.file;
		if (tok.kind == TSTRINGLIT) {
			/* XXX: handle escape sequences (reuse string decoding from expr.c) */
			newloc.file = strchr(tok.lit, '"') + 1;
			*strchr(newloc.file, '"') = '\0';
			scan(&tok);
		}
		while (tok.kind == TNUMBER)
			scan(&tok);
		scansetloc(newloc);
	} else if (strcmp(name, "error") == 0) {
		if (active) {
			struct array msg = {0};
			scan(&tok);
			while (tok.kind != TNEWLINE && tok.kind != TEOF) {
				const char *s = toktext(&tok);
				if (s) {
					arrayaddbuf(&msg, s, strlen(s));
					arrayaddbuf(&msg, " ", 1);
				}
				scan(&tok);
			}
			arrayaddbuf(&msg, "", 1);
			error(&tok.loc, "#error %s", (char *)msg.val);
		} else {
			scan(&tok);
			while (tok.kind != TNEWLINE && tok.kind != TEOF)
				scan(&tok);
		}
	} else if (strcmp(name, "pragma") == 0) {
		if (active) {
			scan(&tok);
			if (tok.kind == TIDENT && strcmp(tok.lit, "once") == 0)
				mark_once(tok.loc.file);
		}
		while (tok.kind != TNEWLINE && tok.kind != TEOF)
			scan(&tok);
	} else {
		if (active)
			error(&tok.loc, "invalid preprocessor directive #%s", name);
		while (tok.kind != TNEWLINE && tok.kind != TEOF)
			scan(&tok);
	}
done:
	free(name);
	tokencheck(&tok, TNEWLINE, "after preprocessing directive");
	ppflags = oldflags;
}

/* get the next token without expanding it */
static void
nextinto(struct token *t)
{
	static bool newline = true;

	for (;;) {
		scan(t);
		if (newline && t->kind == THASH) {
			directive();
			newline = true;
			continue;
		}
		newline = t->kind == TNEWLINE;
		if (!ppactive())
			continue;
		break;
	}
}

static struct token *
rawnext(void)
{
	struct token *t;

	t = ctxnext();
	if (!t) {
		t = &tok;
		nextinto(t);
	}
	return t;
}

static bool
peekparen(void)
{
	static struct array pending;
	struct token *t;
	struct frame *f, *base;
	size_t n;

	if (ctx.len) {
		base = ctx.val;
		n = ctx.len / sizeof(*f);
		while (n--) {
			f = base + n;
			if (f->ntoken == 0)
				continue;
			if (f->token[0].kind != TLPAREN)
				return false;
			++f->token;
			--f->ntoken;
			return true;
		}
	}
	pending.len = 0;
	do t = arrayadd(&pending, sizeof(*t)), nextinto(t);
	while (t->kind == TNEWLINE);
	if (t->kind == TLPAREN)
		return true;
	t = pending.val;
	ctxpush(t, pending.len / sizeof(*t), NULL, NULL, t[0].space, NULL);
	return false;
}

static void
stringize(struct array *buf, struct token *t)
{
	const char *lit;

	if ((t->space || t->kind == TNEWLINE) && buf->len > 1 && ((char *)buf->val)[buf->len - 1] != ' ')
		arrayaddbuf(buf, " ", 1);
	lit = t->lit ? t->lit : tokstr[t->kind];
	if (t->kind == TSTRINGLIT || t->kind == TCHARCONST) {
		for (; *lit; ++lit) {
			if (*lit == '\\' || *lit == '"')
				arrayaddbuf(buf, "\\", 1);
			arrayaddbuf(buf, lit, 1);
		}
	} else if (lit) {
		arrayaddbuf(buf, lit, strlen(lit));
	}
}

static const char *
toktext(const struct token *t)
{
	if (t->lit)
		return t->lit;
	if ((size_t)t->kind < tokstr_len)
		return tokstr[t->kind];
	return NULL;
}

static int
is_ident_start(int c)
{
	return isalpha(c) || c == '_';
}

static enum tokenkind
guesskind(const char *s)
{
	if (!s || !*s)
		return TOTHER;
	if (s[0] == '"')
		return TSTRINGLIT;
	if (s[0] == '\'')
		return TCHARCONST;
	if (isdigit((unsigned char)s[0]) || (s[0] == '.' && isdigit((unsigned char)s[1])))
		return TNUMBER;
	if (is_ident_start((unsigned char)s[0]))
		return TIDENT;
	return TOTHER;
}

static struct token
paste_tokens(const struct token *a, const struct token *b)
{
	struct token t = {0};
	const char *sa = toktext(a);
	const char *sb = toktext(b);
	size_t la = sa ? strlen(sa) : 0;
	size_t lb = sb ? strlen(sb) : 0;
	char *s = xmalloc(la + lb + 1);

	if (sa)
		memcpy(s, sa, la);
	if (sb)
		memcpy(s + la, sb, lb);
	s[la + lb] = '\0';
	t.kind = guesskind(s);
	t.lit = s;
	t.hide = a->hide || b->hide;
	t.hideset = hideset_union(a->hideset, b->hideset);
	t.space = a->space;
	t.loc = a->loc;
	return t;
}

static void
append_tok(struct array *out, const struct token *t, bool dup)
{
	struct token c = *t;
	if (dup && t->lit) {
		c.lit = strdup(t->lit);
		if (!c.lit)
			fatal("strdup:");
	}
	arrayaddbuf(out, &c, sizeof(c));
}

static void
expand_tokens(struct token *in, size_t nin, struct array *out)
{
	struct token eof = { .kind = TEOF };
	size_t saved_ctx = ctx.len;
	size_t saved_depth = macrodepth;
	struct token *t;

	ctxpush(&eof, 1, NULL, NULL, false, NULL);
	ctxpush(in, nin, NULL, NULL, false, NULL);
	for (;;) {
		t = rawnext();
		if (t->kind == TEOF)
			break;
		if (expand(t))
			continue;
		append_tok(out, t, true);
	}
	ctx.len = saved_ctx;
	macrodepth = saved_depth;
}

static void
macroreplace(struct macro *m, struct macroarg *arg, struct array *out)
{
	size_t i;

	for (i = 0; i < m->ntoken; ++i) {
		struct token *t = &m->token[i];
		size_t idx = macroparam(m, t);

		if (t->kind == THASH && i + 1 < m->ntoken) {
			struct token *n = &m->token[i + 1];
			size_t si = macroparam(m, n);
			if (si == (size_t)-1)
				error(&n->loc, "'%s' is not a macro parameter name", n->lit);
			arrayaddbuf(out, &arg[si].str, sizeof(arg[si].str));
			++i;
			continue;
		}
		if (idx != (size_t)-1) {
			bool paste = false;
			if (i > 0 && m->token[i - 1].kind == THASHHASH)
				paste = true;
			if (i + 1 < m->ntoken && m->token[i + 1].kind == THASHHASH)
				paste = true;
			if (paste) {
				if (arg[idx].ntoken)
					arrayaddbuf(out, arg[idx].token,
					    arg[idx].ntoken * sizeof(*arg[idx].token));
			} else {
				struct array exp = {0};
				struct token *et;
				size_t en;
				expand_tokens(arg[idx].token, arg[idx].ntoken, &exp);
				et = exp.val;
				en = exp.len / sizeof(*et);
				if (en)
					et[0].space = t->space;
				for (size_t j = 0; j < en; ++j)
					append_tok(out, &et[j], true);
				free(exp.val);
			}
			continue;
		}
		arrayaddbuf(out, t, sizeof(*t));
	}
	/* token pasting */
	{
		struct array tmp = *out;
		struct array res = {0};
		struct token *t = tmp.val;
		size_t n = tmp.len / sizeof(*t);
		for (i = 0; i < n; ++i) {
			if (t[i].kind != THASHHASH) {
				arrayaddbuf(&res, &t[i], sizeof(*t));
				continue;
			}
			if (res.len == 0) {
				/* leading ##: ignore */
				continue;
			}
			/* find next token */
			if (i + 1 >= n) {
				res.len = res.len >= sizeof(*t) ? res.len - sizeof(*t) : 0;
				break;
			}
			if (t[i + 1].kind == THASHHASH)
				continue;
			/* remove previous token */
			struct token *prev = (struct token *)((char *)res.val + res.len - sizeof(*t));
			struct token merged = paste_tokens(prev, &t[i + 1]);
			res.len -= sizeof(*t);
			arrayaddbuf(&res, &merged, sizeof(merged));
			++i;
		}
		*out = res;
		free(tmp.val);
	}
}

struct tokalloc {
	struct token tok;
	char lit[];
};

static struct tokalloc *
alloctok(enum tokenkind kind, const char *lit, struct location loc)
{
	struct tokalloc *a;
	size_t len = strlen(lit);

	a = xmalloc(sizeof(*a) + len + 1);
	memset(&a->tok, 0, sizeof(a->tok));
	a->tok.kind = kind;
	a->tok.loc = loc;
	a->tok.lit = a->lit;
	memcpy(a->lit, lit, len + 1);
	return a;
}

struct ppexpr {
	struct token *t;
	size_t n;
	size_t i;
};

static struct token *
ppeek(struct ppexpr *p)
{
	if (p->i >= p->n)
		return NULL;
	return &p->t[p->i];
}

static struct token *
ppnext(struct ppexpr *p)
{
	struct token *t = ppeek(p);
	if (t)
		++p->i;
	return t;
}

static long long ppexpr_or(struct ppexpr *p);
static long long ppexpr_cond(struct ppexpr *p);

static long long
ppnum(const char *s)
{
	char *end = NULL;
	long long v = strtoll(s, &end, 0);
	(void)end;
	return v;
}

static long long
ppchar(const char *lit)
{
	const char *p = lit;
	unsigned v = 0;

	if (*p == 'L' || *p == 'u' || *p == 'U')
		++p;
	if (*p != '\'')
		return 0;
	++p;
	if (*p == '\\') {
		++p;
		switch (*p) {
		case 'a': v = '\a'; break;
		case 'b': v = '\b'; break;
		case 'f': v = '\f'; break;
		case 'n': v = '\n'; break;
		case 'r': v = '\r'; break;
		case 't': v = '\t'; break;
		case 'v': v = '\v'; break;
		case '\\': v = '\\'; break;
		case '\'': v = '\''; break;
		case '\"': v = '\"'; break;
		case 'x':
			++p;
			while (isxdigit((unsigned char)*p)) {
				v = v * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
				    (tolower((unsigned char)*p) - 'a' + 10));
				++p;
			}
			return v;
		default:
			if (*p >= '0' && *p <= '7') {
				int i;
				for (i = 0; i < 3 && *p >= '0' && *p <= '7'; ++i, ++p)
					v = v * 8 + (*p - '0');
				return v;
			}
			return *p;
		}
		return v;
	}
	return (unsigned char)*p;
}

static long long
ppexpr_primary(struct ppexpr *p)
{
	struct token *t = ppeek(p);

	if (!t)
		return 0;
	if (t->kind == TLPAREN) {
		ppnext(p);
		long long v = ppexpr_or(p);
		t = ppeek(p);
		if (t && t->kind == TRPAREN)
			ppnext(p);
		return v;
	}
	if (t->kind == TIDENT && strcmp(t->lit, "defined") == 0) {
		struct token *id;
		ppnext(p);
		id = ppeek(p);
		if (id && id->kind == TLPAREN) {
			ppnext(p);
			id = ppeek(p);
			if (!id || id->kind != TIDENT)
				return 0;
			ppnext(p);
			if (ppeek(p) && ppeek(p)->kind == TRPAREN)
				ppnext(p);
		} else {
			if (!id || id->kind != TIDENT)
				return 0;
			ppnext(p);
		}
		return macroget(id->lit) ? 1 : 0;
	}
	ppnext(p);
	if (t->kind == TNUMBER)
		return ppnum(t->lit);
	if (t->kind == TCHARCONST)
		return ppchar(t->lit);
	if (t->kind == TIDENT)
		return 0;
	return 0;
}

static long long
ppexpr_unary(struct ppexpr *p)
{
	struct token *t = ppeek(p);
	if (!t)
		return 0;
	switch (t->kind) {
	case TADD: ppnext(p); return +ppexpr_unary(p);
	case TSUB: ppnext(p); return -ppexpr_unary(p);
	case TLNOT: ppnext(p); return !ppexpr_unary(p);
	case TBNOT: ppnext(p); return ~ppexpr_unary(p);
	default: return ppexpr_primary(p);
	}
}

static long long
ppexpr_mul(struct ppexpr *p)
{
	long long v = ppexpr_unary(p);
	for (;;) {
		struct token *t = ppeek(p);
		if (!t)
			break;
		if (t->kind == TMUL) {
			ppnext(p);
			v *= ppexpr_unary(p);
		} else if (t->kind == TDIV) {
			ppnext(p);
			long long r = ppexpr_unary(p);
			if (r)
				v /= r;
			else
				v = 0;
		} else if (t->kind == TMOD) {
			ppnext(p);
			long long r = ppexpr_unary(p);
			if (r)
				v %= r;
			else
				v = 0;
		} else {
			break;
		}
	}
	return v;
}

static long long
ppexpr_add(struct ppexpr *p)
{
	long long v = ppexpr_mul(p);
	for (;;) {
		struct token *t = ppeek(p);
		if (!t)
			break;
		if (t->kind == TADD) {
			ppnext(p);
			v += ppexpr_mul(p);
		} else if (t->kind == TSUB) {
			ppnext(p);
			v -= ppexpr_mul(p);
		} else {
			break;
		}
	}
	return v;
}

static long long
ppexpr_shift(struct ppexpr *p)
{
	long long v = ppexpr_add(p);
	for (;;) {
		struct token *t = ppeek(p);
		if (!t)
			break;
		if (t->kind == TSHL) {
			ppnext(p);
			v <<= ppexpr_add(p);
		} else if (t->kind == TSHR) {
			ppnext(p);
			v >>= ppexpr_add(p);
		} else {
			break;
		}
	}
	return v;
}

static long long
ppexpr_rel(struct ppexpr *p)
{
	long long v = ppexpr_shift(p);
	for (;;) {
		struct token *t = ppeek(p);
		if (!t)
			break;
		if (t->kind == TLESS) {
			ppnext(p);
			v = v < ppexpr_shift(p);
		} else if (t->kind == TLEQ) {
			ppnext(p);
			v = v <= ppexpr_shift(p);
		} else if (t->kind == TGREATER) {
			ppnext(p);
			v = v > ppexpr_shift(p);
		} else if (t->kind == TGEQ) {
			ppnext(p);
			v = v >= ppexpr_shift(p);
		} else {
			break;
		}
	}
	return v;
}

static long long
ppexpr_eq(struct ppexpr *p)
{
	long long v = ppexpr_rel(p);
	for (;;) {
		struct token *t = ppeek(p);
		if (!t)
			break;
		if (t->kind == TEQL) {
			ppnext(p);
			v = v == ppexpr_rel(p);
		} else if (t->kind == TNEQ) {
			ppnext(p);
			v = v != ppexpr_rel(p);
		} else {
			break;
		}
	}
	return v;
}

static long long
ppexpr_bitand(struct ppexpr *p)
{
	long long v = ppexpr_eq(p);
	while (ppeek(p) && ppeek(p)->kind == TBAND) {
		ppnext(p);
		v &= ppexpr_eq(p);
	}
	return v;
}

static long long
ppexpr_bitxor(struct ppexpr *p)
{
	long long v = ppexpr_bitand(p);
	while (ppeek(p) && ppeek(p)->kind == TXOR) {
		ppnext(p);
		v ^= ppexpr_bitand(p);
	}
	return v;
}

static long long
ppexpr_bitor(struct ppexpr *p)
{
	long long v = ppexpr_bitxor(p);
	while (ppeek(p) && ppeek(p)->kind == TBOR) {
		ppnext(p);
		v |= ppexpr_bitxor(p);
	}
	return v;
}

static long long
ppexpr_and(struct ppexpr *p)
{
	long long v = ppexpr_bitor(p);
	while (ppeek(p) && ppeek(p)->kind == TLAND) {
		ppnext(p);
		v = v && ppexpr_bitor(p);
	}
	return v;
}

static long long
ppexpr_or(struct ppexpr *p)
{
	long long v = ppexpr_and(p);
	while (ppeek(p) && ppeek(p)->kind == TLOR) {
		ppnext(p);
		v = v || ppexpr_and(p);
	}
	return v;
}

static long long
ppexpr_cond(struct ppexpr *p)
{
	long long cond = ppexpr_or(p);
	struct token *t = ppeek(p);

	if (t && t->kind == TQUESTION) {
		ppnext(p);
		long long tv = ppexpr_cond(p);
		if (ppeek(p) && ppeek(p)->kind == TCOLON)
			ppnext(p);
		long long fv = ppexpr_cond(p);
		return cond ? tv : fv;
	}
	return cond;
}

static long long
ppeval(struct token *toks, size_t ntok)
{
	struct ppexpr p = { .t = toks, .n = ntok, .i = 0 };
	return ppexpr_cond(&p);
}

static void
read_line_tokens(struct array *out)
{
	while (tok.kind != TNEWLINE && tok.kind != TEOF) {
		arrayaddbuf(out, &tok, sizeof(tok));
		scan(&tok);
	}
}

static void
mark_defined_operands(struct token *t, size_t n)
{
	size_t i;

	for (i = 0; i < n; ++i) {
		if (t[i].kind != TIDENT || strcmp(t[i].lit, "defined") != 0)
			continue;
		if (i + 1 < n && t[i + 1].kind == TLPAREN) {
			if (i + 2 < n && t[i + 2].kind == TIDENT)
				t[i + 2].hide = true;
			continue;
		}
		if (i + 1 < n && t[i + 1].kind == TIDENT)
			t[i + 1].hide = true;
	}
}

static char *
unquote(const char *lit)
{
	size_t len = strlen(lit);
	char *s, *d;

	if (len < 2 || lit[0] != '"' || lit[len - 1] != '"')
		return strdup(lit);
	s = xmalloc(len - 1);
	d = s;
	for (size_t i = 1; i + 1 < len; ++i) {
		if (lit[i] == '\\' && i + 1 < len - 1) {
			++i;
			*d++ = lit[i];
			continue;
		}
		*d++ = lit[i];
	}
	*d = '\0';
	return s;
}

static char *
header_from_tokens(struct token *t, size_t n, bool *angle)
{
	size_t i;
	struct array buf = {0};

	if (n == 0)
		return NULL;
	if (t[0].kind == TSTRINGLIT) {
		*angle = false;
		return unquote(t[0].lit);
	}
	if (t[0].kind != TLESS)
		return NULL;
	*angle = true;
	for (i = 1; i < n; ++i) {
		if (t[i].kind == TGREATER)
			break;
		const char *s = toktext(&t[i]);
		if (!s)
			return NULL;
		arrayaddbuf(&buf, s, strlen(s));
	}
	if (i == n || t[i].kind != TGREATER) {
		free(buf.val);
		return NULL;
	}
	arrayaddbuf(&buf, "", 1);
	return buf.val;
}

static bool
fileexists(const char *path)
{
	FILE *f = fopen(path, "r");
	if (!f)
		return false;
	fclose(f);
	return true;
}

static char *
pathjoin(const char *dir, const char *name)
{
	size_t dl = strlen(dir);
	size_t nl = strlen(name);
	char *p;

	p = xmalloc(dl + nl + 2);
	memcpy(p, dir, dl);
	if (dl && dir[dl - 1] != '/')
		p[dl++] = '/';
	memcpy(p + dl, name, nl);
	p[dl + nl] = '\0';
	return p;
}

static char *
filedir(const char *path)
{
	const char *slash = strrchr(path, '/');
	size_t len = slash ? (size_t)(slash - path) : 0;
	char *dir;

	if (!slash)
		return strdup(".");
	dir = xmalloc(len + 1);
	memcpy(dir, path, len);
	dir[len] = '\0';
	return dir;
}

static bool
is_once_file(const char *path)
{
	struct mapkey k;

	mapkey(&k, path, strlen(path));
	return mapget(&oncefiles, &k) != NULL;
}

static bool
was_included(const char *path)
{
	struct mapkey k;

	mapkey(&k, path, strlen(path));
	return mapget(&included_once, &k) != NULL;
}

static void
mark_included(const char *path)
{
	struct mapkey k;

	mapkey(&k, path, strlen(path));
	*mapput(&included_once, &k) = (void *)1;
}

static void
mark_once(const char *path)
{
	struct mapkey k;

	mapkey(&k, path, strlen(path));
	*mapput(&oncefiles, &k) = (void *)1;
}

static size_t
find_incdir(struct array *list, const char *dir, bool *found)
{
	struct incdir *d;
	size_t i;

	*found = false;
	for (i = 0; i < list->len / sizeof(*d); ++i) {
		d = (struct incdir *)list->val + i;
		if (strcmp(d->path, dir) == 0) {
			*found = true;
			return i;
		}
	}
	return 0;
}

static void
add_default_includes(void)
{
	size_t i;

	if (pp_no_stdinc)
		return;
	for (i = 0; i < LEN(includedirs); ++i) {
		if (includedirs[i])
			add_incdir(&inc_system, includedirs[i], PPINC_SYSTEM);
	}
}

static void
include_file(const char *name, bool angle, const char *from, bool next)
{
	struct incdir *d;
	char *path, *fromdir;
	size_t i, quote_start = 0, system_start = 0, after_start = 0;
	bool found;

	fromdir = filedir(from);
	if (next) {
		quote_start = find_incdir(&inc_quote, fromdir, &found);
		if (found)
			quote_start++;
		system_start = find_incdir(&inc_system, fromdir, &found);
		if (found)
			system_start++;
		after_start = find_incdir(&inc_after, fromdir, &found);
		if (found)
			after_start++;
	}
	if (!angle && !next) {
		path = pathjoin(fromdir, name);
		if (fileexists(path)) {
			if (is_once_file(path) && was_included(path)) {
				free(path);
				free(fromdir);
				return;
			}
			mark_included(path);
			scanfrom(path, NULL);
			scanopen();
			free(fromdir);
			return;
		}
		free(path);
	}
	if (!angle) {
		for (i = quote_start; i < inc_quote.len / sizeof(*d); ++i) {
			d = (struct incdir *)inc_quote.val + i;
			path = pathjoin(d->path, name);
			if (fileexists(path)) {
				if (is_once_file(path) && was_included(path)) {
					free(path);
					free(fromdir);
					return;
				}
				mark_included(path);
				scanfrom(path, NULL);
				scanopen();
				free(fromdir);
				return;
			}
			free(path);
		}
	}
	for (i = system_start; i < inc_system.len / sizeof(*d); ++i) {
		d = (struct incdir *)inc_system.val + i;
		path = pathjoin(d->path, name);
		if (fileexists(path)) {
			if (is_once_file(path) && was_included(path)) {
				free(path);
				free(fromdir);
				return;
			}
			mark_included(path);
			scanfrom(path, NULL);
			scanopen();
			free(fromdir);
			return;
		}
		free(path);
	}
	for (i = after_start; i < inc_after.len / sizeof(*d); ++i) {
		d = (struct incdir *)inc_after.val + i;
		path = pathjoin(d->path, name);
		if (fileexists(path)) {
			if (is_once_file(path) && was_included(path)) {
				free(path);
				free(fromdir);
				return;
			}
			mark_included(path);
			scanfrom(path, NULL);
			scanopen();
			free(fromdir);
			return;
		}
		free(path);
	}
	free(fromdir);
	error(&tok.loc, "include file '%s' not found", name);
}

static struct macroarg *expandfunc(struct macro *);

static bool
expand(struct token *t)
{
	struct macro *m;
	bool space;
	struct hideset *hs;
	struct hideset *thide;

	if (t->kind != TIDENT)
		return false;
	if (t->hide)
		return false;
	m = macroget(t->lit);
	if (!m) {
		if (strcmp(t->lit, "__LINE__") == 0) {
			char buf[32];
			struct tokalloc *bt;
			int n = snprintf(buf, sizeof(buf), "%llu", (unsigned long long)t->loc.line);
			if (n < 0)
				fatal("snprintf:");
			bt = alloctok(TNUMBER, buf, t->loc);
			ctxpush(&bt->tok, 1, NULL, NULL, t->space, bt);
			return true;
		}
		if (strcmp(t->lit, "__FILE__") == 0) {
			char *buf;
			size_t len = strlen(t->loc.file);
			struct tokalloc *bt;
			buf = xmalloc(len + 3);
			buf[0] = '"';
			memcpy(buf + 1, t->loc.file, len);
			buf[len + 1] = '"';
			buf[len + 2] = '\0';
			bt = alloctok(TSTRINGLIT, buf, t->loc);
			free(buf);
			ctxpush(&bt->tok, 1, NULL, NULL, t->space, bt);
			return true;
		}
		if (strcmp(t->lit, "__DATE__") == 0 && pp_date[0]) {
			char buf[64];
			struct tokalloc *bt;
			snprintf(buf, sizeof(buf), "\"%s\"", pp_date);
			bt = alloctok(TSTRINGLIT, buf, t->loc);
			ctxpush(&bt->tok, 1, NULL, NULL, t->space, bt);
			return true;
		}
		if (strcmp(t->lit, "__TIME__") == 0 && pp_time[0]) {
			char buf[64];
			struct tokalloc *bt;
			snprintf(buf, sizeof(buf), "\"%s\"", pp_time);
			bt = alloctok(TSTRINGLIT, buf, t->loc);
			ctxpush(&bt->tok, 1, NULL, NULL, t->space, bt);
			return true;
		}
		if (strcmp(t->lit, "__COUNTER__") == 0) {
			char buf[32];
			struct tokalloc *bt;
			snprintf(buf, sizeof(buf), "%lu", pp_counter++);
			bt = alloctok(TNUMBER, buf, t->loc);
			ctxpush(&bt->tok, 1, NULL, NULL, t->space, bt);
			return true;
		}
		return false;
	}
	if (m->hide)
		return false;
	if (hideset_contains(t->hideset, m))
		return false;
	space = t->space;
	thide = t->hideset;
	struct macroarg *arg = NULL;
	if (m->kind == MACROFUNC) {
		if (!peekparen())
			return false;
		arg = expandfunc(m);
	}
	{
		struct array rep = {0};
		macroreplace(m, arg, &rep);
		hs = hideset_add(thide, m);
		{
			struct token *rt = rep.val;
			size_t rn = rep.len / sizeof(*rt);
			for (size_t i = 0; i < rn; ++i)
				rt[i].hideset = hideset_union(rt[i].hideset, hs);
		}
		ctxpush(rep.val, rep.len / sizeof(struct token), m, arg, space, rep.val);
	}
	m->hide = true;
	++macrodepth;
	if (macrodepth > 10000)
		error(&tok.loc, "macro expansion too deep");
	return true;
}

static struct macroarg *
expandfunc(struct macro *m)
{
	struct macroparam *p;
	struct macroarg *arg;
	struct array str, tok;
	size_t i, paren;
	struct token *t;

	/* read macro arguments */
	paren = 0;
	tok = (struct array){0};
	arg = NULL;
	if (m->nparam > 0)
		arg = xreallocarray(NULL, m->nparam, sizeof(*arg));
	t = rawnext();
	for (i = 0; i < m->nparam; ++i) {
		p = &m->param[i];
		if (p->flags & PARAMSTR) {
			str = (struct array){0};
			arrayaddbuf(&str, "\"", 1);
		}
		arg[i].ntoken = 0;
		for (;;) {
			if (t->kind == TEOF)
				error(&t->loc, "EOF when reading macro parameters");
			if (paren == 0 && (t->kind == TRPAREN || t->kind == TCOMMA && !(p->flags & PARAMVAR)))
				break;
			switch (t->kind) {
			case TLPAREN: ++paren; break;
			case TRPAREN: --paren; break;
			}
			if (p->flags & PARAMSTR)
				stringize(&str, t);
			arrayaddbuf(&tok, t, sizeof(*t));
			++arg[i].ntoken;
			t = rawnext();
		}
		if (p->flags & PARAMSTR) {
			arrayaddbuf(&str, "\"", 2);
			arg[i].str = (struct token){
				.kind = TSTRINGLIT,
				.lit = str.val,
			};
		}
		if (t->kind == TRPAREN)
			break;
		t = rawnext();
	}
	if (i + 1 < m->nparam)
		error(&t->loc, "not enough arguments for macro '%s'", m->name);
	if (t->kind != TRPAREN)
		error(&t->loc, "too many arguments for macro '%s'", m->name);
	for (i = 0, t = tok.val; i < m->nparam; ++i) {
		arg[i].token = t;
		t += arg[i].ntoken;
	}
	return arg;
}

static void
keyword(struct token *tok)
{
	static const struct {
		const char *name;
		int value;
	} keywords[] = {
		{"_Alignas",       TALIGNAS},
		{"_Alignof",       TALIGNOF},
		{"_Atomic",        T_ATOMIC},
		{"_Bool",          TBOOL},
		{"_Complex",       T_COMPLEX},
		{"_Decimal128",    T_DECIMAL128},
		{"_Decimal32",     T_DECIMAL32},
		{"_Decimal64",     T_DECIMAL64},
		{"_Generic",       T_GENERIC},
		{"_Imaginary",     T_IMAGINARY},
		{"_Noreturn",      T_NORETURN},
		{"_Static_assert", TSTATIC_ASSERT},
		{"_Thread_local",  TTHREAD_LOCAL},
		{"__alignof__",    TALIGNOF},
		{"__asm",          T__ASM__},
		{"__asm__",        T__ASM__},
		{"__attribute__",  T__ATTRIBUTE__},
		{"__inline",       TINLINE},
		{"__inline__",     TINLINE},
		{"__signed",       TSIGNED},
		{"__signed__",     TSIGNED},
		{"__thread",       TTHREAD_LOCAL},
		{"__typeof",       TTYPEOF},
		{"__typeof__",     TTYPEOF},
		{"__volatile",     TVOLATILE},
		{"__volatile__",   TVOLATILE},
		{"alignas",        TALIGNAS},
		{"alignof",        TALIGNOF},
		{"auto",           TAUTO},
		{"bool",           TBOOL},
		{"break",          TBREAK},
		{"case",           TCASE},
		{"char",           TCHAR},
		{"const",          TCONST},
		{"constexpr",      TCONSTEXPR},
		{"continue",       TCONTINUE},
		{"default",        TDEFAULT},
		{"do",             TDO},
		{"double",         TDOUBLE},
		{"else",           TELSE},
		{"enum",           TENUM},
		{"extern",         TEXTERN},
		{"false",          TFALSE},
		{"float",          TFLOAT},
		{"for",            TFOR},
		{"goto",           TGOTO},
		{"if",             TIF},
		{"inline",         TINLINE},
		{"int",            TINT},
		{"long",           TLONG},
		{"nullptr",        TNULLPTR},
		{"register",       TREGISTER},
		{"restrict",       TRESTRICT},
		{"return",         TRETURN},
		{"short",          TSHORT},
		{"signed",         TSIGNED},
		{"sizeof",         TSIZEOF},
		{"static",         TSTATIC},
		{"static_assert",  TSTATIC_ASSERT},
		{"struct",         TSTRUCT},
		{"switch",         TSWITCH},
		{"thread_local",   TTHREAD_LOCAL},
		{"true",           TTRUE},
		{"typedef",        TTYPEDEF},
		{"typeof",         TTYPEOF},
		{"typeof_unqual",  TTYPEOF_UNQUAL},
		{"union",          TUNION},
		{"unsigned",       TUNSIGNED},
		{"void",           TVOID},
		{"volatile",       TVOLATILE},
		{"while",          TWHILE},
	};
	size_t low = 0, high = LEN(keywords), mid;
	int cmp;

	while (low < high) {
		mid = (low + high) / 2;
		cmp = strcmp(tok->lit, keywords[mid].name);
		if (cmp == 0) {
			tok->kind = keywords[mid].value;
			tok->lit = NULL;
			break;
		}
		if (cmp < 0)
			high = mid;
		else
			low = mid + 1;
	}
}

void
next(void)
{
	struct token *t;

	do t = rawnext();
	while (expand(t) || t->kind == TNEWLINE && !(ppflags & PPNEWLINE));
	tok = *t;
	if (tok.kind == TIDENT)
		keyword(&tok);
}

bool
peek(int kind)
{
	static struct token pending;
	struct token old;

	old = tok;
	next();
	if (tok.kind == kind) {
		next();
		return true;
	}
	pending = tok;
	tok = old;
	ctxpush(&pending, 1, NULL, NULL, pending.space, NULL);
	return false;
}

char *
expect(enum tokenkind kind, const char *msg)
{
	char *lit;

	lit = tokencheck(&tok, kind, msg);
	next();

	return lit;
}

bool
consume(int kind)
{
	if (tok.kind != kind)
		return false;
	next();
	return true;
}
