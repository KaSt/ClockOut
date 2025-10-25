CC ?= gcc
CFLAGS ?= -O3
CPPFLAGS ?=
LDFLAGS ?=
DOCKER ?= docker
UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(UNAME_S),Darwin)
LDLIBS ?= -lncurses
else
LDLIBS ?= -lncursesw
endif

VERSION := $(shell sed -n 's/^static const char CLOCKOUT_VERSION\[\] = "\(.*\)";/\1/p' clockout.c)
DIST_DIR ?= dist

# Prefer pkg-config for locating json-c headers and libraries. If pkg-config
# is not available, fall back to Homebrew's default installation prefix when
# available, and finally rely on the system linker search path.
JSON_C_CFLAGS := $(shell pkg-config --cflags json-c 2>/dev/null)
JSON_C_LIBS := $(shell pkg-config --libs json-c 2>/dev/null)

ifeq ($(JSON_C_LIBS),)
JSON_C_PREFIX := $(shell brew --prefix json-c 2>/dev/null)
ifneq ($(JSON_C_PREFIX),)
JSON_C_CFLAGS := -I$(JSON_C_PREFIX)/include
JSON_C_LIBS := -L$(JSON_C_PREFIX)/lib -ljson-c
else
JSON_C_LIBS := -ljson-c
endif
endif

CPPFLAGS += $(JSON_C_CFLAGS)
LDFLAGS += $(filter -L%,$(JSON_C_LIBS))
LDLIBS += $(filter-out -L%,$(JSON_C_LIBS))

.PHONY: default clean test release \
alpine arch debian gentoo redhat suse slackware ubuntu \
homebrew macports

default: clockout

ci:
	$(MAKE) deps clockout

clockout: clockout.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

test: deps test_clockout
	./test_clockout

test_clockout: tests/test_clockout.c clockout.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -DCLOCKOUT_NO_MAIN $^ $(LDFLAGS) $(LDLIBS) -lm -o $@

clean:
	rm -f *.o
	rm -f clockout
	rm -f test_clockout

deps:
	@sh -c '\
		UNAME_S=$$(uname -s 2>/dev/null); \
		if [ "$$UNAME_S" = "Linux" ]; then \
			if command -v apt-get >/dev/null 2>&1; then \
				sudo apt-get update && sudo apt-get install -y libjson-c-dev libncurses-dev; \
			elif command -v yum >/dev/null 2>&1; then \
				sudo yum install -y json-c-devel ncurses-devel; \
			else \
				echo "Unsupported Linux package manager. Install libjson-c-dev and libncurses-dev manually."; \
				exit 1; \
			fi; \
		elif [ "$$UNAME_S" = "Darwin" ]; then \
			if command -v brew >/dev/null 2>&1; then \
				brew install json-c ncurses; \
			elif command -v port >/dev/null 2>&1; then \
				sudo port selfupdate && sudo port install json-c ncurses; \
			else \
				echo "Homebrew or MacPorts not found. Please install json-c and ncurses with Homebrew (brew install json-c ncurses) or MacPorts (sudo port install json-c ncurses)."; \
				exit 1; \
			fi; \
		else \
			echo "Unsupported OS: $$UNAME_S. Please install json-c and ncurses manually."; \
			exit 1; \
		fi; \
		# Run Snyk security scan if available (per project security best practices) \
		if command -v snyk_code_scan >/dev/null 2>&1; then \
			echo "Running snyk_code_scan..."; \
			snyk_code_scan || true; \
		fi'

release: clockout
	@if ! command -v gh >/dev/null 2>&1; then \
		echo "GitHub CLI (gh) is required for release automation."; \
		exit 1; \
	fi
	@version="$(VERSION)"; \
	if [ -z "$$version" ]; then \
		echo "Unable to determine version from clockout.c"; \
		exit 1; \
	fi; \
	tags=$$(git tag --points-at HEAD); \
	if [ -n "$$tags" ]; then \
		for tag in $$tags; do \
			if ! gh release view "$$tag" >/dev/null 2>&1; then \
				echo "Creating GitHub release for $$tag"; \
				gh release create "$$tag" clockout --title "ClockOut $$tag" --notes "Automated release for $$tag"; \
			else \
				echo "Release $$tag already exists."; \
			fi; \
		done; \
	else \
		tag="v$$version"; \
		if ! git rev-parse "$$tag" >/dev/null 2>&1; then \
			git tag "$$tag"; \
			echo "Created tag $$tag"; \
		fi; \
		if ! gh release view "$$tag" >/dev/null 2>&1; then \
			echo "Creating GitHub release for $$tag"; \
			gh release create "$$tag" clockout --title "ClockOut $$tag" --notes "Automated release for $$tag"; \
		else \
			echo "Release $$tag already exists."; \
		fi; \
	fi
define require_docker
	@if ! command -v $(DOCKER) >/dev/null 2>&1; then \
		echo "Docker is required to build $1 packages."; \
		exit 1; \
	fi
endef

define docker_package
	@mkdir -p $(DIST_DIR)
	@echo "Building $1 package (version $(VERSION))"
	$(DOCKER) build --build-arg VERSION=$(VERSION) -f docker/$1/Dockerfile -t clockout-$1:$(VERSION) .
	@cid=$$($(DOCKER) create clockout-$1:$(VERSION)); \
	$(DOCKER) cp $$cid:/dist/. $(DIST_DIR)/; \
	$(DOCKER) rm $$cid >/dev/null
endef

alpine:
	$(call require_docker,Alpine)
	$(call docker_package,alpine)

arch:
	$(call require_docker,Arch)
	$(call docker_package,arch)

debian:
	$(call require_docker,Debian)
	$(call docker_package,debian)

gentoo:
	$(call require_docker,Gentoo)
	$(call docker_package,gentoo)

redhat:
	$(call require_docker,Red Hat)
	$(call docker_package,redhat)

suse:
	$(call require_docker,openSUSE)
	$(call docker_package,suse)

slackware:
	$(call require_docker,Slackware)
	$(call docker_package,slackware)

ubuntu:
	$(call require_docker,Ubuntu)
	$(call docker_package,ubuntu)

homebrew:
	@mkdir -p $(DIST_DIR)/homebrew
	@git archive --format=tar --prefix=clockout-$(VERSION)/ HEAD | gzip > $(DIST_DIR)/homebrew/clockout-$(VERSION).tar.gz
	@sha=$$( (command -v shasum >/dev/null 2>&1 && shasum -a 256 $(DIST_DIR)/homebrew/clockout-$(VERSION).tar.gz || sha256sum $(DIST_DIR)/homebrew/clockout-$(VERSION).tar.gz) | awk '{print $$1}' ); \
	sed -e "s/@VERSION@/$(VERSION)/g" -e "s/@SHA256@/$$sha/g" packaging/homebrew/clockout.rb.in > $(DIST_DIR)/homebrew/clockout.rb

macports:
	@mkdir -p $(DIST_DIR)/macports
	@git archive --format=tar --prefix=clockout-$(VERSION)/ HEAD | gzip > $(DIST_DIR)/macports/clockout-$(VERSION).tar.gz
	@sha=$$( (command -v shasum >/dev/null 2>&1 && shasum -a 256 $(DIST_DIR)/macports/clockout-$(VERSION).tar.gz || sha256sum $(DIST_DIR)/macports/clockout-$(VERSION).tar.gz) | awk '{print $$1}' ); \
	sed -e "s/@VERSION@/$(VERSION)/g" -e "s/@SHA256@/$$sha/g" packaging/macports/Portfile.in > $(DIST_DIR)/macports/Portfile
