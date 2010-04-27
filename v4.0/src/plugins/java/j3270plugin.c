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
 * Este programa está nomeado como init.c e possui - linhas de código.
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

 #include <jni.h>

 #define LIB3270_MODULE_NAME "java"

 #include <lib3270/config.h>
 #define ENABLE_NLS
 #define GETTEXT_PACKAGE PACKAGE_NAME

 #ifndef WIN32
	#include <sys/types.h>
	#include <unistd.h>
 #endif

 #include <libintl.h>
 #include <glib/gi18n.h>

 #include <lib3270/api.h>
 #include <lib3270/plugins.h>

 // References:
 //		http://www.javafaq.nu/java-article1027.html
 //		http://blog.vinceliu.com/2008/01/writing-your-own-custom-loader-for-java.html

/*---[ Exports ]----------------------------------------------------------------------------------*/

 // extern void pw3270_call_java_script(GtkAction *action, GtkWidget *window);

/*---[ Statics ]----------------------------------------------------------------------------------*/

 static GStaticMutex	mutex	= G_STATIC_MUTEX_INIT;
 static JNIEnv			*env	= NULL;
 static JavaVM			*jvm	= NULL;

/*---[ Implement ]--------------------------------------------------------------------------------*/

static void run_class(GtkWidget *window, const gchar *classname, const gchar *argument, JNIEnv *env)
{
	jclass			 cls;
	jmethodID		 mid;
	jstring			 jstr;
	jobjectArray	 args;

	// Locate and load classfile
	if ((cls = (*env)->FindClass(env, classname)) == 0)
	{
		GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW(window),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_CANCEL,
													_(  "Can't find class %s" ), classname );

		gtk_window_set_title(GTK_WINDOW(dialog), _( "Java script failure" ));
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
		return;
	}

	if ((mid = (*env)->GetStaticMethodID(env, cls, "main", "([Ljava/lang/String;)V")) == 0)
	{
		GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW(window),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_CANCEL,
													_(  "Can't find main in class %s" ), classname );

		gtk_window_set_title(GTK_WINDOW(dialog), _( "Java script failure" ));
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
		return;
	}

	/* create a new java string to be passes to the class */
	if ((jstr = (*env)->NewStringUTF(env, argument ? argument : classname)) == 0)
	{
		GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW(window),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_CANCEL,
													_(  "Out of memory on %s" ), classname );

		gtk_window_set_title(GTK_WINDOW(dialog), _( "Java script failure" ));
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
		return;
	}

   /* create a new string array with a single element containing the string created above */
	args = (*env)->NewObjectArray(env, 1, (*env)->FindClass(env, "java/lang/String"), jstr);
	if (args == 0)
	{
		GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW(window),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_CANCEL,
													_(  "Out of memory on %s" ), classname );

		gtk_window_set_title(GTK_WINDOW(dialog), _( "Java script failure" ));
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
		return;
	}

#ifdef WIN32
	Trace("%s: Calling main(%p,%p,%p,%p)",__FUNCTION__,env,cls,mid,args);
#else
	Trace("%s: Calling main(%p,%p,%p,%p) on pid %d",__FUNCTION__,env,cls,mid,args,getpid());
#endif

	(*env)->CallStaticVoidMethod(env, cls, mid, args);
	Trace("%s: Ends",__FUNCTION__);

 }

 PW3270_PLUGIN_ENTRY void pw3270_call_java_script(GtkAction *action, GtkWidget *window)
 {
	const gchar	*argument	= g_object_get_data(G_OBJECT(action),"script_argument");;
 	const gchar	*classname	= g_object_get_data(G_OBJECT(action),"script_class");

	if(!classname)
	{
		GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW(window),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_CANCEL,
													"%s", _(  "Invalid or malformed action description" ));

		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),"%s",_( "There's no classname" ));
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
		return;
	}

	Trace("Classname: %s",classname);

 	if(!g_static_mutex_trylock(&mutex))
 	{
 		// Can't lock java script engine
		GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW(window),
													GTK_DIALOG_DESTROY_WITH_PARENT,
													GTK_MESSAGE_ERROR,
													GTK_BUTTONS_CANCEL,
													"%s", _(  "Can't start script" ));

		gtk_window_set_title(GTK_WINDOW(dialog), _( "System busy" ));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),"%s",_( "Please, try again in a few moments" ));

        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);

		g_static_mutex_unlock(&mutex);
		return;
 	}

	if(!jvm)
	{
		GKeyFile		*ini			= (GKeyFile *) g_object_get_data(G_OBJECT(window),"pw3270_config");
		JavaVMInitArgs	vm_args;
		JavaVMOption	options[5];
		gchar			*classpath		= NULL;
		gchar			*libpath		= NULL;
		gchar			*program_data	= g_object_get_data(G_OBJECT(window),"pw3270_dpath");;
		jint			rc = 0;
		int				f;


		// Load VM options
		vm_args.version		= JNI_VERSION_1_2;
		vm_args.nOptions	= 0;
		vm_args.options 	= options;

		if(ini)
		{
			// Load VM options
			classpath		= g_key_file_get_string(ini,"java","classpath",NULL);
			libpath			= g_key_file_get_string(ini,"java","libpath",NULL);
		}

		if(!program_data)
			program_data = ".";

#ifdef DEBUG
		options[vm_args.nOptions++].optionString = g_strdup("-verbose");
#endif

#if defined( WIN32 )

		if(libpath)
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.library.path=%s",libpath);
		else
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.library.path=%s;%s" G_DIR_SEPARATOR_S "java",program_data,program_data);

#elif defined( JNIDIR )
		if(libpath)
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.library.path=%s",libpath);
		else
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.library.path=%s:%s:%s" G_DIR_SEPARATOR_S "java",JNIDIR,program_data,program_data);
#else
		if(libpath)
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.library.path=%s",libpath);
#endif

#if defined( WIN32 )

		if(classpath)
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.class.path=%s",classpath);
		else
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.class.path=%s" G_DIR_SEPARATOR_S PACKAGE_NAME ".jar;%s;%s" G_DIR_SEPARATOR_S "java", \
																						program_data,program_data,program_data);

#elif defined( JARDIR )

		if(classpath)
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.class.path=%s",classpath);
		else
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.class.path=%s" G_DIR_SEPARATOR_S PACKAGE_NAME ".jar:%s:%s" G_DIR_SEPARATOR_S "java", \
																						JARDIR,program_data,program_data);
#else
		if(classpath)
			options[vm_args.nOptions++].optionString = g_strdup_printf("-Djava.class.path=%s",classpath);
#endif

		// Create Java Virtual machine
		rc = JNI_CreateJavaVM(&jvm,(void **)&env,&vm_args);

		// Release options
		for(f=0;f<vm_args.nOptions;f++)
		{
			Trace("Releasing option %d: %s",f,options[f].optionString);
			g_free(options[f].optionString);
		}

		if(rc < 0)
		{
			GtkWidget *dialog = gtk_message_dialog_new(	GTK_WINDOW(window),
														GTK_DIALOG_DESTROY_WITH_PARENT,
														GTK_MESSAGE_ERROR,
														GTK_BUTTONS_CANCEL,
														"%s", _(  "Can't create java VM" ));

			gtk_window_set_title(GTK_WINDOW(dialog), _( "Script startup failure" ));
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),_( "The error code was %d" ), rc);

			gtk_dialog_run(GTK_DIALOG (dialog));
			gtk_widget_destroy(dialog);

			jvm = NULL;
			g_static_mutex_unlock(&mutex);
			return;

		}
	}

	run_class(window,classname,argument,env);

	g_static_mutex_unlock(&mutex);
 }

