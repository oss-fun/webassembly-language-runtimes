#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// WebAssembly NOP instruction macro
#if defined(__wasm__) || defined(__wasm32__)
    #define WASM_NOP() __asm__ volatile ("nop")
#else
    // Fallback: CPU-specific nop for native compilation
    #if defined(__x86_64__) || defined(__i386__)
        #define WASM_NOP() __asm__ volatile ("nop" ::: "memory")
    #elif defined(__aarch64__) || defined(__arm__)
        #define WASM_NOP() __asm__ volatile ("nop" ::: "memory")
    #else
        // Generic fallback - volatile operation to prevent optimization
        #define WASM_NOP() do { volatile int dummy = 0; (void)dummy; } while(0)
    #endif
#endif

static char *USAGE_FMT =
"Usage: %s DB_FILE\n"
"   DB_FILE always gets overwritten with a database with basic 'Sample' table.\n"
"\n"
"Program exits on any system error!\n"
"\n";

int insert_sample_data(char *db_file);
int read_sample_data(char *db_file);
int insert(int key, int val, sqlite3 *db);
int select(int key, sqlite3 *db);

int file_exists(char *file_path)
{
  struct stat buff;
  return (stat(file_path, &buff) == 0);
}

void print_progress_bar(int current, int total) {
    int bar_width = 40;  // 進捗バーの横幅
    float progress = (float)current / total;
    int pos = (int)(bar_width * progress);

    printf("\r[");
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) {
            // 緑色で表示
            printf("\x1b[32m#\x1b[0m");
        } else {
            printf("-");
        }
    }
    printf("] %3d%% (%d/%d)", (int)(progress * 100), current, total);
    fflush(stdout);

    if (current == total) {
        printf("\n");
    }
}

int write_db(int total, sqlite3 *db) {
    int halfway = total / 2;
    
    for (int i = 0; i < total; i++) {
        // Check if we've reached the halfway point
        if (i == halfway) {
            // Insert WebAssembly nop instructions
            WASM_NOP();
        }
        
        if (insert(i, i*i, db) != 0) {
            fprintf(stderr, "Insert error at %d\n", i);
            break;
        }

        int dividee = 1;
        if (total/100 > 0) dividee = total / 100;
        if (i % dividee == 0) {
          print_progress_bar(i, total);
          // printf("Progress: inserted %d records...\n", i);
        }
    }
    printf("\n");
    fflush(stdout);
}

void print_intro() {
    printf("\x1b[36m"); // 青緑色
    printf("========================================\n");
    printf("         🧪 SQLite Sample CLI           \n");
    printf("========================================\n");
    printf("\x1b[0m");

    printf("This tool demonstrates simple SQLite usage:\n");
    printf("  - Bulk insertion with predictable key-value pairs\n");
    printf("  - Fast lookup of inserted data\n\n");

    printf("\x1b[4mModes:\x1b[0m\n");
    printf("  \x1b[32m[0] Write Mode\x1b[0m\n");
    printf("      Inserts entries of the form: (key, value) = (i, i * i)\n");
    printf("      → You specify how many entries (N) to insert.\n");
    printf("      → Inserts keys from 0 to N - 1.\n\n");

    printf("  \x1b[34m[1] Read Mode\x1b[0m\n");
    printf("      Input a key → returns corresponding value (if found).\n\n");

    printf("  \x1b[31m[2] Exit\x1b[0m\n");
    printf("      Cleanly terminates the program.\n");

    printf("----------------------------------------------------\n");
    printf("\x1b[3mTip:\x1b[0m Use this to test SQLite performance or simulate app state.\n");
    printf("----------------------------------------------------\n\n");
}

