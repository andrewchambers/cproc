(mirrored on [GitHub][GitHub mirror])

[![builds.sr.ht status](https://builds.sr.ht/~mcf/cproc/commits/master.svg)](https://builds.sr.ht/~mcf/cproc/commits/master)

`cproc` is a [C11] compiler with a native amd64 backend. It is released
under the [ISC] license.

Some [C23 features] and [GNU C extensions] are also implemented.

There is still much to do, but it currently implements most of the
language and is capable of [building software] including itself, mcpp,
gcc 4.7, binutils, and more.

It was inspired by several other small C compilers including [8cc],
[c], [lacc], and [scc].

## Requirements

The compiler itself is written in standard C99 and can be built with
any conforming C99 compiler.

The POSIX driver depends on POSIX.1-2008 interfaces, and the `Makefile`
requires a POSIX-compatible make(1).

At runtime, you will need an assembler and a linker for the target
system. Since the preprocessor is not yet implemented, an external one
is currently required as well.

## Supported targets

The native backend currently supports x86\_64 (System V ABI). The
aarch64 backend is present but not yet implemented.

## Building

Run `./configure` to create a `config.h` and `config.mk` appropriate for
your system. If your system is not supported by the configure script,
you can create these files manually. `config.h` should define several
string arrays (`static char *[]`):

- **`startfiles`**: Objects to pass to the linker at the beginning of
  the link command.
- **`endfiles`**: Objects to pass to the linker at the end of the link
  command (including libc).
- **`preprocesscmd`**: The preprocessor command, and any necessary flags
  for the target system.
- **`codegencmd`**: The backend command, and possibly explicit target flags.
- **`assemblecmd`**: The assembler command.
- **`linkcmd`**: The linker command.

You may also want to customize your environment or `config.mk` with the
appropriate `CC`, `CFLAGS` and `LDFLAGS`.

Once this is done, you can build with

	make

### Bootstrap

The `Makefile` includes several other targets that can be used for
bootstrapping. These targets require the ability to run the tools
specified in `config.h`.

- **`stage2`**: Build the compiler with the initial (`stage1`) output.
- **`stage3`**: Build the compiler with the `stage2` output.
- **`bootstrap`**: Build the `stage2` and `stage3` compilers, and verify
  that they are byte-wise identical.

## What's missing

- Digraph sequences ([6.4.6p3], will not be implemented).
- Inline assembly ([#5]).
- Preprocessor ([#6]).
- Generation of position independent code (i.e. shared libraries,
  modules, PIEs).

## Mailing list

There is a mailing list at [~mcf/cproc@lists.sr.ht]. Feel free to
use it for general discussion, questions, patches, or bug reports
(if you don't have an sr.ht account).

## Issue tracker

Please report any issues to [~mcf/cproc@todo.sr.ht].

## Contributing

Patches are greatly appreciated. Send them to the mailing list
(preferred), or as pull-requests on the [GitHub mirror].

[C11]: http://port70.net/~nsz/c/c11/n1570.html
[ISC]: https://git.sr.ht/~mcf/cproc/blob/master/LICENSE
[C23 features]: https://man.sr.ht/~mcf/cproc/doc/c23.md
[GNU C extensions]: https://man.sr.ht/~mcf/cproc/doc/extensions.md
[building software]: https://man.sr.ht/~mcf/cproc/doc/software.md
[8cc]: https://github.com/rui314/8cc
[c]: https://github.com/andrewchambers/c
[lacc]: https://github.com/larmel/lacc
[scc]: http://www.simple-cc.org/
[5.2.1.1]: http://port70.net/~nsz/c/c11/n1570.html#5.2.1.1
[6.4.6p3]: http://port70.net/~nsz/c/c11/n1570.html#6.4.6p3
[#1]: https://todo.sr.ht/~mcf/cproc/1
[#3]: https://todo.sr.ht/~mcf/cproc/3
[#5]: https://todo.sr.ht/~mcf/cproc/5
[#6]: https://todo.sr.ht/~mcf/cproc/6
[#7]: https://todo.sr.ht/~mcf/cproc/7
[#35]: https://todo.sr.ht/~mcf/cproc/35
[#44]: https://todo.sr.ht/~mcf/cproc/44
[~mcf/cproc@lists.sr.ht]: https://lists.sr.ht/~mcf/cproc
[~mcf/cproc@todo.sr.ht]: https://todo.sr.ht/~mcf/cproc
[GitHub mirror]: https://github.com/michaelforney/cproc
