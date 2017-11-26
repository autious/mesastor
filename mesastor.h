//The MIT License (MIT)
//
//Copyright (c) 2015 Max Danielsson
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#ifndef MESA_H
#define MESA_H

#include <stdbool.h>
#include <stdint.h>

enum mesastor_type
{
    MESA_STREAM
};

enum mesastor_error 
{
    MESA_ERR_OK = 0,
    MESA_ERR_INVALID_FILE_FORMAT,
    MESA_ERR_MALFORMED_ENDIANESS,
    MESA_ERR_UNSUPPORTED_ENDIANESS,
    MESA_ERR_UNSUPPORTED_VERSION,
    MESA_ERR_NO_SUCH_FILE,
    MESA_ERR_CORRUPT_FILE,
    MESA_ERR_IO_SEEK,
    MESA_ERR_IO_READ,
    MESA_NOT_IMPLEMENTED,
    MESA_ERR_MALFORMED_MESASTOR
};

enum mesafile_error
{
    MESAFILE_ERR_NO = 0
};

enum mesastream_whence
{
    MESASTREAM_SEEK_SET,
    MESASTREAM_SEEK_CUR,
    MESASTREAM_SEEK_END
};

struct mesastream
{
    int64_t (*read)  (void* userdata, void* buf, uint64_t size);
    int64_t (*write) (void* userdata, void* buf, uint64_t size);
    int64_t (*seek)  (void* userdata, int64_t offset, enum mesastream_seek_whence whence);
    int64_t (*tell)  (void* userdata);
};

typedef struct mesastream MESASTREAM;

struct mesamem
{
    //Must be implemented as an memory preserving realloc.
    //if a new pointer is generated on resize, it must be returned.
    //if ptr is NULL, it's a fresh allocation request.
    //if a size of 0 is sent, that indicates a deallocation, NULL must be returned.
    //if ptr is NULL and size is 0 it's a NOP, and the function must return NULL.
    void* (*realloc) (void* ptr, uint32_t* size);
};

typedef struct mesamem MESAMEM;

struct mesastor
{
    MESASTREAM* stream;
    void* userdata;

    MESAMEM* mem;

    char* filenames; 
    uint64_t* file_starts;
    uint64_t* file_sizes;
    uint16_t* file_flags;
    
    enum mesastor_type type;

    bool flip_endianess;

    uint32_t filename_size;
    uint32_t file_count;
};

typedef struct mesastor MESASTOR;

struct mesafile
{
    struct mesastor* stor;

    size_t size;
    size_t start_pos;
    size_t fpos; //Relative position from start_pos.

    enum mesafile_error err;
};

typedef struct mesafile MESAFILE;

mesastor_error mesa_open_stream(MESASTOR* out, MESASTREAM* stream, MESAMEM* mem, void* userdata);
mesastor_error mesa_close(MESASTOR* file);

//CPPEXT uint64_t    mesa_size(MESAFILE* stream);
//CPPEXT uint64_t    mesa_seek(MESAFILE* stream, uint64_t offset, uint8_t whence);
//CPPEXT uint64_t    mesa_tell(MESAFILE* stream);
//CPPEXT int64_t     mesa_read(MESAFILE* stream, void* ptr, uint64_t size);
//CPPEXT int64_t     mesa_write(MESAFILE* stream, void* ptr, uint64_t size);

uint32_t    mesa_get_file_count(MESASTOR* stor);
const char* mesa_get_file_name(MESASTOR* stor, uint32_t index);
uint64_t    mesa_get_file_size(MESASTOR* stor, uint32_t index);
uint64_t    mesa_get_file_data(MESASTOR* stor, void* dest);

const char * mesa_get_error_string(MESASTOR* stor);

#endif //MESASTOR_H

#ifdef MESASTOR_IMPL

#include <stdlib.h>
#include <string.h>

static const char magic_bytes[] = { 0x4d, 0x45, 0x53, 0x41, 0x53, 0x54, 0x4f, 0x52 };
static const int MAGIC_LEN = 8;

static const long ENDIANNESS_POS = 8;
static const long VERSION_POS = 12;
static const long FILENAME_LENGTH_POS = 16;
static const long INSTANCE_COUNT_POS = 20;
static const long INSTANCE_START = 32;

static const int UINT32_LEN = 4;
static const int UINT64_LEN = 8;

static const uint32_t endianess = 0xffff0000; //This will always match endianess on "my" system.
static const uint32_t other_endianess = 0x0000ffff; //This will always match endianess on "other" system 
                                                    //(little/big) meaning you will need to flip to 
                                                    //interpret correctly.
static const uint32_t version_number = 0; //While this is zero, the specification isn't soldified, when it reaches 1, it will be.


uint32_t static ftoh( MESASTOR* stor, uint32_t value )
{
    if( stor->flip_endianess )
    {
        uint32_t flipped;
        
        for( int i = 0; i < UINT32_LEN; i++ )
        {
            ((char*)(&flipped))[UINT32_LEN-1-i] = ((char*)(&value))[i];
        } 

        return flipped;
    }
    
    return value; 
}

