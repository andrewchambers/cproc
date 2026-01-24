#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include "util.h"
#include "cc.h"

enum {
	F = 1<<8,
	S = 2<<8
};

static unsigned long long
consttruth(struct expr *e)
{
	if (e->type->prop & PROPCOMPLEX)
		return e->u.constant.c.re != 0 || e->u.constant.c.im != 0;
	if (e->type->prop & PROPFLOAT)
		return e->u.constant.f != 0;
	return e->u.constant.u != 0;
}

static void
cast(struct expr *expr)
{
	unsigned size;
	unsigned long long m;
	struct type *base;

	size = expr->type->size;
	if (expr->type->prop & PROPCOMPLEX) {
		base = expr->type->base;
		if (base->size == 4) {
			expr->u.constant.c.re = (float)expr->u.constant.c.re;
			expr->u.constant.c.im = (float)expr->u.constant.c.im;
		} else if (base->size == 8) {
			expr->u.constant.c.re = (double)expr->u.constant.c.re;
			expr->u.constant.c.im = (double)expr->u.constant.c.im;
		}
		return;
	}
	if (expr->type->prop & PROPFLOAT) {
		if (size == 4)
			expr->u.constant.f = (float)expr->u.constant.f;
		else if (size == 8)
			expr->u.constant.f = (double)expr->u.constant.f;
	} else if (expr->type->prop & PROPINT) {
		expr->u.constant.u &= -1ull >> CHAR_BIT * sizeof(unsigned long long) - size * 8;
		if (expr->type->u.basic.issigned) {
			m = 1ull << size * 8 - 1;
			expr->u.constant.u = (expr->u.constant.u ^ m) - m;
		}
	}
}

static void
unary(struct expr *expr, enum tokenkind op, struct expr *l)
{
	expr->kind = EXPRCONST;
	if (l->type->prop & PROPCOMPLEX) {
		switch (op) {
		case TSUB:
			expr->u.constant.c.re = -l->u.constant.c.re;
			expr->u.constant.c.im = -l->u.constant.c.im;
			break;
		default:
			fatal("internal error; unknown unary expression");
		}
		cast(expr);
		return;
	}
	if (l->type->prop & PROPFLOAT)
		op |= F;
	switch (op) {
	case TSUB:      expr->u.constant.u = -l->u.constant.u; break;
	case TSUB|F:    expr->u.constant.f = -l->u.constant.f; break;
	default:
		fatal("internal error; unknown unary expression");
	}
	cast(expr);
}

