#ifndef WFD_H
#define WFD_H

/* A single node in the sorted linked list of words */
typedef struct WordNode {
    char *word;            /* dynamically allocated word string */
    int count;             /* number of occurrences */
    double freq;           /* frequency = count / total_words (set by wfd_finalize) */
    struct WordNode *next;
} WordNode;

/* Word Frequency Distribution for one file */
typedef struct {
    char *filename;        /* path to the file */
    WordNode *head;        /* head of sorted linked list */
    int total_words;       /* total word count */
} WFD;

/* Create an empty WFD for the given filename */
WFD *wfd_create(const char *filename);

/* Insert a word into the WFD (maintains sorted order, increments count if exists) */
void wfd_add_word(WFD *wfd, const char *word);

/* Convert all counts to frequencies once the file has been fully read */
void wfd_finalize(WFD *wfd);

/* Free all memory associated with a WFD */
void wfd_free(WFD *wfd);

/* Compute the Jensen-Shannon Distance between two WFDs */
double compute_jsd(const WFD *a, const WFD *b);

#endif
