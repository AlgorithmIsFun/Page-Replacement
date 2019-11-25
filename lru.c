#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

typedef struct linked_list{
	int frame;
	struct linked_list *prev;
	struct linked_list *next;
} linked_list;

linked_list *lru_list;
linked_list *front;
linked_list *final;

typedef struct Hash{
	int capacity;
	linked_list **table;
} Hash;

Hash *hash;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	//update final node and hash table
	int fra = final->frame;
	//check if linked list is the size of 1
	if (front == final){
		front = NULL;
	}
	linked_list *temp = final;
	final = final->prev;
	//check if final node exists
	if (final){
		final->next = NULL;
	}
	//free space and empty hash table entry
	free(temp);
	hash->table[fra] = NULL;
	return fra;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	//get linked list node from hash table
	linked_list *ref = hash->table[p->frame >> PAGE_SHIFT];
	//check if page refrenced first time
	if (ref == NULL){
		//allocate space and store values into node
		ref = malloc(sizeof(linked_list));
		ref->frame = p->frame >> PAGE_SHIFT;
		ref->prev = NULL;
		ref->next = front;
		//check if final or front are NULL and handle accordingly
		if (final == NULL){
			final = ref;
		}
		if (front == NULL){
			front = ref;
		}else{
			front->prev = ref;
			front = ref;
		}
		hash->table[p->frame >> PAGE_SHIFT] = ref;
	}
	//check if refrence is not already at the front of the linked list
	else if (ref != front){
		//update location of referenced node
		ref->prev->next = ref->next;
		if (ref->next){
			ref->next->prev = ref->prev;
		}
		if (ref == final){
			final = ref->prev;
			final->next = NULL;
		}
		ref->next = front;
		ref->prev = NULL;
		ref->next->prev = ref;
		front = ref;
	}
	return;
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	//initalize double linked list and hash table
	lru_list = malloc(sizeof(linked_list));
	front = final = NULL;
	hash = malloc(sizeof(Hash));
	hash->capacity = memsize;
	hash->table = malloc(memsize*sizeof(linked_list *));
	for (int i = 0; i < memsize; i++){
		hash->table[i] = NULL;
	}
}
