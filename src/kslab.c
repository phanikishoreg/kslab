/*
 * Slab-allocator implementation
 *
 * GPLv3
 * Copyright 2016 - Phani Gadepalli
 */
#include <stdlib.h>
#include <unistd.h>
#include <kslab.h>
#include <string.h>
#include <assert.h>

#ifdef __DEBUG__
#define KDEBUG printf
#else
#define KDEBUG(fmt,...)
#endif

#define USEC_WAIT 1000
#define ALLOCD 1
#define NPAGES 8
#define NBUFS 8
#define HASH_TABLE_SIZE (PAGE_SIZE)
#define HASH_KEY(v) (((unsigned)v) % HASH_TABLE_SIZE)

/* Assuming PAGE_SIZE is a power of 2 */
#define PAGE_SIZE (getpagesize())
#define ALIGN(sz, align) (align > 0 ? (align < sz ? (sz + (sz % align)) : align) : sz)
#define IS_SMALL(cache) (cache->sz < (PAGE_SIZE >> 3))
#define SMALL_KMEM_SLAB(x) ((struct kmem_slab *)(x + PAGE_SIZE - sizeof(struct kmem_slab)))
#define SMALL_BUF_TO_SLAB(x) (((unsigned)x) & ~(PAGE_SIZE - 1))

struct hash_table {
	struct kmem_bufctl *this;
};

struct kmem_bufctl {
	
	void *buf;
	struct kmem_bufctl *next;
	struct kmem_slab *container;
};

struct kmem_slab {

	struct kmem_cache *type;
	unsigned used, free;

	struct kmem_slab *next, *prev;

	struct kmem_bufctl *freelist;
};

struct kmem_bufctl *
hash_del(struct hash_table *htbl, void *buf)
{
	int key;
	struct kmem_bufctl *tmp, *prev;

	assert(htbl && buf);

	key = HASH_KEY(buf);			

	tmp = (htbl + key)->this;
	prev = NULL;
	while (tmp != NULL) {
		if (tmp->buf == buf) {
			if (prev) prev->next = tmp->next;
			else (htbl + key)->this = tmp->next;
			return tmp;
		}

		prev = tmp;
		tmp = tmp->next;
	}

	return NULL;
}

void
hash_add(struct hash_table *htbl, struct kmem_bufctl *bfc)
{
	void *buf;
	struct kmem_bufctl *tmp;
	int key;

	assert (htbl && bfc);

	buf = bfc->buf;
	assert(buf);

	key = HASH_KEY(buf);

	tmp = (htbl + key)->this;
	bfc->next = tmp;
	(htbl + key)->this = bfc;

	return;
}

static struct kmem_cache *slab_cache, *bufctl_cache;

struct kmem_slab *kmem_cache_grow(struct kmem_cache *kcp);
void kmem_cache_reap(struct kmem_cache *kcp, struct kmem_slab *slb);

/* circular doubly linked list basic funcs */
/* inserting before */
void
cdl_insert(struct kmem_slab *p, struct kmem_slab *q)
{
	q->prev = p->prev;
	q->next = p;

	p->prev->next = q;
	p->prev = q;
}

struct kmem_slab *
cdl_remove(struct kmem_slab *p)
{
	p->prev->next = p->next;
	p->next->prev = p->prev;

	return p;
}

/* singly linked lists basic funcs */
void
sl_insert(struct kmem_bufctl **p, struct kmem_bufctl *q)
{
	q->next = *p;
	*p = q;
}

struct kmem_bufctl *
sl_remove(struct kmem_bufctl **p)
{
	struct kmem_bufctl *x;

	x = *p;
	*p = (*p)->next;

	return x;
}

