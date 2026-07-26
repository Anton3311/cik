#include <stdint.h>

extern void assert(uint64_t);

int main(int argc, char* argv[]) {
	assert((0b101i8 << 3i8) == 0b101000i8);
	assert((0b101i16 << 3i16) == 0b101000i16);
	assert((0b101i32 << 3i32) == 0b101000i32);
	assert((0b101i64 << 3i64) == 0b101000i64);

	assert((0b101ui8 << 3ui8) == 0b101000ui8);
	assert((0b101ui16 << 3ui16) == 0b101000ui16);
	assert((0b101ui32 << 3ui32) == 0b101000ui32);
	assert((0b101ui64 << 3ui64) == 0b101000ui64);

	assert((0b101i8 >> 2i8) == 0b1i8);
	assert((0b101i16 >> 2i16) == 0b1i16);
	assert((0b101i32 >> 2i32) == 0b1i32);
	assert((0b101i64 >> 2i64) == 0b1i64);

	assert((0b101ui8 >> 2ui8) == 0b1ui8);
	assert((0b101ui16 >> 2ui16) == 0b1ui16);
	assert((0b101ui32 >> 2ui32) == 0b1ui32);
	assert((0b101ui64 >> 2ui64) == 0b1ui64);



	assert((0b101i8 << 8i8) == 0b0i8);
	assert((0b101i16 << 16i16) == 0b0i16);
	assert((0b101i32 << 32i32) == 0b101i32);
	assert((0b101i64 << 64i64) == 0b101i64);

	assert((0b101ui8 << 8ui8) == 0b0ui8);
	assert((0b101ui16 << 16ui16) == 0b0ui16);
	assert((0b101ui32 << 32ui32) == 0b101ui32);
	assert((0b101ui64 << 64ui64) == 0b101ui64);

	assert((0b101i8 >> 8i8) == 0b0i8);
	assert((0b101i16 >> 16i16) == 0b0i16);
	assert((0b101i32 >> 32i32) == 0b101i32);
	assert((0b101i64 >> 64i64) == 0b101i64);

	assert((0b101ui8 >> 8ui8) == 0b0ui8);
	assert((0b101ui16 >> 16ui16) == 0b0ui16);
	assert((0b101ui32 >> 32ui32) == 0b101ui32);
	assert((0b101ui64 >> 64ui64) == 0b101ui64);
	return 0;
}
