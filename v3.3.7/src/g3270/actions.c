/*
* Copyright 2008, Banco do Brasil S.A.
*
* This file is part of g3270
*
* This program file is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; version 3 of the License.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program in a file named COPYING; if not, write to the
* Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301 USA
*
* Authors:
*
* Perry Werneck<perry.werneck@gmail.com>
*
*/

 #include <gdk/gdk.h>
 #include <gdk/gdkkeysyms.h>

 #include "config.h"
 #include <globals.h>

 #include "g3270.h"
 #include <lib3270/kybdc.h>
 #include <lib3270/actionsc.h>
 #include <lib3270/toggle.h>
 #include <lib3270/hostc.h>

 #ifndef GDK_NUMLOCK_MASK
	#define GDK_NUMLOCK_MASK GDK_MOD2_MASK
 #endif

/*---[ Prototipes ]---------------------------------------------------------------------------------------------*/

/*---[ Callback tables ]----------------------------------------------------------------------------------------*/

 #ifdef DEBUG
	#define LIB3270_ACTION(key,state,action) { key, state, #key " (" #state ")", (void (*)(GtkWidget *, gpointer)) action_lib3270, (gpointer) action, #action }
	#define PF_ACTION(key,state,action) { key, state, #key " (" #state ")", (void (*)(GtkWidget *, gpointer)) action_pf, (gpointer) action, "action_pf(" #action ")" }
	#define G3270_ACTION(key,state,action) { key, state, #key " (" #state ")", (void (*)(GtkWidget *, gpointer)) action, 0, #action }
	#define TOGGLE_ACTION(key,state,action) { key, state, #key " (" #state ")", (void (*)(GtkWidget *, gpointer)) action_Toggle, (gpointer) action, "action_Toggle(" #action ")" }
 #else
	#define LIB3270_ACTION(key,state,action) { key, state, (void (*)(GtkWidget *, gpointer)) action_lib3270, (gpointer) action }
	#define PF_ACTION(key,state,action) { key, state, (void (*)(GtkWidget *, gpointer)) action_pf, (gpointer) action }
	#define G3270_ACTION(key,state,action) { key, state, (void (*)(GtkWidget *, gpointer)) action, 0 }
	#define TOGGLE_ACTION(key,state,action) { key, state, (void (*)(GtkWidget *, gpointer)) action_Toggle, (gpointer) action }
 #endif

 struct WindowActions
 {
	guint	keyval;
	guint	state;

#ifdef DEBUG
	const char	*trace;
#endif


	void (*callback)(GtkWidget *w, gpointer data);
	const gpointer user_data;

#ifdef DEBUG
	const char	 *action_trace;
#endif

 };

#define IS_FUNCTION_KEY(event)   (event->keyval >= GDK_F1 && event->keyval <= GDK_F12 && !(event->state & (GDK_MOD1_MASK|GDK_CONTROL_MASK)))