int main(int argc, char **argv)
{

  // Open the database in memory.
  sqlite3 *db;
  if (sqlite3_open(":memory:", &db) != SQLITE_OK)
  {
    fprintf(stderr, "[ERROR] %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return -1;
  }

  // Initialize the table.
  char *err_msg = NULL;
  int err = sqlite3_exec(db, 
      "CREATE TABLE IF NOT EXISTS Sample"
      "(key TEXT, value TEXT);",
      NULL, NULL, &err_msg);
  if (err != SQLITE_OK) {
    printf("%s\n", err_msg);
    return -1;
  }
  /* printf("CREATE TABLE!\n"); */
  print_intro();

  while (1) {
    // set: 0, get: 1, migration: 2, exit: other
    printf("\x1b[32m[+] Input 0(write) or 1(read) or 2(exit)\n\x1b[m");

    int command = 0;
    char input_str[100];
    scanf("%s", input_str);

    int invalid_input = 0;
    // convert the received value type from string to int.
    for (int i = 0; i < strlen(input_str); i++) {
      char c = input_str[i];
      if ('0' <= c && c <= '9') {
        command = 10 * command + (int)(c - '0');
      }
      else {
        printf("入力がおかしいです\n");
        invalid_input = 1;
        break;
      }
    }
    // continue when input is invalid.
    if (invalid_input) {
      continue;
    }

    switch (command) {
      int key, val, sleep_seccond;
      int entries;
      case 0:
        printf("\x1b[32m[+] SET MODE: Please input 'number of entries'\n\x1b[m");
        scanf("%d", &entries);
        write_db(entries, db);
        break;
      case 1:
        printf("\x1b[32m[+] GET MODE: Please input 'key'\n\x1b[m");
        scanf("%d", &key);
        // Select
        if (select(key, db) != 0) {
          printf("\x1b[31m");
          printf("[ERROR] failed to select\n");
          printf("\x1b[m");

          sqlite3_close(db);
          return -1;
        }
        break;
      case 2:
        printf("Exit\n");
        sqlite3_close(db);
        return 0;
      default:
        continue;
    }
  }

  return 0;
}

int Error(int rc, char* msg) {
    printf("\x1b[31m");
    printf("[ERROR] %s\n", msg);
    printf("[ERROR] errmsg is %s\n", sqlite3_errstr(rc));
    printf("\x1b[m");
    return -1;
}

int insert(int key, int val, sqlite3 *db) {
  char *sql_command = 
    "INSERT INTO Sample (key, value) VALUES (?, ?);";
  sqlite3_stmt* pStmt;

  // prepare
  int status = sqlite3_prepare_v2(db, sql_command, -1, &pStmt, NULL);
  if (status != SQLITE_OK) {
    return Error(status, "failed to sqlite3_prepare_v2.");
  }

  // bind the values to the prepared statement
  sqlite3_bind_int(pStmt, 1, key);
  sqlite3_bind_int(pStmt, 2, val);

  // execute the SQL statement
  do {
    status = sqlite3_step(pStmt);
  } while(status == SQLITE_BUSY);
  
  if (status != SQLITE_DONE) {
    return Error(status, "failed to execute sql statement");
  }

  // finalize
  sqlite3_reset(pStmt);
  sqlite3_clear_bindings(pStmt);
  sqlite3_finalize(pStmt);

  return 0;
}

int select(int key, sqlite3 *db) {
  char *sql_command = 
    "SELECT value FROM Sample WHERE key = ?";
  sqlite3_stmt* pStmt;

  // prepare
  int status = sqlite3_prepare_v2(db, sql_command, -1, &pStmt, NULL);
  if (status != SQLITE_OK) {
    return Error(status, "failed to sqlite3_prepare_v2.");
  }

  // bind the values to the prepared statement
  sqlite3_bind_int(pStmt, 1, key);

  // execute the SQL statement
  int count = 0;
  do {
    status = sqlite3_step(pStmt);
    if (status != SQLITE_ROW) break;

    int value = sqlite3_column_int(pStmt, 0);
    printf("{key, value} = {%d, %d}\n", key, value);
    count++;
  } while(status == SQLITE_ROW);

  // the given key is not found.
  if (count == 0) {
    printf("The given key is not found.\n");
  }

  if (status != SQLITE_DONE) {
    return Error(status, "failed to execute sql statement");
  }

  // finalize
  sqlite3_reset(pStmt);
  sqlite3_clear_bindings(pStmt);
  sqlite3_finalize(pStmt);

  return 0;
}
