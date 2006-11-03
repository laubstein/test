
#include <gdk/gdkkeysyms.h>
#include "g3270.h"

/*---[ Prototipes ]-----------------------------------------------------------*/

static void screen_init(void);
static void screen_disp(Boolean erasing, const struct ea *display);
static void screen_suspend(void);

static void screen_resume(void);
static void screen_type(const char *model_name, int maxROWS, int maxCOLS);

static void cursor_move(int baddr);
static void toggle_monocase(struct toggle *t, enum toggle_type tt);

static void status_ctlr_done(void);
static void status_insert_mode(Boolean on);
static void status_minus(void);
static void status_oerr(int error_type);
static void status_reset(void);
static void status_reverse_mode(Boolean on);
static void status_syswait(void);
static void status_twait(void);
static void status_typeahead(Boolean on);
static void status_compose(Boolean on, unsigned char c, enum keytype keytype);
static void status_lu(const char *lu);
static void ring_bell(void);
static void screen_flip(void);
static void screen_width(int width);
static void error_popup(const char *msg);
static void Redraw_action(Widget w, XEvent *event, String *params, Cardinal *num_params);

static void InputAdded(const INPUT_3270 *ip);
static void InputRemoved(const INPUT_3270 *ip);

/*---[ 3270 Screen callback table ]-------------------------------------------*/

const SCREEN_CALLBACK g3270_screen_callbacks =
{
    sizeof(SCREEN_CALLBACK),
	SCREEN_MAGIC,

    screen_init,
    screen_disp,
    screen_suspend,

	screen_resume,
	screen_type,

	cursor_move,
	toggle_monocase,

	status_ctlr_done,
	status_insert_mode,
	status_minus,
	status_oerr,
	status_reset,
	status_reverse_mode,
	status_syswait,
	status_twait,
	status_typeahead,
	status_compose,
	status_lu,

	ring_bell,
	screen_flip,
	screen_width,

	error_popup,

	Redraw_action
};

