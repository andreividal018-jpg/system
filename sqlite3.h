/*
 * Minimal sqlite3.h stub for N003 BSIT System
 * Declarations for functions used by the system only.
 * Links against /usr/lib/x86_64-linux-gnu/libsqlite3.so.0
 */
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef long long sqlite3_int64;

#define SQLITE_OK       0
#define SQLITE_ROW      100
#define SQLITE_DONE     101

#define SQLITE_OPEN_READWRITE  0x00000002
#define SQLITE_OPEN_CREATE     0x00000004

#define SQLITE_NULL     5
#define SQLITE_INTEGER  1
#define SQLITE_FLOAT    2
#define SQLITE_TEXT     3
#define SQLITE_BLOB     4

#define SQLITE_TRANSIENT  ((void(*)(void*))-1)

int   sqlite3_open_v2(const char*, sqlite3**, int, const char*);
int   sqlite3_close(sqlite3*);
int   sqlite3_exec(sqlite3*, const char*, int(*)(void*,int,char**,char**), void*, char**);
void  sqlite3_free(void*);
const char* sqlite3_errmsg(sqlite3*);
sqlite3_int64 sqlite3_last_insert_rowid(sqlite3*);

int   sqlite3_prepare_v2(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
int   sqlite3_step(sqlite3_stmt*);
int   sqlite3_reset(sqlite3_stmt*);
int   sqlite3_finalize(sqlite3_stmt*);

int         sqlite3_bind_int   (sqlite3_stmt*, int, int);
int         sqlite3_bind_double(sqlite3_stmt*, int, double);
int         sqlite3_bind_text  (sqlite3_stmt*, int, const char*, int, void(*)(void*));
int         sqlite3_bind_null  (sqlite3_stmt*, int);

int            sqlite3_column_count(sqlite3_stmt*);
int            sqlite3_column_type (sqlite3_stmt*, int);
int            sqlite3_column_int  (sqlite3_stmt*, int);
double         sqlite3_column_double(sqlite3_stmt*, int);
const unsigned char* sqlite3_column_text(sqlite3_stmt*, int);
const char*    sqlite3_column_name (sqlite3_stmt*, int);

#ifdef __cplusplus
}
#endif
