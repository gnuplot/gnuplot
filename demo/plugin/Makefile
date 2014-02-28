PLUGIN=demo_plugin

.PHONY: check html distcheck distdir

all: $(PLUGIN).so

GNUPLOTSRC = ../..

CFLAGS = -DHAVE_CONFIG_H -I$(GNUPLOTSRC) -I$(GNUPLOTSRC)/src -rdynamic -fPIC

%.so: %.o
	$(CC) -shared $^ -o $@ 

distclean:	clean
clean:
	rm -f *.o *.so
