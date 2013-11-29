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
 * programa; se não, escreva para a Free Software Foundation, Inc., 51 Franklin
 * St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Este programa está nomeado como ftdialog.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

 #include "v3270ft.h"

/*--[ Widget definition ]----------------------------------------------------------------------------*/

 enum _filename
 {
 	FILENAME_LOCAL,
 	FILENAME_HOST,

	FILENAME_COUNT
 };

 enum _value
 {
 	VALUE_LRECL,
 	VALUE_BLKSIZE,
 	VALUE_PRIMSPACE,
 	VALUE_SECSPACE,
 	VALUE_DFT,

 	VALUE_COUNT
 };

 struct _v3270FTD
 {
	GtkDialog			  parent;
	GtkWidget			* filename[FILENAME_COUNT];	/**< Filenames for the transfer */
	int					  value[VALUE_COUNT];
	LIB3270_FT_OPTION	  options;
 };

 struct _v3270FTDClass
 {
	GtkDialogClass parent_class;

	int dummy;
 };

 G_DEFINE_TYPE(v3270FTD, v3270FTD, GTK_TYPE_DIALOG);

 struct ftoptions
 {
	LIB3270_FT_OPTION	  flag;
	const gchar 		* label;
	const gchar			* tooltip;
 };

 struct ftvalues
 {
	int					  id;
	const gchar 		* label;
	const gchar			* tooltip;
 };

/*--[ Implement ]------------------------------------------------------------------------------------*/

static void v3270FTD_class_init(v3270FTDClass *klass)
{
//	GtkDialogClass	* widget_class	= GTK_DIALOG_CLASS(klass);

#if GTK_CHECK_VERSION(3,0,0)

#else

	#error Implementar

#endif // GTK_CHECK_VERSION

}

static void v3270FTD_init(v3270FTD *widget)
{
}

static void browse_file(GtkButton *button,v3270FTD *parent)
{
	gboolean	  recv		= (parent->options & LIB3270_FT_OPTION_RECEIVE);
	GtkWidget 	* dialog	= gtk_file_chooser_dialog_new
	(
		recv ? _( "Select file to receive" ) : _( "Select file to send" ),
		GTK_WINDOW(parent),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		_("_Cancel" ),	GTK_RESPONSE_CANCEL,
		recv ? _("_Save") : _("_Send"), GTK_RESPONSE_ACCEPT,
		NULL
	);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_entry_set_text(GTK_ENTRY(parent->filename[FILENAME_LOCAL]),filename);
		g_free(filename);
	}

	gtk_widget_destroy(dialog);


}

static void toggle_option(GtkToggleButton *button, v3270FTD *dialog)
{
	const struct ftoptions *opt = (const struct ftoptions *) g_object_get_data(G_OBJECT(button),"cfg");

	if(gtk_toggle_button_get_active(button))
		dialog->options |= opt->flag;
	else
		dialog->options &= ~opt->flag;

//	g_message("Option \"%s\" is %s flag=%08lx",opt->label,gtk_toggle_button_get_active(button) ? "ON" : "OFF", (unsigned long) opt->flag);

}

static GtkWidget * ftoption_new(v3270FTD *dialog, const struct ftoptions *opt)
{
	GtkContainer	* frame = GTK_CONTAINER(gtk_frame_new(_("Transfer options")));
	GtkGrid			* grid = GTK_GRID(gtk_grid_new());
	int				  f;

	gtk_grid_set_row_homogeneous(grid,TRUE);
	gtk_grid_set_column_homogeneous(grid,TRUE);
	gtk_grid_set_column_spacing(grid,5);
	gtk_grid_set_row_spacing(grid,5);

	for(f=0;opt[f].label;f++)
	{
		GtkWidget * button = gtk_check_button_new_with_mnemonic(gettext(opt[f].label));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),(dialog->options & opt[f].flag) != 0);
		gtk_widget_set_tooltip_text(GTK_WIDGET(button),gettext(opt[f].tooltip));
		gtk_widget_set_hexpand(button,TRUE);
		gtk_grid_attach(grid,button,f&1,f/2,1,1);
		g_object_set_data(G_OBJECT(button),"cfg",(gpointer) &opt[f]);
		g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(toggle_option),dialog);
	}

	gtk_container_add(frame,GTK_WIDGET(grid));

	return GTK_WIDGET(frame);
}

