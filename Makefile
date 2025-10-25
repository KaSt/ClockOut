CC ?= gcc
CFLAGS ?= -O4
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?= -lncursesw

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

.PHONY: default clean release

default: deps clockout

clockout: clockout.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -f *.o
	rm -f clockout

deps:
	sudo apt install -y libjson-c-dev libncurses-dev

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
