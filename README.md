## Overview

Paging is a key factor that affects the performance of an application. How efficiently an
application gets the data it wants to work on can affect its runtime. With ever-changing
requirements of applications and the underlying hardware, custom paging is required to
get better throughput out of applications. However, we acknowledge the importance of
fundamental paging schemes, which are still handy in terms of understanding more
complicated paging schemes.
As outlined, for this project, we have implemented paging schemes which evict pages in
random or in FIFO fashion using the APIs provided for page creation, page mapping,
disk read, disk write, etc. We have also implemented a custom paging policy, which is a
simple approximation to LFU and low on computation to achieve the same.
The aforementioned schemes have been written to emphasise readability while
incorporating metrics to collect information such as the numbers of page-faults, page
writes, page evictions, disk reads, disk writes, etc. A few simple test cases have also
been introduced to check if the implemented policies are working as expected by
comparing the output of two sort algorithms, of which one does the sorting without
having to compare and swap elements.

## Experiment Setup

The experiments are carried out in order to study the characteristics of different eviction policies.
The graphs compare the performance of these policies which gives us a better explanation of
how each policy behaves, what it does best and how it can be used efficiently.

Command used:
`./src/virtmem 100 <frames> <policy> <program>`

Where frames is the number of `frames` that are to be allocated while running virtmem. It lies
between 1 and 100.

`policy` is the page eviction scheme that is to be used while the program is executed. Which
can be either rand, fifo or custom.

Three policies are implemented in the experiments.
1. `rand` : Frame to be allocated to the new page is randomly chosen.
2. `fifo` : The page which has stayed the longest in memory is replaced by the new page.
3. `custom` : An approximated LFU is implemented as a frame allocation strategy.

`program` is the piece of code that operates on virtual memory to solve a simple computation.
The program can either be sort , focus or scan.

`test` is another program that provides additional test cases that we have developed for testing
our paging implementation. The test cases include:

`mean_mode` : the program randomly assigns a value between 0 and 127 to all elements
in the data. While doing so the test program keeps track of the frequency of each
number that is being stored in virtual memory and updates the total count of all numbers
that are being generated. Once all the data is allocated, it displays the mean and mode
of the data.
Run command: `./src/virtmem 100 <frames> <policy> mean_mode`

`count_sort` : similar to the computation above, the program keeps track of the
frequency of all the elements that are being allocated. These frequencies will be used to
sort the data in place without having to compare and swap elements. The same data is
sorted using quicksort. The output for count sort and quick sort is compared element by
element to ensure that data is not lost or corrupted when compare and swap is
performed for sorting.
Run Command: `./virtmem 100 <frames> <policy> count_sort`

## Custom Page Replacement Algorithm
We implemented an approximation to LFU (Least Frequently Used) algorithm as our
custom replacement policy. In LFU, a page accessed the least number of times within a
time window is chosen for eviction when a page needs to be replaced in memory.
Access could be either a page read or page write. To realize this algorithm, we need to
maintain a count for each page and increment the count whenever the page has been
accessed or modified.

### Challenges in implementing traditional LFU
1. The handler needs to know a page is being accessed by the program every time to
maintain an accurate count.
2. This implies that the pages in the frame must always have no access rights. As a
result, an exception is raised every time the program tries to use a page.
3. The dirty bit is necessary as it decides which pages are written to the disk before
evicting. In this implementation, it becomes complex to keep track of the dirty bit.
4. We then decided to implement an approximation to LFU rather than trying to
implement something as complex as stated above.

### Salient features
1. The custom algorithm maintains a data structure to count the number of times page
faults occur for each page. The handler refers to this data structure in the case of
evictions.
2. The handler is called whenever the program is trying to access a page that is not in
memory or when it tries to write to a read-only page. When the handler is being
called, it updates the count which is later used in the eviction policy.
3. At the time of eviction, the handler iterates through all the pages in memory to find
the page which was accessed the least number of times. It then decides to evict this
page to place the interested page in memory.

### Behaviour of the algorithm
1. At the beginning of the program, the algorithm acts fairly unbiased. This is because
the pages hardly have any count and they take turns to be evicted, just like FIFO.
2. But as count grows, the algorithm gives more priority to the page accessed
frequently in the past.
3. As a consequence, it reduces the number of evictions in total. This also brings down
the number of disk reads and writes.
