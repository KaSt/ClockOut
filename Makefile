CC ?= gcc
CFLAGS ?= -O4
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?= -lncursesw

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

.PHONY: default clean

default: clockout

clockout: clockout.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -f *.o
	rm -f clockout
