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
 * Este programa está nomeado como clipboard.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

 #include <gtk/gtk.h>
 #include <pw3270.h>
 #include <malloc.h>
 #include "v3270.h"
 #include "private.h"
 #include <lib3270/selection.h>

/*--[ Globals ]--------------------------------------------------------------------------------------*/

 enum
 {
 	CLIPBOARD_TYPE_TEXT,
 };

 static const GtkTargetEntry targets[] =
 {
	{ "COMPOUND_TEXT", 	0, CLIPBOARD_TYPE_TEXT },
	{ "UTF8_STRING", 	0, CLIPBOARD_TYPE_TEXT },
 };

/*--[ Implement ]------------------------------------------------------------------------------------*/

static void clipboard_clear(GtkClipboard *clipboard, GObject *obj)
{
	trace("%s widget=%p",__FUNCTION__,obj);

}

static void clipboard_get(GtkClipboard *clipboard, GtkSelectionData *selection, guint target, GObject *obj)
{
	v3270 * widget = GTK_V3270(obj);

	trace("%s: widget=%p target=\"%s\"",__FUNCTION__,obj,targets[target].target);

	switch(target)
	{
	case CLIPBOARD_TYPE_TEXT:
		if(!widget->clipboard)
			lib3270_ring_bell(widget->host);
		else
			gtk_selection_data_set_text(selection,widget->clipboard,-1);
		break;

	default:
		g_warning("Unexpected clipboard type %d\n",target);
	}
}

const gchar * v3270_get_selected_text(GtkWidget *widget)
{
	v3270 *terminal;
	gchar *text;

	g_return_val_if_fail(GTK_IS_V3270(widget),NULL);

	terminal = GTK_V3270(widget);

	if(terminal->clipboard)
	{
		g_free(terminal->clipboard);
		terminal->clipboard = NULL;
	}

	text = lib3270_get_selected(terminal->host);

	if(!text)
	{
		g_signal_emit(widget,v3270_widget_signal[SIGNAL_CLIPBOARD], 0, FALSE);
		lib3270_ring_bell(terminal->host);
		return NULL;
	}

	terminal->clipboard = g_convert(text, -1, "UTF-8", lib3270_get_charset(terminal->host), NULL, NULL, NULL);

	free(text);

	return terminal->clipboard;
}

void v3270_copy_clipboard(GtkWidget *widget)
{
	const gchar		* text		= v3270_get_selected_text(widget);
	GtkClipboard	* clipboard	= gtk_widget_get_clipboard(widget,GDK_SELECTION_CLIPBOARD);

	if(!text)
		return;

	if(gtk_clipboard_set_with_owner(	clipboard,
										targets,
										G_N_ELEMENTS(targets),
										(GtkClipboardGetFunc)	clipboard_get,
										(GtkClipboardClearFunc) clipboard_clear,
										G_OBJECT(widget)
										))
	{
		gtk_clipboard_set_can_store(clipboard,targets,1);
	}

	g_signal_emit(widget,v3270_widget_signal[SIGNAL_CLIPBOARD], 0, TRUE);

}

void v3270_paste_string(GtkWidget *widget, const gchar *text, const gchar *encoding)
{
 	gchar 		* buffer 	= NULL;
 	H3270		* session 	= v3270_get_session(widget);
	const gchar * charset 	= lib3270_get_charset(session);
 	gboolean	  next;

 	if(!text)
		return;
	else if(g_strcasecmp(encoding,charset))
		buffer = g_convert(text, -1, charset, encoding, NULL, NULL, NULL);
	else
		buffer = g_strdup(text);

    if(!buffer)
    {
    	/* Conversion failed, update special chars and try again */
    	int f;

    	static const struct _xlat
    	{
    		const gchar *from;
    		const gchar *to;
    	} xlat[] =
    	{
    		{ "–",		"-"		},
    		{ "→",		"->"	},
    		{ "←",		"<-" 	},
    		{ "©",		"(c)"	},
    		{ "↔",		"<->"	},
    		{ "™",		"(TM)"	},
    		{ "®",		"(R)"	},
    		{ "“",		"\""	},
    		{ "”",		"\""	},
    		{ "…",		"..."	},
    		{ "•",		"*"		},
    		{ "․",		"."		},
    		{ "·",		"*"		},

    	};

		gchar *string = g_strdup(text);

		// FIXME (perry#1#): Is there any better way for a "sed" here?
		for(f=0;f<G_N_ELEMENTS(xlat);f++)
		{
			gchar *ptr = g_strstr_len(string,-1,xlat[f].from);

			if(ptr)
			{
				gchar *old = string;
				gchar **tmp = g_strsplit(old,xlat[f].from,-1);
				string = g_strjoinv(xlat[f].to,tmp);
				g_strfreev(tmp);
				g_free(old);
			}
		}

		buffer = g_convert(string, -1, charset, encoding, NULL, NULL, NULL);

		if(!buffer)
		{
			// Still failing, convert line by line
			gchar **ln = g_strsplit(string,"\n",-1);

			for(f=0;ln[f];f++)
			{
				GError	*error	= NULL;
				gchar	*str	= g_convert(ln[f], -1, charset, encoding, NULL, NULL, &error);

				if(!str)
				{
					GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW( gtk_widget_get_toplevel(widget)),
																GTK_DIALOG_DESTROY_WITH_PARENT,
																GTK_MESSAGE_ERROR,
																GTK_BUTTONS_OK,
																_(  "Can't convert line %d from %s to %s" ),f+1, encoding, charset);

					gtk_window_set_title(GTK_WINDOW(dialog), _( "Charset error" ) );
					gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),"%s\n%s",error->message, ln[f]);

					gtk_dialog_run(GTK_DIALOG (dialog));
					gtk_widget_destroy(dialog);

					break;
				}
				else
				{
					g_free(str);
				}

			}
			g_strfreev(ln);

		}

		g_free(string);
    }

	if(buffer)
	{
		/* Remove TABS */
		gchar *ptr;

		for(ptr = buffer;*ptr;ptr++)
		{
			if(*ptr == '\t')
				*ptr = ' ';
		}
	}
    else
	{
		g_signal_emit(widget,v3270_widget_signal[SIGNAL_PASTENEXT], 0, FALSE);
		return;
	}

	next = lib3270_paste(session,(unsigned char *) buffer) ? TRUE : FALSE;

	trace("Pastenext is %s",next ? "On" : "Off");

	g_free(buffer);

	g_signal_emit(widget,v3270_widget_signal[SIGNAL_PASTENEXT], 0, next);

}

static void text_received(GtkClipboard *clipboard, const gchar *text, GtkWidget *widget)
{
	v3270_paste_string(widget,text,"UTF-8");
}

void v3270_paste_clipboard(GtkWidget *widget)
{
	gtk_clipboard_request_text(gtk_widget_get_clipboard(widget,GDK_SELECTION_CLIPBOARD),(GtkClipboardTextReceivedFunc) text_received,(gpointer) widget);
}

