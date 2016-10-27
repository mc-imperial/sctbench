all: js-bug testharness

old: js-nobug testharness

patch: js-patch testharness


js-bug:
	./scripts/build-js.sh 1.8

js-nobug:
	./scripts/build-js.sh 1.7

js-patch:
	./scripts/build-js.sh 1.8-patch

testharness:
	./scripts/build-testharness.sh
	cp ./src/runme.sh ./bin/

clean:
	./scripts/clean.sh
