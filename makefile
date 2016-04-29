all: MemoryManager
    
MemoryManager.o: MemoryManager.c
	gcc -std=c99 -Wall -c MemoryManager.c
	gcc -std=c99 -Wall MemoryManager.o
	
clean:
	rm -rf *o MemoryManager
