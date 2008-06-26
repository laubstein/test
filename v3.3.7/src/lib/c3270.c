/*
 * Modifications and original code Copyright 1993, 1994, 1995, 1996,
 *    2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008 by Paul Mattes.
 * Original X11 Port Copyright 1990 by Jeff Sparkes.
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.
 *
 * Copyright 1989 by Georgia Tech Research Corporation, Atlanta, GA 30332.
 *   All Rights Reserved.  GTRC hereby grants public use of this software.
 *   Derivative works based on this software must incorporate this copyright
 *   notice.
 *
 * c3270 and wc3270 are distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the file LICENSE for more details.
 */

/*
 *	c3270.c
 *		A curses-based 3270 Terminal Emulator
 *		A Windows console 3270 Terminal Emulator
 *		Main proceudre.
 */

#include "globals.h"
#if !defined(_WIN32) /*[*/
#include <sys/wait.h>
#include <signal.h>
#endif /*]*/
#include <errno.h>
#include "appres.h"
#include "3270ds.h"
#include "resources.h"

#include "actionsc.h"
#include "ansic.h"
#include "charsetc.h"
#include "ctlrc.h"
#include "ftc.h"
#include "gluec.h"
#include "hostc.h"
#include "idlec.h"
#include "keymapc.h"
#include "kybdc.h"
#include "macrosc.h"
#include "popupsc.h"
#include "printerc.h"
#include "screenc.h"
#include "telnetc.h"
#include "togglesc.h"
#include "trace_dsc.h"
#include "utf8c.h"
#include "utilc.h"
#include "xioc.h"

#if defined(HAVE_LIBREADLINE) /*[*/
#include <readline/readline.h>
#if defined(HAVE_READLINE_HISTORY_H) /*[*/
#include <readline/history.h>
#endif /*]*/
#endif /*]*/

#if defined(_WIN32) /*[*/
#include <windows.h>
#include "winversc.h"
#include "windirsc.h"
#endif /*]*/

#include <lib3270/api.h>

static unsigned long	c3270_AddInput(int source, void (*fn)(void));
static void			c3270_RemoveInput(unsigned long id);

#if !defined(_WIN32) /*[*/
static unsigned long	c3270_AddOutput(int source, void (*fn)(void));
#endif

static unsigned long	c3270_AddExcept(int source, void (*fn)(void));
static unsigned long	c3270_AddTimeOut(unsigned long interval_ms, void (*proc)(void));
static void 			c3270_RemoveTimeOut(unsigned long timer);

static void interact(void);
static void stop_pager(void);
static Boolean process_events(Boolean block);

#if defined(HAVE_LIBREADLINE) /*[*/
static CPPFunction attempted_completion;
static char *completion_entry(const char *, int);
#endif /*]*/

/* Pager state. */
static FILE *pager = NULL;

#if !defined(_WIN32) /*[*/
/* Base keymap. */
static char *base_keymap1 =
"Ctrl<Key>]: Escape\n"
"Ctrl<Key>a Ctrl<Key>a: Key(0x01)\n"
"Ctrl<Key>a Ctrl<Key>]: Key(0x1d)\n"
"Ctrl<Key>a <Key>c: Clear\n"
"Ctrl<Key>a <Key>e: Escape\n"
"Ctrl<Key>a <Key>i: Insert\n"
"Ctrl<Key>a <Key>r: Reset\n"
"Ctrl<Key>a <Key>l: Redraw\n"
"Ctrl<Key>a <Key>m: Compose\n"
"Ctrl<Key>a <Key>^: Key(notsign)\n"
"<Key>DC: Delete\n"
"<Key>UP: Up\n"
"<Key>DOWN: Down\n"
"<Key>LEFT: Left\n"
"<Key>RIGHT: Right\n"
"<Key>HOME: Home\n"
"Ctrl<Key>a <Key>1: PA(1)\n"
"Ctrl<Key>a <Key>2: PA(2)\n"
"Ctrl<Key>a <Key>3: PA(3)\n";
static char *base_keymap2 =
"<Key>F1: PF(1)\n"
"Ctrl<Key>a <Key>F1: PF(13)\n"
"<Key>F2: PF(2)\n"
"Ctrl<Key>a <Key>F2: PF(14)\n"
"<Key>F3: PF(3)\n"
"Ctrl<Key>a <Key>F3: PF(15)\n"
"<Key>F4: PF(4)\n"
"Ctrl<Key>a <Key>F4: PF(16)\n"
"<Key>F5: PF(5)\n"
"Ctrl<Key>a <Key>F5: PF(17)\n"
"<Key>F6: PF(6)\n"
"Ctrl<Key>a <Key>F6: PF(18)\n";
static char *base_keymap3 =
"<Key>F7: PF(7)\n"
"Ctrl<Key>a <Key>F7: PF(19)\n"
"<Key>F8: PF(8)\n"
"Ctrl<Key>a <Key>F8: PF(20)\n"
"<Key>F9: PF(9)\n"
"Ctrl<Key>a <Key>F9: PF(21)\n"
"<Key>F10: PF(10)\n"
"Ctrl<Key>a <Key>F10: PF(22)\n"
"<Key>F11: PF(11)\n"
"Ctrl<Key>a <Key>F11: PF(23)\n"
"<Key>F12: PF(12)\n"
"Ctrl<Key>a <Key>F12: PF(24)\n";

/* Base keymap, 3270 mode. */
static char *base_3270_keymap =
"Ctrl<Key>a <Key>a: Attn\n"
"Ctrl<Key>c: Clear\n"
"Ctrl<Key>d: Dup\n"
"Ctrl<Key>f: FieldMark\n"
"Ctrl<Key>h: Erase\n"
"Ctrl<Key>i: Tab\n"
"Ctrl<Key>j: Newline\n"
"Ctrl<Key>l: Redraw\n"
"Ctrl<Key>m: Enter\n"
"Ctrl<Key>r: Reset\n"
"Ctrl<Key>u: DeleteField\n"
"<Key>IC: ToggleInsert\n"
"<Key>DC: Delete\n"
"<Key>BACKSPACE: Erase\n"
"<Key>HOME: Home\n"
"<Key>END: FieldEnd\n";

