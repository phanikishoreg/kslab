/*
 * Slab-allocator based on "The Slab Allocator" from Jeff Bonwick
 *
 * GPL v3
 * Copyright 2016 - Phani Gadepalli
 */
#ifndef __KSLAB_H__
#define __KSLAB_H__

/* Slab-allocator Front-end API */
#define CACHE_NAME_MAX 32

enum alloc_flags {
	KM_SLEEP   = (1),
	KM_NOSLEEP = (1 << 1),
};

struct kmem_slab;
struct hash_table;

struct kmem_cache {
	char name[CACHE_NAME_MAX + 1];
	size_t sz_aligned, sz;

	/* unused incase of small slabs */
	struct hash_table *htbl;

	void (*ctor)(void *, size_t);
	void (*dtor)(void *, size_t);
	/* 
	 * slab_full -> points to first slab that is full.. 
	 * slab_free -> first slab that has 1 or more free bufs
	 */
	struct kmem_slab *slab_full, *slab_free;
};

extern struct kmem_cache *kmem_cache_create(char *name, size_t sz, int align, void (*ctor)(void *, size_t), void (*dtor)(void *, size_t));
extern void kmem_cache_destroy(struct kmem_cache *kcp);

extern void *kmem_cache_alloc(struct kmem_cache *kcp, int flags);
extern void kmem_cache_free(struct kmem_cache *kcp, void *buf);

#endif /* __KSLAB_H__ */
