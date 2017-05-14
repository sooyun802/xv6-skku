#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "slab.h"

struct {
	struct spinlock lock;
	struct slab slab[NSLAB];
} stable;

int charOffset[9] = {64, 32, 16, 8, 4, 2, 1, 1, 1};

int getAbit(char temp, int n){
	return (temp & (1 << (7-n))) >> (7-n);
}

char setAbit(char temp, int n, int b){
	if(b)
		return temp | (1 << (7-n));
	return temp & (~(1 << (7-n)));
}

int findEmpty(int index, int page){
	int i, j;
	char *bitmap = stable.slab[index].bitmap + page*charOffset[index];
	char temp;

	for(i = 0; i < charOffset[index]; i++){
		temp = *(bitmap+i);
		for(j = 0; j < 8; j++){
			if(j >= stable.slab[index].num_objects_per_page)
				break;
			if(getAbit(temp, j) == 0){
				*(bitmap+i) = setAbit(temp, j, 1);
				return i*8 + j;
			}
		}
	}

	return -1;
}

void slabinit(){
	/* fill in the blank */
	int i, j, sizes[9] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};
	int object_num[9] = {512, 256, 128, 64, 32, 16, 8, 4, 2};

	acquire(&stable.lock);
	for(i = 0; i < NSLAB; i++){
		stable.slab[i].size = sizes[i];
		stable.slab[i].num_pages = 1;
		stable.slab[i].num_free_objects = object_num[i];
		stable.slab[i].num_used_objects = 0;
		stable.slab[i].num_objects_per_page = object_num[i];
		stable.slab[i].bitmap = kalloc();
		for(j = 0; j < 4096; j++){
			stable.slab[i].bitmap[j] = 0;
		}
		for(j = 0; j < MAX_PAGES_PER_SLAB; j++){
			stable.slab[i].page[j] = 0;
		}
		stable.slab[i].page[0] = kalloc();
	}
	release(&stable.lock);
}

char *kmalloc(int size){
	/* fill in the blank */
	int i, index = 0, object = 0;

	for(i = 0; i < NSLAB; i++){
		if(stable.slab[i].size - size >= 0){
			index = i;
			break;
		}
	}

	acquire(&stable.lock);
	if(stable.slab[index].num_free_objects == 0){
		stable.slab[index].page[stable.slab[index].num_pages] = kalloc();
		stable.slab[index].num_pages += 1;
		stable.slab[index].num_free_objects += stable.slab[index].num_objects_per_page;
	}

	for(i = 0; i < stable.slab[index].num_pages; i++){
		object = findEmpty(index, i);
		if(object == -1)
			continue;
		else{
			stable.slab[index].num_free_objects -= 1;
			stable.slab[index].num_used_objects += 1;
			release(&stable.lock);
			return stable.slab[index].page[i] + object*stable.slab[index].size;
		}
	}
	return 0;
}

void kmfree(char *addr){
	/* fill in the blank */
	int i, j, k;
	char temp;

	acquire(&stable.lock);
	for(i = 0; i < NSLAB; i++){
		for(j = 0; j < stable.slab[i].num_pages; j++){
			for(k = 0; k < stable.slab[i].num_objects_per_page; k++){
				if(addr == (stable.slab[i].page[j] + k*stable.slab[i].size)){
					temp = *(stable.slab[i].bitmap + j*charOffset[i] + k/8);
					*(stable.slab[i].bitmap + j*charOffset[i] + k/8) = setAbit(temp, k%8, 0);
					stable.slab[i].num_free_objects += 1;
					stable.slab[i].num_used_objects -= 1;
					release(&stable.lock);
					return;
				}
			}
		}
	}
}

void slabdump(){
	struct slab *s;

	cprintf("size\tnum_pages\tused_objects\tfree_objects\n");
	for(s = stable.slab; s < &stable.slab[NSLAB]; s++){
		cprintf("%d\t%d\t\t%d\t\t%d\n", s->size, s->num_pages, s->num_used_objects, s->num_free_objects);
	}
}