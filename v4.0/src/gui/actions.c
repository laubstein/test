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
 * Este programa está nomeado como actions.c e possui 1088 linhas de código.
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

 #include <lib3270/config.h>

 #include "gui.h"
 #include <gdk/gdkkeysyms.h>
 #include <errno.h>

 #include <globals.h>

 #include <lib3270/kybdc.h>
 #include <lib3270/actionsc.h>
 #include <lib3270/toggle.h>
 #include <lib3270/plugins.h>

 #ifndef GDK_NUMLOCK_MASK
	#define GDK_NUMLOCK_MASK GDK_MOD2_MASK
 #endif

/*---[ Prototipes ]---------------------------------------------------------------------------------------------*/

// static void action_Clear(GtkWidget *w, gpointer user_data);

 static void erase_input_action(void);
 static void clear_fields_action(void);

 static void action_Up(GtkWidget *w, gpointer user_data);
 static void action_Down(GtkWidget *w, gpointer user_data);
 static void action_Left(GtkWidget *w, gpointer user_data);
 static void action_Right(GtkWidget *w, gpointer user_data);
 static void action_Tab(void);
 static void action_BackTab(void);
 static void action_enter(GtkWidget *w, gpointer user_data);
 static void action_Disconnect(GtkWidget *w, gpointer user_data);
 static void action_PrintScreen(GtkWidget *w, gpointer user_data);
 static void action_PrintSelected(GtkWidget *w, gpointer user_data);
 static void action_PrintClipboard(GtkWidget *w, gpointer user_data);
 static void action_Quit(void);
 static void action_About(GtkWidget *w, gpointer user_data);

 static void action_ToggleGDKDebug(GtkToggleAction *action, gpointer user_data);
 static void action_SaveScreen(void);
 static void action_SaveSelected(void);
 static void action_SaveClipboard(void);
 static void action_DumpScreen(void);
 static void action_LoadScreenDump(void);

/*---[ Gui toggles ]--------------------------------------------------------------------------------------------*/

 static const struct _gui_toggle_info
 {
    const gchar     *name;
    const gchar     *label;
    const gchar     *tooltip;
    const gchar     *stock_id;
    const gchar     *accelerator;
    const gboolean  def;
 } gui_toggle_info[GUI_TOGGLE_COUNT] =
 {
    { "Bold", 			    N_( "Bold" ),			    NULL,	NULL,	NULL, FALSE	},
    { "KeepSelected", 	    N_( "Keep selected" ),	    NULL,	NULL,	NULL, FALSE	},
    { "Underline",		    N_( "Show Underline" ), 	NULL,   NULL,   NULL, FALSE },
    { "AutoConnect",    	N_( "Connect on startup" ),	NULL,   NULL,   NULL, TRUE  }
 };

 gboolean gui_toggle[GUI_TOGGLE_COUNT] = { 0 };

/*---[ Keyboard action ]----------------------------------------------------------------------------------------*/

 struct WindowActions
 {
	guint			keyval;
	GdkModifierType	state;
	void (*callback)(guint arg);
	guint			arg;
 };

#ifdef CONFIGURABLE_KEYBOARD_ACTIONS

	static struct WindowActions	*keyproc_table 		= NULL;
	static int 					keyproc_table_size	= 0;

#else

	#define INTERNAL_ACTION(key,state,action) { key, state, (void (*)(guint)) action, 0 }

 	static const struct WindowActions keyproc_table[] =
 	{
		INTERNAL_ACTION(	GDK_Left,			0,					action_Left),
		INTERNAL_ACTION(	GDK_Up,				0,					action_Up),
		INTERNAL_ACTION(	GDK_Right,			0,					action_Right),
		INTERNAL_ACTION(	GDK_Down,			0,					action_Down),

		INTERNAL_ACTION(	GDK_KP_Left,		0,					action_Left),
		INTERNAL_ACTION(	GDK_KP_Up,			0,					action_Up),
		INTERNAL_ACTION(	GDK_KP_Right,		0,					action_Right),
		INTERNAL_ACTION(	GDK_KP_Down,		0,					action_Down),

		INTERNAL_ACTION(	GDK_ISO_Left_Tab,	0,					action_BackTab),
		INTERNAL_ACTION(	GDK_Tab,			0,					action_Tab),
		INTERNAL_ACTION(	GDK_KP_Add,			GDK_NUMLOCK_MASK,	action_Tab),

 	};

 	static const int keyproc_table_size = (sizeof(keyproc_table)/sizeof(struct WindowActions));


#endif

/*---[ Action tables ]------------------------------------------------------------------------------------------*/

