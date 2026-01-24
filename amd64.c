#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "cc.h"

#define GP_MAX 6
#define FP_MAX 8

struct strbuf {
	char *buf;
	size_t len;
	size_t cap;
};

static void
sbgrow(struct strbuf *sb, size_t add)
{
	size_t need;

	need = sb->len + add + 1;
	if (need <= sb->cap)
		return;
	if (!sb->cap)
		sb->cap = 256;
	while (sb->cap < need)
		sb->cap *= 2;
	sb->buf = xreallocarray(sb->buf, sb->cap, 1);
}

enum valkind {
	V_NONE,
	V_GLOBAL,
	V_INTCONST,
	V_LOCAL,
	V_PARAMREF,
	V_TEMP,
};

struct value {
	enum valkind kind;
	unsigned id;
	bool thread;
	long offset;
	union {
		char *name;
		unsigned long long i;
	} u;
};

struct block {
	char *name;
	char *label;
	unsigned id;
	bool placed;
	struct strbuf code;
	struct block *next;
};

struct func {
	struct decl *decl;
	char *name;
	struct type *type;
	struct block *start;
	struct block *last;
	struct block *cur;
	struct map gotos;
	unsigned next_label;
	long stack_size;
	int depth;
	long vararg_overflow;
	long vararg_regsave;
	int vararg_gp_offset;
	int vararg_fp_offset;
	long retbuf_offset;
	bool is_vararg;
};

struct lvalue {
	struct value *addr;
	struct bitfield bits;
};

struct switchcase {
	struct treenode node;
	struct block *body;
};

static struct func *curfunc;

static const char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8b", "%r9b"};
static const char *argreg16[] = {"%di", "%si", "%dx", "%cx", "%r8w", "%r9w"};
static const char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static const char *argreg64[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static unsigned ldconst_id;

static int is_flonum(struct type *t);
static int is_longdouble(struct type *t);
static struct type *basetype(struct type *t);
static void funccopy(struct func *f, int size);
static void eval_vla(struct func *f, struct type *t);

static int
align_to(int n, int align)
{
	return (n + align - 1) / align * align;
}

static long
alloc_stack(struct func *f, int size, int align)
{
	f->stack_size = align_to((int)f->stack_size + size, align);
	return -f->stack_size;
}

static void
emitf(struct func *f, const char *fmt, ...)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(NULL, 0, fmt, ap);
	va_end(ap);
	if (n < 0)
		fatal("vsnprintf failed");
	sbgrow(&f->cur->code, (size_t)n);
	va_start(ap, fmt);
	vsnprintf(f->cur->code.buf + f->cur->code.len, f->cur->code.cap - f->cur->code.len, fmt, ap);
	va_end(ap);
	f->cur->code.len += (size_t)n;
}

static struct value *
mkvalue(enum valkind kind)
{
	struct value *v;

	v = xmalloc(sizeof(*v));
	*v = (struct value){0};
	v->kind = kind;
	return v;
}

struct value *
mkintconst(unsigned long long n)
{
	struct value *v;

	v = mkvalue(V_INTCONST);
	v->u.i = n;
	return v;
}

struct value *
mkglobal(struct decl *d)
{
	static unsigned id;
	struct value *v;

	v = mkvalue(V_GLOBAL);
	v->thread = d->kind == DECLOBJECT && d->u.obj.storage == SDTHREAD;
	if (d->asmname) {
		v->u.name = d->asmname;
		v->id = 0;
	} else if (d->linkage == LINKNONE) {
		v->u.name = d->name ? d->name : "obj";
		v->id = ++id;
	} else {
		v->u.name = d->name;
		v->id = 0;
	}
	return v;
}

