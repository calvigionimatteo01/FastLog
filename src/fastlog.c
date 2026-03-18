#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 2048
#define HASH_SIZE 10007
#define DEFAULT_TOP_K 10
#define MAX_TOP_K 100
#define IP_STR_LEN 64
#define INITIAL_ENTRY_CAPACITY 128

typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR,
    STATUS_BAD_PARAM
} status_t;

typedef struct Node {
    char ip[IP_STR_LEN];
    int count;
    struct Node *next;
} node_t;

typedef struct Entry {
    char ip[IP_STR_LEN];
    int count;
} entry_t;

typedef struct CliConfig {
    const char *file_name;
    const char *filter_str;
    int top_k;
    int json_output;
    int debug_enabled;
} cli_config_t;

/* globale per semplicità, non passo puntatori ovunque */
static node_t *g_table[HASH_SIZE];

static void init_table(void);
static void free_table(void);
static unsigned int hash_ip(const char *str);
static status_t update_ip_counter(const char *ip);
static int line_matches_filter(const char *line, const char *filter_str);
static status_t extract_ip_from_line(const char *line, char *ip_buf, size_t ip_buf_size);
static status_t process_file(const cli_config_t *cli_cfg);
static status_t collect_entries(entry_t **entries_arr, int *entries_num);
static int cmp_entry_desc(const void *a, const void *b);
static void print_usage(const char *prog_name);
static status_t parse_cli_args(int argc, char *argv[], cli_config_t *cli_cfg);
static void print_report(const entry_t *entries_arr, int entries_num, int top_k, int json_output);
static void print_error(const char *msg);
static void debug_print_config(const cli_config_t *cli_cfg);

static void init_table(void) {
    memset(g_table, 0, sizeof(g_table));
}

static void free_table(void) {
    for (int i = 0; i < HASH_SIZE; i++) {
        node_t *curr_node = g_table[i];

        while (curr_node != NULL) {
            node_t *next_node = curr_node->next;
            free(curr_node);
            curr_node = next_node;
        }

        g_table[i] = NULL;
    }
}

static unsigned int hash_ip(const char *str) {
    unsigned int h = 0U;

    while (*str != '\0') {
        h = (h * 31U) + (unsigned char)*str;
        str++;
    }

    return h % HASH_SIZE;
}

static status_t update_ip_counter(const char *ip) {
    unsigned int idx = hash_ip(ip);
    node_t *curr_node = g_table[idx];

    if (ip == NULL) {
        return STATUS_BAD_PARAM;
    }

    while (curr_node != NULL) {
        if (strcmp(curr_node->ip, ip) == 0) {
            curr_node->count++;
            return STATUS_OK;
        }

        curr_node = curr_node->next;
    }

    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (new_node == NULL) {
        return STATUS_ERROR;
    }

    snprintf(new_node->ip, sizeof(new_node->ip), "%s", ip);
    new_node->count = 1;
    new_node->next = g_table[idx];
    g_table[idx] = new_node;

    return STATUS_OK;
}

static int line_matches_filter(const char *line, const char *filter_str) {
    if (line == NULL) {
        return 0;
    }

    if (filter_str == NULL) {
        return 1;
    }

    return (strstr(line, filter_str) != NULL);
}

static status_t extract_ip_from_line(const char *line, char *ip_buf, size_t ip_buf_size) {
    if (line == NULL || ip_buf == NULL || ip_buf_size == 0U) {
        return STATUS_BAD_PARAM;
    }

    if (sscanf(line, "%63s", ip_buf) != 1) {
        return STATUS_ERROR;
    }

    ip_buf[ip_buf_size - 1U] = '\0';

    return STATUS_OK;
}

