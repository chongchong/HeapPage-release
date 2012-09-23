#include <iostream>
#include <stdlib.h>
#include <memory.h>

#include "heappage.h"
#include "heapfile.h"
#include "bufmgr.h"
#include "db.h"

//------------------------------------------------------------------
// Constructor of HeapPage
//
// Input     : Page ID
// Output    : None
//------------------------------------------------------------------
void HeapPage::Init(PageID pageNo)
{								
	pid = pageNo;
	nextPage = INVALID_PAGE;
	prevPage = INVALID_PAGE;
	numOfSlots = 0;
	freePtr = (short)HEAPPAGE_DATA_SIZE-1;
	freeSpace = (short)HEAPPAGE_DATA_SIZE;
}

//------------------------------------------------------------------
// HeapPage::SetNextPage
//
// Input    : The PageID for next page.
// Output   : None.
// Purpose  : Set the PageID for next page.
// Return   : None.
//------------------------------------------------------------------
void HeapPage::SetNextPage(PageID pageNo)
{
	nextPage = pageNo;
}

//------------------------------------------------------------------
// HeapPage::SetNextPage
//
// Input    : The PageID for previous page.
// Output   : None.
// Purpose  : Set the PageID for previous page.
// Return   : None.
//------------------------------------------------------------------
void HeapPage::SetPrevPage(PageID pageNo)
{
	prevPage = pageNo;
}

//------------------------------------------------------------------
// HeapPage::GetNextPage
//
// Input    : None.
// Output   : None.
// Purpose  : Get the PageID of next page.
// Return   : The PageID of next page.
//------------------------------------------------------------------
PageID HeapPage::GetNextPage()
{
	return nextPage;
}

//------------------------------------------------------------------
// HeapPage::SetPrevPage
//
// Input    : The PageID for previous page.
// Output   : None.
// Purpose  : Get the PageID of previous page.
// Return   : The PageID of previous page.
//------------------------------------------------------------------
PageID HeapPage::GetPrevPage()
{
	return prevPage;
}

//------------------------------------------------------------------
// HeapPage::PageNo
//
// Input    : None.
// Output   : None.
// Purpose  : Get the PageID of this page.
// Return   : The PageID of this page.
//------------------------------------------------------------------
PageID HeapPage::PageNo() 
{
	return pid;
}

//------------------------------------------------------------------
// HeapPage::GetSlotAtIndex
//
// Input    : the slot number.
// Output   : None.
// Purpose  : Get the pointer to the slot at the given slot number in the slot directory.
// Return   : The pointer to the requested slot.
//------------------------------------------------------------------
HeapPage::Slot* HeapPage::GetSlotAtIndex(int slotNumber) {
	return (Slot *)&data[slotNumber * sizeof(Slot)];
}

//------------------------------------------------------------------
// HeapPage::GetContiguousFreeSpaceSize
//
// Input    : None.
// Output   : None.
// Return   : The size of the contiguous free space region.
//------------------------------------------------------------------
int HeapPage::GetContiguousFreeSpaceSize() {	
	return freePtr - numOfSlots * sizeof(Slot) + 1;
}

//------------------------------------------------------------------
// HeapPage::AppendNewSlot
//
// Input    : None.
// Output   : None.
// Purpose  : Increase the size of the slot directory by appending a new slot at the end.
// Return   : A pointer to the slot appended by the function.
//------------------------------------------------------------------
HeapPage::Slot* HeapPage::AppendNewSlot(){		
	if (GetContiguousFreeSpaceSize() >= sizeof(Slot)) {
		freeSpace -= sizeof(Slot);
		return GetSlotAtIndex(numOfSlots++);
	}
	return NULL; // not enough space
}

//------------------------------------------------------------------
// HeapPage::CompressPage
//
// Input     : None 
// Output    : The page after compression.
// Purpose   : To reclaim the free space in the page left as holes after deletion.
// Return    : OK if everything went OK, FAIL otherwise.
//------------------------------------------------------------------
Status HeapPage::CompressPage() {	
	// First, sort tuples of <index, offset> of all slots by descending offsets (end to beginning)
	vector<tuple<short, short>> indexAndOffset;
	for (short i=0;i<numOfSlots;i++) { // create a vector of slot indices and offsets
		indexAndOffset.push_back(make_tuple(i,GetSlotAtIndex(i)->offset));
	}
	// We were told that using std::sort is allowed since it runs in O(nlogn) time on average according to the documentation
	std::sort(indexAndOffset.begin(), indexAndOffset.end(), &HeapPage::SortByDescendingOffset);
	freePtr = HEAPPAGE_DATA_SIZE - 1; // reset freePtr to the end
	for (short k=0;k<numOfSlots;k++) { // rearrange all records
		Slot *slot = GetSlotAtIndex(std::get<0>(indexAndOffset.at(k)));
		short recordLength = slot->length;
		if (recordLength != -1) {
			freePtr -= recordLength;
			short newOffset = freePtr+1;
			if (memcpy(data+newOffset,data+slot->offset,recordLength) != data+freePtr+1){
				return FAIL;
			}
			slot->offset = newOffset;
		}
	}
	return OK;
}

