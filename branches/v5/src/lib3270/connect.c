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
 * Este programa está nomeado como connect.c e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

#if defined(_WIN32)
	#include <winsock2.h>
	#include <windows.h>
#endif

#include "globals.h"
#include <errno.h>

#if defined(_WIN32)
	#include <ws2tcpip.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <fcntl.h>
#endif

#ifdef HAVE_ICONV
	#include <iconv.h>
#endif // HAVE_ICONV

#if defined(_WIN32) /*[*/
	#define SOCK_CLOSE(s)	closesocket(s->sock); s->sock = -1;
#else /*][*/
	#define SOCK_CLOSE(s)	close(s->sock); s->sock = -1;
#endif /*]*/

#include <stdlib.h>
#include "statusc.h"
#include "hostc.h"
#include "trace_dsc.h"
#include "utilc.h"
#include "telnetc.h"
#include "screen.h"
#include <lib3270/internals.h>

/*---[ Implement ]-------------------------------------------------------------------------------*/


static void net_connected(H3270 *hSession)
{
	int 		err;
	socklen_t	len		= sizeof(err);

	trace("%s",__FUNCTION__);
	RemoveSource(hSession->ns_write_id);
	hSession->ns_write_id = NULL;

	if(getsockopt(hSession->sock, SOL_SOCKET, SO_ERROR, (char *) &err, &len) < 0)
	{
		lib3270_disconnect(hSession);
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Network error" ),
								_( "Unable to get connection state." ),
#ifdef _WIN32
								"%s", lib3270_win32_strerror(WSAGetLastError())
#else
								_( "%s" ), strerror(errno)
#endif // _WIN32
							);
		return;
	}
	else if(err)
	{
		lib3270_disconnect(hSession);
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Network error" ),
								_( "Unable to connect to host." ),
#ifdef _WIN32
								_( "%s"), lib3270_win32_strerror(err)
#else
								_( "%s" ), strerror(err)
#endif // _WIN32
							);
		trace("%s",__FUNCTION__);
		return;
	}


#ifdef _WIN32
	hSession->ns_exception_id	= AddExcept(hSession->sockEvent, hSession, net_exception);
	hSession->ns_read_id		= AddInput(hSession->sockEvent, hSession, net_input);
#else
	hSession->ns_exception_id	= AddExcept(hSession->sock, hSession, net_exception);
	hSession->ns_read_id		= AddInput(hSession->sock, hSession, net_input);
#endif // WIN32
	hSession->excepting	= 1;
	hSession->reading 	= 1;

#if defined(HAVE_LIBSSL)
	if(hSession->ssl_con && hSession->secure == LIB3270_SSL_UNDEFINED)
	{
		if(ssl_negotiate(hSession))
			return;
	}
#endif

	lib3270_setup_session(hSession);


	lib3270_set_connected(hSession);

}

#if defined(_WIN32) /*[*/
 static void sockstart(H3270 *session)
 {
	static int initted = 0;
	WORD wVersionRequested;
	WSADATA wsaData;

	if (initted)
		return;

	initted = 1;

	wVersionRequested = MAKEWORD(2, 2);

	if (WSAStartup(wVersionRequested, &wsaData) != 0)
	{
		lib3270_popup_dialog(	session,
								LIB3270_NOTIFY_CRITICAL,
								N_( "Network startup error" ),
								N_( "WSAStartup failed" ),
								"%s", lib3270_win32_strerror(GetLastError()) );

		_exit(1);
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		lib3270_popup_dialog(	session,
								LIB3270_NOTIFY_CRITICAL,
								N_( "Network startup error" ),
								N_( "Bad winsock version" ),
								N_( "Can't use winsock version %d.%d" ), LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
		_exit(1);
	}
 }
#endif /*]*/

 LIB3270_EXPORT int lib3270_connect_host(H3270 *hSession, const char *hostname, const char *srvc, LIB3270_CONNECT_OPTION opt)
 {
 	int					  s;
	int					  optval;
	struct addrinfo		  hints;
	struct addrinfo 	* result		= NULL;
	struct addrinfo 	* rp			= NULL;

	if(!hostname)
		return EINVAL;

	if(!srvc)
		srvc = "telnet";

	CHECK_SESSION_HANDLE(hSession);

	lib3270_main_iterate(hSession,0);

	if(hSession->auto_reconnect_inprogress)
		return EAGAIN;

	if(hSession->sock > 0)
		return EBUSY;

#if defined(_WIN32)
	sockstart(hSession);
#endif

	set_ssl_state(hSession,LIB3270_SSL_UNSECURE);

	hSession->ever_3270	= False;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family 	= AF_UNSPEC;	/* Allow IPv4 or IPv6 */
	hints.ai_socktype	= SOCK_STREAM;	/* Stream socket */
	hints.ai_flags		= AI_PASSIVE;	/* For wildcard IP address */
	hints.ai_protocol	= 0;			/* Any protocol */
	hints.ai_canonname	= NULL;
	hints.ai_addr		= NULL;
	hints.ai_next		= NULL;

	if(*hostname == '$')
	{
		const char *name = getenv(hostname+1);
		if(!name)
		{
			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									_( "Unable to find selected hostname." ),
									_( "Can't determine value for environment variable \"%s\" " ),
									hostname);
			lib3270_set_disconnected(hSession);
			return ENOENT;
		}
		hostname = name;
	}

	status_changed(hSession,LIB3270_STATUS_RESOLVING);

	s = getaddrinfo(hostname, srvc, &hints, &result);

	if(s != 0)
	{
		char buffer[4096];

		snprintf(buffer,4095,_( "Can't connect to %s:%s"), hostname, srvc);

#if defined(WIN32) && defined(HAVE_ICONV)
	{
		char 		  tmpbuffer[4096];
		const char 	* msg 		= gai_strerror(s);
		size_t		  in 		= strlen(msg);
		size_t		  out 		= 4096;
		char		* ptr		= tmpbuffer;

		iconv_t hConv = iconv_open(lib3270_win32_local_charset(),"UTF-8");

		trace("Antes: [%s]",msg);
		if(iconv(hConv,&msg,&in,&ptr,&out) != ((size_t) -1))
			msg = tmpbuffer;
		trace("Depois: [%s]",msg);

		iconv_close(hConv);

		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								buffer,
								"%s",
								msg);
	}

#else
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Connection error" ),
								buffer,
								"%s",
								gai_strerror(s));
