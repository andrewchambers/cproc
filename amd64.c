#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "cc.h"

struct value {
	enum {
		VALUE_NONE,
		VALUE_GLOBAL,
		VALUE_INTCONST,
		VALUE_FLTCONST,
		VALUE_TEMP,
		VALUE_STACK,
	} kind;
	struct type *type;
	unsigned id;
	long long off;
	union {
		char *name;
		unsigned long long i;
		long double f;
	} u;
};

struct lvalue {
	struct value *addr;
	struct bitfield bits;
};

struct block {
	char *name;
	unsigned id;
	bool term;
	struct array code;
	struct block *next;
};

struct switchcase {
	struct treenode node;
	struct block *body;
};

struct func {
	struct decl *decl, *namedecl;
	char *name;
	struct type *type;
	struct block *start, *end;
	struct map gotos;
	struct value *retptr;
	long long stack;
	unsigned id;
	int vararg_gp;
	int vararg_fp;
	long long vararg_save;
	long long vararg_overflow;
};

static struct array out;

static const char *const argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static const char *const xmmreg[] = {"%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7"};

void funclabel(struct func *, struct block *);
struct value *funcexpr(struct func *, struct expr *);
static void loadfromslot(struct func *, struct value *, struct type *, const char *);
static struct value *storeregtemp(struct func *, struct type *, const char *);

static bool
isaggregate(struct type *t)
{
	return t->kind == TYPESTRUCT || t->kind == TYPEUNION || t->kind == TYPEARRAY;
}

static bool
isfloat(struct type *t)
{
	return t->prop & PROPFLOAT;
}

static bool
isx87(struct type *t)
{
	return t->kind == TYPELDOUBLE;
}

static bool
isssefloat(struct type *t)
{
	return isfloat(t) && !isx87(t);
}

static bool
isintlike(struct type *t)
{
	return t->prop & PROPINT || t->kind == TYPEPOINTER || t->kind == TYPENULLPTR;
}

static struct type *
inttype(struct type *t)
{
	if (t->kind == TYPEPOINTER || t->kind == TYPENULLPTR)
		return &typeulong;
	return t;
}

static bool
issigned(struct type *t)
{
	t = inttype(t);
	return t->prop & PROPINT && t->u.arith.issigned;
}

static unsigned long long
typesize(struct type *t)
{
	if (t->kind == TYPEVOID)
		return 0;
	if (t->kind == TYPEPOINTER || t->kind == TYPENULLPTR)
		return 8;
	return t->size;
}

static void
bufadd(struct array *buf, const void *src, size_t len)
{
	arrayaddbuf(buf, src, len);
}

static void
bufputs(struct array *buf, const char *s)
{
	bufadd(buf, s, strlen(s));
}

static void
bufprintf(struct array *buf, const char *fmt, ...)
{
	va_list ap;
	char *s;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (n < 0)
		fatal("vsnprintf failed");
	s = xmalloc((size_t)n + 1);
	va_start(ap, fmt);
	vsnprintf(s, (size_t)n + 1, fmt, ap);
	va_end(ap);
	bufadd(buf, s, (size_t)n);
	free(s);
}

static void
emit(struct func *f, const char *fmt, ...)
{
	va_list ap;
	char *s;
	int n;

	if (f->end->term)
		funclabel(f, mkblock("dead"));
	va_start(ap, fmt);
	n = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (n < 0)
		fatal("vsnprintf failed");
	s = xmalloc((size_t)n + 1);
	va_start(ap, fmt);
	vsnprintf(s, (size_t)n + 1, fmt, ap);
	va_end(ap);
	bufadd(&f->end->code, s, (size_t)n);
	free(s);
}

static void
emitnamebuf(struct array *buf, struct value *v)
{
	if (v->kind != VALUE_GLOBAL)
		fatal("internal error: expected global symbol");
	if (v->id) {
		if (v->u.name)
			bufprintf(buf, ".L%s.%u", v->u.name, v->id);
		else
			bufprintf(buf, ".L.%u", v->id);
	} else {
		bufputs(buf, v->u.name);
	}
}

static char *
symdup(struct value *v)
{
	struct array b = {0};
	char *s;

	emitnamebuf(&b, v);
	bufadd(&b, "", 1);
	s = b.val;
	return s;
}

static long long
imm(unsigned long long n)
{
	return (long long)n;
}

static const char *
reg32(const char *reg)
{
	if (strcmp(reg, "%rax") == 0) return "%eax";
	if (strcmp(reg, "%rcx") == 0) return "%ecx";
	if (strcmp(reg, "%rdx") == 0) return "%edx";
	if (strcmp(reg, "%rsi") == 0) return "%esi";
	if (strcmp(reg, "%rdi") == 0) return "%edi";
	if (strcmp(reg, "%r8") == 0) return "%r8d";
	if (strcmp(reg, "%r9") == 0) return "%r9d";
	if (strcmp(reg, "%r10") == 0) return "%r10d";
	if (strcmp(reg, "%r11") == 0) return "%r11d";
	fatal("internal error: no 32-bit name for register %s", reg);
}

static const char *
reg16(const char *reg)
{
	if (strcmp(reg, "%rax") == 0) return "%ax";
	if (strcmp(reg, "%rcx") == 0) return "%cx";
	if (strcmp(reg, "%rdx") == 0) return "%dx";
	if (strcmp(reg, "%rsi") == 0) return "%si";
	if (strcmp(reg, "%rdi") == 0) return "%di";
	if (strcmp(reg, "%r8") == 0) return "%r8w";
	if (strcmp(reg, "%r9") == 0) return "%r9w";
	if (strcmp(reg, "%r10") == 0) return "%r10w";
	if (strcmp(reg, "%r11") == 0) return "%r11w";
	fatal("internal error: no 16-bit name for register %s", reg);
}

static const char *
reg8(const char *reg)
{
	if (strcmp(reg, "%rax") == 0) return "%al";
	if (strcmp(reg, "%rcx") == 0) return "%cl";
	if (strcmp(reg, "%rdx") == 0) return "%dl";
	if (strcmp(reg, "%rsi") == 0) return "%sil";
	if (strcmp(reg, "%rdi") == 0) return "%dil";
	if (strcmp(reg, "%r8") == 0) return "%r8b";
	if (strcmp(reg, "%r9") == 0) return "%r9b";
	if (strcmp(reg, "%r10") == 0) return "%r10b";
	if (strcmp(reg, "%r11") == 0) return "%r11b";
	fatal("internal error: no 8-bit name for register %s", reg);
}

static long long
allocstack(struct func *f, unsigned long long size, int align)
{
	if (align < 1)
		align = 1;
	if (align > 16)
		align = 16;
	f->stack = ALIGNUP(f->stack, align);
	f->stack += ALIGNUP(size ? size : 1, align);
	return -f->stack;
}

static struct value *
mkstackvalue(struct func *f, struct type *t, unsigned long long size, int align, int kind)
{
	struct value *v;

	v = xmalloc(sizeof(*v));
	v->kind = kind;
	v->type = t;
	v->id = 0;
	v->off = allocstack(f, size, align);
	return v;
}

static struct value *
mktmp(struct func *f, struct type *t)
{
	return mkstackvalue(f, t, typesize(t), t->align ? t->align : 8, VALUE_TEMP);
}

void
switchcase(struct switchcases *cases, unsigned long long i, struct block *b)
{
	struct switchcase *c;

	c = treeinsert(&cases->root, i, sizeof(*c));
	if (!c->node.new)
		error(&tok.loc, "multiple 'case' labels with same value");
	c->body = b;
}

struct block *
mkblock(char *name)
{
	static unsigned id;
	struct block *b;

	b = xmalloc(sizeof(*b));
	b->name = name;
	b->id = ++id;
	b->term = false;
	b->code = (struct array){0};
	b->next = NULL;
	return b;
}

struct value *
mkglobal(struct decl *d)
{
	static unsigned id;
	struct array b = {0};
	struct value *v;

	if (d->kind == DECLOBJECT && d->u.obj.storage == SDTHREAD)
		error(&tok.loc, "thread-local storage is not supported by the amd64 backend");
	v = xmalloc(sizeof(*v));
	v->kind = VALUE_GLOBAL;
	v->type = d->type;
	v->off = 0;
	if (d->asmname) {
		v->u.name = d->asmname;
		v->id = 0;
	} else {
		v->u.name = d->name;
		v->id = d->linkage == LINKNONE ? ++id : 0;
	}
	if (v->id == 0 && (d->weak || d->alias)) {
		if (d->weak) {
			bufputs(&b, "\t.weak ");
			emitnamebuf(&b, v);
			bufputs(&b, "\n");
		}
		if (d->visibility == VISHIDDEN) {
			bufputs(&b, "\t.hidden ");
			emitnamebuf(&b, v);
			bufputs(&b, "\n");
		} else if (d->visibility == VISPROTECTED) {
			bufputs(&b, "\t.protected ");
			emitnamebuf(&b, v);
			bufputs(&b, "\n");
		}
		if (d->alias) {
			bufputs(&b, "\t.set ");
			emitnamebuf(&b, v);
			bufprintf(&b, ", %s\n", d->alias);
		}
		bufadd(&out, b.val, b.len);
		free(b.val);
	}
	return v;
}

struct value *
mkintconst(unsigned long long n)
{
	struct value *v;

	v = xmalloc(sizeof(*v));
	v->kind = VALUE_INTCONST;
	v->type = NULL;
	v->id = 0;
	v->off = 0;
	v->u.i = n;
	return v;
}

static struct value *
mkfltconst(struct type *t, long double n)
{
	struct value *v;

	v = xmalloc(sizeof(*v));
	v->kind = VALUE_FLTCONST;
	v->type = t;
	v->id = 0;
	v->off = 0;
	v->u.f = n;
	return v;
}

