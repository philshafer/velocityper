/*
 * Copyright (c) 2023, Phil Shafer
 * All rights reserved.
 * This SOFTWARE is licensed under the LICENSE provided in the
 * LICENSE file. By downloading, installing, copying, or otherwise
 * using the SOFTWARE, you agree to be bound by the terms of that
 * LICENSE.
 * Phil Shafer, May 2023
 */

#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#ifdef NOCONFIG
#define VELOCITYPER_VERSION "[unknown]" /* Fake a value */
#else
#include "config.h"
#endif /* NOCONFIG */

/*
 * We carry time internally as milliseconds.  Anything more seems
 * like overkill.
 */
#define NSECS_PER_uSEC 1000	/* Nanoseconds per microsecond */
#define uSECS_PER_MSEC 1000	/* Microseconds per millisecond */
#define MSECS_PER_SEC  1000	/* Milliseconds per second */

#define NSECS_PER_MSEC (NSECS_PER_uSEC * uSECS_PER_MSEC)

typedef unsigned long long milliseconds_t;

static struct opts {
    int o_fd;			/* Holds our file descriptor */
    FILE *o_confirm;		/* Terminal to read confirmations from */
    milliseconds_t o_bump;	/* Random 'bump' to pause */
    int o_debug;		/* Debug flag */
    milliseconds_t o_end;	/* Time to pause before each line ends */
    char *o_file;		/* '--file' option */
    int o_force;		/* Force: skip confirmations  */
    int o_dryrun;		/* Dry run: don't confirm anything */
    milliseconds_t o_line;	/* Time to pause after each line */
    milliseconds_t o_pause;	/* Time to pause for '\p' */
    int o_skip;			/* Don't process anything */
    char *o_tty;		/* Terminal to prompt on */
    milliseconds_t o_wait;	/* Time to pause after each character */
} opts;

/*
 * These next two table _MUST_ be kept in the same order.
 */
static struct option long_opts[] = {
    { "bump", required_argument, NULL, 'b' },
    { "confirm", required_argument, NULL, 'C' },
    { "debug", no_argument, NULL, 'D' },
    { "end", required_argument, NULL, 'e' },
    { "force", no_argument, NULL, 'F' },
    { "file", required_argument, NULL, 'f' },
    { "help", no_argument, NULL, 'h' },
    { "line", required_argument, NULL, 'l' },
    { "dry-run", no_argument, NULL, 'n' },
    { "parse-confirm", required_argument, NULL, 'P' },
    { "pause", required_argument, NULL, 'p' },
    { "sleep", required_argument, NULL, 'S' },
    { "skip", no_argument, NULL, 's' },
    { "tput", required_argument, NULL, 'T' },
    { "tty", required_argument, NULL, 't' },
    { "version", no_argument, NULL, 'v' },
    { "wait", required_argument, NULL, 'w' },
    { NULL, 0, NULL, 0 }
};

static const char *help_opts[] = {
    /* b */ " <delay>", "Bump the 'wait' timer by a random amount",
    /* C */ " <tty>", "Provide a terminal name for prompting confirmations",
    /* D */ "", "Enable debug output",
    /* e */ " <end>", "Provide a delay before newlines",
    /* F */ "", "Ignore confirmation requests",
    /* f */ " <file>", "Provide a file for content data",
    /* h */ "", "Display this help message",
    /* l */ " <delay>", "Set delay between lines of data",
    /* n */ "", "Do not place data in input buffer; just echo it",
    /* P */ " <msg>", "Emit a message and wait for confirmation",
    /* p */ " <delay>", "Set the 'pause' delay",
    /* S */ " <time>", "Sleep immediately for the given period",
    /* s */ "", "Do not make output or perform delays",
    /* T */ " <capname>", "Emit terminal capability ('clear' or 'home')",
    /* t */ " <tty>", "Provide a terminal for pushing input data",
    /* v */ "", "Emit version information (and exit)",
    /* w */ " <delay>", "Set the 'wait' delay between characters",
    NULL
};

static const char *program_name;

static void
print_help (void)
{
    int has_help = 1;
    char buf[128];
    struct option *opt;
    const char **help = help_opts;

    printf("Usage: %s [options...] [strings...]\n", program_name ?: "velocityper");

    for (opt = long_opts; opt->name; opt++, help += 2) {
	if (has_help && *help == NULL)
	    has_help = 0;	/* Done run out of data: SNO */

	const char *arg_info = !has_help ? "" : *help;

	snprintf(buf, sizeof(buf), "-%c%s | --%s%s",
		 opt->val, arg_info, opt->name, arg_info);

	printf("    %-30s  %s\n", buf, has_help ? help[1] : "");
    }

    exit(1);
}


/* Forward declaration */
static void handle_file (const char *fname);

