/** 
 * CES-33 Final Project
 * 
 *  Memory Manager Simulator
 * 
 *  Felipe Tuyama de F. Barbosa
 */
 
/**
 * 	Memory Manager Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/**
 * 	Memory Manager Defines
 */
// Max Number Definitions
#define MaxStringLength 	7
#define NumThreads			2

// Virtual Memory Pages
#define PagesAmount  		256
#define TLBEntriesAmount	15

// Physical Memory RAM
#define FramesAmount 		128
#define FrameBytesSize 		256

/**
 * 	Memory Manager Structs
 */
// Page Table (256 pages)
typedef struct pageTable {
	int pageNumber[PagesAmount], frameNumber[PagesAmount];
} PageTable;

// Page (256 bytes)
typedef struct page {
	char PageContent[FrameBytesSize];
} Page;

// TLB - Maps Pages on Physical Memory (16 entries)
typedef struct tlb {
	int pageNumber[TLBEntriesAmount], frameNumber[TLBEntriesAmount];
	unsigned int LRU[TLBEntriesAmount];
} TLB;

// Physical Memory (65.536 bytes)
typedef struct memory {
	Page frame[FramesAmount];
	int available[FramesAmount];
	unsigned int LRU[FramesAmount];
} Memory;

// Statistics
typedef struct statistics {
	int TranslatedAddressesCounter;
	int PageFaultsCounter;
	int TLBHitsCounter;
} Statistics;

/**
 * 	Threads
 */
pthread_t threads[NumThreads];

/**
 * 	Program Global Variables
 */
FILE *addresses, *result, *backingStore;
char line[MaxStringLength] = "";

Statistics *_statistics;
PageTable *_pageTable;
Memory *_memory;
Page *_page;
TLB *_TLB;

/**
 * 	Output results methods
 */
// Write Output results
void writeOut(int virtualAddress, int realAddress, int value)
{
	fprintf(result, "Virtual address: %d ", virtualAddress);
    fprintf(result, "Physical address: %d ", realAddress);
    fprintf(result, "Value: %d\n", value);
}

// Statistics Output Log
void statisticsLog()
{
	float pageFaultRate = _statistics->PageFaultsCounter;
		  pageFaultRate = pageFaultRate/_statistics->TranslatedAddressesCounter;
	float tlbHitsRate = _statistics->TLBHitsCounter;
		  tlbHitsRate = tlbHitsRate/_statistics->TranslatedAddressesCounter;
	fprintf(result, "Number of Translated Addresses = %d\n", _statistics->TranslatedAddressesCounter);
	fprintf(result, "Page Faults = %d\n", _statistics->PageFaultsCounter);
	fprintf(result, "Page Fault Rate = %.3f\n", pageFaultRate);
	fprintf(result, "TLB Hits = %d\n", _statistics->TLBHitsCounter);
	fprintf(result, "TLB Hit Rate = %.3f\n", tlbHitsRate);
}

/**
 * 	Initialization/Finalization methods
 */
// Initializing the Memory Manager
void initialize()
{
	backingStore = fopen("BACKING_STORE.bin", "r");
	addresses = fopen("addresses.txt", "r");
    result = fopen("result.txt", "w");
    
    _statistics = (Statistics*)malloc(sizeof(Statistics));
	_pageTable = (PageTable*)malloc(sizeof(PageTable));
	_memory = (Memory*)malloc(sizeof(Memory));
	_page = (Page*)malloc(sizeof(Page));
	_TLB = (TLB*)malloc(sizeof(TLB));
	
	for (int i = 0; i < PagesAmount; i++) 
		_pageTable->frameNumber[i] = _pageTable->pageNumber[i] = -1;
	
	for (int i = 0; i < TLBEntriesAmount; i++)
		_TLB->frameNumber[i] = _TLB->pageNumber[i] = _TLB->LRU[i] = -1;
		
	for (int i = 0; i < FramesAmount; i++)
		_memory->available[i] = 1;
		
	_statistics->TranslatedAddressesCounter = 0;
	_statistics->PageFaultsCounter = 0;
	_statistics->TLBHitsCounter = 0;
}

// Finalizing the Memory Manager
void finalize()
{
	statisticsLog();
	fclose(backingStore);
	fclose(addresses);
    fclose(result);
    
    free(_pageTable);
    free(_memory);
    free(_page);
    free(_TLB);
}

/**
 * 	Managing Page Table methods
 */
// Finding Requested Page on Page Table
int findPageOnPageTable(int pageNumber)
{
	for (int i = 0; i < PagesAmount; i++) 
		if (_pageTable->pageNumber[i] == pageNumber)
			return _pageTable->frameNumber[i];
	
	// Requested Page not found on TLB. (Page Fault)
	return -1;
}

// Setting Used Page on Page Table
void setPageOnPageTable(int pageNumber, int frameNumber)
{
	_pageTable->pageNumber[pageNumber] = pageNumber;
	_pageTable->frameNumber[pageNumber] = frameNumber;
}
/**
 * 	Managing TLB methods
 */
// Update TLB LRU
void updateTLBLRUusing(int index)
{
	for (int i = 0; i < TLBEntriesAmount; i++)
		_TLB->LRU[i] = _TLB->LRU[i] >> 1;
	_TLB->LRU[index] = _TLB->LRU[index] | (1 << 31);
}

