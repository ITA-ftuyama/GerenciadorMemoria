#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MaxStringLength 	7

// Virtual Memory Pages
#define PagesAmount  		256
#define TLBEntriesAmount	15

// Physical Memory RAM
#define FramesAmount 		256
#define FrameBytesSize 		256

// Page Address (16 bits)
typedef struct pageAddress {
	int pageNumber, offset;
} PageAddress;

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

// Physical Memory Address (16 bits)
typedef struct frameAddress {
	int frameNumber, offset;
} FrameAddress;

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

FILE *addresses, *result, *backingStore;
char line[MaxStringLength] = "";

Statistics *_statistics;
PageAddress *_pageAddress;
FrameAddress *_frameAddress;
PageTable *_pageTable;
Memory *_memory;
Page *_page;
TLB *_TLB;

// Notification - File not found
FILE *readFile(char *name)
{
	FILE *file = fopen (name, "r");
	if (file == NULL)  {
		printf("%s not Found.\n", name);
		exit (1);
	}
	return file;
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

// Initializing the Memory Manager
void initialize()
{
	backingStore = fopen("BACKING_STORE.bin", "r");
	addresses = readFile("addresses.txt");
    result = fopen("result.txt", "w");
    
    _statistics = (Statistics*)malloc(sizeof(Statistics));
	_pageAddress = (PageAddress*)malloc(sizeof(PageAddress));
	_frameAddress = (FrameAddress*)malloc(sizeof(FrameAddress));
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
    
	free(_pageAddress);
    free(_frameAddress);
    free(_pageTable);
    free(_memory);
    free(_page);
    free(_TLB);
}

// Manage Backing_Store
void getBackingStorePage(int pageNumber, int frameNumber)
{
	_statistics->PageFaultsCounter++;
	fseek(backingStore, pageNumber*FrameBytesSize, SEEK_SET);
	fread(_memory->frame[frameNumber].PageContent, FrameBytesSize, 1, backingStore);
}

// Searching Requested Page on Page Table
int searchPageOnPageTable(int pageNumber)
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

// Update TLB LRU
void updateTLBLRUWith(int index, int bit)
{
	for (int i = 0; i < TLBEntriesAmount; i++)
		_TLB->LRU[i] = _TLB->LRU[i] >> 1;
	_TLB->LRU[index] = _TLB->LRU[index] | (bit << 31);
}

// Searching Requested Page on TLB
int searchPageOnTLB(int pageNumber)
{
	for (int i = 0; i < TLBEntriesAmount; i++)
		if (_TLB->pageNumber[i] == pageNumber) {
			_statistics->TLBHitsCounter++;
			updateTLBLRUWith(i, 1);
			return _TLB->frameNumber[i];
		}
	
	// Requested Page not found on TLB.
	return -1;
}

// Update MEM LRU
void updateMEMLRUWith(int index, int bit)
{
	for (int i = 0; i < FramesAmount; i++)
		_memory->LRU[i] = _memory->LRU[i] >> 1;
	_memory->LRU[index] = _memory->LRU[index] | (bit << 31);
}

// Search Available Frame on memory
int searchAvailableFrameOnMemory()
{
	for (int i = 0; i < FramesAmount; i++) 
		if (_memory->available[i] == 1) {
			_memory->available[i] = 0;
			return i;
		}
	return -1;
}

// Search Oldest Frame on memory using LRU
int searchOldestFrameOnMemory()
{
	printf("Aging"); getchar();
	// Aging Algorithm using LRU
	int newFrameIndex = 0;
	for (int i = 0; i < FramesAmount; i++)
		if (_memory->LRU[i] < _memory->LRU[newFrameIndex])
			newFrameIndex = i;
	return newFrameIndex;
}

// Search Frame on memory
int searchFrameOnMemory()
{
	int chosenFrame = searchAvailableFrameOnMemory();
	if (chosenFrame == -1)
		chosenFrame = searchOldestFrameOnMemory();
	updateTLBLRUWith(chosenFrame, 1);
	return chosenFrame;
}

// Setting Used Page on TLB
void setPageOnTLB(int pageNumber, int frameNumber)
{
	// Aging Algorithm using LRU
	int newTLBindex = 0;
	for (int i = 0; i < TLBEntriesAmount; i++)
		if (_TLB->LRU[i] < _TLB->LRU[newTLBindex])
			newTLBindex = i;
	
	updateTLBLRUWith(newTLBindex, 1);
	_TLB->frameNumber[newTLBindex] = frameNumber;
	_TLB->pageNumber[newTLBindex] = pageNumber;
}

// Debug Page Address
void debugPageAddress(int address, PageAddress *pageAdd)
{
	printf ("Virtual Address: %5d ", address); 
	printf ("PageNumber : %3d ", pageAdd->pageNumber);
	printf ("PageOffset : %3d", pageAdd->offset);
	getchar();
}

// Debug Real Address
void debugFrameAddress(int address, FrameAddress *frameAdd)
{
	printf ("  Real  Address: %5d ", address); 
	printf ("FrameNumber: %3d ", frameAdd->frameNumber);
	printf ("FrameOffset: %3d", frameAdd->offset);
	getchar();
}

// Debug Something
void debugSomething(int virtualAddress, int frameNumber)
{
	printf("vAdd: %d ", virtualAddress);
	printf("value: %d", _memory->frame[frameNumber].PageContent[_frameAddress->offset]); 
	printf(" frameNumber: %d pageNumber: %d", frameNumber, _pageAddress->pageNumber); 
	printf(" offset: %d", _frameAddress->offset);
	printf(" rAdd: %d", frameNumber*PagesAmount + _pageAddress->offset);
	getchar();
}

void writeOut(int virtualAddress, int realAddress, int value)
{
	fprintf(result, "Virtual address: %d ", virtualAddress);
    fprintf(result, "Physical address: %d ", realAddress);
    fprintf(result, "Value: %d\n", value);
}

int main()
{
	initialize();
    int virtualAddress, realAddress, value = 0;
    while(fgets(line , MaxStringLength, addresses))
    {
		// Read new virtual Address
		_statistics->TranslatedAddressesCounter++;
        virtualAddress = atoi(line);
        
        // Parse read virtual Address
        _pageAddress->pageNumber = virtualAddress/PagesAmount;
        _pageAddress->offset = virtualAddress & (PagesAmount-1);
        //debugPageAddress(virtualAddress, _pageAddress);
        
        // Query frameNumber on TLB
        int frameNumber = searchPageOnTLB(_pageAddress->pageNumber);
        _frameAddress->frameNumber = frameNumber;
        _frameAddress->offset = _pageAddress->offset;
        
        // If TLB search fails
        if (frameNumber == -1)
        {
			// Query frameNumber on Page Table
			frameNumber = searchPageOnPageTable(_pageAddress->pageNumber);
			_frameAddress->frameNumber = frameNumber;
			
			// If Page Fault
			if (frameNumber == -1)
			{
				// Load on memory entire page of BACKING_STORE
				frameNumber = searchAvailableFrameOnMemory();
				_frameAddress->frameNumber = frameNumber;
				getBackingStorePage(_pageAddress->pageNumber, frameNumber);
				
				// Set up accessed page on Page Table
				setPageOnPageTable(_pageAddress->pageNumber, frameNumber);
			}
			// Set up accessed page on TLB
			setPageOnTLB(_pageAddress->pageNumber, frameNumber);
		}
		else {
			//debugSomething(virtualAddress, frameNumber);
		}
		// Getting the memory's desired value
		value = _memory->frame[frameNumber].PageContent[_frameAddress->offset];
		
		// Parse real Address
		realAddress = frameNumber*PagesAmount + _pageAddress->offset;
        //debugFrameAddress(realAddress, _frameAddress);
        writeOut(virtualAddress, realAddress, value);
    }
    finalize();
    return 0;
}
