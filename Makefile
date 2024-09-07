CC		 = gcc
CFLAGS	 = -Wall -W -c
LFLAGS	 = -lopengl32 -lgdi32
CFLAGS_D = $(CFLAGS) -g -DDEBUG -DVERBOSE=3
CFLAGS_R = $(CFLAGS) -O3 -s -DNDEBUG -DRELEASE
LFLAGS_D = $(LFLAGS)
LFLAGS_R = -s -static $(LFLAGS)
SRC		 = $(wildcard src/*.c)
OBJ_D	 = $(patsubst src/%.c,o/d/%.o,$(SRC))
OBJ_R	 = $(patsubst src/%.c,o/r/%.o,$(SRC))


all: o o/r o/d bsp.exe bsp_d.exe


bsp.exe: $(OBJ_R)
	$(CC) $^ -o bsp.exe $(LFLAGS_R)

bsp_d.exe: $(OBJ_D)
	$(CC) $^ -o bsp_d.exe $(LFLAGS_D)


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


clean:
	rm o/d/*.o o/r/*.o bsp*.exe