/*
	The name of the action.
	The stock id for the action, or the name of an icon from the icon theme.
	The label for the action. This field should typically be marked for translation, see gtk_action_group_set_translation_domain(). If label is NULL, the label of the stock item with id stock_id is used.
	The accelerator for the action, in the format understood by gtk_accelerator_parse().
	The tooltip for the action. This field should typically be marked for translation, see gtk_action_group_set_translation_domain().
	The function to call when the action is activated.

	http://library.gnome.org/devel/gtk/stable/gtk-Stock-Items.html

 */
 static const GtkActionEntry online_action_entries[] =
 {
	{	"Redraw",			NULL,					N_( "Redraw screen" ),		NULL,				NULL,	G_CALLBACK(action_Redraw)			},
	{	"SaveScreen",		NULL,					N_( "Save screen" ),		NULL,				NULL,	G_CALLBACK(action_SaveScreen)		},
	{	"PrintScreen",		GTK_STOCK_PRINT,		N_( "Print" ),				"Print",			NULL,	G_CALLBACK(action_PrintScreen)		},
	{	"DumpScreen",		NULL,					N_( "Dump screen" ),		"<Alt>D",			NULL,	G_CALLBACK(action_DumpScreen)		},
 	{	"Disconnect",		GTK_STOCK_DISCONNECT,	N_( "_Disconnect" ),		NULL,				NULL,	G_CALLBACK(action_Disconnect)		},

	/* Select actions */
	{	"SelectField",		NULL,					N_( "Select Field" ),		"<Ctrl>f",			NULL,	G_CALLBACK(action_SelectField)		},

	{ 	"SelectRight",		NULL,					N_( "Select Right" ),		"<Shift>Right",		NULL,	G_CALLBACK(action_SelectRight)		},
	{ 	"SelectLeft",		NULL,					N_( "Select Left" ),		"<Shift>Left",		NULL,	G_CALLBACK(action_SelectLeft)		},
	{ 	"SelectUp",			NULL,					N_( "Select Up" ),			"<Shift>Up",		NULL,	G_CALLBACK(action_SelectUp)			},
	{ 	"SelectDown",		NULL,					N_( "Select Down" ),		"<Shift>Down",		NULL,	G_CALLBACK(action_SelectDown)		},

	{	"SelectionRight",	NULL,					N_( "Selection Right" ),	"<Alt>Right",		NULL,	G_CALLBACK(action_SelectionRight)	},
	{	"SelectionLeft",	NULL,					N_( "Selection Left" ),		"<Alt>Left",		NULL,	G_CALLBACK(action_SelectionLeft)	},
	{	"SelectionUp",		NULL,					N_( "Selection Up" ),		"<Alt>Up",			NULL,	G_CALLBACK(action_SelectionUp)		},
	{	"SelectionDown",	NULL,					N_( "Selection Down" ),		"<Alt>Down",		NULL,	G_CALLBACK(action_SelectionDown)	},

	/* Cursor Movement */
	{ 	"CursorRight",		GTK_STOCK_GO_FORWARD,	N_( "Right" ),				"Right",			NULL,	G_CALLBACK(action_Right)			},
	{ 	"CursorLeft",		GTK_STOCK_GO_BACK,		N_( "Left" ),				"Left",				NULL,	G_CALLBACK(action_Left)				},
	{ 	"CursorUp",			GTK_STOCK_GO_UP,		N_( "Up" ),					"Up",				NULL,	G_CALLBACK(action_Up)				},
	{ 	"CursorDown",		GTK_STOCK_GO_DOWN,		N_( "Down" ),				"Down",				NULL,	G_CALLBACK(action_Down)				},

	{	"NextField",		GTK_STOCK_GOTO_LAST,	N_( "Next field" ),			"Tab",				NULL,	G_CALLBACK(action_Tab)				},
	{	"PreviousField",	GTK_STOCK_GOTO_FIRST,	N_( "Previous field" ),		"<Shift>Tab",		NULL,	G_CALLBACK(action_BackTab)			},

 	/* Edit actions */
 	{	"PasteNext",		NULL,					N_( "Paste next" ),			"<Shift><Ctrl>v",	NULL,	G_CALLBACK(action_PasteNext)		},
 	{	"PasteTextFile",	NULL,					N_( "Paste text file" ),	NULL,				NULL,	G_CALLBACK(action_PasteTextFile)	},
 	{	"Reselect",			NULL,					N_( "Reselect" ),			"<Shift><Ctrl>r",	NULL,	G_CALLBACK(Reselect)				},
#if defined( GTK_STOCK_SELECT_ALL )
 	{	"SelectAll",		GTK_STOCK_SELECT_ALL,	N_( "Select all" ),			"<Ctrl>a",			NULL,	G_CALLBACK(action_SelectAll)		},
#else
 	{	"SelectAll",		NULL,					N_( "Select all" ),			"<Ctrl>a",			NULL,	G_CALLBACK(action_SelectAll)		},
#endif
 	{	"EraseInput",		GTK_STOCK_CLEAR,		N_( "Erase input" ),		NULL,				NULL,	G_CALLBACK(erase_input_action)		},
 	{	"ClearFields",		NULL,					N_( "Clear" ),				NULL,				NULL,	G_CALLBACK(clear_fields_action)		},
 };

 static const GtkActionEntry ft_action_entries[] =
 {
	/* File-transfer actions */
 	{	"FTMenu",   		NULL,					N_( "File Transfer" ),		NULL,				NULL,	NULL								},
	{   "SendReceive",		NULL,					N_( "Send/Receive" ),		NULL,				NULL,	NULL								},
 	{	"Download",			GTK_STOCK_SAVE,			N_( "Receive file" ),		"<Alt>d",			NULL,	G_CALLBACK(action_Download)			},
 	{	"Upload",   		GTK_STOCK_OPEN,			N_( "Send file" ),   		"<Alt>u",			NULL,	G_CALLBACK(action_Upload)			},
 };

 static const GtkActionEntry offline_action_entries[] =
 {
 	{	"Connect",			GTK_STOCK_CONNECT,		N_( "_Connect" ),			NULL,				NULL,	G_CALLBACK(action_Connect)			},
	{	"LoadScreenDump",	NULL,					N_( "Load screen dump" ),	"<Alt>R",			NULL,	G_CALLBACK(action_LoadScreenDump)	},
	{ 	"SetHostname",		GTK_STOCK_HOME,			N_( "Set hostname" ),		NULL,				NULL,	G_CALLBACK(action_SetHostname)		},
 };

 static const GtkActionEntry selection_action_entries[] =
 {
 	{	"Append",			GTK_STOCK_ADD,			N_( "Add to copy" ),		"<Shift><Ctrl>c",	NULL,	G_CALLBACK(action_Append)			},
 	{	"Unselect",			NULL,					N_( "Unselect" ),			"<Shift><Ctrl>u",	NULL,	G_CALLBACK(action_ClearSelection)	},
 	{	"Copy",				GTK_STOCK_COPY,			N_( "Copy" ),				NULL,				NULL,	G_CALLBACK(action_Copy)				},
 	{	"CopyAsTable",		NULL,					N_( "Copy as table" ),		"<Shift>T",			NULL,	G_CALLBACK(action_CopyAsTable)		},
 	{	"PrintSelected",	NULL,					N_( "Print selected" ),		NULL,				NULL,	G_CALLBACK(action_PrintSelected)	},
	{	"SaveSelected",		NULL,					N_( "Save selected" ),		NULL,				NULL,	G_CALLBACK(action_SaveSelected)		},
 };

 static const GtkActionEntry clipboard_action_entries[] =
 {
	{	"PrintClipboard",	NULL,					N_( "Print copy" ),			NULL,				NULL,	G_CALLBACK(action_PrintClipboard)	},
	{	"SaveClipboard",	NULL,					N_( "Save copy" ),			NULL,				NULL,	G_CALLBACK(action_SaveClipboard)	},

 };

 static const GtkActionEntry paste_action_entries[] =
 {
 	{	"Paste",			GTK_STOCK_PASTE,		N_( "Paste" ),				NULL,				NULL,	G_CALLBACK(action_Paste)			},
 };

 static const GtkActionEntry common_action_entries[] =
 {
 	/* Top menus */
 	{	"FileMenu",			NULL,					N_( "_File" ),				NULL,				NULL,	NULL								},
 	{	"NetworkMenu",		NULL,					N_( "_Network" ),			NULL,				NULL,	NULL								},
 	{	"HelpMenu",			NULL,					N_( "Help" ),				NULL,				NULL,	NULL								},
 	{	"EditMenu",			NULL,					N_( "_Edit" ),				NULL,				NULL,	NULL								},
 	{	"OptionsMenu",		NULL,					N_( "_Options" ),			NULL,				NULL,	NULL								},
 	{	"SettingsMenu",		NULL,					N_( "Settings" ),			NULL,				NULL,	NULL								},
 	{	"ScriptsMenu",		NULL,					N_( "Scripts" ),			NULL,				NULL,	NULL								},
 	{	"ViewMenu",			NULL,					N_( "_View" ),				NULL,				NULL,	NULL								},
 	{	"DebugMenu",		NULL,					N_( "Debug" ),				NULL,				NULL,	NULL								},

	/* Sub menus */
	{	"FontSettings",		GTK_STOCK_SELECT_FONT,	N_( "Select font" ),		NULL,				NULL,	NULL								},

 	/* Stock menus */
 	{	"Preferences",		GTK_STOCK_PREFERENCES,	N_( "Preferences" ),		NULL,				NULL,	NULL								},
 	{	"Network",			GTK_STOCK_NETWORK,		N_( "Network" ),			NULL,				NULL,	NULL								},
 	{	"Properties",		GTK_STOCK_PROPERTIES,	N_( "Properties" ),			NULL,				NULL,	NULL								},

	/* Misc actions */
 	{	"About",			GTK_STOCK_ABOUT,		N_( "About" ),				NULL,				NULL,	G_CALLBACK(action_About)			},
 	{	"Quit",				GTK_STOCK_QUIT,			N_( "_Quit" ),				NULL,				NULL,	G_CALLBACK(action_Quit)				},
 	{	"SelectColors",		GTK_STOCK_SELECT_COLOR,	N_( "Colors" ),				NULL,				NULL,	G_CALLBACK(action_SelectColors)		},

 	/* Save actions */
 	{	"Save",				GTK_STOCK_SAVE,			N_( "Save" ),				NULL,				NULL,	NULL								},

	/* Terminal Actions */
	{ 	"Return",			GTK_STOCK_APPLY,		N_( "Return" ),				"Return",			NULL,	G_CALLBACK(action_enter)			},
	{ 	"Enter",			NULL,					N_( "Enter" ),				"KP_Enter",			NULL,	G_CALLBACK(action_enter)			},
 };


 GtkActionGroup		*action_group[ACTION_GROUP_MAX] = { NULL };


