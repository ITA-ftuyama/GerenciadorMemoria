#! /bin/bash

rm -rf *o MemoryManager_LRU
gcc -std=c99 -Wall -c MemoryManager_LRU.c
gcc MemoryManager_LRU.o -o MemoryManager_LRU -lpthread -lm

if [ $# -eq 0 ]
then
	./MemoryManager_LRU
else
	./MemoryManager_LRU $1
fi

exit 0