static status_t process_file(const cli_config_t *cli_cfg) {
    FILE *fp = NULL;
    char line[MAX_LINE_LEN];
    char ip[IP_STR_LEN];

    if (cli_cfg == NULL || cli_cfg->file_name == NULL) {
        return STATUS_BAD_PARAM;
    }

    fp = fopen(cli_cfg->file_name, "r");
    if (fp == NULL) {
        return STATUS_ERROR;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {

        if (!line_matches_filter(line, cli_cfg->filter_str)) {
            continue;
        }

        if (extract_ip_from_line(line, ip, sizeof(ip)) != STATUS_OK) {
            continue;
        }

        if (update_ip_counter(ip) != STATUS_OK) {
            fclose(fp);
            return STATUS_ERROR;
        }
    }

    fclose(fp);
    return STATUS_OK;
}

static status_t collect_entries(entry_t **entries_arr, int *entries_num) {
    int capacity = INITIAL_ENTRY_CAPACITY;
    int size = 0;
    entry_t *local_arr = NULL;

    if (entries_arr == NULL || entries_num == NULL) {
        return STATUS_BAD_PARAM;
    }

    local_arr = (entry_t *)malloc((size_t)capacity * sizeof(entry_t));
    if (local_arr == NULL) {
        return STATUS_ERROR;
    }

    for (int i = 0; i < HASH_SIZE; i++) {
        node_t *curr_node = g_table[i];

        while (curr_node != NULL) {
            if (size >= capacity) {
                int new_capacity = capacity * 2;

                entry_t *tmp_arr = (entry_t *)realloc(local_arr, (size_t)new_capacity * sizeof(entry_t));
                if (tmp_arr == NULL) {
                    free(local_arr);
                    return STATUS_ERROR;
                }

                local_arr = tmp_arr;
                capacity = new_capacity;
            }

            snprintf(local_arr[size].ip, sizeof(local_arr[size].ip), "%s", curr_node->ip);
            local_arr[size].count = curr_node->count;
            size++;

            curr_node = curr_node->next;
        }
    }

    *entries_arr = local_arr;
    *entries_num = size;

    return STATUS_OK;
}

static int cmp_entry_desc(const void *a, const void *b) {
    const entry_t *ea = (const entry_t *)a;
    const entry_t *eb = (const entry_t *)b;

    if (eb->count > ea->count) {
        return 1;
    }

    if (eb->count < ea->count) {
        return -1;
    }

    return strcmp(ea->ip, eb->ip);
}

static void print_usage(const char *prog_name) {
    printf("Usage:\n");
    printf("  %s <file> [--filter STR] [--top N] [--json] [--debug]\n", prog_name);
}

static void print_error(const char *msg) {
    if (msg != NULL) {
        fprintf(stderr, "Error: %s\n", msg);
    }
}

static void debug_print_config(const cli_config_t *cli_cfg) {
    if (cli_cfg == NULL) {
        return;
    }

    printf("[DEBUG] file_name    = %s\n", cli_cfg->file_name);
    printf("[DEBUG] filter_str   = %s\n", (cli_cfg->filter_str != NULL) ? cli_cfg->filter_str : "(null)");
    printf("[DEBUG] top_k        = %d\n", cli_cfg->top_k);
    printf("[DEBUG] json_output  = %d\n", cli_cfg->json_output);
    printf("[DEBUG] debug_enable = %d\n", cli_cfg->debug_enabled);
}

static status_t parse_cli_args(int argc, char *argv[], cli_config_t *cli_cfg) {
    if (cli_cfg == NULL || argc < 2) {
        return STATUS_BAD_PARAM;
    }

    cli_cfg->file_name = argv[1];
    cli_cfg->filter_str = NULL;
    cli_cfg->top_k = DEFAULT_TOP_K;
    cli_cfg->json_output = 0;
    cli_cfg->debug_enabled = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--filter") == 0) {
            if ((i + 1) >= argc) {
                return STATUS_BAD_PARAM;
            }

            cli_cfg->filter_str = argv[++i];
        }
        else if (strcmp(argv[i], "--top") == 0) {
            if ((i + 1) >= argc) {
                return STATUS_BAD_PARAM;
            }

            cli_cfg->top_k = atoi(argv[++i]);

            if (cli_cfg->top_k <= 0 || cli_cfg->top_k > MAX_TOP_K) {
                return STATUS_BAD_PARAM;
            }
        }
        else if (strcmp(argv[i], "--json") == 0) {
            cli_cfg->json_output = 1;
        }
        else if (strcmp(argv[i], "--debug") == 0) {
            cli_cfg->debug_enabled = 1;
        }
        else {
            return STATUS_BAD_PARAM;
        }
    }

    return STATUS_OK;
}

static void print_report(const entry_t *entries_arr, int entries_num, int top_k, int json_output) {
    int limit = top_k;

    if (entries_arr == NULL || entries_num <= 0) {
        if (json_output) {
            printf("[]\n");
        }
        else {
            printf("Nessun dato disponibile.\n");
        }
        return;
    }

    if (limit > entries_num) {
        limit = entries_num;
    }

    if (json_output) {
        printf("[\n");

        for (int i = 0; i < limit; i++) {
            printf("  {\"ip\": \"%s\", \"count\": %d}%s\n",
                   entries_arr[i].ip,
                   entries_arr[i].count,
                   (i == (limit - 1)) ? "" : ",");
        }

        printf("]\n");
    }
    else {
        for (int i = 0; i < limit; i++) {
            printf("%d) %s -> %d\n", i + 1, entries_arr[i].ip, entries_arr[i].count);
        }
    }
}

int main(int argc, char *argv[]) {
    cli_config_t cli_cfg;
    entry_t *entries_arr = NULL;
    int entries_num = 0;
    status_t ret_status;

    ret_status = parse_cli_args(argc, argv, &cli_cfg);
    if (ret_status != STATUS_OK) {
        print_usage(argv[0]);
        print_error("parametri non validi");
        return 1;
    }

    if (cli_cfg.debug_enabled) {
        debug_print_config(&cli_cfg);
    }

    init_table();

    ret_status = process_file(&cli_cfg);
    if (ret_status != STATUS_OK) {
        print_error("errore durante processing file");
        free_table();
        return 1;
    }

    ret_status = collect_entries(&entries_arr, &entries_num);
    if (ret_status != STATUS_OK) {
        print_error("errore raccolta dati");
        free_table();
        return 1;
    }

    qsort(entries_arr, (size_t)entries_num, sizeof(entry_t), cmp_entry_desc);

    print_report(entries_arr, entries_num, cli_cfg.top_k, cli_cfg.json_output);

    free(entries_arr);
    free_table();

    return 0;
}