static void
addrtoreg(struct func *f, struct value *v, const char *reg)
{
	char *s;

	switch (v->kind) {
	case VALUE_GLOBAL:
		s = symdup(v);
		emit(f, "\tleaq %s(%%rip), %s\n", s, reg);
		free(s);
		break;
	case VALUE_STACK:
		emit(f, "\tleaq %lld(%%rbp), %s\n", v->off, reg);
		break;
	case VALUE_TEMP:
		if (isaggregate(v->type)) {
			emit(f, "\tleaq %lld(%%rbp), %s\n", v->off, reg);
		} else {
			loadfromslot(f, v, v->type, reg);
		}
		break;
	case VALUE_INTCONST:
		emit(f, "\tmovq $%lld, %s\n", imm(v->u.i), reg);
		break;
	default:
		fatal("internal error: cannot take address of value");
	}
}

static void
slotaddr(struct func *f, struct value *v, const char *reg)
{
	char *s;

	switch (v->kind) {
	case VALUE_GLOBAL:
		s = symdup(v);
		emit(f, "\tleaq %s(%%rip), %s\n", s, reg);
		free(s);
		break;
	case VALUE_STACK:
	case VALUE_TEMP:
		emit(f, "\tleaq %lld(%%rbp), %s\n", v->off, reg);
		break;
	default:
		fatal("internal error: value has no stack slot");
	}
}

static void
addrtooffsetreg(struct func *f, struct value *v, unsigned long long off, const char *reg)
{
	addrtoreg(f, v, reg);
	if (off)
		emit(f, "\tleaq %llu(%s), %s\n", off, reg, reg);
}

static void
loadfromaddr(struct func *f, struct value *addr, unsigned long long off, struct type *t, const char *reg)
{
	unsigned long long size = typesize(t);

	addrtooffsetreg(f, addr, off, "%r11");
	switch (size) {
	case 1:
		emit(f, "\t%s (%%r11), %s\n", issigned(t) ? "movsbq" : "movzbq", reg);
		break;
	case 2:
		emit(f, "\t%s (%%r11), %s\n", issigned(t) ? "movswq" : "movzwq", reg);
		break;
	case 4:
		if (issigned(t))
			emit(f, "\tmovslq (%%r11), %s\n", reg);
		else
			emit(f, "\tmovl (%%r11), %s\n", reg32(reg));
		break;
	case 8:
		emit(f, "\tmovq (%%r11), %s\n", reg);
		break;
	default:
		fatal("internal error: invalid scalar load size %llu", size);
	}
}

static void
loadfromslot(struct func *f, struct value *slot, struct type *t, const char *reg)
{
	unsigned long long size = typesize(t);

	slotaddr(f, slot, "%r11");
	switch (size) {
	case 1:
		emit(f, "\t%s (%%r11), %s\n", issigned(t) ? "movsbq" : "movzbq", reg);
		break;
	case 2:
		emit(f, "\t%s (%%r11), %s\n", issigned(t) ? "movswq" : "movzwq", reg);
		break;
	case 4:
		if (issigned(t))
			emit(f, "\tmovslq (%%r11), %s\n", reg);
		else
			emit(f, "\tmovl (%%r11), %s\n", reg32(reg));
		break;
	case 8:
		emit(f, "\tmovq (%%r11), %s\n", reg);
		break;
	default:
		fatal("internal error: invalid scalar load size %llu", size);
	}
}

static void
storetoaddr(struct func *f, struct value *addr, unsigned long long off, struct type *t, const char *reg)
{
	unsigned long long size = typesize(t);

	addrtooffsetreg(f, addr, off, "%r11");
	switch (size) {
	case 1:
		emit(f, "\tmovb %s, (%%r11)\n", reg8(reg));
		break;
	case 2:
		emit(f, "\tmovw %s, (%%r11)\n", reg16(reg));
		break;
	case 4:
		emit(f, "\tmovl %s, (%%r11)\n", reg32(reg));
		break;
	case 8:
		emit(f, "\tmovq %s, (%%r11)\n", reg);
		break;
	default:
		fatal("internal error: invalid scalar store size %llu", size);
	}
}

static void
storetoslot(struct func *f, struct value *slot, unsigned long long off, struct type *t, const char *reg)
{
	unsigned long long size = typesize(t);

	slotaddr(f, slot, "%r11");
	if (off)
		emit(f, "\tleaq %llu(%%r11), %%r11\n", off);
	switch (size) {
	case 1:
		emit(f, "\tmovb %s, (%%r11)\n", reg8(reg));
		break;
	case 2:
		emit(f, "\tmovw %s, (%%r11)\n", reg16(reg));
		break;
	case 4:
		emit(f, "\tmovl %s, (%%r11)\n", reg32(reg));
		break;
	case 8:
		emit(f, "\tmovq %s, (%%r11)\n", reg);
		break;
	default:
		fatal("internal error: invalid scalar store size %llu", size);
	}
}

static void
valtoreg(struct func *f, struct value *v, struct type *t, const char *reg)
{
	switch (v->kind) {
	case VALUE_INTCONST:
		emit(f, "\tmovq $%lld, %s\n", imm(v->u.i), reg);
		break;
	case VALUE_GLOBAL:
	case VALUE_STACK:
		addrtoreg(f, v, reg);
		break;
	case VALUE_TEMP:
		if (isfloat(t))
			fatal("internal error: integer load requested for floating value");
		loadfromslot(f, v, t, reg);
		break;
	default:
		fatal("internal error: invalid value kind");
	}
}

static void
bufldconst(struct array *buf, long double d)
{
	union {
		long double ld;
		unsigned char b[16];
	} u;
	int i;

	u.ld = d;
	for (i = 0; i < 10; ++i)
		bufprintf(buf, "\t.byte %u\n", u.b[i]);
	bufputs(buf, "\t.zero 6\n");
}

static char *
floatconst(long double d, struct type *t)
{
	static unsigned id;
	struct array b = {0};
	union {
		float f;
		uint32_t u;
	} sf;
	union {
		double d;
		uint64_t u;
	} df;
	char *name;

	bufprintf(&b, ".LCF%u", ++id);
	bufadd(&b, "", 1);
	name = b.val;
	bufputs(&out, "\t.section .rodata\n\t.balign ");
	bufprintf(&out, "%d\n", t->align);
	bufprintf(&out, "%s:\n", name);
	if (t->size == 4) {
		sf.f = (float)d;
		bufprintf(&out, "\t.long %" PRIu32 "\n", sf.u);
	} else if (t->size == 8) {
		df.d = d;
		bufprintf(&out, "\t.quad %" PRIu64 "\n", df.u);
	} else {
		bufldconst(&out, d);
	}
	return name;
}

static void
valtoxmm(struct func *f, struct value *v, struct type *t, const char *reg)
{
	char *name;

	assert(isssefloat(t));
	switch (v->kind) {
	case VALUE_FLTCONST:
		name = floatconst(v->u.f, t);
		emit(f, "\t%s %s(%%rip), %s\n", t->size == 4 ? "movss" : "movsd", name, reg);
		free(name);
		break;
	case VALUE_TEMP:
		emit(f, "\t%s %lld(%%rbp), %s\n", t->size == 4 ? "movss" : "movsd", v->off, reg);
		break;
	default:
		fatal("internal error: invalid floating value");
	}
}

static const char *
x87loadins(struct type *t)
{
	switch (t->size) {
	case 4: return "flds";
	case 8: return "fldl";
	case 16: return "fldt";
	default: fatal("internal error: invalid floating load size %llu", t->size);
	}
}

static const char *
x87storeins(struct type *t)
{
	switch (t->size) {
	case 4: return "fstps";
	case 8: return "fstpl";
	case 16: return "fstpt";
	default: fatal("internal error: invalid floating store size %llu", t->size);
	}
}

static void
zerox87padreg(struct func *f, const char *reg)
{
	emit(f, "\tmovw $0, 10(%s)\n", reg);
	emit(f, "\tmovl $0, 12(%s)\n", reg);
}

static void
valtox87(struct func *f, struct value *v, struct type *t)
{
	char *name;

	assert(isfloat(t));
	switch (v->kind) {
	case VALUE_FLTCONST:
		name = floatconst(v->u.f, t);
		emit(f, "\t%s %s(%%rip)\n", x87loadins(t), name);
		free(name);
		break;
	case VALUE_TEMP:
		emit(f, "\t%s %lld(%%rbp)\n", x87loadins(t), v->off);
		break;
	default:
		fatal("internal error: invalid floating value");
	}
}

static void
storexmmtoaddr(struct func *f, struct value *addr, unsigned long long off, struct type *t, const char *reg)
{
	addrtooffsetreg(f, addr, off, "%r11");
	emit(f, "\t%s %s, (%%r11)\n", t->size == 4 ? "movss" : "movsd", reg);
}

static void
storexmmtoslot(struct func *f, struct value *slot, unsigned long long off, struct type *t, const char *reg)
{
	slotaddr(f, slot, "%r11");
	if (off)
		emit(f, "\tleaq %llu(%%r11), %%r11\n", off);
	emit(f, "\t%s %s, (%%r11)\n", t->size == 4 ? "movss" : "movsd", reg);
}

static struct value *
storexmmtemp(struct func *f, struct type *t, const char *reg)
{
	struct value *v = mktmp(f, t);

	storexmmtoslot(f, v, 0, t, reg);
	return v;
}

static void
storex87toaddr(struct func *f, struct value *addr, unsigned long long off, struct type *t)
{
	addrtooffsetreg(f, addr, off, "%r11");
	emit(f, "\t%s (%%r11)\n", x87storeins(t));
	if (isx87(t))
		zerox87padreg(f, "%r11");
}