#else /*][*/

/* Base keymap. */
static char *base_keymap =
       "Alt <Key>1:      PA(1)\n"
       "Alt <Key>2:      PA(2)\n"
       "Alt <Key>3:      PA(3)\n"
  "Alt Ctrl <Key>]:      Key(0x1d)\n"
      "Ctrl <Key>]:      Escape\n"
       "Alt <Key>^:      Key(notsign)\n"
       "Alt <Key>c:      Clear\n"
       "Alt <Key>l:      Redraw\n"
       "Alt <Key>m:      Compose\n"
     "Shift <Key>F1:     PF(13)\n"
     "Shift <Key>F2:     PF(14)\n"
     "Shift <Key>F3:     PF(15)\n"
     "Shift <Key>F4:     PF(16)\n"
     "Shift <Key>F5:     PF(17)\n"
     "Shift <Key>F6:     PF(18)\n"
     "Shift <Key>F7:     PF(19)\n"
     "Shift <Key>F8:     PF(20)\n"
     "Shift <Key>F9:     PF(21)\n"
     "Shift <Key>F10:    PF(22)\n"
     "Shift <Key>F11:    PF(23)\n"
     "Shift <Key>F12:    PF(24)\n";

/* Base keymap, 3270 mode. */
static char *base_3270_keymap =
      "Ctrl <Key>a:      Attn\n"
       "Alt <Key>a:      Attn\n"
      "Ctrl <Key>c:      Clear\n"
      "Ctrl <Key>d:      Dup\n"
       "Alt <Key>d:      Dup\n"
      "Ctrl <Key>f:      FieldMark\n"
       "Alt <Key>f:      FieldMark\n"
      "Ctrl <Key>h:      Erase\n"
       "Alt <Key>i:      Insert\n"
"Shift Ctrl <Key>i:      BackTab\n"
      "Ctrl <Key>i:      Tab\n"
      "Ctrl <Key>j:      Newline\n"
      "Ctrl <Key>l:      Redraw\n"
      "Ctrl <Key>m:      Enter\n"
      "Ctrl <Key>r:      Reset\n"
       "Alt <Key>r:      Reset\n"
      "Ctrl <Key>u:      DeleteField\n"
      "Ctrl <Key>v:      Paste\n"
           "<Key>INSERT: ToggleInsert\n"
     "Shift <Key>TAB:    BackTab\n"
           "<Key>BACK:   Erase\n"
     "Shift <Key>END:    EraseEOF\n"
           "<Key>END:    FieldEnd\n"
     "Shift <Key>LEFT:   PreviousWord\n"
     "Shift <Key>RIGHT:  NextWord\n";
#endif /*]*/

Boolean any_error_output = False;
Boolean escape_pending = False;
Boolean stop_pending = False;
Boolean dont_return = False;


static const struct lib3270_callbacks callbacks =
{
	sizeof(struct lib3270_callbacks),

	c3270_AddTimeOut,
	c3270_RemoveTimeOut,

	c3270_AddInput,
	c3270_RemoveInput,

	c3270_AddExcept,

#if !defined(_WIN32) /*[*/
	c3270_AddOutput
#endif /*]*/

};

#if defined(_WIN32) /*[*/
char *instdir = NULL;
char myappdata[MAX_PATH];
#endif /*]*/

void
usage(char *msg)
{
	if (msg != CN)
		Warning(msg);
	xs_error("Usage: %s [options] [ps:][LUname@]hostname[:port]",
		programname);
}

#if defined(_WIN32) /*[*/
/*
 * Figure out the install directory and our data directory.
 */
static void
save_dirs(char *argv0)
{
    	char *bsl;

	/* Extract the installation directory from argv[0]. */
	bsl = strrchr(argv0, '\\');
	if (bsl != NULL) {
	    	instdir = NewString(argv0);
		instdir[(bsl - argv0) + 1] = '\0';
	} else
	    	instdir = "";

	/* Figure out the application data directory. */
	if (get_dirs(NULL, myappdata) < 0)
	    	x3270_exit(1);
}
#endif /*]*/

/* Callback for connection state changes. */
static void
main_connect(Boolean ignored)
{
	if (CONNECTED || appres.disconnect_clear) {
#if defined(C3270_80_132) /*[*/
		if (appres.altscreen != CN)
			ctlr_erase(False);
		else
#endif /*]*/
			ctlr_erase(True);
	}
}

/* Callback for application exit. */
static void
main_exiting(Boolean ignored)
{
	if (escaped)
		stop_pager();
	else
		screen_suspend();
}

/* Make sure error messages are seen. */
static void
pause_for_errors(void)
{
	char s[10];

	if (any_error_output) {
		printf("[Press <Enter>] ");
		fflush(stdout);
		(void) fgets(s, sizeof(s), stdin);
		any_error_output = False;
	}
}

#if !defined(_WIN32) /*[*/
/* Empty SIGCHLD handler, ensuring that we can collect child exit status. */
static void
sigchld_handler(int ignored)
{
#if !defined(_AIX) /*[*/
	(void) signal(SIGCHLD, sigchld_handler);
#endif /*]*/
}
#endif /*]*/

