
#ifndef __PROGRAM_FRAMEWORK_MEMORYPOOL_H__
#define __PROGRAM_FRAMEWORK_MEMORYPOOL_H__
#ifdef __cplusplus
extern "C" {
#endif

#define NGX_ALIGNMENT   sizeof(unsigned long)

#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((u_long) (p) + ((u_int) a - 1)) & ~((u_int) a - 1))

typedef struct ngx_pool_s            ngx_pool_t;


#define NGX_MAX_ALLOC_FROM_POOL  (4096 - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)

typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;
    void                 *data;
    ngx_pool_cleanup_t   *next;
};

typedef struct ngx_pool_large_s  ngx_pool_large_t;

struct ngx_pool_large_s {
    ngx_pool_large_t     *next;
    void                 *alloc;
};


typedef struct {
    u_char               *last;
    u_char               *end;
    ngx_pool_t           *next;
    uint				 failed;
} ngx_pool_data_t;


struct ngx_pool_s {
    ngx_pool_data_t       d;
    size_t                max;
    ngx_pool_t           *current;
    ngx_pool_large_t     *large;
    ngx_pool_cleanup_t   *cleanup;
};


typedef struct {
    int              	 fd;
    char                 *name;
} ngx_pool_cleanup_file_t;

void *ngx_alloc(size_t size);
void *ngx_calloc(size_t size);

void* mem_memalign_posix(size_t alignment, size_t size);
void* mem_memalign(size_t alignment, size_t size);

ngx_pool_t *ngx_create_pool(size_t size);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
int ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, int fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);


#ifdef __cplusplus
}
#endif
#endif