//------------------------------------------------------------------
// HeapPage::getEmptySlot
//
// Input     : a short number which represents the index of an empty slot
// Output    : the index of the first empty slot found
// Purpose   : find the first empty slot 
// Return    : the first empty slot found
//------------------------------------------------------------------
HeapPage::Slot* HeapPage::GetEmptySlot(short& index){
	short i=0;
	for(i=0;i<numOfSlots;i++){
		if (GetSlotAtIndex(i)->length ==-1) {
			index=i;
			return GetSlotAtIndex(i);
		}
	}
	index=i;
	return NULL;
}

//------------------------------------------------------------------
// HeapPage::InsertRecord
//
// Input     : Pointer to the record and the record's length 
// Output    : Record ID of the record inserted.
// Purpose   : Insert a record into the page
// Return    : OK if everything went OK, DONE if sufficient space is not available.
//------------------------------------------------------------------
Status HeapPage::InsertRecord(const char *recPtr, int length, RecordID& rid)
{					
	if (length<=0) return FAIL;
	if (AvailableSpace()>=length) { // there is enough space on the page for a new slot (if needed) and the record
		short index;
		Slot* newslot=GetEmptySlot(index);
		if (newslot == NULL){ // slot directory is full
			if (GetContiguousFreeSpaceSize()<sizeof(Slot)+length){ // not enough space for a slot and the record in the contiguous free space area
				Status cs=CompressPage();
				if (cs==FAIL) return FAIL;
			}
			newslot = AppendNewSlot();
		}
		//if (newslot == NULL) return DONE; 
		if (GetContiguousFreeSpaceSize()<length) { // slot directory not full, but not enough space for a record at the end
			Status cs=CompressPage();
			if (cs==FAIL) return FAIL;
		}
		freePtr = freePtr-length;
		freeSpace= freeSpace-length;
		FillSlot(newslot,freePtr+1,length);
		if (memcpy(data + freePtr+1,recPtr,length) != data+freePtr+1){
			return FAIL;
		}
		rid.slotNo=index;
		rid.pageNo=pid;
		return OK;
	} 
	else return DONE;
}

//------------------------------------------------------------------
// HeapPage::HasNoOtherValidSlot
//
// Input    : Index of the slot that's checked against
// Output   : If all slots before it are invalid, false otherwise
// Purpose  : A helper function for DeleteRecord
// Return   : True if all slots before it are invalid, false otherwise
//------------------------------------------------------------------
bool HeapPage::HasNoOtherValidSlot(int slotNO)
{
	for (int i=0;i<numOfSlots;i++){
		if ((i!=slotNO)&&(!SlotIsEmpty(GetSlotAtIndex(i)))) return false;
	}
	return true;
}

//------------------------------------------------------------------
// HeapPage::DeleteRecord 
//
// Input    : Record ID
// Output   : None
// Purpose  : Delete a record from the page
// Return   : OK if successful, FAIL otherwise  
//------------------------------------------------------------------ 
Status HeapPage::DeleteRecord(RecordID rid)
{
	if ((rid.pageNo!=pid) || (rid.slotNo>numOfSlots-1))return FAIL;	
	Slot *slot =GetSlotAtIndex(rid.slotNo); 
	if (slot->length==INVALID_SLOT) return FAIL;
	if (HasNoOtherValidSlot(rid.slotNo)) { // if it's the only valid slot left
		freeSpace =  freeSpace + numOfSlots* sizeof(Slot)+slot->length;
		freePtr = HEAPPAGE_DATA_SIZE - 1; // reset freePtr
		numOfSlots=0;
	}
	else {
		if (rid.slotNo==numOfSlots-1) { // if it's the last slot
			//delete slot;
			numOfSlots--;
			freeSpace = freeSpace + sizeof(Slot)+slot->length;
		}
		else {   // if it's just a normal slot
			freeSpace=freeSpace+slot->length;
			slot ->length= INVALID_SLOT;
		}
		if (SmallestOffset() == GetSlotAtIndex(rid.slotNo)->offset) freePtr += slot->length;
	}
	return OK;
}

