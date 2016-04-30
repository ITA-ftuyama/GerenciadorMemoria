all: MemoryManager
    
MemoryManager.o: MemoryManager.c
	gcc -std=c99 -Wall -c MemoryManager.c
	
MemoryManager: MemoryManager.o
	gcc MemoryManager.o -o MemoryManager -lpthread -lm
	
clean:
	rm -rf *o MemoryManager
