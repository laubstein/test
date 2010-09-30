/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270.
 *
 * Copyright (C) <2008> <Banco do Brasil S.A.>
 *
 * Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
 * os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
 * Free Software Foundation.
 *
 * Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
 * GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
 * A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
 * obter mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
 * programa;  se  não, escreva para a Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA, 02111-1307, USA
 *
 * Este programa está nomeado como screen.c e possui 1301 linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 * licinio@bb.com.br		(Licínio Luis Branco)
 * kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
 * macmiranda@bb.com.br		(Marco Aurélio Caldas Miranda)
 *
 */


#include "gui.h"
#include "oia.h"
#include "fonts.h"
#include "actions1.h"

#include <lib3270/config.h>
#ifndef __APPLE__
	#include <malloc.h>
#endif
#include <string.h>
#include <errno.h>
// #include <lib3270/localdefs.h>
#include <lib3270/toggle.h>

#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE
	#include <sys/types.h>
	#include <signal.h>
#endif

#ifdef WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#include <stdlib.h>
#endif

/*---[ Structures ]----------------------------------------------------------------------------------------*/

/*---[ Prototipes ]----------------------------------------------------------------------------------------*/

 static void	  setsize(int rows, int cols);
 static int  	  addch(int row, int col, int c, unsigned short attr);
 static void	  set_charset(char *dcs);

 static void	  erase(void);
 static void	  display(void);

 static int	  SetSuspended(int state);
 static void	  SetScript(SCRIPT_STATE state);
 static void	  set_cursor(CURSOR_MODE mode);
 static void	  set_oia(OIA_FLAG id, int on);
 static void	  set_lu(const char *lu);
 static void	  changed(int bstart, int bend);
 static void	  error(const char *fmt, va_list arg);
 static void 	  warning(const char *fmt, va_list arg);
 static void	  syserror(const char *title, const char *message, const char *system);
 static int	  init(void);
 static void 	  update_toggle(int ix, int value, int reason, const char *name);
// static void	  show_timer(long seconds);
// static void	  DrawImage(GdkDrawable *drawable, GdkGC *gc, int id, int x, int y, int Width, int Height);
// static void 	  DrawImageByWidth(GdkDrawable *drawable, GdkGC *gc, int id, int x, int y, int Width, int Height);
 static gchar	* convert_monocase(int c, gsize *sz);
 static gchar	* convert_regular(int c, gsize *sz);
 static int	  popup_dialog(H3270 *session, PW3270_DIALOG type, const char *title, const char *msg, const char *fmt, va_list arg);

/*---[ Globals ]-------------------------------------------------------------------------------------------*/

 const struct lib3270_screen_callbacks program_screen_callbacks =
 {
	sizeof(struct lib3270_screen_callbacks),

	init,					// int (*init)(void);

	popup_dialog,			// int	(*popup_dialog)(H3270 *session, int type, const char *title, const char *msg, const char *fmt, va_list arg);
	error,					// void (*Error)(const char *fmt, va_list arg);
	warning,				// void (*Warning)(const char *fmt, va_list arg);
	syserror,				// void	(*SysError)(const char *title, const char *message, const char *system);

	setsize,				// void (*setsize)(int rows, int cols);
	addch,					// void (*addch)(int row, int col, int c, int attr);
	set_charset,			// void (*charset)(char *dcs);
	settitle,				// void (*title)(char *text);
	changed,				// void (*changed)(int bstart, int bend);
	gdk_beep,				// void (*ring_bell)(void);
	action_Redraw,			// void (*redraw)(void);
	update_cursor_position,	// void (*move_cursor)(int row, int col);
	SetSuspended,			// int	(*set_suspended)(int state);
	SetScript,				// void	(*set_script)(SCRIPT_STATE state);
	NULL,					// void (*reset)(int lock);
	SetStatusCode,			// void (*status)(STATUS_CODE id);
	set_cursor,				// void (*cursor)(CURSOR_MODE mode);
	set_lu,					// void (*lu)(const char *lu);
	set_oia,				// void (*set)(OIA_FLAG id, int on);

	erase,					// void (*erase)(void);
	display,				// void	(*display)(int bstart, int bend);
	update_toggle,			// void (*toggle_changed)(int ix, int value, int reason, const char *name);
	oia_set_timer,			// void	(*show_timer)(long seconds);

	gui_console_window_new,						// HCONSOLE	(*console_new)(const char *title, const char *label);
	gui_console_window_delete,					// void		(*console_delete)(HCONSOLE hwnd);
	gui_console_window_append,					// int		(*console_append)(HCONSOLE hwnd, const char *fmt, ...);
	gui_console_window_wait_for_user_entry,		// char *	(*console_entry)(HCONSOLE hwnd);

 };

