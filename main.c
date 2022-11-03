#include "main.h"
#include "metods.h"
#include "structs.h"
#include "typesOfCunks.h"

int main(int argc, const char *argv[])
{
	uchar buff13[13];
	uLong currPos = 0;
	int err;

	if (argc != 3)
	{
		printf("wrong number of arguments");
		return ERROR_INVALID_DATA;
	}

	FILE *readerOfFile = fopen(argv[1], "rb");
	if (readerOfFile == NULL)
	{
		printf("file not found to read");
		return ERROR_NOT_FOUND;
	}

	// read PNG
	if ((err = readBytes(8, &currPos, buff13, readerOfFile)) != ERROR_SUCCESS)
	{
		// ошибка выведена в функции и закрыт файл
		return err;
	}
	if (!isPNG(buff13))
	{
		// это не png
		fprintf(stderr, "file not png");
		fclose(readerOfFile);
		return ERROR_INVALID_DATA;
	}

	// read IHDR len, type and data
	chunkWithoutCRC IHDR;
	if ((err = readChunk(&IHDR, &currPos, buff13, readerOfFile)) != ERROR_SUCCESS)
	{
		return err;
	}
	if (!isChunk(IHDR.type, ihdr))
	{
		fprintf(stderr, "not found IHDR");
		fclose(readerOfFile);
		free(IHDR.type);
		return ERROR_INVALID_DATA;
	}
	free(IHDR.type);

	// data of file
	size_t height = getLen(IHDR.data, 4);
	size_t width = getLen(IHDR.data, 0);
	uchar bitDepth = IHDR.data[8];
	uchar typeOfColour = IHDR.data[9];
	uchar typeOfCompress = IHDR.data[10];
	uchar typeOfFilter = IHDR.data[11];
	uchar interlace = IHDR.data[12];
	// chek valid data
	if (typeOfColour != 0 && typeOfColour != 2)
	{
		fprintf(stderr, "unsupported color type\n");
		fclose(readerOfFile);
		return ERROR_INVALID_DATA;
	}
	if (typeOfCompress != 0)
	{
		fprintf(stderr, "unsupported compress type\n");
		fclose(readerOfFile);
		return ERROR_INVALID_DATA;
	}
	if (typeOfFilter != 0)
	{
		fprintf(stderr, "unsupported filter type\n");
		fclose(readerOfFile);
		return ERROR_INVALID_DATA;
	}
	if (interlace != 0)
	{
		fprintf(stderr, "unsupported ADAM7 interlace\n");
		fclose(readerOfFile);
		return ERROR_INVALID_DATA;
	}
	// printf("h: %zu, w: %zu, bitD: %i, typeOfColour: %i, typeOfCompress: %i, interlace: %i\n", height, width,
	// bitDepth, typeOfColour, typeOfCompress, interlace);

	// read IDAT len, type and data
	chunkWithoutCRC IDAT;
	if ((err = concatChunks(&IDAT, idat, &currPos, buff13, readerOfFile)) != ERROR_SUCCESS)
	{
		// не найдет IDAT или произошла ошибка в момент его поиска и закрыт файл
		fprintf(stderr, "not found IDATs or diff err\n");
		return err;
	}

	// printf("len: %i, type: %s\n", IDAT.lenght, IDAT.type);

	// end of readFile
	fclose(readerOfFile);

	// uncompress IDAT.data
	uLong cntBytes = height * width * ((typeOfColour == 2) ? 3 : 1) + height;
	uchar *uncomressBytes;
	if ((err = createBuff(cntBytes, &uncomressBytes)) != ERROR_SUCCESS)
	{
		closeChunk(&IDAT);
		return err;
	}
	if ((err = myUncompress(uncomressBytes, &cntBytes, &IDAT)) != ERROR_SUCCESS)
	{
		// ошибка выведена
		closeChunk(&IDAT);
		free(uncomressBytes);
		return err;
	}
	// printf("size of file: %lu, size of comress data: %u\n", cntBytes, IDAT.lenght);
	closeChunk(&IDAT);

	// filter processing of uncompress IDAT.data
	if (procFilters(uncomressBytes, cntBytes, height, width, typeOfColour) != ERROR_SUCCESS)
	{
		// ошибка выведенная в функции
		free(uncomressBytes);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	// start of writing file
	FILE *writerOfFile = fopen(argv[2], "wb");
	if (writerOfFile == NULL)
	{
		printf("file not found to write");
		free(uncomressBytes);
		return ERROR_NOT_FOUND;
	}
	if (typeOfColour == 0)
		fprintf(writerOfFile, "P5\n");
	else
		fprintf(writerOfFile, "P6\n");
	fprintf(writerOfFile, "%zu %zu\n", width, height);
	uint maxValue = pow(2, bitDepth) - 1;
	fprintf(writerOfFile, "%i\n", maxValue);
	cntBytes -= height;
	if (fwrite(uncomressBytes, sizeof(uchar), cntBytes, writerOfFile) != cntBytes)
	{
		fprintf(stderr, "error of output");
		fclose(writerOfFile);
		free(uncomressBytes);
		return ERROR_NOT_ENOUGH_MEMORY;
	}

	free(uncomressBytes);
	fclose(writerOfFile);
}