/*---[ Keyboard tables ]------------------------------------------------------*/

 static const KEYTABLE keys[] =
 {
//	{ "BREAK",	KEY_BREAK },
	{ "DOWN",		GDK_Down 		},
	{ "UP",			GDK_Up 			},
	{ "LEFT",		GDK_Left		},
	{ "RIGHT",		GDK_Right 		},
//	{ "HOME",	KEY_HOME },
	{ "BACKSPACE",	GDK_BackSpace	},
//	{ "F0",		KEY_F0 },
//	{ "DL",		KEY_DL },
//	{ "IL",		KEY_IL },
//	{ "DC",		KEY_DC },
//	{ "IC",		KEY_IC },
//	{ "EIC",	KEY_EIC },
//	{ "CLEAR",	KEY_CLEAR },
//	{ "EOS",	KEY_EOS },
//	{ "EOL",	KEY_EOL },
//	{ "SF",		KEY_SF },
//	{ "SR",		KEY_SR },
//	{ "NPAGE",	KEY_NPAGE },
//	{ "PPAGE",	KEY_PPAGE },
//	{ "STAB",	KEY_STAB },
//	{ "CTAB",	KEY_CTAB },
//	{ "CATAB",	KEY_CATAB },
//	{ "ENTER",	KEY_ENTER },
//	{ "SRESET",	KEY_SRESET },
//	{ "RESET",	KEY_RESET },
//	{ "PRINT",	KEY_PRINT },
//	{ "LL",		KEY_LL },
//	{ "A1",		KEY_A1 },
//	{ "A3",		KEY_A3 },
//	{ "B2",		KEY_B2 },
//	{ "C1",		KEY_C1 },
//	{ "C3",		KEY_C3 },
//	{ "BTAB",	KEY_BTAB },
//	{ "BEG",	KEY_BEG },
//	{ "CANCEL",	KEY_CANCEL },
//	{ "CLOSE",	KEY_CLOSE },
//	{ "COMMAND",	KEY_COMMAND },
//	{ "COPY",	KEY_COPY },
//	{ "CREATE",	KEY_CREATE },
//	{ "END",	KEY_END },
//	{ "EXIT",	KEY_EXIT },
//	{ "FIND",	KEY_FIND },
//	{ "HELP",	KEY_HELP },
//	{ "MARK",	KEY_MARK },
//	{ "MESSAGE",	KEY_MESSAGE },
//	{ "MOVE",	KEY_MOVE },
//	{ "NEXT",	KEY_NEXT },
//	{ "OPEN",	KEY_OPEN },
//	{ "OPTIONS",	KEY_OPTIONS },
//	{ "PREVIOUS",	KEY_PREVIOUS },
//	{ "REDO",	KEY_REDO },
//	{ "REFERENCE",	KEY_REFERENCE },
//	{ "REFRESH",	KEY_REFRESH },
//	{ "REPLACE",	KEY_REPLACE },
//	{ "RESTART",	KEY_RESTART },
//	{ "RESUME",	KEY_RESUME },
//	{ "SAVE",	KEY_SAVE },
//	{ "SBEG",	KEY_SBEG },
//	{ "SCANCEL",	KEY_SCANCEL },
//	{ "SCOMMAND",	KEY_SCOMMAND },
//	{ "SCOPY",	KEY_SCOPY },
//	{ "SCREATE",	KEY_SCREATE },
//	{ "SDC",	KEY_SDC },
//	{ "SDL",	KEY_SDL },
//	{ "SELECT",	KEY_SELECT },
//	{ "SEND",	KEY_SEND },
//	{ "SEOL",	KEY_SEOL },
//	{ "SEXIT",	KEY_SEXIT },
//	{ "SFIND",	KEY_SFIND },
//	{ "SHELP",	KEY_SHELP },
//	{ "SHOME",	KEY_SHOME },
//	{ "SIC",	KEY_SIC },
//	{ "SLEFT",	KEY_SLEFT },
//	{ "SMESSAGE",	KEY_SMESSAGE },
//	{ "SMOVE",	KEY_SMOVE },
//	{ "SNEXT",	KEY_SNEXT },
//	{ "SOPTIONS",	KEY_SOPTIONS },
//	{ "SPREVIOUS",	KEY_SPREVIOUS },
//	{ "SPRINT",	KEY_SPRINT },
//	{ "SREDO",	KEY_SREDO },
//	{ "SREPLACE",	KEY_SREPLACE },
//	{ "SRIGHT",	KEY_SRIGHT },
//	{ "SRSUME",	KEY_SRSUME },
//	{ "SSAVE",	KEY_SSAVE },
//	{ "SSUSPEND",	KEY_SSUSPEND },
//	{ "SUNDO",	KEY_SUNDO },
//	{ "SUSPEND",	KEY_SUSPEND },
//	{ "UNDO",	KEY_UNDO },
	{ CN, 0 }

 };

 const KEYBOARD_INFO g3270_keyboard_info =
 {
	sizeof(KEYBOARD_INFO),
	KEYBOARD_MAGIC,

	ring_bell,
	InputAdded,
	InputRemoved,

	keys

 };

/*---[ Implement screen callbacks ]-------------------------------------------*/

static void screen_init(void)
{
}

static void screen_disp(Boolean erasing, const struct ea *display)
{
}

static void screen_suspend(void)
{
}

static void screen_resume(void)
{
}

static void screen_type(const char *model_name, int maxROWS, int maxCOLS)
{
}

static void cursor_move(int baddr)
{
}

static void toggle_monocase(struct toggle *t, enum toggle_type tt)
{
}

static void status_ctlr_done(void)
{
}

static void status_insert_mode(Boolean on)
{
}

static void status_minus(void)
{
}

static void status_oerr(int error_type)
{
}

static void status_reset(void)
{
}

static void status_reverse_mode(Boolean on)
{
}

static void status_syswait(void)
{
}

static void status_twait(void)
{
}

static void status_typeahead(Boolean on)
{
}

static void status_compose(Boolean on, unsigned char c, enum keytype keytype)
{
}

static void status_lu(const char *lu)
{
}

static void ring_bell(void)
{
}

static void screen_flip(void)
{
}

static void screen_width(int width)
{
}

static void error_popup(const char *msg)
{
}

static void Redraw_action(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
}

static void InputAdded(const INPUT_3270 *ip)
{
   // http://developer.gnome.org/doc/API/glib/glib-the-main-event-loop.html#G-MAIN-ADD-POLL

/*
struct GPollFD
{
  gint		fd;
  gushort 	events;
  gushort 	revents;
};
*/
	DBGPrintf("Input Source %p added",ip);
}

static void InputRemoved(const INPUT_3270 *ip)
{
	DBGPrintf("Input Source %p removed",ip);
}

