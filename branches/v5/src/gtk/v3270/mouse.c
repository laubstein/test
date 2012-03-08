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
 * Este programa está nomeado como mouse.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

 #include <gtk/gtk.h>
 #include <gdk/gdk.h>
 #include <pw3270.h>
 #include "v3270.h"
 #include "private.h"
 #include <lib3270/selection.h>

/*--[ Implement ]------------------------------------------------------------------------------------*/

static int decode_position(v3270 *widget, gdouble x, gdouble y)
{
	GdkPoint point;
	int r,c;

	point.x = ((x-widget->metrics.left)/widget->metrics.width);
	point.y = ((y-widget->metrics.top)/widget->metrics.spacing);

	lib3270_get_screen_size(widget->host,&r,&c);

	if(point.x >= 0 && point.y >= 0 && point.x < c && point.y < r)
		return (point.y * c) + point.x;

	return -1;
}

gboolean v3270_button_press_event(GtkWidget *widget, GdkEventButton *event)
{
	int baddr = decode_position(GTK_V3270(widget),event->x,event->y);

	if(baddr < 0)
		return FALSE;

	trace("%s button=%d type=%d",__FUNCTION__,event->button,event->type);

	switch(event->button)
	{
	case 1:
		lib3270_set_cursor_address(GTK_V3270(widget)->host,baddr);
		GTK_V3270(widget)->selecting = 1;
		lib3270_clear_selection(GTK_V3270(widget)->host);
		break;

	default:
		trace("%s button=%d type=%d",__FUNCTION__,event->button,event->type);
	}

	return FALSE;
}

gboolean v3270_button_release_event(GtkWidget *widget, GdkEventButton*event)
{
	switch(event->button)
	{
	case 1:
		GTK_V3270(widget)->selecting = 0;
		break;

	default:
		trace("%s button=%d type=%d",__FUNCTION__,event->button,event->type);
	}


	return FALSE;
}


gboolean v3270_motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
	int baddr = decode_position(GTK_V3270(widget),event->x,event->y);

	if(baddr < 0)
		return FALSE;

	if(GTK_V3270(widget)->selecting)
		lib3270_select_to(GTK_V3270(widget)->host,baddr);

	return FALSE;
}