static void
storex87toslot(struct func *f, struct value *slot, unsigned long long off, struct type *t)
{
	slotaddr(f, slot, "%r11");
	if (off)
		emit(f, "\tleaq %llu(%%r11), %%r11\n", off);
	emit(f, "\t%s (%%r11)\n", x87storeins(t));
	if (isx87(t))
		zerox87padreg(f, "%r11");
}

static void
storex87tostack(struct func *f, unsigned long long off, struct type *t)
{
	emit(f, "\t%s %llu(%%rsp)\n", x87storeins(t), off);
	if (isx87(t)) {
		emit(f, "\tmovw $0, %llu(%%rsp)\n", off + 10);
		emit(f, "\tmovl $0, %llu(%%rsp)\n", off + 12);
	}
}

static struct value *
storex87temp(struct func *f, struct type *t)
{
	struct value *v = mktmp(f, t);

	storex87toslot(f, v, 0, t);
	return v;
}

static void
inttox87(struct func *f, struct value *v, struct type *t)
{
	struct value *slot;

	slot = mktmp(f, &typeullong);
	valtoreg(f, v, t, "%rax");
	storetoslot(f, slot, 0, &typeullong, "%rax");
	emit(f, "\tfildq %lld(%%rbp)\n", slot->off);
}

static struct value *
x87tointtemp(struct func *f, struct type *dst)
{
	struct value *cw, *slot;

	cw = mktmp(f, &typeuint);
	slot = mktmp(f, typesize(dst) <= 4 ? &typeint : &typeullong);
	emit(f, "\tfnstcw %lld(%%rbp)\n", cw->off);
	emit(f, "\tmovw %lld(%%rbp), %%ax\n", cw->off);
	emit(f, "\tandw $0xf3ff, %%ax\n");
	emit(f, "\torw $0x0c00, %%ax\n");
	emit(f, "\tmovw %%ax, %lld(%%rbp)\n", cw->off + 2);
	emit(f, "\tfldcw %lld(%%rbp)\n", cw->off + 2);
	emit(f, "\t%s %lld(%%rbp)\n", typesize(dst) <= 4 ? "fistpl" : "fistpq", slot->off);
	emit(f, "\tfldcw %lld(%%rbp)\n", cw->off);
	loadfromslot(f, slot, dst, "%rax");
	return storeregtemp(f, dst, "%rax");
}

static struct value *
floatbooltemp(struct func *f, struct value *v, struct type *t)
{
	if (isx87(t)) {
		valtox87(f, v, t);
		emit(f, "\tfldz\n");
		emit(f, "\tfucomip %%st(1), %%st\n");
		emit(f, "\tfstp %%st(0)\n");
	} else {
		valtoxmm(f, v, t, "%xmm0");
		emit(f, "\tpxor %%xmm1, %%xmm1\n");
		emit(f, "\tucomi%s %%xmm1, %%xmm0\n", t->size == 4 ? "ss" : "sd");
	}
	emit(f, "\tsetne %%al\n");
	emit(f, "\tmovzbq %%al, %%rax\n");
	return storeregtemp(f, &typebool, "%rax");
}

static struct value *
storeregtemp(struct func *f, struct type *t, const char *reg)
{
	struct value *v = mktmp(f, t);

	storetoslot(f, v, 0, t, reg);
	return v;
}

static void
copymem(struct func *f, struct value *dst, unsigned long long dstoff, struct value *src, unsigned long long srcoff, unsigned long long size)
{
	unsigned long long off = 0;

	addrtooffsetreg(f, dst, dstoff, "%rdi");
	addrtooffsetreg(f, src, srcoff, "%rsi");
	while (size >= 8) {
		emit(f, "\tmovq %llu(%%rsi), %%rax\n", off);
		emit(f, "\tmovq %%rax, %llu(%%rdi)\n", off);
		off += 8;
		size -= 8;
	}
	if (size >= 4) {
		emit(f, "\tmovl %llu(%%rsi), %%eax\n", off);
		emit(f, "\tmovl %%eax, %llu(%%rdi)\n", off);
		off += 4;
		size -= 4;
	}
	if (size >= 2) {
		emit(f, "\tmovw %llu(%%rsi), %%ax\n", off);
		emit(f, "\tmovw %%ax, %llu(%%rdi)\n", off);
		off += 2;
		size -= 2;
	}
	if (size) {
		emit(f, "\tmovb %llu(%%rsi), %%al\n", off);
		emit(f, "\tmovb %%al, %llu(%%rdi)\n", off);
	}
}

static void
copyfromreg(struct func *f, struct value *dst, unsigned long long dstoff, const char *srcreg, unsigned long long size)
{
	unsigned long long off = 0;

	addrtooffsetreg(f, dst, dstoff, "%rdi");
	while (size >= 8) {
		emit(f, "\tmovq %llu(%s), %%rax\n", off, srcreg);
		emit(f, "\tmovq %%rax, %llu(%%rdi)\n", off);
		off += 8;
		size -= 8;
	}
	if (size >= 4) {
		emit(f, "\tmovl %llu(%s), %%eax\n", off, srcreg);
		emit(f, "\tmovl %%eax, %llu(%%rdi)\n", off);
		off += 4;
		size -= 4;
	}
	if (size >= 2) {
		emit(f, "\tmovw %llu(%s), %%ax\n", off, srcreg);
		emit(f, "\tmovw %%ax, %llu(%%rdi)\n", off);
		off += 2;
		size -= 2;
	}
	if (size) {
		emit(f, "\tmovb %llu(%s), %%al\n", off, srcreg);
		emit(f, "\tmovb %%al, %llu(%%rdi)\n", off);
	}
}

static void
copytoreg(struct func *f, const char *dstreg, struct value *src, unsigned long long srcoff, unsigned long long size)
{
	unsigned long long off = 0;

	addrtooffsetreg(f, src, srcoff, "%rsi");
	while (size >= 8) {
		emit(f, "\tmovq %llu(%%rsi), %%rax\n", off);
		emit(f, "\tmovq %%rax, %llu(%s)\n", off, dstreg);
		off += 8;
		size -= 8;
	}
	if (size >= 4) {
		emit(f, "\tmovl %llu(%%rsi), %%eax\n", off);
		emit(f, "\tmovl %%eax, %llu(%s)\n", off, dstreg);
		off += 4;
		size -= 4;
	}
	if (size >= 2) {
		emit(f, "\tmovw %llu(%%rsi), %%ax\n", off);
		emit(f, "\tmovw %%ax, %llu(%s)\n", off, dstreg);
		off += 2;
		size -= 2;
	}
	if (size) {
		emit(f, "\tmovb %llu(%%rsi), %%al\n", off);
		emit(f, "\tmovb %%al, %llu(%s)\n", off, dstreg);
	}
}

static void
zeromem(struct func *f, struct value *dst, unsigned long long off, unsigned long long end)
{
	if (off >= end)
		return;
	emit(f, "\txorq %%rax, %%rax\n");
	while (end - off >= 8) {
		storetoaddr(f, dst, off, &typeulong, "%rax");
		off += 8;
	}
	if (end - off >= 4) {
		storetoaddr(f, dst, off, &typeuint, "%rax");
		off += 4;
	}
	if (end - off >= 2) {
		storetoaddr(f, dst, off, &typeushort, "%rax");
		off += 2;
	}
	if (end - off) {
		storetoaddr(f, dst, off, &typeuchar, "%rax");
	}
}

static struct value *
convert(struct func *f, struct type *dst, struct type *src, struct value *v)
{
	struct value *r;

	if (dst == &typevoid)
		return NULL;
	if (src == dst)
		return v;
	if (isaggregate(dst) || isaggregate(src))
		return v;
	if (dst->kind == TYPEBOOL) {
		if (isfloat(src))
			return floatbooltemp(f, v, src);
		valtoreg(f, v, src, "%rax");
		emit(f, "\ttestq %%rax, %%rax\n");
		emit(f, "\tsetne %%al\n");
		emit(f, "\tmovzbq %%al, %%rax\n");
		return storeregtemp(f, dst, "%rax");
	}
	if (isfloat(dst) && isfloat(src) && (isx87(dst) || isx87(src))) {
		valtox87(f, v, src);
		return storex87temp(f, dst);
	}
	if (isssefloat(dst) && isssefloat(src)) {
		valtoxmm(f, v, src, "%xmm0");
		if (dst->size == 8 && src->size == 4)
			emit(f, "\tcvtss2sd %%xmm0, %%xmm0\n");
		else if (dst->size == 4 && src->size == 8)
			emit(f, "\tcvtsd2ss %%xmm0, %%xmm0\n");
		return storexmmtemp(f, dst, "%xmm0");
	}
	if (isfloat(dst) && isintlike(src)) {
		if (isx87(dst)) {
			inttox87(f, v, src);
			return storex87temp(f, dst);
		}
		valtoreg(f, v, src, "%rax");
		if (dst->size == 4)
			emit(f, "\tcvtsi2ssq %%rax, %%xmm0\n");
		else
			emit(f, "\tcvtsi2sdq %%rax, %%xmm0\n");
		return storexmmtemp(f, dst, "%xmm0");
	}
	if (isintlike(dst) && isfloat(src)) {
		if (isx87(src)) {
			valtox87(f, v, src);
			return x87tointtemp(f, dst);
		}
		valtoxmm(f, v, src, "%xmm0");
		if (typesize(dst) <= 4)
			emit(f, "\t%s %%xmm0, %%eax\n", src->size == 4 ? "cvttss2sil" : "cvttsd2sil");
		else
			emit(f, "\t%s %%xmm0, %%rax\n", src->size == 4 ? "cvttss2siq" : "cvttsd2siq");
		return storeregtemp(f, dst, "%rax");
	}
	valtoreg(f, v, src, "%rax");
	r = storeregtemp(f, dst, "%rax");
	return r;
}