/*
 static const struct _imagedata
 {
	const unsigned char	*data;
    gint					width;
    gint					height;
    short					color;
 } imagedata[] =
 {
 	{ locked_bits,		locked_width,	locked_height,   	TERMINAL_COLOR_SSL 		},
 	{ unlocked_bits,	unlocked_width,	unlocked_height, 	TERMINAL_COLOR_SSL 		},
 	{ shift_bits,		shift_width, 	shift_height,		TERMINAL_COLOR_OIA 		},
 	{ four_bits,		four_width, 	four_height,		TERMINAL_COLOR_OIA 		},
 };

 #define IMAGE_COUNT (sizeof(imagedata)/sizeof(struct _imagedata))

 static struct _pix
 {
 	GdkPixbuf *base;
 	GdkPixbuf *pix;
 	int		   Width;
 	int		   Height;
 } pix[IMAGE_COUNT];
*/

 int 			  terminal_rows						= 0;
 int 			  terminal_cols						= 0;
 int			  left_margin						= 0;
 int			  top_margin						= 0;

 ELEMENT		* screen							= NULL;
 char			* charset							= NULL;
 char			* window_title						= PROGRAM_NAME;

 gboolean		  screen_updates_enabled			= FALSE;
 int			  terminal_buffer_length			= 0;

 static gchar 	* (*convert_charset)(int c, gsize *sz) = convert_regular;