int
main(int argc, char *argv[])
{
	const char	*cl_hostname = CN;

#if defined(_WIN32) /*[*/
	(void) get_version_info();
	(void) save_dirs(argv[0]);
#endif /*]*/

	printf("%s\n\n"
		"Copyright 1989-2008 by Paul Mattes, GTRC and others.\n"
		"Type 'show copyright' for full copyright information.\n"
		"Type 'help' for help information.\n\n",
		build);

	if(Register3270Callbacks(&callbacks))
	{
		fprintf(stderr,"Can't register into lib3270 callback table\n");
		return -1;
	}

	add_resource("keymap.base",
#if defined(_WIN32) /*[*/
	    base_keymap
#else /*][*/
	    xs_buffer("%s%s%s", base_keymap1, base_keymap2, base_keymap3)
#endif /*]*/
	    );
	add_resource("keymap.base.3270", NewString(base_3270_keymap));

	argc = parse_command_line(argc, (const char **)argv, &cl_hostname);

	if (charset_init(appres.charset) != CS_OKAY) {
		xs_warning("Cannot find charset \"%s\"", appres.charset);
		(void) charset_init(CN);
	}
	action_init();

#if defined(HAVE_LIBREADLINE) /*[*/
	/* Set up readline. */
	rl_readline_name = "c3270";
	rl_initialize();
	rl_attempted_completion_function = attempted_completion;
#if defined(RL_READLINE_VERSION) /*[*/
	rl_completion_entry_function = completion_entry;
#else /*][*/
	rl_completion_entry_function = (Function *)completion_entry;
#endif /*]*/
#endif /*]*/

	/*
	 * If any error messages have been displayed, make sure the
	 * user sees them -- before screen_init() clears the screen.
	 */
	pause_for_errors();
	screen_init();

	kybd_init();
	idle_init();
	keymap_init();
	hostfile_init();
	hostfile_init();
	ansi_init();

	sms_init();

	register_schange(ST_CONNECT, main_connect);
	register_schange(ST_3270_MODE, main_connect);
	register_schange(ST_EXITING, main_exiting);
#if defined(X3270_FT) /*[*/
	ft_init();
#endif /*]*/
#if defined(X3270_PRINTER) /*[*/
	printer_init();
#endif /*]*/

#if !defined(_WIN32) /*[*/
	/* Make sure we don't fall over any SIGPIPEs. */
	(void) signal(SIGPIPE, SIG_IGN);

	/* Make sure we can collect child exit status. */
	(void) signal(SIGCHLD, sigchld_handler);
#endif /*]*/

	/* Handle initial toggle settings. */
#if defined(X3270_TRACE) /*[*/
	if (!appres.debug_tracing) {
		appres.toggle[DS_TRACE].value = False;
		appres.toggle[EVENT_TRACE].value = False;
	}
#endif /*]*/
	initialize_toggles();

	/* Connect to the host. */
	screen_suspend();
	if (cl_hostname != CN) {
		appres.once = True;
		if (host_connect(cl_hostname) < 0)
			x3270_exit(1);
		/* Wait for negotiations to complete or fail. */
		while (!IN_ANSI && !IN_3270) {
			(void) process_events(True);
			if (!PCONNECTED)
				x3270_exit(1);
		}
		pause_for_errors();
	} else {
		if (appres.secure) {
			Error("Must specify hostname with secure option");
		}
		appres.once = False;
		interact();
	}
	screen_resume();
	screen_disp(False);
	peer_script_init();

	/* Process events forever. */
	while (1) {
		if (!escaped)
			(void) process_events(True);
		if (appres.cbreak_mode && escape_pending) {
			escape_pending = False;
			screen_suspend();
		}
		if (!CONNECTED) {
			screen_suspend();
			(void) printf("Disconnected.\n");
			if (appres.once)
				x3270_exit(0);
			interact();
			screen_resume();
		} else if (escaped) {
			interact();
			trace_event("Done interacting.\n");
			screen_resume();
		}

#if !defined(_WIN32) /*[*/
		if (children && waitpid(0, (int *)0, WNOHANG) > 0)
			--children;
#else /*][*/
		printer_check();
#endif /*]*/
		screen_disp(False);
	}
}

#if !defined(_WIN32) /*[*/
/*
 * SIGTSTP handler for use while a command is running.  Sets a flag so that
 * c3270 will stop before the next prompt is printed.
 */
static void
running_sigtstp_handler(int ignored unused)
{
	signal(SIGTSTP, SIG_IGN);
	stop_pending = True;
}

/*
 * SIGTSTP haandler for use while the prompt is being displayed.
 * Acts immediately by setting SIGTSTP to the default and sending it to
 * ourselves, but also sets a flag so that the user gets one free empty line
 * of input before resuming the connection.
 */
static void
prompt_sigtstp_handler(int ignored unused)
{
	if (CONNECTED)
		dont_return = True;
	signal(SIGTSTP, SIG_DFL);
	kill(getpid(), SIGTSTP);
}
#endif /*]*/

