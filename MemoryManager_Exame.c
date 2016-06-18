/** 
 * CES-33 Final Project
 * 
 *  Memory Manager Simulator
 * 
 *  Felipe Tuyama de F. Barbosa
 *	Luiz Angel Rocha Rafael
 */
 
/**
 * 	Memory Manager Includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * 	Memory Manager Defines
 */
// Max Number Definitions
#define MaxStringLength			7

// Virtual Memory Pages
#define PagesAmount				256
#define TLBEntriesAmount		16

// Physical Memory RAM
#define FramesAmount			128
#define FrameBytesSize			256

// Address Bits
#define SegmentBits				4
#define PageBits				256
#define OffsetBits				256

//Segmentation
#define SegmentsAmount			4

//Files
#define inputfile_default 		"addresses.txt"
#define backingStore_default 	"BACKING_STORE.bin"
#define result_default 			"result.txt"

	/**
	*     Memory Manager Structs
	*/

// Page Table (256 pages)
typedef struct pageTable {
	int frameNumber[PagesAmount];
	unsigned int FIFO[FramesAmount];
} PageTable;

// Segmentation (4 segmentations)
typedef struct segmentation {
		PageTable pageTable[SegmentsAmount];
} Segmentation;

// Page (256 bytes)
typedef struct page {
	char PageContent[FrameBytesSize];
} Page;

// TLB - Maps Pages on Physical Memory (16 entries)
typedef struct tlb {
	int segmentNumber[TLBEntriesAmount];
	int pageNumber[TLBEntriesAmount];
	int frameNumber[TLBEntriesAmount];
	unsigned int FIFO[TLBEntriesAmount];
} TLB;

// Physical Memory (65.536 bytes)
typedef struct memory {
	Page frame[SegmentsAmount*FramesAmount];
	int available[SegmentsAmount*FramesAmount];
	int availableSegmentation[SegmentsAmount];
} Memory;

// Statistics
typedef struct statistics {
	int TranslatedAddressesCounter;
	int SegmentationFaultsCounter;
	int PageFaultsCounter;
	int TLBHitsCounter;
} Statistics;

/**
*     Program Global Variables
*/
FILE *addresses, *result, *backingStore;
char line[MaxStringLength] = "";

Segmentation *_descriptorTable;
Statistics *_statistics;
Memory *_memory;
TLB *_TLB;

/**
*     Output results methods
*/
// Write Output results
void writeOut(int segmentNumber, int virtualAddress, int realAddress, int value)
{
	fprintf(result, "Virtual address: %d-%d ", segmentNumber, virtualAddress);
	fprintf(result, "Physical address: %d-%d ", segmentNumber, realAddress);
	fprintf(result, "Value: %d\n", value);
	_statistics->TranslatedAddressesCounter++;
}

// Statistics Output Log
void statisticsLog()
{
	float segmentationFaultRate = _statistics->SegmentationFaultsCounter;
	segmentationFaultRate = segmentationFaultRate / _statistics->TranslatedAddressesCounter;
	float pageFaultRate = _statistics->PageFaultsCounter;
	pageFaultRate = pageFaultRate / _statistics->TranslatedAddressesCounter;
	float tlbHitsRate = _statistics->TLBHitsCounter;
	tlbHitsRate = tlbHitsRate / _statistics->TranslatedAddressesCounter;
	
	fprintf(result, "Number of Translated Addresses = %d\n", _statistics->TranslatedAddressesCounter);
	fprintf(result, "Segmentation Faults = %d\n", _statistics->SegmentationFaultsCounter);
	fprintf(result, "Segmentation Fault Rate = %.3f\n", segmentationFaultRate);
	fprintf(result, "Page Faults = %d\n", _statistics->PageFaultsCounter);
	fprintf(result, "Page Fault Rate = %.3f\n", pageFaultRate);
	fprintf(result, "TLB Hits = %d\n", _statistics->TLBHitsCounter);
	fprintf(result, "TLB Hit Rate = %.3f\n", tlbHitsRate);
}

/**
*     Initialization/Finalization methods
*/
// Initializing the Memory Manager
void initialize(char * inputfile)
{
	backingStore = fopen(backingStore_default, "r");
	addresses = fopen(inputfile, "r");
	result = fopen(result_default, "w");

	_statistics = (Statistics*)malloc(sizeof(Statistics));
	_memory = (Memory*)malloc(sizeof(Memory));
	_TLB = (TLB*)malloc(sizeof(TLB));
	_descriptorTable = (Segmentation*)malloc(SegmentsAmount * sizeof(Segmentation));

	for (int j = 0; j < SegmentsAmount; j ++) {
		for (int i = 0; i < PagesAmount; i++) {
			_descriptorTable[j].pageTable->frameNumber[i] = -1;
			_descriptorTable[j].pageTable->FIFO[i] = -1;
		}
	}

	for (int i = 0; i < TLBEntriesAmount; i++)
		_TLB->segmentNumber[i] = _TLB->frameNumber[i] = _TLB->pageNumber[i] = _TLB->FIFO[i] = -1;

	for (int i = 0; i < SegmentsAmount; i++)
		_memory->availableSegmentation[i] = -1;
	
	for (int i = 0; i < SegmentsAmount*FramesAmount; i++)
		_memory->available[i] = 1;

	_statistics->TranslatedAddressesCounter = 0;
	_statistics->SegmentationFaultsCounter = 0;
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

	free(_descriptorTable);
	free(_memory);
	free(_TLB);
}

