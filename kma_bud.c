/***************************************************************************
 *	Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *		Purpose: Kernel memory allocator based on the power-of-two free list
 * 						algorithm
 *		Author: Stefan Birrer
 *		Version: $Revision: 1.2 $
 *		Last Modification: $Date: 2009/10/31 21:28:52 $
 *		File: $RCSfile: kma_p2fl.c,v $
 *		Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *	ChangeLog:
 * -------------------------------------------------------------------------
 *		$Log: kma_p2fl.c,v $
 *		Revision 1.2	2009/10/31 21:28:52	jot836
 *		This is the current version of KMA project 3.
 *		It includes:
 *		- the most up-to-date handout (F'09)
 *		- updated skeleton including
 *				file-driven test harness,
 *				trace generator script,
 *				support for evaluating efficiency of algorithm (wasted memory),
 *				gnuplot support for plotting allocation and waste,
 *				set of traces for all students to use (including a makefile and README of the settings),
 *		- different version of the testsuite for use on the submission site, including:
 *				scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *		Revision 1.1	2005/10/24 16:07:09	sbirrer
 *		- skeleton
 *
 *		Revision 1.2	2004/11/05 15:45:56	sbirrer
 *		- added size as a parameter to kma_free
 *
 *		Revision 1.1	2004/11/03 23:04:03	sbirrer
 *		- initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/************Private include**********************************************/
#include "kpage.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*	defines and typedefs should have their names in all caps.
 *	Global variables begin with g. Global constants with k. Local
 *	variables should be in all lower case. When initializing
 *	structures and arrays, line everything up in neat columns.
 */

	
typedef struct
{
	int size;
	int pagespace;
	void* nextfree;
	void* prevfree;
	void* buddy;
	void* pageheader;
	void* pagelink;
	kpage_t* pagepointer;
} header;

#define PAGESIZE 8192
#define DEBUG 0

/************Global Variables*********************************************/
static kpage_t* kpage; 
int request = 0;
int alloc = 0;
/************Function Prototypes******************************************/
void kmainit();
void* search(kma_size_t, int);
void* split_block(void*, kma_size_t);
void coalesce_blocks(void*);
/************External Declaration*****************************************/

/**************Implementation***********************************************/

void*
kma_malloc(kma_size_t size)
{
	int totalsize;
	header *newheader,*pageheader;
	
	if(kpage==NULL || kpage->ptr == NULL)
 		kmainit();
	if(size + sizeof(header)>PAGESIZE)
 		return NULL;
	totalsize = size + sizeof(header);
	
	request=request+size;
	
	if(totalsize <= 32)				
	{ 
		newheader=(header*)search(32, 0);
		newheader->nextfree=kpage->ptr;
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+32;
		return((void*)newheader+sizeof(header));
	}
	
	else if(totalsize <= 64) 	
	{ 
		newheader=(header*)search(64, 0);
		newheader->nextfree=kpage->ptr+sizeof(header);
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+64;
		return((void*)newheader+sizeof(header));
	}
	else if(totalsize <= 128)	
	{ 
		newheader=(header*)search(128, 0);
		newheader->nextfree=kpage->ptr+sizeof(header)*2;
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+128;
		return((void*)newheader+sizeof(header));
	}
	else if(totalsize <= 256)	
	{ 
		newheader=(header*)search(256, 0);
		newheader->nextfree=kpage->ptr+sizeof(header)*3;
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+256;
		return((void*)newheader+sizeof(header));
	}
	else if(totalsize <= 512)	
	{ 
		newheader=(header*)search(512, 0);
		newheader->nextfree=kpage->ptr+sizeof(header)*4;
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+512;
		return((void*)newheader+sizeof(header));
	}
	else if(totalsize <= 1024) 
	{ 
		newheader=(header*)search(1024, 0);
		newheader->nextfree=kpage->ptr+sizeof(header)*5;
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+1024;
		return((void*)newheader+sizeof(header));
	}
	else if(totalsize <= 2048) 
	{ 
		newheader=(header*)search(2048, 0);
		newheader->nextfree=kpage->ptr+sizeof(header)*6;
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+2048;
		return((void*)newheader+sizeof(header));
	}
	else if(totalsize <= 4096) 
	{ 
		newheader=(header*)search(4096, 0);
		newheader->nextfree=kpage->ptr+sizeof(header)*7;
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+4096;
		return((void*)newheader+sizeof(header));
	}
	else if(totalsize <= 8192) 
	{ 
		newheader=(header*)search(8192, 0);
		newheader->nextfree=kpage->ptr+sizeof(header)*8;
		pageheader=(header*)newheader->pageheader;
		pageheader->pagespace=pageheader->pagespace-newheader->size;
		alloc=alloc+8192;
		return((void*)newheader+sizeof(header));
	}
	else	return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
	header *freeheader,*pageheader,*headertofree,*preheader,*listtoadd;
	int i,flag;
	
	freeheader = (header*)(ptr-sizeof(header));
	listtoadd = (header*)freeheader->nextfree;
	freeheader->nextfree = listtoadd->nextfree;
	listtoadd->nextfree = ptr-sizeof(header); 		
	pageheader=(header*)freeheader->pageheader;
	pageheader->pagespace= pageheader->pagespace+freeheader->size;
	// attempt to coalesce blocks
	coalesce_blocks(ptr);
	if(pageheader->pagespace == 8192)																								//If a page has free space of 8192, then it could be freed.
	{
		headertofree = pageheader;
		while(headertofree->pagelink!=NULL)																								
		{
			preheader = headertofree;
			while(preheader->nextfree!=headertofree) 																		//Find the former block in the link list to delete this block from list.
			{
 				preheader = preheader->nextfree;
			}
			preheader->nextfree = headertofree->nextfree;
			headertofree=headertofree->pagelink; 							
		}
		preheader = headertofree;
		while(preheader->nextfree!=headertofree)
		{
 			preheader = preheader->nextfree;
		}
		preheader->nextfree = headertofree->nextfree;
		free_page(pageheader->pagepointer); 
		flag=0;
		listtoadd=(header*)kpage->ptr;
		for(i=0;i<9;i++)
			{
			if(listtoadd->nextfree!=listtoadd) flag=1;
			listtoadd++;
			}	
		if(flag==0)	{free_page(kpage);kpage=NULL; printf("The total size of all requested blocks is %d. The actual allocated size is %d. The efficiency is %f\n",request,alloc,(double)request/(double)alloc);} 																
 	}
}

