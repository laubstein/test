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
 * Este programa está nomeado como config.c e possui 389 linhas de código.
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

 #include <gtk/gtk.h>
 #include "common.h"
 #include <stdarg.h>
 #include <glib/gstdio.h>

#ifdef WIN32
	#include <windows.h>
	#define WIN_REGISTRY_ENABLED 1
#endif // WIN32

/*--[ Globals ]--------------------------------------------------------------------------------------*/

#ifdef WIN_REGISTRY_ENABLED

 	static const gchar *registry_path = "SOFTWARE\\"PACKAGE_NAME;

#else

	static GKeyFile		* program_config = NULL;
 	static const gchar	* mask = "%s" G_DIR_SEPARATOR_S PACKAGE_NAME ".conf";

#endif // WIN_REGISTRY_ENABLED

/*--[ Implement ]------------------------------------------------------------------------------------*/

#ifdef WIN32

gchar * get_last_error_msg(void)
{
	LPVOID	  lpMsgBuf;
	DWORD	  dw	= GetLastError();
	gchar	* ptr;
	gsize 	  bytes_written;

	FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					dw,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf,
					0, NULL );

	for(ptr=lpMsgBuf;*ptr != '\n';ptr++);
	*ptr = 0;

	ptr = g_locale_to_utf8(lpMsgBuf,strlen(lpMsgBuf)-1,NULL,&bytes_written,NULL);

	LocalFree(lpMsgBuf);

	return ptr;
}

#endif // WIN32

#ifdef WIN_REGISTRY_ENABLED
 static BOOL registry_open_key(const gchar *group, const gchar *key, REGSAM samDesired, HKEY *hKey)
 {
 	static HKEY	  predefined[] = { HKEY_CURRENT_USER, HKEY_USERS, HKEY_LOCAL_MACHINE };
 	int			  f;
	gchar		* path = g_strdup_printf("%s\\%s\\%s",registry_path,group,key);

	for(f=0;f<G_N_ELEMENTS(predefined);f++)
	{
		if(RegOpenKeyEx(predefined[f],path,0,samDesired,hKey) == ERROR_SUCCESS)
		{
			g_free(path);
			return TRUE;
		}
	}

	g_free(path);
	return FALSE;
 }
#else
 static gchar * search_for_ini(void)
 {
 	static const gchar * (*dir[])(void) =
 	{
 		g_get_user_config_dir,
 		g_get_user_data_dir,
 		g_get_home_dir,

	};
 	gchar *filename;
 	int f;

	const gchar * const *sysconfig;

#ifdef DEBUG
	filename = g_strdup_printf(mask,".");
	trace("Checking for %s",filename);
	if(g_file_test(filename,G_FILE_TEST_IS_REGULAR))
		return filename;
	g_free(filename);
#endif

 	for(f=0;f<G_N_ELEMENTS(dir);f++)
	{
		filename = g_strdup_printf(mask,dir[f]());
		trace("Checking for %s",filename);
		if(g_file_test(filename,G_FILE_TEST_IS_REGULAR))
			return filename;
		g_free(filename);
	}

	sysconfig = g_get_system_config_dirs();
 	for(f=0;sysconfig[f];f++)
	{
		filename = g_strdup_printf(mask,sysconfig[f]);
		trace("Checking for %s",filename);
		if(g_file_test(filename,G_FILE_TEST_IS_REGULAR))
			return filename;
		g_free(filename);
	}

	sysconfig = g_get_system_data_dirs();
 	for(f=0;sysconfig[f];f++)
	{
		filename = g_strdup_printf(mask,sysconfig[f]);
		trace("Checking for %s",filename);
		if(g_file_test(filename,G_FILE_TEST_IS_REGULAR))
			return filename;
		g_free(filename);
	}

 	return g_strdup_printf(mask,g_get_user_config_dir());
 }
