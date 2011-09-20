/**
 * @file bson.h
 * @brief BSON Declarations
 */

/*    Copyright 2009-2011 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef _BSON_H_
#define _BSON_H_

#include "platform.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

MONGO_EXTERN_C_START

#define BSON_OK 0
#define BSON_ERROR -1

enum bson_error_t {
    BSON_SIZE_OVERFLOW = 1 /**< Trying to create a BSON object larger than INT_MAX. */
};

enum bson_validity_t {
    BSON_VALID = 0,                 /**< BSON is valid and UTF-8 compliant. */
    BSON_NOT_UTF8 = ( 1<<1 ),       /**< A key or a string is not valid UTF-8. */
    BSON_FIELD_HAS_DOT = ( 1<<2 ),  /**< Warning: key contains '.' character. */
    BSON_FIELD_INIT_DOLLAR = ( 1<<3 ), /**< Warning: key starts with '$' character. */
    BSON_ALREADY_FINISHED = ( 1<<4 )  /**< Trying to modify a finished BSON object. */
};

enum bson_binary_subtype_t {
    BSON_BIN_BINARY = 0,
    BSON_BIN_FUNC = 1,
    BSON_BIN_BINARY_OLD = 2,
    BSON_BIN_UUID = 3,
    BSON_BIN_MD5 = 5,
    BSON_BIN_USER = 128
};

typedef enum {
    BSON_EOO = 0,
    BSON_DOUBLE = 1,
    BSON_STRING = 2,
    BSON_OBJECT = 3,
    BSON_ARRAY = 4,
    BSON_BINDATA = 5,
    BSON_UNDEFINED = 6,
    BSON_OID = 7,
    BSON_BOOL = 8,
    BSON_DATE = 9,
    BSON_NULL = 10,
    BSON_REGEX = 11,
    BSON_DBREF = 12, /**< Deprecated. */
    BSON_CODE = 13,
    BSON_SYMBOL = 14,
    BSON_CODEWSCOPE = 15,
    BSON_INT = 16,
    BSON_TIMESTAMP = 17,
    BSON_LONG = 18
} bson_type;

typedef int bson_bool_t;

typedef struct {
    const char *cur;
    bson_bool_t first;
} bson_iterator;

typedef struct {
    char *data;
    char *cur;
    int dataSize;
    bson_bool_t finished;
    size_t stack[32];
    int stackPos;
    int err; /**< Bitfield representing errors or warnings on this buffer */
    char *errstr; /**< A string representation of the most recent error or warning. */
} bson;

#pragma pack(1)
typedef union {
    char bytes[12];
    int ints[3];
} bson_oid_t;
#pragma pack()

typedef int64_t bson_date_t; /* milliseconds since epoch UTC */

typedef struct {
    int i; /* increment */
    int t; /* time in seconds */
} bson_timestamp_t;

/* ----------------------------
   READING
   ------------------------------ */

/**
 * Size of a BSON object.
 *
 * @param b the BSON object.
 *
 * @return the size.
 */
int bson_size( const bson *b );

/**
 * Print a string representation of a BSON object.
 *
 * @param b the BSON object to print.
 */
void bson_print( bson *b );

/**
 * Return a pointer to the raw buffer stored by this bson object.
 *
 * @param b a BSON object
 */
const char *bson_data( bson *b );

/**
 * Print a string representation of a BSON object.
 *
 * @param bson the raw data to print.
 * @param depth the depth to recurse the object.x
 */
void bson_print_raw( const char *bson , int depth );

/**
 * Advance a bson_iterator to the named field.
 *
 * @param it the bson_iterator to use.
 * @param obj the BSON object to use.
 * @param name the name of the field to find.
 *
 * @return the type of the found object or BSON_EOO if it is not found.
 */
bson_type bson_find( bson_iterator *it, const bson *obj, const char *name );

/**
 * Initialize a bson_iterator.
 *
 * @param i the bson_iterator to initialize.
 * @param bson the BSON object to associate with the iterator.
 */
void bson_iterator_init( bson_iterator *i , const bson *b );

