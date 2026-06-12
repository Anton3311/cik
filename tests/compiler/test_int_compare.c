#include <stdint.h>

void assert(uint64_t);

int main(int argc, char* argv[]) {
	// i8
	assert(10i8 == 10i8);
	assert(10i8 != 5i8);
	assert(10i8 > 5i8);
	assert(5 < 10i8);
	assert(10i8 >= 5i8);
	assert(5 <= 10i8);
	assert(10i8 >= 10i8);
	assert(10i8 <= 10i8);

#if 0
	// i16
	assert(10i16 == 10i16);
	assert(10i16 != 5i16);
	assert(10i16 > 5i16);
	assert(5 < 10i16);
	assert(10i16 >= 5i16);
	assert(5 <= 10i16);
	assert(10i16 >= 10i16);
	assert(10i16 <= 10i16);
#endif

	// i32
	assert(10i32 == 10i32);
	assert(10i32 != 5i32);
	assert(10i32 > 5i32);
	assert(5 < 10i32);
	assert(10i32 >= 5i32);
	assert(5 <= 10i32);
	assert(10i32 >= 10i32);
	assert(10i32 <= 10i32);

	// i64
	assert(10i64 == 10i64);
	assert(10i64 != 5i64);
	assert(10i64 > 5i64);
	assert(5 < 10i64);
	assert(10i64 >= 5i64);
	assert(5 <= 10i64);
	assert(10i64 >= 10i64);
	assert(10i64 <= 10i64);

	// ui8
	assert(10ui8 == 10ui8);
	assert(10ui8 != 5ui8);
	assert(10ui8 > 5ui8);
	assert(5 < 10ui8);
	assert(10ui8 >= 5ui8);
	assert(5 <= 10ui8);
	assert(10ui8 >= 10ui8);
	assert(10ui8 <= 10ui8);

#if 0
	// ui16
	assert(10ui16 == 10ui16);
	assert(10ui16 != 5ui16);
	assert(10ui16 > 5ui16);
	assert(5 < 10ui16);
	assert(10ui16 >= 5ui16);
	assert(5 <= 10ui16);
	assert(10ui16 >= 10ui16);
	assert(10ui16 <= 10ui16);
#endif

	// ui32
	assert(10ui32 == 10ui32);
	assert(10ui32 != 5ui32);
	assert(10ui32 > 5ui32);
	assert(5 < 10ui32);
	assert(10ui32 >= 5ui32);
	assert(5 <= 10ui32);
	assert(10ui32 >= 10ui32);
	assert(10ui32 <= 10ui32);

	// ui64
	assert(10ui64 == 10ui64);
	assert(10ui64 != 5ui64);
	assert(10ui64 > 5ui64);
	assert(5 < 10ui64);
	assert(10ui64 >= 5ui64);
	assert(5 <= 10ui64);
	assert(10ui64 >= 10ui64);
	assert(10ui64 <= 10ui64);

	return 0;
}
