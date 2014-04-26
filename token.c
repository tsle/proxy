#include "token.h"
#include <stdio.h>

void addToken(struct token** head, struct token* t) {
  struct token* curr = *head;
  t->next = NULL;
  if(head == NULL) {
    head = &t;
  } else {
    while(curr->next != NULL)
      curr = curr->next;
    curr->next = t;
  }
}

void freeTokenList(struct token** head) {
  struct token* curr = *head;
  freeTokens(curr);
  free(head);
}

void freeTokens(struct token* t) {
  if(t == NULL)
    return;
  freeTokens(t->next);
  freeToken(t);
}

void freeToken(struct token* t) {
  if (t == NULL) return;
  if (t->text != NULL)
    free(t->text);
  free(t);
}
