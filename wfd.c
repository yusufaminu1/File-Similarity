#include "wfd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

WFD *wfd_create(const char *filename) {
    WFD *wfd = malloc(sizeof(WFD));
    if (!wfd) { perror("malloc"); exit(EXIT_FAILURE); }
    wfd->filename = strdup(filename);
    if (!wfd->filename) { perror("strdup"); exit(EXIT_FAILURE); }
    wfd->head = NULL;
    wfd->total_words = 0;
    return wfd;
}

void wfd_add_word(WFD *wfd, const char *word) {
    wfd->total_words++;
    WordNode *temp = wfd->head;
    WordNode *prev = NULL;

    while(temp!=NULL){
        if(strcmp(temp->word,word)<0){
            prev = temp;
            temp = temp->next;
        }else if(strcmp(temp->word,word)==0){
            temp->count++;
            return;
        }else if(strcmp(temp->word,word)>0){
            break;
        }
        
        
    }
   WordNode *wfdnew = malloc(sizeof(WordNode));
    if (!wfdnew) { perror("malloc"); exit(EXIT_FAILURE); }
    wfdnew->word  = strdup(word);
    if (!wfdnew->word) { perror("strdup"); exit(EXIT_FAILURE); }
    wfdnew->count = 1;
    wfdnew->freq  = 0.0;
    if (prev == NULL) {
        wfd->head = wfdnew;
    }else{
        prev->next = wfdnew;
    }
    wfdnew->next = temp;

    
}

void wfd_finalize(WFD *wfd) {
    WordNode *temp = wfd->head;
    if(wfd->total_words == 0){
        return;
    }
    while(temp!=NULL){
        temp->freq = (double)temp->count/wfd->total_words;
        temp = temp->next;
    }
}

void wfd_free(WFD *wfd) {
    WordNode *curr = wfd->head;
    
    while(curr!=NULL){
        WordNode *next = curr->next;
        free(curr->word);
        free(curr);
        curr = next;
    }
    free(wfd->filename);
    free(wfd);
}

double compute_jsd(const WFD *a, const WFD *b) {
    /* TODO:
     * Use simultaneous iteration over both sorted lists to compute JSD.
     *
     * Keep running totals kld1 and kld2.
     * At each step, compare the current word in each list:
     *   - word appears in both:  f1 = a->freq, f2 = b->freq, advance both
     *   - word only in a:        f1 = a->freq, f2 = 0,       advance a
     *   - word only in b:        f1 = 0,       f2 = b->freq, advance b
     *
     * Then:
     *   mean = 0.5 * (f1 + f2)
     *   if f1 > 0: kld1 += f1 * log2(f1 / mean)
     *   if f2 > 0: kld2 += f2 * log2(f2 / mean)
     *
     * Finally return sqrt(0.5 * kld1 + 0.5 * kld2)
     */
    double kld1 = 0.0;
    double kld2 = 0.0;
    WordNode *nodeA = a->head;
    WordNode *nodeB = b->head;

    while(nodeA != NULL || nodeB != NULL) {
      double f1 = 0.0;
      double f2 = 0.0;
      int compare;
      if (nodeA == NULL) { compare = 1; }
      else if (nodeB == NULL) { compare = -1; }
      else { compare = strcmp(nodeA->word, nodeB->word); }

      if(compare < 0) {
        f1 = nodeA->freq;
        nodeA = nodeA->next;
      } else if (compare > 0) {
        f2 = nodeB->freq;
        nodeB = nodeB->next;
      } else {
        f1 = nodeA->freq;
        f2 = nodeB->freq;
        nodeA = nodeA->next;
        nodeB = nodeB->next;
      }

      double mean = 0.5 * (f1 + f2);
      if(f1 > 0) { kld1 += f1 * log2(f1 / mean); }
      if(f2 > 0) { kld2 += f2 * log2(f2 / mean); }
    }

    return sqrt(0.5 * kld1 + 0.5 * kld2);
}
