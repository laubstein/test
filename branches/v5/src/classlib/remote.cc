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
 * Este programa está nomeado como remote.cc e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

 #include <lib3270/config.h>

 #if defined(HAVE_DBUS)
	#include <stdio.h>
	#include <dbus/dbus.h>
	#include <string.h>
	#include <malloc.h>
	#include <sys/types.h>
	#include <unistd.h>
 #endif // HAVE_DBUS

 #if defined(WIN32)
	#include <windows.h>
	#include <pw3270/ipcpackets.h>
 #endif // WIN32

 #include <pw3270/class.h>
 #include <lib3270/log.h>

#if defined(HAVE_DBUS)
 static const char	* prefix_dest	= "br.com.bb.";
 static const char	* prefix_path	= "/br/com/bb/";
#endif // HAVE_DBUS

/*--[ Implement ]--------------------------------------------------------------------------------------------------*/

 namespace PW3270_NAMESPACE
 {

	class remote : public session
 	{
	private:
#if defined(WIN32)

		HANDLE			  hPipe;

		int query_intval(HLLAPI_PACKET id)
		{
			struct hllapi_packet_query      query		= { id };
			struct hllapi_packet_result		response;
			DWORD							cbSize		= sizeof(query);
			if(TransactNamedPipe(hPipe,(LPVOID) &query, cbSize, &response, sizeof(response), &cbSize,NULL))
				return response.rc;

			throw exception(GetLastError(),"%s","Transaction error");
		}

		string * query_string(void *query, size_t szQuery, size_t len)
		{
			struct hllapi_packet_text			* response;
			DWORD								  cbSize	= sizeof(struct hllapi_packet_text)+len;
			string								* s;

			response = (struct hllapi_packet_text *) malloc(cbSize+2);
			memset(response,0,cbSize+2);

			if(TransactNamedPipe(hPipe,(LPVOID) query, szQuery, &response, cbSize, &cbSize,NULL))
			{
				if(response->packet_id)
					s = new string("");
				else
					s = new string(response->text);
			}
			else
			{
				s = new string("");
			}

			free(response);

			return s;
		}

		int query_intval(void *pkt, size_t szQuery, bool dynamic = false)
		{
			struct hllapi_packet_result		response;
			DWORD							cbSize		= (DWORD) szQuery;
			BOOL 							status;

			status = TransactNamedPipe(hPipe,(LPVOID) pkt, cbSize, &response, sizeof(response), &cbSize,NULL);

			if(dynamic)
				free(pkt);

			if(status)
				return response.rc;

			throw exception(GetLastError(),"%s","Transaction error");

		}


#elif defined(HAVE_DBUS)

		#define HLLAPI_PACKET_IS_CONNECTED	"isConnected"
		#define HLLAPI_PACKET_GET_CSTATE	"getConnectionState"
		#define HLLAPI_PACKET_IS_READY		"isReady"
		#define HLLAPI_PACKET_DISCONNECT	"disconnect"
		#define HLLAPI_PACKET_GET_CURSOR	"getCursorAddress"
		#define HLLAPI_PACKET_ENTER			"enter"
		#define HLLAPI_PACKET_QUIT			"quit"

		DBusConnection	* conn;
		char			* dest;
		char			* path;
		char			* intf;

		DBusMessage * create_message(const char *method)
		{
			DBusMessage * msg = dbus_message_new_method_call(	this->dest,		// Destination
																this->path,		// Path
																this->intf,		// Interface
																method);		// method

			if (!msg)
				throw exception("Error creating DBUS message for method %s",method);

			return msg;

		}

		DBusMessage	* call(DBusMessage *msg)
		{
			DBusMessage		* reply;
			DBusError		  error;

			dbus_error_init(&error);
			reply = dbus_connection_send_with_reply_and_block(conn,msg,10000,&error);
			dbus_message_unref(msg);

			if(!reply)
			{
				exception e = exception("%s",error.message);
				dbus_error_free(&error);
				throw e;
			}
			return reply;
		}

		string * get_string(DBusMessage * msg)
		{
			if(msg)
			{
				DBusMessageIter iter;

				if(dbus_message_iter_init(msg, &iter))
				{
					if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING)
					{
						string 		* rc;
						const char	* str;
						dbus_message_iter_get_basic(&iter, &str);
						trace("Response: [%s]",str);
						rc = new string(str);
						dbus_message_unref(msg);
						return rc;
					}

					exception e = exception("DBUS Return type was %c, expecting %c",dbus_message_iter_get_arg_type(&iter),DBUS_TYPE_INT32);
					dbus_message_unref(msg);

					throw e;

				}

			}

			return NULL;
		}

		string * query_string(const char *method)
		{
			return get_string(call(create_message(method)));
		}

		int get_intval(DBusMessage * msg)
		{
			if(msg)
			{
				DBusMessageIter iter;

				if(dbus_message_iter_init(msg, &iter))
				{
					if(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INT32)
					{
						dbus_int32_t iSigned;
						dbus_message_iter_get_basic(&iter, &iSigned);
						dbus_message_unref(msg);
						return (int) iSigned;
					}

					exception e = exception("DBUS Return type was %c, expecting %c",dbus_message_iter_get_arg_type(&iter),DBUS_TYPE_INT32);

					dbus_message_unref(msg);
					throw e;
				}
			}
			return -1;
		}

		int query_intval(const char *method)
		{
			if(conn)
				return get_intval(call(create_message(method)));
			return -1;
		}

#else

		int query_intval(const char *method)
		{
			throw exception("Call to unimplemented RPC method \"%s\"",method);
			return -1
		}

#endif

	public:

		remote(const char *session)
		{
#if defined(WIN32)
			static DWORD	  dwMode = PIPE_READMODE_MESSAGE;
			char	 		  buffer[4096];
			char			* str = strdup(session);
			char			* ptr;

			hPipe  = INVALID_HANDLE_VALUE;

			for(ptr=str;*ptr;ptr++)
			{
				if(*ptr == ':')
					*ptr = '_';
				else
					*ptr = tolower(*ptr);
			}

			snprintf(buffer,4095,"\\\\.\\pipe\\%s",str);

			free(str);

			if(!WaitNamedPipe(buffer,NMPWAIT_USE_DEFAULT_WAIT))
			{
				exception e = exception(GetLastError(),"Service instance %s unavailable",str);
				free(str);
				throw e;
				return;
			}

			hPipe = CreateFile(buffer,GENERIC_WRITE|GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);

			if(hPipe == INVALID_HANDLE_VALUE)
			{
				throw exception("Can´t create service pipe %s",buffer);
			}
			else if(!SetNamedPipeHandleState(hPipe,&dwMode,NULL,NULL))
			{
				exception e = exception(GetLastError(),"%s","Can´t set pipe state");
				CloseHandle(hPipe);
				hPipe = INVALID_HANDLE_VALUE;
				throw e;
			}

#elif defined(HAVE_DBUS)

			DBusError	  err;
			int			  rc;
			char		* str = strdup(session);
			char		* ptr;
			char		  busname[4096];
			char		  pidname[10];
			int			  pid			= (int) getpid();

			trace("%s str=%p",__FUNCTION__,str);

			for(ptr=str;*ptr;ptr++)
				*ptr = tolower(*ptr);

			ptr = strchr(str,':');

			if(ptr)
			{
				size_t 		  sz;

				*(ptr++) = 0;

				// Build destination
				sz		= strlen(ptr)+strlen(str)+strlen(prefix_dest)+2;
				dest	= (char *) malloc(sz+1);
				strncpy(dest,prefix_dest,sz);
				strncat(dest,str,sz);
				strncat(dest,".",sz);
				strncat(dest,ptr,sz);

				// Build path
				sz		= strlen(str)+strlen(prefix_path);
				path	= (char *) malloc(sz+1);
				strncpy(path,prefix_path,sz);
				strncat(path,str,sz);

				// Build intf
				sz		= strlen(str)+strlen(prefix_dest)+1;
				intf	= (char *) malloc(sz+1);
				strncpy(intf,prefix_dest,sz);
				strncat(intf,str,sz);

			}
			else
			{
				size_t sz;

				// Build destination
				sz		= strlen(str)+strlen(prefix_dest)+2;
				dest	= (char *) malloc(sz+1);
				strncpy(dest,prefix_dest,sz);
				strncat(dest,str,sz);

				// Build path
				sz		= strlen(str)+strlen(prefix_path);
				path	= (char *) malloc(sz+1);
				strncpy(path,prefix_path,sz);
				strncat(path,str,sz);

				// Build intf
				sz		= strlen(str)+strlen(prefix_dest)+1;
				intf	= (char *) malloc(sz+1);
				strncpy(intf,prefix_dest,sz);
				strncat(intf,str,sz);

			}

			trace("DBUS:\nDestination:\t[%s]\nPath:\t\t[%s]\nInterface:\t[%s]",dest,path,intf);

			free(str);

			dbus_error_init(&err);

			conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
			trace("dbus_bus_get conn=%p",conn);

			if (dbus_error_is_set(&err))
			{
				exception e = exception("DBUS Connection Error (%s)", err.message);
				dbus_error_free(&err);
				throw e;
				return;
			}

			if(!conn)
			{
				throw exception("%s", "DBUS Connection failed");
				return;
			}

			memset(pidname,0,10);
			for(int f = 0; f < 9 && pid > 0;f++)
			{
				pidname[f] = 'a'+(pid % 25);
				pid /= 25;
			}

			snprintf(busname, 4095, "%s.rx3270.br.com.bb",pidname);

			trace("Busname: [%s]",busname);

			rc = dbus_bus_request_name(conn, busname, DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
			trace("dbus_bus_request_name rc=%d",rc);

			if (dbus_error_is_set(&err))
			{
				exception e = exception("Name Error (%s)", err.message);
				dbus_error_free(&err);
				throw e;
				return;
			}

			if(rc != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
			{
				trace("%s: DBUS request for name %s failed",__FUNCTION__, busname);
				throw exception("DBUS request for \"%s\" failed",session);
				return;
			}

			trace("%s: Using DBUS name %s",__FUNCTION__,busname);

			DBusMessage * msg = create_message("setScript");

			if(msg)
			{
				const char			* id   = "r";
				static const dbus_int32_t	  flag = 1;
				dbus_message_append_args(msg, DBUS_TYPE_STRING, &id, DBUS_TYPE_INT32, &flag, DBUS_TYPE_INVALID);
				get_intval(call(msg));
			}

#else

			throw exception("%s","RPC support is incomplete.");

#endif
		}

		virtual ~remote()
		{
#if defined(WIN32)

			if(hPipe != INVALID_HANDLE_VALUE)
				CloseHandle(hPipe);

#elif defined(HAVE_DBUS)

			try
			{
				DBusMessage * msg = create_message("setScript");
				if(msg)
				{
					const char					* id   = "r";
					static const dbus_int32_t	  flag = 0;
					dbus_message_append_args(msg, DBUS_TYPE_STRING, &id, DBUS_TYPE_INT32, &flag, DBUS_TYPE_INVALID);
					get_intval(call(msg));
				}
			}
			catch(exception e)
			{

			}

			free(dest);
			free(path);
			free(intf);

#else

#endif
		}

		bool is_connected(void)
		{
			return query_intval(HLLAPI_PACKET_IS_CONNECTED) != 0;
		}

		LIB3270_CSTATE get_cstate(void)
		{
			return (LIB3270_CSTATE) query_intval(HLLAPI_PACKET_GET_CSTATE);
		}

		int connect(const char *uri, bool wait)
		{
#if defined(WIN32)

			size_t							  cbSize	= sizeof(struct hllapi_packet_connect)+strlen(uri);
			struct hllapi_packet_connect	* pkt		= (struct hllapi_packet_connect *) malloc(cbSize);

			pkt->packet_id	= HLLAPI_PACKET_CONNECT;
			pkt->wait		= (unsigned char) wait;
			strcpy(pkt->hostname,uri);

			trace("Sending %s",pkt->hostname);

			return query_intval((void *) pkt,cbSize,true);

#elif defined(HAVE_DBUS)

			int rc;
			DBusMessage * msg = create_message("connect");
			if(!msg)
				return -1;

			dbus_message_append_args(msg, DBUS_TYPE_STRING, &uri, DBUS_TYPE_INVALID);

			rc = get_intval(call(msg));

			if(!rc && wait)
				return wait_for_ready(120);

			return rc;

#else
			return -1;

#endif
		}

		int wait_for_ready(int seconds)
		{
#if defined(WIN32)

			time_t end = time(0)+seconds;

			while(time(0) < end)
			{
				if(!is_connected())
					return ENOTCONN;

				if(is_ready())
					return 0;

				Sleep(250);
			}

			return ETIMEDOUT;

#elif defined(HAVE_DBUS)

			time_t end = time(0)+seconds;

			while(time(0) < end)
			{
				static const dbus_int32_t delay = 2;

				DBusMessage		* msg = create_message("waitForReady");
				int				  rc;

				if(!msg)
					return -1;

				dbus_message_append_args(msg, DBUS_TYPE_INT32, &delay, DBUS_TYPE_INVALID);

				rc = get_intval(call(msg));
				trace("waitForReady exits with rc=%d",rc);

				if(rc != ETIMEDOUT)
					return rc;
			}

			return ETIMEDOUT;

#else

			return -1;

#endif

		}

		bool is_ready(void)
		{
			return query_intval(HLLAPI_PACKET_IS_READY) != 0;
		}


		int disconnect(void)
		{
			return query_intval(HLLAPI_PACKET_DISCONNECT);
		}


		int wait(int seconds)
		{
#if defined(WIN32)

			time_t end = time(0)+seconds;

			while(time(0) < end)
			{
				if(!is_connected())
					return ENOTCONN;
				Sleep(500);
			}

			return 0;

#elif defined(HAVE_DBUS)

			time_t end = time(0)+seconds;

			while(time(0) < end)
			{
				if(!is_connected())
					return ENOTCONN;
				usleep(500);
			}

			return 0;

#else

			return -1;

#endif

		}

		int iterate(bool wait)
		{
#if defined(WIN32)
			return 0;
#elif defined(HAVE_DBUS)
			return 0;
#else
			return -1;
#endif
		}

		string * get_text_at(int row, int col, size_t sz)
		{
#if defined(WIN32)

			struct hllapi_packet_query_at query	= { HLLAPI_PACKET_GET_TEXT_AT, (unsigned short) row, (unsigned short) col, (unsigned short) sz };

			return query_string(&query,sizeof(query),sz);

#elif defined(HAVE_DBUS)

			dbus_int32_t r = (dbus_int32_t) row;
			dbus_int32_t c = (dbus_int32_t) col;
			dbus_int32_t l = (dbus_int32_t) sz;

			DBusMessage * msg = create_message("getTextAt");
			if(!msg)
				return NULL;

			trace("%s(%d,%d,%d)",__FUNCTION__,r,c,l);
			dbus_message_append_args(msg, DBUS_TYPE_INT32, &r, DBUS_TYPE_INT32, &c, DBUS_TYPE_INT32, &l, DBUS_TYPE_INVALID);

			return get_string(call(msg));

#else

			return NULL;

#endif

		}

		int set_text_at(int row, int col, const char *str)
		{
#if defined(WIN32)

			struct hllapi_packet_text_at 	* query;
			struct hllapi_packet_result	  	  response;
			DWORD							  cbSize		= sizeof(struct hllapi_packet_text_at)+strlen((const char *) str);

			query = (struct hllapi_packet_text_at *) malloc(cbSize);
			query->packet_id 	= HLLAPI_PACKET_SET_TEXT_AT;
			query->row			= row;
			query->col			= col;
			strcpy(query->text,(const char *) str);

			TransactNamedPipe(hPipe,(LPVOID) query, cbSize, &response, sizeof(response), &cbSize,NULL);

			free(query);

			return response.rc;

#elif defined(HAVE_DBUS)

			dbus_int32_t r = (dbus_int32_t) row;
			dbus_int32_t c = (dbus_int32_t) col;

			DBusMessage * msg = create_message("setTextAt");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &r, DBUS_TYPE_INT32, &c, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

			return -1;

#else

			return -1;

#endif

		}

		int cmp_text_at(int row, int col, const char *text)
		{
#if defined(WIN32)

			struct hllapi_packet_text_at 	* query;
			size_t							  cbSize		= sizeof(struct hllapi_packet_text_at)+strlen(text);

			query = (struct hllapi_packet_text_at *) malloc(cbSize);
			query->packet_id 	= HLLAPI_PACKET_CMP_TEXT_AT;
			query->row			= row;
			query->col			= col;
			strcpy(query->text,text);

			return query_intval((void *) query, cbSize, true);

#elif defined(HAVE_DBUS)

			dbus_int32_t r = (dbus_int32_t) row;
			dbus_int32_t c = (dbus_int32_t) col;

			DBusMessage * msg = create_message("cmpTextAt");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &r, DBUS_TYPE_INT32, &c, DBUS_TYPE_STRING, &text, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

#endif

			return 0;
		}

		int wait_for_text_at(int row, int col, const char *key, int timeout)
		{
			time_t end = time(0)+timeout;

			while(time(0) < end)
			{
				if(!is_connected())
					return ENOTCONN;

				if(!cmp_text_at(row,col,key))
					return 0;

#ifdef WIN32
				Sleep(500);
#else
				usleep(500);
#endif
			}

			return ETIMEDOUT;
		}

		string * get_text(int baddr, size_t len)
		{
#if defined(WIN32)
			struct hllapi_packet_query_offset query = { HLLAPI_PACKET_GET_TEXT_AT_OFFSET, (unsigned short) baddr, (unsigned short) len };
			return query_string(&query,sizeof(query),len);
#else
			dbus_int32_t b = (dbus_int32_t) baddr;
			dbus_int32_t l = (dbus_int32_t) len;

			DBusMessage * msg = create_message("getText");
			if(!msg)
				return NULL;

			trace("%s(%d,%d)",__FUNCTION__,b,l);
			dbus_message_append_args(msg, DBUS_TYPE_INT32, &b, DBUS_TYPE_INT32, &l, DBUS_TYPE_INVALID);

			return get_string(call(msg));
#endif
		}


		int set_cursor_position(int row, int col)
		{
#if defined(WIN32)

			struct hllapi_packet_cursor query = { HLLAPI_PACKET_SET_CURSOR_POSITION, (unsigned short) row, (unsigned short) col };

			return query_intval((void *) &query, sizeof(query));

#elif defined(HAVE_DBUS)

			dbus_int32_t r = (dbus_int32_t) row;
			dbus_int32_t c = (dbus_int32_t) col;

			DBusMessage * msg = create_message("setCursorAt");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &r, DBUS_TYPE_INT32, &c, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

#endif

			return -1;
		}

		int set_cursor_addr(int addr)
		{
#if defined(WIN32)

			struct hllapi_packet_addr query = { HLLAPI_PACKET_SET_CURSOR, (unsigned short) addr };

			return query_intval((void *) &query, sizeof(query));

#elif defined(HAVE_DBUS)

			dbus_int32_t k = (dbus_int32_t) addr;

			DBusMessage * msg = create_message("setCursorAddress");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &k, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

#endif

			return -1;
		}

		int get_cursor_addr(void)
		{
			return query_intval(HLLAPI_PACKET_GET_CURSOR);
		}


		int enter(void)
		{
			return query_intval(HLLAPI_PACKET_ENTER);
		}

		int pfkey(int key)
		{
#if defined(WIN32)

			struct hllapi_packet_keycode query = { HLLAPI_PACKET_PFKEY, (unsigned short) key };

			return query_intval((void *) &query, sizeof(query));

#elif defined(HAVE_DBUS)

			dbus_int32_t k = (dbus_int32_t) key;

			DBusMessage * msg = create_message("pfKey");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &k, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

			return -1;

#else

			return -1;

#endif

		}

		int pakey(int key)
		{
#if defined(WIN32)

			struct hllapi_packet_keycode query = { HLLAPI_PACKET_PAKEY, (unsigned short) key };

			return query_intval((void *) &query, sizeof(query));

#elif defined(HAVE_DBUS)

			dbus_int32_t k = (dbus_int32_t) key;

			DBusMessage * msg = create_message("paKey");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &k, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}
			return -1;

#else

			return -1;

#endif

		}

		int quit(void)
		{
			return query_intval(HLLAPI_PACKET_QUIT);
		}

		int set_toggle(LIB3270_TOGGLE ix, bool value)
		{
#if defined(WIN32)

			struct hllapi_packet_set query = { HLLAPI_PACKET_SET_TOGGLE, (unsigned short) ix, (unsigned short) value };

			return query_intval((void *) &query, sizeof(query));

#elif defined(HAVE_DBUS)

			dbus_int32_t i = (dbus_int32_t) ix;
			dbus_int32_t v = (dbus_int32_t) value;

			DBusMessage * msg = create_message("setToggle");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &i, DBUS_TYPE_INT32, &v, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

			return -1;

#else
			return -1;

#endif

		}

		int emulate_input(const char *str)
		{
#if defined(WIN32)

			size_t                                len           = strlen(str);
			struct hllapi_packet_emulate_input 	* query;
			size_t							      cbSize		= sizeof(struct hllapi_packet_emulate_input)+len;

			query = (struct hllapi_packet_emulate_input *) malloc(cbSize);
			query->packet_id 	= HLLAPI_PACKET_EMULATE_INPUT;
			query->len			= len;
			query->pasting		= 1;
			strcpy(query->text,str);

			return query_intval((void *) query, cbSize, true);

#elif defined(HAVE_DBUS)

			DBusMessage * msg = create_message("input");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}
			return -1;

#else

			return -1;

#endif

		}

		int get_field_start(int baddr)
		{
#if defined(WIN32)

			struct hllapi_packet_addr query = { HLLAPI_PACKET_FIELD_START, (unsigned short) baddr };

			return query_intval((void *) &query, sizeof(query));

#elif defined(HAVE_DBUS)

			dbus_int32_t k = (dbus_int32_t) baddr;

			DBusMessage * msg = create_message("getFieldStart");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &k, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

			return -1;

#else

			return -1;

#endif

		}

		int get_field_len(int baddr)
		{
#if defined(WIN32)

			struct hllapi_packet_addr query = { HLLAPI_PACKET_FIELD_LEN, (unsigned short) baddr };

			return query_intval((void *) &query, sizeof(query));

#elif defined(HAVE_DBUS)

			dbus_int32_t k = (dbus_int32_t) baddr;

			DBusMessage * msg = create_message("getFieldLength");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &k, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

			return -1;

#else

			return -1;

#endif
		}

		int get_next_unprotected(int baddr)
		{
#if defined(WIN32)

			struct hllapi_packet_addr query = { HLLAPI_PACKET_NEXT_UNPROTECTED, (unsigned short) baddr };

			return query_intval((void *) &query, sizeof(query));

#elif defined(HAVE_DBUS)

			dbus_int32_t k = (dbus_int32_t) baddr;

			DBusMessage * msg = create_message("getNextUnprotected");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_INT32, &k, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

			return -1;

#else

			return -1;

#endif

		}

		int set_clipboard(const char *text)
		{
#if defined(WIN32)

			return -1;

#elif defined(HAVE_DBUS)

			DBusMessage * msg = create_message("setClipboard");
			if(msg)
			{
				dbus_message_append_args(msg, DBUS_TYPE_STRING, &text, DBUS_TYPE_INVALID);
				return get_intval(call(msg));
			}

			return -1;

#else

			return -1;

#endif

		}

		string * get_clipboard(void)
		{
#if defined(WIN32)

			return NULL;

#elif defined(HAVE_DBUS)

			trace("%s",__FUNCTION__);
			return query_string("getClipboard");

#endif

			return NULL;
		}

 	};

	session	* session::create_remote(const char *session)
	{
		return new remote(session);
	}

 }

