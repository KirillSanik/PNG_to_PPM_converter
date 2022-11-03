#ifndef metods_h
#define metods_h

#include "main.h"
#include "structs.h"
#include "typesOfCunks.h"

int createBuff(size_t size, uchar **buff)
{
	if (!(*buff = malloc(size * sizeof(uchar))))
	{
		// не выделилась память
		fprintf(stderr, "no memory allocated");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	return ERROR_SUCCESS;
}

int resize(size_t size, uchar **buff)
{
	uchar *temp = realloc(*buff, sizeof(uchar) * size);
	if (temp)
	{
		*buff = temp;
	}
	else
	{
		fprintf(stderr, "error of reallocate memory");
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	return ERROR_SUCCESS;
}

int readBytes(uint cntBytes, uLong *startReadInd, uchar *buff, FILE *readerOfFile)
{
	// move pointer
	if (fseek(readerOfFile, *startReadInd, SEEK_SET))
	{
		// ошибка перемещения указателя
		fclose(readerOfFile);
		fprintf(stderr, "pointer move error");
		return ERROR_UNKNOWN;
	}
	uLong tmpCntBytes;
	// read to buff
	if ((tmpCntBytes = fread(buff, sizeof(uchar), cntBytes, readerOfFile)) != cntBytes)
	{
		if (feof(readerOfFile))
		{
			// конец файла, не удалось считать нужное количество
			*startReadInd += tmpCntBytes;
			fclose(readerOfFile);
			fprintf(stderr, "end of file, file corrupted");
			return ERROR_OUTOFMEMORY;
		}
		else
		{
			// неведомая ошибка fread
			fclose(readerOfFile);
			fprintf(stderr, "unknown error of file");
			return ERROR_UNKNOWN;
		}
	}
	*startReadInd += tmpCntBytes;
	return ERROR_SUCCESS;
}

uint getLen(uchar *buff, uint start)
{
	uint res = 0;
	uint tmpValue = 16777216;
	for (uint i = start; i < 4 + start; i++, tmpValue /= 256)
	{
		res += buff[i] * tmpValue;
	}
	return res;
}

void copyArr(uchar *res, const uchar *buff, size_t start, size_t size)
{
	for (size_t i = start; i < start + size; i++)
		res[i - start] = buff[i];
	return;
}

int findChunk(const uchar *chunk, uLong *currPos, FILE *readerOfFile)
{
	const uint sizeOfBuff = 2048;
	uchar tmpBuff[2048];

	long tmpCntBytes = -1;
	while (tmpCntBytes != 0)
	{
		// pointer move
		if (fseek(readerOfFile, *currPos, SEEK_SET))
		{
			// ошибка перемещения указателя
			fprintf(stderr, "pointer move error");
			fclose(readerOfFile);
			return ERROR_UNKNOWN;
		}
		// read to tmpBuff
		if ((tmpCntBytes = fread(tmpBuff, sizeof(uchar), sizeOfBuff, readerOfFile)) != sizeOfBuff)
		{
			if (ferror(readerOfFile))
			{
				// неведомая ошибка fread
				fprintf(stderr, "unknown error of file");
				fclose(readerOfFile);
				return ERROR_UNKNOWN;
			}
		}
		if (tmpCntBytes < 4)
		{
			return ERROR_NOT_FOUND;
		}
		*currPos += tmpCntBytes;
		int tmpInd = -1;
		// chek subArrays
		for (uint i = 0; i < tmpCntBytes - 3; i++)
		{
			// copyArr(tmp, tmpBuff, i, 4);
			if (isChunk((tmpBuff + i), chunk))
			{
				tmpInd = i;
				break;
			}
		}
		if (tmpInd != -1)
		{
			*currPos = *currPos - tmpCntBytes + tmpInd;
			return ERROR_SUCCESS;
		}
		*currPos -= 3;
	}
	return ERROR_NOT_FOUND;
}

int readChunk(chunkWithoutCRC *chunk, uLong *currPos, uchar *buff, FILE *readerOfFile)
{
	// read len
	int err;
	if ((err = readBytes(4, currPos, buff, readerOfFile)) != ERROR_SUCCESS)
	{
		// ошибка выведена в функции и закрыт файл
		return err;
	}
	chunk->lenght = getLen(buff, 0);

	// read type
	if ((err = readBytes(4, currPos, buff, readerOfFile)) != ERROR_SUCCESS)
	{
		// ошибка выведена в функции и закрыт файл
		return err;
	}
	// copy type
	if (createBuff(4, &chunk->type) != ERROR_SUCCESS)
	{
		// ошбика выведенна в функции
		fclose(readerOfFile);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	copyArr(chunk->type, buff, 0, 4);

	// use buff13 or create huge buff
	uchar *tmpBuff;
	if (chunk->lenght <= 13)
	{
		tmpBuff = buff;
	}
	else if ((err = createBuff(chunk->lenght, &tmpBuff)) != ERROR_SUCCESS)
	{
		// ошибка выведена в функции
		fclose(readerOfFile);
		return err;
	}

	// read data
	if ((err = readBytes(chunk->lenght, currPos, tmpBuff, readerOfFile)) != ERROR_SUCCESS)
	{
		// ошибка выведена в функции и закрыт файл
		return err;
	}
	chunk->data = tmpBuff;

	return ERROR_SUCCESS;
}

bool chekMode1(long i, size_t width, uchar shift)
{
	return 1 <= i % width && i % width <= shift;
}

bool chekMode2(long i, size_t width)
{
	return i / width == 0;
}

bool chekMode3(long i, size_t width, uchar shift)
{
	return (i / width == 0) || (1 <= i % width && i % width <= shift);
}

uchar calcMode4(uchar prev, uchar upper, uchar prevUpper)
{
	int p = prev + upper - prevUpper;
	int pPrev = abs(p - prev);
	int pUpper = abs(p - upper);
	int pPrevUpper = abs(p - prevUpper);
	if (pPrev <= pUpper && pPrev <= pPrevUpper)
		return prev;
	else if (pUpper <= pPrevUpper)
		return upper;
	return prevUpper;
}

int procFilters(uchar *uncomressBytes, uLong zlibWindowSize, size_t height, size_t width, uchar typeOfColour)
{
	uchar currMode = 0;
	uchar prevShift = (typeOfColour == 0) ? 1 : 3;
	size_t tmpHeight = height;
	size_t tmpWidth = width * prevShift + 1;
	// printf("size: %zu\n", tmpHeight * tmpWidth);

	// filter processing
	for (long i = 0; i < zlibWindowSize; i++)
	{
		if (i % tmpWidth == 0)
		{
			currMode = uncomressBytes[i];
			// printf("str: %lu, mode: %i\n", i / (tmpHeight * 3), currMode);
			continue;
		}
		uchar leftByte = 0;
		uchar upperByte = 0;
		uchar leftUpperByte = 0;
		if (!chekMode1(i, tmpWidth, prevShift))
			leftByte = uncomressBytes[i - prevShift];
		if (!chekMode2(i, tmpWidth))
			upperByte = uncomressBytes[i - tmpWidth];
		if (!chekMode3(i, tmpWidth, prevShift))
			leftUpperByte = uncomressBytes[i - tmpWidth - prevShift];
		switch (currMode)
		{
		case 1:
			uncomressBytes[i] += leftByte;
			break;
		case 2:
			uncomressBytes[i] += uncomressBytes[i - tmpWidth];
			break;
		case 3:
			uncomressBytes[i] += (int)(leftByte + upperByte) / 2;
			break;
		case 4:
			uncomressBytes[i] += calcMode4(leftByte, upperByte, leftUpperByte);
			break;
		}
	}

	// change array
	for (uLong i = 0, skipMode = 0; i < zlibWindowSize; i++)
	{
		if (i % tmpWidth == 0)
		{
			skipMode++;
			continue;
		}
		uncomressBytes[i - skipMode] = uncomressBytes[i];
	}

	// resize array
	if (resize(height * width * prevShift, &uncomressBytes) != ERROR_SUCCESS)
	{
		// ошибка выведенная в функции
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	return ERROR_SUCCESS;
}

int concatChunks(chunkWithoutCRC *resChunk, const uchar *chunk, uLong *currPos, uchar *buff, FILE *readerOfFile)
{
	int err;
	uint capacity = 10;
	resChunk->lenght = 0;
	resChunk->type = NULL;
	resChunk->data = NULL;
	if (createBuff(capacity, &resChunk->data) != ERROR_SUCCESS)
	{
		// ошбика выведенна в функции
		fclose(readerOfFile);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	if (createBuff(4, &resChunk->type) != ERROR_SUCCESS)
	{
		// ошбика выведенна в функции
		fclose(readerOfFile);
		free(resChunk->data);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	chunkWithoutCRC tmpChunk;
	while (findChunk(chunk, currPos, readerOfFile) == ERROR_SUCCESS)
	{
		*currPos -= 4;
		// read chunk len, type and data
		if ((err = readChunk(&tmpChunk, currPos, buff, readerOfFile)) != ERROR_SUCCESS)
		{
			// ошибка выведенна в функции и файл закрыт
			return err;
		}
		resChunk->lenght += tmpChunk.lenght;
		copyArr(resChunk->type, tmpChunk.type, 0, 4);

		// resize chunk
		if (resChunk->lenght > capacity)
		{
			capacity = capacity * 2 >= resChunk->lenght ? capacity * 2 : resChunk->lenght;
			if (resize(capacity, &resChunk->data) != ERROR_SUCCESS)
			{
				// ошибка выведенная в функции
				fclose(readerOfFile);
				closeChunk(resChunk);
				return ERROR_NOT_ENOUGH_MEMORY;
			}
		}

		// copy tmp to res
		for (size_t i = 0; i < tmpChunk.lenght; i++)
			resChunk->data[i + resChunk->lenght - tmpChunk.lenght] = tmpChunk.data[i];

		// free array
		if (tmpChunk.lenght > 13)
			free(tmpChunk.data);
		free(tmpChunk.type);
	}
	if (resize(resChunk->lenght, &resChunk->data) != ERROR_SUCCESS)
	{
		// ошибка выведенная в функции
		fclose(readerOfFile);
		closeChunk(resChunk);
		return ERROR_NOT_ENOUGH_MEMORY;
	}
	if (resChunk->lenght == 0)
	{
		fprintf(stderr, "not found IDAT");
		fclose(readerOfFile);
		closeChunk(resChunk);
		return ERROR_UNKNOWN;
	}
	return ERROR_SUCCESS;
}

int myUncompress(uchar *uncompressBytes, uLong *cntBytes, chunkWithoutCRC *IDAT)
{
	int err;
#ifdef ISAL
	struct inflate_state uncompress;
	isal_inflate_init(&uncompress);
	uncompress.next_in = IDAT->data;
	uncompress.avail_in = IDAT->lenght;
	uncompress.next_out = uncompressBytes;
	uncompress.avail_out = cntBytes;
	uncompress.crc_flag = IGZIP_ZLIB;
	if ((err = isal_inflate(&uncompress)) != COMP_OK)
	{
		fprintf(stderr, "error in isa-l library in inflate function\n");
		return ERROR_UNKNOWN;
	}
#elif defined(LIBDEFLATE)
	if ((err = libdeflate_zlib_decompress(libdeflate_alloc_decompressor(), IDAT->data, IDAT->lenght, uncompressBytes, *cntBytes, NULL) != LIBDEFLATE_SUCCESS))
	{
		switch (err)
		{
		case LIBDEFLATE_BAD_DATA:
			fprintf(stderr, "invalid data\n");
			return ERROR_INVALID_DATA;
		case LIBDEFLATE_SHORT_OUTPUT:
			fprintf(stderr, "not enought memory in uncompress\n");
			return ERROR_NOT_ENOUGH_MEMORY;
		case LIBDEFLATE_INSUFFICIENT_SPACE:
			fprintf(stderr, "not enought memory in dest buff\n");
			return ERROR_NOT_ENOUGH_MEMORY;
		}
	}
#elif defined(ZLIB)
	if ((err = uncompress(uncompressBytes, cntBytes, IDAT->data, IDAT->lenght)) != Z_OK)
	{
		switch (err)
		{
		case Z_MEM_ERROR:
			fprintf(stderr, "not enought memory in uncompress\n");
			return ERROR_NOT_ENOUGH_MEMORY;
		case Z_BUF_ERROR:
			fprintf(stderr, "not enought memory in dest buff\n");
			return ERROR_NOT_ENOUGH_MEMORY;
		case Z_DATA_ERROR:
			fprintf(stderr, "invalid data\n");
			return ERROR_INVALID_DATA;
		}
	}
#endif
	return ERROR_SUCCESS;
}

#endif