/*---[ Implement ]----------------------------------------------------------------------------------------------*/

 GCallback get_action_callback_by_name(const gchar *name)
 {
	// TODO (perry#8#): Find a better way (search all registered actions by name?)
	static const struct _list
	{
		const GtkActionEntry *action;
		int sz;
	} list[] =
	{
		{ online_action_entries,	G_N_ELEMENTS(online_action_entries)		},
		{ ft_action_entries,		G_N_ELEMENTS(ft_action_entries)			},
		{ offline_action_entries,	G_N_ELEMENTS(offline_action_entries)	},
		{ selection_action_entries, G_N_ELEMENTS(selection_action_entries)	},
		{ clipboard_action_entries, G_N_ELEMENTS(clipboard_action_entries)	},
		{ paste_action_entries,		G_N_ELEMENTS(paste_action_entries)		},
		{ common_action_entries,	G_N_ELEMENTS(common_action_entries)		}
	};

	int f, i;

	for(f=0;f<G_N_ELEMENTS(list);f++)
	{
		for(i=0;i<list[f].sz;i++)
		{
			if(!strcmp(name,list[f].action[i].name))
				return list[f].action[i].callback;
		}
	}

	Trace("Can't find action \"%s\"",name);

 	return NULL;
 }

 static void clear_and_call(GtkAction *action, int (*call)(void))
 {
 	action_ClearSelection();
 	call();
 }

 static void clear_fields_action(void)
 {
 	clear_and_call(0,action_ClearFields);
 }

 static void erase_input_action(void)
 {
 	clear_and_call(0,action_EraseInput);
 }

 void action_Up(GtkWidget *w, gpointer user_data)
 {
 	clear_and_call(0,action_CursorUp);
 }

 void action_Down(GtkWidget *w, gpointer user_data)
 {
 	clear_and_call(0,action_CursorDown);
 }

 void action_Left(GtkWidget *w, gpointer user_data)
 {
 	clear_and_call(0,action_CursorLeft);
 }

 void action_Right(GtkWidget *w, gpointer user_data)
 {
 	clear_and_call(0,action_CursorRight);
 }

 void action_Tab(void)
 {
 	action_NextField();
 }

 void action_BackTab(void)
 {
 	action_PreviousField();
 }

 void DisableNetworkActions(void)
 {
	gtk_action_group_set_sensitive(online_actions,FALSE);
	gtk_action_group_set_sensitive(offline_actions,FALSE);
 }

 static void action_Disconnect(GtkWidget *w, gpointer user_data)
 {
 	Trace("%s Connected:%d Widget: %p",__FUNCTION__,PCONNECTED,w);

 	if(!PCONNECTED)
 		return;

	DisableNetworkActions();
 	action_ClearSelection();
 	host_disconnect(0);
 }


 void action_Connect(void)
 {
    const gchar *host;

 	Trace("%s Connected:%d",__FUNCTION__,PCONNECTED);

 	if(PCONNECTED)
 		return;

 	action_ClearSelection();

	host = GetString("Network","Hostname",CN);

    if(host == CN)
    {
        action_SetHostname();
    }
    else
    {
        DisableNetworkActions();
        gtk_widget_set_sensitive(topwindow,FALSE);
        RunPendingEvents(0);

        if(host_connect(host,1) == ENOTCONN)
        {
            Warning( N_( "Negotiation with %s failed!" ),host);
        }

        gtk_widget_set_sensitive(topwindow,TRUE);
        gtk_widget_grab_focus(terminal);

    }

/*
	// TODO (perry#5#): If there's no previous server ask for it.


	Trace("Calling %s","host_reconnect");
 	host_reconnect(0);
*/
 }

 void action_enter(GtkWidget *w, gpointer user_data)
 {
 	action_ClearSelection();
 	if(PCONNECTED)
		action_Enter();
	else
		action_Connect();
 }

 static void action_PrintScreen(GtkWidget *w, gpointer user_data)
 {
	PrintText(PROGRAM_NAME, GetScreenContents(TRUE));
 }

 static void action_PrintSelected(GtkWidget *w, gpointer user_data)
 {
	PrintText(PROGRAM_NAME, GetSelection());
 }

 static void action_PrintClipboard(GtkWidget *w, gpointer user_data)
 {
	PrintText(PROGRAM_NAME, GetClipboard());
 }

 static void action_Quit(void)
 {
 	action_Save();
 	program_quit();
 }

 static void action_About(GtkWidget *w, gpointer user_data)
 {
 	static const char *authors[] = {	"Paul Mattes <Paul.Mattes@usa.net>",
										"GTRC",
										"Perry Werneck <perry.werneck@gmail.com>",
										"and others",
										NULL};

	static const char license[] =
	N_( "This program is free software; you can redistribute it and/or "
		"modify it under the terms of the GNU General Public License as "
 		"published by the Free Software Foundation; either version 2 of the "
		"License, or (at your option) any later version.\n\n"
		"This program is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
		"GNU General Public License for more details.\n\n"
		"You should have received a copy of the GNU General Public License "
		"along with this program; if not, write to the Free Software "
		"Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02111-1307 "
		"USA" );

	GdkPixbuf	*logo = NULL;
	gchar		*filename;

	/* Load image logo */
	if(program_logo && g_file_test(program_logo,G_FILE_TEST_IS_REGULAR))
		 logo = gdk_pixbuf_new_from_file(program_logo,NULL);

	if(!logo)
	{
		filename = g_strdup_printf("%s%c%s.%s", program_data, G_DIR_SEPARATOR, PROGRAM_NAME, LOGOEXT);

		if(g_file_test(filename,G_FILE_TEST_IS_REGULAR))
			logo = gdk_pixbuf_new_from_file(filename, NULL);

		g_free(filename);
	}

	/* Build and show about dialog */
 	gtk_show_about_dialog(	GTK_WINDOW(topwindow),
#if GTK_MAJOR_VERSION >= 2 && GTK_MINOR_VERSION >= 12
							"program-name",    		program_name,
#else
							"name",    				program_name,
#endif
							"logo",					logo,
							"authors", 				authors,
							"license", 				gettext( license ),
#ifdef HAVE_LIBGNOME
							"comments",				_( "3270 Terminal emulator for Gnome."),
#else
							"comments",				_( "3270 Terminal emulator for GTK."),
#endif
							"version", 				program_fullversion,
							"wrap-license",			TRUE,
							NULL
						);

	if(logo)
		gdk_pixbuf_unref(logo);
 }

 static void toggle_action(GtkToggleAction *action, int id)
 {
 	Trace("%s(%d)",__FUNCTION__,id);
 	set_toggle(id,gtk_toggle_action_get_active(action));
 }

 static void toggle_set(GtkAction *action,int id)
 {
 	set_toggle(id,TRUE);
 }

 static void toggle_reset(GtkAction *action,int id)
 {
 	set_toggle(id,FALSE);
 }

 static void toggle_gui(GtkToggleAction *action, int id)
 {
    gui_toggle[id] = gtk_toggle_action_get_active(action);

	screen_suspend();

	SetBoolean("Toggles",gui_toggle_info[id].name,gui_toggle[id]);

    if(id == GUI_TOGGLE_BOLD)
        FontChanged();

	screen_resume();

 }

 static void toggle_toolbar(GtkToggleAction *action, int id)
 {
    gboolean toggle = gtk_toggle_action_get_active(action);

	SetBoolean("Toggles","Toolbar",toggle);

	if(toolbar_widget)
	{
#if GTK_CHECK_VERSION(2,18,0)
		gtk_widget_set_visible(toolbar_widget,toggle);
#else
		if(toggle)
			gtk_widget_show(toolbar_widget);
		else
			gtk_widget_hide(toolbar_widget);
#endif
	}

 }

 static void action_ToggleGDKDebug(GtkToggleAction *action, gpointer user_data)
 {
	gdk_window_set_debug_updates(gtk_toggle_action_get_active(action));
 }

 static void LoadToggleActions(GtkActionGroup *actions)
 {
	// TODO (perry#9#): Add tooltips
 	static const struct _toggle_list
 	{
 		gboolean		set;
 		int				toggle;
		const gchar	*label;
		const gchar	*tooltip;
		const gchar	*stock_id;
	} toggle_list[] =
	{
		{ TRUE,		FULL_SCREEN,	N_( "Fullscreen" ),	NULL,	GTK_STOCK_FULLSCREEN		},
		{ FALSE,	FULL_SCREEN,	N_( "Window"),		NULL,	GTK_STOCK_LEAVE_FULLSCREEN	},
	};

	// TODO (perry#9#): Add tooltips
 	static const struct _toggle_info
 	{
 		const gchar *label;
 		const gchar *tooltip;
 		const gchar *stock_id;
 		const gchar *accelerator;
	}
	toggle_info[N_TOGGLES] =
	{
		{ N_( "Monocase" ),						NULL,	NULL,	NULL				},
		{ N_( "Alt Cursor" ),					NULL,	NULL,	NULL				},
		{ N_( "Blinking Cursor" ),				NULL,	NULL,	NULL				},
		{ N_( "Show timing" ),					NULL,	NULL,	NULL				},
		{ N_( "Track Cursor" ),					NULL,	NULL,	NULL				},
		{ N_( "DS Trace" ),						NULL,	NULL,	NULL				},
		{ N_( "Scroll bar" ),					NULL,	NULL,	NULL				},
		{ N_( "Line Wrap" ),					NULL,	NULL,	NULL				},
		{ N_( "Blank Fill" ),					NULL,	NULL,	NULL				},
		{ N_( "Screen Trace" ),					NULL,	NULL,	NULL				},
		{ N_( "Event Trace" ),					NULL,	NULL,	NULL				},
		{ N_( "Paste with left margin" ),		NULL,	NULL,	NULL				},
		{ N_( "Select by rectangles" ),			NULL,	NULL,	N_( "<alt>S" )		},
		{ N_( "Cross Hair Cursor" ),			NULL,	NULL,	N_( "<alt>X" )		},
		{ N_( "Visible Control chars" ),		NULL,	NULL,	NULL				},
		{ N_( "Aid wait" ),						NULL,	NULL,	NULL				},
		{ N_( "Full Screen" ),					NULL,	NULL,	N_( "<alt>Home" )	},
		{ N_( "Auto-Reconnect" ),				NULL,	NULL,	NULL				},
		{ N_( "Insert" ),						NULL,	NULL,	"Insert"			},
		{ N_( "Keypad" ),						NULL,	NULL,	N_( "<alt>K" )		},
		{ N_( "Smart paste" ),					NULL,   NULL,   NULL    			},

	};

 	int f;
 	GtkToggleAction *action;

	/* Debug toggles */
	action = gtk_toggle_action_new(	"ToggleGDKDebug", _( "Debug screen updates"), NULL,NULL);
	g_signal_connect(G_OBJECT(action),"toggled", G_CALLBACK(action_ToggleGDKDebug),0);
	gtk_action_group_add_action(actions,(GtkAction *) action);

	/* Internal toggles */
 	for(f=0;f< GUI_TOGGLE_COUNT;f++)
 	{
 		gchar *name = g_strconcat("Toggle",gui_toggle_info[f].name,NULL);

#ifdef DEBUG
		if(!gui_toggle_info[f].name)
		{
			Trace("No description for toggle %d",f);
			exit(-1);
		}
#endif
		GtkToggleAction *action = gtk_toggle_action_new(	name,
															gettext(gui_toggle_info[f].label),
															gettext(gui_toggle_info[f].tooltip),
															gui_toggle_info[f].stock_id );

		Trace("%s: %p",name,action);

        gui_toggle[f] = GetBoolean("Toggles",gui_toggle_info[f].name,gui_toggle_info[f].def);

		gtk_toggle_action_set_active(action,gui_toggle[f]);
		g_signal_connect(G_OBJECT(action),"toggled", G_CALLBACK(toggle_gui),(gpointer) f);

		if(gui_toggle_info[f].accelerator)
			gtk_action_group_add_action_with_accel(actions,(GtkAction *) action, gettext(gui_toggle_info[f].accelerator));
		else
			gtk_action_group_add_action(actions,(GtkAction *) action);

		g_free(name);
 	}

	/* Toolbar toggle */
	action = gtk_toggle_action_new("ToggleToolbar", _( "Toolbar"), NULL, NULL);
	gtk_toggle_action_set_active(action,GetBoolean("Toggles","Toolbar",TRUE));
	g_signal_connect(G_OBJECT(action),"toggled", G_CALLBACK(toggle_toolbar),0);
	gtk_action_group_add_action(common_actions,(GtkAction *) action);

	/* Toggle actions */
 	for(f=0;f<N_TOGGLES;f++)
 	{
 		gchar *name = g_strconcat("Toggle",get_toggle_name(f),NULL);

#ifdef DEBUG
		if(!toggle_info[f].label)
		{
			Trace("No description for toggle %d",f);
			exit(-1);
		}
#endif

		GtkToggleAction *action = gtk_toggle_action_new(	name,
															gettext(toggle_info[f].label),
															gettext(toggle_info[f].tooltip),
															toggle_info[f].stock_id );

		Trace("Toggle(%d): %s",f,name);
		gtk_toggle_action_set_active(action,Toggled(f));
		g_signal_connect(G_OBJECT(action),"toggled", G_CALLBACK(toggle_action),(gpointer) f);

#ifndef X3270_TRACE

		if(f == DS_TRACE || f == SCREEN_TRACE || f == EVENT_TRACE)
			gtk_action_set_sensitive((GtkAction *) action,FALSE);

#endif

		if(toggle_info[f].accelerator)
			gtk_action_group_add_action_with_accel(actions,(GtkAction *) action, gettext(toggle_info[f].accelerator));
		else
			gtk_action_group_add_action(actions,(GtkAction *) action);

		g_free(name);
 	}

	/* Set/Reset actions */
 	for(f=0;f< G_N_ELEMENTS(toggle_list);f++)
 	{
 		int		id = toggle_list[f].toggle;
 		gchar	*name = g_strconcat((toggle_list[f].set ? "Set" : "Reset" ),get_toggle_name(id),NULL);

		Trace("Creating action \"%s\"",name);

		GtkAction *action = gtk_action_new(	name,
											gettext(toggle_list[f].label),
											gettext(toggle_list[f].tooltip),
											toggle_list[f].stock_id );

		if(toggle_list[f].set)
		{
			g_signal_connect(G_OBJECT(action),"activate", G_CALLBACK(toggle_set),(gpointer) id);
			if(Toggled(id))
				gtk_action_set_visible(action,FALSE);
		}
		else
		{
			g_signal_connect(G_OBJECT(action),"activate", G_CALLBACK(toggle_reset),(gpointer) id);
			if(!Toggled(id))
				gtk_action_set_visible(action,FALSE);
		}

		gtk_action_group_add_action(actions,action);

		g_free(name);
 	}


 }

 static void Load3270Actions(GtkActionGroup *actions)
 {
 	struct call_3270
 	{
 		const gchar *name;
 		const gchar *label;
 		const gchar *tooltip;
 		const gchar *stock_id;
 		int			 (*call)(void);
 		const gchar *accelerator;
	};

	// TODO (perry#9#): Add tooltips
 	static const struct call_3270 action_clear[] =
	{
		{ "Reset",			N_( "Reset" ),
							NULL,
							NULL,
							action_Reset,
							N_( "<Ctrl>r" ) },

		{ "Escape",			N_( "Escape" ),
							NULL,
							NULL,
							action_Reset,
							"Escape" }
	};

 	static const struct call_3270 action_info[] =
	{
		{ "EraseEOF",		N_( "Erase EOF" ),
							N_( "Erase to the end of the field" ),
							NULL,
							lib3270_EraseEOF,
							"End" },

		{ "EraseEOL",		N_( "Erase EOL" ),
							N_( "Erase to the end of the field or line" ),
							NULL,
							lib3270_EraseEOL,
							NULL },

		{ "Home",			N_( "First Field" ),
							NULL,
							NULL,
							action_FirstField,
							"Home" },

		{ "DeleteWord",		N_( "Delete Word" ),
							NULL,
							NULL,
							action_DeleteWord,
							N_( "<Ctrl>w" )	},

		{ "EraseField",		N_( "Erase Field" ),
							NULL,
							NULL,
							action_DeleteField,
							N_( "<Ctrl>u" ) },

		{ "Delete",			N_( "Delete Char" ),
							NULL,
							NULL,
							action_Delete,
							"Delete" },

		{ "Erase",			N_( "Backspace" ),
							NULL,
							NULL,
							action_Erase,
							"BackSpace" },

		{ "SysREQ",			N_( "Sys Req" ),
							NULL,
							NULL,
							action_SysReq,
							"Sys_Req" },

	};

	int f;

 	for(f=0;f<G_N_ELEMENTS(action_info);f++)
 	{
		GtkAction *action = gtk_action_new(	action_info[f].name,
											gettext(action_info[f].label),
											gettext(action_info[f].tooltip),
											action_info[f].stock_id );

		g_signal_connect(G_OBJECT(action),"activate", G_CALLBACK(action_info[f].call),0 );

		if(action_info[f].accelerator)
			gtk_action_group_add_action_with_accel(actions,(GtkAction *) action, gettext(action_info[f].accelerator));
		else
			gtk_action_group_add_action(actions,(GtkAction *) action);

 	}

 	for(f=0;f<G_N_ELEMENTS(action_clear);f++)
 	{
		GtkAction *action = gtk_action_new(	action_clear[f].name,
											gettext(action_clear[f].label),
											gettext(action_clear[f].tooltip),
											action_clear[f].stock_id );

		g_signal_connect(G_OBJECT(action),"activate", G_CALLBACK(clear_and_call),(gpointer) action_clear[f].call);

		if(action_info[f].accelerator)
			gtk_action_group_add_action_with_accel(actions,(GtkAction *) action, gettext(action_clear[f].accelerator));
		else
			gtk_action_group_add_action(actions,(GtkAction *) action);

 	}
 }

