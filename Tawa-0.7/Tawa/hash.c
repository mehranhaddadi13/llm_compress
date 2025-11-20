/* Context position hash module. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "io.h"
#include "text.h"
#include "transform.h"
#include "model.h"
#include "hash.h"

#define HASHP_SIZE 5003     /* Size of hashp table */
#define HASHP_NUMBER 103669 /* Prime number for hashp function */

#define HASHM_SIZE 103      /* Size of hashm table */

unsigned int Hash_Malloc = 0;
unsigned int Hash_Mcount = 0;

struct hashpType *HashpTable [HASHP_SIZE]; /* The paths hash table */
struct hashpType *HashpUsed = NULL; /* Discarded hash records */
boolean HashpInit = FALSE;          /* = TRUE when it has been initialized */

struct hashmType *HashmTable [HASHM_SIZE]; /* The transform model hash table */
boolean HashmInit = FALSE;          /* = TRUE when it has been initialized */

void
junkHash (void)
{
    printf ("Got here\n");
}

int
hashp (unsigned int transform_model, unsigned int language_model,
       unsigned int input_position, unsigned int context_position)
{
    int hash;

    if (!context_position)
        return (language_model);
    else
      {
	hash = (((input_position+1) * language_model) % HASHP_NUMBER) +
	    (language_model - 1);
	hash = (context_position * hash) % HASHP_SIZE;
	return (hash);
      }
}

struct hashpType *
allocHashp (void)
/* Return a new pointer to a hashp record */
{
    struct hashpType *h;

    if (HashpUsed == NULL)
    {
	Hash_Malloc += sizeof (struct hashpType);
	Hash_Mcount++;
	h = (struct hashpType *) Malloc (21, sizeof (struct hashpType));
	if (h == NULL)
	{
	    fprintf (stderr, "Fatal error: out of hashp record space\n" );
	    exit (1);
	}
    }
    else
    { /* use the first record on the used list */
	h = HashpUsed;
	HashpUsed = HashpUsed->H_next;
    }
    return (h);
}

void
initHashp (unsigned int transform_model)
/* (Re)-initializes the hashp table. */
{
    struct hashpType *here, *tail;
    int h;

    if (!HashpInit)
    {
        HashpInit = TRUE;
	for (h=0; h<HASHP_SIZE; h++)
	{
	    HashpTable [h] = NULL;
	}
	return;
    }

    /* Find tail of used list */
    tail = NULL;
    here = HashpUsed;
    while (here != NULL)
    {
	tail = here;
	here = here->H_next;
    }

    for (h=0; h<HASHP_SIZE; h++)
    {
	here = HashpTable [h];
	/* join this list onto end of used list */
	if (tail == NULL)
	    HashpUsed = here;
	else
	    tail->H_next = here;
	/* find tail of this list */
	while (here != NULL)
	{
	    tail = here;
	    here = here->H_next;
	}
	/* re-initialize this list */
	HashpTable [h] = NULL;
    }
}

void
dumpHashp (unsigned int file, unsigned int transform_model)
/* Dumps the hashp table. */
{
    struct hashpType *here;
    int h, count;

    assert (TXT_valid_file (file));

    count = 0;
    fprintf (Files [file], "Hashp table:\n");
    for (h=0; h < HASHP_SIZE; h++)
    {
	here = HashpTable [h];
	while (here != NULL)
	{
	    count++;
	    fprintf (Files [file], "%p (%4d) %4d %5d %7d %7d %.3f %.3f\n",
		     (void *) here, h, here->H_transform_model, here->H_language_model,
		     here->H_input_position, here->H_context_position,
		     here->H_total_codelength, here->H_symbol_codelength);
	    here = here->H_next;
	}
    }
    fprintf (Files [file], "Number of entries = %d\n", count);
}

unsigned int
countHashp (unsigned int transform_model)
/* Returns the count of the number of entries in the hashm table. */
{
    struct hashpType *here;
    int h, count;

    count = 0;
    for (h=0; h<HASHP_SIZE; h++)
    {
	here = HashpTable [h];
	while (here != NULL)
	{
	    count++;
	    here = here->H_next;
	}
    }
    return (count);
}

struct hashpType *
findHashp (unsigned int transform_model, unsigned int language_model,
	   unsigned int input_position, int context_position)
