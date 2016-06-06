all: MemoryManager
    
MemoryManager.o: MemoryManager_FIFO.c
	gcc -std=c99 -Wall -c MemoryManager_FIFO.c
	
MemoryManager: MemoryManager_FIFO.o
	gcc MemoryManager_FIFO.o -o MemoryManager_FIFO -lpthread -lm
	
clean:
	rm -rf *o MemoryManager_FIFO
