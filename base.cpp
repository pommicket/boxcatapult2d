// this file includes functions, etc. used just about everywhere

#if DEBUG
#define logln(...) printf(__VA_ARGS__), printf("\n");
#else
#define logln(...)
#endif

// allocates aligned temporary memory 
static u8 *tmp_alloc(State *state, size_t bytes) {
	u32 used = state->tmp_mem_used;
	u32 max_aligns_needed = (u32)(bytes + sizeof(MaxAlign) - 1) / (u32)sizeof(MaxAlign); // = ceil(bytes / sizeof(MaxAlign)) 
	MaxAlign *ret = state->tmp_mem + used;
	if (bytes == 0) {
		return NULL;
	}
	if (used + max_aligns_needed > arr_count(state->tmp_mem)) {
		assert(0);
		return NULL;
	}
	state->tmp_mem_used += max_aligns_needed;
	memset(ret, 0, bytes);
	return (u8 *)ret;
}

/* 
these functions save and restore the state of the temporary memory.
*/
static u32 tmp_push(State *state) {
	return state->tmp_mem_used;
}

static void tmp_pop(State *state, u32 mark) {
	state->tmp_mem_used = mark;
}

#define tmp_alloc_object(state, type) ((type *)tmp_alloc((state), sizeof(type)))
#define tmp_alloc_arr(state, type, n) ((type *)tmp_alloc((state), (n) * sizeof(type)))
#define calloc_object(type) ((type *)calloc(1, sizeof(type)))
#define calloc_arr(type, n) ((type *)calloc((n), sizeof(type)))
