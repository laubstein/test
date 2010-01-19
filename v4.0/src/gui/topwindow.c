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
 * Este programa está nomeado como topwindow.c e possui 508 linhas de código.
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
 #include "uiparser.h"

 #include <gdk/gdkkeysyms.h>
 #include <lib3270/config.h>

 #include <globals.h>
 #include <lib3270/kybdc.h>
 #include <lib3270/actionsc.h>
 #include <lib3270/toggle.h>
 #include <lib3270/plugins.h>

/*---[ Globals ]------------------------------------------------------------------------------------------------*/

 GtkWidget	*topwindow 		= NULL;
 GList 		*main_icon		= NULL;
 gchar		*program_logo	= NULL;

#ifdef MOUSE_POINTER_CHANGE
 GdkCursor	*wCursor[CURSOR_MODE_3270];
#endif

/*---[ Implement ]----------------------------------------------------------------------------------------------*/

 static gboolean delete_event( GtkWidget *widget, GdkEvent  *event, gpointer data )
 {
 	Trace("%s","Destroying top-window");
 	action_Save();
 	gtk_main_quit();
 	Trace("%s","Ok");
    return FALSE;
 }

 static void destroy( GtkWidget *widget, gpointer   data )
 {
 	Trace("%s","Destroying top-window");
 	topwindow = NULL;
	program_quit();
 	Trace("%s","Ok");
 }

 static gboolean key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
 {
 	// http://developer.gnome.org/doc/API/2.0/gtk/GtkWidget.html#GtkWidget-key-press-event
/*
    if(IS_FUNCTION_KEY(event))
		return(PFKey(GetFunctionKey(event)));
    return FALSE;

*/
	if(KeyboardAction(widget,event,user_data))
		return TRUE;

	return FALSE;

 }

 static gboolean key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
 {
/*

    if(IS_FUNCTION_KEY(event))
    {
    	GetFunctionKey(event);
    	return TRUE;
    }
*/
 	return FALSE;
 }

 static void activate_font(GtkCheckMenuItem *item, gchar *text)
 {
	if(gtk_check_menu_item_get_active(item))
	{
		gchar *vlr	= strdup(text);
		char *ptr	= strchr(vlr,',');

		if(ptr)
		{
			*(ptr++) = 0;
			SetString("Terminal","FontSizes",ptr);
		}
		else
		{
			SetString("Terminal","FontSizes",NULL);
		}

		SetString("Terminal","Font",vlr);

		g_free(vlr);

		FontChanged();
	}
 }

 static void LoadSystemFonts(GtkWidget *widget, GtkWidget *topmenu, const gchar *selected)
 {
	// Stolen from http://svn.gnome.org/svn/gtk+/trunk/gtk/gtkfontsel.c
	PangoFontFamily **families;
	gint 			n_families, i;

 	GtkWidget		*menu	= gtk_menu_new();
 	GSList 			*group	= NULL;

	pango_context_list_families(gtk_widget_get_pango_context(widget),&families, &n_families);

	Trace("Font families: %d",n_families);

	for(i=0; i<n_families; i++)
    {
    	if(pango_font_family_is_monospace(families[i]))
    	{
			const gchar 	*name = pango_font_family_get_name (families[i]);
			GtkWidget		*item = gtk_radio_menu_item_new_with_label(group,name);
			gchar			*ptr;

			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
			ptr = g_strdup(name);
			g_object_set_data_full(G_OBJECT(item),"FontName",ptr,g_free);
			g_signal_connect(G_OBJECT(item),"toggled",G_CALLBACK(activate_font),ptr);

			gtk_widget_show(item);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

			if(!strcmp(name,selected))
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),TRUE);

    	}
    }

	g_free(families);

	gtk_widget_show_all(menu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(topmenu),menu);

 }
