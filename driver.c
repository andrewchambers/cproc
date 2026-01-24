#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

enum filetype {
	NONE,   /* detect based on file extension */
	ASM,    /* assembly source */
	ASMPP,  /* assembly source requiring preprocessing */
	C,      /* C source */
	CHDR,   /* C header */
	CPPOUT, /* preprocessed C source */
	OBJ,    /* object file */
};

enum stage {
	PREPROCESS,
	COMPILE,
	ASSEMBLE,
	LINK,
};

#include "config.h"

struct stageinfo {
	const char *name;
	struct array cmd;
	size_t cmdbase;
	pid_t pid;
};

struct input {
	char *name;
	unsigned stages;
	enum filetype filetype;
	bool lib;
};

static struct {
	bool nostdlib;
	bool verbose;
	bool shared;
} flags;
static struct stageinfo stages[] = {
	[PREPROCESS] = {.name = "preprocess"},
	[COMPILE]    = {.name = "compile"},
	[ASSEMBLE]   = {.name = "assemble"},
	[LINK]       = {.name = "link"},
};

static const char *const ignoreflags[] = {
	"fno-builtin",
	"pedantic",
	"pipe"
};

static void
usage(const char *fmt, ...)
{
	va_list ap;

	if (fmt) {
		fprintf(stderr, "%s: ", argv0);
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fputc('\n', stderr);
	}
	fprintf(stderr, "usage: %s [-c|-S|-E] [-shared] [-D name[=value]] [-U name] [-s] [-g] [-o output] input...\n", argv0);
	exit(2);
}

static enum filetype
detectfiletype(const char *name)
{
	const char *dot;

	dot = strrchr(name, '.');
	if (dot) {
		++dot;
		if (strcmp(dot, "c") == 0)
			return C;
		if (strcmp(dot, "h") == 0)
			return CHDR;
		if (strcmp(dot, "i") == 0)
			return CPPOUT;
		if (strcmp(dot, "s") == 0)
			return ASM;
		if (strcmp(dot, "S") == 0)
			return ASMPP;
	}

	return OBJ;
}

static char *
changeext(const char *name, const char *ext)
{
	const char *slash, *dot;
	char *result;
	size_t baselen;

	slash = strrchr(name, '/');
	if (slash)
		name = slash + 1;
	dot = strrchr(name, '.');
	baselen = dot ? (size_t)(--dot - name + 1) : strlen(name);
	result = xmalloc(baselen + strlen(ext) + 2);
	memcpy(result, name, baselen);
	result[baselen] = '.';
	strcpy(result + baselen + 1, ext);

	return result;
}

static int
spawn(pid_t *pid, struct array *args, posix_spawn_file_actions_t *actions)
{
	extern char **environ;
	char **arg;

	if (flags.verbose) {
		fprintf(stderr, "%s: spawning", argv0);
		for (arg = args->val; *arg; ++arg)
			fprintf(stderr, " %s", *arg);
		fputc('\n', stderr);
	}
	return posix_spawnp(pid, *(char **)args->val, actions, NULL, args->val, environ);
}

static int
spawnphase(struct stageinfo *phase, int *fd, char *input, char *output, bool last)
{
	int ret, pipefd[2];
	posix_spawn_file_actions_t actions;

	phase->cmd.len = phase->cmdbase;
	if (last && output) {
		arrayaddptr(&phase->cmd, "-o");
		arrayaddptr(&phase->cmd, output);
	}
	if (input && *fd == -1)
		arrayaddptr(&phase->cmd, input);
	arrayaddptr(&phase->cmd, NULL);

	ret = posix_spawn_file_actions_init(&actions);
	if (ret)
		goto err0;
	if (*fd != -1)
		ret = posix_spawn_file_actions_adddup2(&actions, *fd, 0);
	if (!last) {
		if (pipe(pipefd) < 0) {
			ret = errno;
			goto err1;
		}
		if (fcntl(pipefd[0], F_SETFD, FD_CLOEXEC) < 0) {
			ret = errno;
			goto err2;
		}
		if (fcntl(pipefd[1], F_SETFD, FD_CLOEXEC) < 0) {
			ret = errno;
			goto err2;
		}
		ret = posix_spawn_file_actions_adddup2(&actions, pipefd[1], 1);
		if (ret)
			goto err2;
	}

	ret = spawn(&phase->pid, &phase->cmd, &actions);
	if (ret)
		goto err2;
	if (!last) {
		*fd = pipefd[0];
		close(pipefd[1]);
	}
	posix_spawn_file_actions_destroy(&actions);

	return 0;

err2:
	if (!last) {
		close(pipefd[0]);
		close(pipefd[1]);
	}
err1:
	posix_spawn_file_actions_destroy(&actions);
err0:
	return ret;
}