static void
calcvla(struct func *f, struct type *t)
{
	struct value *length, *basesize;

	if (!(t->prop & PROPVM))
		return;
	if (t->base)
		calcvla(f, t->base);
	if (t->kind == TYPEFUNC || t->size)
		return;
	assert(t->kind == TYPEARRAY);
	if (!t->u.array.size) {
		assert(t->base->size || t->base->kind == TYPEARRAY);
		assert(t->u.array.length);
		length = convert(f, &typeulong, t->u.array.length->type, funcexpr(f, t->u.array.length));
		basesize = t->base->size ? mkintconst(t->base->size) : t->base->u.array.size;
		valtoreg(f, length, &typeulong, "%rax");
		valtoreg(f, basesize, &typeulong, "%rcx");
		emit(f, "\timulq %%rcx, %%rax\n");
		t->u.array.size = storeregtemp(f, &typeulong, "%rax");
	}
}

static void
funcalloc(struct func *f, struct decl *d)
{
	struct value *v;

	assert(!d->type->incomplete);
	calcvla(f, d->type);
	if (!d->type->size) {
		if (!(d->type->prop & PROPVM) || d->type->kind != TYPEARRAY)
			error(&tok.loc, "variably modified object is not supported by the amd64 backend");
		v = mktmp(f, mkpointertype(d->type->base, d->type->qual));
		valtoreg(f, d->type->u.array.size, &typeulong, "%rax");
		emit(f, "\taddq $15, %%rax\n");
		emit(f, "\tandq $-16, %%rax\n");
		emit(f, "\tsubq %%rax, %%rsp\n");
		emit(f, "\tmovq %%rsp, %%rax\n");
		storetoslot(f, v, 0, v->type, "%rax");
		d->value = v;
		return;
	}
	v = mkstackvalue(f, d->type, d->type->size, d->u.obj.align, VALUE_STACK);
	d->value = v;
}

static unsigned long long
bitmask(unsigned width)
{
	return width >= 64 ? ~0ull : (1ull << width) - 1;
}

static struct value *
loadbitfield(struct func *f, struct type *t, struct lvalue lval)
{
	unsigned width = t->size * 8 - lval.bits.before - lval.bits.after;

	loadfromaddr(f, lval.addr, 0, t, "%rax");
	if (lval.bits.before)
		emit(f, "\tshrq $%d, %%rax\n", lval.bits.before);
	if (width < 64) {
		if (issigned(t)) {
			emit(f, "\tshlq $%u, %%rax\n", 64 - width);
			emit(f, "\tsarq $%u, %%rax\n", 64 - width);
		} else {
			emit(f, "\tmovq $%lld, %%rcx\n", imm(bitmask(width)));
			emit(f, "\tandq %%rcx, %%rax\n");
		}
	}
	return storeregtemp(f, t, "%rax");
}

static struct value *
storebitfield(struct func *f, struct type *t, struct lvalue lval, struct value *v)
{
	unsigned width = t->size * 8 - lval.bits.before - lval.bits.after;
	unsigned long long mask = bitmask(width);
	unsigned long long fieldmask = mask << lval.bits.before;

	valtoreg(f, v, t, "%rcx");
	if (width < 64) {
		emit(f, "\tmovq $%lld, %%rdx\n", imm(mask));
		emit(f, "\tandq %%rdx, %%rcx\n");
	}
	if (lval.bits.before)
		emit(f, "\tshlq $%d, %%rcx\n", lval.bits.before);
	loadfromaddr(f, lval.addr, 0, t, "%rax");
	emit(f, "\tmovq $%lld, %%rdx\n", imm(~fieldmask));
	emit(f, "\tandq %%rdx, %%rax\n");
	emit(f, "\torq %%rcx, %%rax\n");
	storetoaddr(f, lval.addr, 0, t, "%rax");
	return loadbitfield(f, t, lval);
}

static struct value *
funcstore(struct func *f, struct type *t, enum typequal tq, struct lvalue lval, struct value *v)
{
	if (tq & QUALCONST)
		error(&tok.loc, "cannot store to 'const' object");
	if (lval.bits.before || lval.bits.after)
		return storebitfield(f, t, lval, v);
	if (isaggregate(t)) {
		copymem(f, lval.addr, 0, v, 0, t->size);
		return lval.addr;
	}
	if (isx87(t)) {
		valtox87(f, v, t);
		storex87toaddr(f, lval.addr, 0, t);
		return v;
	}
	if (isfloat(t)) {
		valtoxmm(f, v, t, "%xmm0");
		storexmmtoaddr(f, lval.addr, 0, t, "%xmm0");
		return v;
	}
	valtoreg(f, v, t, "%rax");
	storetoaddr(f, lval.addr, 0, t, "%rax");
	return v;
}

static struct value *
funcload(struct func *f, struct type *t, struct lvalue lval)
{
	if (isaggregate(t))
		return lval.addr;
	if (lval.bits.before || lval.bits.after)
		return loadbitfield(f, t, lval);
	if (isx87(t)) {
		addrtooffsetreg(f, lval.addr, 0, "%r11");
		emit(f, "\tfldt (%%r11)\n");
		return storex87temp(f, t);
	}
	if (isfloat(t)) {
		addrtooffsetreg(f, lval.addr, 0, "%r11");
		emit(f, "\t%s (%%r11), %%xmm0\n", t->size == 4 ? "movss" : "movsd");
		return storexmmtemp(f, t, "%xmm0");
	}
	loadfromaddr(f, lval.addr, 0, t, "%rax");
	return storeregtemp(f, t, "%rax");
}

static int
arggp(struct type *t)
{
	if (isfloat(t))
		return 0;
	return 1;
}

static int
argfp(struct type *t)
{
	return isssefloat(t) ? 1 : 0;
}

static void
loadstackarg(struct func *f, long long off, struct type *t, struct value *dst)
{
	unsigned long long size = typesize(t), i = 0;

	slotaddr(f, dst, "%r10");
	while (size >= 8) {
		emit(f, "\tmovq %lld(%%rbp), %%rax\n", off + (long long)i);
		emit(f, "\tmovq %%rax, %llu(%%r10)\n", i);
		i += 8;
		size -= 8;
	}
	if (size >= 4) {
		emit(f, "\tmovl %lld(%%rbp), %%eax\n", off + (long long)i);
		emit(f, "\tmovl %%eax, %llu(%%r10)\n", i);
		i += 4;
		size -= 4;
	}
	if (size >= 2) {
		emit(f, "\tmovw %lld(%%rbp), %%ax\n", off + (long long)i);
		emit(f, "\tmovw %%ax, %llu(%%r10)\n", i);
		i += 2;
		size -= 2;
	}
	if (size) {
		emit(f, "\tmovb %lld(%%rbp), %%al\n", off + (long long)i);
		emit(f, "\tmovb %%al, %llu(%%r10)\n", i);
	}
}

static void
loadargtoslot(struct func *f, int slot, long long *stackoff, struct type *t, struct value *dst)
{
	int reg = slot;

	if (reg < 6) {
		emit(f, "\tmovq %s, %%rax\n", argreg64[reg]);
	} else {
		emit(f, "\tmovq %lld(%%rbp), %%rax\n", *stackoff);
		*stackoff += 8;
	}
	if (isaggregate(t)) {
		emit(f, "\tmovq %%rax, %%r10\n");
		copyfromreg(f, dst, 0, "%r10", t->size);
	} else {
		storetoslot(f, dst, 0, t, "%rax");
	}
}

struct func *
mkfunc(struct decl *decl, char *name, struct type *t, struct scope *s)
{
	static unsigned id;
	struct func *f;
	struct decl *d;
	int gpslot = 0, fpslot = 0, i;
	long long stackoff = 16;

	f = xmalloc(sizeof(*f));
	memset(f, 0, sizeof(*f));
	f->decl = decl;
	f->name = name;
	f->type = t;
	f->start = f->end = mkblock("start");
	f->id = ++id;
	mapinit(&f->gotos, 8);
	f->vararg_save = 0;
	f->vararg_overflow = 16;
	if (isaggregate(t->base)) {
		f->retptr = mkstackvalue(f, mkpointertype(t->base, QUALNONE), 8, 8, VALUE_TEMP);
		emit(f, "\tmovq %%rdi, %%rax\n");
		storetoslot(f, f->retptr, 0, &typeulong, "%rax");
		++gpslot;
	}
	if (t->u.func.isvararg) {
		f->vararg_save = allocstack(f, 176, 16);
		for (i = 0; i < 6; ++i)
			emit(f, "\tmovq %s, %lld(%%rbp)\n", argreg64[i], f->vararg_save + i * 8);
		for (i = 0; i < 8; ++i)
			emit(f, "\tmovaps %s, %lld(%%rbp)\n", xmmreg[i], f->vararg_save + 48 + i * 16);
	}
		for (d = t->u.func.params; d; d = d->next) {
			if (d->name) {
				funcalloc(f, d);
				if (isx87(d->type)) {
					loadstackarg(f, stackoff, d->type, d->value);
				} else if (isfloat(d->type)) {
				if (fpslot >= 8)
					error(&tok.loc, "floating point stack parameters are not supported by the amd64 backend");
				storexmmtoaddr(f, d->value, 0, d->type, xmmreg[fpslot]);
			} else {
				loadargtoslot(f, gpslot, &stackoff, d->type, d->value);
				}
			}
			if (isx87(d->type))
				stackoff += ALIGNUP(typesize(d->type), 8);
			else if (!isfloat(d->type) && !d->name && gpslot >= 6)
				stackoff += 8;
			gpslot += arggp(d->type);
		fpslot += argfp(d->type);
	}
	if (t->u.func.isvararg) {
		f->vararg_gp = gpslot * 8;
		if (f->vararg_gp > 48)
			f->vararg_gp = 48;
		f->vararg_fp = 48 + fpslot * 16;
		if (f->vararg_fp > 176)
			f->vararg_fp = 176;
		f->vararg_overflow = stackoff;
	}

	t = mkarraytype(&typechar, QUALCONST, strlen(name) + 1);
	d = mkdecl("__func__", DECLOBJECT, t, QUALNONE, LINKNONE);
	d->u.obj.storage = SDSTATIC;
	d->value = mkglobal(d);
	scopeputdecl(s, d);
	f->namedecl = d;

	funclabel(f, mkblock("body"));
	return f;
}

