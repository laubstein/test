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
 * Este programa está nomeado como mouse.c e possui 914 linhas de código.
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

 #include <globals.h>
 #include <lib3270/kybdc.h>
 #include <lib3270/actionsc.h>
 #include <lib3270/toggle.h>
 #include <lib3270/3270ds.h>

/*---[ Defines ]--------------------------------------------------------------*/

/*---[ Prototipes ]-----------------------------------------------------------*/

 static void UpdateSelectedRegion(int start, int end);
 static void SelectField(int row, int col);
 static void SetDragType(int type);

#ifndef USE_PRIMARY_SELECTION
	#define SelectionChanged() /* */
#else
	static void SelectionChanged(void);
#endif

/*---[ Constants ]------------------------------------------------------------*/

/*---[ Statics ]--------------------------------------------------------------*/

 static int 			startRow 	= 0;
 static int 			startCol 	= 0;
 static int 			endRow 		= 0;
 static int 			endCol		= 0;
 static int 			dragRow		= 0;
 static int 			dragCol		= 0;
 static int 			select_mode	= SELECT_MODE_INVALID;

/*---[ Globals ]--------------------------------------------------------------*/

 gboolean	WaitingForChanges 	= TRUE;
 GtkWidget	*SelectionPopup		= 0;
 GtkWidget	*DefaultPopup		= 0;
 int 		drag_type			= DRAG_TYPE_NONE;