/*---[ Implement ]----------------------------------------------------------------------------------------------*/

 static void action_pf(GtkWidget *w, gpointer id)
 {
	action_internal(PF_action, IA_DEFAULT, id, CN);
 }

 void action_PageUP(GtkWidget *w, gpointer user_data)
 {
	WaitingForChanges = TRUE;

	// TODO (perry#1#): Read FN association from configuration file.
	action_internal(PF_action, IA_DEFAULT, "7", CN);
 }

 void action_PageDown(GtkWidget *w, gpointer user_data)
 {
	WaitingForChanges = TRUE;

	// TODO (perry#1#): Read FN association from configuration file.
	action_internal(PF_action, IA_DEFAULT, "8", CN);
 }

 void action_Clear(GtkWidget *w, gpointer user_data)
 {
 	action_ClearSelection();
 	action_internal(EraseInput_action, IA_DEFAULT, CN, CN);
 }

 void action_Up(GtkWidget *w, gpointer user_data)
 {
  	action_ClearSelection();
	action_internal(Up_action, IA_DEFAULT, CN, CN);
 }

 void action_Down(GtkWidget *w, gpointer user_data)
 {
 	action_ClearSelection();
 	action_internal(Down_action, IA_DEFAULT, CN, CN);
 }

 void action_Left(GtkWidget *w, gpointer user_data)
 {
 	action_ClearSelection();
 	action_internal(Left_action, IA_DEFAULT, CN, CN);
 }

 void action_Right(GtkWidget *w, gpointer user_data)
 {
 	action_ClearSelection();
 	action_internal(Right_action, IA_DEFAULT, CN, CN);
 }

 void action_Tab(GtkWidget *w, gpointer user_data)
 {
 	action_internal(Tab_action, IA_DEFAULT, CN, CN);
 }

 void action_BackTab(GtkWidget *w, gpointer user_data)
 {
 	action_internal(BackTab_action, IA_DEFAULT, CN, CN);
 }

 void action_Reset(GtkWidget *w, gpointer user_data)
 {
 	action_ClearSelection();
 	action_internal(Reset_action, IA_DEFAULT, CN, CN);
 }

 static void action_Connect(GtkWidget *w, gpointer user_data)
 {
 	Trace("%s Connected:%d Widget: %p",__FUNCTION__,PCONNECTED,w);

 	if(PCONNECTED)
 		return;

	// TODO (perry#5#): If there's no previous server ask for one.
 	action_ClearSelection();
	action_internal(Reconnect_action, IA_DEFAULT, CN, CN);

 }

 void action_Enter(GtkWidget *w, gpointer user_data)
 {
 	action_ClearSelection();
 	if(PCONNECTED)
		action_internal(Enter_action, IA_DEFAULT, CN, CN);
	else
		action_Connect(w,user_data);
 }

 void action_Insert(void)
 {
 	action_internal(ToggleInsert_action, IA_DEFAULT, CN, CN);
 }

 void action_Home(void)
 {
 	action_internal(Home_action, IA_DEFAULT, CN, CN);
 }

 void action_DeleteWord(void)
 {
 	action_internal(DeleteWord_action, IA_DEFAULT, CN, CN);
 }

 void action_DeleteField(void)
 {
 	action_internal(DeleteField_action, IA_DEFAULT, CN, CN);
 }

 void action_Delete(void)
 {
 	action_internal(Delete_action, IA_DEFAULT, CN, CN);
 }

 void action_BackSpace(void)
 {
 	action_internal(Erase_action, IA_DEFAULT, CN, CN);
 }

 void action_EraseEOF(void)
 {
 	action_internal(EraseEOF_action, IA_DEFAULT, CN, CN);
 }

 static void action_CrossHair(GtkWidget *w, gpointer user_data)
 {
 	do_toggle(CROSSHAIR);
 }

 static void action_BlinkCursor(GtkWidget *w, gpointer user_data)
 {
 	do_toggle(CURSOR_BLINK);
 }

 static void action_RectSelect(GtkWidget *w, gpointer user_data)
 {
 	do_toggle(RECTANGLE_SELECT);
 }

 static void action_FullScreen(GtkWidget *w, gpointer user_data)
 {
 	do_toggle(FULL_SCREEN);
 }

 static void action_ToggleMarginedPaste(void)
 {
 	do_toggle(MARGINED_PASTE);
 }


 static void action_ShowCursorPos(GtkWidget *w, gpointer user_data)
 {
 	do_toggle(CURSOR_POS);
 }

 static void action_AutoReconnect(GtkWidget *w, gpointer user_data)
 {
 	do_toggle(RECONNECT);
 }

 static void action_Disconnect(GtkWidget *w, gpointer user_data)
 {
 	Trace("%s Connected:%d Widget: %p",__FUNCTION__,PCONNECTED,w);

 	if(!PCONNECTED)
 		return;

 	action_ClearSelection();
 	action_internal(Disconnect_action, IA_DEFAULT, CN, CN);
 }

 static void action_PrintScreen(GtkWidget *w, gpointer user_data)
 {
	PrintText("g3270", GetScreenContents(TRUE));
 }

 static void action_PrintSelected(GtkWidget *w, gpointer user_data)
 {
	PrintText("g3270", GetSelection());
 }

 static void action_PrintClipboard(GtkWidget *w, gpointer user_data)
 {
	PrintText("g3270", GetClipboard());
 }

 static void action_Quit(void)
 {
 	action_Save();
 	gtk_main_quit();
 }

 void action_SelectColors(void)
 {
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

 	gtk_show_about_dialog(	GTK_WINDOW(topwindow),
							"program-name",    		PACKAGE_NAME,
							"authors", 				authors,
							"license", 				gettext( license ),
							"comments",				_( "3270 Terminal emulator for GTK."),
							"version", 				PACKAGE_VERSION,
							"translator-credits",	N_( "No Translators" ),
							"wrap-license",			TRUE,
							NULL
						);
 }