// static void LoadFontMenu(GtkWidget *widget, GtkWidget *menu_item)

 GtkWidget * widget_from_action_name(const gchar *name)
 {
 	GtkAction *action = get_action_by_name(name);

	if(action)
		return g_object_get_data(G_OBJECT(action),"ui_widget");

	return NULL;
 }

 static void LoadFontMenu(void)
 {
	gchar		*selected;
	GtkWidget	*menu = widget_from_action_name("fontselect");
	selected = GetString("Terminal","Font","Courier");
	LoadSystemFonts(topwindow, menu, selected);
	g_free(selected);
	return;
 }

 GdkPixbuf * LoadLogo(void)
 {
	GdkPixbuf	*pix = NULL;
	gchar		*filename;

	if(program_logo && g_file_test(program_logo,G_FILE_TEST_IS_REGULAR))
		return gdk_pixbuf_new_from_file(program_logo,NULL);

	filename = g_strdup_printf("%s%c%s.%s", program_data, G_DIR_SEPARATOR, PROGRAM_NAME, LOGOEXT);

	if(g_file_test(filename,G_FILE_TEST_IS_REGULAR))
		pix = gdk_pixbuf_new_from_file(filename, NULL);

	Trace("pixbuf(%s): %p",filename,pix);
	g_free(filename);

	return pix;
 }

#ifdef USE_PRIMARY_SELECTION

 static void selection_owner_changed(GtkClipboard *clipboard, GdkEventOwnerChange *event, gpointer user_data)
 {
 	if(!GTK_WIDGET_HAS_FOCUS(terminal))
		action_ClearSelection();
 }

#endif

#if defined( USE_SELECTIONS ) && !defined(WIN32)

 static void clipboard_text_received(GtkClipboard *clipboard, const gchar *text, gpointer data)
 {
 	set_action_group_sensitive_state(ACTION_GROUP_PASTE,text ? TRUE : FALSE);
 }

 static void clipboard_owner_changed(GtkClipboard *clipboard, GdkEventOwnerChange *event, gpointer user_data)
 {
	gtk_clipboard_request_text(clipboard,clipboard_text_received,0);
 }

