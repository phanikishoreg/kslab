#### Slab Allocator

This Slab-allocator is an unoptimized implementation of [Bon94](https://www.seas.gwu.edu/~gparmer/courses/f16_6907/slab.pdf).
Moreover, I've only implemented Small slabs, slabs with data size less than 1/8th of PAGE_SIZE, 
the optimized path from the paper and tested the basic case of kmem_cache_create(), kmem_cache_alloc(), kmem_cache_free(), and kmem_cache_destroy().

If I had enough time, I'd have worked on compiler optimization for this, especially constant propagation, loop unrolling etc using
preprocessor directives, which would have been a great exercise and learning experience! But deadlines/priorities!

For Big slabs, Works!!!, tested the same cases as for Small slabs.  
kmem_cache_create(), kmem_cache_alloc(), kmem_cache_free(), kmem_cache_destroy() - basic usage. 

Tests done:
1. Blocking/Non-blocking of kmem_cache_alloc() API.
2. Many (alloc->free) on a single objects and verifying previous constructed state: Small/Big Caches.
3. Many objects alloc->free on single cache and verifying previous constructed state: Small/Big Caches.

Not Tested:
1. Multiple slab allocations per cache
2. Multi-threaded env : Not implemented.

## Directory structure

1. include/kslab.h : Interface file exposed to the user
2. src/kslab.c : Implementation of Slab-allocator
3. test/kslab_test.c : Test program, testing basic implementation.
4. README.md: this file
5. Makefile, src/Makefile, test/Makefile: make files for compilation.
6. Other git objects and License files!

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

Valgrind:
```
Time taken in alloc: 55719 free: 36638
==29669== 
==29669== HEAP SUMMARY:
==29669==     in use at exit: 8,320 bytes in 4 blocks
==29669==   total heap usage: 14 allocs, 10 frees, 115,072 bytes allocated
==29669== 
==29669== 4,096 bytes in 1 blocks are possibly lost in loss record 3 of 4
==29669==    at 0x402C580: memalign (in /usr/lib/valgrind/vgpreload_memcheck-x86-linux.so)
==29669==    by 0x8048DBB: __create_small_slab (kslab.c:162)
==29669==    by 0x8048DBB: kmem_cache_grow (kslab.c:564)
==29669==    by 0x80490DA: kmem_cache_create (kslab.c:296)
==29669==    by 0x8048F2B: __create_big_slab (kslab.c:202)
==29669==    by 0x8048F2B: kmem_cache_grow (kslab.c:565)
==29669==    by 0x80490DA: kmem_cache_create (kslab.c:296)
==29669==    by 0x8049872: test_big_cache_oneobj (kslab_test.c:75)
==29669==    by 0x80485E4: main (kslab_test.c:219)
==29669== 
==29669== 4,096 bytes in 1 blocks are possibly lost in loss record 4 of 4
==29669==    at 0x402C580: memalign (in /usr/lib/valgrind/vgpreload_memcheck-x86-linux.so)
==29669==    by 0x8048DBB: __create_small_slab (kslab.c:162)
==29669==    by 0x8048DBB: kmem_cache_grow (kslab.c:564)
==29669==    by 0x80490DA: kmem_cache_create (kslab.c:296)
==29669==    by 0x8048E8C: __create_big_slab (kslab.c:207)
==29669==    by 0x8048E8C: kmem_cache_grow (kslab.c:565)
==29669==    by 0x80490DA: kmem_cache_create (kslab.c:296)
==29669==    by 0x8049872: test_big_cache_oneobj (kslab_test.c:75)
==29669==    by 0x80485E4: main (kslab_test.c:219)
==29669== 
==29669== LEAK SUMMARY:
==29669==    definitely lost: 0 bytes in 0 blocks
==29669==    indirectly lost: 0 bytes in 0 blocks
==29669==      possibly lost: 8,192 bytes in 2 blocks
==29669==    still reachable: 128 bytes in 2 blocks
==29669==         suppressed: 0 bytes in 0 blocks
==29669== Reachable blocks (those to which a pointer was found) are not shown.
==29669== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==29669== 
==29669== For counts of detected and suppressed errors, rerun with: -v
==29669== Use --track-origins=yes to see where uninitialised values come from
==29669== ERROR SUMMARY: 4 errors from 4 contexts (suppressed: 0 from 0)
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ 
```
**Looks like memalign causes fragmentation(and loss) due to aligned_alloc used for Small slabs.**

Test output:
```
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ ./kslab_test 
sizeof A: 32
sizeof B: 1024
:1804289383
1804289383:846930886
846930886:1681692777
1681692777:1714636915
1714636915:1957747793
1957747793:424238335
424238335:719885386
719885386:1649760492
1649760492:596516649
596516649:1189641421
1189641421:1025202362
1025202362:1350490027
1350490027:783368690
783368690:1102520059
1102520059:2044897763
2044897763:1967513926
1967513926:1365180540
1365180540:1540383426
1540383426:304089172
304089172:1303455736
1303455736:35005211
35005211:521595368
521595368:294702567
294702567:1726956429
1726956429:336465782
336465782:861021530
861021530:278722862
278722862:233665123
233665123:2145174067
2145174067:468703135
468703135:1101513929
1101513929:1801979802
1801979802:1315634022
1315634022:635723058
635723058:1369133069
1369133069:1125898167
Time taken in alloc: 35 free: 25
:2077486715
2077486715:1373226340
1373226340:1631518149
1631518149:200747796
200747796:289700723
289700723:1117142618
1117142618:168002245
168002245:150122846
150122846:439493451
439493451:990892921
990892921:1760243555
1760243555:1231192379
1231192379:1622597488
1622597488:111537764
111537764:338888228
338888228:2147469841
Time taken in alloc: 58 free: 42
phani@phani-thinkpad:~/courses/fall16/6907_specialtopics_os/kslab/test$ 
```
** Both alloc/free take less than 100cycles in common-case **