/**
*     Managing Page Table methods
*/

// Finding Requested Page on Page Table
int findPageOnPageTable(int segmentNumber, int pageNumber)
{
	//Return -1 if page is not present (Page Fault)
	return _descriptorTable[segmentNumber].pageTable->frameNumber[pageNumber];
}

// Setting Used Page on Page Table
void setPageOnPageTable(int segmentNumber, int pageNumber, int frameNumber)
{
	_descriptorTable[segmentNumber].pageTable->frameNumber[pageNumber] = frameNumber;
}

/**
*     Managing TLB methods
*/

// Update TLB FIFO
int updateTLBFIFO()
{
	int slot = 0;

	//There is a free slot on TLB
	if (_TLB->FIFO[0] == -1) {

		//Move the FIFO
		for (int i = 0; i < TLBEntriesAmount - 1; i++)
			_TLB->FIFO[i] = _TLB->FIFO[i + 1];

		//Seeks for free slot on the TLB
		for (int i = 0; i < TLBEntriesAmount; i++) {
			if (_TLB->pageNumber[i] == -1) {
				slot = i;
				break;
			}
		}
	}

	//There is not a free slot on TLB
	else {
		//Picks the first slot on TLB
		slot = _TLB->FIFO[0];

		for (int i = 0; i < TLBEntriesAmount - 1; i++)
			_TLB->FIFO[i] = _TLB->FIFO[i + 1];
	}

	_TLB->FIFO[TLBEntriesAmount - 1] = slot;
	return slot;
}

// Finding Requested Page on TLB
int findPageOnTLB(int segmentNumber, int pageNumber)
{
	for (int i = 0; i < TLBEntriesAmount; i++) {
		// Find segment number on TLB
		if (_TLB->segmentNumber[i] == segmentNumber)
			// Find page number on TLB
			if (_TLB->pageNumber[i] == pageNumber) {
				_statistics->TLBHitsCounter++;
				//Requested Page is found on TLB
				return _TLB->frameNumber[i];
		}
	}

	// Requested Page not found on TLB.
	return -1;
}

// Setting Used Page on TLB
void setPageOnTLB(int segmentNumber, int pageNumber, int frameNumber)
{
	int newTLBindex = updateTLBFIFO();

	_TLB->segmentNumber[newTLBindex] = segmentNumber;
	_TLB->frameNumber[newTLBindex] = frameNumber;
	_TLB->pageNumber[newTLBindex] = pageNumber;
}

/**
*     Managing Memory methods
*/

// Find Oldest Frame on memory (first element on FIFO)
int findOldestFrameOnMemory(int segmentNumber, int pageNumber)
{
	//The oldest page on the queue
	int switchedpage = _descriptorTable[segmentNumber].pageTable->FIFO[0];

	//Sets the switched page as unavailable
	int slot = _descriptorTable[segmentNumber].pageTable->frameNumber[switchedpage];
	_descriptorTable[segmentNumber].pageTable->frameNumber[switchedpage] = -1;

	//Updates the
	for (int i = 0; i < FramesAmount - 1; i++) {
		_descriptorTable[segmentNumber].pageTable->FIFO[i] = _descriptorTable[segmentNumber].pageTable->FIFO[i + 1];
	}

	_descriptorTable[segmentNumber].pageTable->FIFO[FramesAmount - 1] = pageNumber;
	return slot;
}


//Updates values of Page Table FIFO
void updatePageTableFIFO(int segmentNumber, int pageNumber) {

	for (int i = 0; i < FramesAmount - 1; i++) {
		_descriptorTable[segmentNumber].pageTable->FIFO[i] = _descriptorTable[segmentNumber].pageTable->FIFO[i + 1];
	}

	//Puts the most recent page as the last of queue
	_descriptorTable[segmentNumber].pageTable->FIFO[FramesAmount - 1] = pageNumber;
}

// Find Available Frame on memory
int findAvailableFrameOnMemory(int segmentNumber, int pageNumber)
{
	int memIndex = segmentNumber*FramesAmount;
	for (int i = memIndex; i < memIndex + FramesAmount; i++) {
		if (_memory->available[i] == 1) {
			_memory->available[i] = 0;
			updatePageTableFIFO(segmentNumber, pageNumber);
			//There is available memory
			return i;
		}
	}

	//There is not available memory
	return -1;
}

