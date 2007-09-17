/*
 * Copyright 1999, 2000, 2002 by Paul Mattes.
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for any purpose and without fee is hereby granted,
 *  provided that the above copyright notice appear in all copies and that
 *  both that copyright notice and this permission notice appear in
 *  supporting documentation.
 *
 * c3270 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the file LICENSE for more details.
 */

/* c3270 version of screenc.h */

#include "lib3270.h"

#define blink_start()
#define display_heightMM()	100
#define display_height()	1
#define display_widthMM()	100
#define display_width()		1

#define mcursor_normal()	screen_set_cursor(0)
#define mcursor_locked()	screen_set_cursor(1)
#define mcursor_waiting()	screen_set_cursor(2)

#define screen_obscured()	False
#define screen_scroll()

extern void cursor_move(int baddr);
extern void ring_bell(void);
extern void screen_132(void);
extern void screen_80(void);
extern void screen_disp(Boolean erasing);
extern void screen_init(void);
extern void screen_flip(void);
extern void screen_resume(void);
extern void screen_suspend(void);
extern void screen_set_cursor(int mode);
extern void toggle_monocase(struct toggle *t, enum toggle_type tt);

extern FILE *start_pager(void);

extern Boolean escaped;

extern void Escape_action(Widget w, XEvent *event, String *params,
    Cardinal *num_params);
extern void Help_action(Widget w, XEvent *event, String *params,
    Cardinal *num_params);
extern void Redraw_action(Widget w, XEvent *event, String *params,
    Cardinal *num_params);
extern void Trace_action(Widget w, XEvent *event, String *params,
    Cardinal *num_params);
extern void Show_action(Widget w, XEvent *event, String *params,
    Cardinal *num_params);

