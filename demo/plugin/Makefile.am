## Process this file with automake to produce Makefile.in -*-Makefile-*-
AUTOMAKE_OPTIONS = foreign

noinst_PROGRAMS = demo_plugin.so

demo_plugin_so_SOURCES = demo_plugin.c
demo_plugin_so_CFLAGS = -rdynamic -fPIC -I$(top_srcdir)/src
demo_plugin_so_LDFLAGS = -shared