void
delfunc(struct func *f)
{
	struct block *b;

	while ((b = f->start)) {
		f->start = b->next;
		free(b->code.val);
		free(b);
	}
	mapfree(&f->gotos, free);
	free(f);
}

struct type *
functype(struct func *f)
{
	return f->type;
}

void
funclabel(struct func *f, struct block *b)
{
	f->end->next = b;
	f->end = b;
}

void
funcjmp(struct func *f, struct block *l)
{
	if (!f->end->term) {
		emit(f, "\tjmp .LB%u\n", l->id);
		f->end->term = true;
	}
}

void
funcjnz(struct func *f, struct value *v, struct type *t, struct block *l1, struct block *l2)
{
	if (f->end->term)
		return;
	if (!t)
		t = &typeint;
	if (isx87(t)) {
		valtox87(f, v, t);
		emit(f, "\tfldz\n");
		emit(f, "\tfucomip %%st(1), %%st\n");
		emit(f, "\tfstp %%st(0)\n");
	} else if (isfloat(t)) {
		valtoxmm(f, v, t, "%xmm0");
		emit(f, "\tpxor %%xmm1, %%xmm1\n");
		emit(f, "\tucomi%s %%xmm1, %%xmm0\n", t->size == 4 ? "ss" : "sd");
	} else {
		valtoreg(f, v, t, "%rax");
		emit(f, "\ttestq %%rax, %%rax\n");
	}
	emit(f, "\tjne .LB%u\n", l1->id);
	emit(f, "\tjmp .LB%u\n", l2->id);
	f->end->term = true;
}

void
funcret(struct func *f, struct value *v)
{
	struct type *rt = f->type->base;

	if (f->end->term)
		return;
	if (rt != &typevoid && v) {
			if (isaggregate(rt)) {
				loadfromslot(f, f->retptr, &typeulong, "%rdi");
				copytoreg(f, "%rdi", v, 0, rt->size);
				loadfromslot(f, f->retptr, &typeulong, "%rax");
			} else if (isx87(rt)) {
				valtox87(f, v, rt);
		} else if (isfloat(rt)) {
			valtoxmm(f, v, rt, "%xmm0");
		} else {
			valtoreg(f, v, rt, "%rax");
		}
	}
	emit(f, "\tjmp .Lret%u\n", f->id);
	f->end->term = true;
}

void
funchlt(struct func *f)
{
	if (!f->end->term) {
		emit(f, "\tud2\n");
		f->end->term = true;
	}
}

struct gotolabel *
funcgoto(struct func *f, char *name)
{
	struct gotolabel *g;
	struct mapkey key;
	size_t idx;

	mapkey(&key, name, strlen(name));
	if (mapput(&f->gotos, &key, &idx)) {
		g = xmalloc(sizeof(*g));
		g->label = mkblock(name);
		g->defined = false;
		f->gotos.vals[idx].p = g;
	}
	return f->gotos.vals[idx].p;
}

static void
emitfuncstring(struct func *f)
{
	struct array b = {0};
	size_t i;

	if (!f->namedecl)
		return;
	bufputs(&b, "\t.data\n\t.balign 1\n");
	emitnamebuf(&b, f->namedecl->value);
	bufputs(&b, ":\n");
	for (i = 0; i < strlen(f->name) + 1; ++i)
		bufprintf(&b, "\t.byte %u\n", (unsigned char)f->name[i]);
	bufadd(&out, b.val, b.len);
	free(b.val);
	f->namedecl = NULL;
}

static struct lvalue
funclval(struct func *f, struct expr *e)
{
	struct lvalue lval = {0};
	struct decl *d;

	if (e->kind == EXPRBITFIELD) {
		lval.bits = e->u.bitfield.bits;
		e = e->base;
	}
	switch (e->kind) {
	case EXPRIDENT:
		d = e->u.ident.decl;
		if (d->kind != DECLOBJECT && d->kind != DECLFUNC)
			error(&tok.loc, "identifier '%s' is not an object or function", d->name);
		if (d == f->namedecl)
			emitfuncstring(f);
		lval.addr = d->value;
		break;
	case EXPRSTRING:
		d = stringdecl(e);
		lval.addr = d->value;
		break;
	case EXPRCOMPOUND:
		if (e->toeval)
			funcexpr(f, e->toeval);
		d = e->u.compound.decl;
		funcinit(f, d, e->u.compound.init, true);
		lval.addr = d->value;
		break;
	case EXPRUNARY:
		if (e->op != TMUL)
			error(&tok.loc, "expression is not an object");
		lval.addr = funcexpr(f, e->base);
		break;
	default:
		if (e->type->kind != TYPESTRUCT && e->type->kind != TYPEUNION)
			error(&tok.loc, "expression is not an object");
		lval.addr = funcexpr(f, e);
	}
	return lval;
}

struct value *
funcbranch(struct func *f, struct expr *e, struct block *bt, struct block *bf)
{
	static struct value one = {.kind = VALUE_INTCONST, .u.i = 1};
	struct expr *l, *r;
	struct value *v;
	struct block *b;

	switch (e->kind) {
	case EXPRBINARY:
		l = e->u.binary.l;
		r = e->u.binary.r;
		switch (e->op) {
		case TEQL:
		case TNEQ:
			r = eval(r);
			if (r->kind == EXPRCONST && r->type->prop & PROPINT && r->u.constant.u == 0) {
				if (e->op == TEQL)
					b = bt, bt = bf, bf = b;
				funcbranch(f, l, bt, bf);
				return &one;
			}
			break;
		case TLOR:
		case TLAND:
			if (e->op == TLOR) {
				b = mkblock("logic_or");
				funcbranch(f, l, bt, b);
			} else {
				b = mkblock("logic_and");
				funcbranch(f, l, b, bf);
			}
			funclabel(f, b);
			funcbranch(f, r, bt, bf);
			return &one;
		}
		break;
	case EXPRCOMMA:
		for (e = e->base; e->next; e = e->next)
			funcexpr(f, e);
		return funcbranch(f, e, bt, bf);
	}
	v = funcexpr(f, e);
	funcjnz(f, v, e->type, bt, bf);
	return v;
}

static struct value *
binaryfloat(struct func *f, enum tokenkind op, struct type *t, struct value *l, struct value *r)
{
	const char *ins = NULL, *cc = NULL;

	if (isx87(t)) {
		valtox87(f, l, t);
		valtox87(f, r, t);
		switch (op) {
		case TADD: emit(f, "\tfaddp %%st, %%st(1)\n"); return storex87temp(f, t);
		case TSUB: emit(f, "\tfsubrp %%st, %%st(1)\n"); return storex87temp(f, t);
		case TMUL: emit(f, "\tfmulp %%st, %%st(1)\n"); return storex87temp(f, t);
		case TDIV: emit(f, "\tfdivrp %%st, %%st(1)\n"); return storex87temp(f, t);
		case TLESS:    cc = "seta";  break;
		case TGREATER: cc = "setb";  break;
		case TLEQ:     cc = "setae"; break;
		case TGEQ:     cc = "setbe"; break;
		case TEQL:     cc = "sete";  break;
		case TNEQ:     cc = "setne"; break;
		default: fatal("internal error: unsupported floating binary op");
		}
		emit(f, "\tfucomip %%st(1), %%st\n");
		emit(f, "\tfstp %%st(0)\n");
		emit(f, "\t%s %%al\n", cc);
		emit(f, "\tmovzbq %%al, %%rax\n");
		return storeregtemp(f, &typeint, "%rax");
	}
	valtoxmm(f, l, t, "%xmm0");
	valtoxmm(f, r, t, "%xmm1");
	switch (op) {
	case TADD: ins = t->size == 4 ? "addss" : "addsd"; break;
	case TSUB: ins = t->size == 4 ? "subss" : "subsd"; break;
	case TMUL: ins = t->size == 4 ? "mulss" : "mulsd"; break;
	case TDIV: ins = t->size == 4 ? "divss" : "divsd"; break;
	case TLESS:    cc = "setb";  break;
	case TGREATER: cc = "seta";  break;
	case TLEQ:     cc = "setbe"; break;
	case TGEQ:     cc = "setae"; break;
	case TEQL:     cc = "sete";  break;
	case TNEQ:     cc = "setne"; break;
	default: fatal("internal error: unsupported floating binary op");
	}
	if (ins) {
		emit(f, "\t%s %%xmm1, %%xmm0\n", ins);
		return storexmmtemp(f, t, "%xmm0");
	}
	emit(f, "\tucomi%s %%xmm1, %%xmm0\n", t->size == 4 ? "ss" : "sd");
	emit(f, "\t%s %%al\n", cc);
	emit(f, "\tmovzbq %%al, %%rax\n");
	return storeregtemp(f, &typeint, "%rax");
}