static void
binary(struct expr *expr, enum tokenkind op, struct expr *l, struct expr *r)
{
	expr->kind = EXPRCONST;
	if (expr->type->prop & PROPCOMPLEX) {
		long double ar = l->u.constant.c.re;
		long double ai = l->u.constant.c.im;
		long double br = r->u.constant.c.re;
		long double bi = r->u.constant.c.im;
		switch (op) {
		case TADD:
			expr->u.constant.c.re = ar + br;
			expr->u.constant.c.im = ai + bi;
			break;
		case TSUB:
			expr->u.constant.c.re = ar - br;
			expr->u.constant.c.im = ai - bi;
			break;
		case TMUL:
			expr->u.constant.c.re = ar * br - ai * bi;
			expr->u.constant.c.im = ar * bi + ai * br;
			break;
		case TDIV: {
			long double denom = br * br + bi * bi;
			expr->u.constant.c.re = (ar * br + ai * bi) / denom;
			expr->u.constant.c.im = (ai * br - ar * bi) / denom;
			break;
		}
		default:
			fatal("internal error; unknown complex binary expression");
		}
		cast(expr);
		return;
	}
	if (l->type->prop & PROPFLOAT)
		op |= F;
	else if (l->type->prop & PROPINT && l->type->u.basic.issigned)
		op |= S;
	switch (op) {
	case TMUL:
	case TMUL|S:     expr->u.constant.u = l->u.constant.u * r->u.constant.u; break;
	case TMUL|F:     expr->u.constant.f = l->u.constant.f * r->u.constant.f; break;
	case TDIV:       expr->u.constant.u = l->u.constant.u / r->u.constant.u; break;
	case TDIV|S:     expr->u.constant.i = l->u.constant.i / r->u.constant.i; break;
	case TDIV|F:     expr->u.constant.f = l->u.constant.f / r->u.constant.f; break;
	case TMOD:       expr->u.constant.u = l->u.constant.u % r->u.constant.u; break;
	case TMOD|S:     expr->u.constant.i = l->u.constant.i % r->u.constant.i; break;
	case TADD:
	case TADD|S:     expr->u.constant.u = l->u.constant.u + r->u.constant.u; break;
	case TADD|F:     expr->u.constant.f = l->u.constant.f + r->u.constant.f; break;
	case TSUB:
	case TSUB|S:     expr->u.constant.u = l->u.constant.u - r->u.constant.u; break;
	case TSUB|F:     expr->u.constant.f = l->u.constant.f - r->u.constant.f; break;
	case TSHL:
	case TSHL|S:     expr->u.constant.u = l->u.constant.u << (r->u.constant.u & 63); break;
	case TSHR:       expr->u.constant.u = l->u.constant.u >> (r->u.constant.u & 63); break;
	case TSHR|S:     expr->u.constant.i = l->u.constant.i >> (r->u.constant.u & 63); break;
	case TBAND:
	case TBAND|S:    expr->u.constant.u = l->u.constant.u & r->u.constant.u; break;
	case TBOR:
	case TBOR|S:     expr->u.constant.u = l->u.constant.u | r->u.constant.u; break;
	case TXOR:
	case TXOR|S:     expr->u.constant.u = l->u.constant.u ^ r->u.constant.u; break;
	case TLESS:      expr->u.constant.u = l->u.constant.u < r->u.constant.u; break;
	case TLESS|S:    expr->u.constant.u = l->u.constant.i < r->u.constant.i; break;
	case TLESS|F:    expr->u.constant.u = l->u.constant.f < r->u.constant.f; break;
	case TGREATER:   expr->u.constant.u = l->u.constant.u > r->u.constant.u; break;
	case TGREATER|S: expr->u.constant.u = l->u.constant.i > r->u.constant.i; break;
	case TGREATER|F: expr->u.constant.u = l->u.constant.f > r->u.constant.f; break;
	case TLEQ:       expr->u.constant.u = l->u.constant.u <= r->u.constant.u; break;
	case TLEQ|S:     expr->u.constant.u = l->u.constant.i <= r->u.constant.i; break;
	case TLEQ|F:     expr->u.constant.u = l->u.constant.f <= r->u.constant.f; break;
	case TGEQ:       expr->u.constant.u = l->u.constant.u >= r->u.constant.u; break;
	case TGEQ|S:     expr->u.constant.u = l->u.constant.i >= r->u.constant.i; break;
	case TGEQ|F:     expr->u.constant.u = l->u.constant.f >= r->u.constant.f; break;
	case TEQL:
	case TEQL|S:     expr->u.constant.u = l->u.constant.u == r->u.constant.u; break;
	case TEQL|F:     expr->u.constant.u = l->u.constant.f == r->u.constant.f; break;
	case TNEQ:
	case TNEQ|S:     expr->u.constant.u = l->u.constant.u != r->u.constant.u; break;
	case TNEQ|F:     expr->u.constant.u = l->u.constant.f != r->u.constant.f; break;
	default:
		fatal("internal error; unknown binary expression");
	}
	cast(expr);
}

