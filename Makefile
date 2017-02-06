BUNDLE = sine_synth.lv2
CFLAGS = -shared -fPIC -DPIC
LV2 = /usr/lib/lv2
LOCAL_LV2 = /usr/local/lib/lv2

LIB_TTLS = $(shell find $(LV2)/schemas.lv2 $(LV2)/lv2core.lv2 $(LV2)/atom.lv2 $(LV2)/midi.lv2 $(LV2)/port-props.lv2 $(LV2)/ui.lv2 $(LV2)/units.lv2 $(LV2)/urid.lv2 $(LV2)/uri-map.lv2 -name '*.ttl')
BUNDLE_TTLS = sine_synth.ttl manifest.ttl

$(BUNDLE): manifest.ttl sine_synth.ttl sine_synth.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp $^ $(BUNDLE)

all: $(BUNDLE)

debug: CFLAGS += -DDEBUG -g
debug: sine_synth.so

sine_synth.so: sine_synth.c
	gcc $< -o $@ $(CFLAGS)

clean:
	rm -rf $(BUNDLE) *.so *.moc.cpp

install: $(BUNDLE)
	mkdir -p $(LOCAL_LV2)
	rm -rf $(LOCAL_LV2)/$(BUNDLE)
	cp -R $(BUNDLE) $(LOCAL_LV2)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

validate:
	sord_validate $(LIB_TTLS) $(BUNDLE_TTLS)

run:
	jalv http://bado.so/plugins/sine_synth