/*---[ Implement ]-----------------------------------------------------------------------------------------*/

 static void changed(int bstart, int bend)
 {
 }

 static int SetSuspended(int state)
 {
 	gboolean enabled = (state == 0);

	if(enabled == screen_updates_enabled)
		return 0;

	screen_updates_enabled = enabled;

 	Trace("%s(%d): screen updates are %s",__FUNCTION__,state,screen_updates_enabled ? "enabled" : "disabled");

	if(screen_updates_enabled)
 	{
 		if(valid_terminal_window())
 		{
			update_cursor_info();
			update_terminal_contents();
 		}
 		gtk_widget_queue_draw(terminal);
 	}

 	return TRUE;
 }


 void settitle(char *text)
 {
 	gchar buffer[80];

 	if(text && *text)
 		g_snprintf(buffer,79,"%s - %s",window_title,text);
	else
 		g_snprintf(buffer,79,"%s",window_title);

 	if(topwindow)
		gtk_window_set_title(GTK_WINDOW(topwindow),buffer);

 }

 void ParseInput(const gchar *string)
 {
    gchar *input = g_convert(string, -1, CHARSET, "UTF-8", NULL, NULL, NULL);

    if(!input)
    {
        Log("Error converting string \"%s\" to %s",string,CHARSET);
        return;
    }

    // NOTE (perry#1#): Is it the best way?
    Input_String((const unsigned char *) input);

    g_free(input);
 }

 static gchar * convert_monocase(int c, gsize *sz)
 {
	gchar in[2]	= { (char) c, 0 };
	gchar *ptr	= g_convert(in, -1, "UTF-8", CHARSET, NULL, sz, NULL);
	gchar *ret;

	if(!ptr)
		return ptr;

	ret = g_utf8_strup(ptr,*sz);

	g_free(ptr);

	return ret;
 }

 static gchar * convert_regular(int c, gsize *sz)
 {
	gchar in[2] = { (char) c, 0 };
 	return g_convert(in, -1, "UTF-8", CHARSET, NULL, sz, NULL);
 }

 void set_monocase(int value, enum toggle_type reason)
 {
	convert_charset = value ? convert_monocase : convert_regular;
	screen_disp();
 }

 static void convert_cg(ELEMENT *el, int c)
 {
 	static const struct _xlat
 	{
 		int c;
 		const gchar *str;
 	} xlat[] =
 	{
		{ 0x8c,	"≤" },	// CG 0xf7, less or equal
		{ 0xae, "≥" },	// CG 0xd9, greater or equal
		{ 0xbe, "≠" },	// CG 0x3e, not equal
//		{ 0xa3, " " },	// CG 0x93, bullet
		{ 0xad, "["	},
		{ 0xbd, "]" }
 	};

 	int f;

 	for(f=0;f < G_N_ELEMENTS(xlat); f++)
 	{
		if(xlat[f].c == c)
		{
//			Trace("%s \"%s\" %d",__FUNCTION__,xlat[f].str,strlen(xlat[f].str));
			strcpy(el->ch,xlat[f].str);
			return;
		}
 	}

	el->cg	= (unsigned short) c;
	*el->ch	= ' ';

 }

 static int addch(int row, int col, int c, unsigned short attr)
 {

 	ELEMENT in;
 	ELEMENT *el;
 	int		baddr = (row*terminal_cols)+col;

 	if(!screen || col >= terminal_cols || row >= terminal_rows)
		return EINVAL;

	if(baddr > terminal_buffer_length)
		return EFAULT;

	memset(&in,0,sizeof(in));

	if(c)
	{
		if(attr & CHAR_ATTR_CG)
		{
			convert_cg(&in,c);
		}
		else
		{
			gsize sz = 1;
			gchar *str = convert_charset(c,&sz);

			if(sz < MAX_CHR_LENGTH)
				memcpy(in.ch,str,sz);
			else
				Log("Invalid size when converting \"%c\" to \"%s\"",c,str);
			g_free(str);

		}
	}

	in.bg = (attr & 0xF0) >> 4;

	if(attr & COLOR_ATTR_FIELD)
		in.fg = (attr & 0x03)+TERMINAL_COLOR_FIELD;
	else
		in.fg = (attr & 0x0F);

	if(attr & COLOR_ATTR_UNDERLINE)
		in.fg |= COLOR_ATTR_UNDERLINE;

	// Get element entry in the buffer, update ONLY if changed
 	el = screen + baddr;

	in.status = el->status;

	if(attr & CHAR_ATTR_MARKER)
		in.status |= ELEMENT_STATUS_FIELD_MARKER;
	else
		in.status &= ~ELEMENT_STATUS_FIELD_MARKER;

	if(el->changed || !memcmp(&in,el,sizeof(ELEMENT)))
		return 0;

	memcpy(el,&in,sizeof(ELEMENT));
	el->changed = TRUE;

	return 0;
 }

 static void setsize(int rows, int cols)
 {
	g_free(screen);
	screen = NULL;

	if(rows && cols)
	{
		terminal_buffer_length = rows*cols;
		screen = g_new0(ELEMENT,terminal_buffer_length);
		terminal_rows = rows;
		terminal_cols = cols;

		if(terminal && terminal->window)
			action_Redraw();

	}

// 	Trace("Terminal set to %d rows with %d cols, screen set to %p",rows,cols,screen);

 }

 /**
  * Erase screen.
  *
  */
 static void erase(void)
 {
 	int f;

	Trace("Erasing screen! (pixmap: %p screen: %p terminal: %p valid: %s)",get_terminal_pixmap(),screen,terminal, valid_terminal_window() ? "Yes" : "No");

	if(screen)
	{
		unsigned char status;

		for(f=0;f<terminal_buffer_length;f++)
		{
			status				= screen[f].status & ~ELEMENT_STATUS_FIELD_MARKER;
			memset(screen+f,0,sizeof(ELEMENT));
			screen[f].ch[0]		= ' ';
			screen[f].status	= status;
		}
	}

	if(valid_terminal_window() && screen_updates_enabled)
	{
		int 	width  = terminal_cols * fontWidth;
		int 	height = terminal_rows * terminal_font_info.spacing;
		cairo_t *cr	= get_terminal_cairo_context();

		gdk_cairo_set_source_color(cr,color+TERMINAL_COLOR_BACKGROUND);
		cairo_rectangle(cr, left_margin, top_margin, width, height);
		cairo_fill(cr);

		cairo_destroy(cr);
		gtk_widget_queue_draw_area(terminal,left_margin,top_margin,width,height);
	}

	if(get_cursor_pixmap())
	{
		gdk_pixmap_unref(get_cursor_pixmap());
		pixmap_cursor = NULL;
	}

 }

 static GPid on_lu_pid = 0;

 static void set_lu(const char *lu)
 {
 	gchar *luname = NULL;

	update_oia_element(OIA_ELEMENT_LUNAME);

 	if(lu)
 	{
		luname = g_convert(lu, -1, "UTF-8", CHARSET, NULL, NULL, NULL);

		if(on_lu_command && !on_lu_pid)
		{
			if(run_script_command_line(on_lu_command, &on_lu_pid))
			{
				on_lu_pid = 0;
			}
		}
 	}
 	else if(on_lu_pid)
 	{
#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE

		Trace("Sending SIGTERM do pid %d",(int) on_lu_pid);

		if(kill( (pid_t) on_lu_pid, SIGTERM) < 0)
		{
			GtkWidget *dialog;

			// Can't kill process, notify user
			dialog = gtk_message_dialog_new(	GTK_WINDOW(topwindow),
												GTK_DIALOG_DESTROY_WITH_PARENT,
												GTK_MESSAGE_WARNING,
												GTK_BUTTONS_OK,
												_(  "Can't stop LU association script" ));

			gtk_window_set_title(GTK_WINDOW(dialog), _( "Can't stop script" ) );
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog) ,"%s", strerror(errno));

			gtk_dialog_run(GTK_DIALOG (dialog));
			gtk_widget_destroy(dialog);

		}
