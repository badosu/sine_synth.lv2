user = $(shell whoami)
ifeq ($(user),root)
INSTALL_DIR = /usr/lib/lv2
else
INSTALL_DIR = ~/.lv2
endif

BUNDLE = sine_synth.lv2
CFLAGS = -shared -fPIC -DPIC
LV2 = /usr/lib/lv2
BUNDLE_TTLS = sine_synth.ttl manifest.ttl
LV2_TTLS = $(shell find third_party/lv2 -name '*.ttl')

$(BUNDLE): $(BUNDLE_TTLS) sine_synth.so sine_synth_gui.so
	rm -rf $(BUNDLE)
	mkdir $(BUNDLE)
	cp $^ $(BUNDLE)

all: $(BUNDLE)

debug: CFLAGS += -DDEBUG -g -ggdb
debug: all

sine_synth.so: sine_synth.c
	gcc $< -o $@ -O2 $(CFLAGS)

clean:
	rm -rf $(BUNDLE) *.so

install: $(BUNDLE)
	mkdir -p $(INSTALL_DIR)
	rm -rf $(INSTALL_DIR)/$(BUNDLE)
	cp -R $(BUNDLE) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(BUNDLE)

validate:
	sord_validate $(LV2_TTLS) $(BUNDLE_TTLS)

run:
	jalv.gtk http://bado.so/plugins/sine_synth

RUTABAGA = ./third_party/rutabaga
RUTABAGA_LIBS = gl freetype2 x11 x11-xcb xkbcommon-x11 xcb-xkb xrender xcb-icccm freetype2
RUTABAGA_S_LIBS = $(RUTABAGA)/build/src/librutabaga.a $(RUTABAGA)/build/styles/librtb_style_default.a $(RUTABAGA)/build/third-party/libuv.a
GUIFLAGS = -I$(RUTABAGA)/third-party -I$(RUTABAGA)/include -Wl,--no-undefined -lpthread -lm -std=gnu99 -fms-extensions -Wall -Werror -Wextra -Wcast-align -Wno-missing-field-initializers -Wno-unused-parameter -ffunction-sections -fdata-sections

submodules:
	git submodule init
	git submodule update
	cd $(RUTABAGA) && git submodule init && git submodule update

submodule_update:
	git submodule update
	cd $(RUTABAGA) && git submodule update

submodule_pull:
	git submodule foreach "git pull"

submodule_check:
	@-test -d .git -a .gitmodules && \
	  git submodule status \
	  | grep -q "^-" \
	  && $(MAKE) submodules || true
	@-test -d .git -a .gitmodules && \
	  git submodule status \
	  | grep -q "^+" \
		&& $(MAKE) submodule_update || true

rutabaga: submodule_check
	cd $(RUTABAGA) && ./waf configure && ./waf

sine_synth_gui.so: sine_synth_gui.c
	gcc $< -o $@ $(CFLAGS) $(GUIFLAGS) `pkg-config --libs $(RUTABAGA_LIBS)` $(RUTABAGA_S_LIBS)
