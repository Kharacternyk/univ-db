#include <stdio.h>
#include <stdint.h>

typedef int64_t db_id_t;

typedef struct {
    FILE *data;
    FILE *index;
    FILE *pool;
    size_t record_size;
} table_t;

typedef struct {
    table_t master;
    table_t slave;
} db_t;

void master_insert(db_t db, db_id_t id, const void *data);
int master_get(db_t db, db_id_t id, void *data);
