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
#include "config.h"
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <lib3270/localdefs.h>
#include <lib3270/toggle.h>

#include "locked.bm"
#include "unlocked.bm"
#include "shift.bm"

/*---[ Structures ]----------------------------------------------------------------------------------------*/

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
	int			clr;
	const char	*string;
 };


/*---[ Prototipes ]----------------------------------------------------------------------------------------*/

 static void	title(char *text);
 static void	setsize(int rows, int cols);
 static int  	addch(int row, int col, int c, unsigned short attr);
 static void	set_charset(char *dcs);
 static void	erase(void);
 static void	suspend(void);
 static void	resume(void);
 static void	set_cursor(CURSOR_MODE mode);
 static void	set_oia(OIA_FLAG id, int on);
 static void	set_compose(int on, unsigned char c, int keytype);
 static void	set_lu(const char *lu);
 static void	DrawImage(GdkDrawable *drawable, GdkGC *gc, int id, int x, int y, int Height, int Width);
 static void	changed(int bstart, int bend);
 static void	errorpopup(const char *msg);
 static void	error(const char *s);
 static int	init(void);

/*---[ Globals ]-------------------------------------------------------------------------------------------*/

 const struct lib3270_screen_callbacks g3270_screen_callbacks =
 {
	sizeof(struct lib3270_screen_callbacks),

	init,			// int (*init)(void);
	error,			// void (*Error)(const char *s);
	NULL,			// void (*Warning)(const char *s);
	setsize,		// void (*setsize)(int rows, int cols);
	addch,			// void (*addch)(int row, int col, int c, int attr);
	set_charset,	// void (*charset)(char *dcs);
	title,			// void (*title)(char *text);
	changed,		// void (*changed)(int bstart, int bend);
	NULL,			// void (*ring_bell)(void);
	action_Redraw,	// void (*redraw)(void);
	MoveCursor,		// void (*move_cursor)(int row, int col);
	suspend,		// void (*suspend)(void);
	resume,			// void (*resume)(void);
	NULL,			// void (*reset)(int lock);
	SetStatusCode,	// void (*status)(STATUS_CODE id);
	set_compose,	// void (*compose)(int on, unsigned char c, int keytype);
	set_cursor,		// void (*cursor)(CURSOR_MODE mode);
	set_lu,			// void (*lu)(const char *lu);
	set_oia,		// void (*set)(OIA_FLAG id, int on);
	erase,			// void (*erase)(void);
	errorpopup,		// void (*popup_an_error)(const char *msg);

 };

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
 };

 #define IMAGE_COUNT (sizeof(imagedata)/sizeof(struct _imagedata))

 static struct _pix
 {
 	GdkPixbuf *base;
 	GdkPixbuf *pix;
 	int		   Width;
 	int		   Height;
 } pix[IMAGE_COUNT];


 int 								terminal_rows	= 0;
 int 								terminal_cols	= 0;
 int								left_margin		= 0;
 int								top_margin		= 0;

 int								fWidth			= 0;
 int								fHeight			= 0;
 ELEMENT							*screen			= NULL;
 char								*charset		= NULL;

 static int						szScreen		= 0;
 static gboolean					draw			= FALSE;
 static const struct status_code	*sts_data		= NULL;
 static unsigned char			compose			= 0;
 static gchar						*luname			= 0;
 static const gchar				*status_msg		= NULL;
 static guint						kbrd_state		= 0;

 static gboolean					oia_flag[OIA_FLAG_USER];