/* Finds and returns the position of the tuple [transform_model, language_model,
   input position, context position] in the hashp table. */
{
    struct hashpType *here;
    unsigned int h;

    h = hashp (transform_model, language_model, input_position, context_position);

    /*
      fprintf (stderr,
      "Language_Model = %d Input = %d Context = %d Value returned = %d\n",
      language_model, input_position, context_position, h)
    */

    /* find if the position already exists */
    here = HashpTable [h];
    while ((here != NULL) && !((transform_model == here->H_transform_model) &&
	(language_model == here->H_language_model) &&
        (input_position == here->H_input_position) &&
        (context_position == here->H_context_position)))
	here = here->H_next;

    return (here);
}
	  
struct hashpType *
addHashp (unsigned int transform_model, unsigned int language_model,
	  unsigned int input_position, int context_position,
	  float total_codelength, float symbol_codelength,
	  boolean *update, boolean *added)
/* Adds the tuple [transform_model, language_model, input position,
   context position] into the hashp table. Returns non-zero update if the
   position is new or updated; and non-zero added if the position is new. */
{
    struct hashpType *here;
    unsigned int h;

    here = findHashp (transform_model, language_model, input_position,
		      context_position);

    if (here != NULL) /* found - update codelength if it is less */
    {
	*added = FALSE;
	if (total_codelength >= here->H_total_codelength)
	    *update = FALSE;
	else
	{
	    *update = TRUE;
	    here->H_total_codelength = total_codelength;
	    here->H_symbol_codelength = symbol_codelength;
	}
    }
    else
    { /* not found - add it at the head of the list */
	*added = TRUE;
	*update = TRUE;

	h = hashp (transform_model, language_model, input_position,
		   context_position);

	here = allocHashp ();
	here->H_transform_model = transform_model;
	here->H_language_model = language_model;
	here->H_input_position = input_position;
	here->H_context_position = context_position;
	here->H_total_codelength = total_codelength;
	here->H_symbol_codelength = symbol_codelength;
	here->H_leaf = NULL;
	here->H_next = HashpTable [h];
	HashpTable [h] = here;
    }

    /*
    fprintf (stderr,
      "Adding language_model %d input position %d context position %d ",
      language_model, input_position, context_position);
    fprintf (stderr, " hash = %4d codelength = %.3f update = %d\n",
             h, total_codelength, *update );
    dumpHashp (Stderr_File, transform_model);
    */

    return (here);
}

int
hashm (unsigned int transform_model, unsigned int language_model)
{
    /* Ignore the transform model number for the time being */

    return (language_model % HASHM_SIZE);
}

struct hashmType *
allocHashm (void)
/* Return a new pointer to a hashm record */
{
    struct hashmType *h;

    Hash_Malloc += sizeof (struct hashmType);
    Hash_Mcount++;
    h = (struct hashmType *) Malloc (21, sizeof (struct hashmType));
    if (h == NULL)
      {
	fprintf (stderr, "Fatal error: out of hashm record space\n" );
	exit (1);
      }

    return (h);
}

void
initHashm (unsigned int transform_model)
/* Initializes the hashm table. */
{
    struct hashmType;
    int h;

    assert (!HashmInit);

    HashmInit = TRUE;
    for (h=0; h<HASHM_SIZE; h++)
      {
	HashmTable [h] = NULL;
      }
    return;
}

void
dumpHashm (unsigned int file, unsigned int transform_model)
/* Dumps the hashm table. */
{
    struct hashmType *here;
    int h, count;

    assert (TXT_valid_file (file));

    count = 0;
    fprintf (Files [file], "Hashm table:\n");
    for (h=0; h<HASHM_SIZE; h++)
    {
	here = HashmTable [h];
	while (here != NULL)
	{
	    count++;
	    fprintf (Files [file], "%p (%4d) %4d %2d %5d %7d %.3f %.3f %.3f\n",
		     (void *) here, h, here->H_transform_model, here->H_transform_type,
		     here->H_language_model, here->H_input_position,
		     here->H_total_codelength, here->H_symbol_codelength,
		     here->H_sentinel_codelength);
	    here = here->H_next;
	}
    }
    fprintf (Files [file], "Number of entries = %d\n", count);
}

void
releaseHashm (unsigned int transform_model)
/* Releases the hashm table to memory for latter re-use. */
{
    struct hashmType *here, *this;
    int h;

    for (h=0; h<HASHM_SIZE; h++)
    {
	here = HashmTable [h];
	while (here != NULL)
	{
	    this = here;
	    here = here->H_next;
	    free (this);
	}
	HashmTable [h] = NULL;
    }
}