void kmainit()
{
	kpage_t* initpage;
	header* headlist;
	initpage = get_page();
	kpage = initpage;
	headlist = (header*)initpage->ptr;
 	
	headlist->size = 32;
	headlist->nextfree = headlist;		
	headlist++;
	
	headlist->size = 64;
	headlist->nextfree = headlist;
	headlist++;
	
	headlist->size = 128;
	headlist->nextfree = headlist;
	headlist++;
	
	headlist->size = 256;
	headlist->nextfree = headlist;
	headlist++;
	
	headlist->size = 512;
	headlist->nextfree = headlist;
	headlist++;
	
	headlist->size = 1024;
	headlist->nextfree = headlist;
	headlist++;
	
	headlist->size = 2048;
	headlist->nextfree = headlist;
	headlist++;
	
	headlist->size = 4096;
	headlist->nextfree = headlist;
	headlist++;
	
	headlist->size = 8192;
	headlist->nextfree = headlist;

}

void* search(kma_size_t size, int rec)
{
	kpage_t *newpage;
	header *buffind,*buffind2,*searchlist;
	void* pointer;

	switch (size)
	{
		case 32:
			searchlist = (header*)kpage->ptr;
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				pointer = search(64, 1);
				// if we're recursing, that means the block is too big; split it to the right size
				if (rec == 1) {
					// reassign new split to pointer
					// I'm not sure if you actually need to reassign the value since generally it would point to the same place
					// but it seems like it makes it more human-readable
					// then again I'm putting in like a bazillion comments
					pointer = split_block(pointer, 64);
				}
				buffind = (header*)pointer;
				buffind->nextfree = searchlist;
				buffind->size = 32;
				searchlist->nextfree = pointer;
				pointer = pointer + 32;
				buffind2 = (header*)pointer;
				buffind2->nextfree = NULL;
				buffind2->size = 32;
				buffind2->pageheader=buffind->pageheader;
				buffind2->pagelink=buffind->pagelink;
				buffind->pagelink= pointer;
				return(pointer);
			}			
			break;
		case 64:
			searchlist = (header*)(kpage->ptr+sizeof(header));
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				pointer = search(128, 1);
				if (rec == 1) {
					pointer = split_block(pointer, 128);
				}
				buffind = (header*)pointer;
				buffind->nextfree = searchlist;
				buffind->size = 64; 		
				searchlist->nextfree = pointer;
				pointer = pointer + 64;
				buffind2 = (header*)pointer;
				buffind2->nextfree = NULL;
				buffind2->size = 64;
				buffind2->pageheader=buffind->pageheader;
				buffind2->pagelink=buffind->pagelink;
				buffind->pagelink= pointer;
				return(pointer);
			}			
			break;
		case 128:
			searchlist = (header*)(kpage->ptr+sizeof(header)*2);
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				pointer = search(256, 1);
				if (rec == 1) {
					pointer = split_block(pointer, 256);
				}
				buffind = (header*)pointer;
				buffind->nextfree = searchlist;
				buffind->size = 128;
				searchlist->nextfree = pointer;
				pointer = pointer + 128;
				buffind2 = (header*)pointer;
				buffind2->nextfree = NULL;
				buffind2->size = 128;
				buffind2->pageheader=buffind->pageheader;
				buffind2->pagelink=buffind->pagelink;
				buffind->pagelink= pointer;
				return(pointer);
			}			
			break;
		case 256:
			searchlist = (header*)(kpage->ptr+sizeof(header)*3);
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				pointer = search(512, 1);
				if (rec == 1) {
					pointer = split_block(pointer, 512);
				}
				buffind = (header*)pointer;
				buffind->nextfree = searchlist;
				buffind->size = 256;
				searchlist->nextfree = pointer;
				pointer = pointer + 256;
				buffind2 = (header*)pointer;
				buffind2->nextfree = NULL;
				buffind2->size = 256;
				buffind2->pageheader=buffind->pageheader;
				buffind2->pagelink=buffind->pagelink;
				buffind->pagelink= pointer;
				return(pointer);
			}			
			break;
		case 512:
			searchlist = (header*)(kpage->ptr+sizeof(header)*4);
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				pointer = search(1024, 1);
				if (rec == 1) {
					pointer = split_block(pointer, 1024);
				}
				buffind = (header*)pointer;
				buffind->nextfree = searchlist;
				buffind->size = 512;
				searchlist->nextfree = pointer;
				pointer = pointer + 512;
				buffind2 = (header*)pointer;
				buffind2->nextfree = NULL;
				buffind2->size = 512;
				buffind2->pageheader=buffind->pageheader;
				buffind2->pagelink=buffind->pagelink;
				buffind->pagelink= pointer;
				return(pointer);
			}			
			break;
		case 1024:
			searchlist = (header*)(kpage->ptr+sizeof(header)*5);
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				pointer = search(2048, 1);
				if (rec == 1) {
					pointer = split_block(pointer, 2048);
				}
				buffind = (header*)pointer;
				buffind->nextfree = searchlist;
				buffind->size = 1024;
				searchlist->nextfree = pointer;
				pointer = pointer + 1024;
				buffind2 = (header*)pointer;
				buffind2->nextfree = NULL;
				buffind2->size = 1024;
				buffind2->pageheader=buffind->pageheader;
				buffind2->pagelink=buffind->pagelink;
				buffind->pagelink= pointer;
				return(pointer);
			}			
			break;
		case 2048:
			searchlist = (header*)(kpage->ptr+sizeof(header)*6);
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				pointer = search(4096, 1);
				if (rec == 1) {
					pointer = split_block(pointer, 4096);
				}
				buffind = (header*)pointer;
				buffind->nextfree = searchlist;
				buffind->size = 2048;
				searchlist->nextfree = pointer;
				pointer = pointer + 2048;
				buffind2 = (header*)pointer;
				buffind2->nextfree = NULL;
				buffind2->size = 2048;
				buffind2->pageheader=buffind->pageheader;
				buffind2->pagelink=buffind->pagelink;
				buffind->pagelink= pointer;
				return(pointer);
			}			
			break;
		case 4096:
			searchlist = (header*)(kpage->ptr+sizeof(header)*7);
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				pointer = search(8192, 1);
				if (rec == 1) {
					pointer = split_block(pointer, 8192);
				}
				buffind = (header*)pointer;
				buffind->nextfree = searchlist;
				buffind->size = 4096;
				searchlist->nextfree = pointer;
				pointer = pointer + 4096;
				buffind2 = (header*)pointer;
				buffind2->nextfree = NULL;
				buffind2->size = 4096;
				buffind2->pageheader=buffind->pageheader;
				buffind2->pagelink=buffind->pagelink;
				buffind->pagelink= pointer;
				return(pointer);
			}			
			break;
		case 8192:
			searchlist = (header*)(kpage->ptr+sizeof(header)*8);
			if(searchlist->nextfree!=searchlist)
			{
				buffind = (header*)searchlist->nextfree;
				searchlist->nextfree = buffind->nextfree;
				return(buffind);
			}
			else
			{
				newpage = get_page();
				buffind = (header*)(newpage->ptr);
				buffind->nextfree = NULL;
				buffind->size = 8192;
				buffind->pagespace=8192;
				buffind->pageheader=buffind;
				buffind->pagelink=NULL;
				buffind->pagepointer=newpage;
				return(newpage->ptr);
			}			
			break;
		default:
			printf("Error in search the header\n");
			break;
	}
	return(0);
}

void* split_block(void* ptr, kma_size_t size) {
	header* tmp1 = (header*) ptr;
	header* tmp2 = (header*) (void*) (ptr + (size/2));
	tmp2->nextfree = (void*) tmp1->nextfree;
	tmp1->nextfree = (void*) tmp2;
	tmp1->buddy = (void*) tmp2;
	tmp2->buddy = (void*) tmp1;
	return (void*) tmp1;
}

void coalesce_blocks(void* ptr) {
	header* tmp = (header*) ptr;
	header* prev = (header*) (void*) (ptr - tmp->size - sizeof(header));
	header* next = (header*) (void*) (ptr + tmp->size);
	if (prev->buddy == ptr) {
		//coalesce with previous:
		// next pointer of prev to point to next of ptr
		// take ptr out of the list
		prev->nextfree = tmp->nextfree;
	} else if (next->buddy == ptr) {
		// coalesce with next:
		// next pointer of ptr to point to next of next
		// take next out of the list
		tmp->nextfree = next->nextfree;
	}
	return;
}
		

#endif // KMA_BUD
