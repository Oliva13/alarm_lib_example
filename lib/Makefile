TARGET = libalarm.so

GCC_CMDLINE = -c -g -fPIC
LD_CMDLINE  = -L/usr/lib -g -shared
CC_INCLUDE  = -I/usr/include

LDLIBS	   = -lpthread  -lrt
C_SRC       = alarms.c

GCC_CMDLINE += -DDEBUG

OBJS        = $(patsubst %.c,%.o,$(C_SRC))

.SUFFIXES:	.o

.c.o:
	gcc $(GCC_CMDLINE) $< -o $@

$(TARGET):	$(OBJS) Makefile
	gcc $(LD_CMDLINE) $(OBJS) $(LDLIBS) -o $@
#	cp $(TARGET) /lib/

clean:
	rm -f *.o $(TARGET)

dep:
	gcc $(CC_INCLUDE) -MM $(C_SRC) > .deps

-include .deps
