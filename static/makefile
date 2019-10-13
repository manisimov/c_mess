CC := gcc
CFLAGS := -c
INCLUDE := -Ilib

LINKER := gcc
LFLAGS := -lmy_static
LINCLUDE := -Llib

OBJECTS := static_lib.o


all: static_lib.exe


static_lib.exe: $(OBJECTS) makefile
	$(LINKER) $(LINCLUDE)  $(OBJECTS) -o static_lib $(LFLAGS)


%.o : %.c makefile
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@
    
clean:
	rm -f *.o *.exe