static inline struct kmem_slab *
__create_small_slab(struct kmem_cache *kcp)
{
	struct kmem_slab *new_slab;
	void *buf = NULL;
	size_t bufsz;
	int i;

	assert(kcp && IS_SMALL(kcp));

	buf = (void *)aligned_alloc(PAGE_SIZE, PAGE_SIZE);
	assert(buf);

	new_slab = (struct kmem_slab *) (buf + PAGE_SIZE - sizeof(struct kmem_slab));
	bufsz = (PAGE_SIZE - sizeof(struct kmem_slab));
	
	new_slab->type = kcp;
	new_slab->used = 0;
	new_slab->next = new_slab->prev = new_slab;
	/* This sz_aligned must include ALIGNED (sz + sizeof(int)) */
	new_slab->free = bufsz / kcp->sz_aligned; 

	for (i = 0 ; i < new_slab->free ; i ++) {
		/* construct objects */
		if (kcp->ctor) kcp->ctor((buf + i * kcp->sz_aligned), kcp->sz);
		*(unsigned *)((buf + i * kcp->sz_aligned) + kcp->sz) = (unsigned)(buf + (i + 1) * kcp->sz_aligned);

	}

	/* ugly as hell.. but couldn't think more to do it right */
	new_slab->freelist = (struct kmem_bufctl *) buf; 

	if (kcp->slab_free == NULL) kcp->slab_free = new_slab;
	else cdl_insert(kcp->slab_free, new_slab);

	return new_slab;
}

static inline struct kmem_slab *
__create_big_slab(struct kmem_cache *kcp)
{
	struct kmem_slab *new_slab, *gslb;
	struct kmem_bufctl *list, *tmp;
	size_t bufsz;
	void *buf;
	int i;

	assert(kcp && !IS_SMALL(kcp));

	if (!slab_cache) {
		slab_cache = kmem_cache_create("slab_cache", sizeof(struct kmem_slab), 0, NULL, NULL);
		assert(slab_cache);
	}

	if (!bufctl_cache) {
		bufctl_cache = kmem_cache_create("bufctl_cache", sizeof(struct kmem_bufctl), 0, NULL, NULL);
		assert(bufctl_cache);
	}

	new_slab = (struct kmem_slab *)kmem_cache_alloc(slab_cache, KM_NOSLEEP);
	if (new_slab == NULL) {
		gslb = kmem_cache_grow(slab_cache);
		assert(gslb);

		/* for multiple-slabs in slab cache */
		new_slab = (struct kmem_slab *)kmem_cache_alloc(slab_cache, KM_NOSLEEP);
	}
	assert(new_slab);

	new_slab->type = kcp;
	new_slab->used = 0;
	new_slab->next = new_slab->prev = new_slab;

	if (kcp->sz_aligned <= PAGE_SIZE) {
		bufsz = PAGE_SIZE * NPAGES; 
	} else {
		bufsz = kcp->sz_aligned * NBUFS;
	}

	buf = (void *)aligned_alloc(PAGE_SIZE, bufsz);
	assert(buf);

	new_slab->free = bufsz / kcp->sz_aligned;

	list = NULL;
	for (i = 0 ; i < new_slab->free ; i ++) {
		tmp = (struct kmem_bufctl *) kmem_cache_alloc(bufctl_cache, KM_NOSLEEP);
		if (tmp == NULL) {
			gslb = kmem_cache_grow(bufctl_cache);
			assert(gslb);
			
			/* for multiple-slabs in bufctl cache */
			tmp = (struct kmem_bufctl *) kmem_cache_alloc(bufctl_cache, KM_NOSLEEP);
		}
		assert(tmp);

		/* construct objects */
		if(kcp->ctor) kcp->ctor((buf + i * kcp->sz_aligned), kcp->sz);

		tmp->buf = buf + (i * kcp->sz_aligned);
		tmp->container = new_slab;
		tmp->next = list;
		list = tmp;
	}	

	new_slab->freelist = list;

	if (kcp->slab_free == NULL) kcp->slab_free = new_slab;
	else cdl_insert(kcp->slab_free, new_slab);

	return new_slab;
}

