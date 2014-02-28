PLUGIN=demo_plugin

.PHONY: check check-am clean-generic cscopelist-am \
	ctags-am distclean-generic distclean-local distdir \
	dvi dvi-am html html-am info info-am install install-am \
	install-data install-data-am install-dvi install-dvi-am \
	install-exec install-exec-am install-html install-html-am \
	install-info install-info-am install-man install-pdf \
	install-pdf-am install-ps install-ps-am install-strip \
	installcheck installcheck-am installdirs maintainer-clean \
	maintainer-clean-generic mostlyclean mostlyclean-generic pdf \
	pdf-am ps ps-am tags-am uninstall uninstall-am


all: $(PLUGIN).so

GNUPLOTSRC = ../..

CFLAGS = -DHAVE_CONFIG_H -I$(GNUPLOTSRC) -I$(GNUPLOTSRC)/src -rdynamic -fPIC

%.so: %.o
	$(CC) -shared $^ -o $@ 

distclean:	clean
clean:
	rm -f *.o *.so
