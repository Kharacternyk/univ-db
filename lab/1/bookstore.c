/* clang-format off */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <readline/readline.h>
/* clang-format on */

#include "db.h"

#define TOKEN(queue) (strsep(&queue, " \t") ?: "")
#define DB_ID_TOKEN(queue) strtol(TOKEN(queue), NULL, 0)
#define DB_INT_TOKEN(dest, queue) dest = strtol(TOKEN(queue), NULL, 0)
#define DB_STR_TOKEN(dest, queue) strncpy(dest, TOKEN(queue), DB_STR_LEN)
#define DB_STR_LEN 128

typedef char db_str_t[DB_STR_LEN];
typedef int32_t db_int_t;

typedef struct {
    db_str_t name;
    db_str_t country;
} publisher_t;

typedef struct {
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
        const char *operation = TOKEN(token_queue);

        if (is_prefix(operation, "insert", 1)) {
            const char *table = TOKEN(token_queue);

            if (is_prefix(table, "publisher", 1)) {
                const db_id_t id = DB_ID_TOKEN(token_queue);
                publisher_t publisher = {};

                DB_STR_TOKEN(publisher.name, token_queue);
                DB_STR_TOKEN(publisher.country, token_queue);

                master_insert(db, id, &publisher);

                goto cleanup;
            }
        }

        if (is_prefix(operation, "get", 1)) {
            const char *table = TOKEN(token_queue);

            if (is_prefix(table, "publisher", 1)) {
                const db_id_t id = DB_ID_TOKEN(token_queue);
                publisher_t publisher;

                if (master_get(db, id, &publisher)) {
                    printf("name: %s\ncountry: %s\n", publisher.name, publisher.country);
                } else {
                    printf("error: no such publisher\n");
                }

                goto cleanup;
            }
        }

        printf("error: invalid command\n");

        cleanup: free(command);
    }
}
