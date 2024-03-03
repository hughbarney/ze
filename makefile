CC      = cc
CFLAGS += -O -std=c11 -Wall -pedantic
LIBS    = -lncurses
LD      = cc
CP      = cp
MV      = mv
RM      = rm

ze1 : ze1.c
	$(LD) $(CFLAGS) -o ze1 ze1.c $(LIBS)

ze2 : ze2.c
	$(LD) $(CFLAGS) -o ze2 ze2.c $(LIBS)

all : ze1 ze2

clean:
	-$(RM) ze1 ze2

