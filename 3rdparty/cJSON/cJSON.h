/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__WINDOWS__) && \
    (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* When compiling for windows, we specify a specific calling convention to avoid
issues where we are being called from a project with a different default calling
convention.  For windows you have 3 define options:

CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever
dllexport symbols CJSON_EXPORT_SYMBOLS - Define this on library build when you
want to dllexport symbols (default) CJSON_IMPORT_SYMBOLS - Define this if you
want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar
behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the CJSON_API_VISIBILITY flag to "export" the same symbols the way
CJSON_EXPORT_SYMBOLS does

*/

#define CJSON_CDECL __cdecl
#define CJSON_STDCALL __stdcall

/* export symbols by default, this is necessary for copy pasting the C and
 * header file */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && \
    !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
#define CJSON_PUBLIC(type) type CJSON_STDCALL
#elif defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_PUBLIC(type) __declspec(dllexport) type CJSON_STDCALL
#elif defined(CJSON_IMPORT_SYMBOLS)
#define CJSON_PUBLIC(type) __declspec(dllimport) type CJSON_STDCALL
#endif
#else /* !__WINDOWS__ */
#define CJSON_CDECL
#define CJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined(__SUNPRO_C)) && \
    defined(CJSON_API_VISIBILITY)
#define CJSON_PUBLIC(type) __attribute__((visibility("default"))) type
#else
#define CJSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 7
#define CJSON_VERSION_PATCH 13

#include <stddef.h>

/* cJSON Types: */
#define cJSON_Invalid (0)
#define cJSON_False (1 << 0)
#define cJSON_True (1 << 1)
#define cJSON_NULL (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array (1 << 5)
#define cJSON_Object (1 << 6)
#define cJSON_Raw (1 << 7) /* raw json */

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

/* The cJSON structure: */
typedef struct cJSON {
  /* next/prev allow you to walk array/object chains. Alternatively, use
   * GetArraySize/GetArrayItem/GetObjectItem */
  struct cJSON *next;
  struct cJSON *prev;
  /* An array or object item will have a child pointer pointing to a chain of
   * the items in the array/object. */
  struct cJSON *child;

  /* The type of the item, as above. */
  int type;

  /* The item's string, if type==cJSON_String  and type == cJSON_Raw */
  char *valuestring;
  /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
  long long int valueint;
  /* The item's number, if type==cJSON_Number */
  double valuedouble;

  /* The item's name string, if this item is the child of, or is in the list of
   * subitems of an object. */
  char *string;
} cJSON;

typedef int cJSON_bool;

/* Limits how deeply nested arrays/objects can be before cJSON rejects to parse
 * them. This is to prevent stack overflows. */
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

/* Memory Management: the caller is always responsible to free the results from
 * all variants of cJSON_Parse (with cJSON_Delete) and cJSON_Print (with stdlib
 * free, cJSON_Hooks.free_fn, or cJSON_free as appropriate). The exception is
 * cJSON_PrintPreallocated, where the caller has full responsibility of the
 * buffer. */
/* Supply a block of JSON, and this returns a cJSON object you can interrogate.
 */
CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value);
/* ParseWithOpts allows you to require (and check) that the JSON is null
 * terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then
 * return_parse_end will contain a pointer to the error so will match
 * cJSON_GetErrorPtr(). */
CJSON_PUBLIC(cJSON *)
cJSON_ParseWithOpts(const char *value, const char **return_parse_end,
                    cJSON_bool require_null_terminated);
CJSON_PUBLIC(cJSON *)
cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length,
                          const char **return_parse_end,
                          cJSON_bool require_null_terminated);

/* Render a cJSON entity to text for transfer/storage without any formatting. */
CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item);
/* Delete a cJSON entity and all subentities. */
CJSON_PUBLIC(void) cJSON_Delete(cJSON *item);

/* Returns the number of items in an array (or object). */
CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array);
/* Retrieve item number "index" from array "array". Returns NULL if
 * unsuccessful. */
CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index);
/* Get item "string" from object. Case insensitive. */
CJSON_PUBLIC(cJSON *)
cJSON_GetObjectItem(const cJSON *const object, const char *const string);

/* These functions check the type of an item */
CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON *const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON *const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON *const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON *const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON *const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON *const item);

/* These calls create a cJSON item of the appropriate type. */
CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool boolean);
CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num, long long int llint);
CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string);
/* raw json */
CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(const char *raw);
CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void);

/* Append item to the specified array/object. */
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(cJSON_bool)
cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);

/* Remove/Detach items from Arrays/Objects. */
CJSON_PUBLIC(cJSON *)
cJSON_DetachItemViaPointer(cJSON *parent, cJSON *const item);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
CJSON_PUBLIC(cJSON *)
cJSON_AddNumberToObject(cJSON *const object, const char *const name,
                        const double number, const long long int llint);
CJSON_PUBLIC(cJSON *)
cJSON_AddStringToObject(cJSON *const object, const char *const name,
                        const char *const string);

/* malloc/free objects using the malloc/free functions that have been set with
 * cJSON_InitHooks */

#ifdef __cplusplus
}
#endif

#endif
