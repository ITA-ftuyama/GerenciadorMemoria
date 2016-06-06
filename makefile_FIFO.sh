#! /bin/bash

rm -rf *o MemoryManager_FIFO
gcc -std=c99 -Wall -c MemoryManager_FIFO.c
gcc MemoryManager_FIFO.o -o MemoryManager_FIFO -lpthread -lm

if [ $# -eq 0 ]
then
	./MemoryManager_FIFO
else
	./MemoryManager_FIFO $1
fi

exit 0