uint64_t static ftoh64( MESASTOR* stor, uint64_t value )
{
    if( stor->flip_endianess )
    {
        uint64_t flipped;
        
        for( int i = 0; i < UINT64_LEN; i++ )
        {
            ((char*)(&flipped))[UINT64_LEN-1-i] = ((char*)(&value))[i];
        } 

        return flipped;
    }
    
    return value; 
}

mesastor_error mesa_open_stream(MESASTOR* stor, MESASTREAM* stream, MESAMEM* mem, void* userdata)
{
    mesastor_error ret = MESA_ERR_OK;

    stor->mem = mem;
    stor->stream.stream = stream;
    stor->stream.userdata = userdata;
    stor->type = MESA_STREAM;
    stor->err = 0;
    stor->flip_endianess = false;
    stor->filename_size = 0;
    stor->file_count = 0;

    stor->filenames = NULL;
    stor->file_starts = NULL;
    stor->file_sizes = NULL;
    stor->file_flags = NULL;

    char* file_info_buffer = NULL;
    
    {
        char r_magic[MAGIC_LEN];
        memset( r_magic, 0, MAGIC_LEN );

        if( -1 == stream->seek( userdata, stor->src.file, 0, MESASTREAM_SEEK_SET ) )
            goto SEEK_ERROR;

        if( MAGIC_LEN != stream->read( userdata, r_magic, MAGIC_LEN ) )
            goto READ_ERROR;

        //Check magic bytes to see if mesa stor file
        if( memcmp( r_magic, magic_bytes, MAGIC_LEN ) != 0 )
        {
            ret = MESA_ERR_INVALID_FILE_FORMAT;
            goto ERROR_ENCOUNTERED;
        }
    }

    {
        uint32_t r_endianess = 0x0;

        if( UINT32_LEN != stream->read( userdata, &r_endianess, UINT32_LEN ) )
            goto READ_ERROR;

        if( r_endianess != endianess )
        {
            if( r_endianess != other_endianess )
            {
                ret = MESA_ERR_MALFORMED_ENDIANESS;
                goto ERROR_ENCOUNTERED;
            }
            else
            {
                stor->flip_endianess = true;
            }
        }
    }

    {
        uint32_t r_version;
        
        if( UINT32_LEN != stream->read( userdata, &r_version, UINT32_LEN ) )
            goto READ_ERROR;

        r_version = ftoh( stor, r_version ); 

        if( r_version != version_number )
        {
            ret = MESA_ERR_UNSUPPORTED_VERSION;
            goto ERROR_ENCOUNTERED;
        }
    }

    {
        uint32_t r_instance_len;

        if( UINT32_LEN != stream->read( userdata, &r_instance_len, UINT32_LEN ) )
            goto READ_ERROR;

        stor->instance_len = ftoh( stor, r_instance_len );
    }

    {
        uint32_t r_file_count;

        if( UINT32_LEN != stream->read( userdata, &r_file_count, UINT32_LEN ) )
            goto READ_ERROR;

        stor->file_count = ftoh( stor, r_file_count );
    }

    {
        stream->seek(userdata, 32, MESASTREAM_SEEK_SET);

        file_info_buffer = stor->mem->realloc(NULL,stor->filename_size+16);

        stor->filenames   =  (uint64_t*)mem->realloc(NULL,stor->filename_size*stor->file_count);
        stor->file_starts =  (uint64_t*)mem->realloc(NULL,sizeof(uint64_t)*stor->file_count);
        stor->file_sizes  =  (uint64_t*)mem->realloc(NULL,sizeof(uint64_t)*stor->file_count);
        stor->file_flags  =  (uint16_t*)mem->realloc(NULL,sizeof(uint16_t)*stor->file_count);
        for( int i = 0; i < stor->file_count; i++ ) {
            if( stor->instance_len != stream->read(userdata, file_info_buffer, stor->filename_size+16)){
                ret = MESA_ERR_IO_READ;
                goto ERROR;
            }

            memcpy(stor->filenames + stor->filename_size * i, file_info_buffer, stor->filename_size);
            memcpy(stor->file_starts + sizeof(uint64_t)*i,    file_info_buffer + stor->filename_size, sizeof(uint64_t));
            memcpy(stor->file_sizes  + sizeof(uint64_t)*i,    file_info_buffer + stor->filename_size + sizeof(uint64_t), sizeof(uint64_t));

            stor->file_starts[i] = ftoh64(stor,stor->file_starts[i]);
            stor->file_sizes[i] =  ftoh64(stor,stor->file_sizes[i]);
        }
        file_info_buffer = stor->mem->realloc(file_info_buffer,0);
    }

    goto END;

ERROR:
    stor->filenames   = mem->realloc(stor->filenames,0);
    stor->file_starts = mem->realloc(stor->file_starts,0);
    stor->file_sizes  = mem->realloc(stor->file_sizes,0);
    stor->file_flags  = mem->realloc(stor->file_flags,0);
    file_info_buffer  = mem->realloc(file_info_buffer,0);
    goto END;
END:
    return ret;
}

