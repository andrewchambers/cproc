#include <stdbool.h>
#include <stddef.h>
#include "util.h"
#include "cc.h"

struct value {
	int dummy;
};

struct block {
	int dummy;
};

struct func {
	int dummy;
};

static void
unimp(void)
{
	fatal("arm64 backend not implemented");
}

void
switchcase(struct switchcases *cases, unsigned long long i, struct block *b)
{
	(void)cases;
	(void)i;
	(void)b;
	unimp();
}

struct block *
mkblock(char *name)
{
	(void)name;
	unimp();
	return NULL;
}

struct value *
mkglobal(struct decl *d)
{
	(void)d;
	unimp();
	return NULL;
}

struct value *
mkintconst(unsigned long long n)
{
	(void)n;
	unimp();
	return NULL;
}

struct func *
mkfunc(struct decl *decl, char *name, struct type *t, struct scope *s)
{
	(void)decl;
	(void)name;
	(void)t;
	(void)s;
	unimp();
	return NULL;
}

void
delfunc(struct func *f)
{
	(void)f;
	unimp();
}

struct type *
functype(struct func *f)
{
	(void)f;
	unimp();
	return NULL;
}

void
funclabel(struct func *f, struct block *b)
{
	(void)f;
	(void)b;
	unimp();
}

struct value *
funcexpr(struct func *f, struct expr *e)
{
	(void)f;
	(void)e;
	unimp();
	return NULL;
}

void
funcdiscard(struct func *f, struct type *t)
{
	(void)f;
	(void)t;
	unimp();
}

void
funcjmp(struct func *f, struct block *l)
{
	(void)f;
	(void)l;
	unimp();
}

void
funcjnz(struct func *f, struct value *v, struct type *t, struct block *l1, struct block *l2)
{
	(void)f;
	(void)v;
	(void)t;
	(void)l1;
	(void)l2;
	unimp();
}

void
funcret(struct func *f, struct value *v)
{
	(void)f;
	(void)v;
	unimp();
}

void
funchlt(struct func *f)
{
	(void)f;
	unimp();
}

struct gotolabel *
funcgoto(struct func *f, char *name)
{
	(void)f;
	(void)name;
	unimp();
	return NULL;
}

void
funcswitch(struct func *f, struct value *v, struct switchcases *c, struct block *defaultlabel)
{
	(void)f;
	(void)v;
	(void)c;
	(void)defaultlabel;
	unimp();
}

void
funcinit(struct func *f, struct decl *d, struct init *init, bool hasinit)
{
	(void)f;
	(void)d;
	(void)init;
	(void)hasinit;
	unimp();
}

void
funcasm(struct func *f, bool is_volatile, const char *templ,
    struct asm_operand *outs, size_t nout,
    struct asm_operand *ins, size_t nin,
    char **clobbers, size_t nclobbers)
{
	(void)f;
	(void)is_volatile;
	(void)templ;
	(void)outs;
	(void)nout;
	(void)ins;
	(void)nin;
	(void)clobbers;
	(void)nclobbers;
	unimp();
}

void
emitfunc(struct func *f, bool global)
{
	(void)f;
	(void)global;
	unimp();
}

void
emitdata(struct decl *d, struct init *init)
{
	(void)d;
	(void)init;
	unimp();
}

void
emitalias(struct decl *d, const char *target, bool weak)
{
	(void)d;
	(void)target;
	(void)weak;
	unimp();
}

void
emitweak(struct decl *d)
{
	(void)d;
	unimp();
}
