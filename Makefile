PREFIX?=/usr/local

all: build stalker

build:
	deps/gyp/gyp --depth=. -Goutput_dir=out -Icommon.gypi --generator-output=build -Dlibrary=static_library -Duv_library=static_library -f make -Dclang=1

stalker:
	make -C build stalker
	cp build/out/Release/stalker stalker

install: stalker
	install stalker $(PREFIX)/bin/stalker

uninstall:
	rm -fr $(PREFIX)/bin/stalker

distclean:
	make clean
	rm -fr build

clean:
	rm -fr build/out/Release/obj.target/stalker/
	rm -fr build/out/Release/stalker
	rm -f stalker

.PHONY: clean distclean