static void
interact(void)
{
	/* In case we got here because a command output, stop the pager. */
	stop_pager();

	trace_event("Interacting.\n");
	if (appres.secure) {
		char s[10];

		printf("[Press <Enter>] ");
		fflush(stdout);
		(void) fgets(s, sizeof(s), stdin);
		return;
	}

#if !defined(_WIN32) /*[*/
	/* Handle SIGTSTP differently at the prompt. */
	signal(SIGTSTP, SIG_DFL);

	/*
	 * Ignore SIGINT at the prompt.
	 * I'm sure there's more we could do.
	 */
	signal(SIGINT, SIG_IGN);
#endif /*]*/

	for (;;) {
		int sl;
		char *s;
#if defined(HAVE_LIBREADLINE) /*[*/
		char *rl_s;
#else /*][*/
		char buf[1024];
#endif /*]*/

		dont_return = False;

		/* Process a pending stop now. */
		if (stop_pending) {
			stop_pending = False;
#if !defined(_WIN32) /*[*/
			signal(SIGTSTP, SIG_DFL);
			kill(getpid(), SIGTSTP);
#endif /*]*/
			continue;
		}

#if !defined(_WIN32) /*[*/
		/* Process SIGTSTPs immediately. */
		signal(SIGTSTP, prompt_sigtstp_handler);
#endif /*]*/
		/* Display the prompt. */
		if (CONNECTED)
		    	(void) printf("Press <Enter> to resume session.\n");
#if defined(HAVE_LIBREADLINE) /*[*/
		s = rl_s = readline("c3270> ");
		if (s == CN) {
			printf("\n");
			exit(0);
		}
#else /*][*/
#if defined(_WIN32) /*[*/
		(void) printf("wc3270> ");
#else /*][*/
		(void) printf("c3270> ");
#endif /*]*/
		(void) fflush(stdout);

		/* Get the command, and trim white space. */
		if (fgets(buf, sizeof(buf), stdin) == CN) {
			printf("\n");
			x3270_exit(0);
		}
		s = buf;
#endif /*]*/
#if !defined(_WIN32) /*[*/
		/* Defer SIGTSTP until the next prompt display. */
		signal(SIGTSTP, running_sigtstp_handler);
#endif /*]*/

		while (isspace(*s))
			s++;
		sl = strlen(s);
		while (sl && isspace(s[sl-1]))
			s[--sl] = '\0';

		/* A null command means go back. */
		if (!sl) {
			if (CONNECTED && !dont_return)
				break;
			else
				continue;
		}

#if defined(HAVE_LIBREADLINE) /*[*/
		/* Save this command in the history buffer. */
		add_history(s);
#endif /*]*/

		/* "?" is an alias for "Help". */
		if (!strcmp(s, "?"))
			s = "Help";

		/*
		 * Process the command like a macro, and spin until it
		 * completes.
		 */
		push_command(s);
		while (sms_active()) {
			(void) process_events(True);
		}

		/* Close the pager. */
		stop_pager();

#if defined(HAVE_LIBREADLINE) /*[*/
		/* Give back readline's buffer. */
		free(rl_s);
#endif /*]*/

		/* If it succeeded, return to the session. */
		if (!macro_output && CONNECTED)
			break;
	}

	/* Ignore SIGTSTP again. */
	stop_pending = False;
#if !defined(_WIN32) /*[*/
	signal(SIGTSTP, SIG_IGN);
#endif /*]*/
}

/* A command is about to produce output.  Start the pager. */
FILE *
start_pager(void)
{
#if !defined(_WIN32) /*[*/
	static char *lesspath = LESSPATH;
	static char *lesscmd = LESSPATH " -EX";
	static char *morepath = MOREPATH;
	static char *or_cat = " || cat";
	char *pager_env;
	char *pager_cmd = CN;

	if (pager != NULL)
		return pager;

	if ((pager_env = getenv("PAGER")) != CN)
		pager_cmd = pager_env;
	else if (strlen(lesspath))
		pager_cmd = lesscmd;
	else if (strlen(morepath))
		pager_cmd = morepath;
	if (pager_cmd != CN) {
		char *s;

		s = Malloc(strlen(pager_cmd) + strlen(or_cat) + 1);
		(void) sprintf(s, "%s%s", pager_cmd, or_cat);
		pager = popen(s, "w");
		Free(s);
		if (pager == NULL)
			(void) perror(pager_cmd);
	}
	if (pager == NULL)
		pager = stdout;
	return pager;
#else /*][*/
	return stdout;
#endif /*]*/
}

/* Stop the pager. */
static void
stop_pager(void)
{
	if (pager != NULL) {
		if (pager != stdout)
			pclose(pager);
		pager = NULL;
	}
}

#if defined(HAVE_LIBREADLINE) /*[*/

static char **matches = (char **)NULL;
static char **next_match;

/* Generate a match list. */
static char **
attempted_completion(char *text, int start, int end)
{
	char *s;
	int i, j;
	int match_count;

	/* If this is not the first word, fail. */
	s = rl_line_buffer;
	while (*s && isspace(*s))
		s++;
	if (s - rl_line_buffer < start) {
		char *t = s;
		struct host *h;

		/*
		 * If the first word is 'Connect' or 'Open', and the
		 * completion is on the second word, expand from the
		 * hostname list.
		 */

		/* See if we're in the second word. */
		while (*t && !isspace(*t))
			t++;
		while (*t && isspace(*t))
			t++;
		if (t - rl_line_buffer < start)
			return NULL;

		/*
		 * See if the first word is 'Open' or 'Connect'.  In future,
		 * we might do other expansions, and this code would need to
		 * be generalized.
		 */
		if (!((!strncasecmp(s, "Open", 4) && isspace(*(s + 4))) ||
		      (!strncasecmp(s, "Connect", 7) && isspace(*(s + 7)))))
			return NULL;

		/* Look for any matches.  Note that these are case-sensitive. */
		for (h = hosts, match_count = 0; h; h = h->next) {
			if (!strncmp(h->name, t, strlen(t)))
				match_count++;
		}
		if (!match_count)
			return NULL;

		/* Allocate the return array. */
		next_match = matches =
		    Malloc((match_count + 1) * sizeof(char **));

		/* Scan again for matches to fill in the array. */
		for (h = hosts, j = 0; h; h = h->next) {
			int skip = 0;

			if (strncmp(h->name, t, strlen(t)))
				continue;

			/*
			 * Skip hostsfile entries that are duplicates of
			 * RECENT entries we've already recorded.
			 */
			if (h->entry_type != RECENT) {
				for (i = 0; i < j; i++) {
					if (!strcmp(matches[i],
					    h->name)) {
						skip = 1;
						break;
					}
				}
			}
			if (skip)
				continue;

			/*
			 * If the string contains spaces, put it in double
			 * quotes.  Otherwise, just copy it.  (Yes, this code
			 * is fairly stupid, and can be fooled by other
			 * whitespace and embedded double quotes.)
			 */
			if (strchr(h->name, ' ') != CN) {
				matches[j] = Malloc(strlen(h->name) + 3);
				(void) sprintf(matches[j], "\"%s\"", h->name);
				j++;
			} else {
				matches[j++] = NewString(h->name);
			}
		}
		matches[j] = CN;
		return NULL;
	}

	/* Search for matches. */
	for (i = 0, match_count = 0; i < actioncount; i++) {
		if (!strncasecmp(actions[i].string, s, strlen(s)))
			match_count++;
	}
	if (!match_count)
		return NULL;

	/* Return what we got. */
	next_match = matches = Malloc((match_count + 1) * sizeof(char **));
	for (i = 0, j = 0; i < actioncount; i++) {
		if (!strncasecmp(actions[i].string, s, strlen(s))) {
			matches[j++] = NewString(actions[i].string);
		}
	}
	matches[j] = CN;
	return NULL;
}

