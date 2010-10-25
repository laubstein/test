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
 * Este programa está nomeado como action_calls.c e possui - linhas de código.
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
 #include "actions.h"

/*---[ Implement ]----------------------------------------------------------------------------------------------*/

 PW3270_ACTION( sethostname )
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
		gtk_entry_set_text(port,GetString("Network","DefaultPort","23"));
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
				SetString("Network","Hostname",buffer);
			}
			break;

		case GTK_RESPONSE_REJECT:
			again = FALSE;
			break;
 		}
 	}

	gtk_widget_destroy(dialog);

 }

 PW3270_ACTION( connect )
 {
    const gchar *host;

 	Trace("%s Connected:%d",__FUNCTION__,PCONNECTED);

 	if(PCONNECTED)
 		return;

 	action_clearselection(0);

	host = GetString("Network","Hostname",CN);

    if(host == CN)
    {
        action_sethostname(action);
    }
    else
    {
    	int rc;

        DisableNetworkActions();
        gtk_widget_set_sensitive(topwindow,FALSE);
        RunPendingEvents(0);

		rc = host_connect(host,1);

		if(rc && rc != EAGAIN)
		{
			// Connection failed, notify user
			GtkWidget *dialog;

			dialog = gtk_message_dialog_new(	GTK_WINDOW(topwindow),
												GTK_DIALOG_DESTROY_WITH_PARENT,
												GTK_MESSAGE_WARNING,
												GTK_BUTTONS_OK,
												_(  "Negotiation with %s failed" ), host);

			gtk_window_set_title(GTK_WINDOW(dialog), _( "Connection error" ) );

			switch(rc)
			{
			case EBUSY:	// System busy (already connected or connecting)
				gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog) ,"%s", _( "Connection already in progress" ));
				break;

			case ENOTCONN:	// Connection failed
				gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog) ,"%s", _( "Can't connect" ));
				break;

			case -1:
				gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog) ,_( "Unexpected error" ));
				break;

			default:
				gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog) ,_( "The error code was %d (%s)" ), rc, strerror(rc));
			}

			gtk_dialog_run(GTK_DIALOG (dialog));
			gtk_widget_destroy(dialog);


		}

		Trace("Topwindow: %p Terminal: %p",topwindow,terminal);

		if(topwindow)
			gtk_widget_set_sensitive(topwindow,TRUE);

		if(terminal)
			gtk_widget_grab_focus(terminal);

    }

	Trace("%s ends",__FUNCTION__);

 }

 PW3270_ACTION( enter )
 {
 	action_clearselection(0);
 	if(PCONNECTED)
		lib3270_enter();
	else
		action_connect(action);
 }


 PW3270_ACTION( about )
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
#if GTK_CHECK_VERSION(2,12,0)
							"program-name",    		program_name,
#else
							"name",    				program_name,
#endif
							"logo",					logo,
							"authors", 				authors,
							"license", 				gettext( license ),
#if defined( HAVE_IGEMAC )
							"comments",				_( "3270 Terminal emulator for Gtk-OSX."),
#elif defined( HAVE_LIBGNOME )
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