/**
 * Initialize a bson iterator from a const char* buffer. Note
 * that this is mostly used internally.
 *
 * @param i the bson_iterator to initialize.
 * @param buffer the buffer to point to.
 */
void bson_iterator_from_buffer( bson_iterator *i, const char *buffer );

/* more returns true for eoo. best to loop with bson_iterator_next(&it) */
/**
 * Check to see if the bson_iterator has more data.
 *
 * @param i the iterator.
 *
 * @return  returns true if there is more data.
 */
bson_bool_t bson_iterator_more( const bson_iterator *i );

/**
 * Point the iterator at the next BSON object.
 *
 * @param i the bson_iterator.
 *
 * @return the type of the next BSON object.
 */
bson_type bson_iterator_next( bson_iterator *i );

/**
 * Get the type of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the type of the current BSON object.
 */
bson_type bson_iterator_type( const bson_iterator *i );

/**
 * Get the key of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return the key of the current BSON object.
 */
const char *bson_iterator_key( const bson_iterator *i );

/**
 * Get the value of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the value of the current BSON object.
 */
const char *bson_iterator_value( const bson_iterator *i );

/* these convert to the right type (return 0 if non-numeric) */
/**
 * Get the double value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the value of the current BSON object.
 */
double bson_iterator_double( const bson_iterator *i );

/**
 * Get the int value of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the value of the current BSON object.
 */
int bson_iterator_int( const bson_iterator *i );

/**
 * Get the long value of the BSON object currently pointed to by the iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
int64_t bson_iterator_long( const bson_iterator *i );

/* return the bson timestamp as a whole or in parts */
/**
 * Get the timestamp value of the BSON object currently pointed to by
 * the iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
bson_timestamp_t bson_iterator_timestamp( const bson_iterator *i );

/**
 * Get the boolean value of the BSON object currently pointed to by
 * the iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
/* false: boolean false, 0 in any type, or null */
/* true: anything else (even empty strings and objects) */
bson_bool_t bson_iterator_bool( const bson_iterator *i );

/**
 * Get the double value of the BSON object currently pointed to by the
 * iterator. Assumes the correct type is used.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
/* these assume you are using the right type */
double bson_iterator_double_raw( const bson_iterator *i );

/**
 * Get the int value of the BSON object currently pointed to by the
 * iterator. Assumes the correct type is used.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
int bson_iterator_int_raw( const bson_iterator *i );

/**
 * Get the long value of the BSON object currently pointed to by the
 * iterator. Assumes the correct type is used.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
int64_t bson_iterator_long_raw( const bson_iterator *i );

/**
 * Get the bson_bool_t value of the BSON object currently pointed to by the
 * iterator. Assumes the correct type is used.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
bson_bool_t bson_iterator_bool_raw( const bson_iterator *i );

/**
 * Get the bson_oid_t value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON object.
 */
bson_oid_t *bson_iterator_oid( const bson_iterator *i );

/**
 * Get the string value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return  the value of the current BSON object.
 */
/* these can also be used with bson_code and bson_symbol*/
const char *bson_iterator_string( const bson_iterator *i );

/**
 * Get the string length of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the length of the current BSON object.
 */
int bson_iterator_string_len( const bson_iterator *i );

/**
 * Get the code value of the BSON object currently pointed to by the
 * iterator. Works with bson_code, bson_codewscope, and BSON_STRING
 * returns NULL for everything else.
 *
 * @param i the bson_iterator
 *
 * @return the code value of the current BSON object.
 */
/* works with bson_code, bson_codewscope, and BSON_STRING */
/* returns NULL for everything else */
const char *bson_iterator_code( const bson_iterator *i );

/**
 * Calls bson_empty on scope if not a bson_codewscope
 *
 * @param i the bson_iterator.
 * @param scope the bson scope.
 */
/* calls bson_empty on scope if not a bson_codewscope */
void bson_iterator_code_scope( const bson_iterator *i, bson *scope );

/**
 * Get the date value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the date value of the current BSON object.
 */
