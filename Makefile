CC ?= gcc
CFLAGS ?= -O3
CPPFLAGS ?=
LDFLAGS ?=
UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(UNAME_S),Darwin)
LDLIBS ?= -lncurses
else
LDLIBS ?= -lncursesw
endif

VERSION := $(shell sed -n 's/^static const char CLOCKOUT_VERSION\[\] = "\(.*\)";/\1/p' clockout.c)

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

.PHONY: default clean test release

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
