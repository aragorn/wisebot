/* $Id$ */
/* calculate a long checksum for a memoryblock.
 * used to verify parser header and dynamic hash header */
#define TEST(a)			((a)?1:0)
unsigned long checksum(unsigned long crc, const void *mem, unsigned int count)
{
	for (; count--; mem++)
		crc = ((crc << 1) + *((unsigned char *)mem)) + 
				TEST(crc & ((unsigned long)1L << (8 * sizeof(unsigned long) - 1)));
	return crc;
}
