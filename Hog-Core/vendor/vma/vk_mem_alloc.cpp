#include "hgpch.h"

#define VMA_IMPLEMENTATION

#if HG_DEBUG
	#define VMA_DEBUG_LOG(format, ...) do { \
	       printf(format, __VA_ARGS__); \
	       printf("\n"); \
	   } while(false)
#endif

#include "vk_mem_alloc.h"