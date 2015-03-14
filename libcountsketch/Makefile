BIN=heap_test #cmsketch_tool

CXX=g++
CC=gcc
CFLAGS=-g -Wall -DNO_DEBUG

.cpp.o:
	$(CXX) $(CFLAGS) -c $^ $(INC_DIR)

.cc.o:
	$(CC) $(CFLAGS) -c -std=c99 -x c $^ $(INC_DIR)

.c.o:
	$(CC) $(CFLAGS) -c $^ $(INC_DIR)

.o:
	$(CC) $(CFLAGS) -o $@ $^ $(LIB)


all: $(BIN)

$(BIN): $(BIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^  $(LIB)

heap_test: minheap.o CountMinSketch.o test_main.o
cmsketch_tool: minheap.o CountMinSketch.o tool_main.o

clean:
	rm -f *.o $(BIN)