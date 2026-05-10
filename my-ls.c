#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define COLOR_BLUE  "\x1b[34m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"

typedef struct {
    bool show_all;
    bool recursive;
} Options;

/* Forward declarations */
static void list_dir(const char *path, const Options *opts, int depth);
static int  compare_entries(const void *a, const void *b);
static char *join_path(const char *dir, const char *name);
static void  parse_flags(const char *arg, Options *opts);
static void  usage(const char *progname);

/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
    Options opts = { .show_all = false, .recursive = false };

    /* Collect paths and parse flags (flags may appear anywhere). */
    const char **paths  = malloc(argc * sizeof(char *));
    int          npath  = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == '\0') {
                fprintf(stderr, "Unknown flag: -\n");
                usage(argv[0]);
                free(paths);
                return 1;
            }
            parse_flags(argv[i] + 1, &opts);
        } else {
            paths[npath++] = argv[i];
        }
    }

    if (npath == 0) {
        list_dir(".", &opts, 0);
    } else {
        for (int i = 0; i < npath; i++) {
            if (npath > 1)
                printf("%s:\n", paths[i]);
            list_dir(paths[i], &opts, 0);
            if (npath > 1 && i + 1 < npath)
                putchar('\n');
        }
    }

    free(paths);
    return 0;
}

/* ------------------------------------------------------------------ */

static void parse_flags(const char *flags, Options *opts)
{
    for (; *flags; flags++) {
        switch (*flags) {
            case 'a': opts->show_all  = true; break;
            case 'R': opts->recursive = true; break;
            default:
                fprintf(stderr, "Unknown flag: -%c\n", *flags);
                break;
        }
    }
}

static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s [-aR] [directory ...]\n", progname);
}

/* ------------------------------------------------------------------ */

/*
 * Collect all entries from `path`, sort them, print them, then
 * recurse into subdirectories if requested.
 */
static void list_dir(const char *path, const Options *opts, int depth)
{
    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        return;
    }

    /* --- collect entries ------------------------------------------ */
    struct dirent **entries = NULL;
    int             count   = 0;

    errno = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Copy the dirent so it remains valid after closedir(). */
        struct dirent *copy = malloc(sizeof(struct dirent));
        if (!copy) { perror("malloc"); break; }
        *copy = *entry;

        struct dirent **tmp = realloc(entries, (count + 1) * sizeof(*entries));
        if (!tmp) { perror("realloc"); free(copy); break; }
        entries = tmp;
        entries[count++] = copy;
        errno = 0;
    }
    if (errno != 0)
        fprintf(stderr, "%s: %s\n", path, strerror(errno));

    closedir(dir);

    /* --- sort ------------------------------------------------------ */
    qsort(entries, count, sizeof(*entries), compare_entries);

    /* --- print & collect subdirs ----------------------------------- */
    const char **subdirs = NULL;
    int          nsub    = 0;

    for (int i = 0; i < count; i++) {
        const char *name   = entries[i]->d_name;
        bool        hidden = (name[0] == '.');

        if (!hidden || opts->show_all) {
            /* Determine type with stat() so we aren't relying on d_type
             * being set (some filesystems leave it as DT_UNKNOWN).       */
            char *full = join_path(path, name);
            struct stat st;
            bool is_dir = (full && stat(full, &st) == 0 && S_ISDIR(st.st_mode));

            if (is_dir) {
                printf(COLOR_BLUE "\t%s" COLOR_RESET "\n", name);

                /* Queue non-. non-.. subdirs for recursion. */
                if (opts->recursive &&
                    !(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)) {
                    char **tmp = realloc(subdirs, (nsub + 1) * sizeof(char *));
                    if (tmp) {
                        subdirs = (const char **)tmp;
                        subdirs[nsub++] = full;
                        full = NULL; /* ownership transferred */
                    }
                }
            } else {
                printf("\t%s\n", name);
            }

            free(full);
        }

        free(entries[i]);
    }
    free(entries);

    /* --- recurse --------------------------------------------------- */
    for (int i = 0; i < nsub; i++) {
        printf("\n%s:\n", subdirs[i]);
        list_dir(subdirs[i], opts, depth + 1);
        free((void *)subdirs[i]);
    }
    free(subdirs);
}

/* ------------------------------------------------------------------ */

static int compare_entries(const void *a, const void *b)
{
    const struct dirent *da = *(const struct dirent **)a;
    const struct dirent *db = *(const struct dirent **)b;
    /* Case-insensitive sort, with hidden files sorted after visible ones
     * (consistent with many ls implementations when -a is active).    */
    const char *na = da->d_name + (da->d_name[0] == '.' ? 1 : 0);
    const char *nb = db->d_name + (db->d_name[0] == '.' ? 1 : 0);
    return strcasecmp(na, nb);
}

/* Returns a heap-allocated "dir/name" string; caller must free(). */
static char *join_path(const char *dir, const char *name)
{
    size_t len = strlen(dir) + 1 + strlen(name) + 1;
    char  *buf = malloc(len);
    if (!buf) { perror("malloc"); return NULL; }
    snprintf(buf, len, "%s/%s", dir, name);
    return buf;
}