/*
http://stackoverflow.com/questions/16539127/gtkentry-change-text-on-user-input
void entry_insert(GtkEntryBuffer *buffer, guint position, gchar *chars, guint n_chars, gpointer user_data)
{

}
static void setup_numeric_entry(GtkEntry *entry)
{
	gtk_entry_set_max_length(entry,10);
	gtk_entry_set_width_chars(entry,10);
	gtk_entry_set_alignment(entry,1);
	gtk_entry_set_input_purpose(entry,GTK_INPUT_PURPOSE_NUMBER);
	g_signal_connect_after(G_OBJECT(entry), "insert-text", G_CALLBACK(entry_insert),NULL);
}
*/

static GtkWidget * ftvalue_new(v3270FTD *dialog, GtkGrid *grid, int r, const struct ftvalues *val)
{
	int		  f;

	for(f=0;val[f].label;f++)
	{
		int				  col	= (f&1)*2;
		int				  row	= (f/2)+r;
		GtkWidget		* label	= gtk_label_new_with_mnemonic(gettext(val[f].label));
		GtkWidget		* entry = GTK_WIDGET(gtk_spin_button_new_with_range(0,99999,1));

		gtk_widget_set_hexpand(GTK_WIDGET(label),TRUE);
		gtk_widget_set_tooltip_text(GTK_WIDGET(label),gettext(val[f].tooltip));
		gtk_misc_set_alignment(GTK_MISC(label),0,0.5);

		gtk_label_set_mnemonic_widget(GTK_LABEL(label),GTK_WIDGET(entry));
		gtk_widget_set_tooltip_text(GTK_WIDGET(entry),gettext(val[f].tooltip));

		g_object_set_data(G_OBJECT(entry),"cfg",(gpointer) &val[f]);

		gtk_grid_attach(grid,GTK_WIDGET(label),col,row,1,1);
		gtk_grid_attach(grid,GTK_WIDGET(entry),col+1,row,1,1);

	}

	return GTK_WIDGET(grid);

}

static GtkWidget * ftradio_new(v3270FTD *dialog, const gchar *title, const gchar *tooltip, const struct ftoptions *opt)
{
	GtkContainer	* frame = GTK_CONTAINER(gtk_frame_new(title));
	GtkBox			* box	= GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2));
	GSList			* lst	= NULL;
	int				  f;

	for(f=0;opt[f].label;f++)
	{
		GtkWidget * button = gtk_radio_button_new_with_label(lst,gettext(opt[f].label));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),(dialog->options & opt[f].flag) != 0);
		if(opt[f].tooltip)
			gtk_widget_set_tooltip_text(button,gettext(opt[f].tooltip));

		g_object_set_data(G_OBJECT(button),"cfg",(gpointer) &opt[f]);
		g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(toggle_option),dialog);

		gtk_box_pack_start(box,button,FALSE,TRUE,2);
		lst =  gtk_radio_button_get_group(GTK_RADIO_BUTTON(button));
	}

	gtk_widget_set_tooltip_text(GTK_WIDGET(frame),tooltip);
	gtk_container_add(frame,GTK_WIDGET(box));
	return GTK_WIDGET(frame);
}