static int
prefix_match (const char *str, const char *base)
{
    for ( ; *str && *base; str++, base++) {
	if (*str != *base)
	    return 0;
    }

    return (*str == '\0');
}

static milliseconds_t
strtoms (char *cp)
{
    milliseconds_t t = 0;
    char *ep = NULL;

    t = strtoull(cp, &ep, 10);
    if (t == 0 && errno == EINVAL)
	err(1, "invalid time value: '%s'", cp);

    /* Look for a suffix */
    if (ep && *ep) {
	if (prefix_match(ep, "seconds")) {
	    t *= MSECS_PER_SEC;
	} else if (prefix_match(ep, "milliseconds") || prefix_match(ep, "ms")) {
	    /* nothing */
	} else if (prefix_match(ep, "microseconds") || prefix_match(ep, "us")) {
	    t /= uSECS_PER_MSEC;
	} else if (prefix_match(ep, "nanoseconds") || prefix_match(ep, "ns")) {
	    t /= NSECS_PER_MSEC;
	} else {
	    errx(1, "unknown time unit: '%s'", ep);
	}
    }

    return t;
}

static void
print_version (void)
{
    printf("%s\n", VELOCITYPER_VERSION);
    exit(0);
}

static void
do_confirm (const char *msg)
{
    char buf[BUFSIZ];
    FILE *inp = opts.o_confirm ?: stdin;
    FILE *outp = opts.o_confirm ?: stdout;

    if (msg == NULL)
	msg = "paused";

    fprintf(outp, "[%s; press enter to continue]\n>>> ", msg);
    fflush(outp);

    (void) fgets(buf, sizeof(buf), inp); /* We don't care about content */
}

static void
do_pause (int p)
{
    struct timespec tv;

    tv.tv_sec = p / MSECS_PER_SEC;
    tv.tv_nsec = (p % MSECS_PER_SEC) * NSECS_PER_MSEC;

    nanosleep(&tv, &tv);
}

static void
handle_char (char c)
{
    if (opts.o_debug)
	fprintf(stderr, "[char %#02x]", c);

    /* Pause before newline */
    if ((c == '\r' || c == '\n') && opts.o_end) {
	do_pause(opts.o_end);
    }

    if (opts.o_dryrun) {
	write(opts.o_fd, &c, 1); /* Write the byte to the fd (not stuffed) */

    } else if (opts.o_skip) {
	return;

    } else if (ioctl(opts.o_fd, TIOCSTI, &c) < 0) {
        perror("ioctl");
        exit(1);
    }

    if (opts.o_wait) {
        int bump = opts.o_bump ? rand() % opts.o_bump : 0;

        do_pause(opts.o_wait + bump);
    }

    /* Pause after newline */
    if ((c == '\r' || c == '\n') && opts.o_line) {
	do_pause(opts.o_line);
    }
}

/**
 * Return the mask for len bits in the first byte of a UTF-8 character
 * of length 'len'.  These bits are set to 1.
 */
static inline int
utf8_len_bits (int len)
{
    const uint64_t len_bits
        = (0xf0 << 4)
        | (0xe0 << 3)
        | (0xc0 << 2)
        | (0x00 << 1);

    return (len_bits >> len) & 0xff;
}

static inline int
utf8_to_len (wchar_t wc)
{
    if ((wc & 0x7f) == wc) /* Simple case */
        return 1;
    if ((wc & 0x7ff) == wc)
        return 2;
    if ((wc & 0xffff) == wc)
        return 3;
    if ((wc & 0x1fffff) == wc)
        return 4;
    return -1;          /* Invalid input wchar */
}

static void
handle_char_utf8 (wchar_t wc)
{
    if (opts.o_debug)
	fprintf(stderr, "[wide %#06lx]", (unsigned long) wc);

    int len = utf8_to_len(wc);

    if (len <= 1) { /* Simple case (and error case) */
        handle_char((char)(wc & 0x7f));
        return;
    }

    char len_bits = utf8_len_bits(len); /* Drop in new length bits */

    switch (len) {
    case 4:
        handle_char(len_bits | (char)(0x80 | ((wc >> 18) & 0x03)));
	len_bits = 0;
        /* fallthru */
    case 3:
        handle_char(len_bits | (char)(0x80 | ((wc >> 12) & 0x3f)));
	len_bits = 0;
        /* fallthru */
    case 2:
        handle_char(len_bits | (char)(0x80 | ((wc >> 6) & 0x3f)));
        handle_char((char)(0x80 | (wc & 0x3f)));
    }
}