static bool
succeeded(const char *phase, pid_t pid, int status)
{
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0)
			return true;
		warn("%s: process %ju exited with status %d", phase, (uintmax_t)pid, WEXITSTATUS(status));
	} else if (WIFSIGNALED(status)) {
		warn("%s: process signaled: %s", phase, strsignal(WTERMSIG(status)));
	} else {
		warn("%s: process failed", phase);
	}
	return false;
}

static void
buildobj(struct input *input, char *output)
{
	const char *phase;
	size_t i, npids;
	pid_t pid;
	int status, ret, fd;
	bool success = true;

	if (input->filetype == OBJ)
		return;
	if (input->stages & 1<<LINK) {
		input->stages &= ~(1<<LINK);
		output = strdup("/tmp/cproc-XXXXXX");
		if (!output)
			fatal("strdup:");
		fd = mkstemp(output);
		if (fd < 0)
			fatal("mkstemp:");
		close(fd);
	} else if (output) {
		if (strcmp(output, "-") == 0)
			output = NULL;
	} else if (input->stages & 1<<ASSEMBLE) {
		output = changeext(input->name, "o");
	} else if (input->stages & 1<<COMPILE) {
		output = changeext(input->name, "s");
	}
	if (strcmp(input->name, "-") == 0)
		input->name = NULL;

	npids = 0;
	for (i = PREPROCESS, fd = -1; input->stages; ++i) {
		if (!(input->stages & 1<<i))
			continue;
		input->stages &= ~(1<<i);
		ret = spawnphase(&stages[i], &fd, input->name, output, !input->stages);
		if (ret) {
			warn("%s: spawn \"%s\": %s", stages[i].name, *(char **)stages[i].cmd.val, strerror(ret));
			goto kill;
		}
		++npids;
	}
	input->name = output;

	while (npids > 0) {
		pid = wait(&status);
		if (pid < 0)
			fatal("waitpid:");
		for (i = 0; i < LEN(stages); ++i) {
			if (pid == stages[i].pid) {
				--npids;
				stages[i].pid = 0;
				phase = stages[i].name;
				break;
			}
		}
		if (i == LEN(stages))
			continue;  /* unknown process */
		if (!succeeded(phase, pid, status)) {
kill:
			if (success && npids > 0) {
				for (i = 0; i < LEN(stages); ++i) {
					if (stages[i].pid)
						kill(stages[i].pid, SIGTERM);
				}
			}
			success = false;
		}
	}
	if (!success) {
		if (output)
			unlink(output);
		exit(1);
	}
}

static void
buildexe(struct input *inputs, size_t ninputs, char *output)
{
	struct stageinfo *s = &stages[LINK];
	size_t i;
	int ret, status;
	pid_t pid;

	arrayaddptr(&s->cmd, "-o");
	arrayaddptr(&s->cmd, output);
	if (!flags.nostdlib && LEN(startfiles)) {
		for (i = 0; i < LEN(startfiles); ++i) {
			const char *arg = startfiles[i];
			if (strcmp(arg, "-l") == 0 && i + 1 < LEN(startfiles)) {
				const char *lib = startfiles[++i];
				if (flags.shared) {
					if (strcmp(lib, ":crt1.o") == 0 || strcmp(lib, ":Scrt1.o") == 0)
						continue;
					if (strcmp(lib, ":crtbegin.o") == 0)
						lib = ":crtbeginS.o";
				}
				arrayaddptr(&s->cmd, "-l");
				arrayaddptr(&s->cmd, (char *)lib);
				continue;
			}
			if (flags.shared && (strcmp(arg, ":crt1.o") == 0 || strcmp(arg, ":Scrt1.o") == 0))
				continue;
			arrayaddptr(&s->cmd, (char *)arg);
		}
	}
	for (i = 0; i < ninputs; ++i) {
		if (inputs[i].lib)
			arrayaddptr(&s->cmd, "-l");
		arrayaddptr(&s->cmd, inputs[i].name);
	}
	if (!flags.nostdlib && LEN(endfiles)) {
		for (i = 0; i < LEN(endfiles); ++i) {
			const char *arg = endfiles[i];
			if (strcmp(arg, "-l") == 0 && i + 1 < LEN(endfiles)) {
				const char *lib = endfiles[++i];
				if (flags.shared && strcmp(lib, ":crtend.o") == 0)
					lib = ":crtendS.o";
				arrayaddptr(&s->cmd, "-l");
				arrayaddptr(&s->cmd, (char *)lib);
				continue;
			}
			if (flags.shared && strcmp(arg, ":crtend.o") == 0) {
				arrayaddptr(&s->cmd, ":crtendS.o");
				continue;
			}
			arrayaddptr(&s->cmd, (char *)arg);
		}
	}
	arrayaddptr(&s->cmd, NULL);

	ret = spawn(&pid, &s->cmd, NULL);
	if (ret)
		fatal("%s: spawn \"%s\": %s", s->name, *(char **)s->cmd.val, strerror(errno));
	if (waitpid(pid, &status, 0) < 0)
		fatal("waitpid %ju:", (uintmax_t)pid);
	for (i = 0; i < ninputs; ++i) {
		if (inputs[i].filetype != OBJ)
			unlink(inputs[i].name);
	}
	exit(!succeeded(s->name, pid, status));
}