/* Return the match list. */
static char *
completion_entry(const char *text, int state)
{
	char *r;

	if (next_match == NULL)
		return CN;

	if ((r = *next_match++) == CN) {
		Free(matches);
		next_match = matches = NULL;
		return CN;
	} else
		return r;
}

#endif /*]*/

/* c3270-specific actions. */

/* Return a time difference in English */
static char *
hms(time_t ts)
{
	time_t t, td;
	long hr, mn, sc;
	static char buf[128];

	(void) time(&t);

	td = t - ts;
	hr = td / 3600;
	mn = (td % 3600) / 60;
	sc = td % 60;

	if (hr > 0)
		(void) sprintf(buf, "%ld %s %ld %s %ld %s",
		    hr, (hr == 1) ?
			get_message("hour") : get_message("hours"),
		    mn, (mn == 1) ?
			get_message("minute") : get_message("minutes"),
		    sc, (sc == 1) ?
			get_message("second") : get_message("seconds"));
	else if (mn > 0)
		(void) sprintf(buf, "%ld %s %ld %s",
		    mn, (mn == 1) ?
			get_message("minute") : get_message("minutes"),
		    sc, (sc == 1) ?
			get_message("second") : get_message("seconds"));
	else
		(void) sprintf(buf, "%ld %s",
		    sc, (sc == 1) ?
			get_message("second") : get_message("seconds"));

	return buf;
}

static void
status_dump(void)
{
	const char *emode, *ftype, *ts;
#if defined(X3270_TN3270E) /*[*/
	const char *eopts;
#endif /*]*/
	const char *ptype;
	extern int linemode; /* XXX */
	extern time_t ns_time;
	extern int ns_bsent, ns_rsent, ns_brcvd, ns_rrcvd;

	action_output("%s", build);
	action_output("%s %s: %d %s x %d %s, %s, %s",
	    get_message("model"), model_name,
	    maxCOLS, get_message("columns"),
	    maxROWS, get_message("rows"),
	    appres.m3279? get_message("fullColor"): get_message("mono"),
	    (appres.extended && !std_ds_host) ? get_message("extendedDs") :
		get_message("standardDs"));
	action_output("%s %s", get_message("terminalName"), termtype);
	if (connected_lu != CN && connected_lu[0])
		action_output("%s %s", get_message("luName"), connected_lu);
	action_output("%s %s (%s)", get_message("characterSet"),
	    get_charset_name(),
#if defined(X3270_DBCS) /*[*/
	    dbcs? "DBCS": "SBCS"
#else /*][*/
	    "SBCS"
#endif /*]*/
	    );
	action_output("%s %ld/%ld (CGCSGID X'%08lX')",
		get_message("hostCodePage"),
		cgcsgid & 0xffff,
		(cgcsgid >> 16) & 0xffff,
		cgcsgid);
#if !defined(_WIN32) /*[*/
	action_output("%s %s", get_message("localeCodeset"), locale_codeset);
	action_output("%s DBCS %s, UTF-8 (wide curses) %s",
		get_message("buildOpts"),
#if defined(X3270_DBCS) /*[*/
		get_message("buildEnabled"),
#else /*][*/
		get_message("buildDisabled"),
#endif /*]*/
#if defined(CURSES_WIDE) /*[*/
		get_message("buildEnabled")
#else /*][*/
		get_message("buildDisabled")
#endif /*]*/
		);
#else /*][*/
	action_output("%s %d", get_message("windowsCodePage"), windows_cp);
#endif /*]*/
	if (appres.key_map) {
		action_output("%s %s", get_message("keyboardMap"),
		    appres.key_map);
	}
	if (CONNECTED) {
		action_output("%s %s",
		    get_message("connectedTo"),
#if defined(LOCAL_PROCESS) /*[*/
		    (local_process && !strlen(current_host))? "(shell)":
#endif /*]*/
			current_host);
#if defined(LOCAL_PROCESS) /*[*/
		if (!local_process) {
#endif /*]*/
			action_output("  %s %d", get_message("port"),
			    current_port);
#if defined(LOCAL_PROCESS) /*[*/
		}
#endif /*]*/
#if defined(HAVE_LIBSSL) /*[*/
		if (secure_connection)
			action_output("  %s", get_message("secure"));
#endif /*]*/
		ptype = net_proxy_type();
		if (ptype) {
		    	action_output("  %s %s  %s %s  %s %s",
				get_message("proxyType"), ptype,
				get_message("server"), net_proxy_host(),
				get_message("port"), net_proxy_port());
		}
		ts = hms(ns_time);
		if (IN_E)
			emode = "TN3270E ";
		else
			emode = "";
		if (IN_ANSI) {
			if (linemode)
				ftype = get_message("lineMode");
			else
				ftype = get_message("charMode");
			action_output("  %s%s, %s", emode, ftype, ts);
		} else if (IN_SSCP) {
			action_output("  %s%s, %s", emode,
			    get_message("sscpMode"), ts);
		} else if (IN_3270) {
			action_output("  %s%s, %s", emode,
			    get_message("dsMode"), ts);
		} else
			action_output("  %s", ts);

#if defined(X3270_TN3270E) /*[*/
		eopts = tn3270e_current_opts();
		if (eopts != CN) {
			action_output("  %s %s", get_message("tn3270eOpts"),
			    eopts);
		} else if (IN_E) {
			action_output("  %s", get_message("tn3270eNoOpts"));
		}
#endif /*]*/

		if (IN_3270)
			action_output("%s %d %s, %d %s\n%s %d %s, %d %s",
			    get_message("sent"),
			    ns_bsent, (ns_bsent == 1) ?
				get_message("byte") : get_message("bytes"),
			    ns_rsent, (ns_rsent == 1) ?
				get_message("record") : get_message("records"),
			    get_message("Received"),
			    ns_brcvd, (ns_brcvd == 1) ?
				get_message("byte") : get_message("bytes"),
			    ns_rrcvd, (ns_rrcvd == 1) ?
				get_message("record") : get_message("records"));
		else
			action_output("%s %d %s, %s %d %s",
			    get_message("sent"),
			    ns_bsent, (ns_bsent == 1) ?
				get_message("byte") : get_message("bytes"),
			    get_message("received"),
			    ns_brcvd, (ns_brcvd == 1) ?
				get_message("byte") : get_message("bytes"));

#if defined(X3270_ANSI) /*[*/
		if (IN_ANSI) {
			struct ctl_char *c = net_linemode_chars();
			int i;
			char buf[128];
			char *s = buf;

			action_output("%s", get_message("specialCharacters"));
			for (i = 0; c[i].name; i++) {
				if (i && !(i % 4)) {
					*s = '\0';
					action_output(buf);
					s = buf;
				}
				s += sprintf(s, "  %s %s", c[i].name,
				    c[i].value);
			}
			if (s != buf) {
				*s = '\0';
				action_output("%s", buf);
			}
		}
#endif /*]*/
	} else if (HALF_CONNECTED) {
		action_output("%s %s", get_message("connectionPending"),
		    current_host);
	} else
		action_output("%s", get_message("notConnected"));
}