#ifdef CONFIGURABLE_KEYBOARD_ACTIONS

 static int InsertKeyboardAction(guint keyval, GdkModifierType state, const gchar *action_name)
 {
 	int f;
 	struct WindowActions *act = NULL;

	if(keyproc_table)
	{
		for(f=0;!act && f < keyproc_table_size;f++)
		{
			if(keyproc_table[f].keyval == keyval && keyproc_table[f].state == state)
				act = keyproc_table+f;
		}
	}

 	if(!act)
 	{
		if(!keyproc_table)
		{
			keyproc_table_size = 0;
			keyproc_table = g_new(struct WindowActions,keyproc_table_size+1);

			Trace("Keyproc_table: %p",keyproc_table);
		}
		else
		{
			keyproc_table = g_renew(struct WindowActions,keyproc_table,keyproc_table_size+1);
		}

		act = keyproc_table + keyproc_table_size;

		keyproc_table_size++;

		Trace("New action created (total: %d)",keyproc_table_size);
 	}

	act->keyval = keyval;
	act->state = state;

	if(g_strncasecmp(action_name,"pf",2))
	{
		act->callback = (void (*)(guint arg)) get_action_callback_by_name(action_name);
	}
	else
	{
		act->arg = atoi(action_name+2);
		Trace("Adding PF%02d action",act->arg);
		if(act->arg > 0 && act->arg < 25)
			act->callback = (void (*)(guint arg)) PFKey;
	}

	Trace("Keyval: %08d State: %08d Action: %s Callback: %p",keyval,state,action_name,act->callback);

	return act->callback ? 0 : ENOENT;
 }

 static void LoadKeyboardActions(void)
 {
 	static const struct _default_key
 	{
		guint keyval;
		GdkModifierType state;
		const gchar *action_name;
 	} default_key[] =
 	{
		{	GDK_Left,			0, "CursorLeft"		},
		{	GDK_Up,				0, "CursorUp"		},
		{	GDK_Right,			0, "CursorRight"	},
		{	GDK_Down,			0, "CursorDown"		},
		{	GDK_KP_Left,		0, "CursorLeft"		},
		{	GDK_KP_Up,			0, "CursorUp"		},
		{	GDK_KP_Right,		0, "CursorRight"	},
		{	GDK_KP_Down,		0, "CursorDown"		},
		{	GDK_ISO_Left_Tab,	0, "PreviousField"	},
		{	GDK_Tab, 			0, "NextField"		},
 	};

	GDir	*dir;
	gchar	*path = g_build_filename(program_data,"ui",NULL);
	int		f;

	// Load default keys
	for(f=0;f<G_N_ELEMENTS(default_key);f++)
		InsertKeyboardAction(default_key[f].keyval, default_key[f].state, default_key[f].action_name);

	for(f=0;f<12;f++)
	{
		gchar *ptr;

		ptr = g_strdup_printf("pf%d",f+1);
		InsertKeyboardAction(GDK_F1+f, 0, ptr);
		g_free(ptr);

		ptr = g_strdup_printf("pf%d",f+13);
		InsertKeyboardAction(GDK_F1+f, GDK_SHIFT_MASK, ptr);
		g_free(ptr);

	}

	// Load configurable ones
	dir = g_dir_open(path,0,NULL);
	if(dir)
	{
		const gchar *name = g_dir_read_name(dir);
		while(name)
		{
			if(g_str_has_suffix(name,"key"))
			{
				gchar	*contents;
				gchar	*filename = g_build_filename(path,name,NULL);

				// TODO (perry#2#): Notify user if the file loading fails.
				if(g_file_get_contents(filename,&contents,NULL,NULL))
				{
					gchar	**line 	= g_strsplit(contents,"\n",-1);

					for(f=0;line[f];f++)
					{
						char *key = line[f];
						char *val;

						for(val=key;*val && *val != '\r';val++);
						*val = 0;

						key = g_strstrip(key);

						if(*key && *key != ';' && *key != '#')
						{

							val = strchr(key,'=');

							if(val)
							{
								guint			keyval;
								GdkModifierType	state;

								*(val++) = 0;
								key = g_strstrip(key);
								val = g_strstrip(val);

								Trace("\"%s\" = \"%s\"",key,val);

								gtk_accelerator_parse(key,&keyval,&state);

								if(!keyval)
									Log("Invalid accelerator id \"%s\" in %s",key,filename);
								else
									InsertKeyboardAction(keyval,state,val);
							}
						}
					}

					g_strfreev(line);
					g_free(contents);
				}

				g_free(filename);
			}
			name = g_dir_read_name(dir);
		}
		g_dir_close(dir);
	}

	g_free(path);
 }
