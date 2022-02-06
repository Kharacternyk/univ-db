#include <unistd.h>
#include <stdlib.h>
#include "db.h"

#define FWRITE(data, file) fwrite(&data, sizeof data, 1, file)
#define FREAD(data, file) fread(&data, sizeof data, 1, file)

typedef struct {
    db_id_t id;
    long offset;
} index_entry_t;

static long index_count(FILE *index) {
    fseek(index, 0, SEEK_END);
    return ftell(index) / (sizeof (index_entry_t));
}

static long index_get(FILE *index, db_id_t id, int remove) {
    long /* exclusive */ upper_bound = index_count(index);
    long /* inclusive */ lower_bound = 0;

    for (;;) {
        if (upper_bound == lower_bound) {
            return ERROR_UNEXISTS;
        }

        const long position = lower_bound + (upper_bound - lower_bound) / 2;
        index_entry_t entry;

        fseek(index, position * sizeof (index_entry_t), SEEK_SET);
        FREAD(entry, index);

        if (entry.id > id) {
            upper_bound = position;
        } else if (entry.id < id) {
            lower_bound = position + 1;
        } else {
            if (remove) {
                index_entry_t entry;
                while (FREAD(entry, index)) {
                    fseek(index, -2 * sizeof entry, SEEK_CUR);
                    FWRITE(entry, index);
                    fseek(index, sizeof entry, SEEK_CUR);
                }
                ftruncate(fileno(index), (index_count(index) - 1) * sizeof entry);
            }
            return entry.offset;
        }
    }
}

static int index_add(FILE *index, db_id_t id, long offset) {
    long /* inclusive */ upper_bound = index_count(index);
    long /* inclusive */ lower_bound = 0;

    for (;;) {
        if (upper_bound == lower_bound) {
            break;
        }

        const long position = lower_bound + (upper_bound - lower_bound) / 2;

        fseek(index, position * sizeof (index_entry_t), SEEK_SET);

        index_entry_t entry;
        FREAD(entry, index);

        if (entry.id > id) {
            upper_bound = position;
        } else if (entry.id < id) {
            lower_bound = position + 1;
        } else {
            return ERROR_EXISTS;
        }
    }

    fseek(index, 0, SEEK_END);

    while (ftell(index) / sizeof (index_entry_t) > lower_bound) {
        index_entry_t entry;
        fseek(index, -sizeof entry, SEEK_CUR);
        FREAD(entry, index);
        FWRITE(entry, index);
        fseek(index, -2 * sizeof entry, SEEK_CUR);
    }

    const index_entry_t entry = {
        .id = id,
        .offset = offset,
    };

    FWRITE(entry, index);

    return OK;
}

static long pool_get(FILE *pool) {
    fseek(pool, 0, SEEK_END);
    const long size = ftell(pool);

    if (!size) {
        return ERROR_UNEXISTS;
    }

    fseek(pool, -sizeof (long), SEEK_END);

    long offset;
    FREAD(offset, pool);
    ftruncate(fileno(pool), size - sizeof (long));

    return offset;
}

static void pool_add(FILE *pool, long offset) {
    fseek(pool, 0, SEEK_END);
    FWRITE(offset, pool);
}

static int insert(table_t table, const void *record, db_id_t id) {
    const long pool_offset = pool_get(table.pool);
    long offset;

    if (pool_offset == ERROR_UNEXISTS) {
        fseek(table.data, 0, SEEK_END);
        offset = ftell(table.data);
    } else {
        fseek(table.data, pool_offset, SEEK_SET);
        offset = pool_offset;
    }

    if (index_add(table.index, id, offset) == ERROR_EXISTS) {
        if (pool_offset != ERROR_UNEXISTS) {
            pool_add(table.pool, pool_offset);
        }
        return ERROR_EXISTS;
    }

    fwrite(record, table.record_size, 1, table.data);
    return OK;
}

static int get(table_t table, void *record, db_id_t id) {
    const long offset = index_get(table.index, id, 0);
    if (offset == ERROR_UNEXISTS) {
        return ERROR_UNEXISTS;
    }

    fseek(table.data, offset, SEEK_SET);
    fread(record, table.record_size, 1, table.data);
    return OK;
}

static int update(table_t table, const void *record, db_id_t id) {
    const long offset = index_get(table.index, id, 0);
    if (offset == ERROR_UNEXISTS) {
        return ERROR_UNEXISTS;
    }

    fseek(table.data, offset, SEEK_SET);
    fwrite(record, table.record_size, 1, table.data);
    return OK;
}

static int delete(table_t table, db_id_t id) {
    const long offset = index_get(table.index, id, 1);
    if (offset == ERROR_UNEXISTS) {
        return ERROR_UNEXISTS;
    }

    pool_add(table.pool, offset);
    return OK;
}

int master_insert(db_t db, const master_record_t *record) {
    return insert(db.master, record, record->id);
}

int master_get(db_t db, master_record_t *record) {
    return get(db.master, record, record->id);
}

int master_update(db_t db, const master_record_t *record) {
    return update(db.master, record, record->id);
}

int master_delete(db_t db, db_id_t id) {
    master_record_t *record = malloc(db.master.record_size);
    if (get(db.master, record, id) == ERROR_UNEXISTS) {
        return ERROR_UNEXISTS;
    }
    delete(db.master, id);

    slave_record_t *slave = malloc(db.slave.record_size);
    slave->id = record->slave_id;

    while (record->slave_count--) {
        slave_get(db, slave);
        delete(db.slave, slave->id);
        slave->id = slave->next;
    }

    free(slave);
    free(record);
    return OK;
}

long master_count(db_t db) {
    return index_count(db.master.index);
}

int slave_insert(db_t db, slave_record_t *record) {
    master_record_t *master_record = malloc(db.master.record_size);
    master_record->id = record->master_id;

    int status = master_get(db, master_record);
    if (status != OK) {
        free(master_record);
        return status;
    }

    ++master_record->slave_count;
    record->next = master_record->slave_id;
    master_record->slave_id = record->id;

    status = insert(db.slave, record, record->id);
    if (status != OK) {
        return status;
    }

    master_update(db, master_record);
    free(master_record);
    return OK;
}

int slave_get(db_t db, slave_record_t *record) {
    return get(db.slave, record, record->id);
}

int slave_update(db_t db, const slave_record_t *record) {
    return update(db.slave, record, record->id);
}

int slave_delete(db_t db, db_id_t id) {
    slave_record_t *record = malloc(db.slave.record_size);
    if (get(db.slave, record, id) == ERROR_UNEXISTS) {
        return ERROR_UNEXISTS;
    }
    delete(db.slave, id);

    master_record_t *master_record = malloc(db.master.record_size);
    master_record->id = record->master_id;
    master_get(db, master_record);

    --master_record->slave_count;

    if (master_record->slave_id == id) {
        master_record->slave_id = record->next;
        master_update(db, master_record);
        free(master_record);
        free(record);
        return OK;
    }

    master_update(db, master_record);
    slave_record_t *sibling = malloc(db.slave.record_size);
    sibling->id = master_record->slave_id;

    while (master_record->slave_count--) {
        slave_get(db, sibling);
        if (sibling->next == record->id) {
            sibling->next = record->next;
            slave_update(db, sibling);
            break;
        }
        sibling->id = sibling->next;
    }

    free(master_record);
    free(record);
    free(sibling);
    return OK;
}

long slave_count(db_t db) {
    return index_count(db.slave.index);
}