/* both of these only work with bson_date */
bson_date_t bson_iterator_date( const bson_iterator *i );

/**
 * Get the time value of the BSON object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the time value of the current BSON object.
 */
time_t bson_iterator_time_t( const bson_iterator *i );

/**
 * Get the length of the BSON binary object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the length of the current BSON binary object.
 */
int bson_iterator_bin_len( const bson_iterator *i );

/**
 * Get the type of the BSON binary object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the type of the current BSON binary object.
 */
char bson_iterator_bin_type( const bson_iterator *i );

/**
 * Get the value of the BSON binary object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON binary object.
 */
const char *bson_iterator_bin_data( const bson_iterator *i );

/**
 * Get the value of the BSON regex object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator
 *
 * @return the value of the current BSON regex object.
 */
const char *bson_iterator_regex( const bson_iterator *i );

/**
 * Get the options of the BSON regex object currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator.
 *
 * @return the options of the current BSON regex object.
 */
const char *bson_iterator_regex_opts( const bson_iterator *i );

/* these work with BSON_OBJECT and BSON_ARRAY */
/**
 * Get the BSON subobject currently pointed to by the
 * iterator.
 *
 * @param i the bson_iterator.
 * @param sub the BSON subobject destination.
 */
void bson_iterator_subobject( const bson_iterator *i, bson *sub );

/**
 * Get a bson_iterator that on the BSON subobject.
 *
 * @param i the bson_iterator.
 * @param sub the iterator to point at the BSON subobject.
 */
void bson_iterator_subiterator( const bson_iterator *i, bson_iterator *sub );

/* str must be at least 24 hex chars + null byte */
/**
 * Create a bson_oid_t from a string.
 *
 * @param oid the bson_oid_t destination.
 * @param str a null terminated string comprised of at least 24 hex chars.
 */
void bson_oid_from_string( bson_oid_t *oid, const char *str );

/**
 * Create a string representation of the bson_oid_t.
 *
 * @param oid the bson_oid_t source.
 * @param str the string representation destination.
 */
void bson_oid_to_string( const bson_oid_t *oid, char *str );

/**
 * Create a bson_oid object.
 *
 * @param oid the destination for the newly created bson_oid_t.
 */
void bson_oid_gen( bson_oid_t *oid );

/**
 * Set a function to be used to generate the second four bytes
 * of an object id.
 *
 * @param func a pointer to a function that returns an int.
 */
void bson_set_oid_fuzz( int ( *func )( void ) );

/**
 * Set a function to be used to generate the incrementing part
 * of an object id (last four bytes). If you need thread-safety
 * in generating object ids, you should set this function.
 *
 * @param func a pointer to a function that returns an int.
 */
void bson_set_oid_inc( int ( *func )( void ) );

/**
 * Get the time a bson_oid_t was created.
 *
 * @param oid the bson_oid_t.
 */
time_t bson_oid_generated_time( bson_oid_t *oid ); /* Gives the time the OID was created */

/* ----------------------------
   BUILDING
   ------------------------------ */

/**
 *  Initialize a new bson object. If not created
 *  with bson_new, you must initialize each new bson
 *  object using this function.
 *
 *  @note When finished, you must pass the bson object to
 *      bson_destroy( ).
 */
void bson_init( bson *b );

/**
 * Initialize a BSON object, and point its data
 * pointer to the provided char*.
 *
 * @param b the BSON object to initialize.
 * @param data the raw BSON data.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_init_data( bson *b , char *data );

/**
 * Initialize a BSON object, and point its data
 * pointer to the provided char*. We assume
 * that the data represents a finished BSON object.
 *
 * @param b the BSON object to initialize.
 * @param data the raw BSON data.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_init_finished_data( bson *b, char *data );

/**
 * Initialize a BSON object, and set its
 * buffer to the given size.
 *
 * @param b the BSON object to initialize.
 * @param size the initial size of the buffer.
 *
 * @return BSON_OK or BSON_ERROR.
 */
void bson_init_size( bson *b, int size );

