/*
 * Test suite based off of program.c
 * to test page management against simple computations
*/

#include "test.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COUNT_SIZE 128

int total;
int count[COUNT_SIZE];

static int compare_bytes(const void *pa, const void *pb) {
	int a = *(char*)pa;
	int b = *(char*)pb;

	if(a<b) {
		return -1;
	} else if(a==b) {
		return 0;
	} else {
		return 1;
	}

}

void compute(char *data, int length) {
	total = 0;
	int i;
	srand(5103);
	//initializing all elements to -1 to keep track of count
    memset(count, -1, COUNT_SIZE*(sizeof(int)));

	for(i=0;i<length;i++) {
        int temp = rand()%COUNT_SIZE;
		data[i] = temp;
        total = total + temp;		
		//to keep track of the frequency of words
		if (count[temp] == -1) {
			//since the first time a value is touched its value would be -1
			count[temp] = count[temp]+2;
		} else {
		    count[temp] = count[temp]+1;
	    }
	}
}

void mean_mode(char *data, int length) {
	//initialize data and perform counting
	compute(data, length);
	float mean = total/length; 
	printf("mean of the randomly generated data %f : ", mean);
	int max_freq = 0, mode = 0;
	for (int i=0; i< COUNT_SIZE; i++) {
	    if (count[i] == -1) {
		    continue;
	    }
	    if (count[i] > max_freq) {
	        mode = i;
	        max_freq = count[i];
	    }
	}
	printf("mode of the randomly generated data %d \n", mode);
}

void count_sort(char *data, int length) {
	//initialize data and perform counting
	compute(data, length);
	int array[length];
    int i = 0, j = 0;
	//populate the sorted array
	while (i<length) {
	   for (; j<COUNT_SIZE; j++) {
	       int c = count[j];
	       if (c != -1) {
	          while(c--) {
       		      array[i++] = j;
	    	  }
	       }
	    }	
	}

	//perform qsort
	qsort(data,length,1,compare_bytes);
	
	//check if qsort is the same in comparison to countsort
	for(i=0; i< length; i++) {
        assert(array[i] == data[i]);
	}
}
