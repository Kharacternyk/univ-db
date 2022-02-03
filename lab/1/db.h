#ifndef DB_H
#define DB_H

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define DB_STR_LEN 128
#define TOKEN(queue) (strsep(&queue, " \t") ?: "")
#define DB_INT_TOKEN(dest, queue) dest = strtol(TOKEN(queue), NULL, 0)
#define DB_ID_TOKEN(dest, queue) DB_INT_TOKEN(dest, queue)
#define DB_STR_TOKEN(dest, queue) strncpy(dest, TOKEN(queue), DB_STR_LEN)
#define FWRITE(data, file) fwrite(&data, sizeof data, 1, file)
#define FREAD(data, file) fread(&data, sizeof data, 1, file)

typedef int64_t db_id_t;
typedef char db_str_t[DB_STR_LEN];
typedef int32_t db_int_t;

typedef struct {
    FILE *data;
    FILE *index;
    FILE *pool;
} table_t;

typedef struct {
    db_id_t id;
    db_str_t name;
    db_str_t country;
} publisher_t;

typedef struct {
    db_id_t id;
    db_id_t publisher_id;
    db_id_t next;
    db_str_t title;
    db_int_t year;
    db_int_t price;
} publication_t;

#endif
