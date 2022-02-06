#include <stdio.h>
#include <stdint.h>

#define OK 0
#define ERROR_EXISTS -1
#define ERROR_UNEXISTS -2

typedef unsigned long db_id_t;
typedef unsigned long db_uint_t;

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
    db_uint_t slave_count;
} master_record_t;

typedef struct {
    db_id_t id;
    db_id_t master_id;
    db_id_t next;
} slave_record_t;

int master_insert(db_t db, const master_record_t *record);
int master_get(db_t db, master_record_t *record);
int master_update(db_t db, const master_record_t *record);
int master_delete(db_t db, db_id_t id);
long master_count(db_t db);

int slave_insert(db_t db, slave_record_t *record);
int slave_get(db_t db, slave_record_t *record);
long slave_count(db_t db);
