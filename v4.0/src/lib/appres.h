/*
 * Modifications Copyright 1993, 1994, 1995, 1996, 1999, 2000, 2001, 2002,
 *  2003, 2004, 2005, 2007 by Paul Mattes.
 * Copyright 1990 by Jeff Sparkes.
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for any purpose and without fee is hereby granted,
 *  provided that the above copyright notice appear in all copies and that
 *  both that copyright notice and this permission notice appear in
 *  supporting documentation.
 *
 * x3270, c3270, s3270 and tcl3270 are distributed in the hope that they will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the file LICENSE
 * for more details.
 */

/*
 *	appres.h
 *		Application resource definitions for x3270, c3270, s3270 and
 *		tcl3270.
 */

#if defined(LIB3270)
	#include <lib3270/toggle.h>
#else
	#define MONOCASE	0
	#define ALT_CURSOR	1
	#define CURSOR_BLINK	2
	#define SHOW_TIMING	3
	#define CURSOR_POS	4

	#if defined(X3270_TRACE) /*[*/
	#define DS_TRACE	5
	#endif /*]*/

	#define SCROLL_BAR	6

	#if defined(X3270_ANSI) /*[*/
	#define LINE_WRAP	7
	#endif /*]*/

	#define BLANK_FILL	8

	#if defined(X3270_TRACE) /*[*/
	#define SCREEN_TRACE	9
	#define EVENT_TRACE	10
	#endif /*]*/

	#define MARGINED_PASTE	11
	#define RECTANGLE_SELECT 12

	#if defined(X3270_DISPLAY) /*[*/
	#define CROSSHAIR	13
	#define VISIBLE_CONTROL	14
	#endif /*]*/

	#if defined(X3270_SCRIPT) || defined(TCL3270) /*[*/
	#define AID_WAIT	15
	#endif /*]*/

	#define N_TOGGLES	16

#endif

/* Toggles */

enum toggle_type { TT_INITIAL, TT_INTERACTIVE, TT_ACTION, TT_FINAL };
struct toggle {
	char	value;		/* toggle value */
	char	changed;	/* has the value changed since init */
	Widget	w[2];		/* the menu item widgets */
	const char *label[2];	/* labels */
	void (*upcall)(struct toggle *, enum toggle_type); /* change value */

#if defined(LIB3270)
	void (*callback)(int value, int reason);
#endif

};

#define toggled(ix)		(appres.toggle[ix].value)
#define toggle_toggle(t) \
	{ (t)->value = !(t)->value; (t)->changed = True; }

/* Application resources */

typedef struct {
	/* Basic colors */
#if defined(X3270_DISPLAY) /*[*/
	Pixel	foreground;
	Pixel	background;
#endif /*]*/

	/* Options (not toggles) */
#if defined(X3270_DISPLAY) || (defined(C3270) && !defined(_WIN32)) /*[*/
	char mono;
#endif /*]*/
	char extended;
	char m3279;
	char modified_sel;
	char once;
#if defined(X3270_DISPLAY) /*[*/
	char visual_bell;
	char menubar;
	char active_icon;
	char label_icon;
	char invert_kpshift;
	char use_cursor_color;
	char allow_resize;
	char no_other;
	char do_confirms;
#if !defined(G3270)
	char reconnect;
#endif
	char visual_select;
	char suppress_host;
	char suppress_font_menu;
# if defined(X3270_KEYPAD) /*[*/
	char keypad_on;
# endif /*]*/
#endif /*]*/
#if defined(C3270) /*[*/
	char all_bold_on;
	char curses_keypad;
	char cbreak_mode;
#endif /*]*/
	char apl_mode;
	char scripted;
	char numeric_lock;
	char secure;
	char oerr_lock;
	char typeahead;
	char debug_tracing;
	char disconnect_clear;
	char highlight_bold;
	char color8;
	char bsd_tm;
	char unlock_delay;
#if defined(X3270_SCRIPT) /*[*/
	char socket;
#endif /*]*/
#if defined(C3270) && defined(_WIN32) /*[*/
	char highlight_underline;
#endif /*]*/

	/* Named resources */
#if defined(X3270_KEYPAD) /*[*/
	char	*keypad;
#endif /*]*/
#if defined(X3270_DISPLAY) || defined(C3270) /*[*/
	char	*key_map;
	char	*compose_map;
	char	*printer_lu;
#endif /*]*/
#if defined(X3270_DISPLAY) /*[*/
	char	*efontname;
	char	*fixed_size;
	char	*debug_font;
	char	*icon_font;
	char	*icon_label_font;
	int		save_lines;
	char	*normal_name;
	char	*select_name;
	char	*bold_name;
	char	*colorbg_name;
	char	*keypadbg_name;
	char	*selbg_name;
	char	*cursor_color_name;
	char	*color_scheme;
	int		bell_volume;
	char	*char_class;
	int		modified_sel_color;
	int		visual_select_color;
#if defined(X3270_DBCS) /*[*/
	char	*input_method;
	char	*preedit_type;
#endif /*]*/
#endif /*]*/
#if defined(X3270_DBCS) /*[*/
	char	*local_encoding;
#endif /*]*/
#if defined(C3270) /*[*/
	char	*meta_escape;
	char	*all_bold;
	char	*altscreen;
	char	*defscreen;
#endif /*]*/
	char	*conf_dir;
	char	*model;
	char	*hostsfile;
	char	*port;
	char	*charset;
	char	*termname;
	char	*login_macro;
	char	*macros;
#if defined(X3270_TRACE) /*[*/
#if !defined(_WIN32) /*[*/
	char	*trace_dir;
#endif /*]*/
	char	*trace_file;
	char	*screentrace_file;
	char	*trace_file_size;
#if defined(X3270_DISPLAY) || defined(WC3270) /*[*/
	char	trace_monitor;
#endif /*]*/
#endif /*]*/
	char	*oversize;
#if defined(X3270_FT) /*[*/
	char	*ft_command;
	int	dft_buffer_size;
#endif /*]*/
	char	*connectfile_name;
	char	*idle_command;
	char	idle_command_enabled;
	char	*idle_timeout;
#if defined(X3270_SCRIPT) /*[*/
	char	*plugin_command;
#endif /*]*/
#if defined(HAVE_LIBSSL) /*[*/
	char	*cert_file;
#endif /*]*/
	char	*proxy;

	/* Toggles */
	struct toggle toggle[N_TOGGLES];

#if defined(X3270_DISPLAY) /*[*/
	/* Simple widget resources */
	Cursor	normal_mcursor;
	Cursor	wait_mcursor;
	Cursor	locked_mcursor;
#endif /*]*/

#if defined(X3270_ANSI) /*[*/
	/* Line-mode TTY parameters */
	char	icrnl;
	char	inlcr;
	char	onlcr;
	char	*erase;
	char	*kill;
	char	*werase;
	char	*rprnt;
	char	*lnext;
	char	*intr;
	char	*quit;
	char	*eof;
#endif /*]*/

#if defined(WC3270) /*[*/
	char	*hostname;
#endif

#if defined(WC3270) /*[*/
	char	*title;
#endif /*]*/

#if defined(USE_APP_DEFAULTS) /*[*/
	/* App-defaults version */
	char	*ad_version;
#endif /*]*/

} AppRes, *AppResptr;

LIB3270_INTERNAL AppRes appres;

// FIXME (perry#2#): Check for right implementation
#if defined(LIB3270)
	#define _( x ) x
	#define N_( x ) x
	#define MSG_( c, s )	s
#else
	#define _( x ) x
	#define N_( x ) x
	#define MSG_( c, s )	get_message(c)
#endif
