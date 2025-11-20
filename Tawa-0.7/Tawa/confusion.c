/* Routines for encoding the confusions. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "model.h"
#include "io.h"
#include "text.h"
#include "transform.h"
#include "confusion.h"

#define MAX_CONFUSION_LENGTH 256   /* Max. length of a confusion string */

#define CONFUSION_DELIMITER " -> " /* Delimitier used for confusion input
				      line */
#define CONFUSION_DELIMITER_LEN 4  /* Delimitier length */
#define CONFUSION_RULE_START 17    /* Start of confusion rule */

int Confusion_Malloc = 0;

struct confusionListType *
addConfusionList (struct confusionListType *head, float codelength, unsigned int confusion, unsigned int confusion_type)
/* Adds a confusion to the confusion list. */
{
    struct confusionListType *here, *newnode;

    /* Does the confusion already exist in the list? */
    here = head;
    while ((here != NULL) && TXT_compare_text (confusion, here->Confusion))
	here = here->Cnext;
    if (here != NULL)
    { /* found - replace the codelength and confusion_type */
	here->Codelength = codelength;
	here->Confusion_type = confusion_type;
    }
    else
    { /* Not found - add at the head of the list */
	Confusion_Malloc += sizeof (struct confusionListType);
        newnode = (struct confusionListType *)
	    Malloc (41, sizeof(struct confusionListType));
	newnode->Confusion = TXT_copy_text (confusion);
	newnode->Confusion_type = TXT_copy_text (confusion_type);
	newnode->Codelength = codelength;
	if (head != NULL)
	    newnode->Cnext = head;
	else
	    newnode->Cnext = NULL;
	head = newnode;
    }
    return (head);
}

void
dumpConfusionText (unsigned int file, unsigned int confusion_text,
		   unsigned int confusion_type)
/* For dumping out the confusion text and type. */
{
    unsigned int pos, symbol, type;
    unsigned int range_text, range_symbol, range_pos;

    assert (TXT_valid_file (file));

    pos = 0;
    while (TXT_get_symbol (confusion_text, pos, &symbol))
    {
      TXT_get_symbol (confusion_type, pos, &type);
      if (!type)
	  TXT_dump_symbol1 (file, symbol);
      else
      {
	switch (type)
	  {
	  case TRANSFORM_SYMBOL_TYPE:
	    fprintf (Files [file], "%%s");
	    break;
	  case TRANSFORM_MODEL_TYPE:
	    fprintf (Files [file], "%%m");
	    break;
	  case TRANSFORM_BOOLEAN_TYPE:
	    fprintf (Files [file], "%%b");
	    break;
	  case TRANSFORM_FUNCTION_TYPE:
	    fprintf (Files [file], "%%f");
	    break;
	  case TRANSFORM_WILDCARD_TYPE:
	    fprintf (Files [file], "%%w");
	    break;
	  case TRANSFORM_RANGE_TYPE:
	    range_text = symbol;
	    if (range_text == NIL)
	        fprintf (Files [file], "%%r");
	    else
	      {
	        fprintf (Files [file], "%%[");
		for (range_pos = 0; range_pos < TXT_length_text (range_text); range_pos++)
		  {
		    TXT_get_symbol (range_text, range_pos, &range_symbol);
		    TXT_dump_symbol (file, range_symbol);
		  }
	        fprintf (Files [file], "]");
	      }
	    break;
	  case TRANSFORM_SENTINEL_TYPE:
	    fprintf (Files [file], "%%$");
	    break;
	  case TRANSFORM_GHOST_TYPE:
	    fprintf (Files [file], "%%_");
	    break;
	  case TRANSFORM_SUSPEND_TYPE:
	    fprintf (Files [file], "%%.");
	    break;
	  default:
	    fprintf (Files [file], "invalid transform type: %d", type);
	    break;
	  }
      }
      pos++;
    }
}

void
dumpConfusionList (unsigned int file, struct confusionListType *head)
/* Dumps out the confusions in the confusion list. */
{
    struct confusionListType *here;

    here = head;
    while (here != NULL)
    {
        fprintf (Files [file], " ");
	dumpConfusionText (file, here->Confusion, here->Confusion_type);
        fprintf (Files [file], " [%.3f]", here->Codelength );
	here = here->Cnext;
    }
}

