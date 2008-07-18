#!/bin/bash

IMAGE=image.w32
LOCALE=$IMAGE/locale
PACKET=$PWD/g3270_`date +%G%m%d%H%M`.zip

rm -fr bin/Release
make Release

if [ "$?" != "0" ]; then
	exit -1
fi

rm -fr $IMAGE
mkdir $IMAGE
if [ "$?" != "0" ]; then
	exit -1
fi

mv bin/Release/g3270.exe $IMAGE
if [ "$?" != "0" ]; then
	exit -1
fi

mv bin/Release/*.dll $IMAGE
if [ "$?" != "0" ]; then
	exit -1
fi

cp image/*.jpg $IMAGE
if [ "$?" != "0" ]; then
	exit -1
fi

cp src/g3270/ui.xml $IMAGE
if [ "$?" != "0" ]; then
	exit -1
fi

mkdir -p $LOCALE/pt_BR/LC_MESSAGES
if [ "$?" != "0" ]; then
	exit -1
fi

msgfmt -c -v -o $LOCALE/pt_BR/LC_MESSAGES/g3270.mo po/pt_BR.po
if [ "$?" != "0" ]; then
	exit -1
fi

mv g3270_*.zip /tmp
cd $IMAGE
zip -9 -m -r $PACKET *
if [ "$?" != "0" ]; then
	exit -1
fi

echo $PACKET gerado!