static void
handle_string (char *str)
{
    char c;
    long val;

    for ( ; (c = *str); str++) {
        if (c == '\\') {
            c = *++str;
            switch (c) {
            case 0:
                return;

            case 'a':
                handle_char(0x07); /* ^G */
                break;

            case 'b':
                handle_char('\b'); /* ^H */
                break;

            case 'e':
                handle_char(0x1b); /* ESC */
                break;

            case 'f':
                handle_char(0x0c); /* ^L */
                break;

            case 'n':
                handle_char('\n');
                break;

            case 'p':
                do_pause(opts.o_pause);
                break;

            case 'P':
		if (!opts.o_force)
		    do_confirm(NULL);
                break;

            case 'r':
                handle_char('\r');
                break;

            case 't':
                handle_char('\t');
                break;

	    case 'u':
		if (str[1] == '{') {
		    wchar_t wc = strtol(str + 2, &str, 0x10);
		    if ((wc != 0 || errno != EINVAL) && *str == '}') {
			handle_char_utf8(wc);
		    }
		}
		break;

	    case 'x':
		if (str[1] == '{') {
		    for (str += 2; *str; str += 2) {
			if (!isxdigit(str[0]) || !isxdigit(str[1]))
			    break;
			
			char num[3] = { str[0], str[1], 0 };
			val = strtol(num, NULL, 0x10);
			if (val != 0 || errno != EINVAL)
			    handle_char(val);
		    }

		} else {
		    if (isxdigit(str[1]) && isxdigit(str[2])) {
			char num[3] = { str[1], str[2], 0 };
			val = strtol(num, NULL, 0x10);
			if (val != 0 || errno != EINVAL)
			    handle_char(val);
			str += 2;
		    }
		}
		break;

	    case '^':
		str += 1;
		val = *str - ('A' - 1); /* "\^G" */
		if (!(0 < val && val < 0x1b)) {
		    val = *str - ('a' - 1); /* "\^g" */
		    if (!(0 < val && val < 0x1b)) {
			switch(*str) {
			case '[': val = 0x1b; break; /* Escape */
			case '\\': val = 0x1c; break; /* File separator */
			case ']': val = 0x1d; break; /* Group separator */
			case '^': val = 0x1e; break; /* Record separator */
			case '_': val = 0x1f; break; /* Unit separator */
			case '?': val = 0x7f; break; /* Delete */
			case '@': val = 0; break;	 /* NUL */
			default: val = ' ';		 /* Nothing better? */
			}
		    }
		}

		handle_char(val);
		break;

            default:
                handle_char(c);
            }
        } else {
            handle_char(c);
        }
    }
}

/*
 * We don't really want to require ncurses and don't want the heartache
 * of initscr() and locale and all that pain.  So we stuff our fingers in
 * our ears and rest in the "knowledge" that vt100 won the terminal wars,
 * and make vt100-compatible output.  What'd you say?  I can't hear you...
 */
static void
handle_tput (const char *optarg)
{
    static const char clear[] = {
	0x1b, 0x5b, 0x48, 0x1b, 0x5b, 0x32, 0x4a, 0x1b, 0x5b, 0x33, 0x4a, 0
    };
    static const char home[] = { 0x1b, 0x5b, 0x48, 0 };
    const char *str = NULL;

    if (strcmp(optarg, "clear") == 0)
	str = clear;
    else if (strcmp(optarg, "home") == 0)
	str = home;
    else
	return;
	
    if (opts.o_dryrun) {
	char c;
	for ( ; (c = *str); str++)
	    handle_char(c);
    } else {
	/*
	 * These characters are written directly to the terminal, not
	 * stuffed into the terminal input stream.
	 */
	(void) write(opts.o_fd, str, strlen(str));
    }
}

static void
process_argv (int ac, char **av)
{
    int rc;
    optind = 1;			/* Reset to allow restarting */
    char *msg = NULL;
    milliseconds_t val;

    while ((rc = getopt_long(ac, av, "b:C:De:Ff:hl:nP:p:S:sT:t:vw:",
                                long_opts, NULL)) != -1) {
        switch (rc) {
        case 'b':
	    opts.o_bump = strtoms(optarg);
            break;

	case 'C':
	    if (opts.o_confirm)
		fclose(opts.o_confirm);

	    opts.o_confirm = fopen(optarg, "r+");
	    if (opts.o_confirm == NULL)
		err(1, "could not open confirmation: '%s'", optarg);
	    break;

        case 'D':
            opts.o_debug = !opts.o_debug;
            break;

        case 'e':
	    opts.o_end = strtoms(optarg);
            break;

        case 'F':
            opts.o_force = !opts.o_force;
            break;

        case 'f':
	    if (opts.o_file)
		err(1, "file name is already provided: '%s'", opts.o_file);
	    opts.o_file = strdup(optarg);
            break;

        case 'h':
	    print_help();
	    break;

        case 'l':
	    opts.o_line = strtoms(optarg);
            break;

        case 'n':
            opts.o_dryrun = !opts.o_dryrun;
            break;

	case 'P':
	    msg = strdup(optarg);
	    break;

        case 'p':
	    opts.o_pause = strtoms(optarg);
            break;

	case 'S':
	    val = strtoms(optarg);
	    if (val)
		do_pause(val);
	    break;

        case 's':
            opts.o_skip = !opts.o_skip;
            break;

	case 'T':
	    handle_tput(optarg);
	    break;

        case 't':
            opts.o_tty = strdup(optarg);
            opts.o_fd = open(optarg, O_WRONLY);
            if (opts.o_fd < 0)
                err(1, "cannot open terminal: '%s'", optarg);
	    break;

	case 'v':
	    print_version();
	    break;

        case 'w':
	    opts.o_wait = strtoms(optarg);
            break;

        default:
            print_help();
        }
    }

    if (msg) {
	do_confirm(msg);
	free(msg);
    }
}