void
releaseConfusionList (struct confusionListType *head)
/* Release the confusions in the confusion list to the free list. */
{
    struct confusionListType *here, *there;

    here = head;
    while (here != NULL)
    {
	there = here;
	TXT_release_text (here->Confusion);
	TXT_release_text (here->Confusion_type);
	here = here->Cnext;
	Free (41, there, sizeof(struct confusionListType));
    }
}

struct confusionTrieType *
createConfusionTrie (struct confusionTrieType *node, unsigned int pos, float codelength,
		     unsigned int context, unsigned int context_type,
		     unsigned int confusion, unsigned int confusion_type)
/* Creates a new node (or reuses old node). Insert CONTEXT & its CONFUSION
   and their types into it. */
{
    struct confusionTrieType *newnode;
    unsigned int symbol, type;

    if (node != NULL)
        newnode = node;
    else
    {
	Confusion_Malloc += sizeof (struct confusionTrieType );
        newnode = (struct confusionTrieType *)
	    Malloc (42, sizeof (struct confusionTrieType));
    }
    assert (TXT_get_symbol (context, pos, &symbol));
    newnode->Csymbol = symbol;
    assert (TXT_get_symbol (context_type, pos, &type));
    newnode->Ctype = type;
    newnode->Context = TXT_copy_text (context);
    newnode->Context_type = TXT_copy_text (context_type);
    newnode->Confusions = addConfusionList (NULL, codelength, confusion, confusion_type);
    newnode->Cnext = NULL;
    newnode->Cdown = NULL;
    return (newnode);
}

struct confusionTrieType *
createConfusion (void)
/* Creates a new (empty) confusion trie. */
{
    struct confusionTrieType *newnode;
    unsigned int zero_context, zero_context_type;

    zero_context = TXT_create_text (); /* create zero length text */
    zero_context_type = TXT_create_text (); /* create zero length text */

    Confusion_Malloc += sizeof (struct confusionTrieType);
    newnode = (struct confusionTrieType *) Malloc (42, sizeof( struct confusionTrieType));
    newnode->Ctype = TRANSFORM_SYMBOL_TYPE;
    newnode->Csymbol = 0;
    newnode->Context = TXT_copy_text (zero_context);
    newnode->Context_type = TXT_copy_text (zero_context_type);
    newnode->Confusions = NULL;
    newnode->Cnext = NULL;
    newnode->Cdown = NULL;
    return (newnode);
}

struct confusionTrieType *
copyConfusionTrie( struct confusionTrieType *node )
/* Creates a new node by copying from an old one. */
{
    struct confusionTrieType *newnode;

    assert( node != NULL );
    Confusion_Malloc += sizeof (struct confusionTrieType);
    newnode = (struct confusionTrieType *)
	Malloc (42, sizeof( struct confusionTrieType));
    newnode->Ctype = node->Ctype;
    newnode->Csymbol = node->Csymbol;
    newnode->Context = TXT_copy_text (node->Context);
    newnode->Context_type = TXT_copy_text (node->Context_type);
    newnode->Confusions = node->Confusions;
    newnode->Cnext = node->Cnext;
    newnode->Cdown = node->Cdown;
    return (newnode);
}

struct confusionTrieType *
findConfusionSymbol (struct confusionTrieType *head, unsigned int csymbol, unsigned int ctype,
		     boolean *found)
/* Find the link that contains the symbol csymbol and return a pointer to it. Assumes
   the links are in ascending lexicographical order. If the symbol is not found,
   return a pointer to the previous link in the list. */
{
    struct confusionTrieType *here, *there;
    boolean found1;

    found1 = FALSE;
    if (head == NULL)
        return (NULL);
    here = head;
    there = NULL;
    while ((here != NULL) && (!found1))
      {
	if (ctype == here->Ctype)
	  {
	    if (csymbol == here->Csymbol)
	        found1 = TRUE;
	    else if (csymbol < here->Csymbol)
	      break;
	  }
	else if (here->Ctype == TRANSFORM_SYMBOL_TYPE)
	  { /* sort normal symbol types to end of the list */
	    there = NULL; /* means add at head of list */
	    break;
	  }
        if (!found1)
	  {
	    there = here;
	    here = here->Cnext;
	  }
      }
    *found = found1;
    if (!found1) /* link already exists */
        return( there );
    else
        return( here );
}