unsigned int
countHashm (unsigned int transform_model)
/* Returns the count of the number of entries in the context position hash
   table. */
{
    struct hashmType *here;
    int h, count;

    count = 0;
    for (h=0; h<HASHM_SIZE; h++)
    {
	here = HashmTable [h];
	while (here != NULL)
	{
	    count++;
	    here = here->H_next;
	}
    }
    return (count);
}

struct hashmType *
findHashm (unsigned int transform_model, unsigned int language_model)
/* Finds and returns the position of the tuple [transform_model, language_model]
   in the hashm table. */
{
    struct hashmType *here;
    unsigned int h;

    h = hashm (transform_model, language_model);

    /*
      fprintf (stderr,
      "Language_Model = %d Value returned = %d\n", language_model, h)
    */

    /* find if the position already exists */
    here = HashmTable [h];
    while ((here != NULL) && !((transform_model == here->H_transform_model) &&
	(language_model == here->H_language_model)))
	here = here->H_next;

    return (here);
}
	  
void
startHashm (unsigned int transform_model, unsigned int transform_type,
	    unsigned int language_model)
/* Starts a new hashm entry for the tuple [transform_model, language_model]. */
{
    struct hashmType *here;
    unsigned int h;

    if (!HashmInit)
        initHashm (transform_model);

    assert (!findHashm (transform_model, language_model));

    h = hashm (transform_model, language_model);

    here = allocHashm ();
    here->H_transform_model = transform_model;
    here->H_transform_type = transform_type;
    here->H_language_model = language_model;
    here->H_context = NIL;
    here->H_sentinel_context = NIL;
    here->H_input_position = 0;
    here->H_total_codelength = 0.0;
    here->H_symbol_codelength = 0.0;
    here->H_sentinel_codelength = 0.0;
    here->H_next = HashmTable [h];
    HashmTable [h] = here;
}

struct hashmType *
updateHashm (unsigned int transform_model, unsigned int language_model,
	     unsigned int source_pos, unsigned int source_symbol)
/* Updates and returns the hashm entry for the tuple [transform_model,
   language_model]. */
{
    struct hashmType *here;
    /*unsigned int transform_type;*/
    float codelength;

    assert (here = findHashm (transform_model, language_model));

    if ((here->H_context == NIL) || (source_pos > here->H_input_position))
      {
	if (here->H_context == NIL) /* first time through */
	  {
	    here->H_context = TLM_create_context (language_model);
	    here->H_sentinel_context = TLM_create_context (language_model);
	  }
	if (source_pos > here->H_input_position)
	  {
	    /* Make sure that each model's context gets updated
	       in sequential order; this is a kludge check for now - the
	       source_text should really be passed to this routine, and
	       (depending on what's required) the context is updated
	       from the old input_position to the new source_pos */
	    assert (source_pos-1 == here->H_input_position);
	  }

	/*transform_type = here->H_transform_type;*/ /* Not used */

        here->H_input_position = source_pos;
	TLM_overlay_context (language_model, here->H_context,
			     here->H_sentinel_context);

	/* Strictly speaking, a "switch" (to another model) symbol should
	   be used here; however, a sentinel symbol can be used instead
	   (hopefully!) as its statistics do not change (unless some
	   application used more than one sentinel symbol), and TLM_find_symbol
	   also does not do any update of the model either */
	TLM_find_symbol (language_model, here->H_sentinel_context,
			 TXT_sentinel_symbol ());
	here->H_sentinel_codelength = TLM_Codelength;

	TLM_update_context (language_model, here->H_context, source_symbol);
	codelength = TLM_Codelength;

	here->H_total_codelength += codelength;
	here->H_symbol_codelength = codelength;
      }
    return (here);
}

/* Scaffolding for hash module
int
main()
{
    unsigned int transform_model, language_model, input_position, update, added;
    float total_codelength, symbol_codelength;
    int context_position;

    transform_model = 1;
    initHashp (transform_model);
    for (;;)
    {
	printf( "language_model? " );
	scanf( "%d", &language_model );
	printf( "input position? " );
	scanf( "%d", &input_position );
	printf( "context position? " );
	scanf( "%d", &context_position );
	printf( "total_codelength? " );
	scanf( "%f", &total_codelength );
	printf( "symbol_codelength? " );
	scanf( "%f", &symbol_codelength );
	addHashp (transform_model, language_model, input_position,
	          context_position, total_codelength, symbol_codelength,
		  &update, &added);
	dumpHashp (stdout, transform_model);
    }
}
*/
