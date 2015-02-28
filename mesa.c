#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include "mesa.h"
#include "mesa-constants.h"


uint32_t ftoh( MESASTOR* stor, uint32_t value )
{
    if( stor->flip_endianess )
    {
        uint32_t flipped;
        
        for( int i = 0; i < UINT_LEN; i++ )
        {
            ((char*)(&flipped))[UINT_LEN-1-i] = ((char*)(&value))[i];
        } 

        return flipped;
    }
    
    return value; 
}

MESASTOR* mesa_open( const char* archive_name )
{
    return mesa_openf( fopen( archive_name , "rb") );
}

MESASTOR* mesa_openf( FILE * file ) 
{
    MESASTOR* stor = MESA_MALLOC( sizeof( MESASTOR ) );
    stor->type = MESA_FHANDLE;
    stor->src.file = NULL;
    stor->err = 0;
    stor->flip_endianess = false;

    if( file == NULL )
    {
        stor->err = MESA_ERR_NO_SUCH_FILE;
        goto ERROR_ENCOUNTERED;
    }
        
    stor->src.file = file;
    
    {
        char *r_magic = alloca( MAGIC_LEN );
        memset( r_magic, 0, MAGIC_LEN );

        if( -1 == fseek( stor->src.file, 0L, SEEK_SET ) )
            goto FSEEK_ERROR;

        if( 1 != fread( r_magic, MAGIC_LEN, 1, stor->src.file ) )
            goto FREAD_ERROR;

        //Check magic bytes to see if mesa stor file
        if( memcmp( r_magic, magic_bytes, MAGIC_LEN ) != 0 )
        {
            stor->err = MESA_ERR_INVALID_FILE_FORMAT;
            goto ERROR_ENCOUNTERED;
        }
    }

    {
        uint32_t r_endianess = 0x0;

        if( -1 == fseek( stor->src.file, ENDIANNESS_POS, SEEK_SET ) )
            goto FSEEK_ERROR;

        if( 1 != fread( &r_endianess, UINT_LEN, 1, stor->src.file ) )
            goto FREAD_ERROR;

        if( r_endianess != endianess )
        {
            if( r_endianess != other_endianess )
            {
                stor->err = MESA_ERR_MALFORMED_ENDIANESS;
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

        if( -1 == fseek( stor->src.file, VERSION_POS, SEEK_SET ) )
            goto FSEEK_ERROR;
        
        if( 1 != fread( &r_version, UINT_LEN, 1, stor->src.file ) )
            goto FREAD_ERROR;

        r_version = ftoh( stor, r_version ); 

        if( r_version != version_number )
        {
            stor->err = MESA_ERR_UNSUPPORTED_VERSION;
            goto ERROR_ENCOUNTERED;
        }
    }

    {
        uint32_t r_instance_len;

        if( -1 == fseek( stor->src.file, INSTANCE_LENGTH_POS, SEEK_SET ) )
            goto FSEEK_ERROR;
        

        if( 1 != fread( &r_instance_len, UINT_LEN, 1, stor->src.file ) )
            goto FREAD_ERROR;

        stor->instance_len = ftoh( stor, r_instance_len );
    }

    {
        uint32_t r_instance_count;

        if( -1 == fseek( stor->src.file, INSTANCE_COUNT_POS, SEEK_SET ) )
            goto FSEEK_ERROR;

        if( 1 != fread( &r_instance_count, UINT_LEN, 1, stor->src.file ) )
            goto FREAD_ERROR;

        stor->instance_count = ftoh( stor, r_instance_count );
    }

    goto END;

FSEEK_ERROR:
    switch( errno )
    {
        case EBADF:
            stor->err = MESA_ERR_EBADF;
            break;

        default:
            stor->err = MESA_ERR_UNKNOWN_IO;
            break;
    }
    goto END;

FREAD_ERROR:
    if( feof( stor->src.file ) )
    {
        stor->err = MESA_ERR_EOF;
    }
    else if( ferror( stor->src.file ) )
    {
        stor->err = MESA_ERR_READ_FERROR;
    }
    else
    {
        stor->err = MESA_ERR_UNKNOWN_IO;
    }

    goto END;
ERROR_ENCOUNTERED:

    goto END;
END:
    return stor;
}

void mesa_close( MESASTOR* stor )
{
    if( stor->type == MESA_FHANDLE )
    {
        fclose( stor->src.file );
        stor->src.file = NULL;
    }

    MESA_FREE( stor );
}

MESAFILE* mesa_fopen( MESASTOR* stor, const char* filename )
{
    char *buf = alloca( stor->instance_len );

    if( stor->type == MESA_FHANDLE )
    {
        fseek( stor->src.file, INSTANCE_START, SEEK_SET );

        for( int i = 0; i < stor->instance_count; i++ )
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
        char** arr = MESA_MALLOC( sizeof( char* ) * (stor->instance_count + 1) );
        memset( arr, 0, sizeof( char* ) * (stor->instance_count + 1) );

        fseek( stor->src.file, INSTANCE_START, SEEK_SET );

        for( int i = 0; i < stor->instance_count; i++ )
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

int mesa_has_error( MESASTOR* stor )
{
    return stor->err;
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