boolean
matchConfusionSymbol (unsigned int model, unsigned int source_symbol,
		      unsigned int previous_symbol, 
		      unsigned int source_text, unsigned int source_pos,
		      unsigned int confusion_symbol, unsigned int confusion_type)
/* Returns TRUE if the symbol matches the confusion symbol and type. */
{
    boolean (*confusion_function)  (unsigned int);
    boolean (*confusion_function1) (unsigned int, unsigned int, unsigned int,
				    unsigned int, unsigned int);

    switch (confusion_type)
      {
      case TRANSFORM_SYMBOL_TYPE:
	return (source_symbol == confusion_symbol);
	break;
      case TRANSFORM_MODEL_TYPE:
	/* Not done yet */
	return (FALSE);
	break;
      case TRANSFORM_BOOLEAN_TYPE:
	confusion_function = (boolean (*) (unsigned int)) (size_t) confusion_symbol;
	return (confusion_function (source_symbol));
	break;
      case TRANSFORM_WILDCARD_TYPE:
	return (TRUE);
	break;
      case TRANSFORM_FUNCTION_TYPE:
	/* get model number and previous symbol: */
	confusion_function1 =
	  (boolean (*) (unsigned int, unsigned int, unsigned int, unsigned int,
			unsigned int)) (size_t) confusion_symbol;
	return (confusion_function1 (model, source_symbol, previous_symbol,
				     source_text, source_pos));
      case TRANSFORM_RANGE_TYPE:
	/* Not done yet */
	return (FALSE);
	break;
      default:
	return (FALSE);
	break;
      }
    return (FALSE);
}

struct confusionTrieType *
insertListConfusion (struct confusionTrieType *head, struct confusionTrieType *here, unsigned int pos, float codelength,
		     unsigned int context, unsigned int context_type, unsigned int confusion, unsigned int confusion_type)
/* Insert new link after here and return the head of the list (which
   may have changed). Maintain the links in ascending lexicographical order. */
{
    struct confusionTrieType *there, *newnode;

    assert (head != NULL);

    if (here == NULL) { /* at the head of the list */
        /* maintain head at the same node position by copying it */
        newnode = copyConfusionTrie (head);
	createConfusionTrie (head, pos, codelength, context, context_type, confusion, confusion_type);
	head->Cnext = newnode;
	return( head );
    }
    newnode = createConfusionTrie (NULL, pos, codelength, context, context_type, confusion, confusion_type);
    there = here->Cnext;
    if (there == NULL) /* at the tail of the list */
	here->Cnext = newnode;
    else { /* in the middle of the list */
	here->Cnext = newnode;
	newnode->Cnext = there;
    }
    return( head );
}

struct confusionTrieType *
addConfusionNode(struct confusionTrieType *node, unsigned int pos, float codelength,
		 unsigned int context, unsigned int context_type,
		 unsigned int confusion, unsigned int confusion_type)
