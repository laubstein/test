#!/bin/bash

case $1 in

	revision)
		dbus-send --session  --print-reply --dest=br.com.bb.pw3270 /br/com/bb/pw3270 br.com.bb.pw3270.getRevision
		;;

	message)
		dbus-send --session  --print-reply --dest=br.com.bb.pw3270 /br/com/bb/pw3270 br.com.bb.pw3270.getMessageID
		;;

	connect)
		dbus-send --session  --print-reply --dest=br.com.bb.pw3270 /br/com/bb/pw3270 br.com.bb.pw3270.connect string:$2
		;;

	disconnect)
		dbus-send --session  --print-reply --dest=br.com.bb.pw3270 /br/com/bb/pw3270 br.com.bb.pw3270.disconnect
		;;

	quit)
		dbus-send --session  --print-reply --dest=br.com.bb.pw3270 /br/com/bb/pw3270 br.com.bb.pw3270.quit
		;;

	get)
		dbus-send --session  --print-reply --dest=br.com.bb.pw3270 /br/com/bb/pw3270 br.com.bb.pw3270.getScreenContents
		;;

	set)
		dbus-send --session  --print-reply --dest=br.com.bb.pw3270 /br/com/bb/pw3270 br.com.bb.pw3270.setTextAt int32:$2 int32:$3 string:$4
		;;

	enter)
		dbus-send --session  --print-reply --dest=br.com.bb.pw3270 /br/com/bb/pw3270 br.com.bb.pw3270.enter
		;;

	*)
		echo "Comando $1 desconhecido"
		;;

esac

