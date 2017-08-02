
#include "common.h"
#include "memory_pool.h"

static inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size, uint align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

//////////////////////////////////////////////////////////////////////////////////
void* ngx_alloc(size_t size)
{
	void  *p = NULL;
	p = malloc(size);
	if(p == NULL)
	{
		PERROR("important error :: malloc\n ");
	}
	return p;
}

void* ngx_calloc(size_t size)
{
	void  *p = NULL;
	p = malloc(size);
	if(p == NULL)
	{
		PERROR("important error :: malloc\n ");
	}else{
		memset(p,0,size);
	}
	return p;
}
#if 1

#define mem_memalign_posix(alignment, size)		ngx_alloc(size)
#define mem_memalign(alignment, size)			ngx_alloc(size)

#else
void* mem_memalign_posix(size_t alignment, size_t size)
{
	void  *p;
    int    err;

	err = posix_memalign(&p, alignment, size);
	if (err) {
		p = NULL;
	}

	return p;
}

void* mem_memalign(size_t alignment, size_t size)
{
	void  *p;

	p = memalign(alignment, size);
	if (p = NULL) {
		PERROR("important error :: memalign\n ");
	}

	return p;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////

ngx_pool_t* ngx_create_pool(size_t size)
{
    ngx_pool_t  *p;

    p = (ngx_pool_t*)mem_memalign_posix(NGX_POOL_ALIGNMENT, size);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_t);
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->large = NULL;
    p->cleanup = NULL;

    return p;
}

void ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;

    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            c->handler(c->data);
        }
    }

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        free(p);

        if (n == NULL) {
            break;
        }
    }
}

void ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->large = NULL;
}

void* ngx_palloc(ngx_pool_t *pool, size_t size)
{
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 1);
    }

    return ngx_palloc_large(pool, size);
}

void* ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);
    }

    return ngx_palloc_large(pool, size);
}

static inline void* ngx_palloc_small(ngx_pool_t *pool, size_t size, uint align)
{
    u_char      *m;
    ngx_pool_t  *p;

    p = pool->current;

    do {
        m = p->d.last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return ngx_palloc_block(pool, size);
}

static void* ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new_;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = (u_char*)mem_memalign(NGX_POOL_ALIGNMENT, psize);
    if (m == NULL) {
        return NULL;
    }

    new_ = (ngx_pool_t *) m;

    new_->d.end = m + psize;
    new_->d.next = NULL;
    new_->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new_->d.last = m + size;

    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }

    p->d.next = new_;

    return m;
}

static void* ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    uint	          n;
    ngx_pool_large_t  *large;

    p = ngx_alloc(size);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = (ngx_pool_large_t*)ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

void* ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;

    p = mem_memalign_posix(alignment, size);
    if (p == NULL) {
        return NULL;
    }

    large = (ngx_pool_large_t*)ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

int ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            free(l->alloc);
            l->alloc = NULL;

            return 0;
        }
    }

    return -5;
}

void* ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        memset(p, 0, size);
    }

    return p;
}

ngx_pool_cleanup_t* ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

    c = (ngx_pool_cleanup_t*)ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    return c;
}

void ngx_pool_run_cleanup_file(ngx_pool_t *p, int fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = (ngx_pool_cleanup_file_t*)(c->data);

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}

void ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = (ngx_pool_cleanup_file_t*)data;

    if (close(c->fd) == -1) {
		PERROR("close \"%s\" failed",c->name);
    }
}

void ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = (ngx_pool_cleanup_file_t*)data;

    int  err;
	
	PDEBUG("file cleanup: fd:%d %s\n",c->fd, c->name);
    if (unlink(c->name) == -1) {
        err = errno;

        if (err != ENOENT) {
			PERROR("unlink \"%s\" failed", c->name);
        }
    }

    if (close(c->fd) == -1) {
		PERROR("close \"%s\" failed", c->name);
    }
}


