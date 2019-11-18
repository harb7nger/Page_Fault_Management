/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"
#include "test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_FRAMES 100

//Inverted page table: 
//An array indicating page mapping for each frame in physical memory
//This is used while evicting frames in rand and fifo policies
int frame_to_page_mapping[MAX_FRAMES]; 
int frame_filled[MAX_FRAMES]; 

//Variable indicating the number of allocated frames. 
int allocated_frames = 0;

//Pointer to disk 
struct disk *the_disk = 0;

//Variable indicating frame replacement policy to be followed
char *policy;

//Data structure to collect frame history statistics for 
//implementing fifo policy
typedef struct frame_history frame_history;
struct frame_history {
	int frame_access[MAX_FRAMES];
	int head;
	int tail;
};
struct frame_history* fifo; 

//Data structure to collect page history statistics for 
//implementing custom policy
int page_history[MAX_FRAMES];

//Stats
int num_page_faults = 0;
int num_disk_reads = 0;
int num_disk_writes = 0;

/*
Function to print stats
*/
void print_stats() {
	printf("\n----SUMMARY----\n");
	printf("Number of Page Faults = %d\n",num_page_faults);
	printf("Number of Disk Reads = %d\n",num_disk_reads);
	printf("Number of Disk Writes = %d\n\n",num_disk_writes);
}

/*
Function to print contents of a page to a file
Arguments: 
	1. pointer to page contents
	2. Number of bytes to be written
	3. page number
	4. read/write operation
Return: None
Description: 
	1. Open file (page_num.txt) in append mode.
	2. Print operation
	3. Print page contents from base pointer(first argument)
	4. Number of bytes printed is given by second argument.
	
*/
void print_mem(char *p, size_t length, int page_num, int operation){

	//Create file name (based on page number)
	char buffer[100];
	sprintf(buffer,"%d.txt",page_num);

	//Open file in append mode
	FILE*fp = fopen(buffer,"a");

	//Print operation(read/write)
	if(operation == 1)
		fprintf(fp,"disk_write:\n");
	else
		fprintf(fp,"disk_read:\n");

	//Print page contents
	for (size_t i = 0; i < length; i++){
		fprintf(fp,"%X ", p[i] & 0xff);
	}
	fprintf(fp,"\n");

	//Close file
	fclose(fp);
}

/*
Function to find frame 
*/
int find_frame(struct page_table *pt) {
	int frame = -1;
	if(allocated_frames >= page_table_get_nframes(pt)){
		if(!strcmp(policy,"rand")) {
			frame = rand()%page_table_get_nframes(pt);
			return frame;
		}
		else if(!strcmp(policy,"fifo")) {
			frame = fifo->head;
			return frame;
		}
		else {
			frame = 0;
			for(int i = 0; i<page_table_get_nframes(pt);i++){
				if(page_history[frame_to_page_mapping[i]]<page_history[frame_to_page_mapping[frame]])
					frame=i;
			}

			return frame;
		}	
	}
	else {
		frame = allocated_frames;
#ifdef DEBUG
		printf("Assigning frame %d as it is empty\n",frame);
#endif
		allocated_frames++;
		return frame;
	}
}

/* 
Function to evict frame
*/
void evict_frame(struct page_table *pt,int frame) {

	int fr, bt;

	//Get the existing page table entry 
	page_table_get_entry(pt,frame_to_page_mapping[frame],&fr,&bt);

#ifdef DEBUG
	printf("Evicting page %d from frame %d\n",frame_to_page_mapping[frame],frame);
#endif
	
	//If page is modified, write back to disk
	if(bt==(PROT_READ|PROT_WRITE)){
		disk_write(the_disk, frame_to_page_mapping[frame], page_table_get_physmem(pt)+(frame*PAGE_SIZE));
		num_disk_writes++;
#ifdef DEBUG
		printf("Disk is written with page %d\n", frame_to_page_mapping[frame]);
#endif
	}

	//Set permission to NONE. 
	//This is to ensure, SIGSEGV exception occurs next time this page is referenced
	page_table_set_entry(pt,frame_to_page_mapping[frame],frame,PROT_NONE);
}

/*
Function to update frame history
*/
void update_frame_history(struct page_table *pt,int frame) {

	if((fifo->tail + 1)%page_table_get_nframes(pt) != fifo->head ){
		if(fifo->tail == -1)
			fifo->head = 0;
		fifo->tail = (++fifo->tail) % page_table_get_nframes(pt);
		fifo->frame_access[fifo->tail] = frame;
	}
	else {
		fifo->frame_access[fifo->head] = frame;
		fifo->head = (++fifo->head) % page_table_get_nframes(pt);	
	}
}

/*
Function to update page history
*/
void update_page_history(struct page_table *pt,int page) {

	page_history[page]++;

}

/*
Function to update frame 
*/
void update_frame(struct page_table *pt, int frame, int page) {

	disk_read(the_disk, page, page_table_get_physmem(pt)+(frame*PAGE_SIZE));
	num_disk_reads++;
	page_table_set_entry(pt,page,frame,PROT_READ);
	frame_to_page_mapping[frame] = page;
	frame_filled[frame] = 1;
#ifdef DEBUG
	page_table_print_entry(pt,page);
#endif

	if(!strcmp(policy,"fifo")){
		update_frame_history(pt,frame);
	}

}

/*
Function to handle SIGSEGV exceptions
Description:
	1. First time a page is seen, set permission to READ
	2. Copy from disk to memory 
		2.1 Handle evictions if needed (Based on the policy)
		2.2 Writeback if page is modified
	3. Second time a page is seen, set permission to READ|WRITE
*/
void page_fault_handler( struct page_table *pt, int page )
{

#ifdef DEBUG
	printf("\npage fault on page #%d\n",page);
#endif

	num_page_faults++;

	int fr=0; //Frame number
	int bt=0; //file permission

	//Get page entry from page table.
        page_table_get_entry(pt, page, &fr, &bt);

	if(!strcmp(policy,"custom")){
		update_page_history(pt,page);
	}

	//If mapping already exists
	if(frame_to_page_mapping[fr]==page){
		
		//If frame has already been assigned to the page,
		//it means the page is already there in Physical Memory
		//Exception occurred due to lack of write permission 
		//Update permission to allow writes
		page_table_set_entry(pt,page,fr,PROT_READ|PROT_WRITE);

#ifdef DEBUG
		printf("Access error for page %d\n", page);
		page_table_print_entry(pt,page);
#endif

	}	
	else {

		//If page fault is occuring for the first time, allocate a frame for the page.
		//Evict an old frame if necessary and copy page from disk to memory
		//Set permission to Read 

		int frame = find_frame(pt);
		if(frame_filled[frame])
			evict_frame(pt,frame);
		update_frame(pt,frame,page);

	}
}

int main( int argc, char *argv[] )
{

	memset(frame_to_page_mapping,-1,MAX_FRAMES*sizeof(int));
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	policy = argv[3];
	const char *program = argv[4];
	fifo = malloc(sizeof(struct frame_history));
	fifo->head=-1;
	fifo->tail=-1;
	struct disk *disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	the_disk = disk;
	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);
	
	
	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program, "mean_mode")) {
		mean_mode(virtmem, npages*PAGE_SIZE);
	} else if(!strcmp(program, "count_sort")) {
		count_sort(virtmem, npages*PAGE_SIZE);
	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);

	}
	
	print_stats();

	//page_table_print(pt);
	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
