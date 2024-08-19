CC = gcc
CXX = g++
CFLAGS = -c
CXXFLAGS = -std=c++0x -c
LDFLAGS =
OUT = ctpkt

OBJ = main.c.o ETC1/rg_etc1.cpp.o ETC1/etc1.cpp.o

all: $(OUT)

$(OUT): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $(OBJ)

main.c.o: main.c
	$(CC) $(CFLAGS) -o $@ main.c

ETC1/rg_etc1.cpp.o: ETC1/rg_etc1.cpp
	$(CXX) $(CXXFLAGS) -o $@ ETC1/rg_etc1.cpp

ETC1/etc1.cpp.o: ETC1/etc1.cpp
	$(CXX) $(CXXFLAGS) -o $@ ETC1/etc1.cpp

main.c.o: ctpkProcess.h

clean:
	rm -f $(OUT) $(OBJ)