/* Add the CONTEXT & its CONFUSION into the NODE of the trie. If NODE is NULL,
   then creates and returns it. */
{
    struct confusionTrieType *here, *pnode;
    unsigned int symbol, type;
    boolean found;

    assert (context != NIL);
    if (node == NULL) {
	node = createConfusionTrie (NULL, pos, codelength, context, context_type, confusion, confusion_type);
	return (node);
    }
    assert (TXT_get_symbol (context, pos, &symbol));
    assert (TXT_get_symbol (context_type, pos, &type));
    here = findConfusionSymbol (node, symbol, type, &found);
    if (!found)
      { /* Not in the list - insert the new context */
        node = insertListConfusion (node, here, pos, codelength, context, context_type, confusion, confusion_type);
	return( node );
      }
    /* Found in the list - is it the same context? */
    if (here->Context != NIL) {
	if (!TXT_compare_text (context, here->Context))
	{
	    here->Confusions = addConfusionList (here->Confusions, codelength, confusion, confusion_type);
	    return( here );
	}
    }
    if (here->Cdown == NULL)
      { /* move old context one level down if needed */
	if (pos+1 < TXT_length_text (here->Context))
	  { /* check if not at end of the context */
	    node = copyConfusionTrie (here);
	    assert (TXT_get_symbol (here->Context, pos+1, &symbol));
	    assert (TXT_get_symbol (here->Context_type, pos+1, &type));
	    node->Csymbol = symbol;
	    node->Ctype = type;
	    here->Context = NIL;
	    here->Context_type = NIL;
	    here->Confusions = NIL;
	    node->Cnext = NULL;
	    here->Cdown = node;
	  }
      }
    if (pos+1 >= TXT_length_text (context))
      { /* end of the context */
	here->Context = TXT_copy_text (context);
	here->Context = TXT_copy_text (context_type);
	here->Confusions = addConfusionList (here->Confusions, codelength, confusion, confusion_type);
	return( here );
      }

    pnode = here->Cdown;
    node = addConfusionNode (pnode, pos+1, codelength, context, context_type, confusion, confusion_type);
    if (!pnode)
        here->Cdown = node;
    return( node );
}

void
addConfusion (struct confusionTrieType *confusions, float codelength,
	      unsigned int context, unsigned int context_type, unsigned int confusion, unsigned int confusion_type)
/* Adds the context and confusion to the trie. */
{
    struct confusionTrieType *node, *pnode;

    assert (confusions != NULL);

    if (TXT_length_text (context) == 0)
	confusions->Confusions = addConfusionList (confusions->Confusions, codelength, confusion, confusion_type);
    else
      {
	pnode = confusions->Cdown;
	node = addConfusionNode (pnode, 0, codelength, context, context_type, confusion, confusion_type);
	if (pnode == NULL)
	    confusions->Cdown = node;
      }
}

struct confusionTrieType *
findConfusionNode(struct confusionTrieType *node, unsigned int pos, unsigned int context, unsigned int context_type)
/* Find the CONTEXT at the NODE of the trie. */
{
    struct confusionTrieType *here;
    unsigned int symbol, type;
    boolean found;

    assert (context != NIL);
    if (node == NULL)
	return (NULL);

    assert (TXT_get_symbol (context, pos, &symbol));
    assert (TXT_get_symbol (context_type, pos, &type));
    here = findConfusionSymbol (node, symbol, type, &found);
    if (!found)
    { /* Not in the list */
	return (NULL);
    }
    /* Found in the list - is it the same context? */
    if (here->Context != NIL)
    {
        if (!TXT_compare_text (context, here->Context) && !TXT_compare_text (context_type, here->Context_type))
	{ /* context matches */
	    return( here );
	}
    }
    if (here->Cdown == NULL)
	return (NULL);

    return (findConfusionNode (here->Cdown, pos+1, context, context_type));
}

struct confusionListType *
findConfusions (struct confusionTrieType *confusions, unsigned int context, unsigned int context_type)
/* Find the list of confusions for CONTEXT and its cumulative frequency. */
{
    struct confusionTrieType *node;

    if (TXT_length_text (context) == 0)
        return (confusions->Confusions);
    else
      {
        node = findConfusionNode (confusions->Cdown, 0, context, context_type);
	if (node == NULL)
	    return (NULL);
	else
	    return (node->Confusions);
      }
}

void
dumpConfusionNode (unsigned int file, struct confusionTrieType *node)
/* Dumps out the contexts at the NODE in the trie. */
{
    assert (TXT_valid_file (file));

    while (node != NULL)
      {
        if (node->Context != NIL)
	  {
	    dumpConfusionText (file, node->Context, node->Context_type);
	    fprintf (Files [file], " ->");
	    dumpConfusionList (file, node->Confusions);
	  }
	fprintf (Files [file], "\n" );
	dumpConfusionNode (file, node->Cdown);
	node = node->Cnext;
      }
}

void
dumpConfusions (unsigned int file, struct confusionTrieType *confusions)
/* Dumps out the contexts in the confusion data structure. */
{

    dumpConfusionList (file, confusions->Confusions);
    fprintf (Files [file], "\n" );

    dumpConfusionNode (file, confusions->Cdown);
}


