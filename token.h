#include <stdlib.h>

struct token {
  char* text;
  struct token* next;
  int size;
};

void addToken(struct token** head, struct token* t);

void freeToken(struct token* t);

void freeTokenList(struct token** head);
void freeTokens(struct token* t);
