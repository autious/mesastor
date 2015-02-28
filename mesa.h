#ifndef MESA_H
#define MESA_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef MESA_MALLOC
#   define MESA_MALLOC(size) malloc(size)
#endif

#ifndef MESA_FREE
#   define MESA_FREE(ptr) free(ptr)
#endif

enum mesastor_type
{
    MESA_FHANDLE,
    MESA_BUF
};

enum mesastor_error 
{
    MESA_ERR_NO = 0,
    MESA_ERR_INVALID_FILE_FORMAT,
    MESA_ERR_MALFORMED_ENDIANESS,
    MESA_ERR_UNSUPPORTED_ENDIANESS,
    MESA_ERR_UNSUPPORTED_VERSION,
    MESA_ERR_NO_SUCH_FILE,
    MESA_ERR_CORRUPT_FILE,
    MESA_ERR_UNKNOWN_IO,
    MESA_ERR_EBADF,
    MESA_ERR_EOF,
    MESA_ERR_READ_FERROR,
    MESA_ERR_FILE_NOT_FOUND,
    MESA_NOT_IMPLEMENTED,
    MESA_ERR_MALFORMED_MESASTOR
};

enum mesafile_error
{
    MESAFILE_ERR_NO = 0
};

struct mesastor
{
    union
    {
        FILE* file;
        const char buf;
    } src;

    enum mesastor_type type;

    enum mesastor_error err;

    bool flip_endianess;

    uint32_t instance_len;
    uint32_t instance_count;
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

//VS2013 seems to compile .c files with C style name mangling.
//If we want .cpp files using these functions to see them we have to 
//tell the compiler that they are mangled thusly.
#ifdef __cplusplus
#define CPPEXT extern "C"
#else
#define CPPEXT 
#endif

CPPEXT MESASTOR*   mesa_open(const char* archive_name);
CPPEXT MESASTOR*   mesa_openf(FILE * file);
//MESASTOR*   mesa_openbuf( const char* buf, size_t size );

CPPEXT void        mesa_close(MESASTOR* file);

CPPEXT MESAFILE*   mesa_fopen(MESASTOR* file, const char* filename);
CPPEXT void        mesa_fclose(MESAFILE* file);

CPPEXT size_t      mesa_fsize(MESAFILE* stream);
CPPEXT size_t      mesa_fread(void *ptr, size_t size, size_t nmemb, MESAFILE* stream);

//Returns an array with pointers to strings 
CPPEXT char**      mesa_getfiles(MESASTOR* stor);
CPPEXT void        mesa_getfiles_free(char** var);

CPPEXT int         mesa_has_error(MESASTOR* stor);

CPPEXT const char * mesa_get_error_string(MESASTOR* stor);

#endif
