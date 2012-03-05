#
# "Software pw3270, desenvolvido com base nos códigos fontes do WC3270  e X3270
# (Paul Mattes Paul.Mattes@usa.net), de emulação de terminal 3270 para acesso a
# aplicativos mainframe. Registro no INPI sob o nome G3270.
#
# Copyright (C) <2008> <Banco do Brasil S.A.>
#
# Este programa é software livre. Você pode redistribuí-lo e/ou modificá-lo sob
# os termos da GPL v.2 - Licença Pública Geral  GNU,  conforme  publicado  pela
# Free Software Foundation.
#
# Este programa é distribuído na expectativa de  ser  útil,  mas  SEM  QUALQUER
# GARANTIA; sem mesmo a garantia implícita de COMERCIALIZAÇÃO ou  de  ADEQUAÇÃO
# A QUALQUER PROPÓSITO EM PARTICULAR. Consulte a Licença Pública Geral GNU para
# obter mais detalhes.
#
# Você deve ter recebido uma cópia da Licença Pública Geral GNU junto com este
# programa;  se  não, escreva para a Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA, 02111-1307, USA
#
# Contatos:
#
# perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
# erico.mendonca@gmail.com	(Erico Mascarenhas de Mendonça)
# licinio@bb.com.br		(Licínio Luis Branco)
# kraucer@bb.com.br		(Kraucer Fernandes Mazuco)
# macmiranda@bb.com.br		(Marco Aurélio Caldas Miranda)
#

#---[ Configuration values ]---------------------------------------------------

PACKAGE_NAME=pw3270
PACKAGE_VERSION=5.0
PACKAGE_TARNAME=pw3270
EXEEXT=

LIB3270_CFLAGS ?= `pkg-config --cflags lib3270`
LIB3270_LIBS ?= `pkg-config --libs lib3270`

DEBUG_CFLAGS=-DDEBUG=1 -g
DEPENDS ?= *.h

#---[ Tools ]------------------------------------------------------------------

MKDIR=/bin/mkdir -p
CC=gcc
LD=gcc

#---[       ]------------------------------------------------------------------

OBJEXT=.o

#---[ Paths ]------------------------------------------------------------------

ROOTDIR ?= "."
OBJDIR  ?= $(ROOTDIR)/.obj
BINDIR  ?= $(ROOTDIR)/.bin

#---[ Rules ]------------------------------------------------------------------

$(OBJDIR)/Debug/%.o: %.c $(DEPENDS)
	@echo $< ...
	@$(MKDIR) `dirname $@`
	@$(CC) $(DEBUG_CFLAGS) $(CFLAGS) $(LIB3270_CFLAGS) -o $@ -c $<


#---[ Targets ]----------------------------------------------------------------

clean-common:
	@rm -fr $(OBJDIR)
	@rm -fr $(BINDIR)
	@rm -f *.log
	@find . -name "*~" -exec rm -f {} \;




