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

#include "g3270.h"
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <lib3270/localdefs.h>
#include <lib3270/toggle.h>

/*---[ Structures ]----------------------------------------------------------------------------------------*/

 #define MAX_CHR_LENGTH 3

 typedef struct _element
 {
 	gchar	ch[MAX_CHR_LENGTH];
 	short	fg;
 	short	bg;
 } ELEMENT;

#ifdef DEBUG
 	#define STATUS_CODE_DESCRIPTION(x,c,y) { #x, c, y }
#else
 	#define STATUS_CODE_DESCRIPTION(x,c,y) { c, y }
#endif

 struct status_code
 {
#ifdef DEBUG
	const char *dbg;
#endif
	int 		clr;
	const char *str;
 };


/*---[ Prototipes ]----------------------------------------------------------------------------------------*/

 static void title(char *text);
 static void setsize(int rows, int cols);
 static int  addch(int row, int col, int c, unsigned short attr);
 static void set_charset(char *dcs);
 static void redraw(void);
 static void status(STATUS_CODE id);
 static void erase(void);
 static void suspend(void);
 static void resume(void);
 static void set_cursor(CURSOR_MODE mode);
 static void set_oia(OIA_FLAG id, int on);
 static void set_compose(int on, unsigned char c, int keytype);
 static void set_lu(const char *lu);

/*---[ Globals ]-------------------------------------------------------------------------------------------*/

 const struct lib3270_screen_callbacks g3270_screen_callbacks =
 {
	sizeof(struct lib3270_screen_callbacks),

	NULL,			// void (*init)(void);
	NULL,			// void (*Error)(const char *s);
	NULL,			// void (*Warning)(const char *s);
	setsize,		// void (*setsize)(int rows, int cols);
	addch,			// void (*addch)(int row, int col, int c, int attr);
	set_charset,	// void (*charset)(char *dcs);
	title,			// void (*title)(char *text);
	NULL,			// void (*changed)(int bstart, int bend);
	NULL,			// void (*ring_bell)(void);
	redraw,			// void (*redraw)(void);
	MoveCursor,		// void (*move_cursor)(int row, int col);
	suspend,		// void (*suspend)(void);
	resume,			// void (*resume)(void);
	NULL,			// void (*reset)(int lock, const char *msg);
	status,			// void (*status)(STATUS_CODE id);
	set_compose,	// void (*compose)(int on, unsigned char c, int keytype);
	set_cursor,		// void (*cursor)(CURSOR_MODE mode);
	set_lu,			// void (*lu)(const char *lu);
	set_oia,		// void (*set)(OIA_FLAG id, int on);
	erase,			// void (*erase)(void);

 };

 int 								terminal_rows	= 0;
 int 								terminal_cols	= 0;
 int								left_margin		= 0;
 int								top_margin		= 0;

 int								fWidth			= 0;
 int								fHeight			= 0;
 int								oiaRow			= -1;

 static ELEMENT					*screen			= NULL;
 static int						szScreen		= 0;
 static char						*charset		= NULL;
 static gboolean					draw			= FALSE;
 static const struct status_code	*sts_data		= NULL;
 static unsigned char			compose			= 0;
 static gchar						*luname			= 0;

 static gboolean	oia_flag[OIA_FLAG_USER];

