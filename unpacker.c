#include "mesa.h"
#include "mesa-constants.h"

int main( int argc, char **argv )
{
    if( argc == 2 )
    {
        MESASTOR* stor = mesa_open( argv[1] ); 

        if( !mesa_has_error( stor ) )
        {
            char** files = mesa_getfiles( stor );
                
            int i = 0;
            while( files[i] != NULL )
            {
                MESAFILE* file = mesa_fopen( stor, files[i] );  

                if( file )
                {
                    FILE * fout;

                    fprintf( stderr, "Opening %s\n", files[i] );
                    fout = fopen( files[i], "r" );

                    if( fout )
                    {
                        fprintf( stderr, "File already exists: %s\n", files[i] );
                        fclose( fout );
                    }
                    else
                    {
                        fout = fopen( files[i], "wb" );
                        if( fout == NULL )
                        {
                            perror("Unable to write");
                        }
                        else
                        {
                            size_t readl;
                            char buf[128];
                            do  
                            {
                                readl = mesa_fread( buf, 1, 128, file );
                                fwrite( buf, 1, readl, fout );
                            } while( readl > 0 );
                            fclose( fout );
                        }
                    }

                    mesa_fclose( file );
                }
                else
                {
                    fprintf( stderr, "unable to open file %s.\n", files[i] );
                }

                i++;
            }
        }

        mesa_close( stor );

        return 0;
    }
    else
    {
        return 1;
    }
}
