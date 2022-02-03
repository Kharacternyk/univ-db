/* clang-format off */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <readline/readline.h>

#include "db.h"
#include "index.h"
/* clang-format on */

int main() {
    const table_t publisher_table = {
        .data = fopen("publisher.data", "w+"),
        .index = fopen("publisher.index", "w+"),
        .pool = fopen("publisher.pool", "w+"),
    };

    for (;;) {
        char *command = readline("db> ");

        if (!command) {
            break;
        }

        char *token_queue = command;
        const char *operation = TOKEN(token_queue);

        if (strcmp(operation, "insert") == 0) {
            const char *table = TOKEN(token_queue);

            if (strcmp(table, "publisher") == 0) {
                publisher_t publisher = {};

                DB_ID_TOKEN(publisher.id, token_queue);
                DB_STR_TOKEN(publisher.name, token_queue);
                DB_STR_TOKEN(publisher.country, token_queue);

                fseek(publisher_table.data, 0, SEEK_END);
                add_offset(publisher_table.index, publisher.id,
                        ftell(publisher_table.data));
                FWRITE(publisher, publisher_table.data);

                goto cleanup;
            }
        }

        if (strcmp(operation, "get") == 0) {
            const char *table = TOKEN(token_queue);

            if (strcmp(table, "publisher") == 0) {
                publisher_t publisher;
                db_id_t id;
                DB_ID_TOKEN(id, token_queue);

                const long offset = get_offset(publisher_table.index, id);
                fseek(publisher_table.data, offset, SEEK_SET);
                FREAD(publisher, publisher_table.data);

                printf("%s %s\n", publisher.name, publisher.country);

                goto cleanup;
            }
        }

        printf("Invalid command\n");

        cleanup: free(command);
    }
}