#endif

 int CreateTopWindow(void)
 {
#ifdef MOUSE_POINTER_CHANGE

	#ifdef WIN32

		static const gchar *cr[CURSOR_MODE_3270] = {	"arrow",
														"wait",
														"arrow",

														"sizenwse",	// Top-left
														"sizenesw",	// Top-right
														"sizens",	// Top
														"sizenesw",	// Bottom-left
														"sizenwse",	// Bottom-right
														"sizens",	// Bottom
														"sizewe",	// Left
														"sizewe",	// Right
														"sizeall"	// Inside

													};

	#else

		static int 		cr[CURSOR_MODE_3270] = { 	GDK_XTERM,
														GDK_WATCH,
														GDK_X_CURSOR,

														GDK_TOP_LEFT_CORNER, 		// Top-left
														GDK_TOP_RIGHT_CORNER,		// Top-right
														GDK_TOP_SIDE,				// Top
														GDK_BOTTOM_LEFT_CORNER,		// Bottom-left
														GDK_BOTTOM_RIGHT_CORNER,	// Bottom-right
														GDK_BOTTOM_SIDE,			// Bottom
														GDK_LEFT_SIDE,				// Left
														GDK_RIGHT_SIDE,				// Right
														GDK_FLEUR					// Inside

													};

	#endif

 	int						f;

#endif


	GtkWidget				*toolbar;
 	GtkWidget				*vbox;
 	GtkWidget				*hbox;
	GtkUIManager			*manager;
	GdkPixbuf				*pix;
	gchar 					*ptr;

	topwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_object_set_data(G_OBJECT(topwindow),"pw3270_config",	(gpointer) GetConf());
	g_object_set_data(G_OBJECT(topwindow),"pw3270_dpath",	(gpointer) program_data);

	init_actions(topwindow);

	pix = LoadLogo();

	if(pix)
	{
		main_icon = g_list_append(main_icon, pix);
		gtk_window_set_icon_list(GTK_WINDOW(topwindow),main_icon);
	}

#ifdef MOUSE_POINTER_CHANGE

	#ifdef WIN32
		for(f=0;f<CURSOR_MODE_3270;f++)
			wCursor[f] = gdk_cursor_new_from_name(gdk_display_get_default(), cr[f]);
	#else
		for(f=0;f<CURSOR_MODE_3270;f++)
			wCursor[f] = gdk_cursor_new(cr[f]);
	#endif

#endif

	g_signal_connect(G_OBJECT(topwindow),	"delete_event", 		G_CALLBACK(delete_event),			0);
	g_signal_connect(G_OBJECT(topwindow),	"destroy", 				G_CALLBACK(destroy),				0);
    g_signal_connect(G_OBJECT(topwindow),	"key-press-event",		G_CALLBACK(key_press_event),		0);
    g_signal_connect(G_OBJECT(topwindow), 	"key-release-event",	G_CALLBACK(key_release_event),		0);

	/* Create terminal window */
	if(!CreateTerminalWindow())
		return -1;

	/* Create UI elements */
	toolbar = gtk_vbox_new(FALSE,0);
    vbox = gtk_vbox_new(FALSE,0);
    hbox = gtk_hbox_new(FALSE,0);

	manager = load_application_ui(topwindow, GTK_BOX(toolbar), GTK_BOX(vbox), GTK_BOX(hbox));
	if(manager)
	{
		static const struct _popup
		{
			const char 	*name;
			GtkWidget		**widget;
		} popup[] =
		{
			{ "defaultpopup",	&DefaultPopup	},
			{ "selectionpopup",	&SelectionPopup	}
		};

		int f;

		for(f=0;f < G_N_ELEMENTS(popup);f++)
		{
			*popup[f].widget = widget_from_action_name(popup[f].name);
			if(*popup[f].widget)
			{
				g_object_ref(*popup[f].widget);
			}
		}

		LoadFontMenu();

		g_object_unref(manager);
	}

	FontChanged();

	gtk_box_pack_start(GTK_BOX(vbox), terminal, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox,TRUE,TRUE,0);

	gtk_box_pack_start(GTK_BOX(toolbar), hbox, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(topwindow),toolbar);

	gtk_widget_show(terminal);
	gtk_widget_show(toolbar);
	gtk_widget_show(hbox);
	gtk_widget_show(vbox);

	gtk_window_set_role(GTK_WINDOW(topwindow), PACKAGE_NAME "_TOP" );

	action_ClearSelection();
	ClearClipboard();

#ifdef USE_PRIMARY_SELECTION

	g_signal_connect(	G_OBJECT(gtk_widget_get_clipboard(topwindow,GDK_SELECTION_PRIMARY)),
						"owner-change",G_CALLBACK(selection_owner_changed),0 );

#endif

	set_action_group_sensitive_state(ACTION_GROUP_ONLINE,FALSE);
	set_action_group_sensitive_state(ACTION_GROUP_OFFLINE,TRUE);
	set_action_group_sensitive_state(ACTION_GROUP_CLIPBOARD,FALSE);

	set_action_sensitive_by_name("Reselect",FALSE);
	set_action_sensitive_by_name("PasteNext",FALSE);

#if defined( USE_SELECTIONS ) && !defined( WIN32 )

	set_action_group_sensitive_state(ACTION_GROUP_PASTE,FALSE);

	g_signal_connect(	G_OBJECT(gtk_widget_get_clipboard(topwindow,GDK_NONE)),"owner-change",G_CALLBACK(clipboard_owner_changed),0 );

	// TODO (perry#1#): Request clipboard only when online to avoid enabling paste options when offline.
	gtk_clipboard_request_text(gtk_widget_get_clipboard(topwindow,GDK_NONE),clipboard_text_received,0);

#else

	set_action_group_sensitive_state(ACTION_GROUP_PASTE,FALSE);

#endif

	gtk_window_set_default_size(GTK_WINDOW(topwindow),590,430);
	ptr = GetString("TopWindow","Title","");
	settitle(ptr);
	g_free(ptr);

	action_Restore();

	gtk_window_set_position(GTK_WINDOW(topwindow),GTK_WIN_POS_CENTER);

	return 0;
 }

 void program_quit(void)
 {
 	host_disconnect(0);
	gtk_main_quit();
 }
