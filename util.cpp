#ifndef arr_count
#define arr_count(a) (sizeof (a) / sizeof *(a))
#endif

static bool streq(char const *a, char const *b) {
	return strcmp(a, b) == 0;
}

// on 16-bit systems, this is 16383. on 32/64-bit systems, this is 1073741823
// it is unusual to have a string that long.
#define STRLEN_SAFE_MAX (UINT_MAX >> 2)

// safer version of strcat. dst_sz includes a null terminator.
static void str_cat(char *dst, size_t dst_sz, char const *src) {
	size_t dst_len = strlen(dst), src_len = strlen(src);

	// make sure dst_len + src_len + 1 doesn't overflow
	if (dst_len > STRLEN_SAFE_MAX || src_len > STRLEN_SAFE_MAX) {
		assert(0);
		return;
	}

	if (dst_len >= dst_sz) {
		// dst doesn't actually contain a null-terminated string!
		assert(0);
		return;
	}

	if (dst_len + src_len + 1 > dst_sz) {
		// number of bytes left in dst, not including null terminator
		size_t n = dst_sz - dst_len - 1;
		memcpy(dst + dst_len, src, n);
		dst[dst_sz - 1] = 0; // dst_len + n == dst_sz - 1
	} else {
		memcpy(dst + dst_len, src, src_len);
		dst[dst_len + src_len] = 0;
	}
}

// safer version of strncpy. dst_sz includes a null terminator.
static void str_cpy(char *dst, size_t dst_sz, char const *src) {
	size_t srclen = strlen(src);
	size_t n = srclen; // number of bytes to copy
	
	if (dst_sz == 0) {
		assert(0);
		return;
	}

	if (dst_sz-1 < n)
		n = dst_sz-1;
	memcpy(dst, src, n);
	dst[n] = 0;
}

/* 
returns the first instance of needle in haystack, ignoring the case of the characters,
or NULL if the haystack does not contain needle
WARNING: O(strlen(haystack) * strlen(needle))
*/
static char *stristr(char const *haystack, char const *needle) {
	size_t needle_len = strlen(needle), haystack_len = strlen(haystack), i, j;
	
	if (needle_len > haystack_len) return NULL; // a larger string can't fit in a smaller string

	for (i = 0; i <= haystack_len - needle_len; ++i) {
		char const *p = haystack + i, *q = needle;
		bool match = true;
		for (j = 0; j < needle_len; ++j) {
			if (tolower(*p) != tolower(*q)) {
				match = false;
				break;
			}
			++p;
			++q;
		}
		if (match)
			return (char *)haystack + i;
	}
	return NULL;
}

static void print_bytes(u8 *bytes, size_t n) {
	u8 *b, *end;
	for (b = bytes, end = bytes + n; b != end; ++b)
		printf("%x ", *b);
	printf("\n");
}

/*
does this predicate hold for all the characters of s. predicate is int (*)(int) instead
of bool (*)(char) so that you can pass isprint, etc. to it.
*/
static bool str_satisfies(char const *s, int (*predicate)(int)) {
	char const *p;
	for (p = s; *p; ++p)
		if (!predicate(*p))
			return false;
	return true;
}

static int is_lowercase_hex_digit(int c) {
	return isdigit(c) || (c >= 'a' && c <= 'f');
}

// can c be a character in an integer (i.e. a '-' or a digit?)
static int is_int_char(int c) {
	return isdigit(c) || c == '-';
}

// can c be a character in a positive float (i.e. a '.' or digit?)
static int is_positive_float_char(int c) {
	return isdigit(c) || c == '.';
}

// can c be a character in a float (i.e. a '-', '.', or digit?)
static int is_float_char(int c) {
	return isdigit(c) || c == '-' || c == '.';
}

// converts s to a 32-bit signed integer. sets *success to whether s is actually an integer, if success is not NULL.
// returns 0 on failure in addition to setting *success to false
// fails if s is -2^31 even though that technically does fit in an i32
static i32 str_to_i32(char const *s, bool *success) {
	bool negative = false;
	if (success) *success = false;
	if (s[0] == '-') {
		negative = true;
		++s;
	}
	
	if (s[0] == '\0') { // empty number
		return 0;
	}

	{
		size_t len = strlen(s);
		if (len > 10 || (len == 10 && s[0] > '2')) {
			// overflow
			return 0;
		}
	}

	if (!str_satisfies(s, isdigit)) { // non-digits in number
		return 0;
	}
	
	{
		unsigned long ul = 0;
		i32 magnitude;
		int ret = sscanf(s, "%lu", &ul);
		(void)ret;
		assert(ret == 1);
		if (ul > I32_MAX)
			return 0;
		magnitude = (i32)ul;
		if (success) *success = true;
		return negative ? -magnitude : +magnitude;
	}
}

// qsort comparison function for strings, usually case insensitive 
static int qsort_stricmp(const void *av, const void *bv) {
	char const *a = *(char const *const *)av;
	char const *b = *(char const *const *)bv;
#ifdef __unix__
	return strcasecmp(a, b);
#elif defined _MSC_VER
	return _stricmp(a, b);
#else
	// give up on case insensitivity 
	return strcmp(a, b);
#endif
}

static void fwrite_bool(FILE *fp, bool x) {
	fwrite(&x, sizeof x, 1, fp);
}

static void fwrite_u32(FILE *fp, u32 x) {
	fwrite(&x, sizeof x, 1, fp);
}

static void fwrite_float(FILE *fp, float x) {
	fwrite(&x, sizeof x, 1, fp);
}

static void fwrite_v2(FILE *fp, v2 v) {
	fwrite(&v, sizeof v, 1, fp);
}

static void fread_bool(FILE *fp) {
	bool x;
	fread(&x, sizeof x, 1, fp);
	return x;
}

static void fread_u32(FILE *fp) {
	u32 x;
	fread(&x, sizeof x, 1, fp);
	return x;
}

static void fread_float(FILE *fp) {
	float x;
	fread(&x, sizeof x, 1, fp);
	return x;
}

static void fread_v2(FILE *fp) {
	v2 v;
	fread(&v, sizeof v, 1, fp);
	return v;
}

