## Process this file with automake to produce Makefile.in

dist_indicor_data_DATA = trackscope.xml

AM_CPPFLAGS = -I$(indicor_incdir)
AM_CXXFLAGS = -Wall

lib_LTLIBRARIES = trackscope.la

trackscope_la_SOURCES = trackscope.cpp trackscope.h 
trackscope_la_LIBADD  = $(indicor_libdir)/libindicor.la 
trackscope_la_LDFLAGS = -module -no-undefined -version-info 0:0:0