#endif // WIN32


		lib3270_set_disconnected(hSession);
		return ENOENT;
	}


#if !defined(_WIN32)
	/* don't share the socket with our children */
	(void) fcntl(hSession->sock, F_SETFD, 1);
#endif

	hSession->ssl_host = 0;

#if defined(HAVE_LIBSSL)
	if(opt&LIB3270_CONNECT_OPTION_SSL)
	{
		hSession->ssl_host = 1;
		ssl_init(hSession);
	}
#endif

	/* connect */
	status_connecting(hSession,1);

	for(rp = result; hSession->sock < 0 && rp != NULL; rp = rp->ai_next)
	{
		hSession->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(hSession->sock < 0)
			continue;

		trace("sock=%d",hSession->sock);

#ifdef WIN32

		if(hSession->sockEvent == NULL)
		{
			char ename[256];

			snprintf(ename, 255, "%s-%d", PACKAGE_NAME, getpid());

			hSession->sockEvent = CreateEvent(NULL, TRUE, FALSE, ename);
			if(hSession->sockEvent == NULL)
			{
				lib3270_popup_dialog(	hSession,
										LIB3270_NOTIFY_CRITICAL,
										N_( "Network startup error" ),
										N_( "Cannot create socket event" ),
										"%s", lib3270_win32_strerror(GetLastError()) );
				_exit(1);
			}
		}

		if (WSAEventSelect(hSession->sock, hSession->sockEvent, FD_READ | FD_CONNECT | FD_CLOSE) != 0)
		{
			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_CRITICAL,
									N_( "Network startup error" ),
									N_( "WSAEventSelect failed" ),
									"%s", lib3270_win32_strerror(GetLastError()) );
			_exit(1);
		}

		WSASetLastError(0);
		u_long iMode=1;
		trace("sock=%d",hSession->sock);

		if(ioctlsocket(hSession->sock,FIONBIO,&iMode))
		{
			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									_( "ioctlsocket(FIONBIO) failed." ),
									"%s", lib3270_win32_strerror(WSAGetLastError()));
			SOCK_CLOSE(hSession);
		}
		else if(connect(hSession->sock, rp->ai_addr, rp->ai_addrlen))
		{
			int err = WSAGetLastError();
			if(err != WSAEWOULDBLOCK)
			{
				char buffer[4096];
				snprintf(buffer,4095,_( "Can't connect to %s:%s"), hostname, srvc);

				lib3270_popup_dialog(	hSession,
										LIB3270_NOTIFY_ERROR,
										_( "Connection error" ),
										buffer,
										"%s", lib3270_win32_strerror(err));
				SOCK_CLOSE(hSession);
			}
		}

		optval = 1;
		if (setsockopt(hSession->sock, SOL_SOCKET, SO_OOBINLINE, (char *)&optval,sizeof(optval)) < 0)
		{
			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									_( "setsockopt(SO_OOBINLINE) has failed" ),
									"%s",
									strerror(errno));
			SOCK_CLOSE(hSession);
		}

		optval = lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_ALIVE) ? 1 : 0;
		if (setsockopt(hSession->sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval)) < 0)
		{
			char buffer[4096];
			snprintf(buffer,4095,N_( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));

			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									buffer,
									"%s",
									strerror(errno));
			SOCK_CLOSE(hSession);
		}
		else
		{
			trace_dsn(hSession,"Network keep-alive is %s\n",optval ? "enabled" : "disabled" );
		}