/*
	The name of the action.
	The stock id for the action, or the name of an icon from the icon theme.
	The label for the action. This field should typically be marked for translation, see gtk_action_group_set_translation_domain(). If label is NULL, the label of the stock item with id stock_id is used.
	The accelerator for the action, in the format understood by gtk_accelerator_parse().
	The tooltip for the action. This field should typically be marked for translation, see gtk_action_group_set_translation_domain().
	The function to call when the action is activated.

	http://library.gnome.org/devel/gtk/stable/gtk-Stock-Items.html

 */
 static const GtkActionEntry internal_action_entries[] =
 {
 	/* Top menus */
 	{	"FileMenu",			NULL,					N_( "_File" ),			NULL,			NULL,	NULL							},
 	{	"NetworkMenu",		NULL,					N_( "_Network" ),		NULL,			NULL,	NULL								},
 	{	"HelpMenu",			NULL,					N_( "Help" ),			NULL,			NULL,	NULL								},
 	{	"EditMenu",			NULL,					N_( "_Edit" ),			NULL,			NULL,	NULL								},
 	{	"OptionsMenu",		NULL,					N_( "_Options" ),		NULL,			NULL,	NULL								},

	/* Misc actions */
 	{	"About",			GTK_STOCK_ABOUT,		N_( "About" ),			NULL,			NULL,	G_CALLBACK(action_About)			},
 	{	"Connect",			GTK_STOCK_CONNECT,		N_( "_Connect" ),		NULL,			NULL,	G_CALLBACK(action_Connect)			},
 	{	"Disconnect",		GTK_STOCK_DISCONNECT,	N_( "_Disconnect" ),	NULL,			NULL,	G_CALLBACK(action_Disconnect)		},
 	{	"Quit",				GTK_STOCK_QUIT,			N_( "_Quit" ),			NULL,			NULL,	G_CALLBACK(action_Quit)				},
 	{	"SelectColors",		GTK_STOCK_SELECT_COLOR,	N_( "Colors" ),			NULL,			NULL,	G_CALLBACK(action_SelectColors)		},

 	/* Edit actions */
 	{	"Copy",				GTK_STOCK_COPY,			N_( "Copy" ),			NULL,			NULL,	G_CALLBACK(action_Copy)				},
 	{	"Append",			GTK_STOCK_ADD,			N_( "Add to copy" ),	NULL,			NULL,	G_CALLBACK(action_Append)			},
 	{	"Paste",			GTK_STOCK_PASTE,		N_( "Paste" ),			NULL,			NULL,	G_CALLBACK(action_Paste)			},
 	{	"PasteNext",		NULL,					N_( "Paste _next" ),	NULL,			NULL,	G_CALLBACK(action_PasteNext)		},
 	{	"Unselect",			NULL,					N_( "_Unselect" ),		NULL,			NULL,	G_CALLBACK(action_ClearSelection)	},
 	{	"Reselect",			NULL,					N_( "_Reselect" ),		NULL,			NULL,	G_CALLBACK(Reselect)				},
 	{	"SelectAll",		GTK_STOCK_SELECT_ALL,	N_( "Select all" ),		"<Alt>A",		NULL,	G_CALLBACK(action_SelectAll)		},
 	{	"Clear",			GTK_STOCK_CLEAR,		N_( "Clear fields" ),	"Clear",		NULL,	G_CALLBACK(action_Clear)			},

 	/* Printer actions */
	{	"PrintScreen",		GTK_STOCK_PRINT,		N_( "Print" ),			"Print",		NULL,	G_CALLBACK(action_PrintScreen)		},
	{	"PrintSelected",	NULL,					N_( "Print selected" ),	NULL,			NULL,	G_CALLBACK(action_PrintSelected)	},
	{	"PrintClipboard",	NULL,					N_( "Print copy" ),		NULL,			NULL,	G_CALLBACK(action_PrintClipboard)	},

	/* Select actions */
	{ 	"SelectRight",		NULL,					N_( "Select Right" ),	"<Shift>Right",	NULL,	G_CALLBACK(action_SelectRight)		},
	{ 	"SelectLeft",		NULL,					N_( "Select Left" ),	"<Shift>Left",	NULL,	G_CALLBACK(action_SelectLeft)		},
	{ 	"SelectUp",			NULL,					N_( "Select Up" ),		"<Shift>Up",	NULL,	G_CALLBACK(action_SelectUp)			},
	{ 	"SelectDown",		NULL,					N_( "Select Down" ),	"<Shift>Down",	NULL,	G_CALLBACK(action_SelectDown)		},
	{	"SelectField",		NULL,					N_( "Select Field" ),	"<Ctrl>f",		NULL,	G_CALLBACK(action_SelectField)		},

	/* Cursor Movement */
	{ 	"CursorRight",		GTK_STOCK_GO_FORWARD,	N_( "Right" ),			"Right",		NULL,	G_CALLBACK(action_Right)			},
	{ 	"CursorLeft",		GTK_STOCK_GO_BACK,		N_( "Left" ),			"Left",			NULL,	G_CALLBACK(action_Left)				},
	{ 	"CursorUp",			GTK_STOCK_GO_UP,		N_( "Up" ),				"Up",			NULL,	G_CALLBACK(action_Up)				},
	{ 	"CursorDown",		GTK_STOCK_GO_DOWN,		N_( "Down" ),			"Down",			NULL,	G_CALLBACK(action_Down)				},
	{ 	"NextField",		NULL,					N_( "Next Field" ),		"Tab",			NULL,	G_CALLBACK(action_Tab)				},
	{ 	"Enter",			NULL,					N_( "Enter" ),			"Return",		NULL,	G_CALLBACK(action_Enter)			},

	/* Terminal Actions */
	{ 	"Reset",			NULL,					N_( "Reset" ),			"<Ctrl>r",		NULL,	G_CALLBACK(action_Reset)			},
	{ 	"Escape",			NULL,					N_( "Escape" ),			"Escape",		NULL,	G_CALLBACK(action_Reset)			},
	{ 	"Home",				NULL,					N_( "Home" ),			"Home",			NULL,	G_CALLBACK(action_Home)				},
	{ 	"Return",			GTK_STOCK_APPLY,		N_( "Return" ),			"Return",		NULL,	G_CALLBACK(action_Enter)			},
	{	"DeleteWord",		NULL,					N_( "Delete Word" ),	"<Ctrl>w",		NULL,	G_CALLBACK(action_DeleteWord)		},
	{	"DeleteField",		NULL,					N_( "Delete Field" ),	"<Ctrl>u",		NULL,	G_CALLBACK(action_DeleteField)		},
	{	"Delete",			NULL,					N_( "Delete Char" ),	"Delete",		NULL,	G_CALLBACK(action_Delete)			},
	{	"Erase",			NULL,					N_( "Backspace" ),		"BackSpace",	NULL,	G_CALLBACK(action_BackSpace)		},
	{	"EraseEOF",			NULL,					N_( "EraseEOF" ),		"End",			NULL,	G_CALLBACK(action_EraseEOF)			},
	{	"PageUP",			NULL,					N_( "Page-Up" ),		"Page_Up",		NULL,	G_CALLBACK(action_PageUP) 			},
	{	"PageDown",			NULL,					N_( "Page-Down" ),		"Page_Down",	NULL,	G_CALLBACK(action_PageDown)			},
	{	"Redraw",			NULL,					N_( "Redraw screen" ),	NULL,			NULL,	G_CALLBACK(action_Redraw)			},

 };


