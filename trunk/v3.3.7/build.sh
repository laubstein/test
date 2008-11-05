#!/bin/sh

svn update

NAME=SisBB
ICON=sisbb/sisbb98.ico
LOGO=sisbb/sisbb98.jpg

BuildWin() {
	#
	# Build windows version
	#
	echo -e "\e]2;Building Windows Installer\a"

	make clean

	ln -sf /usr/i386-mingw32/GTK-Runtime .
	./win32.sh --gtkroot="GTK-Runtime" --locale="locale" --name=$NAME --icon=$ICON --logo=$LOGO
	if [ "$?" != "0" ]; then
		exit -1
	fi

	make Release
	if [ "$?" != "0" ]; then
		exit -1
	fi

	mkdir -p locale/pt_BR/LC_MESSAGES
	if [ "$?" != "0" ]; then
		exit -1
	fi

	msgfmt -c -v -o locale/pt_BR/LC_MESSAGES/$NAME.mo po/pt_BR.po
	if [ "$?" != "0" ]; then
		exit -1
	fi

	wine  ~/.wine/drive_c/Arquivos\ de\ programas/NSIS/makensis.exe g3270.nsi
	if [ "$?" != "0" ]; then
		echo "*** ERRO AO GERAR O PACOTE DE INSTALACAO"
		exit -1
	fi

	scp $NAME-*.exe perry@os2team:public_html/g3270
	mkdir -p ~/win32
	mv $NAME-*.exe ~/win32/

}

BuildRPM() {
	#
	# Build Linux version
	#
	echo -e "\e]2;Building Linux RPM\a"

	autoconf
	./configure

	RPMDIR=`rpm --eval="%{u2p:%{_rpmdir}}"`
	RPMARCH=`rpm --eval="%{u2p:%{_target_cpu}}"`
	VENDOR=`rpm --eval="%{u2p:%{_vendor}}"`
	RELEASE=`grep Release g3270.spec | sed 's/ //g' |cut -d: -f2 |cut -d. -f1`

	make clean
	make tgz
	if [ "$?" != "0" ]; then
		exit -1
	fi

	cp *.tar.gz `rpm --eval="%{u2p:%{_sourcedir}}"`
	if [ "$?" != "0" ]; then
		exit -1
	fi

	rpmbuild -ba g3270.spec
	if [ "$?" != "0" ]; then
		exit -1
	fi

	if [ "$1" != "--debug" ]; then
		echo "Enviando arquivo source para o servidor..."
		scp `rpm --eval="%{u2p:%{_srcrpmdir}}"`/g3270*.src.rpm $USER@suportelinux.df.bb.com.br:src/suse/g3270-latest.src.rpm
		if [ "$?" != "0" ]; then
			echo "Erro ao copiar o pacote fonte"
			exit -1
		fi
	fi

	echo "Enviando arquivo binario para o servidor..."
	scp $RPMDIR/$RPMARCH/g3270*.rpm $USER@os2team.df.intrabb.bb.com.br:public_html/g3270
	if [ "$?" != "0" ]; then
		echo "Erro ao copiar o pacote binario"
		exit -1
	fi

	TARGET_FOLDER=$USER@os2team.df.intrabb.bb.com.br:/home/matriz/pacotes/$VENDOR/bb/$RPMARCH

	if [ "$VENDOR" == "suse" ]; then
		TARGET_FOLDER="$USER@storage:/dados/suse/$(rpm --eval="%{suse_version}" | cut -b1-2)/bb/$RPMARCH"
	fi

	scp $RPMDIR/$RPMARCH/g3270*.rpm $TARGET_FOLDER
	if [ "$?" != "0" ]; then
		echo "Erro ao copiar o pacote binario"
		exit -1
	fi

	mv $RPMDIR/$RPMARCH/g3270*.rpm $TMPDIR
	mv `rpm --eval="%{u2p:%{_srcrpmdir}}"`/g3270*.src.rpm $TMPDIR

	make clean

}

BuildLocal() {
	#
	# Build Local
	#
	echo -e "\e]2;Building Local Debug\a"

	rm -fr /home/perry/bin/g3270
	./configure --enable-plugins --prefix=/home/perry/bin/g3270

	make clean
	make install
	if [ "$?" != "0" ]; then
		exit -1
	fi

	cp  /home/perry/bin/g3270/share/locale/pt_BR/LC_MESSAGES/g3270.mo /usr/share/locale/pt_BR/LC_MESSAGES/g3270.mo
	if [ "$?" != "0" ]; then
		exit -1
	fi

	make bin/Release/plugins/rx3270.so
	mkdir -p /home/perry/bin/g3270/lib/g3270/plugins
	cp bin/Release/plugins/*.so /home/perry/bin/g3270/lib/g3270/plugins
	cp ui/rexx.xml /home/perry/bin/g3270/share/g3270/ui
	cp ui/debug.xml /home/perry/bin/g3270/share/g3270/ui

	start_script=~/bin/g3270.sh

	echo #!/bin/bash > $start_script 
	echo cd /home/perry/bin/g3270/bin >> $start_script
	echo LD_LIBRARY_PATH=../lib ./g3270 >> $start_script 

	chmod +x $start_script

	make clean

}

if [ -z "$1" ]; then

	BuildWin
	BuildRPM
	BuildLocal

else

	until [ -z "$1" ]
	do
		if [ "$1" == "win" ]; then
			BuildWin
		elif [ "$1" == "rpm" ]; then
			BuildRPM
		elif [ "$1" == "local" ]; then
			BuildLocal
		fi

		shift
	done
fi

echo "G3270 Build Ok!"


