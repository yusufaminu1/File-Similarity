#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "wfd.h"

#define SUFFIX    ".txt"
#define BUF_SIZE  4096

/* ------------------------------------------------------------------ */
/* File set: a dynamic array of WFD pointers                           */
/* ------------------------------------------------------------------ */

typedef struct {
    WFD **files;
    int count;
    int capacity;
} FileSet;

void fileset_init(FileSet *fs) {
    fs->capacity = 16;
    fs->count    = 0;
    fs->files    = malloc(fs->capacity * sizeof(WFD *));
    if (!fs->files) { perror("malloc"); exit(EXIT_FAILURE); }
}

void fileset_add(FileSet *fs, WFD *wfd) {
    if (fs->count >= fs->capacity) {
        fs->capacity *= 2;
        fs->files = realloc(fs->files, fs->capacity * sizeof(WFD *));
        if (!fs->files) { perror("realloc"); exit(EXIT_FAILURE); }
    }
    fs->files[fs->count++] = wfd;
}

void fileset_free(FileSet *fs) {
    for (int i = 0; i < fs->count; i++)
        wfd_free(fs->files[i]);
    free(fs->files);
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Returns 1 if 'name' ends with 'suffix' */
int has_suffix(const char *name, const char *suffix) {
    int lensuff =(int)strlen(suffix);
    int lenname = (int)strlen(name);
    int diff = lenname-lensuff;
    
    if(diff>=0){
        char *name2 =name+diff;
        if(strcmp(suffix,name2)==0){
            return 1;
        }
    }
    /* TODO */
    return 0;
}

/* Returns 1 if 'c' is a valid word character (letter, digit, or '-') */
int is_word_char(char c) {
    /* TODO */
    if(isalpha(c) ||isdigit(c)||c=='-'){
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Collection phase                                                    */
/* ------------------------------------------------------------------ */

/* Read a file using open/read/close, tokenize words, return a WFD.
 * Returns NULL on error (after calling perror). */
WFD *read_file(const char *path) {
    /* TODO:
     * 1. open() the file; call perror and return NULL on failure
     * 2. Create a WFD with wfd_create()
     * 3. Maintain a dynamically growing word buffer (start small, double
     *    when full) — the spec says no maximum word length
     * 4. Loop: read() a chunk into a local buffer
     *    - for each byte: if is_word_char(), append tolower() to word buffer
     *    - else if word buffer non-empty, call wfd_add_word() and reset
     * 5. After the loop flush any remaining word in the buffer
     * 6. free the word buffer, close() the fd
     * 7. Call wfd_finalize() and return the WFD
     */
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror(path); return NULL; }
    WFD *this = wfd_create(path);
    int bytes;
    char buf[BUF_SIZE];
    int word_cap = 64;
    char *word = malloc(word_cap);
    if (!word) { perror("malloc"); exit(EXIT_FAILURE); }
    int word_len = 0;
    while ((bytes = read(fd, buf, BUF_SIZE)) > 0) {
        for(int i = 0;i<bytes;i++){
            char c = buf[i];
            if(is_word_char(c)){
                if(word_len+1>=word_cap){
                    word_cap *=2;
                    word = realloc(word,word_cap);
                    if (!word) { perror("realloc"); exit(EXIT_FAILURE); }
                }
                word[word_len] = tolower(c);
                word_len++;
            }else{
                if(word_len>0){
                    word[word_len]='\0';
                    wfd_add_word(this,word);
                    word_len=0;
                }
            }
        }
    }
    if(word_len>0){
        word[word_len]='\0';
        wfd_add_word(this,word);
        word_len=0;
    }
    free(word);
    close(fd);
    wfd_finalize(this);
    return this;
}

/* Recursively traverse a directory, adding matching files to fs */
void process_directory(const char *path, FileSet *fs) {
    /* TODO:
     * 1. opendir(); call perror and return on failure
     * 2. Loop over readdir() entries:
     *    - skip entries whose d_name starts with '.'
     *    - build the full path: path + "/" + d_name
     *    - stat() the full path to determine type
     *    - if directory: recurse with process_directory()
     *    - if regular file and has_suffix(d_name, SUFFIX): call read_file()
     *      and add the result to fs (if non-NULL)
     * 3. closedir()
     */
    DIR *dir = opendir(path);
    if(!dir) { 
      perror(path);
      return;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
      if(entry->d_name[0] =='.') continue;
      char full[1024];
      snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
      struct stat st;
      if(stat(full, &st) < 0) {
        perror(full);
        continue;
      }
      
      if(S_ISDIR(st.st_mode)) {
        process_directory(full, fs);
      } else if(S_ISREG(st.st_mode) && has_suffix(entry->d_name, SUFFIX)) {
        WFD *wfd = read_file(full);
        if(wfd) fileset_add(fs, wfd);
      }
    }
    closedir(dir);
}

/* Handle one command-line argument (file or directory) */
void process_arg(const char *path, FileSet *fs) {
    /* TODO:
     * - stat() the path; call perror and return on failure
     * - if directory: call process_directory()
     * - if regular file: call read_file() and add to fs
     *   (explicit files are added regardless of suffix)
     */
    struct stat st;
    if(stat(path, &st) < 0) { 
      perror(path); 
      return; 
    }
    if(S_ISDIR(st.st_mode)) {
      process_directory(path, fs);
    } else if (S_ISREG(st.st_mode)) {
      WFD *wfd = read_file(path);
      if(wfd) {
        fileset_add(fs, wfd);
      }
    }
}

/* ------------------------------------------------------------------ */
/* Analysis phase                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *file1;
    const char *file2;
    int combined_words;
    double jsd;
} Comparison;

/* Comparator for qsort: decreasing combined word count */
int cmp_comparisons(const void *a, const void *b) {
    const Comparison *compA = (const Comparison *)a;
    const Comparison *compB = (const Comparison *)b;
    return compB->combined_words - compA->combined_words;
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file_or_dir...\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Collection phase */
    FileSet fs;
    fileset_init(&fs);
    for (int i = 1; i < argc; i++)
        process_arg(argv[i], &fs);

    if (fs.count < 2) {
        fprintf(stderr, "Error: need at least 2 files, found %d\n", fs.count);
        fileset_free(&fs);
        return EXIT_FAILURE;
    }

    /* Analysis phase */
    int n = fs.count;
    int num_pairs = n * (n - 1) / 2;

    Comparison *comps = malloc(num_pairs * sizeof(Comparison));
    if (!comps) { perror("malloc"); exit(EXIT_FAILURE); }

    /* TODO:
     * - nested loop over every (i, j) pair with j > i
     * - fill in comps[idx].file1, file2, combined_words, jsd
     */
    int idx = 0;
    for(int i = 0; i < n; i++) {
      for(int j = i + 1; j < n; j++) {
        comps[idx].file1 = fs.files[i]->filename;
        comps[idx].file2 = fs.files[j]->filename;
        comps[idx].combined_words = fs.files[i]->total_words + fs.files[j]->total_words;
        comps[idx].jsd = compute_jsd(fs.files[i], fs.files[j]);
        idx++;
      }
    }

    qsort(comps, num_pairs, sizeof(Comparison), cmp_comparisons);

    /* TODO: print each comparison as "%.5f file1 file2\n" */

    for(int i = 0; i < num_pairs; i++) {
      printf("%.5f %s %s\n", comps[i].jsd, comps[i].file1, comps[i].file2);
    }

    free(comps);
    fileset_free(&fs);
    return 0;
}