static char *
nextarg(char ***argv)
{
	if ((**argv)[2] != '\0')
		return &(**argv)[2];
	++*argv;
	if (!**argv)
		usage(NULL);
	return **argv;
}

static char *
compilecommand(char *arg, const char *backend)
{
	char self[PATH_MAX], *cmd;
	const char *suffixes[] = {"-amd64", "-arm64"};
	size_t baselen, backendlen, i, slen;
	ssize_t nread;

	backendlen = strlen(backend);
	nread = readlink("/proc/self/exe", self, sizeof(self) - 1);
	if (nread == -1) {
		if (snprintf(self, sizeof(self), "%s", arg) >= (int)sizeof(self))
			fatal("argv[0] is too large");
	} else {
		if (nread >= (ssize_t)sizeof(self))
			fatal("target of /proc/self/exe is too large");
		self[nread] = '\0';
	}

	baselen = strlen(self);
	for (i = 0; i < LEN(suffixes); ++i) {
		slen = strlen(suffixes[i]);
		if (baselen > slen && strcmp(self + baselen - slen, suffixes[i]) == 0) {
			baselen -= slen;
			break;
		}
	}
	if (baselen + 1 + backendlen >= sizeof(self))
		fatal("argv[0] is too large");
	self[baselen] = '-';
	memcpy(self + baselen + 1, backend, backendlen + 1);

	cmd = strdup(self);
	if (!cmd)
		fatal("strdup:");
	return cmd;
}

static int
hasprefix(const char *str, const char *pfx)
{
	return memcmp(str, pfx, strlen(pfx)) == 0;
}

static bool
picflag(const char *arg, int *level, bool *pie)
{
	if (strcmp(arg, "-fPIC") == 0) {
		*level = 2;
		*pie = false;
		return true;
	}
	if (strcmp(arg, "-fpic") == 0) {
		*level = 1;
		*pie = false;
		return true;
	}
	if (strcmp(arg, "-fPIE") == 0) {
		*level = 2;
		*pie = true;
		return true;
	}
	if (strcmp(arg, "-fpie") == 0) {
		*level = 1;
		*pie = true;
		return true;
	}
	return false;
}

static void
add_define(struct array *cmd, const char *name, int level)
{
	char buf[64];
	int n = snprintf(buf, sizeof(buf), "%s=%d", name, level);
	char *def;

	if (n < 0 || (size_t)n >= sizeof(buf))
		fatal("macro definition too long");
	def = xmalloc((size_t)n + 1);
	memcpy(def, buf, (size_t)n + 1);
	arrayaddptr(cmd, "-D");
	arrayaddptr(cmd, def);
}