/**
 * Grow a bson object.
 *
 * @param b the bson to grow.
 * @param bytesNeeded the additional number of bytes needed.
 *
 * @return BSON_OK or BSON_ERROR with the bson error object set.
 *   Exits if allocation fails.
 */
int bson_ensure_space( bson *b, const size_t bytesNeeded );

/**
 * Finalize a bson object.
 *
 * @param b the bson object to finalize.
 *
 * @return the standard error code. To deallocate memory,
 *   call bson_destroy on the bson object.
 */
int bson_finish( bson *b );

/**
 * Destroy a bson object.
 *
 * @param b the bson object to destroy.
 *
 */
void bson_destroy( bson *b );

/**
 * Returns a pointer to a static empty BSON object.
 *
 * @param obj the BSON object to initialize.
 *
 * @return the empty initialized BSON object.
 */
/* returns pointer to static empty bson object */
bson *bson_empty( bson *obj );

/**
 * Make a complete copy of the a BSON object.
 * The source bson object must be in a finished
 * state; otherwise, the copy will fail.
 *
 * @param out the copy destination BSON object.
 * @param in the copy source BSON object.
 */
int bson_copy( bson *out, const bson *in ); /* puts data in new buffer. NOOP if out==NULL */

/**
 * Append a previously created bson_oid_t to a bson object.
 *
 * @param b the bson to append to.
 * @param name the key for the bson_oid_t.
 * @param oid the bson_oid_t to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_oid( bson *b, const char *name, const bson_oid_t *oid );

/**
 * Append a bson_oid_t to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the bson_oid_t.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_new_oid( bson *b, const char *name );

/**
 * Append an int to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the int.
 * @param i the int to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_int( bson *b, const char *name, const int i );

/**
 * Append an long to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the long.
 * @param i the long to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_long( bson *b, const char *name, const int64_t i );

/**
 * Append an double to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the double.
 * @param d the double to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_double( bson *b, const char *name, const double d );

/**
 * Append a string to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the string.
 * @param str the string to append.
 *
 * @return BSON_OK or BSON_ERROR.
*/
int bson_append_string( bson *b, const char *name, const char *str );

/**
 * Append len bytes of a string to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the string.
 * @param str the string to append.
 * @param len the number of bytes from str to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_string_n( bson *b, const char *name, const char *str, size_t len );

/**
 * Append a symbol to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the symbol.
 * @param str the symbol to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_symbol( bson *b, const char *name, const char *str );

/**
 * Append len bytes of a symbol to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the symbol.
 * @param str the symbol to append.
 * @param len the number of bytes from str to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_symbol_n( bson *b, const char *name, const char *str, int len );

/**
 * Append code to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the code.
 * @param str the code to append.
 * @param len the number of bytes from str to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_code( bson *b, const char *name, const char *str );

/**
 * Append len bytes of code to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the code.
 * @param str the code to append.
 * @param len the number of bytes from str to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_code_n( bson *b, const char *name, const char *str, int len );

/**
 * Append code to a bson with scope.
 *
 * @param b the bson to append to.
 * @param name the key for the code.
 * @param str the string to append.
 * @param scope a BSON object containing the scope.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_code_w_scope( bson *b, const char *name, const char *code, const bson *scope );

/**
 * Append len bytes of code to a bson with scope.
 *
 * @param b the bson to append to.
 * @param name the key for the code.
 * @param str the string to append.
 * @param len the number of bytes from str to append.
 * @param scope a BSON object containing the scope.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_code_w_scope_n( bson *b, const char *name, const char *code, size_t size, const bson *scope );

/**
 * Append binary data to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the data.
 * @param type the binary data type.
 * @param str the binary data.
 * @param len the length of the data.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_binary( bson *b, const char *name, char type, const char *str, size_t len );

/**
 * Append a bson_bool_t to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the boolean value.
 * @param v the bson_bool_t to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_bool( bson *b, const char *name, const bson_bool_t v );

/**
 * Append a null value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the null value.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_null( bson *b, const char *name );

/**
 * Append an undefined value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the undefined value.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_undefined( bson *b, const char *name );

/**
 * Append a regex value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the regex value.
 * @param pattern the regex pattern to append.
 * @param the regex options.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_regex( bson *b, const char *name, const char *pattern, const char *opts );

/**
 * Append bson data to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the bson data.
 * @param bson the bson object to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_bson( bson *b, const char *name, const bson *bson );

/**
 * Append a BSON element to a bson from the current point of an iterator.
 *
 * @param b the bson to append to.
 * @param name_or_null the key for the BSON element, or NULL.
 * @param elem the bson_iterator.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_element( bson *b, const char *name_or_null, const bson_iterator *elem );

/**
 * Append a bson_timestamp_t value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the timestampe value.
 * @param ts the bson_timestamp_t value to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_timestamp( bson *b, const char *name, bson_timestamp_t *ts );

/* these both append a bson_date */
/**
 * Append a bson_date_t value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the date value.
 * @param millis the bson_date_t to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_date( bson *b, const char *name, bson_date_t millis );

/**
 * Append a time_t value to a bson.
 *
 * @param b the bson to append to.
 * @param name the key for the date value.
 * @param secs the time_t to append.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_time_t( bson *b, const char *name, time_t secs );

/**
 * Start appending a new object to a bson.
 *
 * @param b the bson to append to.
 * @param name the name of the new object.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_start_object( bson *b, const char *name );

/**
 * Start appending a new array to a bson.
 *
 * @param b the bson to append to.
 * @param name the name of the new array.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_start_array( bson *b, const char *name );

/**
 * Finish appending a new object or array to a bson.
 *
 * @param b the bson to append to.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_finish_object( bson *b );

/**
 * Finish appending a new object or array to a bson. This
 * is simply an alias for bson_append_finish_object.
 *
 * @param b the bson to append to.
 *
 * @return BSON_OK or BSON_ERROR.
 */