// Find Segmentation Slot on Memory
int findSegmentationSlotOnMemory(int segmentNumber) 
{	
	// Memory Segmentation Slot
	int segmentationSlot = segmentNumber%SegmentsAmount;
	
	// Segmentation slot already allocated by requested segmentNumber
	if (_memory->availableSegmentation[segmentationSlot] == segmentNumber) {
		// Does nothing
	}
	// Segmentation slot available on memory for requested segmentNumber
	else if (_memory->availableSegmentation[segmentationSlot] == -1) {
		// Segmentation Fault
		_statistics->SegmentationFaultsCounter++;
		_memory->availableSegmentation[segmentationSlot] = segmentNumber;
	}
	// Segmentation slot already used by ANOTHER segmentNumber
	else {
			// Overwrite previous Segmentation allocated on memory
			//   NOT IMPLEMENTED IN THIS PROJECT
			printf("DEATH");
			// Segmentation Fault
			_statistics->SegmentationFaultsCounter++;
			_memory->availableSegmentation[segmentationSlot] = segmentNumber;
	}
	
	// Return Available Segmentation Slot on memory
	return segmentationSlot;
}

// Find Frame on memory
int findFrameOnMemory(int segmentNumber, int pageNumber)
{
	int segmentationSlot = findSegmentationSlotOnMemory(segmentNumber);
	int chosenFrame = findAvailableFrameOnMemory(segmentationSlot, pageNumber);

	if (chosenFrame == -1) {
		chosenFrame = findOldestFrameOnMemory(segmentationSlot, pageNumber);
	}
	return chosenFrame;
}

/**
*  Managing Backing Store methods
*/
// Manage Backing_Store
void getBackingStorePage(int segmentNumber, int pageNumber, int frameNumber)
{
	_statistics->PageFaultsCounter++;
	fseek(backingStore, pageNumber * FrameBytesSize, SEEK_SET);
	fread(_memory->frame[frameNumber].PageContent, FrameBytesSize, 1, backingStore);
}

/**
*     Debug application methods
*/
// Debug TLB
void debugTLB()
{
	printf("nTLBf[");
	for (int i = 0; i < TLBEntriesAmount; i++)
		printf("%d-%3d ", _TLB->segmentNumber[i], _TLB->frameNumber[i]);
	printf("]n");
}

// Debug Page Address
void debugPageAddress(int address, int segmentNumber, int pageNumber, int offset)
{
	printf ("Virtual Address: %5d ", address);
	printf ("SegmentNumber : %d ", segmentNumber);
	printf ("PageNumber : %3d ", pageNumber);
	printf ("PageOffset : %3d", offset);
	getchar();
}

// Debug Real Address
void debugFrameAddress(int address, int segmentNumber, int frameNumber, int offset)
{
	printf ("  Real  Address: %5d ", address);
	printf ("SegmentNumber : %d ", segmentNumber);
	printf ("FrameNumber: %3d ", frameNumber);
	printf ("FrameOffset: %3d", offset);
	getchar();
}

/**
*     Find Frame Number
*/
int findFrameNumber(int segmentNumber, int pageNumber)
{
	// Find frameNumber on TLB
	int frameNumber = findPageOnTLB(segmentNumber, pageNumber);

	// If TLB find fails
	if (frameNumber == -1)
	{
		// Find frameNumber on Page Table
		frameNumber = findPageOnPageTable(segmentNumber, pageNumber);

		// If Page Fault
		if (frameNumber == -1)
		{
			// Load on memory entire page of BACKING_STORE
			frameNumber = findFrameOnMemory(segmentNumber, pageNumber);
			getBackingStorePage(segmentNumber, pageNumber, frameNumber);

			// Set up accessed page on Page Table
			setPageOnPageTable(segmentNumber, pageNumber, frameNumber);
		}
		// Set up accessed page on TLB
		setPageOnTLB(segmentNumber, pageNumber, frameNumber);
	}
	return frameNumber;
}

/**
*     Main Memory Manager
*/
int main(int arc, char** argv)
{
	if (arc == 1)		initialize(inputfile_default);
	else				initialize(argv[1]);

	while (fgets(line , MaxStringLength, addresses))
	{
		// Read new virtual Address
		int virtualAddress = atoi(line);
		int segmentNumber = (virtualAddress / PageBits / OffsetBits) & (SegmentBits - 1);
		int pageNumber =  (virtualAddress / OffsetBits) & (PageBits - 1);
		int offset = virtualAddress & (OffsetBits - 1);

		// Segmentation Number consideration (only 4 segmentations)
		segmentNumber = _statistics->TranslatedAddressesCounter % SegmentsAmount;

		// Find frameNumber
		int frameNumber = findFrameNumber(segmentNumber, pageNumber);
		
		// Parse real Address
		int memIndex = frameNumber + segmentNumber*FramesAmount;
		int value = _memory->frame[memIndex].PageContent[offset];
		int realAddress = frameNumber * PagesAmount + offset;
	  writeOut(segmentNumber, virtualAddress, realAddress, value);

		// Debuggind PageAddress and FrameAddress
		//debugPageAddress(virtualAddress, segmentNumber, pageNumber, offset);
		//debugFrameAddress(realAddress, segmentNumber, frameNumber, offset);
	}
	finalize();
	return 0;
}