#elif defined( WIN32 )

		if(!TerminateProcess((HANDLE) on_lu_pid, -1))
		{
			GtkWidget *dialog;

			// Can't kill process, notify user
			dialog = gtk_message_dialog_new(	GTK_WINDOW(topwindow),
												GTK_DIALOG_DESTROY_WITH_PARENT,
												GTK_MESSAGE_WARNING,
												GTK_BUTTONS_OK,
												_(  "Can't stop LU association script" ));

			gtk_window_set_title(GTK_WINDOW(dialog), _( "Can't stop script" ) );

			gtk_dialog_run(GTK_DIALOG (dialog));
			gtk_widget_destroy(dialog);
		}

#endif

 	}

	CallPlugins("pw3270_plugin_update_luname",luname);

	if(luname)
		g_free(luname);
 }

 static void set_oia(OIA_FLAG id, int on)
 {
 	if(id > OIA_FLAG_USER)
		return;

	oia_flag[id] = on;

	switch(id)
	{
	case OIA_FLAG_BOXSOLID:
		update_oia_element(OIA_ELEMENT_CONNECTION_STATUS);
		break;

	case OIA_FLAG_UNDERA:
		update_oia_element(OIA_ELEMENT_UNDERA);
		break;

	case OIA_FLAG_SECURE:
		update_oia_element(OIA_ELEMENT_SSL_STATE);
		break;

	case OIA_FLAG_TYPEAHEAD:
		update_oia_element(OIA_ELEMENT_TYPEAHEAD_INDICATOR);
		break;

#ifdef OIA_ELEMENT_PRINTER_INDICATOR
	case OIA_FLAG_PRINTER:
		update_oia_element(OIA_ELEMENT_PRINTER_INDICATOR);
		break;
#endif //  OIA_ELEMENT_PRINTER_INDICATOR

#ifdef OIA_ELEMENT_REVERSE_INPUT_INDICATOR
	case OIA_FLAG_REVERSE:
		update_oia_element(OIA_ELEMENT_REVERSE_INPUT_INDICATOR);
		break;
#endif // OIA_ELEMENT_REVERSE_INPUT_INDICATOR

	default:

		// Unexpected flag, update all OIA.
		update_oia();
	}
 }

 static void set_charset(char *dcs)
 {
 	Trace("Screen charset: %s",dcs);
	g_free(charset);
	charset = g_strdup(dcs);
 }

 void SetStatusCode(STATUS_CODE id)
 {
 	if(id == terminal_message_id)
		return;

	terminal_message_id = id;

	if(id == STATUS_CODE_BLANK)
	{
		set_cursor(CURSOR_MODE_NORMAL);
		update_cursor_pixmap();
	}
	else if(id >= STATUS_CODE_USER)
	{
		Log("Unexpected status code %d",(int) id);
		return;
	}

	update_oia_element(OIA_ELEMENT_MESSAGE_AREA);

 }


 CURSOR_MODE cursor_mode = -1;

 static void set_cursor(CURSOR_MODE mode)
 {
 	if(mode == cursor_mode || mode > CURSOR_MODE_USER || !terminal || !terminal->window)
		return;

	cursor_mode = mode;

#ifdef MOUSE_POINTER_CHANGE
	if(drag_type == DRAG_TYPE_NONE)
		gdk_window_set_cursor(terminal->window,wCursor[cursor_mode]);
#endif

 }

 gchar * GetScreenContents(gboolean all)
 {
 	gsize	max = terminal_rows*terminal_cols*MAX_CHR_LENGTH;
 	GString	*str = g_string_sized_new(max);
 	int		row,col;
 	int		pos	= 0;

	for(row = 0; row < terminal_rows;row++)
	{
		gchar		line[max];

		*line = 0;
		for(col = 0; col < terminal_cols;col++)
		{
			if(all || (screen[pos].status & ELEMENT_STATUS_SELECTED))
			{
				if(!*line)
				{
					if(*str->str)
						g_string_append_c(str,'\n');
				}
				g_strlcat(line,*screen[pos].ch ? screen[pos].ch : " ",max);
			}
			pos++;
		}

		if(*line)
			g_string_append(str,g_strchomp(line));

	}

	Trace("Screen-read:\n%s<---\n",str->str);

	return g_string_free(str,FALSE);
 }

 static void syserror(const char *title, const char *message, const char *system)
 {
 	GtkWidget 	*dialog = gtk_message_dialog_new(	GTK_WINDOW(topwindow),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_CLOSE,
													"%s",gettext( message ) );

 	g_warning("%s",message);

	if(title)
		gtk_window_set_title(GTK_WINDOW(dialog), gettext(title));
	else
		gtk_window_set_title(GTK_WINDOW(dialog), _( "System Error" ) );

	if(system)
	{
		// Reencode and show secondary message
#ifdef WIN32
		gsize	bytes_written;
		gchar	*ptr = g_locale_to_utf8(system, -1, NULL, &bytes_written, NULL);

		if(ptr)
		{
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), ptr);
			g_free(ptr);
		}
		else
		{
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", system);
		}
