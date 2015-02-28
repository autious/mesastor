#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mesa-constants.h"

static int validate_files( char ** files, int count ) {
   FILE *f;
   int i;
   for( i = 0; i < count; i++ ) {
       if( NULL == ( f = fopen( files[i], "rb" ) ) ) {
           return i;
       }

       fclose( f );
   }

   return -1;
}

static uint32_t get_header_instance_length( char ** files, int count ) {
    int i;
    uint32_t longest = 0;

    for( i = 0; i < count; i++ ) {
        uint32_t len = strlen( files[i] ) + 1;
        if( len > longest ) {
            longest = len;
        }
    }

    //Shortest 16 block fit, with 16 bit extra for other information.
    return (longest/16)*16 + 32;
}

static void fwritestream( FILE* from, FILE* to )
{
    clearerr( from );
    clearerr( to );

    char buf[1024];
    while( feof( from ) == 0 && ferror( from ) == 0 && ferror( to ) == 0 )
    {
        size_t readl = fread( buf, 1, 1024, from );
        fwrite( buf, 1, readl, to );
    }
}

int main( int argc, char **argv )
{
    assert( sizeof( uint32_t ) == 4 );

    char** filename_start = argv + 1;
    uint32_t filename_count = argc - 1;
    int failure_file = validate_files( filename_start, filename_count );

    if( failure_file != -1 )
    {
        fprintf( stderr, "Unable to open file \"%s\"", (filename_start)[failure_file]);
        perror("");
        return 1;
    }

    FILE * out = tmpfile();

    if( out == NULL ) {
        perror("Unable to open tmp file\n");
        exit(1);
    }

    //Write magic bytes 
    fwrite( magic_bytes, 8, 1, out );
    fwrite( &endianess, 4, 1, out );
    fwrite( &version_number, 4, 1, out );

    uint32_t header_instance_len = get_header_instance_length(filename_start, filename_count );

    fwrite( &header_instance_len, 4, 1, out );
    fwrite( &filename_count, 4, 1, out );
    
    //Skip padding
    fseek( out, 8L, SEEK_CUR );

    uint32_t cur_data_block = 32 + filename_count * header_instance_len;
    uint32_t cur_header = 32 + 0 * header_instance_len;

    for( int i = 0; i < filename_count; i++ )
    {
        const char* cur_file = (filename_start)[i];
        FILE* file_in = fopen( filename_start[i], "rb" );

        if( file_in != NULL )
        {
            assert( strlen( cur_file ) + 1 + 16 <= header_instance_len );

            //Assert the size of the file data.
            fseek( file_in, 0L, SEEK_END );
            uint32_t filelen = ftell( file_in );
            uint32_t file_end_pos ;
            rewind( file_in );

            //Write header
            fseek( out, cur_header, SEEK_SET );
            fwrite( cur_file, strlen( cur_file ) + 1, 1, out );

            fseek( out, cur_header + header_instance_len - 16, SEEK_SET );
            fwrite( &cur_data_block, 4, 1, out );
            fwrite( &filelen, 4, 1, out );
            
            //Write body
            fseek( out, cur_data_block, SEEK_SET );
            fwritestream( file_in, out ); 

            //Move datablock beginning to next file position.
            cur_data_block += (filelen / 16) * 16 + 16;
            //Move data header to next file position.
            cur_header += header_instance_len;
        }
        else
        {
            fprintf( stderr, "Unexpected error trying to reopen file. ABORT\n" );
            exit(1);
        }

        fclose( file_in );
    }

    fseek(out, 0L, SEEK_SET );
   
    fwritestream( out, stdout ); 

    fclose(out);

    return 0;
}