struct kmem_cache *
kmem_cache_create(char *name, size_t sz, int align,
		  void (*ctor)(void *, size_t),
		  void (*dtor)(void *, size_t))
{
	struct kmem_cache *kc;
	struct kmem_slab *slb;
	int i;

	kc = (struct kmem_cache *)malloc(sizeof(struct kmem_cache));
	assert(kc);

	strncpy(kc->name, name, CACHE_NAME_MAX);
	kc->sz = sz;
	kc->ctor = ctor;
	kc->dtor = dtor;

	kc->slab_full = kc->slab_free = NULL;
	
	if (IS_SMALL(kc)) {
		kc->sz_aligned = ALIGN(sz + sizeof(unsigned), align);
		kc->htbl = NULL; /* you don't need hash table here */

	} else {
		kc->sz_aligned = ALIGN(sz, align);

		kc->htbl = (struct hash_table *)malloc((sizeof(struct hash_table)) * HASH_TABLE_SIZE);
		assert(kc->htbl);
		memset(kc->htbl, 0, sizeof(struct hash_table) * HASH_TABLE_SIZE);
	}

	slb = (struct kmem_slab *)kmem_cache_grow(kc);
	assert(slb);

	return kc;
}

static inline void
__destroy_small_slab(struct kmem_cache *kcp, struct kmem_slab *slb)
{
	int i;
	void *buf;

	assert(kcp && IS_SMALL(kcp));
	assert(slb && slb->used == 0);

	slb = cdl_remove(slb);
	buf = (void *)SMALL_BUF_TO_SLAB(slb->freelist);
	for (i = 0 ; i < slb->free ; i ++) {
		/* destruct objects */
		if (kcp->dtor) kcp->dtor((buf + i * kcp->sz_aligned), kcp->sz);
	}

	free(buf);
	buf = NULL;

	return;
}

static inline void
__destroy_big_slab(struct kmem_cache *kcp, struct kmem_slab *slb)
{
	int i;
	struct kmem_bufctl *bfc;
	struct kmem_slab *sl;
	void *buf;

	assert(kcp && !IS_SMALL(kcp));
	assert(slb && slb->used == 0);

	bfc = slb->freelist;
	assert(bfc);

	buf = bfc->buf;

	slb = cdl_remove(slb);

	while (bfc != NULL) {
		if (bfc->buf < buf) buf = bfc->buf;
		
		if(kcp->dtor) kcp->dtor(bfc->buf, kcp->sz);
		kmem_cache_free(bufctl_cache, bfc);

		bfc = bfc->next;
	}

	free(buf);
	buf = NULL;

	kmem_cache_free(slab_cache, slb);

	return;
}

static inline void
__destroy_small_cache(struct kmem_cache *kcp)
{
	int i;
	void *buf;

	assert(kcp && IS_SMALL(kcp));
	assert(!kcp->slab_full);
	assert(kcp->slab_free->next == kcp->slab_free);

	kmem_cache_reap(kcp, kcp->slab_free);
	kcp->slab_free = NULL;

	free(kcp);
	/* to whomsoever this may concern: you've dangling pointer now */

	return;
}

static inline void
__destroy_big_cache(struct kmem_cache *kcp)
{
	assert(kcp && !IS_SMALL(kcp));
	assert(!kcp->slab_full);
	assert(kcp->slab_free->next == kcp->slab_free);

	kmem_cache_reap(kcp, kcp->slab_free);
	kcp->slab_free = NULL;

	if (kcp->htbl) free(kcp->htbl);
	kcp->htbl = NULL;

	free(kcp);
	/* to whomsoever this may concern: you've dangling pointer now */

	return;
}

void
kmem_cache_destroy(struct kmem_cache *kcp)
{
	assert(kcp);

	if (IS_SMALL(kcp)) return __destroy_small_cache(kcp);
	else return __destroy_big_cache(kcp);
}

static inline void *
__alloc_small_buf(struct kmem_cache *kcp, int flags)
{
	struct kmem_slab *tmp;
	void *buf;

	assert(kcp && IS_SMALL(kcp));

	tmp = kcp->slab_free;
	
	if (flags & KM_SLEEP) {
		/* Basic impl of Blocking, Sleep */
		while (!tmp || !tmp->free) usleep(USEC_WAIT);
	} else {
		if (!tmp || !tmp->free) return NULL;
	}

	tmp->used ++;
	tmp->free --;
	buf = (void *)tmp->freelist;
	*(unsigned *)(buf + kcp->sz) = ALLOCD;
	tmp->freelist = (struct kmem_bufctl *)(buf + kcp->sz_aligned);
	
	if (tmp->free == 0) {
		struct kmem_slab *p;

		p = cdl_remove(tmp);
		if (tmp == p) kcp->slab_free = NULL;
		if (kcp->slab_full == NULL) kcp->slab_full = p;
		else cdl_insert(kcp->slab_full, p); 
	} 

	return buf;
}

