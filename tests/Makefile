CC := gcc
LD := ld
AR := ar
RM := rm
CP := cp

.SUFFIXES: .c
.PHONY: clean

%.o: %.c
	$(CC) $(CFLAGS) -I../ -c $<

all: unit-tests

unit-tests: unit-tests.o ../lib$(LIB_NAME).a
	$(CC) $(CFLAGS) -o $@ $< -lcunit -L../ -l$(LIB_NAME) -lpthread

unit-tests.o: unit-tests.c ../dispatch.h

clean:
	-$(RM) unit-tests
	-$(RM) *.o *~
