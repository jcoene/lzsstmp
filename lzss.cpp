//========= Copyright � 1996-2007, Valve Corporation, All rights reserved. ============//
//
//  LZSS Codec. Designed for fast cheap gametime encoding/decoding. Compression results
//  are not aggresive as other alogrithms, but gets 2:1 on most arbitrary uncompressed data.
//
//=====================================================================================//

#include <stdio.h>
#include <stdlib.h>

#define LZSS_LOOKSHIFT    4
#define LZSS_ID           1397971532

// bind the buffer for correct identification
struct lzss_header_t
{
  unsigned int  id;
  unsigned int  actualSize; // always little endian
};

//-----------------------------------------------------------------------------
// Returns uncompressed size of compressed input buffer. Used for allocating output
// buffer for decompression. Returns 0 if input buffer is not compressed.
//-----------------------------------------------------------------------------
unsigned int GetActualSize( unsigned char *pInput )
{
  lzss_header_t *pHeader = (lzss_header_t *)pInput;
  // XXX modified: was reading with wrong endian-ness
  if ( pHeader && pHeader->id == LZSS_ID )
  {
    return pHeader->actualSize;
  }

  printf("unrecognized header: %d, expected %d\n", pHeader->id, LZSS_ID);

  // unrecognized
  return 0;
}

//-----------------------------------------------------------------------------
// Uncompress a buffer, Returns the uncompressed size. Caller must provide an
// adequate sized output buffer or memory corruption will occur.
//-----------------------------------------------------------------------------
unsigned int Uncompress( unsigned char *pInput, unsigned char *pOutput )
{
  unsigned int totalBytes = 0;
  int cmdByte = 0;
  int getCmdByte = 0;

  unsigned int actualSize = GetActualSize( pInput );
  if ( !actualSize )
  {
    printf("can't get actualSize\n");
    return 0;
  }

  printf("actualSize is %d\n", int(actualSize));
  pOutput = (unsigned char *)malloc(int(actualSize));

  pInput += sizeof( lzss_header_t );

  printf("pinput now %d\n", int(sizeof( lzss_header_t )));

  for ( ;; )
  {
    if ( !getCmdByte )
    {
      printf("%d: cmdByte = %x\n", totalBytes, *pInput);
      cmdByte = *pInput++;
    }
    getCmdByte = ( getCmdByte + 1 ) & 0x07;

    if ( cmdByte & 0x01 )
    {
      int position = *pInput++ << LZSS_LOOKSHIFT;
      position |= ( *pInput >> LZSS_LOOKSHIFT );
      int count = ( *pInput++ & 0x0F ) + 1;
      if ( count == 1 )
      {
        break;
      }

      unsigned char *pSource = pOutput - position - 1;
      printf("%d: position = %d, count = %d, source = %d!\n", totalBytes, position, count, int(*pSource));
      for ( int i=0; i<count; i++ )
      {
        printf("%d: writing %x\n", totalBytes, *pSource);
        *pOutput++ = *pSource++;
      }
      totalBytes += count;
    }
    else
    {
      printf("%d: copy %x\n", totalBytes, *pInput);
      *pOutput++ = *pInput++;
      totalBytes++;
    }
    cmdByte = cmdByte >> 1;
    if (totalBytes > 8) {
      printf("%d: last 8 bytes written: %x %x %x %x %x %x %x %x\n",
        totalBytes, *(pOutput-1),*(pOutput-2),*(pOutput-3),*(pOutput-4),
        *(pOutput-5), *(pOutput-6), *(pOutput-7), *(pOutput-8));
    }
  }

  if ( totalBytes != actualSize )
  {
    printf("totalBytes %d, expected %d!\n", int(totalBytes), int(actualSize));
    return 0;
  }

  return totalBytes;
}

int main(int argc, char *argv[]) {
  FILE *infile, *outfile;
  long insize;
  size_t res;
  unsigned int outsize;
  unsigned char *inbuf, *outbuf;

  if (argc != 3) {
    printf("Usage: lzss infile outfile\n");
    return 1;
  }

  if ((infile  = fopen(argv[1], "rb")) == NULL) {
    printf("? %s\n", argv[1]);
    return 1;
  }

  fseek(infile, 0, SEEK_END);
  insize = ftell(infile);

  inbuf = (unsigned char*) malloc(sizeof(unsigned char)*insize);
  if (inbuf == NULL) return 2;

  rewind(infile);
  res = fread(inbuf, 1, insize, infile);
  if (res != insize) {
    printf("fread: expected %d got %d\n", int(insize), int(res));
    return 3;
  }

  printf("read %d, inbuf is %d\n", int(res), int(sizeof(inbuf)));

  if ((outfile = fopen(argv[2], "wb")) == NULL) {
    printf("? %s\n", argv[2]);
    return 1;
  }

  outsize = Uncompress(inbuf, outbuf);

  for (int i = 0; i < outsize; i++) {
    printf("out byte %d: 0x%x\n", i, outbuf[i]);
  }

  printf("outsize is %d\n", int(outsize));

  res = fwrite(outbuf, 1, outsize, outfile);
  if (res != outsize) {
    printf("fwrite: expected %d got %d\n", int(outsize), int(res));
    return 3;
  }

  printf("wrote %d, outbuf is %d\n", int(res), int(sizeof(outbuf)));

  fclose(infile);
  fclose(outfile);

  free(inbuf);

  return 0;
}