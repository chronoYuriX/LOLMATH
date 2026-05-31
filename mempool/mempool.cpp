#include <stdlib.h>
#include <assert.h>
#include "platform.h"

#include <stdio.h>


struct MEMPOOL {
    struct MEMCHUNK {
		void* data;
		size_t size, obj_size;
		bool ok;
		MEMCHUNK(size_t obj_size, size_t objs): size(obj_size * objs), obj_size(obj_size) {
			data = malloc(size);
			ok = data ? 1 : 0;
		}
		~MEMCHUNK() {
			if (ok) {
				if (data) free(data);
				ok = 0;
			}
		}
	};
	MEMPOOL()
};












