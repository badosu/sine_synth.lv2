BUNDLE = sine_synth.lv2
INSTALL_DIR = /usr/local/lib/lv2
CFLAGS = -shared -fPIC -DPIC

$(BUNDLE): manifest.ttl sine_synth.ttl sine_synth.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp $^ $(BUNDLE)

all: $(BUNDLE)

sine_synth.so: sine_synth.c
	gcc $< -o $@ $(CFLAGS)

clean:
	rm -rf $(BUNDLE) *.so *.moc.cpp

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

run:
	jalv http://lv2plug.in/plugins/sine_synth
