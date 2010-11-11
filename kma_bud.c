 /***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_bud.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_bud.c,v $
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <math.h>

/************Private include**********************************************/
#include "kpage.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */
/* single word = 4, double word = 8 alignment */
#define MINPOWER 5 // gives 16 as the size of the smallest buffer
#define MAXBUFSIZE 8192 // assuming that a page is 4 kilobytes...
#define PGSIZE 8192
#define MAXPGNUM 20
#define BUFNO 8 

#define PGROUNDUP(sz) (((sz)+PGSIZE-1) & ~(PGSIZE-1))


typedef struct fl {
	kma_size_t rnd_sz;
	int exp;
	struct fl* next;
	void *buddy;
	int free;
} freelist;

typedef struct flhead {
	kma_size_t rnd_sz;
	void* start;
} fl_header;

/************Global Variables*********************************************/
int fl_count[BUFNO];
//void* pageaddresses[MAXPGNUM];
static int init;
int pgno;

fl_header* freelistlist[BUFNO];
int bmap[MAXBUFSIZE];
static int init;
/************Function Prototypes******************************************/
void add_fl(void *ptr, int bufsize); // add an element to a freelist
void add_fl_buddies(void *ptr1, void *ptr2, int bufsize);
void* rm_fl(void *ptr); // remove an element from the freelist it is in
void split_page(kpage_t *page, kma_size_t size);
void split_block(void *ptr, int sz);
void coalesce_block(void *ptr);
static int kma_init();
/************External Declaration*****************************************/

/**************Implementation***********************************************/

static int kma_init(void)
{
	int i,divby;

	for(i = 0; i < BUFNO; i++) {
		divby = pow(2, i);
		freelistlist[i]->rnd_sz = PAGESIZE / divby;
		freelistlist[i]->start = 0;
	}
	init = 1;
	return 1;
}

void*
kma_malloc(kma_size_t size)
{
  	if (!init && !kma_init())
		return NULL; // initialization error
	void* result;
	int ndx = BUFNO - 1; // index for free list
	int bufsize = 1 << MINPOWER; // smallest buffer size
	size += sizeof(freelist); // account for the header

	if (size > MAXBUFSIZE) return NULL; // malloc size request is larger than a page

	// round up loop, inefficient
	while (bufsize < size) {
		ndx--;
		bufsize <<= 1;
	}

	// after rounding up, ndx points to the correct free list
	if(freelistlist[ndx]->start != 0) // if there is a freelist of that size
		result = rm_fl(freelistlist[ndx]); // remove the freelist from the list and relinkify the rest of the freelist nodes
	else { // otherwise, there are no freelists of that size
		int foundsplittable = 0;
		int splitwhere = BUFNO;
		
		// see if there are any larger spaces open to split
		while (--ndx >= 0) {
			if (freelistlist[ndx]->start != 0) {
				foundsplittable = 1;
				splitwhere = ndx;
				result = split_block(freelistlist[ndx]->start, freelistlist[ndx]->rnd_sz/2);
				break;
			}
		}
		
		// no larger freelists either
		kpage_t *pageptr = get_page();
		split_block(pageptr, size);
		
		result = rm_fl(freelistlist[ndx]); // have to remove one to satisfy the malloc request
	}
	return result;
}

void 
kma_free(void* ptr, kma_size_t size)
{
  ptr = (freelist*) ptr;
  int i;
  for (i = 0; i < BUFNO; i++) {
  	if (freelistlist[i]->rnd_sz == size && freelistlist[i]->ptr != 0) {
  		//there's a freelist of the size of our block, and it has at least one other free list node in it
  		// I don't remember where I was going with this
  	}
  }
  
}

// not entirely sure we need this anymore—replaced by split_block.
// we can just call split_block on a block size of PAGESIZE I think?
void split_page(kpage_t *page, kma_size_t size)
{
	// current size of split
	int cur_size = PAGESIZE / 2;
	int i;
	freelist *tmp;
	freelist *tmp_bud;
	// set up initial ptrs for the buddies
	tmp->ptr = page;
	tmp_bud->ptr = *(&tmp + cur_size);
	void* ret;
	// until we get to the size we need:
	while(cur_size >= size) {
		// decrement size of chunk
		cur_size = cur_size / 2;
		// remove last iteration's buddy from its list
		ret = rm_fl(tmp_bud);
		// create new ptr at its beginning
		tmp->ptr = ret;
		// create buddy at tmp's addr + the current block size
		tmp_bud->ptr = *(&tmp + cur_size);
		for (i = 0; i < BUFNO; i++) {
			// check if the current list is the right size
			if (freelistlist[i]->rnd_sz == cur_size) {
				// if so, add ptr and buddy to it
				tmp->next = freelistlist[i]->start;
				freelistlist[i]->start = tmp;
				tmp_bud->next = freelistlist[i]->start;
				freelistlist[i]->start = tmp_bud;
				// found it, so we're done here
				break;
			}
		}
	}
}

// issue: will coalesce blocks to their old buddies, but what about if they were split multiple times?
// the previous buddy will still have the ptr to this one, but this one won't have its old buddy
// (amirite?)
void coalesce_blocks(void *ptr, int sz) {
	freelist tmp;
	tmp = (freelist*) ptr;
	if (tmp->buddy->free == 1) {
		void* result;
		result = rm_fl(ptr);
		add_fl(tmp->buddy->ptr, pow(2, tmp->exp + 1));
	}
	return;
}

// gets block, halfway into block
// then adds them both and makes them buddies
// and returns the first one
void* split_block(void *ptr, int sz) {
	void *tmp = rm_fl(ptr);
	void *tmp_bud = *(&tmp + sz);
	add_fl_buddies(tmp, tmp_bud);
	return tmp;
}

// regular add_fl for single adds (e.g. re-adding free'd blocks whose buddy is still in use)
void add_fl(void *ptr, int bufsize) { // ptr is a pointer to the beginning of the freelist
	// tmp is a new element in the free list that we are adding
	freelist tmp; // need to figure out WHERE this is pointing to
	freelist *tmp2;
	tmp.next = (struct fl* ) ((char *) ptr + bufsize); // we assign the next pointer of our next element to the beginning of the freelist
	tmp2 = (freelist *) ptr; // the new beginning of the free list is tmp
	tmp = *tmp2;
	return;	
}

void add_fl_buddies(void *ptr1, void *ptr2 int bufsize) { // ptr is a pointer to the beginning of the void 
	// tmp is a new element in the free list that we are adding
	freelist tmp; // need to figure out WHERE this is pointing to
	freelist *tmp_tmp;
	freelist tmp2;
	freelist *tmp2_tmp;
	tmp.next = (struct fl* ) ((char *) ptr1 + bufsize); // we assign the next pointer of our next element to the beginning of the freelist
	tmp_tmp = (freelist *) ptr1; // the new beginning of the free list is tmp
	tmp = *tmp_tmp;
	tmp2.next = (struct fl*) ((char *) ptr2 + bufsize);
	tmp2_tmp = (freelist *) ptr2;
	tmp2 = *tmp2_tmp;
	tmp.buddy = tmp2;
	tmp2.buddy = tmp;
	tmp.free = 1;
	tmp2.free = 1;
	return;	
}

void* rm_fl(void *ptr) { 
	freelist tmp;
	freelist *tmp2;
	tmp2 = (freelist *) ptr;
	tmp = *tmp2;
	tmp = *(tmp.next);
	coalesce_blocks(ptr);
//	ptr = (void *) &tmp;
	return ((char *)tmp2 + sizeof(freelist));
}

#endif // KMA_BUD
