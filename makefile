CC      = cc
CFLAGS += -O -std=c11 -Wall -pedantic
LIBS    = -lncurses
LD      = cc
CP      = cp
MV      = mv
RM      = rm

ze : ze.c
	$(LD) $(CFLAGS) -o ze ze.c $(LIBS)

clean:
	-$(RM) ze ze.o

install:
	-$(CP) ze $(HOME)/bin/ze
