/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
 *             algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_p2fl.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_p2fl.c,v $
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
#ifdef KMA_P2FL
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>

/************Private include**********************************************/
#include "kpage.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
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

#define FREE_TEST(ptr) (SIZE(ptr) & 0x1) // tests whether the block is free

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

//#define MAXPAGESIZE 4096

typedef struct fl {
	kma_size_t sz;
	struct fl* next;
} freelist;

/*
struct fl {
	size_t sz;
	fl *next;
};
*/

/************Global Variables*********************************************/

freelist freelistlist[BUFNO];
//typedef union fl freelist;
static int init;

/************Function Prototypes******************************************/

void add_fl(freelist *ptr); // add an element to a freelist
freelist* rm_fl(freelist *ptr); // remove an element from the freelist it is in
static int kma_init(void);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	if (!init && !kma_init())
		return NULL; // initialization error
	void* result;
	int ndx = 0; // index for free list
	int bufsize = 1 << MINPOWER; // smallest buffer size
	size += sizeof(freelist); // account for the header

	if (size > MAXBUFSIZE) return NULL; // malloc size request is larger than a page

	// round up loop, inefficient
	while (bufsize < size) {
		ndx++;
		bufsize <<= 1;
	}

	// after rounding up, ndx points to the correct free list
	if(freelistlist[ndx].next != 0) // if there is a freelist of that size
	//	result = (rm_fl(freelistlist[ndx].next)); // remove the freelist and relinkify the rest of the freelist nodes
	return;
	else { // otherwise, there are no freelists of that size
		kpage_t* pagein = get_page(); // we get a page
		int i;
		for(i = 0; i < (PGSIZE / bufsize); i++) {
			// find out how many of these rounded up pieces can fit in a page
			// iterate and add that many free lists to the freelistlist of that size
		add_fl(freelistlist[ndx].next);
		}
		// guaranteed to have n - 1 freelists in the freelistlist!
		//result = (rm_fl(freelistlist[ndx].next)); // have to remove one to satisfy the malloc request
	}
	//return result;
}


void
kma_free(void* ptr, kma_size_t size)
{
  int ndx = 0;
	int bufsize = 1 << MINPOWER;
	size += sizeof(freelist); // size is the size of the memory allocation as seen by the requester, need to account for size that header adds in our alloc
	if (size > MAXBUFSIZE) return; // just in case a free to something larger than a page is called for its corresponding malloc

	while(bufsize < size) { // bad round up loop, bad!
		ndx++;
		bufsize <<= 1;
	}

	add_fl(freelistlist[ndx].next); 
	// note that there is no actual way to return pages to the system...
}

// initialize the page request by breaking up the chunk and mapping it to the free list
static int kma_init(void) {
	int i,bufsize;
	bufsize = 1 << MINPOWER;

	for(i = 0; i < BUFNO; i++) {
		freelistlist[i].sz = bufsize;
		freelistlist[i].next = 0;
		bufsize <<= 1;
	}
	init = 1;
	return 1;
}

void add_fl(freelist *ptr) {
	freelist tmp;
	if (ptr == 0) {
		tmp.sz = 1;
		tmp.next = 0;
	}
	else {
		tmp.sz = 1;
		tmp.next = ptr;
		tmp = *(ptr - 1);
		ptr = &tmp;
	}
	return;	
}

freelist* rm_fl(freelist *ptr) {
	freelist* tmp = ptr;
	if(&ptr == 0) {
		(*ptr).sz = 0;
		ptr = 0;
		return tmp;
	}
	else {
		while(&ptr != 0) {
			ptr = (*ptr).next;
		}
		(*ptr).sz = 0;
		ptr = 0;
		return tmp;
	}
}


#endif // KMA_P2FL