#endif  //  #ifdef WIN_REGISTRY_ENABLED

 gboolean get_boolean_from_config(const gchar *group, const gchar *key, gboolean def)
 {
#ifdef WIN_REGISTRY_ENABLED

	HKEY key_handle;

	if(registry_open_key(group,key,KEY_READ,&key_handle))
	{
		DWORD			  data;
		gboolean		  ret 		= def;
		unsigned long	  datalen	= sizeof(data);
		unsigned long	  datatype;

		if(RegQueryValueExA(key_handle,NULL,NULL,&datatype,(BYTE *) &data,&datalen) == ERROR_SUCCESS)
		{
			if(datatype == REG_DWORD)
				ret = data ? TRUE : FALSE;
			else
				g_warning("Unexpected registry data type in %s\\%s\\%s",registry_path,group,key);
		}

		RegCloseKey(key_handle);

		return ret;

	}

#else

	if(program_config)
	{
		GError		* err = NULL;
		gboolean	  val = g_key_file_get_boolean(program_config,group,key,&err);
		if(err)
			g_error_free(err);
		else
			return val;
	}
#endif // WIN_REGISTRY_ENABLED

	return def;
 }

 gint get_integer_from_config(const gchar *group, const gchar *key, gint def)
 {
#ifdef WIN_REGISTRY_ENABLED

	HKEY key_handle;

	if(registry_open_key(group,key,KEY_READ,&key_handle))
	{
		DWORD			data;
		gint			ret = def;
		unsigned long	datalen	= sizeof(data);
		unsigned long	datatype;

		if(RegQueryValueExA(key_handle,NULL,NULL,&datatype,(BYTE *) &data,&datalen) == ERROR_SUCCESS)
		{
			if(datatype == REG_DWORD)
				ret = (gint) data;
			else
				g_warning("Unexpected registry data type in %s\\%s\\%s",registry_path,group,key);
		}

		RegCloseKey(key_handle);

		return ret;

	}

#else

	if(program_config)
	{
		GError	* err = NULL;
		gint	  val = g_key_file_get_integer(program_config,group,key,&err);
		if(err)
			g_error_free(err);
		else
			return val;
	}
#endif // WIN_REGISTRY_ENABLED

	return def;
 }


 gchar * get_string_from_config(const gchar *group, const gchar *key, const gchar *def)
 {
#ifdef WIN_REGISTRY_ENABLED

	HKEY key_handle;
	BYTE data[4096];
	unsigned long datatype;
	unsigned long datalen 	= sizeof(data);
	gchar *ret				= NULL;

	if(!registry_open_key(group,key,KEY_READ,&key_handle))
		return g_strdup(def);

	if(RegQueryValueExA(key_handle,NULL,NULL,&datatype,data,&datalen) == ERROR_SUCCESS)
	{
		ret = (char *) malloc(datalen+1);
		memcpy(ret,data,datalen);
		ret[datalen+1] = 0;
	}
	else if(def)
	{
		ret = g_strdup(def);
	}

	RegCloseKey(key_handle);

	return ret;

#else

	if(program_config)
	{
		gchar *ret = g_key_file_get_string(program_config,group,key,NULL);
		if(ret)
			return ret;
	}

	if(def)
		return g_strdup(def);

	return NULL;

#endif // WIN_REGISTRY_ENABLED
 }

void configuration_init(void)
{
#ifdef WIN_REGISTRY_ENABLED

#else

	gchar *filename = search_for_ini();

	if(program_config)
		g_key_file_free(program_config);

	program_config = g_key_file_new();

	if(filename)
	{
		g_key_file_load_from_file(program_config,filename,G_KEY_FILE_NONE,NULL);
		g_free(filename);
	}

#endif // WIN_REGISTRY_ENABLED
}

static void set_string(const gchar *group, const gchar *key, const gchar *fmt, va_list args)
{
	gchar * value = g_strdup_vprintf(fmt,args);

#ifdef WIN_REGISTRY_ENABLED

	gchar * path = g_strdup_printf("%s\\%s",registry_path,group);
 	HKEY	hKey;
 	DWORD	disp;

	if(RegCreateKeyEx(HKEY_CURRENT_USER,path,0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&hKey,&disp) == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey,key,0,REG_SZ,(const BYTE *) value,strlen(value)+1);
		RegCloseKey(hKey);
	}

	g_free(path);

#else

	g_key_file_set_string(program_config,group,key,value);

#endif

	g_free(value);
}

void set_string_to_config(const gchar *group, const gchar *key, const gchar *fmt, ...)
{
 	va_list args;
	va_start(args, fmt);
	set_string(group,key,fmt,args);
	va_end(args);
}

void set_boolean_to_config(const gchar *group, const gchar *key, gboolean val)
{
#ifdef WIN_REGISTRY_ENABLED

 	HKEY	hKey;
 	DWORD	disp;
	gchar * path = g_strdup_printf("%s\\%s",registry_path,group);

	trace("Creating key %s",path);
	if(RegCreateKeyEx(HKEY_CURRENT_USER,path,0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&hKey,&disp) == ERROR_SUCCESS)
	{
		DWORD	value = val ? 1 : 0;
		LONG	rc = RegSetValueEx(hKey, key, 0, REG_DWORD,(const BYTE *) &value,sizeof(value));

		SetLastError(rc);

		if(rc != ERROR_SUCCESS)
		{
			gchar *msg = get_last_error_msg();
			g_warning("Error \"%s\" when setting key HKCU\\%s\\%s",msg,path,key);
			g_free(msg);
		}
		RegCloseKey(hKey);
	}
	else
	{
		gchar *msg = get_last_error_msg();
		g_warning("Error \"%s\" when creating key HKCU\\%s",msg,path);
		g_free(msg);
	}

	g_free(path);

#else

	if(program_config)
		g_key_file_set_boolean(program_config,group,key,val);

#endif // WIN_REGISTRY_ENABLED

}

