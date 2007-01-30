

 #include "g3270.h"
 #include "lib/kybdc.h"
 #include "lib/actionsc.h"
 #include "lib/tablesc.h"
 #include "lib/utilc.h"
 #include "lib/ctlrc.h"
 #include "lib/3270ds.h"

/*---[ Defines ]--------------------------------------------------------------*/

 #define SELECTING_RESTORED 0x0D
 #define SELECTING_CLICK	0x0E
 #define SELECTING_KEYBOARD 0x0F

 typedef struct _position
 {
 	unsigned char	updated;
 	long 			x;
 	long 			y;
 	long			row;
 	long			col;
 } POSITION;

 enum _selection_positions
 {
 	SELECTION_BEGIN,
 	SELECTION_END,

 	SELECTION_COUNT
 };

 typedef struct _cursor
 {
 	GdkCursor		*cursor;
 	GdkCursorType	type;
 } CURSOR;

/*---[ Prototipes ]-----------------------------------------------------------*/

/*---[ Constants ]------------------------------------------------------------*/

/*---[ Statics ]--------------------------------------------------------------*/

 static POSITION 	pos[SELECTION_COUNT];
 static GdkCursor 	*pointer	= 0;

 static int			selected_cursor = -1;
 static CURSOR		cursor[]		= {	{	0,	GDK_TOP_LEFT_CORNER     },
										{	0,	GDK_TOP_RIGHT_CORNER    },
										{	0,	GDK_BOTTOM_LEFT_CORNER  },
										{	0,	GDK_BOTTOM_RIGHT_CORNER },
									  };

/*---[ Globals ]--------------------------------------------------------------*/

 unsigned short selecting = 0;
 unsigned short modifier  = 0;

/*---[ Implement ]------------------------------------------------------------*/

/**
 * Invalida uma caixa de selecao.
 *
 * Recalcula as coordenadas de terminal correspondentes as posicoes de mouse
 * capturadas e redesenha toda a caixa de selecao.
 *
 */
 static void InvalidateSelectionBox(void)
 {
 	int rows;
 	int cols;
 	int f;

    if(!Get3270DeviceBuffer(&rows, &cols))
       return;

    for(f=0;f<SELECTION_COUNT;f++)
    {
    	if(pos[f].updated)
    	{
            pos[f].col = (pos[f].x - left_margin) / font->Width;

            if(pos[f].col < 0)
               pos[f].col = 0;

			if(pos[f].col > cols)
			   pos[f].col = cols;

            pos[f].row = (pos[f].y - top_margin)  / (font->Height + line_spacing);

			if(pos[f].row < 0)
			   pos[f].row = 0;

			if(pos[f].row > rows)
			   pos[f].row = rows;

			pos[f].x   = (pos[f].col * font->Width) + left_margin;
			pos[f].y   = (pos[f].row * (font->Height+line_spacing))+top_margin;

    		pos[f].updated = 0;

    	}
    }

 	RedrawTerminalContents();
 }