void
loadConfusion (unsigned int transform_model, unsigned int line,
	       float codelength)
/* Read in the confusion input line. */
{
    char context [MAX_CONFUSION_LENGTH], confusion [MAX_CONFUSION_LENGTH];
    unsigned int delim_pos, context_len, confusion_len, confusion_start;
    boolean found_delimeter;

    /* scan along along delimiter is found */
    delim_pos = CONFUSION_RULE_START;
    found_delimeter = TXT_getstring_text (line, CONFUSION_DELIMITER,
					  &delim_pos);
    assert (found_delimeter);

    context_len = delim_pos - CONFUSION_RULE_START - 1;
    assert (context_len < MAX_CONFUSION_LENGTH);
    TXT_extractstring_text (line, context, CONFUSION_RULE_START, context_len);

    confusion_start = delim_pos + CONFUSION_DELIMITER_LEN + 1;
    confusion_len = TXT_length_text (line) - confusion_start + 1;
    assert (confusion_len < MAX_CONFUSION_LENGTH);
    TXT_extractstring_text (line, confusion, confusion_start, confusion_len);

    TTM_add_transform (transform_model, codelength, context, confusion);
}

void
loadConfusions (unsigned int file, unsigned int transform_model)
/* Loads the confusions from the file. */
{
    unsigned int line;
    int freq, total;
    float codelength;

    line = TXT_create_text ();

    /* Read in the confusion data */
    while (TXT_readline_text (file, line) != EOF)
    {
	TXT_scanf_text (line, "%d %d", &freq, &total);
	assert ((freq > 0) && (total > 0) && (freq <= total));

	codelength = Codelength (0, freq, total);
	loadConfusion (transform_model, line, codelength);
    }

    TXT_release_text (line);
}

void
releaseConfusionNode (struct confusionTrieType *node)
/* Releases the NODE in the trie to the free list. */
{
    struct confusionTrieType *old_node;

    while (node != NULL)
      {
	old_node = node;
        TXT_release_text (node->Context);
	releaseConfusionList (node->Confusions);
	releaseConfusionNode (node->Cdown);
	node = node->Cnext;
	Free (42, old_node, sizeof(struct confusionTrieType));
      }
}

void
releaseConfusions (struct confusionTrieType *confusions)
/* Releases the confusion data structure to the free list. */
{
    releaseConfusionNode (confusions);
}

/* Scaffolding for Confusion module.
int
main()
{
    struct confusionTrieType *confusions;
    struct confusionListType *confusions_list;
    unsigned int context, context0, confusion;
    float codelength;

    context0 = TXT_create_text ();
    context = TXT_create_text ();
    confusion = TXT_create_text ();

    confusions = createConfusion ();

    for (;;)
    {
	printf ("Context? ");
	TXT_readline_text (stdin, context);

	printf ("Confusion? ");
	TXT_readline_text (stdin, confusion);

	printf ("Codelength? ");
	scanf ("%f", &codelength);
	addConfusion (confusions, codelength, context, confusion);
	addConfusion (confusions, codelength, context0, confusion);
	dumpConfusions (stdout, confusions);

	confusions_list = findConfusions (confusions, context, context_type);
	printf ("List of confusions: ");
	printf ("\n");

        dumpConfusionList (stdout, confusions_list);
        printf ("\n");
        while (confusions_list != NULL)
        {
	    TXT_dump_text (stdout, confusions_list->Confusion, TXT_dump_symbol1);
            printf (" %8.3f\n", confusions_list->Codelength);
            confusions_list = confusions_list->Cnext;
        }
        printf ("\n");

	confusions_list = findConfusions (confusions, context0, context0_type);
	printf ("List of null context confusions: ");
	printf ("\n");

        dumpConfusionList (stdout, confusions_list);
        printf ("\n");
        while (confusions_list != NULL)
        {
	    TXT_dump_text (stdout, confusions_list->Confusion, TXT_dump_symbol1);
            printf (" %8.3f\n", confusions_list->Codelength);
            confusions_list = confusions_list->Cnext;
        }
        printf ("\n");
    }
}
*/
