/*
 * "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
 * (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
 * aplicativos mainframe. Registro no INPI sob o nome G3270. Registro no INPI sob o nome G3270.
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
 * Este programa está nomeado como globals.h e possui - linhas de código.
 *
 * Contatos:
 *
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 *
 */

 #include "lib3270jni.h"

 #include <lib3270.h>
 #include <lib3270/session.h>
 #include <lib3270/log.h>

/*--[ Defines ]--------------------------------------------------------------------------------------*/

/*
 #define session_request(env, obj)	H3270	* session 	= lib3270_get_default_session_handle(); \
									session->widget		= &jni_data;

 #define session_release()			session->widget		= 0;
*/

/*
 #define 	PW3270_JNI_BEGIN	pw3270_env = env; pw3270_obj = obj;
 #define 	PW3270_JNI_END		pw3270_env = NULL; pw3270_obj = NULL;
*/

 #define 	PW3270_JNI_BEGIN	pw3270_env = env; pw3270_obj = obj; \
								__android_log_print(ANDROID_LOG_VERBOSE, PACKAGE_NAME, "%s.begin env=%p obj=%p",__FUNCTION__,env,obj);

 #define 	PW3270_JNI_END		__android_log_print(ANDROID_LOG_VERBOSE, PACKAGE_NAME, "%s.end env=%p obj=%p",__FUNCTION__,pw3270_env,pw3270_obj); \
								pw3270_env = NULL; pw3270_obj = NULL;

 #define	PW3270_JNI_ENV		pw3270_env
 #define	PW3270_JNI_OBJ		pw3270_obj

 #define	PW3270_SESSION		lib3270_get_default_session_handle()

 #define pw3270_jni_call_void(name, sig, ...) 	pw3270_env->CallVoidMethod(pw3270_obj,lib3270_getmethodID(name,sig), __VA_ARGS__)
 #define pw3270_jni_call_int(name, sig, ...) 	pw3270_env->CallIntMethod(pw3270_obj,lib3270_getmethodID(name,sig), __VA_ARGS__)
 #define pw3270_jni_new_string(str)				pw3270_env->NewStringUTF(str)
 #define pw3270_jni_new_byte_array(len)			pw3270_env->NewByteArray(len)

/*--[ Globals ]--------------------------------------------------------------------------------------*/

 extern JNIEnv	 * pw3270_env;
 extern jobject	   pw3270_obj;


 jmethodID lib3270_getmethodID(const char *name, const char *sig);