struct expr *
eval(struct expr *expr)
{
	struct expr *l, *r, *c;
	struct decl *d;
	struct type *t;

	t = expr->type;
	switch (expr->kind) {
	case EXPRIDENT:
		d = expr->u.ident.decl;
		if (d->kind != DECLCONST)
			break;
		expr->kind = EXPRCONST;
		expr->u.constant.u = d->u.enumconst;
		break;
	case EXPRCOMPOUND:
		d = expr->u.compound.decl;
		if (d->u.obj.storage != SDSTATIC)
			break;
		d->value = mkglobal(d);
		emitdata(d, expr->u.compound.init);
		expr->kind = EXPRIDENT;
		expr->u.ident.decl = d;
		break;
	case EXPRUNARY:
		l = eval(expr->base);
		switch (expr->op) {
		case TBAND:
			switch (l->kind) {
			case EXPRUNARY:
				if (l->op == TMUL)
					expr = eval(l->base);
				break;
			case EXPRSTRING:
				l->u.ident.decl = stringdecl(l);
				l->kind = EXPRIDENT;
				expr->base = l;
				break;
			}
			break;
		case TMUL:
			break;
		default:
			if (l->kind != EXPRCONST)
				break;
			unary(expr, expr->op, l);
			break;
		}
		break;
	case EXPRCAST:
		l = eval(expr->base);
		if (l->kind == EXPRCONST) {
			expr->kind = EXPRCONST;
			if (t->prop & PROPCOMPLEX) {
				if (l->type->prop & PROPCOMPLEX) {
					expr->u.constant.c = l->u.constant.c;
				} else if (l->type->prop & PROPFLOAT) {
					expr->u.constant.c.re = l->u.constant.f;
					expr->u.constant.c.im = 0;
				} else if (l->type->prop & PROPINT) {
					expr->u.constant.c.re = l->type->u.basic.issigned ? l->u.constant.i : l->u.constant.u;
					expr->u.constant.c.im = 0;
				}
			} else if (l->type->prop & PROPCOMPLEX) {
				long double re = l->u.constant.c.re;
				if (t->kind == TYPEBOOL) {
					expr->u.constant.u = (l->u.constant.c.re != 0 || l->u.constant.c.im != 0);
				} else if (t->prop & PROPFLOAT) {
					expr->u.constant.f = re;
				} else if (t->prop & PROPINT) {
					if (t->u.basic.issigned) {
						if (re < -0x1p63 || re >= 0x1p63)
							error(&tok.loc, "integer part of complex constant %Lg cannot be represented as signed integer", re);
						expr->u.constant.i = re;
					} else {
						if (re < 0.0 || re >= 0x1p64)
							error(&tok.loc, "integer part of complex constant %Lg cannot be represented as unsigned integer", re);
						expr->u.constant.u = re;
					}
				} else {
					expr->u.constant = l->u.constant;
				}
			} else if (l->type->prop & PROPINT && t->prop & PROPFLOAT) {
				if (l->type->u.basic.issigned)
					expr->u.constant.f = l->u.constant.i;
				else
					expr->u.constant.f = l->u.constant.u;
			} else if (l->type->prop & PROPFLOAT && t->prop & PROPINT) {
				if (t->u.basic.issigned) {
					if (l->u.constant.f < -0x1p63 || l->u.constant.f >= 0x1p63)
						error(&tok.loc, "integer part of floating-point constant %Lg cannot be represented as signed integer", l->u.constant.f);
					expr->u.constant.i = l->u.constant.f;
				} else {
					if (l->u.constant.f < 0.0 || l->u.constant.f >= 0x1p64)
						error(&tok.loc, "integer part of floating-point constant %Lg cannot be represented as unsigned integer", l->u.constant.f);
					expr->u.constant.u = l->u.constant.f;
				}
			} else {
				expr->u.constant = l->u.constant;
			}
			cast(expr);
		} else if (l->type->kind == TYPEPOINTER) {
			/*
			A cast from a pointer to integer is not a valid constant
			expression, but C11 allows implementations to recognize
			other forms of constant expressions (6.6p10), and some
			programs expect this functionality.
			*/
			if (t->kind == TYPEPOINTER || t->prop & PROPINT && t->size == typelong.size)
				expr = l;
		}
		break;
	case EXPRBINARY:
		l = eval(expr->u.binary.l);
		r = eval(expr->u.binary.r);
		expr->u.binary.l = l;
		expr->u.binary.r = r;
		switch (expr->op) {
		case TADD:
			if (r->kind == EXPRBINARY)
				c = l, l = r, r = c;
			/* fallthrough */
		case TSUB:
			if (r->kind != EXPRCONST)
				break;
			if (l->kind == EXPRCONST) {
				binary(expr, expr->op, l, r);
			} else if (l->kind == EXPRBINARY && l->type->kind == TYPEPOINTER && l->op == TADD && l->u.binary.r->kind == EXPRCONST) {
				/* (P + C1) ± C2  ->  P + (C1 ± C2) */
				binary(expr->u.binary.r, expr->op, l->u.binary.r, r);
				expr->op = TADD;
				expr->u.binary.l = l->u.binary.l;
			}
			break;
		case TLOR:
			if (l->kind != EXPRCONST)
				break;
			return consttruth(l) ? l : r;
		case TLAND:
			if (l->kind != EXPRCONST)
				break;
			return consttruth(l) ? r : l;
		case TEQL:
		case TNEQ:
			if (l->kind != EXPRCONST || r->kind != EXPRCONST)
				break;
			if (l->type->prop & PROPCOMPLEX || r->type->prop & PROPCOMPLEX) {
				int eq = l->u.constant.c.re == r->u.constant.c.re &&
					 l->u.constant.c.im == r->u.constant.c.im;
				expr->kind = EXPRCONST;
				expr->u.constant.u = expr->op == TEQL ? eq : !eq;
				cast(expr);
				break;
			}
			binary(expr, expr->op, l, r);
			break;
		default:
			if (l->kind != EXPRCONST || r->kind != EXPRCONST)
				break;
			binary(expr, expr->op, l, r);
		}
		break;
	}

	return expr;
}
