## Process this file with automake to produce Makefile.in

dist_indicor_data_DATA = audine1.xml audine2.xml

AM_CPPFLAGS = -I$(indicor_incdir)
AM_CXXFLAGS = -Wall

lib_LTLIBRARIES = audine.la

audine_la_SOURCES = fitshead.cpp fitshead.h \
	audine.cpp audine.h \
	state.cpp state.h \
	chip.cpp chip.h \
	shutter.cpp shutter.h \
	imagseq.cpp imagseq.h \
	storage.cpp storage.h \
	perscount.h

audine_la_LIBADD  =  $(indicor_libdir)/libindicor.la
audine_la_LDFLAGS = -module -no-undefined -version-info 0:0:0