/*---[ Implement ]-----------------------------------------------------------------------------------------*/

 static void set_compose(int on, unsigned char c, int keytype)
 {
 	if(on)
 		compose = c;
	else
		compose = 0;
 }

 static void suspend(void)
 {
 	draw = FALSE;
 }

 static void resume(void)
 {
 	draw = TRUE;
 	if(terminal && pixmap)
 	{
		DrawScreen(terminal, color, pixmap);
		gtk_widget_queue_draw(terminal);
 	}
 }

 static void title(char *text)
 {
 	Trace("Window %p title: \"%s\"",topwindow,text);
 	if(topwindow)
		gtk_window_set_title(GTK_WINDOW(topwindow),text);

 }

 void ParseInput(const gchar *string)
 {
    gchar *input = g_convert(string, -1, charset ? charset : "ISO-8859-1", "UTF-8", NULL, NULL, NULL);

    if(!input)
    {
        Log("Error converting string \"%s\" to %s",string,charset);
        return;
    }

    Trace("Converted input string: \"%s\"",input);

    // NOTE (perry#1#): Is it the best way?
    Input_String((const unsigned char *) input);

    g_free(input);
 }

 static int addch(int row, int col, int c, unsigned short attr)
 {
	gchar	in[2] = { (char) c, 0 };
	gsize	sz;
	gchar	*ch;
	ELEMENT temp;
 	ELEMENT *el;
 	int		pos = (row*terminal_cols)+col;

 	if(!screen || col >= terminal_cols || row >= terminal_rows)
		return EINVAL;

	if(pos > szScreen)
		return EFAULT;

	memset(&temp,0,sizeof(temp));

	if(c)
	{
		if(charset)
		{
			ch = g_convert(in, -1, "UTF-8", charset, NULL, &sz, NULL);

			if(sz < MAX_CHR_LENGTH)
			{
				memcpy(temp.ch,ch,sz);
			}
			else
			{
				Log("Invalid size when converting \"%s\" to \"%s\"",in,ch);
				memset(temp.ch,0,MAX_CHR_LENGTH);
			}
			g_free(ch);

		}
		else
		{
			memcpy(temp.ch,in,2);
		}

	}
	else
	{
		memset(temp.ch,0,MAX_CHR_LENGTH);
	}

	// TODO (perry#1#): Get the correct colors
	temp.bg = (attr & 0xF0) >> 4;
	temp.fg = (attr & 0x0F);

	// Get element entry in the buffer, update ONLY if changed
 	el = screen + pos;

	if(!memcmp(el,&temp,sizeof(ELEMENT)))
		return 0;

	memcpy(el,&temp,sizeof(ELEMENT));

	if(draw && terminal && pixmap)
	{
		// Update pixmap, queue screen redraw.
		gint x, y;
		PangoLayout *layout = gtk_widget_create_pango_layout(terminal,el->ch);

		x = left_margin + (col * fWidth);
		y = top_margin + (row * fHeight);

		gdk_draw_layout_with_colors(	pixmap,
										terminal->style->fg_gc[GTK_WIDGET_STATE(terminal)],
										x,y,
										layout,
										color+el->fg,
										color+el->bg );

		g_object_unref(layout);

		gtk_widget_queue_draw_area(terminal,x,y,fWidth,fHeight);
	}

	return 0;
 }

 static void setsize(int rows, int cols)
 {
	g_free(screen);
	screen = NULL;

	if(rows && cols)
	{
		szScreen = rows*cols;
		screen = g_new0(ELEMENT,szScreen);
		terminal_rows = rows;
		terminal_cols = cols;
	}

 	Trace("Terminal set to %d rows with %d cols, screen set to %p",rows,cols,screen);

 }

 static void redraw(void)
 {
 	DrawScreen(terminal,color,pixmap);
 }

 /**
  * Erase screen.
  *
  */
 static void erase(void)
 {
	GdkGC		*gc;
	int			width;
	int			height;
	int			f;

	Trace("Erasing screen! (pixmap: %p screen: %p)",pixmap,screen);

	if(screen)
	{
		memset(screen,0,szScreen);
		for(f=0;f<szScreen;f++)
			screen[f].ch[0] = ' ';
	}

	if(terminal && pixmap)
	{
		gc = terminal->style->fg_gc[GTK_WIDGET_STATE(terminal)];
		gdk_drawable_get_size(pixmap,&width,&height);
		gdk_gc_set_foreground(gc,color);
		gdk_draw_rectangle(pixmap,gc,1,0,0,width,height);
		if(oiaRow > 0)
			DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw(terminal);
	}
 }

 static void set_lu(const char *lu)
 {
 	if(luname)
 	{
		g_free(luname);
		luname = NULL;
 	}

 	if(lu)
		luname = g_convert(lu, -1, "UTF-8", charset ? charset : "ISO-8859-1", NULL, NULL, NULL);

	if(terminal && pixmap && oiaRow > 0)
	{
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw(terminal);
	}

 }

 void DrawOIA(GtkWidget *widget, GdkColor *clr, GdkDrawable *draw)
 {
	GdkGC 		*gc		= widget->style->fg_gc[GTK_WIDGET_STATE(widget)];
	PangoLayout *layout;
	int   		row		= oiaRow;
	int			width	= (fWidth*terminal_cols);
	GdkColor	*bg		= clr+TERMINAL_COLOR_OIA_BACKGROUND;
	GdkColor	*fg		= clr+TERMINAL_COLOR_OIA;
	int			col		= left_margin;
	char		str[11];

	gdk_gc_set_foreground(gc,bg);
	gdk_draw_rectangle(draw,gc,1,left_margin,row,width,fHeight+1);

	gdk_gc_set_foreground(gc,fg);
	gdk_draw_line(draw,gc,left_margin,row,left_margin+width,row);
	row++;

	layout = gtk_widget_create_pango_layout(widget,"4");
	gdk_draw_layout_with_colors(draw,gc,col,row,layout,bg,fg);
	col += fWidth;

	if(oia_flag[OIA_FLAG_UNDERA])
	{
		pango_layout_set_text(layout,(IN_E) ? "B" : "A",-1);
		gdk_draw_layout_with_colors(draw,gc,col,row,layout,fg,bg);
	}

	col += fWidth;

	str[1] = 0;
	if(IN_ANSI)
		*str = 'N';
	else if(oia_flag[OIA_FLAG_BOXSOLID])
		*str = ' ';
	else if(IN_SSCP)
		*str = 'S';
	else
		*str = '?';

	pango_layout_set_text(layout,str,-1);
	gdk_draw_layout_with_colors(draw,gc,col,row,layout,bg,fg);

	col += (fWidth<<1);

	if(sts_data && sts_data->str)
	{
		pango_layout_set_text(layout,sts_data->str,-1);
		gdk_draw_layout_with_colors(draw,gc,col,row,layout,clr+sts_data->clr,bg);
	}

	memset(str,' ',10);

	col = left_margin+(fWidth*(terminal_cols-36));

	if(compose)
	{
		str[0] = 'C';
		str[1] = compose;
	}

	// NOTE (perry#9#): I think it would be better if we use some svg images instead of text.
	str[3] = oia_flag[OIA_FLAG_TYPEAHEAD]	? 'T' : ' ';
	str[4] = oia_flag[OIA_FLAG_REVERSE] 	? 'R' : ' ';
	str[5] = oia_flag[OIA_FLAG_INSERT]		? 'I' : ' ';
	str[6] = oia_flag[OIA_FLAG_PRINTER]		? 'P' : ' ';
	str[7] = 0;

	pango_layout_set_text(layout,str,-1);
	gdk_draw_layout_with_colors(draw,gc,col,row,layout,fg,bg);


	if(luname)
	{
		pango_layout_set_text(layout,luname,-1);
		gdk_draw_layout_with_colors(draw,gc,left_margin+(fWidth*(terminal_cols-25)),row,layout,clr+TERMINAL_COLOR_OIA_LU,bg);
	}

	if(Toggled(CURSOR_POS))
	{
		sprintf(str,"%03d/%03d",cRow,cCol);
		pango_layout_set_text(layout,str,-1);
		gdk_draw_layout_with_colors(draw,gc,left_margin+(fWidth*(terminal_cols-7)),row,layout,clr+TERMINAL_COLOR_OIA_CURSOR,bg);
	}

	g_object_unref(layout);

 }

 static void set_oia(OIA_FLAG id, int on)
 {
 	if(id > OIA_FLAG_USER)
		return;

 	oia_flag[id] = on;

 	if(terminal && pixmap)
 	{
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw(terminal); // FIXME (perry#2#): Redraw only the oia.
 	}
 }


 /**
  * Draw entire buffer.
  *
  * @param	widget	Widget to be used as reference.
  * @param	clr		List of colors to be used when drawing.
  * @param	draw	The image destination.
  *
  */
 int DrawScreen(GtkWidget *widget, GdkColor *clr, GdkDrawable *draw)
 {
	GdkGC		*gc;
	PangoLayout *layout;
	ELEMENT		*el			= screen;
	int			x;
	int			y;
	int			row;
	int			col;
	int			width;
	int			height;

	if(!el)
		return -1;

	gc = widget->style->fg_gc[GTK_WIDGET_STATE(widget)];

	// Fill pixmap with background color
	gdk_drawable_get_size(draw,&width,&height);
	gdk_gc_set_foreground(gc,clr);
	gdk_draw_rectangle(draw,gc,1,0,0,width,height);

	// Draw screen contens
	layout = gtk_widget_create_pango_layout(widget," ");
	pango_layout_get_pixel_size(layout,&fWidth,&fHeight);

	y = top_margin;
	for(row = 0; row < terminal_rows;row++)
	{
		x = left_margin;
		for(col = 0; col < terminal_cols;col++)
		{
			// Set character attributes in the layout
			if(el->ch && *el->ch != ' ' && *el->ch)
			{
				pango_layout_set_text(layout,el->ch,-1);
//				pango_layout_get_pixel_size(layout,&width,&height);
				gdk_draw_layout_with_colors(draw,gc,x,y,layout,clr+el->fg,clr+el->bg);
			}

			el++;
			x += fWidth;
		}
		y += fHeight;
	}

	g_object_unref(layout);

	oiaRow = y;

	return 0;

 }


 static void set_charset(char *dcs)
 {
 	Trace("Screen charset: %s",dcs);
	g_free(charset);
	charset = g_strdup(dcs);
 }

 static STATUS_CODE last_id = (STATUS_CODE) -1;

 static void status(STATUS_CODE id)
 {
 	static const struct status_code tbl[] =
 	{
		STATUS_CODE_DESCRIPTION( STATUS_CODE_BLANK,				TERMINAL_COLOR_OIA_STATUS_OK, 		"" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_SYSWAIT,			TERMINAL_COLOR_OIA_STATUS_OK, 		"X System" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_TWAIT,				TERMINAL_COLOR_OIA_STATUS_OK, 		"X Wait" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_CONNECTED,			TERMINAL_COLOR_OIA_STATUS_OK, 		"" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_DISCONNECTED,		TERMINAL_COLOR_OIA_STATUS_INVALID, 	"X Disconnected" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_AWAITING_FIRST,	TERMINAL_COLOR_OIA_STATUS_OK,		"X" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_MINUS,				TERMINAL_COLOR_OIA_STATUS_OK,		"X -f" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_PROTECTED,			TERMINAL_COLOR_OIA_STATUS_INVALID,	"X Protected" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_NUMERIC,			TERMINAL_COLOR_OIA_STATUS_INVALID,	"X Numeric" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_OVERFLOW,			TERMINAL_COLOR_OIA_STATUS_INVALID,	"X Overflow" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_INHIBIT,			TERMINAL_COLOR_OIA_STATUS_INVALID,	"X Inhibit" ),
		STATUS_CODE_DESCRIPTION( STATUS_CODE_X,					TERMINAL_COLOR_OIA_STATUS_INVALID,	"X" ),
	};

	/* Check if status has changed to avoid unnecessary redraws */
	if(id == last_id && !sts_data)
		return;

	if(id >= STATUS_CODE_USER)
	{
		Log("Unexpected status code %d",(int) id);
		return;
	}

	last_id = id;

	sts_data = tbl+id;

	Trace("Status changed to %s (%s)",sts_data->str,sts_data->dbg);

	DrawOIA(terminal,color,pixmap);
	gtk_widget_queue_draw(terminal);

 }

 static void set_cursor(CURSOR_MODE mode)
 {
 	if(terminal && terminal->window && mode < CURSOR_MODE_USER)
		gdk_window_set_cursor(terminal->window,wCursor[mode]);

 }
