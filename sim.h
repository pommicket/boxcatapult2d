// enums with a specified width are a clang C extension & available in C++11
#if defined __clang__ || __cplusplus >= 201103L
#define ENUM_U8 typedef enum : u8 
#define ENUM_U8_END(name) name
#define ENUM_U16 typedef enum : u16
#define ENUM_U16_END(name) name
#else
#define ENUM_U8 enum
#define ENUM_U8_END(name) ; typedef u8 name
#define ENUM_U16 enum
#define ENUM_U16_END(name) ; typedef u16 name
#endif


#ifdef __GNUC__
#define maybe_unused __attribute__((unused))
#else
#define maybe_unused
#endif

typedef struct {
	v2 center;
	float size;
	float angle;
} Platform;

typedef struct {
	bool initialized;
	i32 win_width, win_height; // width,height of window

	u32 nplatforms;
	Platform platforms[1000];

	u32 tmp_mem_used; // this is not measured in bytes, but in MaxAligns 
#define TMP_MEM_BYTES (4L<<20)
	MaxAlign tmp_mem[TMP_MEM_BYTES / sizeof(MaxAlign)];
#if DEBUG
	u32 magic_number;
	#define MAGIC_NUMBER 0x1234ACAB
#endif
} State;

