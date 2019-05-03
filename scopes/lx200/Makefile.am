## Process this file with automake to produce Makefile.in

dist_indicor_data_DATA = lx200simple.xml

AM_CPPFLAGS = -I$(indicor_incdir)
AM_CXXFLAGS = -Wall

lib_LTLIBRARIES =lx200.la

lx200_la_SOURCES = lx200.cpp lx200.h \
	lx200cmd.cpp lx200cmd.h \
	basiccmd.cpp basiccmd.h

lx200_la_LIBADD  =  $(indicor_libdir)/libindicor.la
lx200_la_LDFLAGS = -module -no-undefined -version-info 0:0:0