#else
		fcntl(hSession->sock, F_SETFL,fcntl(hSession->sock,F_GETFL,0)|O_NONBLOCK);

		errno = 0;
		if(connect(hSession->sock, rp->ai_addr, rp->ai_addrlen))
		{
			if( errno != EINPROGRESS )
			{
				char buffer[4096];
				snprintf(buffer,4095,_( "Can't connect to %s:%s"), hostname, srvc);

				lib3270_popup_dialog(	hSession,
										LIB3270_NOTIFY_ERROR,
										_( "Connection error" ),
										buffer,
										"%s",
										strerror(errno));
				SOCK_CLOSE(hSession);
			}
		}

		optval = 1;
		if (setsockopt(hSession->sock, SOL_SOCKET, SO_OOBINLINE, (char *)&optval,sizeof(optval)) < 0)
		{
			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									_( "setsockopt(SO_OOBINLINE) has failed" ),
									"%s",
									strerror(errno));
			SOCK_CLOSE(hSession);
		}

		optval = lib3270_get_toggle(hSession,LIB3270_TOGGLE_KEEP_ALIVE) ? 1 : 0;
		if (setsockopt(hSession->sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, sizeof(optval)) < 0)
		{
			char buffer[4096];
			snprintf(buffer,4095,N_( "Can't %s network keep-alive" ), optval ? _( "enable" ) : _( "disable" ));

			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									buffer,
									"%s",
									strerror(errno));
			SOCK_CLOSE(hSession);
		}
		else
		{
			trace_dsn(hSession,"Network keep-alive is %s\n",optval ? "enabled" : "disabled" );
		}

#endif // WIN32
	}

	freeaddrinfo(result);

	// set options for inline out-of-band data and keepalives

	/*
#if defined(OMTU)
	else if (setsockopt(hSession->sock, SOL_SOCKET, SO_SNDBUF, (char *)&mtu,sizeof(mtu)) < 0)
	{
		popup_a_sockerr(hSession, N_( "setsockopt(%s)" ), "SO_SNDBUF");
		SOCK_CLOSE(hSession);
	}
#endif

	*/

	if(hSession->sock < 0)
	{
		lib3270_set_disconnected(hSession);
		return ENOTCONN;
	}

	// Connecting, set callbacks, wait for connection
	hSession->cstate = LIB3270_PENDING;
	lib3270_st_changed(hSession, LIB3270_STATE_HALF_CONNECT, True);

#ifdef _WIN32
	trace("Sockevent=%08lx callback=%p",hSession->sockEvent,net_connected);
	hSession->ns_write_id = AddOutput(hSession->sockEvent, hSession, net_connected);
#else
	hSession->ns_write_id = AddOutput(hSession->sock, hSession, net_connected);
#endif // WIN32

	trace("%s: Connection in progress",__FUNCTION__);

	if(opt&LIB3270_CONNECT_OPTION_WAIT)
	{
		time_t end = time(0)+120;

		while(time(0) < end)
		{
			lib3270_main_iterate(hSession,1);

			switch(hSession->cstate)
			{
			case LIB3270_PENDING:
			case LIB3270_CONNECTED_INITIAL:
			case LIB3270_CONNECTED_ANSI:
			case LIB3270_CONNECTED_3270:
			case LIB3270_CONNECTED_INITIAL_E:
			case LIB3270_CONNECTED_NVT:
			case LIB3270_CONNECTED_SSCP:
				break;

			case LIB3270_NOT_CONNECTED:
				return ENOTCONN;

			case LIB3270_CONNECTED_TN3270E:
				return 0;

			default:
				lib3270_write_log(hSession,"connect", "%s: State changed to unexpected state %d",__FUNCTION__,hSession->cstate);
				return -1;
			}

		}

		lib3270_disconnect(hSession);
		return ETIMEDOUT;
	}

	return 0;

 }

int non_blocking(H3270 *hSession, Boolean on)
{
#ifdef WIN32
		WSASetLastError(0);
		u_long iMode= on ? 1 : 0;

		if(ioctlsocket(hSession->sock,FIONBIO,&iMode))
		{
			lib3270_popup_dialog(	hSession,
									LIB3270_NOTIFY_ERROR,
									_( "Connection error" ),
									_( "ioctlsocket(FIONBIO) failed." ),
									"%s", lib3270_win32_strerror(GetLastError()));
		}
#else

	int f;

	if ((f = fcntl(hSession->sock, F_GETFL, 0)) == -1)
	{
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Socket error" ),
								_( "fcntl() error when getting socket state." ),
								_( "%s" ), strerror(errno)
							);

		return -1;
	}

	if (on)
		f |= O_NDELAY;
	else
		f &= ~O_NDELAY;

	if (fcntl(hSession->sock, F_SETFL, f) < 0)
	{
		lib3270_popup_dialog(	hSession,
								LIB3270_NOTIFY_ERROR,
								_( "Socket error" ),
								on ? _( "Can't set socket to blocking mode." ) : _( "Can't set socket to non blocking mode" ),
								_( "%s" ), strerror(errno)
							);
		return -1;
	}

#endif

	trace("Socket %d is %s",hSession->sock, on ? "non-blocking" : "blocking");

	return 0;
}

