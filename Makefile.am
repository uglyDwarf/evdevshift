
bin_PROGRAMS = evdevshift

BUILT_SOURCES = config_bison.h
evdevshift_SOURCES = ops.c ops.h parser.c parser.h ev_process.c ev_process.h ctrl_defs.h \
                     config_lexer.l config_bison.y evdevshift.c

AM_YFLAGS = -r all -d -p eds
AM_LFLAGS = -P eds

dist_doc_DATA = README

EXTRA_DIST = Makefile_standalone scafold2.py test1.conf presets/Saiteck_AV8R-01.conf
dist_pkgdata_DATA = LICENSE presets/Saiteck_AV8R-01.conf