/**
 * Configura a caixa de selecao.
 *
 * Recalcula novas coordenadas de tela de acordo com as coordenadas de terminal.
 * Chamada a cada "resize" da janela principal e no caso de acoes de teclado ou
 * campo. Tambem forca o recalculo de todos os indicadores e redesenho da caixa
 * de selecao.
 *
 */
 void ConfigureSelectionBox(void)
 {
 	int f;

 	if(!selecting)
 	   return;

	DBGPrintf("Select from %ldx%ld to %ldx%ld",
						pos[0].row,pos[0].col,
						pos[1].row,pos[1].col);

    for(f=0;f<SELECTION_COUNT;f++)
    {
	   pos[f].updated 	= 1;
	   pos[f].x   		= (pos[f].col * font->Width) + left_margin;
	   pos[f].y   		= (pos[f].row * (font->Height+line_spacing))+top_margin;
    }

    InvalidateSelectionBox();
 }

 static void SelectCursor(int cr)
 {
 	if(cr == selected_cursor)
 	   return;

	selected_cursor = cr;
	DBGTrace(selected_cursor);

    if(terminal && terminal->window)
    {
 	   if(cr < 0)
 	   {
	      if(pointer)
             gdk_window_set_cursor(terminal->window,pointer);
	      return;
 	   }
       gdk_window_set_cursor(terminal->window,cursor[cr].cursor);
    }
 }

 void DrawSelectionBox(GdkDrawable *drawable, GdkGC *gc)
 {
 	int x[2];
 	int y[2];

 	if(!selecting)
 	   return;

    x[0] = min(pos[SELECTION_BEGIN].x,pos[SELECTION_END].x);
    x[1] = max(pos[SELECTION_BEGIN].x,pos[SELECTION_END].x);

    y[0] = min(pos[SELECTION_BEGIN].y,pos[SELECTION_END].y);
    y[1] = max(pos[SELECTION_BEGIN].y,pos[SELECTION_END].y);

    if( (x[0] == x[1]) || (y[0] == y[1]) )
       return;

    gdk_gc_set_foreground(gc,selection_cmap);
    gdk_draw_rectangle(drawable,gc,1,x[0],y[0],x[1]-x[0],y[1]-y[0]);

    gdk_gc_set_foreground(gc,selection_cmap+1);
    gdk_draw_rectangle(drawable,gc,0,x[0],y[0],x[1]-x[0],y[1]-y[0]);

 }

 gboolean mouse_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
 {

    DBGTracex(event->button);
    DBGTracex(event->type);

    switch( ((event->type & 0x0F) << 4) | (event->button & 0x0F))
 	{
	case ((GDK_BUTTON_PRESS & 0x0F) << 4) | 1:
	   DBGMessage("Button 1 Click");

	   pos[SELECTION_BEGIN].updated =
       pos[SELECTION_END].updated   = 1;

	   pos[SELECTION_BEGIN].x       =
       pos[SELECTION_END].x         = (unsigned long) event->x;

       pos[SELECTION_BEGIN].y       =
       pos[SELECTION_END].y         = (unsigned long) event->y;

       selecting = 1;
	   break;

	case ((GDK_2BUTTON_PRESS & 0x0F) << 4) | 1:
	   DBGMessage("Button 1 Double-Click");
       selecting = 3;
	   break;

	case ((GDK_BUTTON_PRESS & 0x0F) << 4) | 3:
	   DBGMessage("Button 2 Click");
	   modifier = 1;
	   break;

	case ((GDK_2BUTTON_PRESS & 0x0F) << 4) | 3:
	   DBGMessage("Button 2 Double-Click");
	   modifier = 2;
	   break;

	default:
	   DBGPrintf("Unknown mouse click %d with button %d",event->type, event->button);
	   return 0;

 	}

    InvalidateSelectionBox();

 	return 0;
 }

 static unsigned long copy_timer = 0;

 static void RemoveCopyTimer(void)
 {
    if(copy_timer)
	{
	   RemoveTimeOut(copy_timer);
	   copy_timer = 0;
	}
 }

 static void copy_text(void)
 {
    DBGMessage("**** TIMED COPY ****");
    RemoveCopyTimer();
    action_copy(0,0);
 }

 static void FieldAction(int row, int col)
 {
    int 			rows;
    int 			cols;
    int 			baddr;
    const struct ea *trm   = Get3270DeviceBuffer(&rows, &cols);
    int				length;
    int				maxlength;

	if(!trm)
	   return;

    baddr     = find_field_attribute((row * cols) + col);
    maxlength = (rows * cols) - baddr;

    DBGTrace(maxlength);

    for(length=1; !trm[baddr+length].fa && length < maxlength;length++);
    length--;

    DBGTrace(length);

    /*
     * If the field begins with "F", is protected and folowed by numeric
     * digits act as a clickable function key indicator.
     *
     */
    if(ebc2asc[trm[baddr+1].cc] == 'F' && length < 4 && FA_IS_PROTECTED(trm[baddr].fa))
    {
    	int  key = 0;
    	int  f;

    	for(f=1; f<length;f++)
    	{
    		unsigned char chr = ebc2asc[trm[baddr+1+f].cc];
    		DBGPrintf("[%c]",chr);
    		if(key >= 0 && isdigit(chr))
    		{
    			key *= 10;
    			key += (chr - '0');
    		}
    		else
    		{
    			key = -1;
    		}
    	}

    	if(key > 0 && key < 12)
    	{
    		// It's a function key! Activate it!
    	    char ks[6];
    	    snprintf(ks,5,"%d",key);
			DBGPrintf("Function: %s",ks);
			action_internal(PF_action, IA_DEFAULT, ks, CN);
    		return;
    	}
    }

    if (!FA_IS_PROTECTED(trm[baddr].fa))
    {
    	// It's not protected, try to select a word.
    	DBGMessage("Double-click in a editable field");
    }

 }

 gboolean mouse_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
 {
    int rows, cols;

    DBGTracex(((selecting & 0x0F) << 8) | ((modifier & 0x0F) << 4) | event->button);

    switch(((selecting & 0x0F) << 8) | ((modifier & 0x0F) << 4) | event->button)
    {
	case 0x101:	// Single click!
	   DBGPrintf("Move cursor position to %ld,%ld",pos[0].row,pos[0].col);
	   if(Get3270DeviceBuffer(&rows, &cols))
          move3270Cursor((pos[0].row * cols) + pos[0].col);
	   break;

	case 0x201:	// Selecting box
	   DBGMessage("Finish selection");
	   break;

    case 0x213:	// Timed Copy!
       copy_timer = AddTimeOut(500, copy_text);
       break;

    case 0x211: // Direct Copy
       copy_text();
       break;

	case 0x223:	// Append!
	   DBGMessage("Append");
       RemoveCopyTimer();
	   action_append(0,0);
	   break;

    case 0x301:	// Double-click (left)
       FieldAction(pos[0].row,pos[0].col);
       break;

    case 0xe13: // Selection-click
	case 0x013:	// Single-left-click

       selecting  = SELECTING_CLICK;
       pos[0].col = pos[1].col = cursor_col;
       pos[0].row = pos[1].row = cursor_row;
       ConfigureSelectionBox();

       pos[1].updated = 1;
       pos[1].x       = (unsigned long) event->x;
       pos[1].y       = (unsigned long) event->y;

       InvalidateSelectionBox();
	   break;

    }

    DBGTrace(event->button);

    if(event->button == 1)
    {
	   selecting =
	   modifier  = 0;
       SelectCursor(-1);
    }

    return 0;
 }

 gboolean mouse_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
 {
    switch(selecting)
    {
	case 1:	// Single click
	case 2: // Selecting
	   pos[SELECTION_END].updated	= 1;
       pos[SELECTION_END].x         = (unsigned long) event->x;
       pos[SELECTION_END].y         = (unsigned long) event->y;
       selecting = 2;
       SelectCursor( (pos[SELECTION_BEGIN].x > pos[SELECTION_END].x ? 0 : 1) | (pos[SELECTION_BEGIN].y > pos[SELECTION_END].y ? 0 : 2));
       InvalidateSelectionBox();
	   break;

    }

    return 0;
 }

 #if GTK == 2

 gboolean mouse_scroll(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
 {
    // FIXME (perry#1#): Read associoation from scroll to function key from configuration file.
    static const char *scroll[] = { "8", "7" };

    if(event->direction < 2 && !WaitForScreen)
 	{
       WaitForScreen = TRUE;
       action_internal(PF_action, IA_DEFAULT, scroll[event->direction], CN);
	}

 	return 0;
 }

#endif

 void LoadMousePointers(GdkDrawable *drawable, GdkGC *gc)
 {
 	int f;
 	for(f = 0; f < (sizeof(cursor)/sizeof(CURSOR)); f++)
 		cursor[f].cursor = gdk_cursor_new(cursor[f].type);
 }

 void SetMousePointer(GdkCursor *cursor)
 {
 	if(!cursor)
 	   return;

    pointer = cursor;

    DBGTracex(pointer);

    if(!selecting)
    {
	   if(terminal && terminal->window && pointer)
	      gdk_window_set_cursor(terminal->window,pointer);
    }
 }

 void action_remove_selection(GtkWidget *w, gpointer data)
 {
 	selecting = 0;
 	InvalidateSelectionBox();
 }

 void action_select_all(GtkWidget *w, gpointer data)
 {
 	int rows;
 	int cols;

    if(!Get3270DeviceBuffer(&rows, &cols))
       return;

    pos[0].updated =
    pos[1].updated = 1;

    pos[0].x       =
    pos[0].y	   = 0;

	pos[1].x       = (cols * font->Width) + left_margin;
	pos[1].y       = (rows * (font->Height+line_spacing))+top_margin;

 	selecting = 2;
 	InvalidateSelectionBox();
 }

 #define GetTerminalSize()	int rows, cols ; \
							if(!Get3270DeviceBuffer(&rows, &cols)) return;


 void action_SelectRight(GtkWidget *w, gpointer data)
 {
 	GetTerminalSize();

    if(!selecting)
    {
       selecting  = SELECTING_KEYBOARD;
       pos[0].col =
       pos[1].col = cursor_col;
       pos[0].row = cursor_row;
       pos[1].row = cursor_row+1;
    }

	if(pos[1].col < cols)
	   pos[1].col++;

    ConfigureSelectionBox();

 }

 void action_SelectDown(GtkWidget *w, gpointer data)
 {
 	GetTerminalSize();

    if(!selecting)
    {
       selecting  = SELECTING_KEYBOARD;
       pos[0].col = cursor_col;
       pos[1].col = cursor_col+1;
       pos[0].row =
       pos[1].row = cursor_row;
    }

	if(pos[1].row < rows)
	   pos[1].row++;

    ConfigureSelectionBox();

 }

 void action_SelectLeft(GtkWidget *w, gpointer data)
 {
    if(!selecting)
    {
       selecting = SELECTING_KEYBOARD;
       pos[0].col =
       pos[1].col = cursor_col;
       pos[0].row = cursor_row;
       pos[1].row = cursor_row+1;
    }

	if(pos[1].col > 0)
	   pos[1].col--;

    ConfigureSelectionBox();

 }

 void action_SelectUp(GtkWidget *w, gpointer data)
 {
 	GetTerminalSize();

    if(!selecting)
    {
       selecting  = SELECTING_KEYBOARD;
       pos[0].col = cursor_col;
       pos[1].col = cursor_col+1;
       pos[0].row =
       pos[1].row = cursor_row;
    }

	if(pos[1].row > 0)
	   pos[1].row--;

    ConfigureSelectionBox();

 }

 void action_SelectionUp(GtkWidget *w, gpointer data)
 {
    GetTerminalSize();

	if(!selecting)
	   return;

    if(pos[0].row <= 1 || pos[1].row < 1)
       return;

	CHKPoint();

    pos[0].row--;
    pos[1].row--;
    ConfigureSelectionBox();

 }

 void action_SelectionDown(GtkWidget *w, gpointer data)
 {
    GetTerminalSize();

	if(!selecting)
	   return;

    if(pos[0].row >= rows || pos[1].row >= rows)
       return;

	CHKPoint();

    pos[0].row++;
    pos[1].row++;
    ConfigureSelectionBox();
 }

 void action_SelectionLeft(GtkWidget *w, gpointer data)
 {
    GetTerminalSize();

	if(!selecting)
	   return;

    if(pos[0].col < 1 || pos[1].col < 1)
       return;

	CHKPoint();

    pos[0].col--;
    pos[1].col--;
    ConfigureSelectionBox();
 }

 void action_SelectionRight(GtkWidget *w, gpointer data)
 {
    GetTerminalSize();

	if(!selecting)
	   return;

    if(pos[0].col >= cols || pos[1].col >= cols)
       return;

	CHKPoint();

    pos[0].col++;
    pos[1].col++;
    ConfigureSelectionBox();
 }

 void action_RestoreSelection(GtkWidget *w, gpointer data)
 {
 	if(!selecting)
 	{
 	   selecting = SELECTING_RESTORED;
       ConfigureSelectionBox();
 	}
 }

 void action_copy(GtkWidget *w, gpointer data)
 {
 	int row[2];
 	int col[2];

 	if(!selecting)
 	   return;

    row[0] = min(pos[SELECTION_BEGIN].row,pos[SELECTION_END].row);
    row[1] = max(pos[SELECTION_BEGIN].row,pos[SELECTION_END].row);

    col[0] = min(pos[SELECTION_BEGIN].col,pos[SELECTION_END].col);
    col[1] = max(pos[SELECTION_BEGIN].col,pos[SELECTION_END].col);

    DBGPrintf("Copy %dx%d<->%dx%d to clipboard",row[0],col[0],row[1],col[1]);

    if( (row[0] == row[1]) || (col[0] == col[1]) )
       return;

	CopyToClipboard(row[0],col[0],row[1],col[1]);

 }

 void action_append(GtkWidget *w, gpointer data)
 {
 	int row[2];
 	int col[2];

 	if(!selecting)
 	   return;

    row[0] = min(pos[SELECTION_BEGIN].row,pos[SELECTION_END].row);
    row[1] = max(pos[SELECTION_BEGIN].row,pos[SELECTION_END].row);

    col[0] = min(pos[SELECTION_BEGIN].col,pos[SELECTION_END].col);
    col[1] = max(pos[SELECTION_BEGIN].col,pos[SELECTION_END].col);

    DBGPrintf("Append %dx%d<->%dx%d to clipboard",row[0],col[0],row[1],col[1]);

    if( (row[0] == row[1]) || (col[0] == col[1]) )
       return;

    AppendToClipboard(row[0],col[0],row[1],col[1]);

 }

 static gpointer exec_with_selection_thread(gpointer cmd)
 {
 	int				rows;
 	int				cols;
 	const struct ea *screen;
 	const struct ea *trm;
 	char			*ptr;
 	char			*mark;

 	char			buffer[1024];

 	int				fd;

    int col;
    int row;

 	int bCol = min(pos[SELECTION_BEGIN].col,pos[SELECTION_END].col);
 	int fCol = max(pos[SELECTION_BEGIN].col,pos[SELECTION_END].col);

 	int bRow = min(pos[SELECTION_BEGIN].row,pos[SELECTION_END].row);
 	int fRow = max(pos[SELECTION_BEGIN].row,pos[SELECTION_END].row);

 	char 			filename[1024];
 	FILE			*arq;

 	if(!cmd)
 	   return 0;

 	screen = Get3270DeviceBuffer(&rows, &cols);

 	if(bCol < 0 || bRow < 0)
 	{
       gdk_threads_enter();

       GtkWidget *widget = gtk_message_dialog_new(
					    GTK_WINDOW(top_window),
                        GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                        GTK_MESSAGE_WARNING,
                        GTK_BUTTONS_OK,
                        _( "Selecione algum texto antes" ) );

       gtk_dialog_run(GTK_DIALOG(widget));
       gtk_widget_destroy (widget);
       gdk_threads_leave();
 	   return 0;
 	}

    snprintf(filename,1023,"%s/%s.XXXXXX",TMPPATH,TARGET);
    fd = mkstemp(filename);

    DBGMessage(filename);

 	arq = fdopen(fd,"w");

 	if(arq)
 	{
	   DBGPrintf("Printing from %d,%d to %d,%d",bRow,bCol,fRow,fCol);

       for(row=bRow;row<fRow;row++)
       {
	      trm  = screen + ((row * cols)+bCol);
		  mark = ptr = buffer;

    	  for(col=bCol;col<fCol;col++)
    	  {
             *ptr = ebc2asc[trm->cc];
    		 if(*ptr > ' ')
    		    mark = ptr;
    		 ptr++;
    		 trm++;
		   }
    	   *(mark+1) = 0;

    	   DBGPrintf("%d\t%s",row,buffer);

    	   fprintf(arq,"%s\n",buffer);
       }

       fclose(arq);

	   snprintf(buffer,1023,cmd,filename);
	   DBGMessage(buffer);
       system(buffer);

       remove(filename);

 	}
 	else
 	{
 		Error("Unable to open \"%s\" for writing",filename);
 	}

	return 0;
 }

 void action_exec_with_selection(GtkWidget *w, gpointer data)
 {
#if GTK == 2
    GThread   *thd = 0;
    thd =  g_thread_create( exec_with_selection_thread, (gpointer) data, 0, NULL);
#else
    pthread_t  thd = 0;
    pthread_create(&thd, NULL, (void * (*)(void *)) exec_with_selection_thread, data);
#endif
 }

 void action_print_selection(GtkWidget *w, gpointer data)
 {
    action_exec_with_selection(w,data ? data : "kprinter --nodialog -t " TARGET " %s");
 }

