#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

char *argv0;

static void
vwarn(const char *fmt, va_list ap)
{
	fprintf(stderr, "%s: ", argv0);
	vfprintf(stderr, fmt, ap);
	if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);
}

void
fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);
	exit(1);
}

void *
reallocarray(void *buf, size_t n, size_t m)
{
	if (n > 0 && SIZE_MAX / n < m) {
		errno = ENOMEM;
		return NULL;
	}
	return realloc(buf, n * m);
}

void *
xreallocarray(void *buf, size_t n, size_t m)
{
	buf = reallocarray(buf, n, m);
	if (!buf && n && m)
		fatal("reallocarray:");

	return buf;
}

void *
xmalloc(size_t len)
{
	void *buf;

	buf = malloc(len);
	if (!buf && len)
		fatal("malloc:");

	return buf;
}

char *
progname(char *name, char *fallback)
{
	char *slash;

	if (!name)
		return fallback;
	slash = strrchr(name, '/');
	return slash ? slash + 1 : name;
}

void
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

void
sbaddc(struct strbuf *sb, char c)
{
	sbgrow(sb, 1);
	sb->buf[sb->len++] = c;
	sb->buf[sb->len] = 0;
}

void
sbadds(struct strbuf *sb, const char *s)
{
	size_t n = strlen(s);

	sbgrow(sb, n);
	memcpy(sb->buf + sb->len, s, n);
	sb->len += n;
	sb->buf[sb->len] = 0;
}

void *
arrayadd(struct array *a, size_t n)
{
	void *v;

	if (a->cap - a->len < n) {
		do a->cap = a->cap ? a->cap * 2 : 256;
		while (a->cap - a->len < n);
		a->val = realloc(a->val, a->cap);
		if (!a->val)
			fatal("realloc");
	}
	v = (char *)a->val + a->len;
	a->len += n;

	return v;
}

void
arrayaddptr(struct array *a, void *v)
{
	*(void **)arrayadd(a, sizeof(v)) = v;
}

void
arrayaddbuf(struct array *a, const void *src, size_t n)
{
	memcpy(arrayadd(a, n), src, n);
}

void *
arraylast(struct array *a, size_t n)
{
	if (a->len == 0)
		return NULL;
	assert(n <= a->len);
	return (char *)a->val + a->len - n;
}

void
listinsert(struct list *list, struct list *new)
{
	new->next = list->next;
	new->prev = list;
	list->next->prev = new;
	list->next = new;
}

void
listremove(struct list *list)
{
	list->next->prev = list->prev;
	list->prev->next = list->next;
	list->next = list->prev = NULL;
}
