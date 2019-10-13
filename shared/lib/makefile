CC := gcc
CFLAGS :=   -fPIC\
            -c
INCLUDE :=

LINKER  := gcc
LFLAGS  := -shared

OBJECTS :=  first.o\
            second.o\
            third.o
            
          
            
all: libmy_shared.dll
            
libmy_shared.dll : $(OBJECTS) makefile
	$(LINKER) $(LFLAGS) $(OBJECTS) -o $@

%.o : %.c makefile
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm -f *.o *.a