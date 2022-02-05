#include "db.h"

#define FWRITE(data, file) fwrite(&data, sizeof data, 1, file)
#define FREAD(data, file) fread(&data, sizeof data, 1, file)

static long get_offset(FILE *index, db_id_t id) {
    fseek(index, 0, SEEK_SET);
    for (;;) {
        db_id_t current_id;
        long offset = -1;

        if (!FREAD(current_id, index)) {
            return offset;
        }

        FREAD(offset, index);

        if (current_id == id) {
            return offset;
        }
    }
}

static void add_offset(FILE *index, db_id_t id, long offset) {
    fseek(index, 0, SEEK_END);
    FWRITE(id, index);
    FWRITE(offset, index);
}

void master_insert(db_t db, db_id_t id, const void *data) {
    fseek(db.master.data, 0, SEEK_END);
    add_offset(db.master.index, id, ftell(db.master.data));
    fwrite(data, db.master.record_size, 1, db.master.data);
}

int master_get(db_t db, db_id_t id, void *data) {
    const long offset = get_offset(db.master.index, id);
    if (offset == -1) {
        return 0;
    }

    fseek(db.master.data, offset, SEEK_SET);
    fread(data, db.master.record_size, 1, db.master.data);
    return 1;
}
