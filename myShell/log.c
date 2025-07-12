#include "log.h"
#include <stdlib.h>
#include <string.h>

void log_init(log_t *l) {
    l->head = NULL;
}

void log_destroy(log_t* l) {
    node* cur = l->head;
    while(cur != NULL) {
        node* tmp = cur;
        cur = cur->next;
        free(tmp->cmd);
        free(tmp);
    }
    l->head = NULL;
}

void log_push(log_t* l, const char *item) {
    node* newnode = (node*)malloc(sizeof(node));
    if (!newnode) exit(1);
    newnode->cmd = (char*)malloc(strlen(item) + 1);
    if (!newnode->cmd) exit(1);
    strcpy(newnode->cmd, item);
    newnode->next = l->head;
    l->head = newnode;
}

char *log_search(log_t* l, const char *prefix) {
    node* cur = l->head;
    size_t len = strlen(prefix);
    while(cur != NULL) {
        if(len && strncmp(cur->cmd, prefix, len) == 0) {
            return cur->cmd;
        }
        cur = cur->next;
    }
    return NULL;
}