static struct value *
binaryint(struct func *f, enum tokenkind op, struct type *rt, struct type *lt, struct value *l, struct value *r)
{
	const char *cc = NULL;

	valtoreg(f, l, lt, "%rax");
	valtoreg(f, r, lt, "%rcx");
	switch (op) {
	case TADD:
		emit(f, "\taddq %%rcx, %%rax\n");
		break;
	case TSUB:
		emit(f, "\tsubq %%rcx, %%rax\n");
		break;
	case TMUL:
		emit(f, "\timulq %%rcx, %%rax\n");
		break;
	case TDIV:
	case TMOD:
		if (issigned(lt))
			emit(f, "\tcqto\n\tidivq %%rcx\n");
		else
			emit(f, "\txorq %%rdx, %%rdx\n\tdivq %%rcx\n");
		if (op == TMOD)
			emit(f, "\tmovq %%rdx, %%rax\n");
		break;
	case TSHL:
		emit(f, "\tshlq %%cl, %%rax\n");
		break;
	case TSHR:
		emit(f, "\t%s %%cl, %%rax\n", issigned(lt) ? "sarq" : "shrq");
		break;
	case TBOR:
		emit(f, "\torq %%rcx, %%rax\n");
		break;
	case TBAND:
		emit(f, "\tandq %%rcx, %%rax\n");
		break;
	case TXOR:
		emit(f, "\txorq %%rcx, %%rax\n");
		break;
	case TLESS:    cc = issigned(lt) ? "setl"  : "setb";  break;
	case TGREATER: cc = issigned(lt) ? "setg"  : "seta";  break;
	case TLEQ:     cc = issigned(lt) ? "setle" : "setbe"; break;
	case TGEQ:     cc = issigned(lt) ? "setge" : "setae"; break;
	case TEQL:     cc = "sete";  break;
	case TNEQ:     cc = "setne"; break;
	default:
		fatal("internal error: unsupported integer binary op");
	}
	if (cc) {
		emit(f, "\tcmpq %%rcx, %%rax\n");
		emit(f, "\t%s %%al\n", cc);
		emit(f, "\tmovzbq %%al, %%rax\n");
	}
	return storeregtemp(f, rt, "%rax");
}

static bool
directcall(struct expr *e, struct value **fn)
{
	if (e->kind != EXPRUNARY || e->op != TBAND)
		return false;
	e = e->base;
	if (e->kind != EXPRIDENT || e->u.ident.decl->kind != DECLFUNC)
		return false;
	*fn = e->u.ident.decl->value;
	return true;
}

static struct value *
funccall(struct func *f, struct expr *e)
{
	struct expr *arg;
	struct type *rt, **argtypes;
	struct value **args, *callee = NULL, *ret = NULL, *direct = NULL;
	size_t i, nargs, totalstack = 0, stackbytes, stackoff = 0;
	int gp = 0, fp = 0, al = 0;
	char *name;

	rt = e->type;
	nargs = e->u.call.nargs;
	args = xreallocarray(NULL, nargs, sizeof(args[0]));
	argtypes = xreallocarray(NULL, nargs, sizeof(argtypes[0]));
	for (arg = e->u.call.args, i = 0; arg; arg = arg->next, ++i) {
		argtypes[i] = arg->type;
		args[i] = funcexpr(f, arg);
	}
	if (!directcall(e->base, &direct))
		callee = funcexpr(f, e->base);
	if (isaggregate(rt)) {
		ret = mktmp(f, rt);
		++gp;
	}
	for (i = 0; i < nargs; ++i) {
		if (isx87(argtypes[i])) {
			totalstack += ALIGNUP(typesize(argtypes[i]), 8);
		} else if (isssefloat(argtypes[i])) {
			if (fp >= 8)
				error(&tok.loc, "floating point stack arguments are not supported by the amd64 backend");
			++fp;
		} else {
			if (gp >= 6)
				totalstack += 8;
			++gp;
		}
	}
	stackbytes = ALIGNUP(totalstack, 16);
	if (stackbytes)
		emit(f, "\tsubq $%zu, %%rsp\n", stackbytes);
	gp = isaggregate(rt) ? 1 : 0;
	fp = 0;
	if (isaggregate(rt)) {
		if (gp - 1 < 6) {
			addrtoreg(f, ret, argreg64[gp - 1]);
		} else {
			addrtoreg(f, ret, "%rax");
			emit(f, "\tmovq %%rax, %zu(%%rsp)\n", stackoff);
			stackoff += 8;
		}
	}
	for (i = 0; i < nargs; ++i) {
		if (isx87(argtypes[i])) {
			valtox87(f, args[i], argtypes[i]);
			storex87tostack(f, stackoff, argtypes[i]);
			stackoff += ALIGNUP(typesize(argtypes[i]), 8);
			continue;
		}
		if (isssefloat(argtypes[i])) {
			valtoxmm(f, args[i], argtypes[i], xmmreg[fp++]);
			continue;
		}
		if (isaggregate(argtypes[i]))
			addrtoreg(f, args[i], "%rax");
		else
			valtoreg(f, args[i], argtypes[i], "%rax");
		if (gp < 6) {
			emit(f, "\tmovq %%rax, %s\n", argreg64[gp]);
		} else {
			emit(f, "\tmovq %%rax, %zu(%%rsp)\n", stackoff);
			stackoff += 8;
		}
		++gp;
	}
	al = fp;
	if (callee) {
		valtoreg(f, callee, e->base->type, "%r11");
		emit(f, "\tmovb $%d, %%al\n", al);
		emit(f, "\tcall *%%r11\n");
	} else {
		name = symdup(direct);
		emit(f, "\tmovb $%d, %%al\n", al);
		emit(f, "\tcall %s\n", name);
		free(name);
	}
	if (stackbytes)
		emit(f, "\taddq $%zu, %%rsp\n", stackbytes);
	free(args);
	free(argtypes);
	if (isaggregate(rt))
		return ret;
	if (rt == &typevoid)
		return NULL;
	if (isx87(rt))
		return storex87temp(f, rt);
	if (isfloat(rt))
		return storexmmtemp(f, rt, "%xmm0");
	return storeregtemp(f, rt, "%rax");
}

static void
vaargoverflow(struct func *f, unsigned long long size, int align)
{
	if (align > 8) {
		emit(f, "\tmovq 8(%%rdi), %%r10\n");
		emit(f, "\taddq $%d, %%r10\n", align - 1);
		emit(f, "\tandq $-%d, %%r10\n", align);
	} else {
		emit(f, "\tmovq 8(%%rdi), %%r10\n");
	}
	emit(f, "\tleaq %llu(%%r10), %%rax\n", ALIGNUP(size, align > 8 ? align : 8));
	emit(f, "\tmovq %%rax, 8(%%rdi)\n");
}

static struct value *
funcvaarg(struct func *f, struct expr *apexpr, struct type *t)
{
	static unsigned id;
	struct value *ap, *v;
	unsigned n = ++id;
	unsigned long long size = typesize(t);
	int align = t->align ? t->align : 8;

	ap = funcexpr(f, apexpr);
	valtoreg(f, ap, apexpr->type, "%rdi");
	if (isssefloat(t)) {
		emit(f, "\tmovl 4(%%rdi), %%eax\n");
		emit(f, "\tcmpl $%d, %%eax\n", 176 - 16);
		emit(f, "\tja .Lva_stack%u\n", n);
		emit(f, "\tmovq 16(%%rdi), %%r10\n");
		emit(f, "\taddq %%rax, %%r10\n");
		emit(f, "\taddl $16, 4(%%rdi)\n");
		emit(f, "\tjmp .Lva_done%u\n", n);
		emit(f, ".Lva_stack%u:\n", n);
		vaargoverflow(f, size, align);
		emit(f, ".Lva_done%u:\n", n);
		} else if (isx87(t) || (isaggregate(t) && size > 8)) {
			vaargoverflow(f, size, align);
	} else {
		emit(f, "\tmovl 0(%%rdi), %%eax\n");
		emit(f, "\tcmpl $%d, %%eax\n", 48 - 8);
		emit(f, "\tja .Lva_stack%u\n", n);
		emit(f, "\tmovq 16(%%rdi), %%r10\n");
		emit(f, "\taddq %%rax, %%r10\n");
		emit(f, "\taddl $8, 0(%%rdi)\n");
		emit(f, "\tjmp .Lva_done%u\n", n);
		emit(f, ".Lva_stack%u:\n", n);
		vaargoverflow(f, size, align);
		emit(f, ".Lva_done%u:\n", n);
	}
	v = mktmp(f, t);
	if (isaggregate(t)) {
		copyfromreg(f, v, 0, "%r10", size);
	} else if (isx87(t)) {
		emit(f, "\tfldt (%%r10)\n");
		storex87toslot(f, v, 0, t);
	} else if (isfloat(t)) {
		emit(f, "\t%s (%%r10), %%xmm0\n", t->size == 4 ? "movss" : "movsd");
		storexmmtoslot(f, v, 0, t, "%xmm0");
	} else {
		switch (size) {
		case 1:
			emit(f, "\t%s (%%r10), %%rax\n", issigned(t) ? "movsbq" : "movzbq");
			break;
		case 2:
			emit(f, "\t%s (%%r10), %%rax\n", issigned(t) ? "movswq" : "movzwq");
			break;
		case 4:
			if (issigned(t))
				emit(f, "\tmovslq (%%r10), %%rax\n");
			else
				emit(f, "\tmovl (%%r10), %%eax\n");
			break;
		default:
			emit(f, "\tmovq (%%r10), %%rax\n");
			break;
		}
		storetoslot(f, v, 0, t, "%rax");
	}
	return v;
}