static inline void *
__alloc_big_buf(struct kmem_cache *kcp, int flags)
{
	struct kmem_slab *tmp;
	struct kmem_bufctl *tmpbf;
	void *buf;

	assert(kcp && !IS_SMALL(kcp));

	tmp = kcp->slab_free;
	
	if (flags & KM_SLEEP) {
		/* Basic impl of Blocking, Sleep */
		while (!tmp || !tmp->free) usleep(USEC_WAIT);
	} else {
		if (!tmp || !tmp->free) return NULL;
	}

	tmp->used ++;
	tmp->free --;
	tmpbf = tmp->freelist;
	tmp->freelist = tmp->freelist->next;
	buf = tmpbf->buf;

	hash_add(kcp->htbl, tmpbf);

	if (tmp->free == 0) {
		struct kmem_slab *p;

		p = cdl_remove(tmp);
		if (tmp == p) kcp->slab_free = NULL;
		if (kcp->slab_full == NULL) kcp->slab_full = p;
		else cdl_insert(kcp->slab_full, p); 
	} 

	return buf;
}

void *
kmem_cache_alloc(struct kmem_cache *kcp, int flags)
{
	assert(kcp);

	if (IS_SMALL(kcp)) return __alloc_small_buf(kcp, flags);
	else return __alloc_big_buf(kcp, flags);
}

static inline void
__free_any_buf(struct kmem_cache *kcp, struct kmem_slab *slb)
{
	int full = 0;

	if (slb->free == 0) full = 1;
	slb->free ++;
	slb->used --;

	if (slb->used == 0) {
		/* if this is the only slab, don't destroy it until kmem_cache_destroy() */
		if (kcp->slab_full == NULL && (slb->next == slb)) return;

		kmem_cache_reap(kcp, slb);
	} 
	if (full) {
		slb = cdl_remove(slb);
		if (slb == kcp->slab_full) kcp->slab_full = NULL;

		if (kcp->slab_free == NULL) kcp->slab_free = slb;
		else cdl_insert(kcp->slab_free, slb);
	}

}

static inline void
__free_small_buf(struct kmem_cache *kcp, void *buf)
{
	int full = 0;
	struct kmem_slab *slb;

	assert(kcp && buf);
	assert(*(unsigned *)(buf + kcp->sz) == (unsigned)ALLOCD);

	slb = SMALL_KMEM_SLAB(SMALL_BUF_TO_SLAB(buf));
	*((unsigned *)(buf + kcp->sz)) = (unsigned)(slb->freelist);
	/* crazy */
	slb->freelist = (struct kmem_bufctl *)buf;
	__free_any_buf(kcp, slb);

	return;
}

static inline void
__free_big_buf(struct kmem_cache *kcp, void *buf)
{
	int full = 0;
	struct kmem_bufctl *bfc;
	struct kmem_slab *slb;

	assert(kcp && buf);

	bfc = hash_del(kcp->htbl, buf);
	assert(bfc);

	slb = bfc->container;
	bfc->next = slb->freelist;
	slb->freelist = bfc;

	__free_any_buf(kcp, slb);
	return;
}

void
kmem_cache_free(struct kmem_cache *kcp, void *buf)
{
	assert(kcp && buf);

	if (IS_SMALL(kcp)) __free_small_buf(kcp, buf);
	else __free_big_buf(kcp, buf);
}

struct kmem_slab *
kmem_cache_grow(struct kmem_cache *kcp)
{
	assert(kcp);

	if (IS_SMALL(kcp)) return __create_small_slab(kcp);
	else return __create_big_slab(kcp);
}

void
kmem_cache_reap(struct kmem_cache *kcp, struct kmem_slab *slb)
{
	assert(kcp);

	if (IS_SMALL(kcp)) __destroy_small_slab(kcp, slb);
	else __destroy_big_slab(kcp, slb);
}