/*---[ Implement ]-----------------------------------------------------------------------------------------*/

 static void changed(int bstart, int bend)
 {
 	WaitingForChanges = FALSE;
 }

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
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw(terminal);
 	}
 }

 static void title(char *text)
 {
 	Trace("Window %p title: \"%s\"",topwindow,text);
 	if(topwindow)
		gtk_window_set_title(GTK_WINDOW(topwindow),text ? text : PACKAGE_NAME);

 }

 void ParseInput(const gchar *string)
 {
    gchar *input = g_convert(string, -1, CHARSET, "UTF-8", NULL, NULL, NULL);

    if(!input)
    {
        Log("Error converting string \"%s\" to %s",string,CHARSET);
        return;
    }

    Trace("Converted input string: \"%s\"",input);

    // NOTE (perry#1#): Is it the best way?
    Input_String((const unsigned char *) input);

    g_free(input);
 }

 static int addch(int row, int col, int c, unsigned short attr)
 {
 	gchar	ch[MAX_CHR_LENGTH];
 	short	fg;
 	short	bg;
 	ELEMENT *el;
 	int		pos = (row*terminal_cols)+col;

 	if(!screen || col >= terminal_cols || row >= terminal_rows)
		return EINVAL;

	if(pos > szScreen)
		return EFAULT;

	memset(ch,0,MAX_CHR_LENGTH);

	if(c)
	{
		gsize sz;
		gchar in[2] = { (char) c, 0 };
		gchar *str = g_convert(in, -1, "UTF-8", CHARSET, NULL, &sz, NULL);

		if(sz < MAX_CHR_LENGTH)
			memcpy(ch,str,sz);
		else
			Log("Invalid size when converting \"%s\" to \"%s\"",in,ch);
		g_free(str);

	}

	bg = (attr & 0xF0) >> 4;

	if(attr & COLOR_ATTR_FIELD)
		fg = (attr & 0x03)+TERMINAL_COLOR_FIELD;
	else
		fg = (attr & 0x0F);

	// Get element entry in the buffer, update ONLY if changed
 	el = screen + pos;

	if( !(bg != el->bg || fg != el->fg || memcmp(el->ch,ch,MAX_CHR_LENGTH)))
		return 0;

	el->bg = bg;
	el->fg = fg;
	memcpy(el->ch,ch,MAX_CHR_LENGTH);

	if(draw && terminal && pixmap)
	{
		// Update pixmap, queue screen redraw.
		gint 	x, y;
	 	GdkGC	*gc		= gdk_gc_new(pixmap);

		PangoLayout *layout = gtk_widget_create_pango_layout(terminal,el->ch);

		x = left_margin + (col * fWidth);
		y = top_margin + (row * fHeight);

		DrawElement(pixmap,color,gc,layout,x,y,el);

		gdk_gc_destroy(gc);
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

 void action_Redraw(void)
 {
 	DrawScreen(terminal,color,pixmap);
	DrawOIA(terminal,color,pixmap);
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
		gc = gdk_gc_new(pixmap);
		gdk_drawable_get_size(pixmap,&width,&height);
		gdk_gc_set_foreground(gc,color);
		gdk_draw_rectangle(pixmap,gc,1,0,0,width,height);
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw(terminal);
		gdk_gc_destroy(gc);
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
		luname = g_convert(lu, -1, "UTF-8", CHARSET, NULL, NULL, NULL);

	if(terminal && pixmap)
	{
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw_area(terminal,left_margin,OIAROW,fWidth*terminal_cols,fHeight+1);
	}

 }

 void DrawOIA(GtkWidget *widget, GdkColor *clr, GdkDrawable *draw)
 {
    /*
     * The status line is laid out thusly (M is maxCOLS):
     *
     *   0          "4" in a square
     *   1          "A" underlined
     *   2          solid box if connected, "?" in a box if not
     *   3..7       empty
     *   8...       message area
     *   M-43       SSL Status
     *   M-41       Meta indication ("M" or blank)
     *   M-40       Alt indication ("A" or blank)
     *   M-39       Shift indication (Special symbol/"^" or blank)
     *   M-38..M-37 empty
     *   M-36       Compose indication ("C" or blank)
     *   M-35       Compose first character
     *   M-34       empty
     *   M-33       Typeahead indication ("T" or blank)
     *   M-31       Alternate keymap indication ("K" or blank)
     *   M-30       Reverse input mode indication ("R" or blank)
     *   M-29       Insert mode indication (Special symbol/"I" or blank)
     *   M-28       Printer indication ("P" or blank)
     *   M-27       Script indication ("S" or blank)
     *   M-26		empty
     *   M-25..M-14	LU Name

     *   M-15..M-9  command timing (Clock symbol and m:ss, or blank)
     *   M-7..M     cursor position (rrr/ccc or blank)
     *
     */
	GdkGC 		*gc;
	PangoLayout *layout;
	int   		row		= OIAROW;
	int			width	= (fWidth*terminal_cols);
	GdkColor	*bg		= clr+TERMINAL_COLOR_OIA_BACKGROUND;
	GdkColor	*fg		= clr+TERMINAL_COLOR_OIA;
	int			col		= left_margin;
	char		str[11];

	if(!draw)
		return;

	gc = gdk_gc_new(draw);

	gdk_gc_set_foreground(gc,bg);
	gdk_draw_rectangle(draw,gc,1,left_margin,row,width,fHeight+1);

	gdk_gc_set_foreground(gc,clr+TERMINAL_COLOR_OIA_SEPARATOR);
	gdk_draw_line(draw,gc,left_margin,row,left_margin+width,row);
	row++;

	gdk_gc_set_foreground(gc,fg);

	//  0          "4" in a square
	layout = gtk_widget_create_pango_layout(widget,"4");
	gdk_draw_layout_with_colors(draw,gc,col,row,layout,bg,fg);
	col += fWidth;

	//  1          "A" underlined
	if(oia_flag[OIA_FLAG_UNDERA])
	{
		pango_layout_set_text(layout,(IN_E) ? "B" : "A",-1);
		gdk_draw_layout_with_colors(draw,gc,col,row,layout,fg,bg);
	}

	col += fWidth;

	// 2          solid box if connected, "?" in a box if not
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


	// 8...       message area
	col += (fWidth<<2);

#ifdef DEBUG
	if(sts_data)
	{
		pango_layout_set_text(layout,status_msg && *status_msg ? status_msg : sts_data->dbg,-1);
		gdk_draw_layout_with_colors(draw,gc,col,row,layout,clr+sts_data->clr,bg);
	}
	else
	{
		pango_layout_set_text(layout,"STATUS_CODE_NONE",-1);
		gdk_draw_layout_with_colors(draw,gc,col,row,layout,clr+TERMINAL_COLOR_OIA_STATUS_INVALID,bg);
	}
#else
	if(sts_data && status_msg)
	{
		pango_layout_set_text(layout,status_msg,-1);
		gdk_draw_layout_with_colors(draw,gc,col,row,layout,clr+sts_data->clr,bg);
	}
#endif

	memset(str,' ',10);

    // M-36       Compose indication ("C" or blank)
	col = left_margin+(fWidth*(terminal_cols-36));

	if(compose)
	{
		str[0] = 'C';
		str[1] = compose;
	}

	//	M-39       Shift indication (Special symbol/"^" or blank)
	if(kbrd_state & GDK_SHIFT_MASK)
		DrawImage(draw,gc,2,left_margin+(fWidth*(terminal_cols-39)),row+1,fHeight-2,fWidth);

	// NOTE (perry#9#): I think it would be better if we use images (SVG?) instead of text.

    //   M-33       Typeahead indication ("T" or blank)
	str[3] = oia_flag[OIA_FLAG_TYPEAHEAD]	? 'T' : ' ';

    //   M-32       SSL Status - USING IMAGE IN M-43 - see DrawImage() below

    //   M-31       Alternate keymap indication ("K" or blank)

    //   M-30       Reverse input mode indication ("R" or blank)
	str[6] = oia_flag[OIA_FLAG_REVERSE] 	? 'R' : ' ';

    //   M-29       Insert mode indication (Special symbol/"I" or blank)
	str[7] = oia_flag[OIA_FLAG_INSERT]		? 'I' : ' ';

    //   M-28       Printer indication ("P" or blank)
	str[8] = oia_flag[OIA_FLAG_PRINTER]		? 'P' : ' ';

	str[9] = 0;

	pango_layout_set_text(layout,str,-1);
	gdk_draw_layout_with_colors(draw,gc,col,row,layout,fg,bg);

	// Draw SSL indicator (M-43)
	DrawImage(draw,gc,oia_flag[OIA_FLAG_SECURE] ? 0 : 1 ,left_margin+(fWidth*(terminal_cols-43)),row+1,fHeight-2,fWidth);

	// M-25 LU Name
	if(luname)
	{
		pango_layout_set_text(layout,luname,-1);
		gdk_draw_layout_with_colors(draw,gc,left_margin+(fWidth*(terminal_cols-25)),row,layout,clr+TERMINAL_COLOR_OIA_LU,bg);
	}

	//  M-7..M     cursor position (rrr/ccc or blank)
	if(Toggled(CURSOR_POS))
	{
		sprintf(str,"%03d/%03d",cRow+1,cCol+1);
		pango_layout_set_text(layout,str,-1);
		gdk_draw_layout_with_colors(draw,gc,left_margin+(fWidth*(terminal_cols-7)),row,layout,clr+TERMINAL_COLOR_OIA_CURSOR,bg);
	}

	g_object_unref(layout);
	gdk_gc_destroy(gc);

 }

 static void set_oia(OIA_FLAG id, int on)
 {
 	if(id > OIA_FLAG_USER)
		return;

 	oia_flag[id] = on;

 	if(terminal && pixmap)
 	{
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw_area(terminal,left_margin,OIAROW,fWidth*terminal_cols,fHeight+1);
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

	if(!(el && draw && widget))
		return -1;

	gc = gdk_gc_new(draw);

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
				DrawElement(draw,clr,gc,layout,x,y,el);
			}
			else if(el->selected)
			{
				gdk_gc_set_foreground(gc,clr+TERMINAL_COLOR_SELECTED_BG);
				gdk_draw_rectangle(draw,gc,1,x,y,fWidth,fHeight);
			}

			el++;
			x += fWidth;
		}
		y += fHeight;
	}

	g_object_unref(layout);
	gdk_gc_destroy(gc);

	return 0;

 }


 static void set_charset(char *dcs)
 {
 	Trace("Screen charset: %s",dcs);
	g_free(charset);
	charset = g_strdup(dcs);
 }

 static STATUS_CODE last_id = (STATUS_CODE) -1;

 void SetStatusCode(STATUS_CODE id)
 {
 	static const struct status_code tbl[STATUS_CODE_USER] =
 	{
		STATUS_CODE_DESCRIPTION(	STATUS_CODE_BLANK,
									TERMINAL_COLOR_OIA_STATUS_OK,
									N_( "a" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_SYSWAIT,
									TERMINAL_COLOR_OIA_STATUS_OK,
									N_( "bX System" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_TWAIT,
									TERMINAL_COLOR_OIA_STATUS_OK,
									N_( "cX Wait" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_CONNECTED,
									TERMINAL_COLOR_OIA_STATUS_OK,
									N_( "d" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_DISCONNECTED,
									TERMINAL_COLOR_OIA_STATUS_INVALID,
									N_( "eX Disconnected" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_AWAITING_FIRST,
									TERMINAL_COLOR_OIA_STATUS_OK,
									N_( "fX" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_MINUS,
									TERMINAL_COLOR_OIA_STATUS_OK,
									N_( "gX -f" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_PROTECTED,
									TERMINAL_COLOR_OIA_STATUS_INVALID,
									N_( "hX Protected" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_NUMERIC,
									TERMINAL_COLOR_OIA_STATUS_INVALID,
									N_( "iX Numeric" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_OVERFLOW,
									TERMINAL_COLOR_OIA_STATUS_INVALID,
									N_( "jX Overflow" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_INHIBIT,
									TERMINAL_COLOR_OIA_STATUS_INVALID,
									N_( "kX Inhibit" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_X,
									TERMINAL_COLOR_OIA_STATUS_INVALID,
									N_( "lX" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_RESOLVING,
									TERMINAL_COLOR_OIA_STATUS_WARNING,
									N_( "mX Resolving" ) ),

		STATUS_CODE_DESCRIPTION(	STATUS_CODE_CONNECTING,
									TERMINAL_COLOR_OIA_STATUS_WARNING,
									N_( "nX Connecting" ) ),


	};

	/* Check if status has changed to avoid unnecessary redraws */
	if(id == last_id && !sts_data)
		return;

	if(id >= STATUS_CODE_USER)
	{
		Log("Unexpected status code %d",(int) id);
		return;
	}

	last_id 	= id;
	sts_data 	= tbl+id;
	status_msg	= gettext(sts_data->string)+1;

	Trace("Status changed to %s (%s)",sts_data->dbg,sts_data->string+1);

	// FIXME (perry#2#): Find why the library is keeping the cursor as "locked" in some cases. When corrected this "workaround" can be removed.
	if(id == STATUS_CODE_BLANK)
		set_cursor(CURSOR_MODE_NORMAL);

	DrawOIA(terminal,color,pixmap);
	gtk_widget_queue_draw_area(terminal,left_margin,OIAROW,fWidth*terminal_cols,fHeight+1);

 }

 static void set_cursor(CURSOR_MODE mode)
 {
 	if(terminal && terminal->window && mode < CURSOR_MODE_USER)
		gdk_window_set_cursor(terminal->window,wCursor[mode]);

 }

 static int Loaded = 0;

 void LoadImages(GdkDrawable *drawable, GdkGC *gc)
 {
 	int			f;
 	GdkPixmap	*temp;

	if(!Loaded)
	{
    	memset(pix,0,sizeof(struct _pix) * IMAGE_COUNT);
    	Loaded = 1;
	}

 	for(f=0;f<IMAGE_COUNT;f++)
 	{
 		// Load bitmap setting the right colors
 		temp = gdk_pixmap_create_from_data(	drawable,
											(const gchar *) imagedata[f].data,
                                       		imagedata[f].width,
                                       		imagedata[f].height,
											gdk_drawable_get_depth(drawable),
											color+imagedata[f].color,
											color+TERMINAL_COLOR_OIA_BACKGROUND );

		if(pix[f].base)
			gdk_pixbuf_unref(pix[f].base);

		pix[f].base = gdk_pixbuf_get_from_drawable(	0,
													temp,
													gdk_drawable_get_colormap(drawable),
													0,0,
													0,0,
													imagedata[f].width,
													imagedata[f].height );

        gdk_pixmap_unref(temp);
 	}

 }

 static void DrawImage(GdkDrawable *drawable, GdkGC *gc, int id, int x, int y, int Height, int Width)
 {
    double ratio;
    int    temp;

 	if( ((Height != pix[id].Height) || (Width != pix[id].Width)) && pix[id].pix )
 	{
 		gdk_pixbuf_unref(pix[id].pix);
		pix[id].pix = 0;
 	}

 	if(!pix[id].pix)
 	{
 		/* Resize by Height */
        ratio = ((double) gdk_pixbuf_get_width(pix[id].base)) / ((double) gdk_pixbuf_get_height(pix[id].base));
		temp  = (int) ((double) ratio * ((double) Height));
	    pix[id].pix = gdk_pixbuf_scale_simple(pix[id].base,temp,Height,GDK_INTERP_HYPER);
 	}

    if(pix[id].pix)
    {
   	   pix[id].Height = Height;
	   pix[id].Width  = Width;
	   gdk_pixbuf_render_to_drawable(pix[id].pix,drawable,gc,0,0,x,y,-1,-1,GDK_RGB_DITHER_NORMAL,0,0);
    }
 }

 void DrawElement(GdkDrawable *draw, GdkColor *clr, GdkGC *gc, PangoLayout *layout, int x, int y, ELEMENT *el)
 {
 	if(!(gc && draw && layout && el))
		return;

	pango_layout_set_text(layout,el->ch,-1);

	if(el->selected)
		gdk_draw_layout_with_colors(draw,gc,x,y,layout,color+TERMINAL_COLOR_SELECTED_FG,clr+TERMINAL_COLOR_SELECTED_BG);
	else
		gdk_draw_layout_with_colors(draw,gc,x,y,layout,color+el->fg,clr+el->bg);

 }

 void UpdateKeyboardState(guint state)
 {
 	if(state == kbrd_state)
		return;

 	kbrd_state = state;

 	// TODO (perry#1#): Draw only the state flags.
 	if(terminal && pixmap)
 	{
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw_area(terminal,left_margin,OIAROW,fWidth*terminal_cols,fHeight+1);
 	}

 }

 gchar * GetScreenContents(gboolean all)
 {
 	gchar 	*buffer;
 	gsize	max = terminal_rows*terminal_cols*MAX_CHR_LENGTH;
 	int		row,col;
 	int		pos	= 0;

	buffer = g_malloc(max);
	*buffer = 0;
	for(row = 0; row < terminal_rows;row++)
	{
		gchar		line[terminal_cols*MAX_CHR_LENGTH];
		gboolean 	sel = FALSE;

		*line = 0;
		for(col = 0; col < terminal_cols;col++)
		{
			if(all || screen[pos].selected)
			{
				sel = TRUE;
				g_strlcat(line,*screen[pos].ch ? screen[pos].ch : " ",max);
			}
			pos++;
		}
		if(sel)
		{
			g_strlcat(buffer,g_strchomp(line),max);
			g_strlcat(buffer,"\n",max);
		}

	}

	g_strchomp(buffer);
	return g_realloc(buffer,strlen(buffer)+1);
 }

 static void errorpopup(const char *msg)
 {
 	GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW(topwindow),
												GTK_DIALOG_DESTROY_WITH_PARENT,
												GTK_MESSAGE_ERROR,
												GTK_BUTTONS_CLOSE,
												"%s",msg );

 	g_critical(msg);

	gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);

 }

 static void error(const char *s)
 {
 	g_error("%s",s);
 	gtk_main_quit();
 }

 static int init(void)
 {
	// TODO (perry#1#): Get screen size from configuration file
	ctlr_set_rows_cols(2, 80, 24);

	return 0;
 }
