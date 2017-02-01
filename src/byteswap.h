#ifndef BYTESWAP_H
#define BYTESWAP_H

template<typename T>
static inline __attribute((always_inline))
T byte_swap(T t)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return t;
#else
	struct internal
	{
		BLAZING
		int64_t _byte_swap(int64_t x)
		{
			x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
			x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
			x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
			return x;
		}

		BLAZING
		int32_t _byte_swap(int32_t x)
		{
			x = (x & 0x0000FFFF) << 16 | (x & 0xFFFF0000) >> 16;
			x = (x & 0x00FF00FF) << 8  | (x & 0xFF00FF00) >> 8;
			return x;
		}

		BLAZING
		int16_t _byte_swap(int16_t t)
		{
			return (t & 0xFF00) >> 8 | (t & 0x00FF) << 8;
		}

		BLAZING
		int8_t _byte_swap(int8_t t)
		{
			return t;
		}

		BLAZING __attribute((flatten))
		uint64_t _byte_swap(uint64_t x) { return (uint64_t) _byte_swap((int64_t) x); }
		BLAZING __attribute((flatten))
		uint32_t _byte_swap(uint32_t x) { return (uint32_t) _byte_swap((int32_t) x); }
		BLAZING __attribute((flatten))
		uint16_t _byte_swap(uint16_t x) { return (uint16_t) _byte_swap((int16_t) x); }
		BLAZING __attribute((flatten))
		uint8_t  _byte_swap(uint8_t  x) { return x; }
#if __BYTE_ORDER__ != __ORDER_BIG_ENDIAN__
		WARM __attribute((flatten))
		bool isLittleEndian()
		{
			int i = 1;
			return (int)*((unsigned char *)&i)==1;
		}
#endif
	};

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	 return internal::_byte_swap(t);
#else
	return internal::isLittleEndian() ? t : internal::_byte_swap(t);
#endif //__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#endif //__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
}

#endif // BYTESWAP_H
