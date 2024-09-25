CC = gcc
CFLAGS = -c -O2
LDFLAGS = -lz
OUT = zlib-sarc

OBJ = main.c.o

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

main.c.o: main.c
	$(CC) $(CFLAGS) -o $@ main.c

main.c.o: sarcProcess.h
main.c.o: zlibProcess.h
main.c.o: common.h

clean:
	rm -f $(OUT) $(OBJ)
