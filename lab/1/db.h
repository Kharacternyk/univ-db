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

typedef struct {
    db_id_t id;
    db_id_t slave_id;
} master_record_t;

typedef struct {
    db_id_t id;
    db_id_t master_id;
    db_id_t next;
} slave_record_t;

int master_insert(db_t db, const master_record_t *record);
int master_get(db_t db, db_id_t id, master_record_t *record);
int master_delete(db_t db, db_id_t id);
long master_count(db_t db);