static void
copyright_dump(void)
{
	action_output(" ");
	action_output("Modifications and original code Copyright 1993, 1994, 1995, 1996,");
	action_output(" 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008 by Paul Mattes.");
	action_output("Original X11 Port Copyright 1990 by Jeff Sparkes.");
	action_output("DFT File Transfer Code Copyright October 1995 by Dick Altenbern.");
	action_output("RPQNAMES Code Copyright 2004, 2005 by Don Russell.");
	action_output("  Permission to use, copy, modify, and distribute this software and its");
	action_output("  documentation for any purpose and without fee is hereby granted,");
	action_output("  provided that the above copyright notice appear in all copies and that");
	action_output("  both that copyright notice and this permission notice appear in");
	action_output("  supporting documentation.");
	action_output(" ");
	action_output("Copyright 1989 by Georgia Tech Research Corporation, Atlanta, GA 30332.");
	action_output("  All Rights Reserved.  GTRC hereby grants public use of this software.");
	action_output("  Derivative works based on this software must incorporate this copyright");
	action_output("  notice.");
	action_output(" ");
	action_output(
#if defined(_WIN32) /*[*/
	"wc3270"
#else /*][*/
	"c3270"
#endif /*]*/
	" is distributed in the hope that it will be useful, but WITHOUT ANY");
	action_output("WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS");
	action_output("FOR A PARTICULAR PURPOSE.  See the file LICENSE for more details.");
	action_output(" ");
}

void
Show_action(Widget w unused, XEvent *event unused, String *params,
    Cardinal *num_params)
{
	action_debug(Show_action, event, params, num_params);
	if (*num_params == 0) {
		action_output("  Show copyright   copyright information");
		action_output("  Show stats       connection statistics");
		action_output("  Show status      same as 'Show stats'");
		action_output("  Show keymap      current keymap");
		return;
	}
	if (!strncasecmp(params[0], "stats", strlen(params[0])) ||
	    !strncasecmp(params[0], "status", strlen(params[0]))) {
		status_dump();
	} else if (!strncasecmp(params[0], "keymap", strlen(params[0]))) {
		keymap_dump();
	} else if (!strncasecmp(params[0], "copyright", strlen(params[0]))) {
		copyright_dump();
	} else
		popup_an_error("Unknown 'Show' keyword");
}

#if defined(X3270_TRACE) /*[*/
/* Trace([data|keyboard][on[filename]|off]) */
void
Trace_action(Widget w unused, XEvent *event unused, String *params,
    Cardinal *num_params)
{
	int tg = 0;
	Boolean both = False;
	Boolean on = False;

	action_debug(Trace_action, event, params, num_params);
	if (*num_params == 0) {
		action_output("Data tracing is %sabled.",
		    toggled(DS_TRACE)? "en": "dis");
		action_output("Keyboard tracing is %sabled.",
		    toggled(EVENT_TRACE)? "en": "dis");
		return;
	}
	if (!strcasecmp(params[0], "Data"))
		tg = DS_TRACE;
	else if (!strcasecmp(params[0], "Keyboard"))
		tg = EVENT_TRACE;
	else if (!strcasecmp(params[0], "Off")) {
		both = True;
		on = False;
		if (*num_params > 1) {
			popup_an_error("Trace(): Too many arguments for 'Off'");
			return;
		}
	} else if (!strcasecmp(params[0], "On")) {
		both = True;
		on = True;
	} else {
		popup_an_error("Trace(): Unknown trace type -- "
		    "must be Data or Keyboard");
		return;
	}
	if (!both) {
		if (*num_params == 1 || !strcasecmp(params[1], "On"))
			on = True;
		else if (!strcasecmp(params[1], "Off")) {
			on = False;
			if (*num_params > 2) {
				popup_an_error("Trace(): Too many arguments "
				    "for 'Off'");
				return;
			}
		} else {
			popup_an_error("Trace(): Must be 'On' or 'Off'");
			return;
		}
	}

	if (both) {
		if (on && *num_params > 1)
			appres.trace_file = NewString(params[1]);
		if ((on && !toggled(DS_TRACE)) || (!on && toggled(DS_TRACE)))
			do_toggle(DS_TRACE);
		if ((on && !toggled(EVENT_TRACE)) ||
		    (!on && toggled(EVENT_TRACE)))
			do_toggle(EVENT_TRACE);
	} else if ((on && !toggled(tg)) || (!on && toggled(tg))) {
		if (on && *num_params > 2)
			appres.trace_file = NewString(params[2]);
		do_toggle(tg);
	}
}
#endif /*]*/

