PLUGIN=demo_plugin

all: $(PLUGIN).so

GNUPLOTSRC = ../..

CFLAGS = -DHAVE_CONFIG_H -I$(GNUPLOTSRC) -I$(GNUPLOTSRC)/src -rdynamic -fPIC

%.so: %.o
	$(CC) -shared $^ -o $@ 

clean:
	rm -f *.o *.so
