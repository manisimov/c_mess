CC := gcc
CFLAGS := -c
INCLUDE := -Ilib

LINKER := gcc
LFLAGS := -lmy_shared
LINCLUDE := -Llib

OBJECTS := shared_lib.o


all: shared_lib.exe


shared_lib.exe: $(OBJECTS) makefile
	$(LINKER) $(LINCLUDE)  $(OBJECTS) -o shared_lib $(LFLAGS)


%.o : %.c makefile
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@
    
clean:
	rm -f *.o *.exe