#endif

 GtkUIManager * LoadApplicationUI(GtkWidget *widget)
 {
 	static const struct _group
 	{
		gchar 					*name;
		const GtkActionEntry 	*entries;
		guint 					n_entries;
	} group[ACTION_GROUP_MAX] =
 	{
		{ "Common",				common_action_entries,		G_N_ELEMENTS(common_action_entries)		},
		{ "Online",				online_action_entries,		G_N_ELEMENTS(online_action_entries)		},
		{ "Offline",			offline_action_entries,		G_N_ELEMENTS(offline_action_entries)	},
		{ "Selection",			selection_action_entries,	G_N_ELEMENTS(selection_action_entries)	},
		{ "Clipboard",			clipboard_action_entries,	G_N_ELEMENTS(clipboard_action_entries)	},
		{ "Paste",				paste_action_entries,		G_N_ELEMENTS(paste_action_entries)		},
		{ "FileTransfer",		ft_action_entries,			G_N_ELEMENTS(ft_action_entries)			},
	};

	GtkUIManager 	*ui_manager = gtk_ui_manager_new(); // http://library.gnome.org/devel/gtk/stable/GtkUIManager.html
	GError			*error = NULL;
	int				f;
	gchar			*path;
	gchar			*filename;
 	GDir			*dir;

	// Load actions
	for(f=0;f < ACTION_GROUP_MAX; f++)
	{
		action_group[f] = gtk_action_group_new(group[f].name);
		gtk_action_group_set_translation_domain(action_group[f], PACKAGE_NAME);
		gtk_action_group_add_actions(action_group[f],group[f].entries, group[f].n_entries, topwindow);
	}

	Load3270Actions(online_actions);
	LoadToggleActions(common_actions);
	LoadCustomActions(ui_manager,action_group,ACTION_GROUP_MAX,GetConf());

#ifdef CONFIGURABLE_KEYBOARD_ACTIONS
	LoadKeyboardActions();
#endif

	// Add actions and load UI
	for(f=0;f < ACTION_GROUP_MAX; f++)
		gtk_ui_manager_insert_action_group(ui_manager,action_group[f], 0);

	path = g_build_filename(program_data,"ui",NULL);

    dir = g_dir_open(path,0,NULL);

	Trace("Searching for UI definitions in %s (dir: %p)",path,dir);

    if(!dir)
    {
		WarningPopup( _( "Can't find UI definitions in\n%s" ), path);
    }
    else
    {
		const gchar *name = g_dir_read_name(dir);
		while(name)
		{
			filename = g_build_filename(path,name,NULL);

			if(g_str_has_suffix(filename,"xml"))
			{
				Trace("Loading %s",filename);

				if(!gtk_ui_manager_add_ui_from_file(ui_manager,filename,&error))
				{
					if(error && error->message)
						Warning( _( "Can't load %s: %s" ),filename,error->message);
					else
						Warning( _( "Can't load %s" ), filename);
				}
			}

			g_free(filename);
			name = g_dir_read_name(dir);
		}
		g_dir_close(dir);
    }

	g_free(path);

	gtk_ui_manager_ensure_update(ui_manager);
	AddPluginUI(ui_manager,program_data);

	gtk_ui_manager_ensure_update(ui_manager);
	return ui_manager;
 }

 int GetFunctionKey(GdkEventKey *event)
 {
 	int rc = (event->keyval - GDK_F1)+1;

 	if(event->state & GDK_SHIFT_MASK)
 	   rc += 12;

	return rc;
 }

 gboolean PFKey(guint key)
 {
 	// TODO (perry#2#): Make it configurable

	Trace("Running PF %d",key);

	if(!TOGGLED_KEEP_SELECTED)
		action_ClearSelection();
 	action_PFKey(key);
	return TRUE;
 }

 gboolean KeyboardAction(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
 {

 	int		f;

	/* Is function key? */

#ifndef CONFIGURABLE_KEYBOARD_ACTIONS

	if(IS_FUNCTION_KEY(event))
	{
		Trace("PF%d",GetFunctionKey(event));
		PFKey(GetFunctionKey(event));
		return TRUE;
	}

#endif

    /* Check for special keyproc actions */
	for(f=0; f < keyproc_table_size;f++)
	{
		if(keyproc_table[f].keyval == event->keyval && (event->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_ALT_MASK)) == keyproc_table[f].state && keyproc_table[f].callback)
		{
			Trace("%p(%d)",keyproc_table[f].callback,keyproc_table[f].arg);
			keyproc_table[f].callback(keyproc_table[f].arg);
			return TRUE;
		}
	}

	return FALSE;
 }

 static int SaveText(const char *title, gchar *text)
 {

	GtkWidget *dialog = gtk_file_chooser_dialog_new( gettext(title),
                                                     GTK_WINDOW(topwindow),
                                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                                     GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_SAVE,	GTK_RESPONSE_ACCEPT,
                                                     NULL );


	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GError	*error = NULL;
		gchar	*filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		if(!g_file_set_contents(filename,text,-1,&error))
		{
			Warning( N_( "Error saving %s\n%s" ), filename, error->message ? error->message : N_( "Unexpected error" ));
			g_error_free(error);
		}

		g_free(filename);
	}

	gtk_widget_destroy(dialog);
 	g_free(text);
 	return 0;
 }

 static void action_LoadScreenDump(void)
 {
 	gchar		*ptr;
	GKeyFile	*conf   = GetConf();

	GtkWidget 	*dialog = gtk_file_chooser_dialog_new( _( "Load screen dump" ),
														GTK_WINDOW(topwindow),
														GTK_FILE_CHOOSER_ACTION_OPEN,
														GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
														GTK_STOCK_OPEN,	GTK_RESPONSE_ACCEPT,
														NULL );


	ptr = g_key_file_get_string(conf,"uri","ScreenDump",NULL);
	if(ptr)
	{
			gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(dialog),ptr);
			g_free(ptr);
	}

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GError		*error = NULL;
		gchar		*filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gsize		sz;
		struct ea	*buffer	= NULL;
		gchar		*ptr;

		ptr = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		g_key_file_set_string(conf,"uri","ScreenDump",ptr);
		g_free(ptr);

		if(!g_file_get_contents(filename, (gchar **) ((void *) &buffer), &sz, &error))
		{
			Warning( N_( "Error loading %s\n%s" ), filename, error->message ? error->message : N_( "Unexpected error" ));
			g_error_free(error);
		}
		else
		{
			sz /= sizeof(struct ea);
			if(set_device_buffer(buffer,sz))
				Warning( N_( "Can't set device buffer contents" ) );

			gtk_widget_set_sensitive(terminal,TRUE);
			gtk_action_group_set_sensitive(online_actions,TRUE);
			gtk_action_group_set_sensitive(offline_actions,TRUE);

			gtk_widget_queue_draw(terminal);
			gtk_widget_grab_focus(terminal);

		}
		g_free(filename);
		g_free(buffer);
	}

	gtk_widget_destroy(dialog);
 }

 static void action_DumpScreen(void)
 {
 	gchar		*ptr;
	GKeyFile	*conf   = GetConf();

	GtkWidget 	*dialog = gtk_file_chooser_dialog_new( _( "Dump screen contents" ),
														 GTK_WINDOW(topwindow),
														 GTK_FILE_CHOOSER_ACTION_SAVE,
														 GTK_STOCK_CANCEL,	GTK_RESPONSE_CANCEL,
														 GTK_STOCK_SAVE,	GTK_RESPONSE_ACCEPT,
														 NULL );


	ptr = g_key_file_get_string(conf,"uri","ScreenDump",NULL);
	if(ptr)
	{
			gtk_file_chooser_set_uri(GTK_FILE_CHOOSER(dialog),ptr);
			g_free(ptr);
	}

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GError		*error = NULL;
		gchar		*filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		int			sz;
		struct ea	*buffer = copy_device_buffer(&sz);

		ptr = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		g_key_file_set_string(conf,"uri","ScreenDump",ptr);
		g_free(ptr);

		if(!g_file_set_contents(filename,(gchar *) buffer,sz*sizeof(struct ea),&error))
		{
			Warning( N_( "Error saving %s\n%s" ), filename, error->message ? error->message : N_( "Unexpected error" ));
			g_error_free(error);
		}
		g_free(filename);
		free(buffer);
	}

	gtk_widget_destroy(dialog);
 }

  static void action_SaveScreen(void)
 {
	SaveText(N_( "Save screen contents" ), GetScreenContents(TRUE));
 }

 static void action_SaveSelected(void)
 {
	SaveText(N_( "Save selected text" ), GetSelection());
 }

 static void action_SaveClipboard(void)
 {
	SaveText(N_( "Save clipboard contents" ), GetClipboard());
 }

 void SetHostname(const gchar *hostname)
 {
	SetString("Network","Hostname",hostname);
	CallPlugins("SetHostname",hostname);
 }

 void action_SetHostname(void)
 {
 	char			*hostname;
 	char			*ptr;
 	gboolean		again		= TRUE;
	char 			buffer[1024];
 	GtkTable		*table		= GTK_TABLE(gtk_table_new(2,4,FALSE));
 	GtkEntry		*host		= GTK_ENTRY(gtk_entry_new());
 	GtkEntry		*port		= GTK_ENTRY(gtk_entry_new());
 	GtkToggleButton	*checkbox	= GTK_TOGGLE_BUTTON(gtk_check_button_new_with_label( _( "Secure connection" ) ));
 	GtkWidget 		*dialog 	= gtk_dialog_new_with_buttons(	_( "Select hostname" ),
																GTK_WINDOW(topwindow),
																GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
																GTK_STOCK_CONNECT,	GTK_RESPONSE_ACCEPT,
																GTK_STOCK_CANCEL,	GTK_RESPONSE_REJECT,
																NULL	);

	gtk_window_set_icon_name(GTK_WINDOW(dialog),GTK_STOCK_HOME);
	gtk_entry_set_max_length(host,0xFF);
	gtk_entry_set_width_chars(host,60);

	gtk_entry_set_max_length(port,6);
	gtk_entry_set_width_chars(port,7);

	gtk_table_attach(table,gtk_label_new( _( "Hostname:" ) ), 0,1,0,1,0,0,5,0);
	gtk_table_attach(table,GTK_WIDGET(host), 1,2,0,1,GTK_EXPAND|GTK_FILL,0,0,0);

	gtk_table_attach(table,gtk_label_new( _( "Port:" ) ), 2,3,0,1,0,0,5,0);
	gtk_table_attach(table,GTK_WIDGET(port), 3,4,0,1,GTK_FILL,0,0,0);

	gtk_table_attach(table,GTK_WIDGET(checkbox), 1,2,1,2,GTK_EXPAND|GTK_FILL,0,0,0);

	gtk_container_set_border_width(GTK_CONTAINER(table),5);

	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),GTK_WIDGET(table),FALSE,FALSE,2);

	strncpy(hostname = buffer,GetString("Network","Hostname",""),1023);

