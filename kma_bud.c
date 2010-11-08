
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
#define ALIGNMENT 4

// rounding function for a multiple of alignment
#define ALIGNUP(size) (((size) + (ALIGNMENT - 1) & ~0x7)

#define SIZE_T_SIZE (ALIGNUP(sizeof(size_t)))

/* header and footer field size */
#define HEADSIZE SIZE_T_SIZE
#define FOOTSIZE SIZE_T_SIZE

// block size = f(ptr->header)
#define SIZE(ptr) (*(size_t *)ptr)
#define USIZE(ptr) (SIZE(ptr) & ~0x1) // masksfree bit

#define FREE_TEST(ptr) (SIZE(ptr) & 0x1)
// tests whether the block is free

#define INC_PTR(ptr, n) ((void *) ((char *) ptr + n))

// footer = f(header)
#define FOOT(ptr) INC_PTR(ptr, (HEADSIZE + USIZE(ptr)))

// reading a free list's link
#define HEADLINK(ptr) ((void *)SIZE(INC_PTR(ptr, 4)))
#define FOOTLINK(ptr) ((void *)SIZE(INC_PTR(FOOT(ptr),4)))

// writing a free list's link
#define SET_HEADLINK(ptr1, ptr2) (SIZE(INC_PTR(ptr1,4)) = (size_t)ptr2)
#define SET_FOOTLINK(ptr1,ptr2) (SIZE(INC_PTR(FOOT(ptr1),4)) = (size_t)ptr2)

#define MINPOWER 4 // gives 16 as the size of the smallest buffer
#define MAXBUFSIZE 4096 // assuming that a page is 4 kilobytes...
#define PGSIZE 4096

#define BUFNO 6

#define PGROUNDUP(sz) (((sz)+PGSIZE-1) & ~(PGSIZE-1))
 
typedef struct fl {
	void* ptr;
	struct fl* next;
} freelist;

// this is a struct for the headers of the free lists
// because it makes it easier for me to read the code
typedef struct flhead {
	kma_size_t rnd_sz;
	struct fl* start;
} fl_header;
/************Global Variables*********************************************/
fl_header* freelistlist[BUFNO];
int bmap[MAXBUFSIZE];
static int init;
/************Function Prototypes******************************************/
void add_fl(void *ptr); // add an element to a freelist
void* rm_fl(void *ptr); // remove an element from the freelist it is in
void split_page(kpage_t *page, kma_size_t size);
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
				result = split_block(freelistlist[ndx], freelistlist[ndx]->rnd_sz/2);
				break;
			}
		}
		
		// no larger freelists either
		kpage_t *pageptr = get_page();
		split_page(pageptr, size);
		
		result = rm_fl(freelistlist[ndx]); // have to remove one to satisfy the malloc request
	}
	return result;
}

void 
kma_free(void* ptr, kma_size_t size)
{
  ;
}

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

void* split_block(void *ptr, int sz) {
	void *tmp = (freelist *)rm_fl(ptr);
	void *tmp_bud = *(&tmp + sz);
	add_fl(tmp);
	add_fl(tmp_bud);
	return tmp;
}

void add_fl(void *ptr) {
	
	freelist *tmp = (freelist *)ptr;
	tmp->next = ptr;

	ptr = tmp;
	return;	
}

void* rm_fl(void *ptr) {
	freelist *tmp = (freelist *)ptr;
	tmp = tmp->next;
	ptr = tmp;
	return *(&ptr + sizeof(freelist));
}

#endif // KMA_BUD
