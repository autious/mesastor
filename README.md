# Mesastor
Mesastor is a simple data-package format suitable for large quantities of small files. It focuses on fast extraction.

The specification is also endianness aware but not endianness specific for optimal platform performance. The header contains
an endianness specifier and an implemenation can choose to either ignore (and fail) or convert a file that i being loded.

## File specification
Asset file format MESASTOR(Max Emil Stefan Alexander STORage format)

This file format is a pure binary format intended to store data
tightly and simply as a large concatenated file.

The file ALWAYS begins with 8(0) bytes that in ASCII would print the
text "MESASTOR" this string will never change  in any subsequent
version.

Then there is a 4 byte section reserved for indicating endianness of
header integers.  This values is either 0xff 0xff 0x00 0x00 for big
endian, or 0x00 0x00 0xff 0xff for little endian systems. This
ordering assists in figuring out matching endianess as the value will
equal an uint32_t that is 0xffff0000 on systems that match the the
endianess of the file. Otherwise the reader will have to convert the
values (or deny loading the file).

Following the first 12 bytes there are 4(12) bytes unsigned integer
indicating the version, initially the value is 0x00000001. Indicating
version 1. This version must always be a perfect match with the
parsers implementation.

Following these 4 bytes there is a 4(16) byte uint declaring the
length of each file header block. This value MUST be a multiple of 16
bytes to simplify memory aligned copies from disk, meaning that the
last 16 bytes should be aligned.  The last 16 bytes must be included
in the size, it can't just specify the size of the name in memory.
Names stored in the header must be end-padded with any number of NULL
bytes to simplify copies to stack memory when parsing the file header.

Then there's another 4(20) byte variable indicating how many header
instances the file contains.

Then there is a padding of 12 bytes up to (32) where the header block
always begins.

After the headers there is the previously specified data blocks.

Each new data block MUST start on a 16 byte aligned address in the
file, if a file doesn't fill up the missing space at the end of its
allocated block, it SHOULD be padded with NULL bytes.


0                4                8                12               16
|----------------|----------------|----------------|----------------|16
|    "MESA"           "STOR"          ENDIANESS    |     VERSION    |
|----------------|----------------|----------------|----------------|32
| Instance Length  Instance Count      PADDING           PADDING    |
|----------------|----------------|----------------|----------------|
|                                                                   |
|                              Filename...                          |
|   ............................................................    |
|    Start Pos   |     Size       |    PADDING     |   PADDING      |
|-------------------------------------------------------------------|
|                                                                   |
|                 POSSIBLE REPEAT OF PREVIOUS BLOCK                 |
|                                                                   | 
|-------------------------------------------------------------------|
|                                                                   |
|                                                                   |
|                           DATA BLOCK                              |
|                                                                   |
|                                                                   |
|-------------------------------------------------------------------|

