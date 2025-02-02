
PLATFORM ?= linux

INCL = -I../../include/ -I ../../../../ -I../../
DEF = -DTEKLUA -DTEKLIB

LIBS = -L/usr/local/lib -L../../../../lib/$(PLATFORM) \
	-lhal -lexec -ltime -lutil -lio -liohnd_default -lvisual \
	-ltekc -ltekdebug -L/usr/X11R6/lib -lX11 -lXext \
	-pthread

.c.o:
	$(CC) -O2 $(DEF) $(INCL) -fPIC -DPIC -c $? -o $@

all: vis.so

install:
	install -d /usr/local/lib/lua/5.1/tek/
	install -p vis.so /usr/local/lib/lua/5.1/tek/
	
vis.so: lua_tek_vis.o lua_tek_vis_lua.o
	$(CC) $? -shared -o $@ $(LIBS)

clean:
	-rm *.o *.so