//------------------------------------------------------------------
// HeapPage::SmallestOffset
//
// Input    : None
// Output   : Offset of the last (closest to slot directory) record in the data region
// Purpose  : A helper function for Delete to determine if the record to be deleted is the last one
// Return   : The offset of last record 
//------------------------------------------------------------------ 
short HeapPage::SmallestOffset() {
	short smallest = HEAPPAGE_DATA_SIZE;
	for (int i=0;i<numOfSlots;i++) {
		Slot *slot = GetSlotAtIndex(i);
		if (slot->length != INVALID_SLOT) {
			if (slot->offset < smallest) smallest = slot->offset;
		}
	}
	return smallest;
}

//------------------------------------------------------------------
// HeapPage::FirstRecord
//
// Input    : None
// Output   : record id of the first record on a page
// Purpose  : To find the first record on a page
// Return   : OK if successful, DONE otherwise
//------------------------------------------------------------------
Status HeapPage::FirstRecord(RecordID& rid)
{
	for (short i=0; i<numOfSlots; i++) {
		if (!SlotIsEmpty(GetSlotAtIndex(i))) {
			rid.pageNo = pid;
			rid.slotNo = i;
			return OK;
		}
	}
	return DONE;
}

//------------------------------------------------------------------
// HeapPage::NextRecord
//
// Input    : ID of the current record
// Output   : ID of the next record
// Return   : Return DONE if no more records exist on the page; 
//            otherwise OK
//------------------------------------------------------------------
Status HeapPage::NextRecord (RecordID curRid, RecordID& nextRid)
{
	if (curRid.pageNo != pid || curRid.slotNo < 0 || curRid.slotNo >= numOfSlots) return FAIL;
	for (short i=curRid.slotNo+1; i<numOfSlots; i++) {
		if (!SlotIsEmpty(GetSlotAtIndex(i))){
			nextRid.pageNo = pid;
			nextRid.slotNo = i;
			return OK;
		}
	}
	return DONE;
}

//------------------------------------------------------------------
// HeapPage::GetRecord
//
// Input    : Record ID
// Output   : Records length and a copy of the record itself
// Purpose  : To retrieve a _copy_ of a record with ID rid from a page
// Return   : OK if successful, FAIL otherwise
//------------------------------------------------------------------
Status HeapPage::GetRecord(RecordID rid, char *recPtr, int& length)
{
	if (rid.pageNo != pid || rid.slotNo >= numOfSlots || rid.slotNo < 0) return FAIL;
	Slot *slot = GetSlotAtIndex(rid.slotNo);
	short slotLength = slot->length;
	short slotOffset = slot->offset;
	if (length >= slotLength) {
		if (memcpy(recPtr, data+slotOffset, slotLength) == recPtr) {
			length = slotLength;
			return OK;
		} 
		else free(recPtr);
	}
	return FAIL;
}

//------------------------------------------------------------------
// HeapPage::ReturnRecord
//
// Input    : Record ID
// Output   : pointer to the record, record's length
// Purpose  : To output a _pointer_ to the record
// Return   : OK if successful, FAIL otherwise
//------------------------------------------------------------------
Status HeapPage::ReturnRecord(RecordID rid, char*& recPtr, int& length)
{	
	if (rid.pageNo != pid || rid.slotNo >= numOfSlots || rid.slotNo < 0) return FAIL;
	Slot *slot = GetSlotAtIndex(rid.slotNo);
	if (slot->length != -1) {
		recPtr = &data[slot->offset];
		length = slot->length;			
		return OK;
	}
	return FAIL;
}

//------------------------------------------------------------------
// HeapPage::AvailableSpace
//
// Input    : None
// Output   : None
// Purpose  : To return the amount of available space
// Return   : The amount of available space on the heap file page for inserting a record.
//------------------------------------------------------------------
int HeapPage::AvailableSpace(void)
{											
	return (GetNumOfRecords()!=numOfSlots ? freeSpace : freeSpace - sizeof(Slot));
}

//------------------------------------------------------------------
// HeapPage::IsEmpty
//
// Input    : None
// Output   : None
// Purpose  : Check if there is any record in the page.
// Return   : true if the HeapPage is empty, and false otherwise.
//------------------------------------------------------------------
bool HeapPage::IsEmpty(void)
{							
	for (int i=0; i<numOfSlots; i++) {
		if (!SlotIsEmpty(GetSlotAtIndex(i))) return false;
	}
	return true;
}

//------------------------------------------------------------------
// HeapPage::GetNumOfRecords
//
// Input    : None
// Output   : None.
// Purpose  : To determine the number of records on the page (not necessarily equal to the number of slots).
// Return   : The number of records in the page.
//------------------------------------------------------------------
int HeapPage::GetNumOfRecords()
{								
	int num = 0;
	for (int i=0; i<numOfSlots; i++) {
		if (!SlotIsEmpty(GetSlotAtIndex(i))) num++;
	}
	return num;
}