/*---[ Implement ]------------------------------------------------------------*/

 int GetSelectedRectangle(GdkRectangle *rect)
 {
	// FIXME (perry#3#): First check if the selection area isn't rectangular.
 	if(!Toggled(RECTANGLE_SELECT))
		return EINVAL;

	rect->x			= startCol;
	rect->y			= startRow;
	rect->width		= (endCol - startCol)+1;
	rect->height	= (endRow - startRow)+1;

	return 0;
 }

 static GtkAction *reselect = NULL;
 static GtkAction *copyastable = NULL;

 void SetSelectionMode(int m)
 {
 	if(m == select_mode)
		return;

	if(m == SELECT_MODE_NONE && select_mode != SELECT_MODE_INVALID)
	{
		if(!reselect)
			reselect = gtk_action_group_get_action(online_actions,"Reselect");
		gtk_action_set_sensitive(reselect,TRUE);
	}

	if(!copyastable)
		copyastable = gtk_action_group_get_action(selection_actions,"CopyAsTable");
	gtk_action_set_sensitive(copyastable,m == SELECT_MODE_RECTANGLE);

	select_mode = m;

	gtk_action_group_set_sensitive(selection_actions,(select_mode == SELECT_MODE_NONE) ? FALSE : TRUE );

 }

 static int CheckForFunction(int baddr, int length)
 {
 	int ret = 0;
	if( !(*screen[baddr+1].ch == 'F' && FA_IS_PROTECTED(get_field_attribute(baddr))) )
		return ret;

	baddr++;
	while(--length > 0)
	{
		baddr++;
		if(*screen[baddr].ch > '9' || *screen[baddr].ch < '0')
			return 0;

		ret = (ret*10)+(*screen[baddr].ch - '0');
		Trace("Field@%d: %c",baddr,*screen[baddr].ch);
	}

 	return ret;
 }

 void action_SelectField(void)
 {
 	int 		pos;
 	gboolean	redraw	= FALSE;

	if(!screen)
		return;

	for(pos = 0; pos < (terminal_rows * terminal_cols);pos++)
	{
		if(screen[pos].status & ELEMENT_STATUS_SELECTED)
		{
			redraw = TRUE;
			screen[pos].status &= ~ELEMENT_STATUS_SELECTED;
		}
	}

	SelectField(cRow,cCol);

	SetSelectionMode(SELECT_MODE_FIELD);

	if(redraw && terminal && pixmap)
	{
		DrawScreen(terminal,color,pixmap);
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw(terminal);
	}

 }

 static void SelectField(int row, int col)
 {
 	int baddr = find_field_attribute((row * terminal_cols) + col);
 	int length = find_field_length(baddr);
 	int function = 0;

	Trace("Field %d with %d bytes",baddr,length);

	if(length < 0 || length > ((terminal_cols * terminal_cols) - baddr))
		return;

	if(length < 3)
		function = CheckForFunction(baddr,length);

	if(!action_PFKey(function))
		return;

	startRow = (baddr+1) / terminal_cols;
	startCol = (baddr+1) % terminal_cols;

	UpdateSelectedRegion(baddr+1,baddr+length);

	baddr += length;
	endRow = baddr / terminal_cols;
	endCol = baddr % terminal_cols;

 }

 static void SetSelection(gboolean selected)
 {
 	int 			pos;
 	gboolean		redraw	= FALSE;
	unsigned char	status;

	if(!screen)
		return;


	for(pos = 0; pos < (terminal_rows * terminal_cols);pos++)
	{
		if(selected)
			status = screen[pos].status | ELEMENT_STATUS_SELECTED;
		else
			status = screen[pos].status & ~ELEMENT_STATUS_SELECTED;

		if(screen[pos].status != status)
		{
			redraw = TRUE;
			screen[pos].status = status;
		}
	}

	if(redraw && terminal && pixmap)
	{
		DrawScreen(terminal,color,pixmap);
		DrawOIA(terminal,color,pixmap);
		gtk_widget_queue_draw(terminal);
	}
 }

 #define DecodePosition(event,row,col) _DecodePosition(event->x,event->y,&row,&col)

 static int _DecodePosition(long x, long y, int *row, int *col)
 {
 	int rc = 0;

 	if(x < left_margin)
 	{
 		*col = 0;
 		rc = -1;
 	}
	else if(x > (left_margin+(terminal_cols * fWidth)))
	{
		*col = terminal_cols-1;
		rc = -1;
	}
	else
	{
		*col = ((((unsigned long) x) - left_margin)/fWidth);
	}

	if(y < top_margin)
	{
		*row = 0;
		rc = -1;
	}
	else if(y > (top_margin+(terminal_rows * fHeight)))
	{
		*row = terminal_rows-1;
		rc = -1;
	}
	else
	{
		*row = ((((unsigned long) y) - top_margin)/fHeight);
	}

	return rc;
 }


 #define BUTTON_FLAG_COMBO	0x80

 static gint button_flags = 0;

 gboolean mouse_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
 {
	// http://library.gnome.org/devel/gdk/stable/gdk-Event-Structures.html#GdkEventButton
	GtkWidget *w;

	Trace("Button press: %d",event->button);

	switch( ((event->type & 0x0F) << 4) | (event->button & 0x0F) | button_flags)
	{
	case ((GDK_BUTTON_PRESS & 0x0F) << 4) | 1:

		switch(drag_type)
		{
		case DRAG_TYPE_NONE:

			action_ClearSelection();

			if(!DecodePosition(event,startRow,startCol))
				button_flags |= BUTTON_FLAG_COMBO;

			break;

		case DRAG_TYPE_INSIDE:
			SetSelectionMode(SELECT_MODE_DRAG);
			DecodePosition(event,dragRow,dragCol);
			dragRow -= startRow;
			dragCol -= startCol;
			Trace("Selection mode Drag: %d (Position: %d,%d)",SELECT_MODE_DRAG,dragRow,dragCol);
			break;

		default:
			Trace("Selection mode Drag: %d",SELECT_MODE_DRAG);
			SetSelectionMode(SELECT_MODE_DRAG);

		}
		break;

	case ((GDK_2BUTTON_PRESS & 0x0F) << 4) | 1 | BUTTON_FLAG_COMBO:
		DecodePosition(event,startRow,startCol);
		SetSelectionMode(SELECT_MODE_FIELD);
		Trace("Button 1 double-clicked at %ld,%ld (%d,%d)",(long) event->x, (long) event->y,startRow,startCol);
		break;

	case ((GDK_BUTTON_PRESS & 0x0F) << 4) | 3:
		w = (select_mode == SELECT_MODE_NONE) ? DefaultPopup : SelectionPopup;
		Trace("Button 2 clicked at %ld,%ld Menu: %p",(long) event->x, (long) event->y, w);
		if(w)
		{
			gtk_menu_set_screen(GTK_MENU(w), gtk_widget_get_screen(widget));
			gtk_menu_popup(GTK_MENU(w), NULL, NULL, NULL, NULL, event->button,event->time);
		}
		break;

	case ((GDK_2BUTTON_PRESS & 0x0F) << 4) | 3:
		Trace("Button 2 double-clicked at %ld,%ld",(long) event->x, (long) event->y);
		break;

	case ((GDK_2BUTTON_PRESS & 0x0F) << 4) | 3 | BUTTON_FLAG_COMBO:
		Trace("Button 2 double-clicked in combo mode at %ld,%ld",(long) event->x, (long) event->y);
		SetSelectionMode(SELECT_MODE_APPEND);
		break;

	case ((GDK_BUTTON_PRESS & 0x0F) << 4) | 3 | BUTTON_FLAG_COMBO:
		Trace("Button 2 clicked in combo mode at %ld,%ld",(long) event->x, (long) event->y);
		SetSelectionMode(SELECT_MODE_COPY);
		break;

#ifdef DEBUG
	default:
		Trace("Unexpected mouse click %d with button %d flag: %d",event->type, event->button,button_flags);
#endif
	}

 	return 0;
 }

 gboolean mouse_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
 {
	int row,col;

	DecodePosition(event,row,col);

	Trace("Button %d release Action: %d",event->button, (int) ( ((select_mode & 0x0F) << 4) | (event->button & 0x0F)));

	switch( ((select_mode & 0x0F) << 4) | (event->button & 0x0F))
 	{
	case ((SELECT_MODE_NONE & 0x0F) << 4) | 1: // Single click, just move cursor
		Trace("Single click (button: %d)",event->button);
		action_ClearSelection();
		if(row >= 0 && row <= terminal_rows && col >= 0 && col <= terminal_cols)
			cursor_move((row*terminal_cols)+col);
		break;

	case ((SELECT_MODE_NONE & 0x0F) << 4) | 2: // Single click on button 2
		Trace("Single click (button: %d)",event->button);
		action_PasteSelection();
		break;

	case ((SELECT_MODE_FIELD & 0x0F) << 4) | 1:	// Double click, select field
		Trace("Selecting field (button: %d)",event->button);
		SelectField(startRow,startCol);
		break;

	case ((SELECT_MODE_COPY & 0x0F) << 4) | 1:
		Trace("Copy text (button: %d)",event->button);
		action_Copy();
		action_ClearSelection();
		break;

	case ((SELECT_MODE_APPEND & 0x0F) << 4) | 1:
		Trace("Append text (button: %d)",event->button);
		action_Append();
		action_ClearSelection();
		break;

	case ((SELECT_MODE_DRAG & 0x0F) << 4) | 1: // Left Drag
		select_mode &= ~SELECT_MODE_DRAG;
		select_mode |= SELECT_MODE_RECTANGLE;
		SetDragType(DRAG_TYPE_NONE);
		Trace("Ending selection drag (Button: %d New mode: %d)",event->button,select_mode);
		break;

	case ((SELECT_MODE_RECTANGLE & 0x0F) << 4) | 1: // End rectangle select
		break;

#ifdef DEBUG
	default:
		Trace("Unexpected action %04x mode: %d",((select_mode & 0x0F) << 4) | (event->button & 0x0F),select_mode);
#endif
 	}

	if(event->button == 1)
		button_flags &= ~BUTTON_FLAG_COMBO;

    return 0;
 }

 static void UpdateSelectedRectangle(void)
 {
 	int				x,y,row,col;
	GdkGC			*gc		= NULL;
	PangoLayout 	*layout	= NULL;
 	int				pos		= 0;
 	int				left;
 	int				right;
 	int				top;
 	int				bottom;

	if(!(select_mode && screen && terminal))
		return;

	// Clear selection box, invalidate drawing area.
	if(pixmap)
		gc = gdk_gc_new(pixmap);

	if(terminal)
		layout = gtk_widget_create_pango_layout(terminal," ");

	if(startRow > endRow)
	{
		top = endRow;
		bottom = startRow;
	}
	else
	{
		top = startRow;
		bottom = endRow;
	}

	if(startCol > endCol)
	{
		left = endCol;
		right = startCol;
	}
	else
	{
		left = startCol;
		right = endCol;
	}

	y = top_margin;
	for(row = 0; row < terminal_rows;row++)
	{
		x = left_margin;
		for(col = 0; col < terminal_cols;col++)
		{
			unsigned char status = screen[pos].status & ELEMENT_STATUS_FIELD_MASK;

			if(col >= left && col <= right && row >= top && row <= bottom)
			{
				status |= ELEMENT_STATUS_SELECTED;

				if(col == left)
					status |= SELECTION_BOX_LEFT;

				if(col == right)
					status |= SELECTION_BOX_RIGHT;

				if(row == top)
					status |= SELECTION_BOX_TOP;

				if(row == bottom)
					status |= SELECTION_BOX_BOTTOM;
			}

			if(screen[pos].status != status)
			{
				// Changed, mark to update
				screen[pos].status = status;
				DrawElement(pixmap,color,gc,x,y,screen+pos);
				gtk_widget_queue_draw_area(terminal,x,y,fWidth,fHeight);
			}
			pos++;
			x += fWidth;
		}
		y += fHeight;
	}

	if(layout)
		g_object_unref(layout);

	if(gc)
		gdk_gc_destroy(gc);

	SelectionChanged();
 }

 static void UpdateSelectedText(void)
 {
 	UpdateSelectedRegion((startRow * terminal_cols)+startCol,(endRow * terminal_cols)+endCol);
 }

 static void UpdateSelectedRegion(int start, int end)
 {
 	int			pos		= 0;
 	int			x,y,row,col;
	GdkGC		*gc		= NULL;

	if(!(screen && terminal))
		return;

 	if(start > end)
 	{
		int temp = start;
		start = end;
		end = temp;
 	}

	if(pixmap)
		gc = gdk_gc_new(pixmap);

	y = top_margin;
	for(row = 0; row < terminal_rows;row++)
	{
		x = left_margin;
		for(col = 0; col < terminal_cols;col++)
		{
			unsigned char status = screen[pos].status & ELEMENT_STATUS_FIELD_MASK;

			if (pos >= start && pos <= end)
			{
				status |= ELEMENT_STATUS_SELECTED;

				if(!(row && (screen[pos-terminal_cols].status) & ELEMENT_STATUS_SELECTED))
					status |= SELECTION_BOX_TOP;

				if(!(pos && col && (screen[pos-1].status) & ELEMENT_STATUS_SELECTED))
					status |= SELECTION_BOX_LEFT;

				if(pos+1 > end || col == (terminal_cols-1))
					status |= SELECTION_BOX_RIGHT;

				if((pos+terminal_cols) > end)
					status |= SELECTION_BOX_BOTTOM;
			}

			if(screen[pos].status != status)
			{
				// Changed, mark to update
				screen[pos].status = status;
				DrawElement(pixmap,color,gc,x,y,screen+pos);
				gtk_widget_queue_draw_area(terminal,x,y,fWidth,fHeight);
			}
			pos++;
			x += fWidth;
		}
		y += fHeight;
	}

	if(gc)
		gdk_gc_destroy(gc);


	SelectionChanged();

 }

 void set_rectangle_select(int value, int reason)
 {
 	if(select_mode != SELECT_MODE_RECTANGLE && select_mode != SELECT_MODE_TEXT)
		return;

	action_ClearSelection();
	SetSelectionMode(value ? SELECT_MODE_RECTANGLE : SELECT_MODE_TEXT);

	if(select_mode == SELECT_MODE_TEXT)
		UpdateSelectedText();
	else
		UpdateSelectedRectangle();
 }

 void Reselect(void)
 {
	SetSelectionMode(Toggled(RECTANGLE_SELECT) ? SELECT_MODE_RECTANGLE : SELECT_MODE_TEXT);
	if(select_mode == SELECT_MODE_TEXT)
		UpdateSelectedText();
	else
		UpdateSelectedRectangle();
 }

 static void SetDragType(int type)
 {
 	if(drag_type == type)
		return;

 	drag_type = type;

#ifdef MOUSE_POINTER_CHANGE
 	if(terminal && terminal->window)
 	{
 		Trace("Type: %d",type);

 		if(type >= 0)
			gdk_window_set_cursor(terminal->window,wCursor[CURSOR_MODE_USER+type]);
 		else
			gdk_window_set_cursor(terminal->window,wCursor[cursor_mode]);
 	}
#endif

 }

 gboolean mouse_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
 {
	int row, col, r, c;

	if(button_flags & BUTTON_FLAG_COMBO)
	{
		// Moving with button 1 pressed, update selection
		switch(select_mode)
		{
		case SELECT_MODE_NONE:	// Start selection
			SetSelectionMode(Toggled(RECTANGLE_SELECT) ? SELECT_MODE_RECTANGLE : SELECT_MODE_TEXT);
			return mouse_motion(widget,event,user_data); // Recursive call to update selection box

		case SELECT_MODE_RECTANGLE:
			DecodePosition(event,endRow,endCol);
			UpdateSelectedRectangle();
			break;

		case SELECT_MODE_TEXT:
			DecodePosition(event,endRow,endCol);
			UpdateSelectedText();
			break;

		}
	}
	else if(select_mode == SELECT_MODE_RECTANGLE)
	{
		int row, col;

		if(DecodePosition(event,row,col))
		{
			SetDragType(DRAG_TYPE_NONE);
		}
		else if(row < startRow || row > endRow || col < startCol || col > endCol)
		{
			SetDragType(DRAG_TYPE_NONE);
		}
		else if(row == startRow && col == startCol)
		{
			Trace("Top-left (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_TOP_LEFT);
		}
		else if(row == startRow && col == endCol)
		{
			Trace("Top-right (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_TOP_RIGHT);
		}
		else if(row == startRow)
		{
			Trace("Top (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_TOP);
		}
		else if(row == endRow && col == startCol)
		{
			Trace("Bottom-left (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_BOTTOM_LEFT);
		}
		else if(row == endRow && col == endCol)
		{
			Trace("Bottom-right (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_BOTTOM_RIGHT);
		}
		else if(row == endRow)
		{
			Trace("Bottom (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_BOTTOM);
		}
		else if(col == startCol)
		{
			Trace("Left (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_LEFT);
		}
		else if(col == endCol)
		{
			Trace("Right (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_RIGHT);
		}
		else if(col >= startCol && col <= endCol && row >= startRow && row <= endRow)
		{
			Trace("Inside (%d,%x)",row,col);
			SetDragType(DRAG_TYPE_INSIDE);
		}
		else
		{
			SetDragType(DRAG_TYPE_NONE);
		}

	}
	else if(select_mode == SELECT_MODE_DRAG)
	{
		DecodePosition(event,row,col);

		Trace("Drag_type: %d Position: %d,%d",drag_type,row,col);

		switch(drag_type)
		{
		case DRAG_TYPE_TOP_LEFT:
			if(row <= endRow)
				startRow = row;
			if(col <= endCol)
				startCol = col;
			break;

		case DRAG_TYPE_TOP_RIGHT:
			if(row <= endRow)
				startRow = row;
			if(col >= startCol)
				endCol = col;
			break;

		case DRAG_TYPE_TOP:
			if(row <= endRow)
				startRow = row;
			break;

		case DRAG_TYPE_BOTTOM_LEFT:
			if(row >= startRow)
				endRow = row;
			if(col <= endCol)
				startCol = col;
			break;

		case DRAG_TYPE_BOTTOM_RIGHT:
			if(row >= startRow)
				endRow = row;
			if(col >= startCol)
				endCol = col;
			break;

		case DRAG_TYPE_BOTTOM:
			if(row >= startRow)
				endRow = row;
			break;

		case DRAG_TYPE_LEFT:
			if(col <= endCol)
				startCol = col;
			break;

		case DRAG_TYPE_RIGHT:
			if(col >= startCol)
				endCol = col;
			break;

		case DRAG_TYPE_INSIDE:

			r = endRow - startRow;
			c = endCol - startCol;

			startRow = row-dragRow;

			// Get new row
			if(startRow < 0)
				startRow = 0;
			endRow = startRow + r;
			if(endRow >= (terminal_rows-1))
			{
				endRow = (terminal_rows-1);
				startRow = endRow - r;
			}

			// Get new col
			startCol = col-dragCol;
			if(startCol < 0)
				startCol = 0;
			endCol = startCol + c;

			if(endCol >= (terminal_cols-1))
			{
				endCol = (terminal_cols-1);
				startCol = endCol - c;
			}

			break;
		}

		UpdateSelectedRectangle();

	}

    return 0;
 }

 gboolean mouse_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
 {
 	static const int pfkey[]			= { 7,8 };
 	static const gchar *action_name[]	= { "ScrollUP", "ScrollDown" };

    if(event->direction < 2 && event->direction >= 0 && !WaitingForChanges)
 	{
		GtkAction *action = gtk_action_group_get_action(common_actions,action_name[event->direction]);

		WaitingForChanges = TRUE;

		if(action)
			gtk_action_activate(action);
		else
			action_PFKey(pfkey[event->direction]);
	}

 	return 0;
 }

 void action_ClearSelection(void)
 {
 	if(select_mode == SELECT_MODE_NONE)
		return;

 	SetDragType(DRAG_TYPE_NONE);
	SetSelection(FALSE);
	SetSelectionMode(SELECT_MODE_NONE);

// #ifdef USE_PRIMARY_SELECTION
//	gtk_clipboard_set_text(gtk_widget_get_clipboard(topwindow,GDK_SELECTION_PRIMARY),"",-1);
// #endif

 }

 void action_SelectAll(GtkWidget *w, gpointer user_data)
 {
 	SetSelection(TRUE);
	SetSelectionMode(Toggled(RECTANGLE_SELECT) ? SELECT_MODE_RECTANGLE : SELECT_MODE_TEXT);
 }

 static void doSelect(int (*call)(void))
 {
 	if(select_mode == SELECT_MODE_NONE)
 	{
 		SetSelectionMode(Toggled(RECTANGLE_SELECT) ? SELECT_MODE_RECTANGLE : SELECT_MODE_TEXT);
 		startRow = endRow = cRow;
 		startCol = endCol = cCol;
 	}

 	call();

	endRow = cRow;
	endCol = cCol;

	Reselect();
 }

 void action_SelectLeft(GtkWidget *w, gpointer user_data)
 {
 	doSelect(action_CursorLeft);
 }

 void action_SelectRight(GtkWidget *w, gpointer user_data)
 {
 	doSelect(action_CursorRight);
 }

 void action_SelectUp(GtkWidget *w, gpointer user_data)
 {
 	doSelect(action_CursorUp);
 }

 void action_SelectDown(GtkWidget *w, gpointer user_data)
 {
 	doSelect(action_CursorDown);
 }

 static void MoveSelection(int row, int col)
 {
 	if(select_mode == SELECT_MODE_NONE)
		return;

	startRow 	+= row;
	endRow 		+= row;
	startCol 	+= col;
	endCol 		+= col;

	Reselect();

 }

 void action_SelectionUp(GtkWidget *w, gpointer user_data)
 {
	if(startRow > 0 && endRow > 0)
		MoveSelection(-1,0);
 }

 void action_SelectionDown(GtkWidget *w, gpointer user_data)
 {
 	int maxrow = terminal_rows-1;

	if(startRow < maxrow && endRow < maxrow)
		MoveSelection(1,0);
 }

 void action_SelectionLeft(GtkWidget *w, gpointer user_data)
 {
	if(startCol > 0 && endCol > 0)
		MoveSelection(0,-1);
 }

 void action_SelectionRight(GtkWidget *w, gpointer user_data)
 {
 	int maxcol = terminal_cols-1;

	if(startCol < maxcol && endCol < maxcol)
		MoveSelection(0,1);
 }

 gboolean mouse_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
 {
/*
#if defined(WIN32) && defined(MOUSE_POINTER_CHANGE)

	switch(event->type)
	{
	case GDK_ENTER_NOTIFY:
		break;

	case GDK_LEAVE_NOTIFY:
		break;
	}

#endif
*/
 	return 0;
 }

#ifdef USE_PRIMARY_SELECTION
 static void SelectionChanged(void)
 {
	gchar *selection = GetSelection();
	gtk_clipboard_set_text(gtk_widget_get_clipboard(topwindow,GDK_SELECTION_PRIMARY),selection,-1);
	g_free(selection);
 }
#endif

