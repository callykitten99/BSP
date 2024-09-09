ifndef OS
$(error OS is not set)
endif
ifeq ($(OS),LINUX)
CFLAGS_OS = -DOS_LINUX
LFLAGS_OS = -Wl,-Bdynamic -lm -lxcb -lX11 -lGL
else ifeq ($(OS),WINDOWS)
CFLAGS_OS = -DOS_WINDOWS
LFLAGS_OS = -static -lopengl32 -lgdi32
else
$(error OS is not supported)
endif

CC		 = gcc
CFLAGS	 = -Wall -W -c $(CFLAGS_OS)
LFLAGS	 = $(LFLAGS_OS)
CFLAGS_D = $(CFLAGS) -g -DDEBUG -DVERBOSE=3
CFLAGS_R = $(CFLAGS) -O3 -s -DNDEBUG -DRELEASE
LFLAGS_D = $(LFLAGS)
LFLAGS_R = -s $(LFLAGS)
SRC		 = $(wildcard src/*.c)
OBJ_D	 = $(patsubst src/%.c,o/d/%.o,$(SRC))
OBJ_R	 = $(patsubst src/%.c,o/r/%.o,$(SRC))


ifeq ($(OS),WINDOWS)

all: o o/r o/d bsp.exe bsp_d.exe

bsp.exe: $(OBJ_R)
	$(CC) $^ -o bsp.exe $(LFLAGS_R)

bsp_d.exe: $(OBJ_D)
	$(CC) $^ -o bsp_d.exe $(LFLAGS_D)

else ifeq ($(OS),LINUX)

all: o o/r o/d bsp bsp_d

bsp: $(OBJ_R)
	$(CC) $^ -o bsp $(LFLAGS_R)

bsp_d: $(OBJ_D)
	$(CC) $^ -o bsp_d $(LFLAGS_D)

endif


$(OBJ_R): o/r/%.o: src/%.c
	$(CC) $(CFLAGS_R) $^ -o $@ 

$(OBJ_D): o/d/%.o: src/%.c
	$(CC) $(CFLAGS_D) $^ -o $@

o/d: o
	mkdir $@

o/r: o
	mkdir $@

o:
	mkdir $@


ifeq ($(OS),WINDOWS)

clean:
	rm o/d/*.o o/r/*.o bsp*.exe

else ifeq ($(OS),LINUX)

clean:
	rm o/d/*.o o/r/*.o bsp bsp_d

endif

