all: nspr-bug testharness

sleep: nspr-sleep testharness

patch: nspr-patch testharness

nspr-bug:
	./scripts/build-nspr.sh default

nspr-sleep:
	./scripts/build-nspr.sh sleep

testharness:
	./scripts/build-testharness.sh
	cp ./src/runme.sh ./bin/

clean:
	./scripts/clean.sh
