CC := gcc
CFLAGS := -c
INCLUDE :=

AR := ar
ARFLAGS := rcs

OBJECTS :=  first.o\
            second.o\
            third.o
            
          
            
all: libmy_static.a
            
libmy_static.a : $(OBJECTS) makefile
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

%.o : %.c makefile
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm -f *.o *.a