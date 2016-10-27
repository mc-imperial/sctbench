PREFIX=${PARSECDIR}/pkgs/apps/fluidanimate/inst/${PARSECPLAT}
TARGET=fluidanimate

all:
	$(MAKE) -f Makefile.${version} all

clean:
	$(MAKE) -f Makefile.${version} clean

install:
	mkdir -p $(PREFIX)/bin
	cp -f $(TARGET) $(PREFIX)/bin/$(TARGET)