// Finding Requested Page on TLB
int findPageOnTLB(int pageNumber)
{
	for (int i = 0; i < TLBEntriesAmount; i++)
		if (_TLB->pageNumber[i] == pageNumber) {
			_statistics->TLBHitsCounter++;
			updateTLBLRUusing(i);
			return _TLB->frameNumber[i];
		}
	
	// Requested Page not found on TLB.
	return -1;
}

// Setting Used Page on TLB
void setPageOnTLB(int pageNumber, int frameNumber)
{
	// Aging Algorithm using LRU
	int newTLBindex = 0;
	for (int i = 0; i < TLBEntriesAmount; i++)
		if (_TLB->LRU[i] < _TLB->LRU[newTLBindex])
			newTLBindex = i;
	
	updateTLBLRUusing(newTLBindex);
	_TLB->frameNumber[newTLBindex] = frameNumber;
	_TLB->pageNumber[newTLBindex] = pageNumber;
}
/**
 * 	Managing Memory methods
 */
// Update MEM LRU
void updateMEMLRUusing(int index)
{
	for (int i = 0; i < FramesAmount; i++)
		_memory->LRU[i] = _memory->LRU[i] >> 1;
	_memory->LRU[index] = _memory->LRU[index] | (1 << 31);
}

// Find Oldest Frame on memory using LRU
int findOldestFrameOnMemory()
{
	int newFrameIndex = 0;
	for (int i = 0; i < FramesAmount; i++)
		if (_memory->LRU[i] < _memory->LRU[newFrameIndex])
			newFrameIndex = i;
	return newFrameIndex;
}

// Find Available Frame on memory
int findAvailableFrameOnMemory()
{
	for (int i = 0; i < FramesAmount; i++) 
		if (_memory->available[i] == 1) {
			_memory->available[i] = 0;
			return i;
		}
	return -1;
}

// Find Frame on memory
int findFrameOnMemory()
{
	int chosenFrame = findAvailableFrameOnMemory();
	if (chosenFrame == -1)
		chosenFrame = findOldestFrameOnMemory();
	updateMEMLRUusing(chosenFrame);
	return chosenFrame;
}

/**
 *  Managing Backing Store methods
 */
// Manage Backing_Store
void getBackingStorePage(int pageNumber, int frameNumber)
{
	_statistics->PageFaultsCounter++;
	fseek(backingStore, pageNumber*FrameBytesSize, SEEK_SET);
	fread(_memory->frame[frameNumber].PageContent, FrameBytesSize, 1, backingStore);
}

/**
 * 	Debug application methods
 */
// Debug Page Address
void debugPageAddress(int address, int pageNumber, int offset)
{
	printf ("Virtual Address: %5d ", address); 
	printf ("PageNumber : %3d ", pageNumber);
	printf ("PageOffset : %3d", offset);
	getchar();
}

// Debug Real Address
void debugFrameAddress(int address, int frameNumber, int offset)
{
	printf ("  Real  Address: %5d ", address); 
	printf ("FrameNumber: %3d ", frameNumber);
	printf ("FrameOffset: %3d", offset);
	getchar();
}

void *thread_findOnPageTable(void *pageNumber) {
	int *pageNumber = (int*)pageNumber;
	// Usar mutex para frameNumber
	
}

void *thread_findOnTLB(void *pageNumber) {
	int *pageNumber = (int*)pageNumber;
	// Usar mutex para frameNumber
	
}
/**
 * 	Main Memory Manager
 */
int main()
{
	initialize();
    while(fgets(line , MaxStringLength, addresses))
    {
		// Read new virtual Address
		_statistics->TranslatedAddressesCounter++;
        int virtualAddress = atoi(line);
        
        // Parse read virtual Address
        int pageNumber =  virtualAddress/PagesAmount;
        int offset = virtualAddress & (PagesAmount-1);
        
        // Find frameNumber on TLB
        int frameNumber = findPageOnTLB(pageNumber);
        
        // Find frameNumber on TLB and PageTable at same time
        pthread_create(&threads[0], NULL, thread_findOnTLB, &(pageNumber));
        pthread_create(&threads[1], NULL, thread_findOnPageTable, &(pageNumber));
        
        pthread_join(threads[0], NULL);
        pthread_join(threads[1], NULL);
        
        // If TLB find fails
        if (frameNumber == -1)
        {
			// Find frameNumber on Page Table
			frameNumber = findPageOnPageTable(pageNumber);
			
			// If Page Fault
			if (frameNumber == -1)
			{
				// Load on memory entire page of BACKING_STORE
				frameNumber = findFrameOnMemory();
				getBackingStorePage(pageNumber, frameNumber);
				
				// Set up accessed page on Page Table
				setPageOnPageTable(pageNumber, frameNumber);
			}
			// Set up accessed page on TLB
			setPageOnTLB(pageNumber, frameNumber);
		}
		// Getting memory's desired value
		int value = _memory->frame[frameNumber].PageContent[offset];
		
		// Parse real Address
		int realAddress = frameNumber*PagesAmount + offset;
		
		// Debuggind PageAddress and FrameAddress
        //debugPageAddress(virtualAddress, pageNumber, offset);
        //debugFrameAddress(realAddress, frameNumber, offset);
        
        writeOut(virtualAddress, realAddress, value);
    }
    finalize();
    return 0;
}
