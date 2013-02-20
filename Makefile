REPORTER  ?= spec
TESTS     ?= test/*.coffee

all: build

build:
	node-gyp build

clean:
	node-gyp clean

test:
	@PATH="./node_modules/.bin:${PATH}" && NODE_PATH="./lib:$(NODE_PATH)" \
	mocha \
	--reporter $(REPORTER) \
	--require should \
	--compilers coffee:coffee-script \
	$(TESTS)

.PHONY: build clean test
