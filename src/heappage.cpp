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
	freePtr = (short)MAX_SPACE;
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
	return (Slot *)&data[slotNumber * sizeof(Slot)];		// wc373	NEED TO CHECK IN ADJACENT WITH AppendNewSlot
}

//------------------------------------------------------------------
// HeapPage::GetContiguousFreeSpaceSize
//
// Input    : None.
// Output   : None.
// Return   : The size of the contiguous free space region.
//------------------------------------------------------------------
int HeapPage::GetContiguousFreeSpaceSize() {				// wc373
	return freePtr - numOfSlots * sizeof(Slot);
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
		Slot *newSlot = new Slot((short)++numOfSlots, -1);
		*(data + numOfSlots * sizeof(Slot)) = *((char *)newSlot); // ????? NEED TO CONVERT FROM STRUCT TO CHAR
		return newSlot;
	}
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
	//TODO: add your code here
	return FAIL;
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
	//TODO: add your code here
	return FAIL;
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
	//TODO: add your code here
	return FAIL;
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
	//TODO: add your code here
	return FAIL;
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
	//TODO: add your code here
	return FAIL;
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
	//TODO: add your code here
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
	//TODO: add your code here
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