GtkWidget * v3270_dialog_ft_new(LIB3270_FT_OPTION options)
{
	v3270FTD *dialog = g_object_new(GTK_TYPE_V3270FTD, NULL);

	// Set defaults
	dialog->options = options;

	// Filename entry
	int f;

	GtkWidget * label[FILENAME_COUNT] =
	{
		gtk_label_new_with_mnemonic( _( "_Local file name:" ) ),
		gtk_label_new_with_mnemonic( _( "_Host file name:" ) )
	};

	for(f=0;f<FILENAME_COUNT;f++)
	{
		dialog->filename[f] = gtk_entry_new();
		gtk_label_set_mnemonic_widget(GTK_LABEL(label[f]),dialog->filename[f]);
		gtk_widget_set_hexpand(dialog->filename[f],TRUE);
	}

	GtkGrid *grid = GTK_GRID(gtk_grid_new());
	gtk_grid_set_row_homogeneous(grid,FALSE);
	gtk_grid_set_column_homogeneous(grid,FALSE);
	gtk_grid_set_column_spacing(grid,5);
	gtk_grid_set_row_spacing(grid,5);

	GtkButton * browse = GTK_BUTTON(gtk_button_new_from_icon_name("text-x-generic",GTK_ICON_SIZE_BUTTON));
	gtk_button_set_focus_on_click(browse,FALSE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(browse),_("Select file"));
	g_signal_connect(G_OBJECT(browse),"clicked",G_CALLBACK(browse_file),dialog);

	if(options & LIB3270_FT_OPTION_RECEIVE)
	{
		// It's receiving file first host filename, then local filename
		gtk_window_set_title(GTK_WINDOW(dialog),_( "Receive file from host" ));

		gtk_grid_attach(grid,label[FILENAME_HOST],0,0,1,1);
		gtk_grid_attach(grid,label[FILENAME_LOCAL],0,1,1,1);

		gtk_grid_attach(grid,dialog->filename[FILENAME_HOST],1,0,3,1);
		gtk_grid_attach(grid,dialog->filename[FILENAME_LOCAL],1,1,3,1);
		gtk_grid_attach(grid,GTK_WIDGET(browse),5,1,1,1);

		gtk_widget_set_tooltip_text(dialog->filename[FILENAME_HOST],_("Name of the source file."));
		gtk_widget_set_tooltip_text(dialog->filename[FILENAME_LOCAL],_("Where to save the received file."));

		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),GTK_WIDGET(grid),FALSE,TRUE,2);

		// Create options box
		static const struct ftoptions opt[] =
		{
				{
					LIB3270_FT_OPTION_ASCII,
					N_( "_Text file." ),
					N_( "Check for text files.")
				},
				{
					LIB3270_FT_OPTION_CRLF,
					N_( "Add _CR at end of line." ),
					N_( "Adds Newline characters to each host file record before transferring it to the local workstation.")
				},
				{
					LIB3270_FT_OPTION_APPEND,
					N_( "_Append to destination" ),
					N_( "Appends the source file to the destination file.")
				},
				{
					LIB3270_FT_OPTION_REMAP,
					N_("_Remap to ASCII Characters."),
					N_("Remap the text to ensure maximum compatibility between the workstation's character set and encoding and the host's EBCDIC code page.")
				},
				{
					0,
					NULL,
					NULL
				}
		};

		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),ftoption_new(dialog,opt),FALSE,TRUE,2);

		// Create values box
		/*
		static const struct ftvalues val[] =
		{
			{
				VALUE_DFT,
				N_( "DFT B_uffer size:" ),
				N_( "Buffer size for DFT-mode transfers. Can range from 256 to 32768. Larger values give better performance, but some hosts may not be able to support them." )
			},

			{
				0,
				NULL,
				NULL
			}
		};

		// gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),ftvalue_new(dialog,val),FALSE,TRUE,2);
		*/
	}
	else
	{
		// It's sending file first local filename, then hostfilename
		gtk_window_set_title(GTK_WINDOW(dialog),_( "Send file to host" ));

		gtk_grid_attach(grid,label[FILENAME_LOCAL],0,0,1,1);
		gtk_grid_attach(grid,label[FILENAME_HOST],0,1,1,1);

		gtk_grid_attach(grid,dialog->filename[FILENAME_LOCAL],1,0,3,1);
		gtk_grid_attach(grid,GTK_WIDGET(browse),5,0,1,1);

		gtk_grid_attach(grid,dialog->filename[FILENAME_HOST],1,1,3,1);

		gtk_widget_set_tooltip_text(dialog->filename[FILENAME_HOST],_("Name of the target file."));
		gtk_widget_set_tooltip_text(dialog->filename[FILENAME_LOCAL],_("Path of the local file to send."));

		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),GTK_WIDGET(grid),FALSE,TRUE,2);

		// Create options box
		static const struct ftoptions opt[] =
		{
				{
					LIB3270_FT_OPTION_ASCII,
					N_( "_Text file." ),
					N_( "Check for text files.")
				},
				{
					LIB3270_FT_OPTION_CRLF,
					N_( "_CR delimited file." ),
					N_( "Remove the default newline characters in local files before transferring them to the host.")
				},
				{
					LIB3270_FT_OPTION_APPEND,
					N_( "_Append to destination" ),
					N_( "Appends the source file to the destination file.")
				},
				{
					LIB3270_FT_OPTION_REMAP,
					N_("_Remap to EBCDIC Characters."),
					N_("Remap the text to ensure maximum compatibility between the workstation's character set and encoding and the host's EBCDIC code page.")
				},
				{
					0,
					NULL,
					NULL
				}
		};

		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),ftoption_new(dialog,opt),FALSE,TRUE,2);


		// Create format box
		GtkGrid * grid = GTK_GRID(gtk_grid_new());
		gtk_grid_set_column_spacing(grid,5);
		gtk_grid_set_row_spacing(grid,5);

		// Create record format box
		static const struct ftoptions recfm[] =
		{
			{
				LIB3270_FT_RECORD_FORMAT_DEFAULT,
				N_("Default"),
				N_("Use host default record format.")
			},
			{
				LIB3270_FT_RECORD_FORMAT_FIXED,
				N_("Fixed"),
				N_("Creates a file with fixed-length records.")
			},
			{
				LIB3270_FT_RECORD_FORMAT_VARIABLE,
				N_("Variable"),
				N_("Creates a file with variable-length records.")
			},
			{
				LIB3270_FT_RECORD_FORMAT_UNDEFINED,
				N_("Undefined"),
				N_("Creates a file with undefined-length records (TSO hosts only).")
			},
			{
				0,
				NULL,
				NULL
			}
		};

		gtk_grid_attach(	grid,
							ftradio_new(dialog,_("Record format"),_("Controls the record format of files created on the host."),recfm),
							0,0,2,1
						);


		// Create allocation unit box
		static const struct ftoptions units[] =
		{
			{
				LIB3270_FT_ALLOCATION_UNITS_DEFAULT,
				N_("Default"),
				NULL
			},
			{
				LIB3270_FT_ALLOCATION_UNITS_TRACKS,
				N_("Tracks"),
				NULL
			},
			{
				LIB3270_FT_ALLOCATION_UNITS_CYLINDERS,
				N_("Cylinders"),
				NULL
			},
			{
				LIB3270_FT_ALLOCATION_UNITS_AVBLOCK,
				N_("Avblock"),
				NULL
			},
			{
				0,
				NULL,
				NULL
			}
		};

		gtk_grid_attach(	grid,
							ftradio_new(dialog,_("Space allocation units"),_("Specifies the units for the TSO host primary and secondary space options."),units),
							2,0,2,1
						);


		// Create values box
		static const struct ftvalues val[] =
		{
			{
				VALUE_LRECL,
				N_( "Record Length:" ),
				N_( "Specifies the record length (or maximum record length) for files created on the host." )
			},


			{
				VALUE_PRIMSPACE,
				N_( "Primary space:" ),
				N_( "Primary allocation for a file created on a TSO host.\nThe units are given by the space allocation units option." )
			},

			{
				VALUE_BLKSIZE,
				N_( "Block size:" ),
				N_( "Specifies the block size for files created on the host (TSO hosts only)." )
			},

			{
				VALUE_SECSPACE,
				N_( "Secondary space:" ),
				N_( "Secondary allocation for a file created on a TSO host.\nThe units are given by the space allocation units option." )
			},

			{
				VALUE_DFT,
				N_( "DFT B_uffer size:" ),
				N_( "Buffer size for DFT-mode transfers. Can range from 256 to 32768. Larger values give better performance, but some hosts may not be able to support them." )
			},

			{
				0,
				NULL,
				NULL
			}
		};

		ftvalue_new(dialog,grid,1,val);

		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),GTK_WIDGET(grid),FALSE,TRUE,2);

	}

	return GTK_WIDGET(dialog);
}