struct block *
mkblock(char *name)
{
	struct block *b;
	char label[128];

	b = xmalloc(sizeof(*b));
	*b = (struct block){0};
	b->name = name;
	b->id = ++curfunc->next_label;
	snprintf(label, sizeof(label), ".L%s_%u", curfunc->name, b->id);
	b->label = xmalloc(strlen(label) + 1);
	strcpy(b->label, label);
	return b;
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

struct func *
mkfunc(struct decl *decl, char *name, struct type *t, struct scope *s)
{
	struct func *f;
	struct decl *d;
	int nparam = 0;

	f = xmalloc(sizeof(*f));
	*f = (struct func){0};
	f->decl = decl;
	f->name = name;
	f->type = t;
	f->is_vararg = t->u.func.isvararg;
	f->retbuf_offset = 0;
	mapinit(&f->gotos, 8);
	curfunc = f;

	f->start = f->last = mkblock("entry");
	f->start->placed = true;
	f->cur = f->start;

	/* reserve slot for hidden return buffer if returning struct/union */
	if (t->base->kind == TYPESTRUCT || t->base->kind == TYPEUNION) {
		f->retbuf_offset = alloc_stack(f, 8, 8);
	}

	/* allocate stack slots for parameters */
	for (d = t->u.func.params; d; d = d->next) {
		struct value *pv;
		int align = d->type->align;
		int size = (int)d->type->size;

		if (d->type->kind == TYPESTRUCT || d->type->kind == TYPEUNION || d->type->kind == TYPEARRAY) {
			/* pass by reference, store pointer */
			align = 8;
			size = 8;
			pv = mkvalue(V_PARAMREF);
		} else {
			pv = mkvalue(V_LOCAL);
		}
		pv->offset = alloc_stack(f, size, align);
		d->value = pv;
		nparam++;
	}

	if (f->is_vararg) {
		int gp = 0;
		int fp = 0;
		int stack_bytes = 0;

		if (t->base->kind == TYPESTRUCT || t->base->kind == TYPEUNION)
			gp = 1;
		for (d = t->u.func.params; d; d = d->next) {
			struct type *pt = d->type;
			if (pt->kind == TYPESTRUCT || pt->kind == TYPEUNION || pt->kind == TYPEARRAY)
				pt = mkpointertype(pt, QUALNONE);
			if (is_longdouble(pt)) {
				int pad = (16 - (stack_bytes % 16)) % 16;
				stack_bytes += pad + 16;
			} else if (is_flonum(pt)) {
				if (fp < FP_MAX) {
					fp++;
				} else {
					stack_bytes += 8;
				}
			} else {
				if (gp < GP_MAX) {
					gp++;
				} else {
					stack_bytes += 8;
				}
			}
		}
		f->vararg_gp_offset = gp * 8;
		f->vararg_fp_offset = 48 + fp * 16;
		f->vararg_overflow = 16 + stack_bytes;
		f->vararg_regsave = alloc_stack(f, 48 + 16 * FP_MAX, 16);
	}

	for (d = t->u.func.params; d; d = d->next) {
		if (d->type->prop & PROPVM)
			eval_vla(f, d->type);
	}

	/* set up __func__ */
	{
		struct type *ct;
		struct decl *fd;
		struct expr *e;
		char *buf;
		size_t len;
		ct = mkarraytype(&typechar, QUALCONST, strlen(name) + 1);
		fd = mkdecl("__func__", DECLOBJECT, ct, QUALNONE, LINKNONE);
		fd->u.obj.storage = SDSTATIC;
		fd->value = mkglobal(fd);
		scopeputdecl(s, fd);

		len = strlen(name) + 1;
		buf = xmalloc(len);
		memcpy(buf, name, len);
		e = xmalloc(sizeof(*e));
		*e = (struct expr){0};
		e->kind = EXPRSTRING;
		e->type = ct;
		e->lvalue = true;
		e->u.string.size = len;
		e->u.string.data = buf;
		emitdata(fd, mkinit(0, ct->size, (struct bitfield){0}, e));
	}

	return f;
}


void
delfunc(struct func *f)
{
	struct block *b;

	while (b = f->start) {
		f->start = b->next;
		free(b->code.buf);
		free(b->label);
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
	if (!b->placed) {
		f->last->next = b;
		f->last = b;
		b->placed = true;
	}
	f->cur = b;
}

void
funcjmp(struct func *f, struct block *l)
{
	emitf(f, "\tjmp %s\n", l->label);
}

static void cmp_zero(struct func *f, struct type *t);

void
funcjnz(struct func *f, struct value *v, struct type *t, struct block *l1, struct block *l2)
{
	(void)v;
	t = basetype(t);
	if (is_longdouble(t)) {
		emitf(f, "\tfldz\n");
		emitf(f, "\tfxch %%st(1)\n");
		emitf(f, "\tfucomip %%st(1), %%st\n");
		emitf(f, "\tfstp %%st(0)\n");
		return;
	}
	if (is_flonum(t)) {
		if (t->size == 4) {
			emitf(f, "\txorps %%xmm1, %%xmm1\n");
			emitf(f, "\tucomiss %%xmm1, %%xmm0\n");
		} else {
			emitf(f, "\txorpd %%xmm1, %%xmm1\n");
			emitf(f, "\tucomisd %%xmm1, %%xmm0\n");
		}
		emitf(f, "\tjp %s\n", l1->label);
		emitf(f, "\tjne %s\n", l1->label);
		emitf(f, "\tjmp %s\n", l2->label);
		return;
	}
	cmp_zero(f, t);
	emitf(f, "\tjne %s\n", l1->label);
	emitf(f, "\tjmp %s\n", l2->label);
}

void
funcret(struct func *f, struct value *v)
{
	(void)v;
	if (f->type->base->kind == TYPESTRUCT || f->type->base->kind == TYPEUNION) {
		/* copy return value into hidden retbuf */
		emitf(f, "\tmov %ld(%%rbp), %%rdi\n", f->retbuf_offset);
		emitf(f, "\tmov %%rax, %%rsi\n");
		funccopy(f, (int)f->type->base->size);
		emitf(f, "\tmov %ld(%%rbp), %%rax\n", f->retbuf_offset);
	}
	emitf(f, "\tleave\n\tret\n");
}

void
funchlt(struct func *f)
{
	(void)f;
	emitf(curfunc, "\tud2\n");
}

struct gotolabel *
funcgoto(struct func *f, char *name)
{
	void **entry;
	struct gotolabel *g;
	struct mapkey key;

	mapkey(&key, name, strlen(name));
	entry = mapput(&f->gotos, &key);
	g = *entry;
	if (!g) {
		g = xmalloc(sizeof(*g));
		g->label = mkblock(name);
		g->defined = false;
		*entry = g;
	}
	return g;
}

static int
is_flonum(struct type *t)
{
	return t->prop & PROPFLOAT;
}

static int
is_longdouble(struct type *t)
{
	t = basetype(t);
	return t->kind == TYPELDOUBLE;
}

static int
is_integer(struct type *t)
{
	return t->prop & PROPINT || t->kind == TYPEPOINTER || t->kind == TYPENULLPTR;
}

static struct type *
basetype(struct type *t)
{
	if (t->kind == TYPEENUM)
		return t->base;
	return t;
}

static void
push(struct func *f)
{
	emitf(f, "\tpush %%rax\n");
	f->depth++;
}

static void
pop(struct func *f, const char *reg)
{
	emitf(f, "\tpop %s\n", reg);
	f->depth--;
}

static void
pushf(struct func *f)
{
	emitf(f, "\tsub $8, %%rsp\n");
	emitf(f, "\tmovsd %%xmm0, (%%rsp)\n");
	f->depth++;
}

static void
popf(struct func *f, int reg)
{
	emitf(f, "\tmovsd (%%rsp), %%xmm%d\n", reg);
	emitf(f, "\tadd $8, %%rsp\n");
	f->depth--;
}

static void
pushld(struct func *f)
{
	emitf(f, "\tsub $16, %%rsp\n");
	emitf(f, "\tfstpt (%%rsp)\n");
	f->depth += 2;
}

static void
popld(struct func *f)
{
	emitf(f, "\tfldt (%%rsp)\n");
	emitf(f, "\tadd $16, %%rsp\n");
	f->depth -= 2;
}

static void
gen_addr(struct func *f, struct expr *e);
static void
gen_expr(struct func *f, struct expr *e);

static void
load(struct func *f, struct type *t)
{
	t = basetype(t);
	if (t->kind == TYPEARRAY || t->kind == TYPESTRUCT || t->kind == TYPEUNION || t->kind == TYPEFUNC)
		return;
	if (is_longdouble(t)) {
		emitf(f, "\tfldt (%%rax)\n");
		return;
	}
	if (is_flonum(t)) {
		if (t->size == 4)
			emitf(f, "\tmovss (%%rax), %%xmm0\n");
		else
			emitf(f, "\tmovsd (%%rax), %%xmm0\n");
		return;
	}
	switch (t->size) {
	case 1:
		emitf(f, "\tmov%sbl (%%rax), %%eax\n", t->u.basic.issigned ? "s" : "z");
		return;
	case 2:
		emitf(f, "\t%s (%%rax), %%eax\n", t->u.basic.issigned ? "movswl" : "movzwl");
		return;
	case 4:
		emitf(f, "\tmov (%%rax), %%eax\n");
		return;
	case 8:
		emitf(f, "\tmov (%%rax), %%rax\n");
		return;
	}
	fatal("unsupported load size");
}

static void
store(struct func *f, struct type *t)
{
	t = basetype(t);
	if (t->kind == TYPEARRAY || t->kind == TYPESTRUCT || t->kind == TYPEUNION)
		return;
	if (is_longdouble(t)) {
		emitf(f, "\tfstpt (%%rdi)\n");
		emitf(f, "\tfldt (%%rdi)\n");
		return;
	}
	if (is_flonum(t)) {
		if (t->size == 4)
			emitf(f, "\tmovss %%xmm0, (%%rdi)\n");
		else
			emitf(f, "\tmovsd %%xmm0, (%%rdi)\n");
		return;
	}
	switch (t->size) {
	case 1:
		emitf(f, "\tmov %%al, (%%rdi)\n");
		return;
	case 2:
		emitf(f, "\tmov %%ax, (%%rdi)\n");
		return;
	case 4:
		emitf(f, "\tmov %%eax, (%%rdi)\n");
		return;
	case 8:
		emitf(f, "\tmov %%rax, (%%rdi)\n");
		return;
	}
	fatal("unsupported store size");
}

static void
load_bitfield(struct func *f, struct type *t, struct bitfield bits)
{
	int sz = (int)t->size;
	int width = sz * 8 - bits.before - bits.after;
	int regbits = sz <= 4 ? 32 : 64;
	unsigned long long mask;
	int shift;

	if (sz <= 4)
		emitf(f, "\tmov (%%rax), %%eax\n");
	else
		emitf(f, "\tmov (%%rax), %%rax\n");

	if (bits.before) {
		if (sz <= 4)
			emitf(f, "\tshr $%d, %%eax\n", bits.before);
		else
			emitf(f, "\tshr $%d, %%rax\n", bits.before);
	}

	if (t->u.basic.issigned) {
		if (width < regbits) {
			shift = regbits - width;
			if (sz <= 4) {
				emitf(f, "\tshl $%d, %%eax\n", shift);
				emitf(f, "\tsar $%d, %%eax\n", shift);
			} else {
				emitf(f, "\tshl $%d, %%rax\n", shift);
				emitf(f, "\tsar $%d, %%rax\n", shift);
			}
		}
	} else if (width < regbits) {
		if (width == 64)
			mask = ~0ull;
		else
			mask = (1ull << width) - 1;
		if (sz <= 4) {
			emitf(f, "\tand $%u, %%eax\n", (unsigned)mask);
		} else {
			emitf(f, "\tmov $%llu, %%rcx\n", mask);
			emitf(f, "\tand %%rcx, %%rax\n");
		}
	}
}

static void
store_bitfield(struct func *f, struct type *t, struct bitfield bits)
{
	unsigned long long mask;
	int sz = (int)t->size;
	int width = sz * 8 - bits.before - bits.after;

	mask = ~0ull;
	mask >>= 64 - sz * 8;
	mask &= (width == 64) ? ~0ull : ((1ull << width) - 1);
	mask <<= bits.before;

	if (sz <= 4) {
		emitf(f, "\tmov %%eax, %%ecx\n");
		if (bits.before)
			emitf(f, "\tshl $%d, %%ecx\n", bits.before);
		emitf(f, "\tand $%u, %%ecx\n", (unsigned)mask);
		emitf(f, "\tmov (%%rdi), %%eax\n");
		emitf(f, "\tand $%u, %%eax\n", (unsigned)(~mask));
		emitf(f, "\tor %%ecx, %%eax\n");
		emitf(f, "\tmov %%eax, (%%rdi)\n");
	} else {
		emitf(f, "\tmov %%rax, %%rcx\n");
		if (bits.before)
			emitf(f, "\tshl $%d, %%rcx\n", bits.before);
		emitf(f, "\tmov $%llu, %%r11\n", mask);
		emitf(f, "\tand %%r11, %%rcx\n");
		emitf(f, "\tmov (%%rdi), %%rax\n");
		emitf(f, "\tmov $%llu, %%r11\n", ~mask);
		emitf(f, "\tand %%r11, %%rax\n");
		emitf(f, "\tor %%rcx, %%rax\n");
		emitf(f, "\tmov %%rax, (%%rdi)\n");
	}
}

static struct value *
vla_size_slot(struct func *f, struct type *t)
{
	struct value *v;

	if (t->u.array.size)
		return t->u.array.size;
	v = mkvalue(V_LOCAL);
	v->offset = alloc_stack(f, 8, 8);
	t->u.array.size = v;
	return v;
}

static void
eval_vla(struct func *f, struct type *t)
{
	struct value *sz;

	if (!t || !(t->prop & PROPVM))
		return;
	switch (t->kind) {
	case TYPEARRAY:
		if (t->u.array.size)
			return;
		if (t->u.array.length)
			gen_expr(f, t->u.array.length);
		else
			emitf(f, "\tmov $0, %%rax\n");
		push(f);
		eval_vla(f, t->base);
		if (t->base->kind == TYPEARRAY && t->base->size == 0)
			emitf(f, "\tmov %ld(%%rbp), %%rcx\n", t->base->u.array.size->offset);
		else
			emitf(f, "\tmov $%llu, %%rcx\n", t->base->size);
		pop(f, "%rax");
		emitf(f, "\timul %%rcx, %%rax\n");
		sz = vla_size_slot(f, t);
		emitf(f, "\tmov %%rax, %ld(%%rbp)\n", sz->offset);
		return;
	case TYPEPOINTER:
		eval_vla(f, t->base);
		return;
	case TYPEFUNC: {
		struct decl *p;
		eval_vla(f, t->base);
		for (p = t->u.func.params; p; p = p->next)
			eval_vla(f, p->type);
		return;
	}
	default:
		return;
	}
}

static void
cmp_zero(struct func *f, struct type *t)
{
	t = basetype(t);
	if (is_longdouble(t)) {
		emitf(f, "\tfldz\n");
		emitf(f, "\tfxch %%st(1)\n");
		emitf(f, "\tfucomip %%st(1), %%st\n");
		emitf(f, "\tfstp %%st(0)\n");
		return;
	}
	if (is_flonum(t)) {
		if (t->size == 4) {
			emitf(f, "\txorps %%xmm1, %%xmm1\n");
			emitf(f, "\tucomiss %%xmm1, %%xmm0\n");
		} else {
			emitf(f, "\txorpd %%xmm1, %%xmm1\n");
			emitf(f, "\tucomisd %%xmm1, %%xmm0\n");
		}
		return;
	}
	if (is_integer(t) && t->size <= 4)
		emitf(f, "\tcmp $0, %%eax\n");
	else
		emitf(f, "\tcmp $0, %%rax\n");
}

static void
tobool(struct func *f, struct type *t)
{
	t = basetype(t);
	if (is_longdouble(t)) {
		emitf(f, "\tfldz\n");
		emitf(f, "\tfxch %%st(1)\n");
		emitf(f, "\tfucomip %%st(1), %%st\n");
		emitf(f, "\tfstp %%st(0)\n");
		emitf(f, "\tsetp %%al\n\tsetne %%dl\n\tor %%dl, %%al\n\tmovzb %%al, %%eax\n");
		return;
	}
	if (is_flonum(t)) {
		if (t->size == 4) {
			emitf(f, "\txorps %%xmm1, %%xmm1\n");
			emitf(f, "\tucomiss %%xmm1, %%xmm0\n");
		} else {
			emitf(f, "\txorpd %%xmm1, %%xmm1\n");
			emitf(f, "\tucomisd %%xmm1, %%xmm0\n");
		}
		emitf(f, "\tsetp %%al\n\tsetne %%dl\n\tor %%dl, %%al\n\tmovzb %%al, %%eax\n");
		return;
	}
	cmp_zero(f, t);
	emitf(f, "\tsetne %%al\n\tmovzb %%al, %%eax\n");
}

enum {
	I8, I16, I32, I64, U8, U16, U32, U64, F32, F64
};

static int
gettypeid(struct type *t)
{
	t = basetype(t);
	if (t->kind == TYPEPOINTER || t->kind == TYPENULLPTR)
		return U64;
	switch (t->kind) {
	case TYPEBOOL:
		return U8;
	case TYPECHAR:
		return t->u.basic.issigned ? I8 : U8;
	case TYPESHORT:
		return t->u.basic.issigned ? I16 : U16;
	case TYPEINT:
		return t->u.basic.issigned ? I32 : U32;
	case TYPELONG:
	case TYPELLONG:
		return t->u.basic.issigned ? I64 : U64;
	case TYPEFLOAT:
		return F32;
	case TYPEDOUBLE:
		return F64;
	default:
		return U64;
	}
}

static const char i32i8[] = "movsbl %al, %eax";
static const char i32u8[] = "movzbl %al, %eax";
static const char i32i16[] = "movswl %ax, %eax";
static const char i32u16[] = "movzwl %ax, %eax";
static const char i32f32[] = "cvtsi2ssl %eax, %xmm0";
static const char i32i64[] = "movsxd %eax, %rax";
static const char i32f64[] = "cvtsi2sdl %eax, %xmm0";

static const char u32f32[] = "mov %eax, %eax; cvtsi2ssq %rax, %xmm0";
static const char u32i64[] = "mov %eax, %eax";
static const char u32f64[] = "mov %eax, %eax; cvtsi2sdq %rax, %xmm0";

static const char i64f32[] = "cvtsi2ssq %rax, %xmm0";
static const char i64f64[] = "cvtsi2sdq %rax, %xmm0";

static const char u64f32[] = "cvtsi2ssq %rax, %xmm0";
static const char u64f64[] =
	"test %rax,%rax; js 1f; pxor %xmm0,%xmm0; cvtsi2sd %rax,%xmm0; jmp 2f; "
	"1: mov %rax,%rdi; and $1,%eax; pxor %xmm0,%xmm0; shr %rdi; "
	"or %rax,%rdi; cvtsi2sd %rdi,%xmm0; addsd %xmm0,%xmm0; 2:";

static const char f32i8[] = "cvttss2sil %xmm0, %eax; movsbl %al, %eax";
static const char f32u8[] = "cvttss2sil %xmm0, %eax; movzbl %al, %eax";
static const char f32i16[] = "cvttss2sil %xmm0, %eax; movswl %ax, %eax";
static const char f32u16[] = "cvttss2sil %xmm0, %eax; movzwl %ax, %eax";
static const char f32i32[] = "cvttss2sil %xmm0, %eax";
static const char f32u32[] = "cvttss2siq %xmm0, %rax";
static const char f32i64[] = "cvttss2siq %xmm0, %rax";
static const char f32u64[] = "cvttss2siq %xmm0, %rax";
static const char f32f64[] = "cvtss2sd %xmm0, %xmm0";

static const char f64i8[] = "cvttsd2sil %xmm0, %eax; movsbl %al, %eax";
static const char f64u8[] = "cvttsd2sil %xmm0, %eax; movzbl %al, %eax";
static const char f64i16[] = "cvttsd2sil %xmm0, %eax; movswl %ax, %eax";
static const char f64u16[] = "cvttsd2sil %xmm0, %eax; movzwl %ax, %eax";
static const char f64i32[] = "cvttsd2sil %xmm0, %eax";
static const char f64u32[] = "cvttsd2siq %xmm0, %rax";
static const char f64i64[] = "cvttsd2siq %xmm0, %rax";
static const char f64u64[] = "cvttsd2siq %xmm0, %rax";
static const char f64f32[] = "cvtsd2ss %xmm0, %xmm0";

static const char *cast_table[][10] = {
	/* from:  i8       i16      i32      i64      u8       u16      u32      u64      f32      f64 */
	/* i8 */  {NULL,    NULL,    NULL,    i32i64,  i32u8,   i32u16,  NULL,    i32i64,  i32f32,  i32f64},
	/* i16 */ {i32i8,   NULL,    NULL,    i32i64,  i32u8,   i32u16,  NULL,    i32i64,  i32f32,  i32f64},
	/* i32 */ {i32i8,   i32i16,  NULL,    i32i64,  i32u8,   i32u16,  NULL,    i32i64,  i32f32,  i32f64},
	/* i64 */ {i32i8,   i32i16,  NULL,    NULL,    i32u8,   i32u16,  NULL,    NULL,    i64f32,  i64f64},

	/* u8 */  {i32i8,   NULL,    NULL,    i32i64,  NULL,    NULL,    NULL,    i32i64,  i32f32,  i32f64},
	/* u16 */ {i32i8,   i32i16,  NULL,    i32i64,  i32u8,   NULL,    NULL,    i32i64,  i32f32,  i32f64},
	/* u32 */ {i32i8,   i32i16,  NULL,    u32i64,  i32u8,   i32u16,  NULL,    u32i64,  u32f32,  u32f64},
	/* u64 */ {i32i8,   i32i16,  NULL,    NULL,    i32u8,   i32u16,  NULL,    NULL,    u64f32,  u64f64},

	/* f32 */ {f32i8,   f32i16,  f32i32,  f32i64,  f32u8,   f32u16,  f32u32,  f32u64,  NULL,    f32f64},
	/* f64 */ {f64i8,   f64i16,  f64i32,  f64i64,  f64u8,   f64u16,  f64u32,  f64u64,  f64f32,  NULL},
};

static void
cast(struct func *f, struct type *from, struct type *to)
{
	int t1, t2;
	struct type *bf, *bt;

	if (to->kind == TYPEVOID)
		return;
	if (to->kind == TYPEBOOL) {
		tobool(f, from);
		return;
	}
	bf = basetype(from);
	bt = basetype(to);
	if (bf->kind == TYPELDOUBLE || bt->kind == TYPELDOUBLE) {
		if (bf->kind == TYPELDOUBLE && bt->kind == TYPELDOUBLE)
			return;
		if (bt->kind == TYPELDOUBLE) {
			if (bf->prop & PROPINT || bf->kind == TYPEPOINTER || bf->kind == TYPENULLPTR) {
				if (bf->size == 1)
					emitf(f, "\tmov%sbq %%al, %%rax\n", bf->u.basic.issigned ? "s" : "z");
				else if (bf->size == 2)
					emitf(f, "\tmov%swq %%ax, %%rax\n", bf->u.basic.issigned ? "s" : "z");
				else if (bf->size == 4) {
					if (bf->u.basic.issigned)
						emitf(f, "\tmovsxd %%eax, %%rax\n");
					else
						emitf(f, "\tmov %%eax, %%eax\n");
				}
				emitf(f, "\tsub $8, %%rsp\n");
				emitf(f, "\tmov %%rax, (%%rsp)\n");
				emitf(f, "\tfildq (%%rsp)\n");
				emitf(f, "\tadd $8, %%rsp\n");
				return;
			}
			if (bf->kind == TYPEFLOAT || bf->kind == TYPEDOUBLE) {
				emitf(f, "\tsub $8, %%rsp\n");
				if (bf->size == 4) {
					emitf(f, "\tmovss %%xmm0, (%%rsp)\n");
					emitf(f, "\tflds (%%rsp)\n");
				} else {
					emitf(f, "\tmovsd %%xmm0, (%%rsp)\n");
					emitf(f, "\tfldl (%%rsp)\n");
				}
				emitf(f, "\tadd $8, %%rsp\n");
				return;
			}
		} else {
			if (bt->kind == TYPEFLOAT || bt->kind == TYPEDOUBLE) {
				emitf(f, "\tsub $8, %%rsp\n");
				if (bt->size == 4) {
					emitf(f, "\tfstps (%%rsp)\n");
					emitf(f, "\tmovss (%%rsp), %%xmm0\n");
				} else {
					emitf(f, "\tfstpl (%%rsp)\n");
					emitf(f, "\tmovsd (%%rsp), %%xmm0\n");
				}
				emitf(f, "\tadd $8, %%rsp\n");
				return;
			}
			if (bt->prop & PROPINT || bt->kind == TYPEPOINTER || bt->kind == TYPENULLPTR) {
				emitf(f, "\tsub $8, %%rsp\n");
				emitf(f, "\tfisttpq (%%rsp)\n");
				emitf(f, "\tmov (%%rsp), %%rax\n");
				emitf(f, "\tadd $8, %%rsp\n");
				if (bt->kind == TYPEPOINTER || bt->kind == TYPENULLPTR)
					return;
				switch (bt->size) {
				case 1:
					emitf(f, "\tmov%sbq %%al, %%rax\n", bt->u.basic.issigned ? "s" : "z");
					break;
				case 2:
					emitf(f, "\tmov%swq %%ax, %%rax\n", bt->u.basic.issigned ? "s" : "z");
					break;
				case 4:
					if (bt->u.basic.issigned)
						emitf(f, "\tmovsxd %%eax, %%rax\n");
					else
						emitf(f, "\tmov %%eax, %%eax\n");
					break;
				default:
					break;
				}
				return;
			}
		}
		fatal("unsupported long double cast");
	}
	t1 = gettypeid(from);
	t2 = gettypeid(to);
	if (cast_table[t1][t2])
		emitf(f, "\t%s\n", cast_table[t1][t2]);
}

static void
gen_addr(struct func *f, struct expr *e)
{
	struct decl *d;
	struct value *v;

	switch (e->kind) {
	case EXPRIDENT:
		d = e->u.ident.decl;
		v = d->value;
		if (d->kind != DECLOBJECT && d->kind != DECLFUNC)
			error(&tok.loc, "identifier '%s' is not an object or function", d->name);
		if (v->kind == V_PARAMREF) {
			emitf(f, "\tmov %ld(%%rbp), %%rax\n", v->offset);
			return;
		}
		if (v->kind == V_LOCAL || v->kind == V_TEMP) {
			emitf(f, "\tlea %ld(%%rbp), %%rax\n", v->offset);
			return;
		}
		if (v->kind == V_GLOBAL) {
			if (v->id)
				emitf(f, "\tlea .L%u.%s(%%rip), %%rax\n", v->id, v->u.name);
			else
				emitf(f, "\tlea %s(%%rip), %%rax\n", v->u.name);
			return;
		}
		break;
	case EXPRSTRING:
		d = stringdecl(e);
		v = d->value;
		if (v->kind == V_GLOBAL) {
			if (v->id)
				emitf(f, "\tlea .L%u.%s(%%rip), %%rax\n", v->id, v->u.name);
			else
				emitf(f, "\tlea %s(%%rip), %%rax\n", v->u.name);
			return;
		}
		break;
	case EXPRCOMPOUND:
		if (e->toeval)
			gen_expr(f, e->toeval);
		if (e->type->prop & PROPVM)
			eval_vla(f, e->type);
		if (e->u.compound.decl->u.obj.storage == SDSTATIC) {
			d = e->u.compound.decl;
			if (!d->value)
				d->value = mkglobal(d);
			if (d->value->kind == V_GLOBAL) {
				if (d->value->id)
					emitf(f, "\tlea .L%u.%s(%%rip), %%rax\n", d->value->id, d->value->u.name);
				else
					emitf(f, "\tlea %s(%%rip), %%rax\n", d->value->u.name);
				return;
			}
		} else {
			funcinit(f, e->u.compound.decl, e->u.compound.init, true);
			v = e->u.compound.decl->value;
			emitf(f, "\tlea %ld(%%rbp), %%rax\n", v->offset);
			return;
		}
		break;
	case EXPRUNARY:
		if (e->op == TMUL) {
			gen_expr(f, e->base);
			return;
		}
		break;
	case EXPRCALL:
		if (e->type->kind == TYPESTRUCT || e->type->kind == TYPEUNION) {
			gen_expr(f, e);
			return;
		}
		break;
	case EXPRASSIGN:
	case EXPRCOND:
		if (e->type->kind == TYPESTRUCT || e->type->kind == TYPEUNION) {
			gen_expr(f, e);
			return;
		}
		break;
	case EXPRCOMMA:
		{
			struct expr *cur = e->base;
			for (; cur->next; cur = cur->next)
				gen_expr(f, cur);
			gen_addr(f, cur);
		}
		return;
	case EXPRBITFIELD:
		gen_addr(f, e->base);
		return;
	default:
		break;
	}
	error(&tok.loc, "not an lvalue");
}

static void
funccopy(struct func *f, int size)
{
	int i;

	for (i = 0; i < size; i += 8) {
		int chunk = size - i;
		if (chunk >= 8) {
			emitf(f, "\tmov %d(%%rsi), %%rax\n", i);
			emitf(f, "\tmov %%rax, %d(%%rdi)\n", i);
			continue;
		}
		if (chunk >= 4) {
			emitf(f, "\tmov %d(%%rsi), %%eax\n", i);
			emitf(f, "\tmov %%eax, %d(%%rdi)\n", i);
			i += 3;
			continue;
		}
		if (chunk >= 2) {
			emitf(f, "\tmov %d(%%rsi), %%ax\n", i);
			emitf(f, "\tmov %%ax, %d(%%rdi)\n", i);
			i += 1;
			continue;
		}
		emitf(f, "\tmov %d(%%rsi), %%al\n", i);
		emitf(f, "\tmov %%al, %d(%%rdi)\n", i);
	}
}

static void
assign_lvalue(struct func *f, struct expr *lhs, struct expr *rhs)
{
	struct type *t;

	if (lhs->kind == EXPRTEMP) {
		if (!lhs->u.temp) {
			struct value *tv = mkvalue(V_TEMP);
			tv->offset = alloc_stack(f, 8, 8);
			lhs->u.temp = tv;
		}
		gen_expr(f, rhs);
		emitf(f, "\tmov %%rax, %ld(%%rbp)\n", lhs->u.temp->offset);
		return;
	}

	t = lhs->type;
	if (lhs->kind == EXPRBITFIELD) {
		gen_addr(f, lhs->base);
		push(f);
		gen_expr(f, rhs);
		pop(f, "%rdi");
		store_bitfield(f, t, lhs->u.bitfield.bits);
		emitf(f, "\tmov %%rdi, %%rax\n");
		load_bitfield(f, t, lhs->u.bitfield.bits);
		return;
	}

	gen_addr(f, lhs);
	push(f);
	gen_expr(f, rhs);
	pop(f, "%rdi");

	if (t->kind == TYPESTRUCT || t->kind == TYPEUNION) {
		emitf(f, "\tmov %%rax, %%rsi\n");
		funccopy(f, (int)t->size);
		emitf(f, "\tmov %%rdi, %%rax\n");
		return;
	}
	store(f, t);
}

static void
gen_arg(struct func *f, struct expr *arg)
{
	if (arg->type->kind == TYPESTRUCT || arg->type->kind == TYPEUNION) {
		struct value *tmp = mkvalue(V_TEMP);
		tmp->offset = alloc_stack(f, (int)arg->type->size, arg->type->align);
		gen_expr(f, arg);
		emitf(f, "\tlea %ld(%%rbp), %%rdi\n", tmp->offset);
		emitf(f, "\tmov %%rax, %%rsi\n");
		funccopy(f, (int)arg->type->size);
		emitf(f, "\tlea %ld(%%rbp), %%rax\n", tmp->offset);
		return;
	}
	gen_expr(f, arg);
}

static void
gen_expr(struct func *f, struct expr *e)
{
	struct decl *d;
	struct value *v;
	struct type *t;
	struct block *b1, *b2, *b3;
	struct expr *arg;
	int gp, fp;
	int nargs, i;
	bool isvararg;
	bool retbuf;
	unsigned long long imm;
	union { double f; unsigned long long u; } conv;

	switch (e->kind) {
	case EXPRIDENT:
		d = e->u.ident.decl;
		if (d->kind == DECLCONST) {
			v = d->value;
			emitf(f, "\tmov $%llu, %%rax\n", v->u.i);
			return;
		}
		gen_addr(f, e);
		load(f, e->type);
		return;
	case EXPRCONST:
		t = basetype(e->type);
		if (is_flonum(t)) {
			if (is_longdouble(t)) {
				long double ld = (long double)e->u.constant.f;
				unsigned char bytes[16];
				unsigned id = ++ldconst_id;
				memcpy(bytes, &ld, sizeof(bytes));
				emitf(f, "\t.section .rodata\n\t.align 16\n.LC%u:\n", id);
				for (i = 0; i < 16; ++i)
					emitf(f, "\t.byte %u\n", (unsigned)bytes[i]);
				emitf(f, "\t.text\n");
				emitf(f, "\tfldt .LC%u(%%rip)\n", id);
			} else if (t->size == 4) {
				union { float f; unsigned u; } c32;
				c32.f = (float)e->u.constant.f;
				emitf(f, "\tmov $%u, %%eax\n", c32.u);
				emitf(f, "\tmovd %%eax, %%xmm0\n");
			} else {
				conv.f = e->u.constant.f;
				imm = conv.u;
				emitf(f, "\tmov $%llu, %%rax\n", imm);
				emitf(f, "\tmovq %%rax, %%xmm0\n");
			}
			return;
		}
		emitf(f, "\tmov $%llu, %%rax\n", e->u.constant.u);
		return;
	case EXPRSTRING:
		gen_addr(f, e);
		return;
	case EXPRCOMPOUND:
		gen_addr(f, e);
		load(f, e->type);
		return;
	case EXPRBITFIELD:
		gen_addr(f, e->base);
		load_bitfield(f, e->type, e->u.bitfield.bits);
		return;
	case EXPRUNARY:
		switch (e->op) {
		case TBAND:
			gen_addr(f, e->base);
			return;
		case TMUL:
			gen_expr(f, e->base);
			load(f, e->type);
			return;
		case TSUB:
			gen_expr(f, e->base);
			if (is_longdouble(e->type)) {
				emitf(f, "\tfchs\n");
			} else if (is_flonum(e->type)) {
				if (e->type->size == 4) {
					emitf(f, "\txorps %%xmm1, %%xmm1\n");
					emitf(f, "\tsubss %%xmm0, %%xmm1\n");
					emitf(f, "\tmovaps %%xmm1, %%xmm0\n");
				} else {
					emitf(f, "\txorpd %%xmm1, %%xmm1\n");
					emitf(f, "\tsubsd %%xmm0, %%xmm1\n");
					emitf(f, "\tmovapd %%xmm1, %%xmm0\n");
				}
			} else {
				emitf(f, "\tneg %%rax\n");
			}
			return;
		case TLNOT:
			gen_expr(f, e->base);
			tobool(f, e->base->type);
			emitf(f, "\txor $1, %%eax\n");
			return;
		case TBNOT:
			gen_expr(f, e->base);
			emitf(f, "\tnot %%rax\n");
			return;
		default:
			fatal("unhandled unary op");
		}
		break;
	case EXPRINCDEC: {
		struct type *bt;
		unsigned long long step;

		if (e->base->kind == EXPRBITFIELD) {
			gen_addr(f, e->base->base);
		} else {
			gen_addr(f, e->base);
		}
		push(f);
		load(f, e->base->type);
		if (is_longdouble(e->base->type)) {
			if (e->u.incdec.post)
				pushld(f);
			emitf(f, "\tfld1\n");
			if (e->op == TINC)
				emitf(f, "\tfaddp %%st, %%st(1)\n");
			else
				emitf(f, "\tfsubrp %%st, %%st(1)\n");
			pop(f, "%rdi");
			store(f, e->base->type);
			if (e->u.incdec.post) {
				emitf(f, "\tfstp %%st(0)\n");
				popld(f);
			}
			return;
		}
		if (is_flonum(e->base->type)) {
			if (e->u.incdec.post)
				pushf(f);
			if (e->base->type->size == 4) {
				emitf(f, "\tmov $0x3f800000, %%eax\n");
				emitf(f, "\tmovd %%eax, %%xmm1\n");
				if (e->op == TINC)
					emitf(f, "\taddss %%xmm1, %%xmm0\n");
				else
					emitf(f, "\tsubss %%xmm1, %%xmm0\n");
			} else {
				emitf(f, "\tmov $0x3ff0000000000000, %%rax\n");
				emitf(f, "\tmovq %%rax, %%xmm1\n");
				if (e->op == TINC)
					emitf(f, "\taddsd %%xmm1, %%xmm0\n");
				else
					emitf(f, "\tsubsd %%xmm1, %%xmm0\n");
			}
			pop(f, "%rdi");
			store(f, e->base->type);
			if (e->u.incdec.post)
				popf(f, 0);
			return;
		}
		bt = e->base->type;
		if (bt->kind == TYPEPOINTER)
			step = bt->base->size;
		else
			step = 1;
		if (e->u.incdec.post)
			push(f);
		if (e->op == TINC)
			emitf(f, "\tadd $%llu, %%rax\n", step);
		else
			emitf(f, "\tsub $%llu, %%rax\n", step);
		if (e->u.incdec.post) {
			pop(f, "%rdx");
			pop(f, "%rdi");
			store(f, e->base->type);
			emitf(f, "\tmov %%rdx, %%rax\n");
		} else {
			pop(f, "%rdi");
			store(f, e->base->type);
		}
		return;
	}
	case EXPRCAST:
		if (e->toeval)
			gen_expr(f, e->toeval);
		if (e->type->prop & PROPVM)
			eval_vla(f, e->type);
		gen_expr(f, e->base);
		cast(f, e->base->type, e->type);
		return;
	case EXPRBINARY:
		if (e->op == TLOR || e->op == TLAND) {
			b1 = mkblock("logic_true");
			b2 = mkblock("logic_false");
			b3 = mkblock("logic_join");
			gen_expr(f, e->u.binary.l);
			if (e->op == TLOR)
				funcjnz(f, NULL, e->u.binary.l->type, b1, b2);
			else
				funcjnz(f, NULL, e->u.binary.l->type, b1, b2);

			funclabel(f, b1);
			if (e->op == TLOR) {
				emitf(f, "\tmov $1, %%eax\n");
			} else {
				gen_expr(f, e->u.binary.r);
				tobool(f, e->u.binary.r->type);
			}
			funcjmp(f, b3);

			funclabel(f, b2);
			if (e->op == TLOR) {
				gen_expr(f, e->u.binary.r);
				tobool(f, e->u.binary.r->type);
			} else {
				emitf(f, "\tmov $0, %%eax\n");
			}
			funcjmp(f, b3);

			funclabel(f, b3);
			return;
		}
		if (is_longdouble(e->u.binary.l->type)) {
			gen_expr(f, e->u.binary.r);
			pushld(f);
			gen_expr(f, e->u.binary.l);
			popld(f);
			if (e->op == TADD) {
				emitf(f, "\tfaddp %%st, %%st(1)\n");
				return;
			} else if (e->op == TSUB) {
				emitf(f, "\tfsubrp %%st, %%st(1)\n");
				return;
			} else if (e->op == TMUL) {
				emitf(f, "\tfmulp %%st, %%st(1)\n");
				return;
			} else if (e->op == TDIV) {
				emitf(f, "\tfdivrp %%st, %%st(1)\n");
				return;
			} else {
				emitf(f, "\tfxch %%st(1)\n");
				emitf(f, "\tfucomip %%st(1), %%st\n");
				emitf(f, "\tfstp %%st(0)\n");
				switch (e->op) {
				case TEQL:
					emitf(f, "\tsete %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				case TNEQ:
					emitf(f, "\tsetne %%al\n\tsetp %%dl\n\tor %%dl, %%al\n");
					break;
				case TLESS:
					emitf(f, "\tsetb %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				case TLEQ:
					emitf(f, "\tsetbe %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				case TGREATER:
					emitf(f, "\tseta %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				case TGEQ:
					emitf(f, "\tsetae %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				default:
					fatal("unhandled float compare");
				}
				emitf(f, "\tand $1, %%al\n\tmovzb %%al, %%eax\n");
				return;
			}
		}
		if (is_flonum(e->u.binary.l->type)) {
			gen_expr(f, e->u.binary.r);
			pushf(f);
			gen_expr(f, e->u.binary.l);
			popf(f, 1);
			if (e->op == TADD)
				emitf(f, "\tadd%s %%xmm1, %%xmm0\n", e->u.binary.l->type->size == 4 ? "ss" : "sd");
			else if (e->op == TSUB)
				emitf(f, "\tsub%s %%xmm1, %%xmm0\n", e->u.binary.l->type->size == 4 ? "ss" : "sd");
			else if (e->op == TMUL)
				emitf(f, "\tmul%s %%xmm1, %%xmm0\n", e->u.binary.l->type->size == 4 ? "ss" : "sd");
			else if (e->op == TDIV)
				emitf(f, "\tdiv%s %%xmm1, %%xmm0\n", e->u.binary.l->type->size == 4 ? "ss" : "sd");
			else {
				const char *sz = e->u.binary.l->type->size == 4 ? "ss" : "sd";
				emitf(f, "\tucomi%s %%xmm0, %%xmm1\n", sz);
				switch (e->op) {
				case TEQL:
					emitf(f, "\tsete %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				case TNEQ:
					emitf(f, "\tsetne %%al\n\tsetp %%dl\n\tor %%dl, %%al\n");
					break;
				case TLESS:
					emitf(f, "\tseta %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				case TLEQ:
					emitf(f, "\tsetae %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				case TGREATER:
					emitf(f, "\tsetb %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				case TGEQ:
					emitf(f, "\tsetbe %%al\n\tsetnp %%dl\n\tand %%dl, %%al\n");
					break;
				default:
					fatal("unhandled float compare");
				}
				emitf(f, "\tand $1, %%al\n\tmovzb %%al, %%eax\n");
			}
			return;
		}

		gen_expr(f, e->u.binary.r);
		push(f);
		gen_expr(f, e->u.binary.l);
		pop(f, "%rdi");

		t = basetype(e->u.binary.l->type);
		{
			int sz = t->size <= 4 ? 4 : 8;
			const char *ax = sz == 4 ? "%eax" : "%rax";
			const char *di = sz == 4 ? "%edi" : "%rdi";
			const char *cx = sz == 4 ? "%ecx" : "%rcx";
			bool issigned = (t->prop & PROPINT) && t->u.basic.issigned;
		switch (e->op) {
		case TADD:
			emitf(f, "\tadd %s, %s\n", di, ax);
			return;
		case TSUB:
			emitf(f, "\tsub %s, %s\n", di, ax);
			return;
		case TMUL:
			emitf(f, "\timul %s, %s\n", di, ax);
			return;
		case TDIV:
			if (issigned) {
				if (sz == 4)
					emitf(f, "\tcdq\n\tidiv %s\n", di);
				else
					emitf(f, "\tcqo\n\tidiv %s\n", di);
			} else {
				emitf(f, "\txor %%edx, %%edx\n\tdiv %s\n", di);
			}
			return;
		case TMOD:
			if (issigned) {
				if (sz == 4)
					emitf(f, "\tcdq\n\tidiv %s\n", di);
				else
					emitf(f, "\tcqo\n\tidiv %s\n", di);
			} else {
				emitf(f, "\txor %%edx, %%edx\n\tdiv %s\n", di);
			}
			emitf(f, "\tmov %s, %s\n", sz == 4 ? "%edx" : "%rdx", ax);
			return;
		case TSHL:
			emitf(f, "\tmov %s, %s\n\tshl %%cl, %s\n", di, cx, ax);
			return;
		case TSHR:
			emitf(f, "\tmov %s, %s\n", di, cx);
			if (issigned)
				emitf(f, "\tsar %%cl, %s\n", ax);
			else
				emitf(f, "\tshr %%cl, %s\n", ax);
			return;
		case TBAND:
			emitf(f, "\tand %s, %s\n", di, ax);
			return;
		case TBOR:
			emitf(f, "\tor %s, %s\n", di, ax);
			return;
		case TXOR:
			emitf(f, "\txor %s, %s\n", di, ax);
			return;
		case TEQL:
		case TNEQ:
		case TLESS:
		case TLEQ:
		case TGREATER:
		case TGEQ:
			emitf(f, "\tcmp %s, %s\n", di, ax);
			if (e->op == TEQL)
				emitf(f, "\tsete %%al\n");
			else if (e->op == TNEQ)
				emitf(f, "\tsetne %%al\n");
			else if (e->op == TLESS)
				emitf(f, "\tset%s %%al\n", issigned ? "l" : "b");
			else if (e->op == TLEQ)
				emitf(f, "\tset%s %%al\n", issigned ? "le" : "be");
			else if (e->op == TGREATER)
				emitf(f, "\tset%s %%al\n", issigned ? "g" : "a");
			else
				emitf(f, "\tset%s %%al\n", issigned ? "ge" : "ae");
			emitf(f, "\tmovzb %%al, %%eax\n");
			return;
		default:
			fatal("unhandled binary op");
		}
		}
		break;
	case EXPRCOND:
		b1 = mkblock("cond_true");
		b2 = mkblock("cond_false");
		b3 = mkblock("cond_join");
		gen_expr(f, e->base);
		funcjnz(f, NULL, e->base->type, b1, b2);
		funclabel(f, b1);
		gen_expr(f, e->u.cond.t);
		funcjmp(f, b3);
		funclabel(f, b2);
		gen_expr(f, e->u.cond.f);
		funcjmp(f, b3);
		funclabel(f, b3);
		return;
	case EXPRASSIGN:
		assign_lvalue(f, e->u.assign.l, e->u.assign.r);
		return;
	case EXPRCOMMA:
		for (arg = e->base; arg->next; arg = arg->next) {
			gen_expr(f, arg);
			if (is_longdouble(arg->type))
				emitf(f, "\tfstp %%st(0)\n");
		}
		gen_expr(f, arg);
		return;
	case EXPRCALL:
		{
			struct expr **args;
			bool *pass_stack;
			int *stack_pad;
			int nstack;
			struct value *ret = NULL;
			int fp_used = 0;
			int spill_depth = f->depth;
			long spill_off = 0;
			int stack_bytes;

			isvararg = e->base->type->base->u.func.isvararg;
			retbuf = e->type->kind == TYPESTRUCT || e->type->kind == TYPEUNION;

			if (spill_depth) {
				spill_off = alloc_stack(f, spill_depth * 8, 8);
				for (i = 0; i < spill_depth; ++i) {
					emitf(f, "\tmov %d(%%rsp), %%r11\n", i * 8);
					emitf(f, "\tmov %%r11, %ld(%%rbp)\n", spill_off + i * 8);
				}
				emitf(f, "\tadd $%d, %%rsp\n", spill_depth * 8);
				f->depth -= spill_depth;
			}

			nargs = 0;
			for (arg = e->u.call.args; arg; arg = arg->next)
				nargs++;
			args = nargs ? xmalloc(sizeof(*args) * (size_t)nargs) : NULL;
			pass_stack = nargs ? xmalloc(sizeof(*pass_stack) * (size_t)nargs) : NULL;
			stack_pad = nargs ? xmalloc(sizeof(*stack_pad) * (size_t)nargs) : NULL;
			for (arg = e->u.call.args, i = 0; arg; arg = arg->next, ++i)
				args[i] = arg;

			if (retbuf) {
				int size = (int)e->type->size;
				int align = e->type->align;
				ret = mkvalue(V_TEMP);
				ret->offset = alloc_stack(f, size, align);
			}

			gp = retbuf ? 1 : 0;
			fp = 0;
			for (i = 0; i < nargs; ++i) {
				stack_pad[i] = 0;
				if (is_longdouble(args[i]->type)) {
					pass_stack[i] = true;
				} else if (is_flonum(args[i]->type)) {
					if (fp < FP_MAX) {
						pass_stack[i] = false;
						fp++;
					} else {
						pass_stack[i] = true;
					}
				} else {
					if (gp < GP_MAX) {
						pass_stack[i] = false;
						gp++;
					} else {
						pass_stack[i] = true;
					}
				}
			}
			fp_used = fp;
			stack_bytes = 0;
			for (i = nargs - 1; i >= 0; --i) {
				if (!pass_stack[i])
					continue;
				int align = is_longdouble(args[i]->type) ? 16 : 8;
				int size = is_longdouble(args[i]->type) ? 16 : 8;
				int pad = (align - (stack_bytes % align)) % align;
				stack_pad[i] = pad;
				stack_bytes += pad + size;
			}
			nstack = stack_bytes / 8;

			if ((f->depth + nstack) % 2) {
				emitf(f, "\tsub $8, %%rsp\n");
				f->depth++;
				nstack++;
			}

			/* push stack args (right-to-left) */
			for (i = nargs - 1; i >= 0; --i) {
				if (!pass_stack[i])
					continue;
				if (stack_pad[i]) {
					emitf(f, "\tsub $%d, %%rsp\n", stack_pad[i]);
					f->depth += stack_pad[i] / 8;
				}
				gen_arg(f, args[i]);
				if (is_longdouble(args[i]->type))
					pushld(f);
				else if (is_flonum(args[i]->type))
					pushf(f);
				else
					push(f);
			}

			/* push register args (right-to-left) */
			for (i = nargs - 1; i >= 0; --i) {
				if (pass_stack[i])
					continue;
				gen_arg(f, args[i]);
				if (is_flonum(args[i]->type))
					pushf(f);
				else
					push(f);
			}

			/* compute callee address before loading argument registers */
			bool direct = e->base->kind == EXPRIDENT && e->base->u.ident.decl->kind == DECLFUNC;
			if (!direct) {
				gen_expr(f, e->base);
				emitf(f, "\tmov %%rax, %%r10\n");
			}

			/* pop register args in left-to-right order */
			gp = retbuf ? 1 : 0;
			fp = 0;
			for (i = 0; i < nargs; ++i) {
				if (pass_stack[i])
					continue;
				if (is_flonum(args[i]->type))
					popf(f, fp++);
				else
					pop(f, argreg64[gp++]);
			}
			if (retbuf)
				emitf(f, "\tlea %ld(%%rbp), %%rdi\n", ret->offset);
			if (isvararg)
				emitf(f, "\tmov $%d, %%eax\n", fp_used);

			if (direct) {
				struct value *fv = e->base->u.ident.decl->value;
				if (fv->id)
					emitf(f, "\tcall .L%u.%s\n", fv->id, fv->u.name);
				else
					emitf(f, "\tcall %s\n", fv->u.name);
			} else {
				emitf(f, "\tcall *%%r10\n");
			}

			if (nstack) {
				emitf(f, "\tadd $%d, %%rsp\n", nstack * 8);
				f->depth -= nstack;
			}
			if (spill_depth) {
				emitf(f, "\tsub $%d, %%rsp\n", spill_depth * 8);
				for (i = 0; i < spill_depth; ++i) {
					emitf(f, "\tmov %ld(%%rbp), %%r11\n", spill_off + i * 8);
					emitf(f, "\tmov %%r11, %d(%%rsp)\n", i * 8);
				}
				f->depth += spill_depth;
			}
			if (retbuf)
				emitf(f, "\tlea %ld(%%rbp), %%rax\n", ret->offset);
			return;
		}
	case EXPRBUILTIN:
		switch (e->u.builtin.kind) {
		case BUILTINVASTART:
			gen_expr(f, e->base);
			if (!f->is_vararg)
				fatal("va_start in non-variadic function");
			emitf(f, "\tmovl $%d, (%%rax)\n", f->vararg_gp_offset);
			emitf(f, "\tmovl $%d, 4(%%rax)\n", f->vararg_fp_offset);
			emitf(f, "\tlea %ld(%%rbp), %%rdx\n", f->vararg_overflow);
			emitf(f, "\tmov %%rdx, 8(%%rax)\n");
			emitf(f, "\tlea %ld(%%rbp), %%rdx\n", f->vararg_regsave);
			emitf(f, "\tmov %%rdx, 16(%%rax)\n");
			return;
		case BUILTINVAARG: {
			struct type *at = e->type;
			int sz = (int)at->size;
			int asz = align_to(sz, 8);
			bool issigned = (at->prop & PROPINT) && at->u.basic.issigned;
			unsigned label = ++f->next_label;
			const char *fname = f->name;
			if (e->toeval)
				gen_expr(f, e->toeval);
			if (at->prop & PROPVM)
				eval_vla(f, at);
			gen_expr(f, e->base);
			emitf(f, "\tmov %%rax, %%r11\n");
			if (is_longdouble(at)) {
				emitf(f, "\tmov 8(%%r11), %%rdx\n");
				emitf(f, "\tadd $15, %%rdx\n");
				emitf(f, "\tand $-16, %%rdx\n");
				emitf(f, "\tfldt (%%rdx)\n");
				emitf(f, "\tadd $16, %%rdx\n");
				emitf(f, "\tmov %%rdx, 8(%%r11)\n");
				return;
			} else if (is_flonum(at)) {
				emitf(f, "\tmov 4(%%r11), %%edx\n");
				emitf(f, "\tcmp $%d, %%edx\n", 48 + 16 * FP_MAX);
				emitf(f, "\tjae .Lvaarg_%s_overflow%u\n", fname, label);
				emitf(f, "\tmov 16(%%r11), %%r10\n");
				if (at->size == 4)
					emitf(f, "\tmovss (%%r10,%%rdx), %%xmm0\n");
				else
					emitf(f, "\tmovsd (%%r10,%%rdx), %%xmm0\n");
				emitf(f, "\tadd $16, %%edx\n");
				emitf(f, "\tmov %%edx, 4(%%r11)\n");
				emitf(f, "\tjmp .Lvaarg_%s_done%u\n", fname, label);
			} else {
				emitf(f, "\tmov (%%r11), %%edx\n");
				emitf(f, "\tcmp $%d, %%edx\n", 8 * GP_MAX);
				emitf(f, "\tjae .Lvaarg_%s_overflow%u\n", fname, label);
				emitf(f, "\tmov 16(%%r11), %%r10\n");
				switch (sz) {
				case 1: emitf(f, "\tmov%sbl (%%r10,%%rdx), %%eax\n", issigned ? "s" : "z"); break;
				case 2: emitf(f, "\t%s (%%r10,%%rdx), %%eax\n", issigned ? "movswl" : "movzwl"); break;
				case 4: emitf(f, "\tmov (%%r10,%%rdx), %%eax\n"); break;
				case 8: emitf(f, "\tmov (%%r10,%%rdx), %%rax\n"); break;
				default: fatal("va_arg size");
				}
				emitf(f, "\tadd $8, %%edx\n");
				emitf(f, "\tmov %%edx, (%%r11)\n");
				emitf(f, "\tjmp .Lvaarg_%s_done%u\n", fname, label);
			}
			emitf(f, ".Lvaarg_%s_overflow%u:\n", fname, label);
			emitf(f, "\tmov 8(%%r11), %%rdx\n");
			if (is_flonum(at)) {
				if (at->size == 4)
					emitf(f, "\tmovss (%%rdx), %%xmm0\n");
				else
					emitf(f, "\tmovsd (%%rdx), %%xmm0\n");
			} else {
				switch (sz) {
				case 1: emitf(f, "\tmov%sbl (%%rdx), %%eax\n", issigned ? "s" : "z"); break;
				case 2: emitf(f, "\t%s (%%rdx), %%eax\n", issigned ? "movswl" : "movzwl"); break;
				case 4: emitf(f, "\tmov (%%rdx), %%eax\n"); break;
				case 8: emitf(f, "\tmov (%%rdx), %%rax\n"); break;
				default: fatal("va_arg size");
				}
			}
			emitf(f, "\tadd $%d, %%rdx\n", asz);
			emitf(f, "\tmov %%rdx, 8(%%r11)\n");
			emitf(f, ".Lvaarg_%s_done%u:\n", fname, label);
			return;
		}
		case BUILTINALLOCA:
			gen_expr(f, e->base);
			emitf(f, "\tadd $15, %%rax\n");
			emitf(f, "\tand $-16, %%rax\n");
			emitf(f, "\tsub %%rax, %%rsp\n");
			emitf(f, "\tmov %%rsp, %%rax\n");
			return;
		case BUILTINUNREACHABLE:
			emitf(f, "\tud2\n");
			return;
		default:
			fatal("unhandled builtin");
		}
	case EXPRTEMP:
		v = e->u.temp;
		if (!v)
			fatal("temp not initialized");
		emitf(f, "\tmov %ld(%%rbp), %%rax\n", v->offset);
		return;
	case EXPRSIZEOF:
		if (e->base)
			gen_expr(f, e->base);
		t = e->u.szof.type;
		if (t->kind == TYPEARRAY && t->size == 0) {
			eval_vla(f, t);
			emitf(f, "\tmov %ld(%%rbp), %%rax\n", t->u.array.size->offset);
		} else {
			emitf(f, "\tmov $%llu, %%rax\n", t->size);
		}
		return;
	default:
		fatal("unimplemented expression");
	}
}

struct value *
funcexpr(struct func *f, struct expr *e)
{
	static struct value regval = {.kind = V_NONE};

	gen_expr(f, e);
	return &regval;
}

void
funcdiscard(struct func *f, struct type *t)
{
	if (is_longdouble(t))
		emitf(f, "\tfstp %%st(0)\n");
}

static void
print_sym(struct value *v)
{
	if (v->id)
		printf(".L%u.%s", v->id, v->u.name);
	else
		printf("%s", v->u.name);
}

__attribute__((format(printf, 2, 3)))
static void
emit_data_value(unsigned long long size, const char *fmt, ...)
{
	va_list ap;

	if (size == 1)
		printf(".byte ");
	else if (size == 2)
		printf(".short ");
	else if (size == 4)
		printf(".long ");
	else
		printf(".quad ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

static void
emit_data_item(struct expr *expr, unsigned long long size)
{
	struct decl *decl;
	unsigned long long v;
	union { double f; unsigned long long u; } conv;

	switch (expr->kind) {
	case EXPRUNARY:
		if (expr->op != TBAND)
			fatal("not address expr");
		expr = expr->base;
		if (expr->kind != EXPRIDENT)
			error(&tok.loc, "initializer is not a constant expression");
		decl = expr->u.ident.decl;
		if (!decl->value)
			decl->value = mkglobal(decl);
		emit_data_value(size, "%s", "");
		print_sym(decl->value);
		break;
	case EXPRBINARY:
		if ((expr->op != TADD && expr->op != TSUB) || expr->u.binary.l->kind != EXPRUNARY || expr->u.binary.r->kind != EXPRCONST)
			error(&tok.loc, "initializer is not a constant expression");
		emit_data_item(expr->u.binary.l, size);
		printf(" %c %llu", expr->op == TADD ? '+' : '-', expr->u.binary.r->u.constant.u);
		break;
	case EXPRCONST:
		if (expr->type->kind == TYPELDOUBLE) {
			long double ld = (long double)expr->u.constant.f;
			unsigned char bytes[16];
			memcpy(bytes, &ld, sizeof(bytes));
			for (size_t i = 0; i < 16; ++i)
				printf(".byte %u\n", (unsigned)bytes[i]);
			break;
		}
		if (expr->type->prop & PROPFLOAT) {
			if (expr->type->size == 4) {
				union { float f; unsigned u; } c32;
				c32.f = (float)expr->u.constant.f;
				v = c32.u;
			} else {
				conv.f = expr->u.constant.f;
				v = conv.u;
			}
		} else {
			v = expr->u.constant.u;
		}
		if (size == 1)
			v &= 0xff;
		else if (size == 2)
			v &= 0xffff;
		else if (size == 4)
			v &= 0xffffffffu;
		emit_data_value(size, "%llu", v);
		break;
	case EXPRSTRING:
		if (expr->type->base->size == 1) {
			size_t n = 0;
			printf(".ascii \"");
			for (size_t i = 0; i < expr->u.string.size && i < size; ++i) {
				unsigned char c = ((unsigned char *)expr->u.string.data)[i];
				if (isprint(c) && c != '"' && c != '\\')
					putchar(c);
				else
					printf("\\%03o", c);
				n++;
			}
			printf("\"");
			if (size > n)
				printf("\n.zero %llu", (unsigned long long)(size - n));
		} else {
			/* wide strings: emit as bytes */
			size_t w = expr->type->base->size;
			size_t max = size / w;
			size_t n = 0;
			for (size_t i = 0; i < expr->u.string.size && i < max; ++i) {
				unsigned long long c = 0;
				switch (w) {
				case 2: c = ((uint_least16_t *)expr->u.string.data)[i]; break;
				case 4: c = ((uint_least32_t *)expr->u.string.data)[i]; break;
				default: break;
				}
				if (w == 2)
					printf(".short %llu\n", c);
				else
					printf(".long %llu\n", c);
				n++;
			}
			if (size > n * w)
				printf(".zero %llu\n", (unsigned long long)(size - n * w));
		}
		break;
	default:
		error(&tok.loc, "initializer is not a constant expression");
	}
}

void
emitdata(struct decl *d, struct init *init)
{
	struct init *cur;
	unsigned long long offset = 0, start, end, bits = 0;
	int align;

	for (cur = init; cur; cur = cur->next)
		cur->expr = eval(cur->expr);

	align = d->u.obj.align;
	if (d->linkage == LINKEXTERN) {
		printf(".globl ");
		print_sym(d->value);
		printf("\n");
	} else if (!d->value->id) {
		printf(".local ");
		print_sym(d->value);
		printf("\n");
	}

	if (!init) {
		printf(".bss\n");
		printf(".align %d\n", align);
		print_sym(d->value);
		printf(":\n");
		printf(".zero %llu\n", d->type->size);
		return;
	}

	printf(".data\n");
	printf(".align %d\n", align);
	print_sym(d->value);
	printf(":\n");

	while (init) {
		cur = init;
		while ((init = init->next) && init->start * 8 + init->bits.before < cur->end * 8 - cur->bits.after) {
			/* merge overlapping initializers for unions */
			if (cur->expr->kind != EXPRSTRING || init->expr->kind != EXPRCONST)
				break;
			unsigned i = (init->start - cur->start) / cur->expr->type->base->size;
			switch (cur->expr->type->base->size) {
			case 1: ((unsigned char *)cur->expr->u.string.data)[i] = init->expr->u.constant.u; break;
			case 2: ((uint_least16_t *)cur->expr->u.string.data)[i] = init->expr->u.constant.u; break;
			case 4: ((uint_least32_t *)cur->expr->u.string.data)[i] = init->expr->u.constant.u; break;
			}
		}
		start = cur->start + cur->bits.before / 8;
		end = cur->end - (cur->bits.after + 7) / 8;
		if (offset < start && bits) {
			printf(".byte %u\n", (unsigned)bits);
			++offset;
			bits = 0;
		}
		if (offset < start)
			printf(".zero %llu\n", start - offset);
		if (cur->bits.before || cur->bits.after) {
			bits |= cur->expr->u.constant.u << cur->bits.before % 8;
			for (offset = start; offset < end; ++offset, bits >>= 8)
				printf(".byte %u\n", (unsigned)bits & 0xff);
			bits &= 0x7f >> (cur->bits.after + 7) % 8;
		} else {
			emit_data_item(cur->expr, cur->end - cur->start);
			printf("\n");
			offset = end;
		}
	}
	if (bits) {
		printf(".byte %u\n", (unsigned)bits);
		++offset;
	}
	if (offset < d->type->size)
		printf(".zero %llu\n", d->type->size - offset);
}

void
funcswitch(struct func *f, struct value *v, struct switchcases *c, struct block *defaultlabel)
{
	(void)v;
	struct switchcase *sc;
	int sz = (int)c->type->size;

	if (!c->root) {
		funcjmp(f, defaultlabel);
		return;
	}

	/* simple linear search over all cases */
	sc = c->root;
	{
		void *stack[64];
		int sp = 0;
		while (sc || sp) {
			while (sc) {
				stack[sp++] = sc;
				sc = sc->node.child[0];
			}
			sc = stack[--sp];
			if (sz <= 4) {
				emitf(f, "\tcmp $%u, %%eax\n", (unsigned)sc->node.key);
			} else {
				emitf(f, "\tmov $%llu, %%rcx\n", sc->node.key);
				emitf(f, "\tcmp %%rcx, %%rax\n");
			}
			emitf(f, "\tje %s\n", sc->body->label);
			sc = sc->node.child[1];
		}
	}
	funcjmp(f, defaultlabel);
}

void
funcinit(struct func *f, struct decl *d, struct init *init, bool hasinit)
{
	int size;
	int align;

	if (d->value)
		return;
	if (d->type->prop & PROPVM)
		eval_vla(f, d->type);
	if (d->type->kind == TYPEARRAY && d->type->size == 0 || d->u.obj.align > 16) {
		struct value *pv;
		int a;
		long dynsz = 0;

		pv = mkvalue(V_PARAMREF);
		pv->offset = alloc_stack(f, 8, 8);
		d->value = pv;

		if (d->type->kind == TYPEARRAY && d->type->size == 0) {
			if (!d->type->u.array.size)
				fatal("missing VLA size");
			emitf(f, "\tmov %ld(%%rbp), %%rax\n", d->type->u.array.size->offset);
			a = d->u.obj.align;
			if (a < d->type->align)
				a = d->type->align;
		} else {
			dynsz = (long)d->type->size;
			emitf(f, "\tmov $%ld, %%rax\n", dynsz);
			a = d->u.obj.align;
		}
		if (a < 16)
			a = 16;
		emitf(f, "\tadd $%d, %%rax\n", a - 1);
		emitf(f, "\tadd $15, %%rax\n");
		emitf(f, "\tand $-16, %%rax\n");
		emitf(f, "\tsub %%rax, %%rsp\n");
		emitf(f, "\tmov %%rsp, %%rax\n");
		emitf(f, "\tadd $%d, %%rax\n", a - 1);
		emitf(f, "\tand $-%d, %%rax\n", a);
		emitf(f, "\tmov %%rax, %ld(%%rbp)\n", pv->offset);
		if (!hasinit || d->type->kind == TYPEARRAY && d->type->size == 0)
			return;
		/* memset to zero */
		emitf(f, "\tmov %ld(%%rbp), %%rdi\n", pv->offset);
		emitf(f, "\tmov $0, %%esi\n");
		emitf(f, "\tmov $%ld, %%edx\n", dynsz);
		emitf(f, "\tcall memset\n");

		for (; init; init = init->next) {
			struct expr *expr = init->expr;
			if (expr->kind == EXPRSTRING) {
				unsigned long long i;
				unsigned long long w = expr->type->base->size;
				for (i = 0; i < expr->u.string.size && i * w < init->end - init->start; ++i) {
					emitf(f, "\tmov %ld(%%rbp), %%rdi\n", pv->offset);
					if (init->start + i * w)
						emitf(f, "\tadd $%llu, %%rdi\n", init->start + i * w);
					switch (w) {
					case 1: emitf(f, "\tmov $%u, %%al\n", ((unsigned char *)expr->u.string.data)[i]); emitf(f, "\tmov %%al, (%%rdi)\n"); break;
					case 2: emitf(f, "\tmov $%u, %%ax\n", ((uint_least16_t *)expr->u.string.data)[i]); emitf(f, "\tmov %%ax, (%%rdi)\n"); break;
					case 4: emitf(f, "\tmov $%u, %%eax\n", ((uint_least32_t *)expr->u.string.data)[i]); emitf(f, "\tmov %%eax, (%%rdi)\n"); break;
					}
				}
				continue;
			}
		gen_expr(f, expr);
		emitf(f, "\tmov %ld(%%rbp), %%rdi\n", pv->offset);
		if (init->start)
			emitf(f, "\tadd $%llu, %%rdi\n", init->start);
		if (init->bits.before || init->bits.after) {
			store_bitfield(f, expr->type, init->bits);
		} else if (expr->type->kind == TYPESTRUCT || expr->type->kind == TYPEUNION) {
			emitf(f, "\tmov %%rax, %%rsi\n");
			funccopy(f, (int)expr->type->size);
		} else {
			store(f, expr->type);
		}
		}
		return;
	}
	align = d->u.obj.align;
	size = (int)d->type->size;
	f->stack_size = align_to((int)f->stack_size + size, align);
	d->value = mkvalue(V_LOCAL);
	d->value->offset = -f->stack_size;

	if (!hasinit)
		return;
	/* memset to zero */
	emitf(f, "\tlea %ld(%%rbp), %%rdi\n", d->value->offset);
	emitf(f, "\tmov $0, %%esi\n");
	emitf(f, "\tmov $%d, %%edx\n", size);
	emitf(f, "\tcall memset\n");

	for (; init; init = init->next) {
		struct expr *expr = init->expr;
		if (expr->kind == EXPRSTRING) {
			unsigned long long i;
			unsigned long long w = expr->type->base->size;
			for (i = 0; i < expr->u.string.size && i * w < init->end - init->start; ++i) {
				emitf(f, "\tlea %ld(%%rbp), %%rdi\n", d->value->offset + init->start + i * w);
				switch (w) {
				case 1: emitf(f, "\tmov $%u, %%al\n", ((unsigned char *)expr->u.string.data)[i]); emitf(f, "\tmov %%al, (%%rdi)\n"); break;
				case 2: emitf(f, "\tmov $%u, %%ax\n", ((uint_least16_t *)expr->u.string.data)[i]); emitf(f, "\tmov %%ax, (%%rdi)\n"); break;
				case 4: emitf(f, "\tmov $%u, %%eax\n", ((uint_least32_t *)expr->u.string.data)[i]); emitf(f, "\tmov %%eax, (%%rdi)\n"); break;
				}
			}
			continue;
		}
		gen_expr(f, expr);
		emitf(f, "\tlea %ld(%%rbp), %%rdi\n", d->value->offset + init->start);
		if (init->bits.before || init->bits.after) {
			store_bitfield(f, expr->type, init->bits);
		} else if (expr->type->kind == TYPESTRUCT || expr->type->kind == TYPEUNION) {
			emitf(f, "\tmov %%rax, %%rsi\n");
			funccopy(f, (int)expr->type->size);
		} else {
			store(f, expr->type);
			if (is_longdouble(expr->type))
				emitf(f, "\tfstp %%st(0)\n");
		}
	}
}

void
emitfunc(struct func *f, bool global)
{
	struct block *b;
	struct decl *p;
	int gp, fp;
	int stack_offset;

	f->stack_size = align_to((int)f->stack_size, 16);

	printf(".text\n");
	if (global)
		printf(".globl %s\n", f->decl->value->u.name);
	printf("%s:\n", f->decl->value->u.name);
	printf("\tpush %%rbp\n\tmov %%rsp, %%rbp\n");
	if (f->stack_size)
		printf("\tsub $%ld, %%rsp\n", f->stack_size);

	gp = 0;
	fp = 0;
	stack_offset = 16;
	if (f->is_vararg) {
		long off = f->vararg_regsave;

		printf("\tmov %%rdi, %ld(%%rbp)\n", off + 0);
		printf("\tmov %%rsi, %ld(%%rbp)\n", off + 8);
		printf("\tmov %%rdx, %ld(%%rbp)\n", off + 16);
		printf("\tmov %%rcx, %ld(%%rbp)\n", off + 24);
		printf("\tmov %%r8, %ld(%%rbp)\n", off + 32);
		printf("\tmov %%r9, %ld(%%rbp)\n", off + 40);
		printf("\tmovdqu %%xmm0, %ld(%%rbp)\n", off + 48);
		printf("\tmovdqu %%xmm1, %ld(%%rbp)\n", off + 64);
		printf("\tmovdqu %%xmm2, %ld(%%rbp)\n", off + 80);
		printf("\tmovdqu %%xmm3, %ld(%%rbp)\n", off + 96);
		printf("\tmovdqu %%xmm4, %ld(%%rbp)\n", off + 112);
		printf("\tmovdqu %%xmm5, %ld(%%rbp)\n", off + 128);
		printf("\tmovdqu %%xmm6, %ld(%%rbp)\n", off + 144);
		printf("\tmovdqu %%xmm7, %ld(%%rbp)\n", off + 160);
	}

	if (f->type->base->kind == TYPESTRUCT || f->type->base->kind == TYPEUNION) {
		printf("\tmov %%rdi, %ld(%%rbp)\n", f->retbuf_offset);
		gp = 1;
	}
	for (p = f->type->u.func.params; p; p = p->next) {
		struct value *v = p->value;
		bool is_ref = v && v->kind == V_PARAMREF;
		int psz = is_ref ? 8 : (int)p->type->size;

		if (!is_ref && is_longdouble(p->type)) {
			if (stack_offset % 16)
				stack_offset += 16 - (stack_offset % 16);
			printf("\tlea %d(%%rbp), %%rsi\n", stack_offset);
			printf("\tlea %ld(%%rbp), %%rdi\n", v->offset);
			printf("\tmov 0(%%rsi), %%rax\n");
			printf("\tmov %%rax, 0(%%rdi)\n");
			printf("\tmov 8(%%rsi), %%rax\n");
			printf("\tmov %%rax, 8(%%rdi)\n");
			stack_offset += 16;
			continue;
		}
		if (!is_ref && is_flonum(p->type) && fp < FP_MAX) {
			if (p->type->size == 4)
				printf("\tmovss %%xmm%d, %ld(%%rbp)\n", fp, v->offset);
			else
				printf("\tmovsd %%xmm%d, %ld(%%rbp)\n", fp, v->offset);
			fp++;
			continue;
		}
		if (gp < GP_MAX) {
			switch (psz) {
			case 1: printf("\tmov %s, %ld(%%rbp)\n", argreg8[gp], v->offset); break;
			case 2: printf("\tmov %s, %ld(%%rbp)\n", argreg16[gp], v->offset); break;
			case 4: printf("\tmov %s, %ld(%%rbp)\n", argreg32[gp], v->offset); break;
			default: printf("\tmov %s, %ld(%%rbp)\n", argreg64[gp], v->offset); break;
			}
			gp++;
			continue;
		}
		if (!is_ref && is_flonum(p->type)) {
			if (p->type->size == 4) {
				printf("\tmovss %d(%%rbp), %%xmm0\n", stack_offset);
				printf("\tmovss %%xmm0, %ld(%%rbp)\n", v->offset);
			} else {
				printf("\tmovsd %d(%%rbp), %%xmm0\n", stack_offset);
				printf("\tmovsd %%xmm0, %ld(%%rbp)\n", v->offset);
			}
		} else {
			switch (psz) {
			case 1:
				printf("\tmov %d(%%rbp), %%al\n", stack_offset);
				printf("\tmov %%al, %ld(%%rbp)\n", v->offset);
				break;
			case 2:
				printf("\tmov %d(%%rbp), %%ax\n", stack_offset);
				printf("\tmov %%ax, %ld(%%rbp)\n", v->offset);
				break;
			case 4:
				printf("\tmov %d(%%rbp), %%eax\n", stack_offset);
				printf("\tmov %%eax, %ld(%%rbp)\n", v->offset);
				break;
			default:
				printf("\tmov %d(%%rbp), %%rax\n", stack_offset);
				printf("\tmov %%rax, %ld(%%rbp)\n", v->offset);
				break;
			}
		}
		stack_offset += 8;
	}

	for (b = f->start; b; b = b->next) {
		printf("%s:\n", b->label);
		if (b->code.len)
			fwrite(b->code.buf, 1, b->code.len, stdout);
	}

	/* implicit return for fallthrough */
	if (f->type->base->kind == TYPESTRUCT || f->type->base->kind == TYPEUNION) {
		printf("\tmov %ld(%%rbp), %%rax\n", f->retbuf_offset);
	} else if (is_longdouble(f->type->base)) {
		printf("\tfldz\n");
	} else if (is_flonum(f->type->base)) {
		if (f->type->base->size == 4)
			printf("\txorps %%xmm0, %%xmm0\n");
		else
			printf("\txorpd %%xmm0, %%xmm0\n");
	} else if (f->type->base->kind != TYPEVOID) {
		printf("\tmov $0, %%eax\n");
	}
	printf("\tleave\n\tret\n");
}

void
funcswitch(struct func *f, struct value *v, struct switchcases *c, struct block *defaultlabel);
