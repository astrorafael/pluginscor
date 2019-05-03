## Process this file with automake to produce Makefile.in

dist_indicor_data_DATA = cor.xml

AM_CPPFLAGS = -I$(indicor_incdir)
AM_CXXFLAGS = -Wall

lib_LTLIBRARIES = cor.la

cor_la_SOURCES = cor.cpp cor.h state.cpp state.h
cor_la_LIBADD  = $(indicor_libdir)/libindicor.la
cor_la_LDFLAGS = -module -no-undefined -version-info 0:0:0


