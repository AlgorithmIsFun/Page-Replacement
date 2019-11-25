#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include <string.h>
#define min(a,b) ((a < b)? a : b)
#define max(a,b) ((a > b)? a : b)
#define delta(b1,b2) (int)((b1 >= b2)? 1 : b2/(double)b1)

extern int memsize;

extern int debug;

extern struct frame *coremap;

int location;

//linked list struct for page list and ghost list
typedef struct linked_list {
	pgtbl_entry_t *ptr;
	struct linked_list *next;
} linked_list;

//arc struct containg all the linked list and required info
typedef struct {
	linked_list *t1, *t2, *b1, *b2;
	int t1_length;
	int t2_length;
	int b1_length;
	int b2_length;
	int capacity;
	int p;
} arc_struct;

arc_struct *arc;

//pop last linked list node from linked list and return the page pointer
pgtbl_entry_t *pop(linked_list *t, int length){
	//check if list is empty, return NULL
	if (t == NULL){
		return NULL;
	}
	//check if linked list has only 1 element
	if (t->next == NULL){
		//free linked list and return the page pointer
		pgtbl_entry_t *ret = t->ptr;
		if (arc->t1 == t){
			free(arc->t1);
			arc->t1 = NULL;
		}
		if (arc->t2 == t){
			free(arc->t2);
			arc->t2 = NULL;
		}
		if (arc->b1 == t){
                        free(arc->b1);
                        arc->b1 = NULL;
                }
		if (arc->b2 == t){
                        free(arc->b2);
                        arc->b2 = NULL;
                }
		t = NULL;
		return ret;
	}
	//loop to the second last node of linked_list
	linked_list *test = t;
	for (int i = 0; i < length-2; i++){
		test = test->next;	
	}
	//return page refrence and free node
	pgtbl_entry_t *ret = test->next->ptr;
	free(test->next);
	test->next = NULL;
	return ret;
}

//update p value in evict if page being refrenced is in b1 or b2
void update_p(int location){
	if (location == 3){	
                arc->p = min(arc->p + delta(arc->b1_length, arc->b2_length), arc->capacity);
        }
        else if (location == 4){
                arc->p = max(arc->p - delta(arc->b2_length, arc->b1_length), 0);
        }

}

//implement replace function
linked_list * replace(){
	//check first condition
	linked_list *last;
	if (arc->t1_length > 0 && ((arc->t1_length > arc->p) || (arc->t1_length == arc->p && location == 4))) {
		//add node into head of b1 and delete from t1
		last = malloc(sizeof(linked_list));
		last->ptr = pop(arc->t1, arc->t1_length);
		if (last->ptr == NULL){
			return NULL;
		}
		last->next = arc->b1;
		arc->b1 = last;
		arc->t1_length -= 1;
		arc->b1_length += 1;
		return arc->b1;
	}else{
		//add node into head of b2 and delete from t2
		last = malloc(sizeof(linked_list));
		last->ptr = pop(arc->t2, arc->t2_length);
		if (last->ptr == NULL){
			return NULL;
		}
		last->next = arc->b2;
		arc->b2 = last;
		arc->t2_length -= 1;
		arc->b2_length += 1;
		return arc->b2;
	}
}

//returns values based on the location of the page reference
int update_location(){
	pgtbl_entry_t *fra = coremap[0].last_pointer;
	linked_list *look_up = arc->b1;
	//look through all nodes of b1 if the pointer is in there
	for (int i = 0; i < arc->b1_length; i++){
		if (look_up->ptr == fra){
			return 3;
		}
		look_up = look_up->next;
	}	
	look_up = arc->b2;
	//look through all nodes of b2 if the pointer is in there
	for (int i = 0; i < arc->b2_length; i++){
		
		if (look_up->ptr == fra){
			return 4;
		}
		look_up = look_up->next;
	}
	return 0;

}
//find which frame number contains the page pointer
int find_frame(pgtbl_entry_t *p){
	for (int i = 0; i < memsize; i++){
		if (coremap[i].pte == p){
			return i;
		}
	}
	return -1;
}