#ifdef HAVE_LIBSSL
	if(!strncmp(hostname,"L:",2))
	{
		gtk_toggle_button_set_active(checkbox,TRUE);
		hostname += 2;
	}
#else
	gtk_toggle_button_set_active(checkbox,FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(checkbox),FALSE);
	if(!strncmp(hostname,"L:",2))
		hostname += 2;
#endif

	ptr = strchr(hostname,':');
	if(ptr)
	{
		*(ptr++) = 0;
		gtk_entry_set_text(port,ptr);
	}
	else
	{
		gtk_entry_set_text(port,GetString("Network","DefaultPort","8023"));
	}

	gtk_entry_set_text(host,hostname);

	gtk_widget_show_all(GTK_WIDGET(table));

 	while(again)
 	{
 		gtk_widget_set_sensitive(dialog,TRUE);
 		switch(gtk_dialog_run(GTK_DIALOG(dialog)))
 		{
		case GTK_RESPONSE_ACCEPT:

			gtk_widget_set_sensitive(dialog,FALSE);

			if(gtk_toggle_button_get_active(checkbox))
				strcpy(buffer,"L:");
			else
				*buffer = 0;

			strncat(buffer,gtk_entry_get_text(host),1023);
			strncat(buffer,":",1023);
			strncat(buffer,gtk_entry_get_text(port),1023);

			if(!host_connect(buffer,1))
			{
				// Connection OK
				again = FALSE;
				SetHostname(buffer);
			}
			break;

		case GTK_RESPONSE_REJECT:
			again = FALSE;
			break;
 		}
 	}

	gtk_widget_destroy(dialog);

 }
