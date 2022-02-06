#include <unistd.h>
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
            return -1;
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
            return 0;
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

    return 1;
}

static long pool_get(FILE *pool) {
    fseek(pool, 0, SEEK_END);
    const long size = ftell(pool);

    if (!size) {
        return -1;
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

int master_insert(db_t db, const master_record_t *record) {
    const long pool_offset = pool_get(db.master.pool);
    long offset;

    if (pool_offset != -1) {
        fseek(db.master.data, pool_offset, SEEK_SET);
        offset = pool_offset;
    } else {
        fseek(db.master.data, 0, SEEK_END);
        offset = ftell(db.master.data);
    }

    if (!index_add(db.master.index, record->id, offset)) {
        if (pool_offset != -1) {
            pool_add(db.master.pool, pool_offset);
        }
        return 0;
    }

    fwrite(record, db.master.record_size, 1, db.master.data);
    return 1;
}

int master_get(db_t db, master_record_t *record) {
    const long offset = index_get(db.master.index, record->id, 0);
    if (offset == -1) {
        return 0;
    }

    fseek(db.master.data, offset, SEEK_SET);
    fread(record, db.master.record_size, 1, db.master.data);
    return 1;
}

int master_update(db_t db, const master_record_t *record) {
    const long offset = index_get(db.master.index, record->id, 0);
    if (offset == -1) {
        return 0;
    }

    fseek(db.master.data, offset, SEEK_SET);
    fwrite(record, db.master.record_size, 1, db.master.data);
    return 1;
}

int master_delete(db_t db, db_id_t id) {
    const long offset = index_get(db.master.index, id, 1);
    if (offset == -1) {
        return 0;
    }

    pool_add(db.master.pool, offset);
    return 1;
}

long master_count(db_t db) {
    return index_count(db.master.index);
}