/* Page to evict is chosen using the ARC algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int arc_evict() {
	//Case if page reference is in b1 or b2 and evict LRU of t1 and t2

	location = update_location();
	if (location == 3 || location == 4){
		update_p(location);
		return find_frame(replace()->ptr);
	}
	//case if page refrence is not in any linked lists
	else{
		//check if |T1| + |B1| = c
		if (arc->t1_length + arc->b1_length == arc->capacity){
			//check if |T1| < c
                	if (arc->t1_length < arc->capacity){
				//pop from b1 and call replace	
				pop(arc->b1, arc->b1_length);
				arc->b1_length -= 1;
				return find_frame(replace()->ptr);
			//check if |B1| == 0
			}else{
				//evict LRU of t1
				linked_list *last = malloc(sizeof(linked_list));
				last->ptr = pop(arc->t1, arc->t1_length);
                		if (last->ptr == NULL){
                	        	return -1;
                		}
                		last->next = arc->b1;
                		arc->b1 = last;
                		arc->t1_length -= 1;
                		arc->b1_length += 1;	
				return find_frame(arc->b1->ptr);
			}
		//check if |T1| + |B1| < c
        	}else if (arc->t1_length + arc->b1_length < arc->capacity){
			//check if |T1| + |T2| + |B1| + |B2| >= c
               		if (arc->t1_length + arc->t2_length + arc->b1_length + arc->b2_length >= arc->capacity){
				//check if |T1| + |T2| + |B1| + |B2| = 2c
                      	 	if (arc->t1_length + arc->t2_length + arc->b1_length + arc->b2_length >= 2*memsize){
					//pop out of b2 (all linked list are full)
					pop(arc->b2, arc->b2_length);
					arc->b2_length -= 1;
                       		}
				return find_frame(replace()->ptr);
			}
               	}
    	}
	//place holder to avoid warning messages. Should never reach this point
	return -1;
}

//delete page from T1 or B1 and place it at MRU of T2
void add_page_T1(pgtbl_entry_t *p, int counter, char *type){
	//add page to MRU in T2
	linked_list *hit_T1 = malloc(sizeof(linked_list));
	linked_list *fre;
        hit_T1->ptr = p;
        hit_T1->next = arc->t2;
        arc->t2 = hit_T1;
	//check if page is MRU in T1 or B1
        if (counter == 0){
		//check if page is in T1
		int result = strcmp(type, "T1");
		if (result == 0){
			//delete node out of linked list T1
			fre = arc->t1;
			if (arc->t1->next == NULL) {
        			arc->t1 = NULL;
			}else{
				arc->t1 = arc->t1->next;
			}
		//check if page is in B1
		}else{
			//delete node out of linked list B1
			fre = arc->b1;
			if (arc->b1->next == NULL){
				arc->b1 = NULL;
			}else{
				arc->b1 = arc->b1->next;
			}
		}
		free(fre);
		fre = NULL;
	//check if page is not MRU in T1 or T2
	}else{
		int result = strcmp(type, "T1");
		//check if page is in T1
		if (result == 0){
			//delete node out of linked list T1
			linked_list *task = arc->t1;
			while (task->next->ptr != p){
				task = task->next;
			}
			fre = task->next;
			task->next = task->next->next;
		//check if page is in B1
		}else{
			//delete node out of linked list B1
			linked_list *task = arc->b1;
                        while (task->next->ptr != p){
                                task = task->next;
                        }
                        fre = task->next;
                        task->next = task->next->next;
		}
		free(fre);
		fre = NULL;
	}
	return;
}
//delete page at T2 or B2 and place at MRU of T2
void add_page_T2(pgtbl_entry_t *p, int counter, char *type){
	//add page to MRU of T2
        linked_list *hit_T2 = malloc(sizeof(linked_list));
        linked_list *fre;
	hit_T2->ptr = p;
        hit_T2->next = arc->t2;
        arc->t2 = hit_T2;
	//check if page is not MRU of T2 or B2
        if (counter > 0){
                int result = strcmp(type, "T2");
		//check if page is in T2
		if (result == 0){
			//delete node out of linked list T2
			linked_list *task = arc->t2;
                	while (task->next->ptr != p){
                		task = task->next;
                	}
                	fre = task->next;
                	task->next = task->next->next;
		//chck if page is in B2
                }else{
			//delete node out of B2
			linked_list *task = arc->b2;
                        while (task->next->ptr != p){
                                task = task->next;
                        }
                        fre = task->next;
                        task->next = task->next->next;
		}
		free(fre);
		fre = NULL;
	//check if page is MRU of T2 or B2
        }else{
		//check if page is in T2
		if (strcmp(type, "T2") == 0){
			//delete node out of linked list T2
			fre = arc->t2->next;
			if (fre != NULL){
                		arc->t2->next = arc->t2->next->next;
				free(fre);
			}
			fre = NULL;
		//check if page is in B2
        	}else{
			//delete node out of linked list B2
			fre = arc->b2->next;
			if (fre != NULL){
				arc->b2->next = arc->b2->next->next;
				free(fre);
			}
			fre = NULL;
		}
	}
	return;
}

/* This function is called on each access to a page to update any information
 * needed by the ARC algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void arc_ref(pgtbl_entry_t *p) {
	
	
	linked_list *test = arc->t1;
	//check if page is in t1
	for (int i = 0; i < arc->t1_length; i++){
		if (p == test->ptr){
			//add page into MRU T2
			add_page_T1(p, i, "T1");
			arc->t1_length -= 1;
			arc->t2_length += 1;
			return;
		}
		test = test->next;
	}
	//check if page is in b1
	test = arc->b1;
        for (int i = 0; i < arc->b1_length; i++){
                if (p == test->ptr){
			//add page into MRU T2
			add_page_T1(p, i, "B1");
                        arc->b1_length -= 1;
                        arc->t2_length += 1;
                        return;
                }
                test = test->next;
        }

	
	//check if page is in t2
	test = arc->t2;
	for (int i = 0; i < arc->t2_length; i++){
		if (p == test->ptr){
			//add page into MRU T2
			add_page_T2(p, i, "T2");
			return;
		}
		test = test->next;
	}
	//check if page is in b2
	test = arc->b2;
	for (int i = 0; i < arc->b2_length; i++){
		if (p == test->ptr){
			//add page into MRU T2
			add_page_T2(p, i, "B2");
			arc->b2_length -= 1;
			arc->t2_length += 1;
			return;
		}
		test = test->next;
	}
	//miss and not in any list. Add to MRU T1
	test = malloc(sizeof(linked_list));
	test->ptr = p;
	test->next = arc->t1;
	arc->t1 = test;
	arc->t1_length += 1;
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void arc_init() {
	//initialize all linked lists and lenghts
	arc = malloc(sizeof(arc_struct));
	arc->t1 = malloc(sizeof(linked_list));
	arc->t2 = malloc(sizeof(linked_list));
  	arc->b1 = malloc(sizeof(linked_list));
  	arc->b2 = malloc(sizeof(linked_list));
	arc->capacity = memsize;
	arc->t1_length = 0;
	arc->t2_length = 0;
	arc->b1_length = 0;
	arc->b2_length = 0;
	arc->p = 0;
}