void set_integer_to_config(const gchar *group, const gchar *key, gint val)
{
#ifdef WIN_REGISTRY_ENABLED

 	HKEY	hKey;
 	DWORD	disp;
	gchar * path = g_strdup_printf("%s\\%s",registry_path,group);

	trace("Creating key %s",path);
	if(RegCreateKeyEx(HKEY_CURRENT_USER,path,0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&hKey,&disp) == ERROR_SUCCESS)
	{
		DWORD	value = (DWORD) val;
		LONG	rc = RegSetValueEx(hKey, key, 0, REG_DWORD,(const BYTE *) &value,sizeof(value));

		SetLastError(rc);

		if(rc != ERROR_SUCCESS)
		{
			gchar *msg = get_last_error_msg();
			g_warning("Error \"%s\" when setting key HKCU\\%s\\%s",msg,path,key);
			g_free(msg);
		}
		RegCloseKey(hKey);
	}
	else
	{
		gchar *msg = get_last_error_msg();
		g_warning("Error \"%s\" when creating key HKCU\\%s",msg,path);
		g_free(msg);
	}

	g_free(path);

#else

	if(program_config)
		g_key_file_set_integer(program_config,group,key,val);

#endif // WIN_REGISTRY_ENABLED

}

void configuration_deinit(void)
{
#ifdef WIN_REGISTRY_ENABLED

#else

	gchar *text;

	if(!program_config)
		return;

	text = g_key_file_to_data(program_config,NULL,NULL);

	if(text)
	{
		gchar *filename = g_strdup_printf(mask,g_get_user_config_dir());

		trace("Saving configuration in \"%s\"",filename);

		g_mkdir_with_parents(g_get_user_config_dir(),S_IRUSR|S_IWUSR);

		g_file_set_contents(filename,text,-1,NULL);

		g_free(text);
	}


	g_key_file_free(program_config);
	program_config = NULL;

#endif // WIN_REGISTRY_ENABLED
}

gchar * build_data_filename(const gchar *first_element, ...)
{
	va_list			  args;
	GString 		* result;
	const gchar 	* element;

#if defined( WIN_REGISTRY_ENABLED )

	static const gchar *reg_datadir = "SOFTWARE\\"PACKAGE_NAME"\\datadir";

 	HKEY	hKey = 0;
 	LONG	rc = 0;

	// Note: This could be needed: http://support.microsoft.com/kb/556009
	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129(v=vs.85).aspx

#ifndef KEY_WOW64_64KEY
	#define KEY_WOW64_64KEY 0x0100
#endif // KEY_WOW64_64KEY

#ifndef KEY_WOW64_32KEY
	#define KEY_WOW64_32KEY	0x0200
#endif // KEY_WOW64_64KEY

	rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,reg_datadir,0,KEY_QUERY_VALUE|KEY_WOW64_64KEY,&hKey);
	SetLastError(rc);

	if(rc == ERROR_SUCCESS)
	{
		char			data[4096];
		unsigned long	datalen	= sizeof(data);		// data field length(in), data returned length(out)
		unsigned long	datatype;					// #defined in winnt.h (predefined types 0-11)

		if(RegQueryValueExA(hKey,NULL,NULL,&datatype,(LPBYTE) data,&datalen) == ERROR_SUCCESS)
		{
			result = g_string_new(g_strchomp(data));
		}
		else
		{
			gchar *msg = get_last_error_msg();
			g_warning("Error \"%s\" getting value from HKLM\\%s",msg,reg_datadir);
			g_free(msg);
			result = g_string_new(".");
		}
		RegCloseKey(hKey);
	}
	else
	{
		static const gchar *datadir = NULL;

		gchar *msg = get_last_error_msg();
		g_warning("Error \"%s\" opening HKLM\\%s, searching system config dirs",msg,reg_datadir);
		g_free(msg);

		if(!datadir)
		{
			const gchar * const * dir = g_get_system_config_dirs();
			int f;

			for(f=0;dir[f] && !datadir;f++)
			{
				gchar *name = g_build_filename(dir[f],PACKAGE_NAME,NULL);

				trace("Searching for \"%s\"",name);

				if(g_file_test(name,G_FILE_TEST_IS_DIR))
					datadir = name;
				else
					g_free(name);
			}

			if(!datadir)
			{
				datadir = g_get_current_dir();
				g_warning("Unable to find application datadir, using %s",datadir);
			}

		}

		result = g_string_new(datadir);

	}

#elif defined(APPDATA) && !defined(WIN32)

	result = g_string_new(APPDATA);

#else

	static const gchar *datadir = NULL;

	if(!datadir)
	{
		const gchar * const * dir = g_get_system_data_dirs();
		int f;

		for(f=0;dir[f] && !datadir;f++)
		{
			gchar *name = g_build_filename(dir[f],PACKAGE_NAME,NULL);
			if(g_file_test(name,G_FILE_TEST_IS_DIR))
				datadir = name;
			else
				g_free(name);
		}

		if(!datadir)
		{
			datadir = g_get_current_dir();
			g_warning("Unable to find application datadir, using %s",datadir);
		}

	}

	result = g_string_new(datadir);

#endif

	va_start(args, first_element);

	for(element = first_element;element;element = va_arg(args, gchar *))
    {
    	g_string_append_c(result,G_DIR_SEPARATOR);
    	g_string_append(result,element);
    }

	va_end(args);

	return g_string_free(result, FALSE);
}