/* Break to the command prompt. */
void
Escape_action(Widget w unused, XEvent *event unused, String *params unused,
    Cardinal *num_params unused)
{
	action_debug(Escape_action, event, params, num_params);
	if (!appres.secure)
		screen_suspend();
}

#if !defined(_WIN32) /*[*/

/* Support for c3270 profiles. */

#define PROFILE_ENV	"C3270PRO"
#define NO_PROFILE_ENV	"NOC3270PRO"
#define DEFAULT_PROFILE	"~/.c3270pro"

/* Read in the .c3270pro file. */
void
merge_profile(void)
{
	const char *fname;
	char *profile_name;

	/* Check for the no-profile environment variable. */
	if (getenv(NO_PROFILE_ENV) != CN)
		return;

	/* Read the file. */
	fname = getenv(PROFILE_ENV);
	if (fname == CN || *fname == '\0')
		fname = DEFAULT_PROFILE;
	profile_name = do_subst(fname, True, True);
	(void) read_resource_file(profile_name, False);
	Free(profile_name);
}

#endif /*]*/


/*----[ Callbacks from XtGlue.c ]-------------------------------------------------------------------------*/

#define InputReadMask	0x1
#define InputExceptMask	0x2
#define InputWriteMask	0x4

/* Input events. */
typedef struct input
{
	struct input *next;
	int source;
	int condition;
	void (*proc)(void);
} input_t;

static input_t *inputs = (input_t *)NULL;
static Boolean inputs_changed = False;

static unsigned long c3270_AddInput(int source, void (*fn)(void))
{
	input_t *ip;

	ip = (input_t *)Malloc(sizeof(input_t));
	ip->source = source;
	ip->condition = InputReadMask;
	ip->proc = fn;
	ip->next = inputs;
	inputs = ip;
	inputs_changed = True;
	return (unsigned long)ip;
}

static void c3270_RemoveInput(unsigned long id)
{
	input_t *ip;
	input_t *prev = (input_t *)NULL;

	for (ip = inputs; ip != (input_t *)NULL; ip = ip->next) {
		if (ip == (input_t *)id)
			break;
		prev = ip;
	}
	if (ip == (input_t *)NULL)
		return;
	if (prev != (input_t *)NULL)
		prev->next = ip->next;
	else
		inputs = ip->next;
	Free(ip);
	inputs_changed = True;
}

#if !defined(_WIN32) /*[*/
static unsigned long c3270_AddOutput(int source, void (*fn)(void))
{
	input_t *ip;

	ip = (input_t *)Malloc(sizeof(input_t));
	ip->source = source;
	ip->condition = InputWriteMask;
	ip->proc = fn;
	ip->next = inputs;
	inputs = ip;
	inputs_changed = True;
	return (unsigned long)ip;
}
#endif /*]*/

static unsigned long c3270_AddExcept(int source, void (*fn)(void))
{
#if defined(_WIN32) /*[*/
	return 0;
#else /*][*/
	input_t *ip;

	ip = (input_t *)Malloc(sizeof(input_t));
	ip->source = source;
	ip->condition = InputExceptMask;
	ip->proc = fn;
	ip->next = inputs;
	inputs = ip;
	inputs_changed = True;
	return (unsigned long)ip;
#endif /*]*/
}

typedef struct timeout {
	struct timeout *next;
#if defined(_WIN32) /*[*/
	unsigned long long ts;
#else /*][*/
	struct timeval tv;
#endif /*]*/
	void (*proc)(void);
	Boolean in_play;
} timeout_t;
#define TN	(timeout_t *)NULL
static timeout_t *timeouts = TN;

#if defined(_WIN32) /*[*/
static void ms_ts(unsigned long long *u)
{
	FILETIME t;

	/* Get the system time, in 100ns units. */
	GetSystemTimeAsFileTime(&t);
	memcpy(u, &t, sizeof(unsigned long long));

	/* Divide by 10 to get ms. */
	*u /= 10ULL;
}
#endif /*]*/

static unsigned long c3270_AddTimeOut(unsigned long interval_ms, void (*proc)(void))
{
	timeout_t *t_new;
	timeout_t *t;
	timeout_t *prev = TN;

	t_new = (timeout_t *)Malloc(sizeof(timeout_t));
	t_new->proc = proc;
	t_new->in_play = False;
#if defined(_WIN32) /*[*/
	ms_ts(&t_new->ts);
	t_new->ts += interval_ms;
#else /*][*/
	(void) gettimeofday(&t_new->tv, NULL);
	t_new->tv.tv_sec += interval_ms / 1000L;
	t_new->tv.tv_usec += (interval_ms % 1000L) * 1000L;
	if (t_new->tv.tv_usec > MILLION) {
		t_new->tv.tv_sec += t_new->tv.tv_usec / MILLION;
		t_new->tv.tv_usec %= MILLION;
	}
#endif /*]*/

	/* Find where to insert this item. */
	for (t = timeouts; t != TN; t = t->next) {
#if defined(_WIN32) /*[*/
		if (t->ts > t_new->ts)
#else /*][*/
		if (t->tv.tv_sec > t_new->tv.tv_sec ||
		    (t->tv.tv_sec == t_new->tv.tv_sec &&
		     t->tv.tv_usec > t_new->tv.tv_usec))
#endif /*]*/
			break;
		prev = t;
	}

	/* Insert it. */
	if (prev == TN) {	/* Front. */
		t_new->next = timeouts;
		timeouts = t_new;
	} else if (t == TN) {	/* Rear. */
		t_new->next = TN;
		prev->next = t_new;
	} else {				/* Middle. */
		t_new->next = t;
		prev->next = t_new;
	}

	return (unsigned long)t_new;
}

