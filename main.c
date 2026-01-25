#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"
#include "arg.h"
#include "cc.h"

bool pic;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [input]\n", argv0);
	exit(2);
}

int
main(int argc, char *argv[])
{
	bool pponly = false;
	char *output = NULL, *target = NULL;
	struct array defs = {0}, undefs = {0}, incs = {0};
	struct incopt {
		int kind;
		char *path;
	} *incopt;
	bool nostdinc = false;

	argv0 = progname(argv[0], "cproc-backend");
	ARGBEGIN {
	case 'E':
		pponly = true;
		break;
	case 'f':
		if (strcmp(opt_, "fPIC") == 0 || strcmp(opt_, "fpic") == 0 ||
		    strcmp(opt_, "fPIE") == 0 || strcmp(opt_, "fpie") == 0) {
			pic = true;
			done_ = 1;
			break;
		}
		usage();
	case 't':
		target = EARGF(usage());
		break;
	case 'o':
		output = EARGF(usage());
		break;
	case 's':
		if (strncmp(opt_, "std=", 4) == 0) {
			done_ = 1;
			break;
		}
		usage();
	case 'D': {
		char *arg = EARGF(usage());
		arrayaddptr(&defs, arg);
		break;
	}
	case 'U': {
		char *arg = EARGF(usage());
		arrayaddptr(&undefs, arg);
		break;
	}
	case 'I': {
		char *arg = EARGF(usage());
		incopt = arrayadd(&incs, sizeof(*incopt));
		incopt->kind = PPINC_SYSTEM;
		incopt->path = arg;
		break;
	}
	case 'i': {
		if (strncmp(opt_, "isystem", 7) == 0) {
			char *arg = opt_[7] ? opt_ + 7 : EARGF(usage());
			incopt = arrayadd(&incs, sizeof(*incopt));
			incopt->kind = PPINC_SYSTEM;
			incopt->path = arg;
			done_ = 1;
			break;
		}
		if (strncmp(opt_, "iquote", 6) == 0) {
			char *arg = opt_[6] ? opt_ + 6 : EARGF(usage());
			incopt = arrayadd(&incs, sizeof(*incopt));
			incopt->kind = PPINC_QUOTE;
			incopt->path = arg;
			done_ = 1;
			break;
		}
		if (strncmp(opt_, "idirafter", 9) == 0) {
			char *arg = opt_[9] ? opt_ + 9 : EARGF(usage());
			incopt = arrayadd(&incs, sizeof(*incopt));
			incopt->kind = PPINC_AFTER;
			incopt->path = arg;
			done_ = 1;
			break;
		}
		usage();
		break;
	}
	case 'n':
		if (strcmp(opt_, "nostdinc") == 0) {
			nostdinc = true;
			done_ = 1;
			break;
		}
		usage();
		break;
	default:
		usage();
	} ARGEND

	targinit(target);

	if (output && !freopen(output, "w", stdout))
		fatal("open %s:", output);

	if (argc) {
		while (argc--)
			scanfrom(argv[argc], NULL);
		scanopen();
	} else {
		scanfrom("<stdin>", stdin);
	}

	if (nostdinc)
		ppnostdinc(true);
	arrayforeach (&incs, incopt)
		ppaddinc(incopt->path, incopt->kind);
	for (size_t i = 0; i < defs.len / sizeof(char *); ++i) {
		char *def = ((char **)defs.val)[i];
		char *dup = strdup(def);
		char *eq = dup ? strchr(dup, '=') : NULL;
		if (!dup)
			fatal("strdup:");
		if (eq) {
			*eq = '\0';
			ppdef(dup, eq + 1);
		} else {
			ppdef(dup, NULL);
		}
		free(dup);
	}
	for (size_t i = 0; i < undefs.len / sizeof(char *); ++i) {
		char *name = ((char **)undefs.val)[i];
		ppundef(name);
	}
	ppinit();
	if (pponly) {
		ppflags |= PPNEWLINE;
		while (tok.kind != TEOF) {
			tokenprint(&tok);
			next();
		}
	} else {
		scopeinit();
		while (tok.kind != TEOF) {
			if (!decl(&filescope, NULL)) {
				if (tok.kind == TSEMICOLON)
					error(&tok.loc, "unexpected ';' at top-level");
				error(&tok.loc, "expected declaration or function definition");
			}
		}
		emittentativedefns();
		fputs("\t.section .note.GNU-stack,\"\",@progbits\n", stdout);
	}

	fflush(stdout);
	if (ferror(stdout))
		fatal("write failed");
	return 0;
}
