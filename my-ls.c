#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

#define BLUE "\x1b[34m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

bool SHOW_ALL = false;
bool RECURSIVE = false;

void print_dir_rec(char *);
void print_dir(char *);
bool contains(char *, char);

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        print_dir(".");
        return 0;
    }
    else
    {
        int directory_end_index = argv[argc - 1][0] == '-' ? argc - 1 : argc;

        if (directory_end_index != argc)
        {
            if (contains(argv[argc - 1], 'a'))
            {
                SHOW_ALL = true;
            }
            if (contains(argv[argc - 1], 'R'))
            {
                RECURSIVE = true;
            }
        }

        for (int i = 1; i < directory_end_index; i++)
        {
            printf("directory : %s\n", argv[i]);
            if (RECURSIVE)
            {
                print_dir_rec(argv[i]);
            }
            else
            {
                print_dir(argv[i]);
            }
            printf("\n\n");
        }
        return 0;
    }
}

bool contains(char *str, char c)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] == c)
            return true;
    }
    return false;
}

void print_dir(char *dir_name)
{
    DIR *dir = opendir(dir_name);
    if (dir == NULL)
    {
        if (errno != 0)
        {
            int saved_errno = errno;
            fprintf(stdout, "%s\n", strerror(saved_errno));
        }
        return;
    }
    int size = 0;
    struct dirent **entries = NULL;
    while (dir)
    {
        struct dirent *entry = readdir(dir);
        if (entry == NULL)
        {
            if (errno != 0)
            {
                int saved_errno = errno;
                fprintf(stdout, "%s\n", strerror(saved_errno));
            }
            closedir(dir);
            goto print;
        }
        entries = realloc(entries, ++size * sizeof(struct dirent *));
        entries[size - 1] = entry;
    }
    closedir(dir);
    return;

print:
    for (int i = 0; i < size; i++)
    {
        if (entries[i]->d_type == 4)
        { // 4 means directory, check dirent.h for other file types
            if (entries[i]->d_name[0] == '.' && !SHOW_ALL)
            {
                continue;
            }
            printf(BLUE);
        }
        printf("\t%s%s\n", entries[i]->d_name, RESET);
    }
}

void print_dir_rec(char *dir_name)
{
    DIR *dir = opendir(dir_name);
    if (dir == NULL)
    {
        if (errno != 0)
        {
            int saved_errno = errno;
            fprintf(stdout, "%s\n", strerror(saved_errno));
        }
        return;
    }
    int size = 0;
    struct dirent **entries = NULL;
    while (dir)
    {
        struct dirent *entry = readdir(dir);
        if (entry == NULL)
        {
            if (errno != 0)
            {
                int saved_errno = errno;
                fprintf(stdout, "%s\n", strerror(saved_errno));
            }
            closedir(dir);
            goto print;
        }
        entries = realloc(entries, ++size * sizeof(struct dirent *));
        entries[size - 1] = entry;
    }
    closedir(dir);
    return;

print:
    struct dirent **directories = NULL;
    int dir_size = 0;
    for (int i = 0; i < size; i++)
    {
        if (entries[i]->d_type == 4)
        { // 4 means directory, check dirent.h for other file types
            if (!(strlen(entries[i]->d_name) == 1 && entries[i]->d_name[0] == '.' && entries[i]->d_name[1] == 0) && !(strlen(entries[i]->d_name) == 2 && entries[i]->d_name[0] == '.' && entries[i]->d_name[1] == '.' && entries[i]->d_name[2] == 0))
            {
                directories = realloc(directories, ++dir_size * sizeof(struct dirent *));
                directories[dir_size - 1] = entries[i];
            }
            if (entries[i]->d_name[0] == '.' && !SHOW_ALL)
            {
                continue;
            }
            printf(BLUE);
        }
        printf("\t%s%s\n", entries[i]->d_name, RESET);
    }
    free(entries);
    if (dir_size == 0)
    {
        return;
    }
    for (int i = 0; i < dir_size; i++)
    {
        char *new_dir_name = malloc(strlen(dir_name) + strlen(directories[i]->d_name) + 2);
        new_dir_name[0] = '\0';
        strcat(new_dir_name, dir_name);
        strcat(new_dir_name, "/");
        strcat(new_dir_name, directories[i]->d_name);
        printf("directory %s\n", new_dir_name);

        print_dir_rec(new_dir_name);
    }
}
