#ifndef HFPAGE_H
#define HFPAGE_H

#include "minirel.h"
#include "page.h"
#include <vector>

const int INVALID_SLOT = -1;

// Size of the data array in the class HeapPage
const int HEAPPAGE_DATA_SIZE = MAX_SPACE - 3 * sizeof(PageID) - 4 * sizeof(short);

// This class defines an object which corresponds to a heap page.
// The heap page manages records of variable length. 
class HeapPage {

protected :
	struct Slot 
	{
		short offset;	// Offset of record from the start of data area.
		short length;	// Length of the record.

	};

	short   numOfSlots;	// Number of slots available (maybe filled or empty).
	short   freePtr;	// Offset from start of data area, where begins the free space for adding new records.
	short   freeSpace;	// Amount of free space in bytes in this page.
	short   type;		// Not used for HeapFile assignment, but will be used in B+-tree assignment.

	PageID  pid;		// Page ID of this page  
	PageID  nextPage;	// Page ID of the next page.
	PageID  prevPage;	// Page ID of the prev page.

	// Data area for this page. Actual records grow from end towards the begining of a page. 
	char data[HEAPPAGE_DATA_SIZE];

	// Check if the slot is empty.
	bool SlotIsEmpty(Slot *slot) {
		return slot->length == INVALID_SLOT;
	}

	// Fill the slot with the given <offset, length> pair.
	void FillSlot(Slot *slot, int offset, int length) {
		slot->offset = offset;
		slot->length = length;
	}

	// Set the slot to be empty.
	void SetSlotEmpty(Slot *slot) {
		slot->length = INVALID_SLOT;
	}

	// Get the pointer to the slot at slot slotNumber in the slot directory.
	Slot* GetSlotAtIndex(int slotNumber);

	// A private method invoked to reclaim space lost in holes created due to deletion.
	// Only invoked when contiguous free space is not enough for a new record.
	Status CompressPage();

	// A private method to calculate the size of the contiguous free space region.
	int GetContiguousFreeSpaceSize();

	// Appends a new slot to the end of the slot directory.
	Slot* AppendNewSlot();

	// MergeSort
	void MergeSort(vector<short> &arr) {
		if (arr.size() < 2) return;
		else {
			int mid = arr.size()/2;
			vector<short>::const_iterator first = arr.begin(),
				half = arr.begin() + mid,
				half2 = arr.begin() + mid+1,
				end = arr.end();

			vector<short>left(first,half);
			vector<short>right(half2,end);

			MergeSort(left);
			MergeSort(right);
			arr = Merge(left,right);
		}
	}

	// Merge
	vector<short> Merge(vector<short> arr1, vector<short> arr2) {
		vector<short> result;
		short arr1Index = 0, arr2Index = 0, resultIndex = 0;
		short size = arr1.size() + arr2.size();
		while (resultIndex < size) {
			if (GetSlotAtIndex(arr1[arr1Index])->offset >= GetSlotAtIndex(arr2[arr2Index])->offset) result.push_back(arr1[arr1Index++]);
			else result.push_back(arr2[arr2Index++]);
			resultIndex++;
		}
		return result;
	}

public:
	// Inialize the page with given PageID.
	void Init(PageID pageNo);

	// Counts the number of records in the page.
	int GetNumOfRecords();

	// Get the PageID of next page.
	PageID GetNextPage();

	// Get the PageID of previous page.
	PageID GetPrevPage();

	// Set the PageID for next page.
	void SetNextPage(PageID pageNo);

	// Set the PageID for previous page.
	void SetPrevPage(PageID pageNo);

	// Get the PageID of this page.
	PageID PageNo();

	// Check if there is any empty slot
	Slot* getEmptySlot(short& index);

	// Insert a record into the page.
	Status InsertRecord(const char* recPtr, int recLen, RecordID& rid);
	
	//check if this is the last valid slot in the slot directory
	bool HeapPage::hasNoOtherValidSlot(int slotNO);

	// Delete a record from the page.
	Status DeleteRecord(RecordID rid);

	// To find the first record on a page.
	Status FirstRecord(RecordID& firstRid);

	// To find the next record on a page.
	Status NextRecord (RecordID curRid, RecordID& nextRid);

	// To retrieve a COPY of a record with ID rid from a page.
	Status GetRecord(RecordID rid, char* recPtr, int& len);

	// To retrieve a POINTER to the record.
	Status ReturnRecord(RecordID rid, char*& recPtr, int& len);

	// To return the amount of space available for inserting a new record.
	int AvailableSpace();

	// Check if there is any record in the page.
	bool IsEmpty();
};

#endif