struct value *
funcexpr(struct func *f, struct expr *e)
{
	struct decl *d;
	struct value *l, *r, *v;
	struct lvalue lval;
	struct block *b[3];
	struct type *t;

	calcvla(f, e->type);
	switch (e->kind) {
	case EXPRIDENT:
		d = e->u.ident.decl;
		switch (d->kind) {
		case DECLOBJECT: return funcload(f, e->type, (struct lvalue){d->value});
		case DECLCONST:  return d->value;
		default: fatal("unimplemented declaration kind %d", d->kind);
		}
	case EXPRCONST:
		if (e->type->prop & PROPINT || e->type->kind == TYPEPOINTER || e->type->kind == TYPENULLPTR)
			return mkintconst(e->u.constant.u);
		if (e->type->prop & PROPFLOAT)
			return mkfltconst(e->type, e->u.constant.f);
		fatal("internal error: unsupported constant type");
	case EXPRBITFIELD:
	case EXPRCOMPOUND:
		lval = funclval(f, e);
		return funcload(f, e->type, lval);
	case EXPRINCDEC:
		lval = funclval(f, e->base);
		l = funcload(f, e->base->type, lval);
		t = e->type;
		if (t->kind == TYPEPOINTER)
			r = mkintconst(t->base->kind == TYPEARRAY && t->base->size == 0 ? 1 : t->base->size);
		else if (t->prop & PROPINT)
			r = mkintconst(1);
		else if (t->prop & PROPFLOAT)
			r = mkfltconst(t, 1);
		else
			fatal("not a scalar");
		v = isfloat(t) ? binaryfloat(f, e->op == TINC ? TADD : TSUB, t, l, r)
		               : binaryint(f, e->op == TINC ? TADD : TSUB, t, t, l, r);
		v = funcstore(f, e->type, e->qual, lval, v);
		return e->u.incdec.post ? l : v;
	case EXPRCALL:
		v = funccall(f, e);
		e = e->base;
		if (e->kind == EXPRUNARY && e->op == TBAND) {
			e = e->base;
			if (e->kind == EXPRIDENT && e->u.ident.decl->u.func.isnoreturn)
				funchlt(f);
		}
		return v;
	case EXPRUNARY:
		switch (e->op) {
		case TBAND:
			lval = funclval(f, e->base);
			return lval.addr;
			case TMUL:
				r = funcexpr(f, e->base);
				return funcload(f, e->type, (struct lvalue){r});
			case TSUB:
				r = funcexpr(f, e->base);
				if (isx87(e->type)) {
					valtox87(f, r, e->type);
				emit(f, "\tfchs\n");
				return storex87temp(f, e->type);
			}
			if (isfloat(e->type)) {
				valtoxmm(f, r, e->type, "%xmm0");
				if (e->type->size == 4) {
					emit(f, "\tmovl $1, %%eax\n");
					emit(f, "\tshll $31, %%eax\n");
					emit(f, "\tmovd %%eax, %%xmm1\n");
					emit(f, "\txorps %%xmm1, %%xmm0\n");
				} else {
					emit(f, "\tmovq $1, %%rax\n");
					emit(f, "\tshlq $63, %%rax\n");
					emit(f, "\tmovq %%rax, %%xmm1\n");
					emit(f, "\txorpd %%xmm1, %%xmm0\n");
				}
				return storexmmtemp(f, e->type, "%xmm0");
			}
			valtoreg(f, r, e->type, "%rax");
			emit(f, "\tnegq %%rax\n");
			return storeregtemp(f, e->type, "%rax");
		}
		fatal("internal error: unknown unary expression");
	case EXPRCAST:
		if (e->toeval)
			funcexpr(f, e->toeval);
		l = funcexpr(f, e->base);
		return convert(f, e->type, e->base->type, l);
	case EXPRBINARY:
		if (e->op == TLOR || e->op == TLAND) {
			v = mktmp(f, &typeint);
			b[0] = mkblock("logic_true");
			b[1] = mkblock("logic_false");
			b[2] = mkblock("logic_join");
			funcbranch(f, e, b[0], b[1]);
			funclabel(f, b[0]);
			emit(f, "\tmovq $1, %%rax\n");
			storetoslot(f, v, 0, &typeint, "%rax");
			funcjmp(f, b[2]);
			funclabel(f, b[1]);
			emit(f, "\txorq %%rax, %%rax\n");
			storetoslot(f, v, 0, &typeint, "%rax");
			funcjmp(f, b[2]);
			funclabel(f, b[2]);
			return v;
			}
			l = funcexpr(f, e->u.binary.l);
			r = funcexpr(f, e->u.binary.r);
			t = e->u.binary.l->type;
		if (t->kind == TYPEPOINTER)
			t = &typeulong;
		if (isfloat(t))
			return binaryfloat(f, e->op, t, l, r);
		return binaryint(f, e->op, e->type, t, l, r);
	case EXPRCOND:
		b[0] = mkblock("cond_true");
		b[1] = mkblock("cond_false");
		b[2] = mkblock("cond_join");
		funcbranch(f, e->base, b[0], b[1]);
		if (e->type == &typevoid) {
			funclabel(f, b[0]);
			funcexpr(f, e->u.cond.t);
			funcjmp(f, b[2]);
			funclabel(f, b[1]);
			funcexpr(f, e->u.cond.f);
			funcjmp(f, b[2]);
			funclabel(f, b[2]);
			return NULL;
		}
			v = mktmp(f, e->type);
			funclabel(f, b[0]);
			l = e->u.cond.t == e->base ? mkintconst(1) : funcexpr(f, e->u.cond.t);
			l = convert(f, e->type, e->u.cond.t->type, l);
			if (isaggregate(e->type)) {
				funcstore(f, e->type, QUALNONE, (struct lvalue){v}, l);
		} else if (isx87(e->type)) {
			valtox87(f, l, e->type);
			storex87toslot(f, v, 0, e->type);
		} else if (isfloat(e->type)) {
			valtoxmm(f, l, e->type, "%xmm0");
			storexmmtoslot(f, v, 0, e->type, "%xmm0");
		} else {
			valtoreg(f, l, e->type, "%rax");
			storetoslot(f, v, 0, e->type, "%rax");
		}
		funcjmp(f, b[2]);
			funclabel(f, b[1]);
			r = funcexpr(f, e->u.cond.f);
			r = convert(f, e->type, e->u.cond.f->type, r);
			if (isaggregate(e->type)) {
				funcstore(f, e->type, QUALNONE, (struct lvalue){v}, r);
		} else if (isx87(e->type)) {
			valtox87(f, r, e->type);
			storex87toslot(f, v, 0, e->type);
		} else if (isfloat(e->type)) {
			valtoxmm(f, r, e->type, "%xmm0");
			storexmmtoslot(f, v, 0, e->type, "%xmm0");
		} else {
			valtoreg(f, r, e->type, "%rax");
			storetoslot(f, v, 0, e->type, "%rax");
		}
		funcjmp(f, b[2]);
		funclabel(f, b[2]);
		return v;
	case EXPRASSIGN:
		r = funcexpr(f, e->u.assign.r);
		if (e->u.assign.l->kind == EXPRTEMP) {
			e->u.assign.l->u.temp = r;
		} else {
			lval = funclval(f, e->u.assign.l);
			r = funcstore(f, e->u.assign.l->type, e->u.assign.l->qual, lval, r);
		}
		return r;
	case EXPRCOMMA:
		for (e = e->base; e->next; e = e->next)
			funcexpr(f, e);
		return funcexpr(f, e);
	case EXPRBUILTIN:
		switch (e->u.builtin.kind) {
		case BUILTINVASTART:
			l = funcexpr(f, e->base);
			valtoreg(f, l, e->base->type, "%rdi");
			emit(f, "\tmovl $%d, 0(%%rdi)\n", f->vararg_gp);
			emit(f, "\tmovl $%d, 4(%%rdi)\n", f->vararg_fp);
			emit(f, "\tleaq %lld(%%rbp), %%rax\n", f->vararg_overflow);
			emit(f, "\tmovq %%rax, 8(%%rdi)\n");
			emit(f, "\tleaq %lld(%%rbp), %%rax\n", f->vararg_save);
			emit(f, "\tmovq %%rax, 16(%%rdi)\n");
			return NULL;
			case BUILTINALLOCA:
				l = funcexpr(f, e->base);
				valtoreg(f, l, e->base->type, "%rax");
				emit(f, "\taddq $15, %%rax\n");
				emit(f, "\tandq $-16, %%rax\n");
				emit(f, "\tsubq %%rax, %%rsp\n");
				v = mktmp(f, e->type);
				emit(f, "\tmovq %%rsp, %%rax\n");
				storetoslot(f, v, 0, e->type, "%rax");
				return v;
			case BUILTINUNREACHABLE:
				funchlt(f);
			return NULL;
		case BUILTINVAARG:
			return funcvaarg(f, e->base, e->type);
		default:
			fatal("internal error: unimplemented builtin");
		}
	case EXPRTEMP:
		assert(e->u.temp);
		return e->u.temp;
	case EXPRSIZEOF:
		t = e->u.szof.type;
		assert(t->kind == TYPEARRAY);
		calcvla(f, t);
		if (e->base)
			funcexpr(f, e->base);
		return t->u.array.size;
	}
	fatal("unimplemented expression %d", e->kind);
	return NULL;
}

void
funcinit(struct func *func, struct decl *d, struct init *init, bool hasinit)
{
	struct lvalue dst;
	struct value *src, *v;
	unsigned long long offset = 0, max = 0;
	size_t i, w;
	struct type *t;

	funcalloc(func, d);
	if (!hasinit)
		return;
	for (; init; init = init->next) {
		zeromem(func, d->value, offset, init->start);
		dst.bits = init->bits;
		if (init->expr->kind == EXPRSTRING) {
			w = init->expr->type->base->size;
			for (i = 0; i < init->expr->u.string.size && i * w < init->end - init->start; ++i) {
				v = mkintconst(0);
				switch (w) {
				case 1: v->u.i = ((unsigned char *)init->expr->u.string.data)[i]; break;
				case 2: v->u.i = ((uint_least16_t *)init->expr->u.string.data)[i]; break;
				case 4: v->u.i = ((uint_least32_t *)init->expr->u.string.data)[i]; break;
				default: assert(0);
				}
				dst.addr = d->value;
				t = init->expr->type->base;
				valtoreg(func, v, t, "%rax");
				storetoaddr(func, dst.addr, init->start + i * w, t, "%rax");
			}
			offset = init->start + i * w;
		} else {
			if (offset < init->end && (dst.bits.before || dst.bits.after))
				zeromem(func, d->value, offset, init->end);
			dst.addr = d->value;
			if (init->start > 0) {
				struct value *addr = mktmp(func, mkpointertype(&typevoid, QUALNONE));
				addrtooffsetreg(func, dst.addr, init->start, "%rax");
				storetoslot(func, addr, 0, addr->type, "%rax");
				dst.addr = addr;
			}
			src = funcexpr(func, init->expr);
			funcstore(func, init->expr->type, QUALNONE, dst, src);
			offset = init->end;
		}
		if (max < offset)
			max = offset;
	}
	zeromem(func, d->value, max, d->type->size);
}

