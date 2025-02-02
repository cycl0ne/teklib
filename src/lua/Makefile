
INCL = -Iinclude/ -I ../../ -I.
DEF = -DTEKLUA -DTEKLIB

LIBS = -L../../lib/linux -ltekc -ltekdebug -lm 

%.o: %.c
	$(CC) -g $(DEF) $(INCL) -fPIC -DPIC -c $< -o $@


all: tek.so

install:
	install -p tek.so /usr/local/lib/lua/5.1/
	
tek.so: lua_teklib.o
	$(CC) lua_teklib.o -shared -o $@ $(LIBS)

clean:
	-rm *.o *.so
