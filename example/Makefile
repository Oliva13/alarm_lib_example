TARGET = alarm

GCC_CMDLINE = -c -g
LD_CMDLINE  = -L./ 
#-L/usr/lib -g
CC_INCLUDE  = -I/usr/include

LDLIBS	   = -lalarm
C_SRC       = main.c

GCC_CMDLINE += -DDEBUG

OBJS        = $(patsubst %.c,%.o,$(C_SRC))

.SUFFIXES:	.o

.c.o:
	gcc $(GCC_CMDLINE) $< -o $@

$(TARGET):	$(OBJS) Makefile
	gcc $(LD_CMDLINE) $(OBJS) $(LDLIBS) -o $@
        
clean:
	rm -f *.o $(TARGET)

dep:
	gcc $(CC_INCLUDE) -MM $(C_SRC) > .deps

-include .deps
