#ifndef typesOfCunks_h
#define typesOfCunks_h

#include "main.h"
#include "structs.h"

const uchar ihdr[] = { 'I', 'H', 'D', 'R' };
const uchar idat[] = { 'I', 'D', 'A', 'T' };

bool isPNG(const uchar *buff)
{
	const uchar png[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	for (uint i = 0; i < 8; i++)
	{
		if (buff[i] != png[i])
			return false;
	}
	return true;
}

bool isChunk(const uchar *buff, const uchar *chekedCunk)
{
	for (uint i = 0; i < 4; i++)
	{
		if (buff[i] != chekedCunk[i])
			return false;
	}
	return true;
}

#endif
