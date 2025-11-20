/* Definitions of various structures for confusions. */

#ifndef CONFUSION_H
#define CONFUSION_H
struct confusionListType
{ /* linked list of confusion strings */
    unsigned int Confusion;        /* The confusion text */
    unsigned int Confusion_type;   /* The type of text for each symbol e.g. symbol, model, function, range */
    float Codelength;              /* Its codelength */
    struct confusionListType *Cnext;
};

struct confusionTrieType
{ /* node in the trie */
  unsigned int Ctype;              /* The current symbol's type */
  unsigned int Csymbol;            /* The current symbol */
  unsigned int Context;            /* The whole context if this is a terminal node */
  unsigned int Context_type;       /* The context type if this is a terminal node */
  struct confusionListType *Confusions;
  /* the list of confusions which the context often gets confused with */
  struct confusionTrieType *Cnext; /* the next link in the list */
  struct confusionTrieType *Cdown; /* the next level down in the trie */
};

extern int Confusion_Malloc;

void
dumpConfusions (unsigned int file, struct confusionTrieType *confusions);

void
loadConfusions (unsigned int file, unsigned int transform_model);
/* Loads the confusions from the file. */

void
releaseConfusions (struct confusionTrieType *confusions);

struct confusionTrieType *
createConfusion (void);

void
addConfusion (struct confusionTrieType *confusions, float codelength,
	      unsigned int context, unsigned int context_type,
	      unsigned int confusion, unsigned int confusion_type);

struct confusionTrieType *
findConfusionSymbol (struct confusionTrieType *head, unsigned int csymbol, unsigned int ctype, boolean *found);

struct confusionListType *
findConfusions (struct confusionTrieType *confusions, unsigned int context, unsigned int context_type);

boolean
matchConfusionSymbol (unsigned int model, unsigned int source_symbol,
		      unsigned int previous_symbol, 
		      unsigned int source_text, unsigned int source_pos,
		      unsigned int confusion_symbol, unsigned int confusion_type);
/* Returns TRUE if the symbol matches the confusion symbol and type. */

#endif
