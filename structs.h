#ifndef structs_h
#define structs_h

#include "main.h"

typedef struct chunkWithoutCRC
{
	uint lenght;
	uchar *type;
	uchar *data;
} chunkWithoutCRC;

void closeChunk(chunkWithoutCRC *chunk)
{
	free(chunk->data);
	free(chunk->type);
}

#endif