static void c3270_RemoveTimeOut(unsigned long timer)
{
	timeout_t *st = (timeout_t *)timer;
	timeout_t *t;
	timeout_t *prev = TN;

	if (st->in_play)
		return;
	for (t = timeouts; t != TN; t = t->next) {
		if (t == st) {
			if (prev != TN)
				prev->next = t->next;
			else
				timeouts = t->next;
			Free(t);
			return;
		}
		prev = t;
	}
}

#if defined(_WIN32) /*[*/
#define MAX_HA	256
#endif /*]*/

/* Event dispatcher. */
Boolean process_events(Boolean block)
{
#if defined(_WIN32) /*[*/
	HANDLE ha[MAX_HA];
	DWORD nha;
	DWORD tmo;
	DWORD ret;
	unsigned long long now;
	int i;
#else /*][*/
	fd_set rfds, wfds, xfds;
	int ns;
	struct timeval now, twait, *tp;
#endif /*]*/
	input_t *ip, *ip_next;
	struct timeout *t;
	Boolean any_events;
	Boolean processed_any = False;

	processed_any = False;
    retry:
	/* If we've processed any input, then don't block again. */
	if (processed_any)
		block = False;
	any_events = False;
#if defined(_WIN32) /*[*/
	nha = 0;
#else /*][*/
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&xfds);
#endif /*]*/
	for (ip = inputs; ip != (input_t *)NULL; ip = ip->next) {
		if ((unsigned long)ip->condition & InputReadMask) {
#if defined(_WIN32) /*[*/
			ha[nha++] = (HANDLE)ip->source;
#else /*][*/
			FD_SET(ip->source, &rfds);
#endif /*]*/
			any_events = True;
		}
#if !defined(_WIN32) /*[*/
		if ((unsigned long)ip->condition & InputWriteMask) {
			FD_SET(ip->source, &wfds);
			any_events = True;
		}
		if ((unsigned long)ip->condition & InputExceptMask) {
			FD_SET(ip->source, &xfds);
			any_events = True;
		}
#endif /*]*/
	}
	if (block) {
		if (timeouts != TN) {
#if defined(_WIN32) /*[*/
			ms_ts(&now);
			if (now > timeouts->ts)
				tmo = 0;
			else
				tmo = timeouts->ts - now;
#else /*][*/
			(void) gettimeofday(&now, (void *)NULL);
			twait.tv_sec = timeouts->tv.tv_sec - now.tv_sec;
			twait.tv_usec = timeouts->tv.tv_usec - now.tv_usec;
			if (twait.tv_usec < 0L) {
				twait.tv_sec--;
				twait.tv_usec += MILLION;
			}
			if (twait.tv_sec < 0L)
				twait.tv_sec = twait.tv_usec = 0L;
			tp = &twait;
#endif /*]*/
			any_events = True;
		} else {
#if defined(_WIN32) /*[*/
			tmo = INFINITE;
#else /*][*/
			tp = (struct timeval *)NULL;
#endif /*]*/
		}
	} else {
#if defined(_WIN32) /*[*/
		tmo = 1;
#else /*][*/
		twait.tv_sec = twait.tv_usec = 0L;
		tp = &twait;
#endif /*]*/
	}

	if (!any_events)
		return processed_any;
#if defined(_WIN32) /*[*/
	ret = WaitForMultipleObjects(nha, ha, FALSE, tmo);
	if (ret == WAIT_FAILED) {
#else /*][*/
	ns = select(FD_SETSIZE, &rfds, &wfds, &xfds, tp);
	if (ns < 0) {
		if (errno != EINTR)
			Warning("process_events: select() failed");
#endif /*]*/
		return processed_any;
	}
	inputs_changed = False;
#if defined(_WIN32) /*[*/
	for (i = 0, ip = inputs; ip != (input_t *)NULL; ip = ip_next, i++) {
#else /*][*/
	for (ip = inputs; ip != (input_t *)NULL; ip = ip_next) {
#endif /*]*/
		ip_next = ip->next;
		if (((unsigned long)ip->condition & InputReadMask) &&
#if defined(_WIN32) /*[*/
		    ret == WAIT_OBJECT_0 + i) {
#else /*][*/
		    FD_ISSET(ip->source, &rfds)) {
#endif /*]*/
			(*ip->proc)();
			processed_any = True;
			if (inputs_changed)
				goto retry;
		}
#if !defined(_WIN32) /*[*/
		if (((unsigned long)ip->condition & InputWriteMask) &&
		    FD_ISSET(ip->source, &wfds)) {
			(*ip->proc)();
			processed_any = True;
			if (inputs_changed)
				goto retry;
		}
		if (((unsigned long)ip->condition & InputExceptMask) &&
		    FD_ISSET(ip->source, &xfds)) {
			(*ip->proc)();
			processed_any = True;
			if (inputs_changed)
				goto retry;
		}
#endif /*]*/
	}

	/* See what's expired. */
	if (timeouts != TN) {
#if defined(_WIN32) /*[*/
		ms_ts(&now);
#else /*][*/
		(void) gettimeofday(&now, (void *)NULL);
#endif /*]*/
		while ((t = timeouts) != TN) {
#if defined(_WIN32) /*[*/
			if (t->ts <= now) {
#else /*][*/
			if (t->tv.tv_sec < now.tv_sec ||
			    (t->tv.tv_sec == now.tv_sec &&
			     t->tv.tv_usec < now.tv_usec)) {
#endif /*]*/
				timeouts = t->next;
				t->in_play = True;
				(*t->proc)();
				processed_any = True;
				Free(t);
			} else
				break;
		}
	}
	if (inputs_changed)
		goto retry;

	return processed_any;
}

