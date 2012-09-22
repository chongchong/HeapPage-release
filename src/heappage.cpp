#include <iostream>
#include <stdlib.h>
#include <memory.h>

#include "heappage.h"
#include "heapfile.h"
#include "bufmgr.h"
#include "db.h"

using namespace std;

//------------------------------------------------------------------
// Constructor of HeapPage
//
// Input     : Page ID
// Output    : None
//------------------------------------------------------------------
void HeapPage::Init(PageID pageNo)
{															// wc373
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
	return (Slot *)&data[slotNumber * sizeof(Slot)];		// wc373	NEED TO CHECK WITH AppendNewSlot
	// or, (Slot *)(data + slotNumber * sizeof(Slot));
}

//------------------------------------------------------------------
// HeapPage::GetContiguousFreeSpaceSize
//
// Input    : None.
// Output   : None.
// Return   : The size of the contiguous free space region.
//------------------------------------------------------------------
int HeapPage::GetContiguousFreeSpaceSize() {				// wc373
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
HeapPage::Slot* HeapPage::AppendNewSlot(){					// wc373	NEED MORE THOUGHTS
	if (GetContiguousFreeSpaceSize() >= sizeof(Slot)) {
		freeSpace -= sizeof(Slot);
		return GetSlotAtIndex(numOfSlots++);
	}
	cout << "not enough space for new slot" << endl;
	return NULL;
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
	// Sort indices of all slots by descending offsets (end to beginning)
	// Access slots by sorted indices one by one to arrange space
	// Reset freePtr
	cout << "will CompressPage" << endl;
	vector<short> index;
	for (short i=0;i<numOfSlots;i++) { // create a vector of slot indices
		index[i]=i;
	}
	MergeSort(index); // indices have been sorted
	freePtr = HEAPPAGE_DATA_SIZE - 1; // reset freePtr to the end
	for (short k=0;k<numOfSlots;k++) { // rearrange all records
		Slot *slot = GetSlotAtIndex(index[k]);
		short recordLength = slot->length;
		if (recordLength != -1) {
			freePtr -= recordLength;
			if (memcpy(data+freePtr+1,data+slot->offset,recordLength) != data+freePtr+1) return FAIL;
		}
	}
	return FAIL;
}


//------------------------------------------------------------------
// HeapPage::getEmptySlot
//
// Input     : a short number which represents the index of an empty slot
// Output    : the index of the first empty slot found
// Purpose   : find the first empty slot 
// Return    : the first empty slot found
//------------------------------------------------------------------
HeapPage::Slot* HeapPage::getEmptySlot(short& index){
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
Status HeapPage::InsertRecord(const char *recPtr, int length, RecordID& rid)		//cw474
{					
	if (length<=0) return FAIL;
	if (AvailableSpace()>=length) {
		short index;
		Slot* newslot=getEmptySlot(index);
		if (newslot == NULL){
			newslot = AppendNewSlot();
		}
		if (newslot == NULL) return DONE;
		if (freePtr-length<0) {
			Status cs=CompressPage();
			if (cs==FAIL) return FAIL;
		}
		freePtr = freePtr-length;
		freeSpace= freeSpace-length;
		FillSlot(newslot,freePtr+1,length);
		if (memcpy(data + freePtr+1,recPtr,length) != data+freePtr+1) return FAIL;
		rid.slotNo=index;
		rid.pageNo=pid;
		return OK;
		

	} 
	else return DONE;
}

//------------------------------------------------------------------
//a helper function for DeleteRecord
// go through all the slots before this slot
// return true if all the slots before are invalid
// return false otherwise
//------------------------------------------------------------------
bool HeapPage::hasNoOtherValidSlot(int slotNO)
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
Status HeapPage::DeleteRecord(RecordID rid)   //cw474
{
	if ((rid.pageNo!=pid) || (rid.slotNo>numOfSlots-1))return FAIL;		// are there other cases that rid may be invalid?
	Slot *slot =GetSlotAtIndex(rid.slotNo); 
	if (slot->length==INVALID_SLOT) return FAIL;
	if (hasNoOtherValidSlot(rid.slotNo)) { // if it's the only valid slot left
		/*for (int i=0;i<numOfSlots;i++){
			delete 	GetSlotAtIndex(i);
		}	*/	// not sure if we need to reserve a new slot like when we create a new arrray
		freeSpace =  freeSpace + numOfSlots* sizeof(Slot)+slot->length;
		numOfSlots=0;
	}
	else 
		if (rid.slotNo==numOfSlots-1) { // if it's the last slot
		//delete slot;
		numOfSlots--;
		freeSpace = freeSpace + sizeof(Slot)+slot->length;
		}
		else {   // if it's just a normal slot
			freeSpace=freeSpace+slot->length;
			slot ->length= INVALID_SLOT;
		}
	


	return OK;
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
{															// wc373
	//cout << "in FirstRecord" << endl;
	//if (&rid == NULL) return FAIL;
	for (short i=0; i<numOfSlots; i++) {
		if (GetSlotAtIndex(i)->length != -1) {
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
{															// wc373
	//cout << "in NextRecord" << endl;
	if (curRid.pageNo != pid || curRid.slotNo < 0 || curRid.slotNo >= numOfSlots) return FAIL;
	for (short i=curRid.slotNo+1; i<numOfSlots; i++) {
		if (GetSlotAtIndex(i)->length != -1){
			nextRid.pageNo = pid;
			nextRid.slotNo = i;
			return OK;
		}
	}
	//cout << "no more records exist on the page" << endl;
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
{															// wc373
	//cout << "in GetRecord" << endl;
	if (rid.pageNo != pid) return FAIL;
	Slot *slot = GetSlotAtIndex(rid.slotNo);
	short slotLength = slot->length;
	short slotOffset = slot->offset;
	if (length >= slotLength) {
		if (memcpy(recPtr, data+slotOffset, slotLength) == recPtr) {
			length = slotLength;
			return OK;
		} 
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
{															// wc373	
	cout << "in ReturnRecord" << endl;
	if (rid.pageNo != pid) return FAIL;
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
{															// wc373
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
{															// wc373
	for (int i=0; i<numOfSlots; i++) {
		if (GetSlotAtIndex(i)->length != -1) return false;
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
{															// wc373
	int num = 0;
	for (int i=0; i<numOfSlots; i++) {
		if (GetSlotAtIndex(i)->length != -1) num++;
	}
	return num;
}
