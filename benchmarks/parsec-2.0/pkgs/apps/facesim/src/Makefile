PREFIX		:= ${PARSECDIR}/pkgs/apps/facesim/inst/${PARSECPLAT}
BINARY		:= facesim

all:
	$(MAKE) -C TaskQ/lib all
	$(MAKE) -C Benchmarks/facesim

install:
	mkdir -p $(PREFIX)/bin
	cp -f Benchmarks/facesim/$(BINARY) $(PREFIX)/bin/$(BINARY)

