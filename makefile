all: MemoryManager
    
MemoryManager.o: MemoryManager_LRU.c
	gcc -std=c99 -Wall -c MemoryManager_LRU.c
	
MemoryManager: MemoryManager_FIFO.o
	gcc MemoryManager_LRU.o -o MemoryManager_LRU -lpthread -lm
	
clean:
	rm -rf *o MemoryManager_LRU
