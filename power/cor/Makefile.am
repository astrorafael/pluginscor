## Process this file with automake to produce Makefile.in

dist_indicor_data_DATA = corpower.xml

AM_CPPFLAGS = -I$(indicor_incdir)
AM_CXXFLAGS = -Wall

lib_LTLIBRARIES = corpower.la

corpower_la_SOURCES = corpower.h corpower.cpp powercmd.h powercmd.cpp 
corpower_la_LIBADD  = $(indicor_libdir)/libindicor.la
corpower_la_LDFLAGS = -module -no-undefined -version-info 0:0:0


