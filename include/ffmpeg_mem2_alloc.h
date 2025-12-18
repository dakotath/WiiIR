#ifndef FFMPEG_MEM2_ALLOC_H
#define FFMPEG_MEM2_ALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize MEM2 allocator.
 * Must be called BEFORE any FFmpeg function.
 */
void ffmpeg_allocator_init(void);

/* Reset allocator (reclaim all memory).
 * Call after closing a video / freeing FFmpeg contexts.
 */
void ffmpeg_allocator_reset(void);

/* FFmpeg malloc replacements (via --malloc-prefix) */
void *ffmpeg_malloc(size_t size);
void *ffmpeg_calloc(size_t nmemb, size_t size);
void *ffmpeg_realloc(void *ptr, size_t size);
void  ffmpeg_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* FFMPEG_MEM2_ALLOC_H */