#else
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", system);
#endif
	}

	gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);

 }

 static void warning(const char *fmt, va_list arg)
 {
 	gchar		*msg	= g_strdup_vprintf(gettext(fmt),arg);
 	GtkWidget 	*dialog;

 	g_warning("%s",msg);

	if(!topwindow)
	{
		g_free(msg);
		return;
	}

 	dialog = gtk_message_dialog_new(	GTK_WINDOW(topwindow),
										GTK_DIALOG_DESTROY_WITH_PARENT,
										GTK_MESSAGE_WARNING,
										GTK_BUTTONS_CLOSE,
										"%s",msg );

	Trace("msg: %s dialog: %p topwindow: %p",msg,dialog,topwindow);

	gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);

	g_free(msg);
 }

 static int popup_dialog(H3270 *session, PW3270_DIALOG type, const char *title, const char *msg, const char *fmt, va_list arg)
 {
 	static const struct _tbl
 	{
 		const gchar	* title;
 		GtkMessageType	  dialog_type;
 		GtkButtonsType 	  dialog_buttons;
 	} tbl[] =
 	{
 		{ N_( "Information" ),	GTK_MESSAGE_INFO,	GTK_BUTTONS_CLOSE 		},
 		{ N_( "Error" ),		GTK_MESSAGE_ERROR, 	GTK_BUTTONS_OK			},
 	};

 	GtkWidget		*dialog;
 	gchar			*secondary		= NULL;
 	GtkResponseType	 response;

 	if(arg && fmt)
		secondary = g_strdup_vprintf(gettext(fmt),arg);

	if(type > G_N_ELEMENTS(tbl))
		type = 0;

	dialog = gtk_message_dialog_new_with_markup(	GTK_WINDOW(topwindow),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													tbl[type].dialog_type,
													tbl[type].dialog_buttons,
													"%s",msg );

	gtk_window_set_title(GTK_WINDOW(dialog), title ? title : tbl[type].title);

	if(secondary)
	{
		gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog),"%s",secondary);
		g_free(secondary);
	}

	if(type == PW3270_DIALOG_CRITICAL)
	{
		// Insert button to close program (Usefull in openSSL message errors
		gtk_dialog_add_button(GTK_DIALOG(dialog),_( "Abort" ), 10);
	}

	response = gtk_dialog_run(GTK_DIALOG (dialog));

	gtk_widget_destroy(dialog);

	if(response == 10)
	{
		Log("Aborting by user action in error dialog \"%s\"",msg);
		gtk_main_quit();
	}

	return (int) response;
 }


 static void error(const char *fmt, va_list arg)
 {
 	gchar		*msg	= g_strdup_vprintf(gettext(fmt),arg);
 	GtkWidget 	*dialog = gtk_message_dialog_new(	GTK_WINDOW(topwindow),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_CLOSE,
													"%s",msg );

 	g_warning("%s",msg);

	gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);

	g_free(msg);
 }

 static int init(void)
 {
	return 0;
 }

 void update_toggle(int ix, int value, int reason, const char *name)
 {
 	GtkAction	*action;
 	gchar		*ptr;

	ptr = g_strconcat("Toggle",name,NULL);
	action = get_action_by_name(ptr);
	g_free(ptr);

	// Update toggle buttons
	if(action)
		gtk_toggle_action_set_active((GtkToggleAction *) action,value);

	// Update toolbar items
	ptr = g_strconcat("ToggleReset",name,NULL);
	action = get_action_by_name(ptr);
	g_free(ptr);

	if(action)
		gtk_action_set_visible(action,value ? TRUE : FALSE);

	ptr = g_strconcat("ToggleSet",name,NULL);
	action = get_action_by_name(ptr);
	g_free(ptr);
	if(action)
		gtk_action_set_visible(action,value ? FALSE : TRUE);

 }

 static gboolean script_timer(gpointer dunno)
 {
	oia_script_blink = !oia_script_blink;
 	update_oia_element(OIA_ELEMENT_SCRIPT_INDICATOR);
 	return oia_script_state != SCRIPT_STATE_NONE;
 }

 static void SetScript(SCRIPT_STATE state)
 {
 	if(oia_script_state == state)
		return;

	if(oia_script_state == SCRIPT_STATE_NONE)
		g_timeout_add((guint) 500, (GSourceFunc) script_timer, 0);

	oia_script_state = state;
	oia_script_blink = TRUE;
 	update_oia_element(OIA_ELEMENT_SCRIPT_INDICATOR);

 }

 static void display(void)
 {
	if(valid_terminal_window() && screen_updates_enabled)
	{
		int		baddr   =  0;
		int		row;
		int		col;
		cairo_t *cr	= get_terminal_cairo_context();

		for(row = 0; row < terminal_rows; row++)
		{
			int		cstart	= -1;
			int 	bstart	= -1;
			int 	bend	= -1;
//#ifdef DEBUG
//			int 	y 		= top_margin+(row * terminal_font_info.spacing);
//#endif

			for(col = 0;col < terminal_cols;col++)
			{
				if(screen[baddr].changed)
				{
					if(bstart < 0)
					{
						bstart = baddr;
						cstart = col;
					}
					bend = baddr;
					screen[baddr].changed = FALSE;

					if(baddr == cursor_position)
						update_cursor_pixmap();

				}
				else if(bstart >= 0)
				{
					GdkRectangle r;
					draw_region(cr,bstart,bend,color,&r);
					gdk_window_invalidate_rect(terminal->window,&r,FALSE);

//					#ifdef DEBUG
//						if(r.x != left_margin+(cstart*fontWidth) || r.y != y || r.width != ((bend-bstart)+1)*fontWidth || r.height != terminal_font_info.spacing)
//						{
//							Trace("%s(%d,%d %d,%d->%d,%d row=%d (%d)) - Unexpected size returned from draw_region",
//													__FUNCTION__,
//													bstart,
//													bend,
//													bstart%terminal_cols,bstart/terminal_cols,
//													bend%terminal_cols,bend/terminal_cols,
//													row,(row*terminal_font_info.spacing)+top_margin);
//							Trace("Rect: %d,%d->%d,%d",r.x,r.y,r.width,r.height);
//							Trace("Area: %d,%d->%d,%d",left_margin+(cstart*fontWidth),y,((bend-bstart)+1)*fontWidth,terminal_font_info.spacing);
//						}
//					#endif

//					gtk_widget_queue_draw_area(terminal,left_margin+(cstart*fontWidth),y,((bend-bstart)+1)*fontWidth,terminal_font_info.spacing);

					bstart = bend = -1;
				}
				baddr++;
			}

			if(bstart >= 0)
			{
				GdkRectangle r;
				draw_region(cr,bstart,bend,color,&r);
				gdk_window_invalidate_rect(terminal->window,&r,FALSE);

//				#ifdef DEBUG
//					if(r.x != left_margin+(cstart*fontWidth) || r.y != y || r.width != ((bend-bstart)+1)*fontWidth || r.height != terminal_font_info.spacing)
//					{
//						Trace("%s(%d,%d) - Unexpected size returned from draw_region",__FUNCTION__,bstart,bend);
//						Trace("Rect: %d,%d->%d,%d",r.x,r.y,r.width,r.height);
//						Trace("Area: %d,%d->%d,%d",left_margin+(cstart*fontWidth),y,((bend-bstart)+1)*fontWidth,terminal_font_info.spacing);
//					}
//				#endif

//				gtk_widget_queue_draw_area(terminal,left_margin+(cstart*fontWidth),y,((bend-bstart)+1)*fontWidth,terminal_font_info.spacing);
			}
		}

		cairo_destroy(cr);

		gdk_window_process_updates(terminal->window,FALSE);

	}
 }
