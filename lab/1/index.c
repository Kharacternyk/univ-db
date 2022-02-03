#include "index.h"

long get_offset(FILE *index, db_id_t id) {
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

void add_offset(FILE *index, db_id_t id, long offset) {
    fseek(index, 0, SEEK_END);
    FWRITE(id, index);
    FWRITE(offset, index);
}