void mesa_close( MESASTOR* stor )
{
    MESA_FREE( stor );
}

MESAFILE* mesa_fopen( MESASTOR* stor, const char* filename )
{
    char *buf = alloca( stor->instance_len );

    if( stor->type == MESA_FHANDLE )
    {
        fseek( stor->src.file, INSTANCE_START, SEEK_SET );

        for( int i = 0; i < stor->file_count; i++ )
        {
            if( 1 != fread( buf, stor->instance_len, 1, stor->src.file ) )
            {
                stor->err = MESA_ERR_MALFORMED_MESASTOR;
                return NULL;
            }

            //See if this is the file we are interested in.
            if( strcmp( filename, buf ) == 0 )
            {
                MESAFILE * file = MESA_MALLOC( sizeof( MESAFILE ) );  

                uint32_t* start_pos, *size;

                start_pos = (uint32_t*)(buf + stor->instance_len - 16);
                size = (uint32_t*)(buf + stor->instance_len - 12);

                file->stor = stor;
                file->err = MESAFILE_ERR_NO;
                file->start_pos = *start_pos;
                file->size = *size;
                file->fpos = 0;

                return file;
            } 
        }

        stor->err = MESA_ERR_FILE_NOT_FOUND;
        return NULL;
    }
    else
    {
        //TODO Other source formats.
    }

    stor->err = MESA_NOT_IMPLEMENTED;
    return NULL;
}

void mesa_fclose( MESAFILE* file )
{
    MESA_FREE( file );
}

size_t mesa_fsize( MESAFILE* stream )
{
    return stream->size;
}

size_t mesa_fread( void *ptr, size_t size, size_t nmemb, MESAFILE* stream )
{
    if( stream->stor->type == MESA_FHANDLE )
    {
        size_t a_nmemb = nmemb;
        while( stream->fpos + a_nmemb * size > stream->size && a_nmemb > 0 )
        {
            a_nmemb--;
        }

        if( a_nmemb > 0 )
        {
            fseek( stream->stor->src.file, stream->start_pos + stream->fpos, SEEK_SET );
            size_t readl = fread( ptr, size, a_nmemb, stream->stor->src.file );
            stream->fpos += readl * size;

            return readl;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        //TODO buf read
        return 0;
    }
}

char** mesa_getfiles( MESASTOR* stor )
{
    if( stor->type == MESA_FHANDLE )
    {
        char** arr = MESA_MALLOC( sizeof( char* ) * (stor->file_count + 1) );
        memset( arr, 0, sizeof( char* ) * (stor->file_count + 1) );

        fseek( stor->src.file, INSTANCE_START, SEEK_SET );

        for( int i = 0; i < stor->file_count; i++ )
        {
            char *buf = alloca( stor->instance_len );
            arr[i] = MESA_MALLOC( sizeof( char ) * stor->instance_len );
            memset( arr[i], 0, stor->instance_len );

            fread( arr[i], stor->instance_len, 1, stor->src.file );
        }

        return arr;
    }
    else
    {
        return NULL;
    }
}

void mesa_getfiles_free( char** var )
{
    if( var != NULL )
    {
        int i = 0;
        while( var[i] != NULL )
        {
            free( var[i] );
            var[i] = NULL;
            i++;
        }
    }

    free( var );
    var = NULL;
}

const char * mesa_get_error_string( MESASTOR* stor )
{
    if( stor != NULL && stor->err >= 0 )
    {
        switch( stor->err )
        {
        case MESA_ERR_NO:
            return "No Error";
        case MESA_ERR_INVALID_FILE_FORMAT:
            return "Invalid file format";
        case MESA_ERR_MALFORMED_ENDIANESS:
            return "Malformed endianess";
        case MESA_ERR_UNSUPPORTED_ENDIANESS:
            return "Unsupported MESASTORE file Endianess";
        case MESA_ERR_UNSUPPORTED_VERSION:
            return "Unsupported MESASTORE file version";
        case MESA_ERR_NO_SUCH_FILE:
            return "No such file";
        case MESA_ERR_CORRUPT_FILE:
            return "Corrupt file";
        case MESA_ERR_UNKNOWN_IO:
            return "Unknown IO error";
        case MESA_ERR_EBADF:
            return "EBADF Bad stream file not open";
        case MESA_ERR_EOF:
            return "Reached a premature EOF";
        case MESA_ERR_READ_FERROR:
            return "Read ferror";
        case MESA_ERR_FILE_NOT_FOUND:
            return "File not found inside MESASTORE file";
        case MESA_NOT_IMPLEMENTED:
            return "Not implemented";
        default:
            return "Unknown error";
        }
    }
    else
    {
        return "Invalid MESASTOR pointer";
    }
}

#endif //MESASTOR_IMPL
