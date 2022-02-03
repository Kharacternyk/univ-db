#ifndef INDEX_H
#define INDEX_H

#include <stdio.h>
#include "db.h"

long get_offset(FILE *index, db_id_t id);
void add_offset(FILE *index, db_id_t id, long offset);

#endif
