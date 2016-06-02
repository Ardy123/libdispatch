CC := gcc
LD := ld
AR := ar
RM := rm
CP := cp
CFLAGS := -Wall -Werror -g
LIB_NAME := dispatch
STATIC_LIB := lib$(LIB_NAME).a

.SUFFIXES: .c
.PHONY: clean tests


%.o: %.c
	$(CC) $(CFLAGS) -c $<

all: $(STATIC_LIB) tests

$(STATIC_LIB): dispatch.o
	$(AR) rcs $@ $^

disbatch.o: dispatch.c dispatch.h

tests:
	$(MAKE) -C tests LIB_NAME=$(LIB_NAME) CFLAGS="$(CFLAGS)"

clean:
	$(MAKE) -C tests clean LIB_NAME=$(LIB_NAME)
	-$(RM) $(STATIC_LIB)
	-$(RM) *.o *~