int bson_append_finish_array( bson *b );

void bson_numstr( char *str, int i );

void bson_incnumstr( char *str );

/* Error handling and stadard library function over-riding. */
/* -------------------------------------------------------- */

/* bson_err_handlers shouldn't return!!! */
typedef void( *bson_err_handler )( const char *errmsg );

typedef int (*bson_printf_func)( const char *, ... );
typedef int (*bson_fprintf_func)( FILE *, const char *, ... );
typedef int (*bson_sprintf_func)( char *, const char *, ... );

extern void *( *bson_malloc_func )( size_t );
extern void *( *bson_realloc_func )( void *, size_t );
extern void ( *bson_free )( void * );

extern bson_printf_func bson_printf;
extern bson_fprintf_func bson_fprintf;
extern bson_sprintf_func bson_sprintf;

extern bson_printf_func bson_errprintf;

/**
 * Allocates memory and checks return value, exiting fatally if malloc() fails.
 *
 * @param size bytes to allocate.
 *
 * @return a pointer to the allocated memory.
 *
 * @sa malloc(3)
 */
void *bson_malloc( size_t size );

/**
 * Changes the size of allocated memory and checks return value,
 * exiting fatally if realloc() fails.
 *
 * @param ptr pointer to the space to reallocate.
 * @param size bytes to allocate.
 *
 * @return a pointer to the allocated memory.
 *
 * @sa realloc()
 */
void *bson_realloc( void *ptr, size_t size );

/**
 * Set a function for error handling.
 *
 * @param func a bson_err_handler function.
 *
 * @return the old error handling function, or NULL.
 */
bson_err_handler set_bson_err_handler( bson_err_handler func );

/* does nothing if ok != 0 */
/**
 * Exit fatally.
 *
 * @param ok exits if ok is equal to 0.
 */
void bson_fatal( int ok );

/**
 * Exit fatally with an error message.
  *
 * @param ok exits if ok is equal to 0.
 * @param msg prints to stderr before exiting.
 */
void bson_fatal_msg( int ok, const char *msg );

/**
 * Invoke the error handler, but do not exit.
 *
 * @param b the buffer object.
 */
void bson_builder_error( bson *b );

MONGO_EXTERN_C_END
#endif