int
main(int argc, char *argv[])
{
	enum stage last = LINK;
	enum filetype filetype = 0;
	char *arg, *end, *output = NULL, *arch;
	const char *backend;
	struct array inputs = {0}, *cmd;
	struct input *input;
	size_t i;
	int pic_level = 0;
	int pie_level = 0;
	const char *pic_arg = NULL;

	argv0 = progname(argv[0], "cproc");

	if (hasprefix(target, "x86_64-") || hasprefix(target, "amd64-")) {
		arch = "x86_64-sysv";
		backend = "amd64";
	} else if (hasprefix(target, "aarch64-")) {
		arch = "aarch64";
		backend = "arm64";
	} else {
		fatal("unsupported target '%s'", target);
		return 1;  /* unreachable */
	}

	arrayaddbuf(&stages[PREPROCESS].cmd, preprocesscmd, sizeof(preprocesscmd));
	arrayaddptr(&stages[COMPILE].cmd, compilecommand(argv[0], backend));
	arrayaddbuf(&stages[ASSEMBLE].cmd, assemblecmd, sizeof(assemblecmd));
	arrayaddbuf(&stages[LINK].cmd, linkcmd, sizeof(linkcmd));
	arrayaddptr(&stages[COMPILE].cmd, "-t");
	arrayaddptr(&stages[COMPILE].cmd, arch);

	for (;;) {
ignore:
		++argv, --argc;
		arg = *argv;
		if (!arg)
			break;
		if (arg[0] != '-' || arg[1] == '\0') {
			input = arrayadd(&inputs, sizeof(*input));
			input->name = arg;
			input->lib = false;
			input->filetype = filetype == NONE && arg[1] ? detectfiletype(arg) : filetype;
			switch (input->filetype) {
			case ASM:    input->stages =                                     1<<ASSEMBLE|1<<LINK; break;
			case ASMPP:  input->stages = 1<<PREPROCESS|                      1<<ASSEMBLE|1<<LINK; break;
			case C:      input->stages = 1<<PREPROCESS|1<<COMPILE|1<<ASSEMBLE|1<<LINK; break;
			case CHDR:   input->stages = 1<<PREPROCESS                                          ; break;
			case CPPOUT: input->stages =               1<<COMPILE|1<<ASSEMBLE|1<<LINK; break;
			case OBJ:    input->stages =                                                 1<<LINK; break;
			default:     usage("reading from standard input requires -x");
			}
			continue;
		}

		/* ignore these parameters */
		for (i = 0; i < LEN(ignoreflags); ++i) {
			if (strcmp(arg + 1, ignoreflags[i]) == 0)
				goto ignore;
		}

		/* TODO: use a binary search for these long parameters */
		{
			int level;
			bool pie;

			if (strcmp(arg, "-fno-pic") == 0 || strcmp(arg, "-fno-PIC") == 0 ||
			    strcmp(arg, "-fno-pie") == 0 || strcmp(arg, "-fno-PIE") == 0) {
				pic_level = 0;
				pie_level = 0;
				pic_arg = NULL;
				continue;
			}
			if (picflag(arg, &level, &pie)) {
				pic_level = level;
				pie_level = pie ? level : 0;
				pic_arg = arg;
				continue;
			}
		}
		if (strcmp(arg, "-nostdlib") == 0) {
			flags.nostdlib = true;
		} else if (strcmp(arg, "-nostdinc") == 0) {
			arrayaddptr(&stages[PREPROCESS].cmd, arg);
		} else if (strcmp(arg, "-static") == 0) {
			arrayaddptr(&stages[LINK].cmd, arg);
		} else if (strcmp(arg, "-shared") == 0) {
			flags.shared = true;
			arrayaddptr(&stages[LINK].cmd, arg);
			if (!pic_level) {
				pic_level = 2;
				pie_level = 0;
				pic_arg = "-fPIC";
			}
		} else if (strcmp(arg, "-include") == 0 || strcmp(arg, "-idirafter") == 0 || strcmp(arg, "-isystem") == 0 || strcmp(arg, "-iquote") == 0) {
			if (!--argc)
				usage(NULL);
			arrayaddptr(&stages[PREPROCESS].cmd, arg);
			arrayaddptr(&stages[PREPROCESS].cmd, *++argv);
		} else if (strncmp(arg, "-std=", 5) == 0) {
			/* pass through to the preprocessor, it may
			 * affect its default definitions */
			arrayaddptr(&stages[PREPROCESS].cmd, arg);
		} else if (strcmp(arg, "-pthread") == 0) {
			arrayaddptr(&stages[LINK].cmd, "-l");
			arrayaddptr(&stages[LINK].cmd, "pthread");
		} else if (strcmp(arg, "-dumpmachine") == 0) {
			/* print target and exit */
			puts(target);
			return 0;
		} else {
			if (arg[2] != '\0' && strchr("cESsv", arg[1]))
				usage(NULL);
			switch (arg[1]) {
			case 'c':
				last = ASSEMBLE;
				break;
			case 'D':
				arrayaddptr(&stages[PREPROCESS].cmd, "-D");
				arrayaddptr(&stages[PREPROCESS].cmd, nextarg(&argv));
				break;
			case 'E':
				last = PREPROCESS;
				break;
			case 'g':
				/* ignore */
				break;
			case 'I':
				arrayaddptr(&stages[PREPROCESS].cmd, "-I");
				arrayaddptr(&stages[PREPROCESS].cmd, nextarg(&argv));
				break;
			case 'L':
				arrayaddptr(&stages[LINK].cmd, "-L");
				arrayaddptr(&stages[LINK].cmd, nextarg(&argv));
				break;
			case 'l':
				input = arrayadd(&inputs, sizeof(*input));
				input->name = nextarg(&argv);
				input->lib = true;
				input->filetype = OBJ;
				input->stages = 1<<LINK;
				break;
			case 'M':
				if (strcmp(arg, "-M") == 0 || strcmp(arg, "-MM") == 0) {
					arrayaddptr(&stages[PREPROCESS].cmd, arg);
					last = PREPROCESS;
				} else if (strcmp(arg, "-MD") == 0 || strcmp(arg, "-MMD") == 0) {
					arrayaddptr(&stages[PREPROCESS].cmd, arg);
				} else if (strcmp(arg, "-MT") == 0 || strcmp(arg, "-MF") == 0) {
					if (!--argc)
						usage(NULL);
					arrayaddptr(&stages[PREPROCESS].cmd, arg);
					arrayaddptr(&stages[PREPROCESS].cmd, *++argv);
				} else {
					usage(NULL);
				}
				break;
			case 'O':
				/* ignore */
				break;
			case 'o':
				output = nextarg(&argv);
				break;
			case 'P':
				arrayaddptr(&stages[PREPROCESS].cmd, "-P");
				break;
			case 'S':
				last = COMPILE;
				break;
			case 's':
				arrayaddptr(&stages[LINK].cmd, "-s");
				break;
			case 'U':
				arrayaddptr(&stages[PREPROCESS].cmd, "-U");
				arrayaddptr(&stages[PREPROCESS].cmd, nextarg(&argv));
				break;
			case 'v':
				flags.verbose = true;
				break;
			case 'W':
				if (arg[2] && arg[3] == ',') {
					switch (arg[2]) {
					case 'p': cmd = &stages[PREPROCESS].cmd; break;
					case 'a': cmd = &stages[ASSEMBLE].cmd; break;
					case 'l': cmd = &stages[LINK].cmd; break;
					default: usage(NULL);
					}
					for (arg += 4; arg; arg = end ? end + 1 : NULL) {
						end = strchr(arg, ',');
						if (end)
							*end = '\0';
						arrayaddptr(cmd, arg);
					}
				} else {
					/* ignore warning flag */
				}
				break;
			case 'x':
				arg = nextarg(&argv);
				if (strcmp(arg, "none") == 0)
					filetype = NONE;
				else if (strcmp(arg, "c") == 0)
					filetype = C;
				else if (strcmp(arg, "c-header") == 0)
					filetype = CHDR;
				else if (strcmp(arg, "cpp-output") == 0)
					filetype = CPPOUT;
				else if (strcmp(arg, "assembler") == 0)
					filetype = ASM;
				else if (strcmp(arg, "assembler-with-cpp") == 0)
					filetype = ASMPP;
				else
					usage("unknown language '%s'", arg);
				break;
			default:
				usage("unknown option '%s'", arg);
			}
		}
	}

	if (pic_level && pic_arg) {
		arrayaddptr(&stages[COMPILE].cmd, (char *)pic_arg);
		add_define(&stages[PREPROCESS].cmd, "__PIC__", pic_level);
		add_define(&stages[PREPROCESS].cmd, "__pic__", pic_level);
		if (pie_level) {
			add_define(&stages[PREPROCESS].cmd, "__PIE__", pie_level);
			add_define(&stages[PREPROCESS].cmd, "__pie__", pie_level);
		}
	}
	for (i = 0; i < LEN(stages); ++i)
		stages[i].cmdbase = stages[i].cmd.len;
	if (inputs.len == 0)
		usage(NULL);
	if (output) {
		if (strcmp(output, "-") == 0) {
			if (last >= ASSEMBLE)
				usage("cannot write object to stdout");
		} else if (last != LINK && inputs.len > sizeof(*input)) {
			usage("cannot specify -o with multiple input files without linking");
		}
	}
	arrayforeach (&inputs, input) {
		/* ignore the input if it doesn't participate in the last stage */
		if (!(input->stages & 1 << last))
			continue;
		/* only run up through the last stage */
		input->stages &= (1 << last + 1) - 1;
		buildobj(input, output);
	}
	if (last == LINK) {
		if (!output)
			output = "a.out";
		buildexe(inputs.val, inputs.len / sizeof(*input), output);
	}
	return 0;
}
