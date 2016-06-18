#! /bin/bash

rm MemoryManager_Exame.o
gcc -std=c99 -Wall -c MemoryManager_Exame.c
gcc MemoryManager_Exame.o -o MemoryManager_Exame -lm

if [ $# -eq 0 ]
then
	./MemoryManager_Exame
else
	./MemoryManager_Exame $1
fi

exit 0