/*
	The name of the action.
	The stock id for the action, or the name of an icon from the icon theme.
	The label for the action. This field should typically be marked for translation, see gtk_action_group_set_translation_domain().
	The accelerator for the action, in the format understood by gtk_accelerator_parse().
	The tooltip for the action. This field should typically be marked for translation, see gtk_action_group_set_translation_domain().
	The function to call when the action is activated.
	The initial state of the toggle action.

	http://library.gnome.org/devel/gtk/stable/GtkActionGroup.html#GtkToggleActionEntry
*/
 static const GtkToggleActionEntry internal_action_toggles[] =
 {
 	{	"ToggleCursorBlink",	NULL,					N_( "Blink Cursor" ),			NULL, 			NULL,	G_CALLBACK(action_BlinkCursor),				FALSE },
 	{	"ToggleCursorPos",		NULL,					N_( "Show Cursor Position" ),	NULL, 			NULL,	G_CALLBACK(action_ShowCursorPos), 			TRUE  },
 	{	"ToggleFullScreen",		NULL,					N_( "Full Screen" ),			NULL,			NULL,	G_CALLBACK(action_FullScreen),				FALSE },
 	{	"ToggleMarginedPaste",	NULL,					N_( "Margined Paste" ),			NULL, 			NULL,	G_CALLBACK(action_ToggleMarginedPaste), 	FALSE },
 	{	"ToggleCrossHair",		NULL,					N_( "Cross Hair Cursor" ),		"<Alt>X",		NULL,	G_CALLBACK(action_CrossHair),				FALSE },
 	{	"ToggleRectSelect",		NULL,					N_( "Rectangle Select" ),		NULL, 			NULL,	G_CALLBACK(action_RectSelect),				FALSE },
 	{	"ToggleReconnect",		NULL,					N_( "Auto-Reconnect" ),			NULL, 			NULL,	G_CALLBACK(action_AutoReconnect), 			FALSE },
 	{	"ToggleInsert",			NULL,					N_( "Insert" ),					"Insert", 		NULL,	G_CALLBACK(action_Insert),		 			FALSE },

 };

 GtkUIManager * LoadApplicationUI(GtkWidget *widget)
 {
	GtkUIManager 	*ui_manager = gtk_ui_manager_new(); // http://library.gnome.org/devel/gtk/stable/GtkUIManager.html
	GtkActionGroup	*actions;
	GError			*error = NULL;
	gchar			*ui;

	actions = gtk_action_group_new("InternalActions");
	gtk_action_group_set_translation_domain(actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions(actions, internal_action_entries, G_N_ELEMENTS (internal_action_entries), topwindow);
	gtk_action_group_add_toggle_actions(actions,internal_action_toggles, G_N_ELEMENTS(internal_action_toggles),0);

	gtk_ui_manager_insert_action_group(ui_manager,actions, 0);

	ui = FindSystemConfigFile("ui.xml");
	if(ui)
	{
		Log("Loading interface from %s",ui);
		if(!gtk_ui_manager_add_ui_from_file(ui_manager,ui,&error))
		{
			if(error && error->message)
				g_error( _( "Can't build Application UI: %s" ),error->message);
			else
				g_error( _( "Can't build Application UI!" ));
		}
		g_free(ui);
	}
	else
	{
		g_error( _( "Can't find UI definition file" ) );
	}

	gtk_ui_manager_ensure_update(ui_manager);
	return ui_manager;
 }

 gboolean KeyboardAction(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
 {
 	// TODO (perry#2#): Put all keyboard actions as accelerators.
 	static const struct WindowActions keyproc[] =
 	{
        G3270_ACTION( GDK_KP_Left,			GDK_SHIFT_MASK,		action_SelectLeft),
        G3270_ACTION( GDK_KP_Up,			GDK_SHIFT_MASK,		action_SelectUp),
        G3270_ACTION( GDK_KP_Right,			GDK_SHIFT_MASK,		action_SelectRight),
        G3270_ACTION( GDK_KP_Down,			GDK_SHIFT_MASK,		action_SelectDown),

        G3270_ACTION( GDK_Left,				GDK_SHIFT_MASK,		action_SelectLeft),
        G3270_ACTION( GDK_Up,				GDK_SHIFT_MASK,		action_SelectUp),
        G3270_ACTION( GDK_Right,			GDK_SHIFT_MASK,		action_SelectRight),
        G3270_ACTION( GDK_Down,				GDK_SHIFT_MASK,		action_SelectDown),

		G3270_ACTION( GDK_Left,				0,					action_Left),
		G3270_ACTION( GDK_Up,				0,					action_Up),
		G3270_ACTION( GDK_Right,			0,					action_Right),
		G3270_ACTION( GDK_Down,				0,					action_Down),

		G3270_ACTION( GDK_KP_Left,			0,					action_Left),
		G3270_ACTION( GDK_KP_Up,			0,					action_Up),
		G3270_ACTION( GDK_KP_Right,			0,					action_Right),
		G3270_ACTION( GDK_KP_Down,			0,					action_Down),

		G3270_ACTION( GDK_ISO_Left_Tab,		0,					action_BackTab),
		G3270_ACTION( GDK_Tab,				0,					action_Tab),
		G3270_ACTION( GDK_KP_Add,			GDK_NUMLOCK_MASK,	action_Tab),

		PF_ACTION(		GDK_Page_Up,		GDK_SHIFT_MASK,		"23"),
		PF_ACTION(		GDK_Page_Down,		GDK_SHIFT_MASK,		"24"),

 	};

 	int		f;
 	char	buffer[10];

	/* Is function key? */
	if(IS_FUNCTION_KEY(event))
    {
		action_ClearSelection();
        sprintf(buffer,"%d",(event->keyval - GDK_F1)+1);
        action_internal(PF_action, IA_DEFAULT, buffer, CN);
        return TRUE;
    }

    /* Check for special keyproc actions */
	for(f=0; f < (sizeof(keyproc)/sizeof(struct WindowActions));f++)
	{
		if(keyproc[f].keyval == event->keyval && (event->state & keyproc[f].state) == keyproc[f].state)
		{
			Trace("Key: %s\tAction: %s",keyproc[f].trace,keyproc[f].action_trace);
			keyproc[f].callback(widget,keyproc[f].user_data);
			return TRUE;
		}
	}


	return FALSE;
 }

