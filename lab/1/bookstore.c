/* clang-format off */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <readline/readline.h>
/* clang-format on */

#include "db.h"

#define TOKEN(queue) (strsep(&queue, " \t=") ?: "")
#define DB_ID_TOKEN(queue) strtol(TOKEN(queue), NULL, 0)
#define DB_INT_TOKEN(dest, queue) dest = strtol(TOKEN(queue), NULL, 0)
#define DB_STR_TOKEN(dest, queue) strncpy(dest, TOKEN(queue), DB_STR_LEN)
#define DB_STR_LEN 128

#define EXISTS(type) printf("error: %s exists\n", type)
#define NO(instance) printf("error: no such %s: %s\n", #instance, instance)
#define NO_TYPE(type) printf("error: no such %s\n", type)

typedef char db_str_t[DB_STR_LEN];
typedef int32_t db_int_t;

typedef struct {
    master_record_t meta;
    db_str_t name;
    db_str_t country;
} publisher_t;

typedef struct {
    slave_record_t meta;
    db_str_t title;
    db_int_t year;
    db_int_t price;
} book_t;

static int is_prefix(const char *prefix, const char *string, size_t min_length) {
    const size_t length = strlen(prefix);
    return length >= min_length && strncmp(prefix, string, length) == 0;
}

int main() {
    const db_t db = {
        .master = {
            .data = fopen("db/publishers.data", "r+"),
            .index = fopen("db/publishers.index", "r+"),
            .pool = fopen("db/publishers.pool", "r+"),
            .record_size = sizeof (publisher_t),
        },
        .slave = {
            .data = fopen("db/books.data", "r+"),
            .index = fopen("db/books.index", "r+"),
            .pool = fopen("db/books.pool", "r+"),
            .record_size = sizeof (book_t),
        },
    };

    for (;;) {
        char *command = readline("\nbookstore> ");

        if (!command) {
            break;
        }

        char *token_queue = command;
        const char *const operation = TOKEN(token_queue);

        if (is_prefix(operation, "insert", 1)) {
            const char *const table = TOKEN(token_queue);
            const db_id_t id = DB_ID_TOKEN(token_queue);

            if (is_prefix(table, "publisher", 1)) {
                publisher_t publisher = {
                    .meta.id = id
                };

                DB_STR_TOKEN(publisher.name, token_queue);
                DB_STR_TOKEN(publisher.country, token_queue);

                if (!master_insert(db, &publisher.meta)) {
                    EXISTS("publisher");
                }
            } else {
                NO(table);
            }
        } else if (is_prefix(operation, "get", 1)) {
            const char *const table = TOKEN(token_queue);
            const db_id_t id = DB_ID_TOKEN(token_queue);

            if (is_prefix(table, "publisher", 1)) {
                publisher_t publisher = {
                    .meta.id = id
                };

                if (master_get(db, &publisher.meta)) {
                    printf("name: %s\ncountry: %s\n", publisher.name, publisher.country);
                } else {
                    NO_TYPE("publisher");
                }
            } else {
                NO(table);
            }
        } else if (is_prefix(operation, "update", 1)) {
            const char *const table = TOKEN(token_queue);
            const db_id_t id = DB_ID_TOKEN(token_queue);

            if (is_prefix(table, "publisher", 1)) {
                publisher_t publisher = {
                    .meta.id = id
                };

                if (!master_get(db, &publisher.meta)) {
                    NO_TYPE("publisher");
                } else {
                    const char *field = TOKEN(token_queue);
                    int success = 1;

                    while (*field) {
                        if (is_prefix(field, "name", 1)) {
                            DB_STR_TOKEN(publisher.name, token_queue);
                        } else if (is_prefix(field, "country", 1)) {
                            DB_STR_TOKEN(publisher.country, token_queue);
                        } else {
                            NO(field);
                            success = 0;
                            break;
                        }

                        field = TOKEN(token_queue);
                    }

                    if (success) {
                        master_update(db, &publisher.meta);
                    }
                }
            } else {
                NO(table);
            }
        } else if (is_prefix(operation, "delete", 1)) {
            const char *table = TOKEN(token_queue);
            const db_id_t id = DB_ID_TOKEN(token_queue);

            if (is_prefix(table, "publisher", 1)) {
                if (!master_delete(db, id)) {
                    NO_TYPE("publisher");
                }
            } else {
                NO(table);
            }
        } else if (is_prefix(operation, "count", 1)) {
            const char *table = TOKEN(token_queue);

            if (is_prefix(table, "publishers", 1)) {
                printf("count: %ld\n", master_count(db));
            } else {
                NO(table);
            }
        } else {
            NO(operation);
        }

        free(command);
    }
}