static void
casesearch(struct func *f, struct value *v, struct switchcase *c, struct block *defaultlabel, unsigned long long min, unsigned long long max, struct type *t)
{
	struct block *label[3];
	struct value *key, *res;
	struct type *ut;

	if (!c) {
		funcjmp(f, defaultlabel);
		return;
	}
	if (min == max) {
		funcjmp(f, c->body);
		return;
	}
	label[0] = mkblock("switch_ne");
	label[1] = mkblock("switch_lt");
	label[2] = mkblock("switch_gt");
	key = mkintconst(c->node.key);
	res = binaryint(f, TEQL, &typeint, t, v, key);
	funcjnz(f, res, &typeint, c->body, label[0]);
	funclabel(f, label[0]);
	ut = typesize(t) == 8 ? &typeulong : &typeuint;
	res = binaryint(f, TLESS, &typeint, ut, v, key);
	funcjnz(f, res, &typeint, label[1], label[2]);
	funclabel(f, label[1]);
	casesearch(f, v, c->node.child[0], defaultlabel, min, c->node.key - 1, t);
	funclabel(f, label[2]);
	casesearch(f, v, c->node.child[1], defaultlabel, c->node.key + 1, max, t);
}

void
funcswitch(struct func *f, struct value *v, struct switchcases *c, struct block *defaultlabel)
{
	casesearch(f, v, c->root, defaultlabel, 0, -1, c->type);
}

static const char *
datadir(unsigned long long size)
{
	switch (size) {
	case 1: return ".byte";
	case 2: return ".short";
	case 4: return ".long";
	case 8: return ".quad";
	default: return NULL;
	}
}

static void dataitem(struct array *, struct expr *, unsigned long long);

static void
dataaddr(struct array *buf, struct expr *expr)
{
	struct decl *decl;

	switch (expr->kind) {
	case EXPRUNARY:
		if (expr->op != TBAND)
			error(&tok.loc, "initializer is not a constant expression");
		expr = expr->base;
		if (expr->kind != EXPRIDENT)
			error(&tok.loc, "initializer is not a constant expression");
		decl = expr->u.ident.decl;
		if (decl->kind == DECLOBJECT && decl->u.obj.storage != SDSTATIC)
			error(&tok.loc, "initializer is not a constant expression");
		emitnamebuf(buf, decl->value);
		break;
	case EXPRBINARY:
		if (expr->op != TADD)
			error(&tok.loc, "initializer is not a constant expression");
		dataaddr(buf, expr->u.binary.l);
		bufputs(buf, "+");
		dataitem(buf, expr->u.binary.r, 0);
		break;
	default:
		error(&tok.loc, "initializer is not a constant expression");
	}
}

static void
dataitem(struct array *buf, struct expr *expr, unsigned long long size)
{
	union {
		float f;
		uint32_t u;
	} sf;
	union {
		double d;
		uint64_t u;
	} df;

	switch (expr->kind) {
	case EXPRUNARY:
	case EXPRBINARY:
		dataaddr(buf, expr);
		break;
	case EXPRCONST:
		if (expr->type->prop & PROPFLOAT) {
			if (expr->type->size == 4) {
				sf.f = (float)expr->u.constant.f;
				bufprintf(buf, "%" PRIu32, sf.u);
			} else {
				df.d = expr->u.constant.f;
				bufprintf(buf, "%" PRIu64, df.u);
			}
		} else {
			bufprintf(buf, "%llu", expr->u.constant.u);
		}
		break;
	case EXPRSTRING:
		(void)size;
		fatal("internal error: string data item should be emitted by caller");
	default:
		error(&tok.loc, "initializer is not a constant expression");
	}
}

static void
emitzeros(struct array *buf, unsigned long long n)
{
	if (n)
		bufprintf(buf, "\t.zero %llu\n", n);
}

void
emitdata(struct decl *d, struct init *init)
{
	struct array b = {0};
	struct init *cur;
	struct type *t;
	unsigned long long offset = 0, start, end, bits = 0;
	size_t i, w;
	int align;
	const char *dir;

	align = d->u.obj.align;
	for (cur = init; cur; cur = cur->next)
		cur->expr = eval(cur->expr);
	bufputs(&b, "\t.data\n");
	if (d->linkage == LINKEXTERN) {
		bufputs(&b, "\t.globl ");
		emitnamebuf(&b, d->value);
		bufputs(&b, "\n");
	}
	bufprintf(&b, "\t.balign %d\n", align);
	emitnamebuf(&b, d->value);
	bufputs(&b, ":\n");
	while (init) {
		cur = init;
		while ((init = init->next) && init->start * 8 + init->bits.before < cur->end * 8 - cur->bits.after) {
			assert(cur->expr->kind == EXPRSTRING);
			assert(init->expr->kind == EXPRCONST);
			i = (init->start - cur->start) / cur->expr->type->base->size;
			switch (cur->expr->type->base->size) {
			case 1: ((unsigned char *)cur->expr->u.string.data)[i]  = init->expr->u.constant.u; break;
			case 2: ((uint_least16_t *)cur->expr->u.string.data)[i] = init->expr->u.constant.u; break;
			case 4: ((uint_least32_t *)cur->expr->u.string.data)[i] = init->expr->u.constant.u; break;
			}
		}
		start = cur->start + cur->bits.before / 8;
		end = cur->end - (cur->bits.after + 7) / 8;
		if (offset < start && bits) {
			bufprintf(&b, "\t.byte %u\n", (unsigned)bits);
			++offset;
			bits = 0;
		}
		emitzeros(&b, start - offset);
		if (cur->bits.before || cur->bits.after) {
			assert(cur->expr->type->prop & PROPINT);
			assert(cur->expr->kind == EXPRCONST);
			bits |= cur->expr->u.constant.u << cur->bits.before % 8;
			for (offset = start; offset < end; ++offset, bits >>= 8)
				bufprintf(&b, "\t.byte %u\n", (unsigned)bits & 0xff);
			bits &= 0x7f >> (cur->bits.after + 7) % 8;
		} else if (cur->expr->kind == EXPRSTRING) {
			w = cur->expr->type->base->size;
			for (i = 0; i < cur->expr->u.string.size && i * w < cur->end - cur->start; ++i) {
				switch (w) {
				case 1: bufprintf(&b, "\t.byte %u\n", ((unsigned char *)cur->expr->u.string.data)[i]); break;
				case 2: bufprintf(&b, "\t.short %" PRIuLEAST16 "\n", ((uint_least16_t *)cur->expr->u.string.data)[i]); break;
				case 4: bufprintf(&b, "\t.long %" PRIuLEAST32 "\n", ((uint_least32_t *)cur->expr->u.string.data)[i]); break;
				default: assert(0);
				}
			}
			offset = cur->start + i * w;
			emitzeros(&b, cur->end - offset);
		} else {
			t = cur->expr->type;
			if (t->kind == TYPEARRAY)
				t = t->base;
			if (isx87(t) && cur->expr->kind == EXPRCONST) {
				bufldconst(&b, cur->expr->u.constant.f);
			} else {
				dir = datadir(typesize(t));
				if (!dir)
					fatal("internal error: unsupported data item size %llu", typesize(t));
				bufprintf(&b, "\t%s ", dir);
				dataitem(&b, cur->expr, cur->end - cur->start);
				bufputs(&b, "\n");
			}
			offset = end;
		}
		offset = end > offset ? end : offset;
	}
	if (bits) {
		bufprintf(&b, "\t.byte %u\n", (unsigned)bits);
		++offset;
	}
	assert(offset <= d->type->size);
	emitzeros(&b, d->type->size - offset);
	bufadd(&out, b.val, b.len);
	free(b.val);
}

void
emitfunc(struct func *f, bool global)
{
	struct array b = {0};
	struct block *blk;
	struct value *v = NULL;
	long long frame;

	if (!f->end->term) {
		if (strcmp(f->name, "main") == 0 && f->type->base == &typeint)
			v = mkintconst(0);
		funcret(f, v);
	}
	frame = ALIGNUP(f->stack, 16);
	bufputs(&b, "\t.text\n");
	if (global) {
		bufputs(&b, "\t.globl ");
		emitnamebuf(&b, f->decl->value);
		bufputs(&b, "\n");
	}
	bufputs(&b, "\t.balign 16\n");
	emitnamebuf(&b, f->decl->value);
	bufputs(&b, ":\n");
	bufputs(&b, "\tpushq %rbp\n\tmovq %rsp, %rbp\n");
	if (frame)
		bufprintf(&b, "\tsubq $%lld, %%rsp\n", frame);
	for (blk = f->start; blk; blk = blk->next) {
		bufprintf(&b, ".LB%u:\n", blk->id);
		bufadd(&b, blk->code.val, blk->code.len);
	}
	bufprintf(&b, ".Lret%u:\n", f->id);
	bufputs(&b, "\tleave\n\tret\n");
	bufadd(&out, b.val, b.len);
	free(b.val);
}

void
emitfinish(void)
{
	bufputs(&out, "\t.section .note.GNU-stack,\"\",@progbits\n");
	if (out.len && fwrite(out.val, 1, out.len, stdout) != out.len)
		fatal("write failed");
}
