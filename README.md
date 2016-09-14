#### Slab Allocator

This Slab-allocator is an unoptimized implementation of [Bon94](https://www.seas.gwu.edu/~gparmer/courses/f16_6907/slab.pdf).
Moreover, I've only implemented Small slabs, slabs with data size less than 1/8th of PAGE_SIZE, 
the optimized path from the paper and tested the basic case of kmem_cache_create(), kmem_cache_alloc(), kmem_cache_free(), and kmem_cache_destroy().

If I had enough time, I'd have worked on compiler optimization for this, especially constant propagation, loop unrolling etc using
preprocessor directives, which would have been a great exercise and learning experience! But deadlines/priorities!

For Big slabs, I've only rough implementation or not in some places, I might end up not doing it before deadline. Sorry!

So, Working only for SMALL SLABS:
kmem_cache_create(), kmem_cache_alloc(), kmem_cache_free(), kmem_cache_destroy() - basic usage. 

Not working: 
* Above APIs for BIG SLABS. 
* In general back-end API and 
* most importantly not re-entrant, Multithreading (mutexes) not implemented. 

## How-To

## Compile

I wanted to make a shared library but for the grader, it gets difficult because of LD_LIBRARY_PATH setting etc in their env. 
So, linking it to the exe, that works without much efforts from either of us.

From the root of this project, run:
```
make clean all
```

## Run

From the test/ directory of this project, run:
```
./kmem_test
```

I've tested with valgrind to see if I've basic mem leaks and there are none. 

## Sample output

Test program creates a cache, allocates as many possible caches and writes some content to it, frees the memory.
When it retries the alloc, it just checks the content of the memory and verified if they are same as what I've set before. And that works!

valgrind:
```
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ valgrind --leak-check=yes ./kslab_test 
==21675== Memcheck, a memory error detector
==21675== Copyright (C) 2002-2013, and GNU GPL'd, by Julian Seward et al.
==21675== Using Valgrind-3.10.1 and LibVEX; rerun with -h for copyright info
==21675== Command: ./kslab_test
==21675== 
sizeof A: 32
Time taken in alloc: 22266 free: 26356
==21675== 
==21675== HEAP SUMMARY:
==21675==     in use at exit: 0 bytes in 0 blocks
==21675==   total heap usage: 2 allocs, 2 frees, 4,160 bytes allocated
==21675== 
==21675== All heap blocks were freed -- no leaks are possible
==21675== 
==21675== For counts of detected and suppressed errors, rerun with: -v
==21675== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ 
```

Cycles for Alloc & Free: (Less than 100cycles!! I'm not too optimistic about it because I've not carefully crafted the code to make 
use of compiler optimizations etc. And also, this is SMALL Slab case, wonder how much more it would be for Bigger ones, especially with
hashing and list search, etc) 
```
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 91 free: 75
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 153 free: 74
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 89 free: 75
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 151 free: 74
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 79 free: 65
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 33 free: 27
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 60 free: 26
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 32 free: 27
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 35 free: 31
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 88 free: 75
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 87 free: 75
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 32 free: 27
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 39 free: 33
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 40 free: 33
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 90 free: 75
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 39 free: 33
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
Time taken in alloc: 52 free: 23
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ 
```