static char *
strskipws (char *cp)
{
    for ( ; *cp; cp++) {
	if (!isspace(*cp))
	    return cp;
    }

    return NULL;
}

static char *
strskiptows (char *cp)
{
    int in_quote = 0;
    char *out = NULL, *skip = NULL;

    for ( ; *cp; cp++) {
	if (*cp == '\\') {
	    if (out == NULL)
		skip = out = cp; /* Start overwriting */

	} else if (*cp == '"') {
	    in_quote = !in_quote;

	    if (out == NULL)
		out = cp; /* Start overwriting */

	    skip = out;

	} else if (in_quote) {
	    /* nothing */

	} else if (isspace(*cp)) {
	    if (out && out != cp)
		*out = '\0';	/* Terminate rewritten string */

	    return cp;
	}

	if (out && !skip)
	    *out++ = *cp;

	skip = NULL;
    }

    if (out)
	*out = '\0';

    return NULL;
}

static int
build_argv (char **av, int avsize, char *cp)
{
    static char dummy[] = "command";
    int ac = 0;

    av[ac++] = dummy;

    while (ac < avsize - 1) {
	cp = strskipws(cp);
	if (cp == NULL)
	    break;

	av[ac++] = cp;
	cp = strskiptows(cp);
	if (cp == NULL)
	    break;

	*cp++ = '\0';
    }

    av[ac] = NULL;
    return ac;
}

/**
 * Process a file for the kdb input buffer.  The syntax is:
 *
 *   # lines that start with '#' are comments
 *   # blank lines are ignored
 *   # lines that start with '-' are command line options:
 *   -p 1000 --wait=100
 *   # lines that start with '\' escape these:
 *   \# the pound sign here is stuffed
 *   \\ and this backslash is as well
 *   # any other lines are stuffed
 *   # the trailing newline is turned into a RETURN ('\r')
 *   # to avoid this, end the line with a backslash
 *   # other backslash characters are support:
 *   #    '\b', '\n', '\p', '\r', '\t', etc
 *   this turns into \
 *   one line\b\b
 */
static void
handle_file (const char *fname)
{
    const int max_av = 32;
    char *av[max_av];
    int ac;
    char buf[BUFSIZ], *cp, *ep;
    FILE *fp = fopen(fname, "r");

    if (fp == NULL)
        err(1, "could not open file '%s'", fname);

    for (;;) {
        if (fgets(buf, sizeof(buf), fp) == NULL)
            break;

        ep = buf + strlen(buf);
        cp = buf;

        if (*cp == '#') {
	    /* If the very first character is '#', it's a comment */
	    continue;
	} else if (*cp == '-') {
	    /* options */
	    if (ep != buf && ep[-1] == '\n')
		ep[-1] = '\0';

	    ac = build_argv(av, max_av, cp);
	    if (ac <= 0)
		continue;

	    process_argv(ac, av);

	} else {
	    if (ep != buf && ep[-1] == '\n') {
		if (ep > buf + 1 && ep[-2] == '\\')
		    ep[-2] = '\0';
		else ep[-1] = '\r';
	    }

	    /*
	     * If the first character is '\', we skip it; this allows us to
	     * have "\# not a comment" lines.
	     */
            if (cp[0] == '\\')
                cp += 1;
            handle_string(cp);
            continue;
        }
       
    }
}

int
main (int argc, char **argv)
{
    int i;

    program_name = argv[0];
    opts.o_fd = 1;

    srand(time(NULL));

    process_argv(argc, argv);

    argc -= optind;
    argv += optind;

    for (i = 0; i < argc; i++) {
        if (i > 0)
             handle_char(' ');
        handle_string(argv[i]);
    }

    while (opts.o_file) {
	handle_file(opts.o_file);

	free(opts.o_file);
	opts.o_file = NULL;
    }

    exit(0);
}
