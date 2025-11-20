/* Path routines. This module keeps track of the minimum codelength
   paths for the different possible versions of the transformed text. */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "hash.h"
#include "model.h"
#include "io.h"
#include "text.h"
#include "confusion.h"
#include "transform.h"
#include "paths.h"

struct leafType
{                                     /* record for storing a leaf in the list */
    struct pathType *L_path;	      /* pointer to leaf in the paths trie */
    struct leafType *L_next;	      /* next in the leaf list */
    struct leafType *L_prev;	      /* previous in the leaf list */
    unsigned int L_pos;               /* Position in input; this will be < current
					 position if this leaf has not been
					 expanded fully yet */
    unsigned int L_symbol;            /* symbol that was encoded */
    unsigned int L_model;             /* current model */
    unsigned int L_context;           /* current context pointer in the model */
    float L_codelength;		      /* Total codelength of the probability associated
					 with this path. */
};

struct leafType *FallenLeaves = NULL; /* Discarded leaves */

struct pathType
{				      /* paths trie record */
    unsigned int P_symbol;	      /* the symbol at this point in the path */
    unsigned int P_model;             /* the model being used to predict the symbol */
    struct pathType *P_child;         /* pointer to child node in the paths trie */
    struct pathType *P_parent;        /* pointer to parent node in the paths trie */
    struct pathType *P_next;          /* pointer to next node in paths trie */
    struct pathType *P_prev;          /* pointer to previous node in paths trie */
};

struct pathQType
{ /* paths queue type used to delete a paths trie without recursion */
    struct pathType *Q_path;          /* pointer to a path node in the paths trie */
    struct pathQType *Q_next;         /* pointer to next node in paths queue */
};


int Leaf_Malloc = 0;
int Path_Malloc = 0;

void
debug_paths ()
/* Dummy routine for debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

void
printPathsMalloc (unsigned int file)
/* Prints out the size of memory allocated. */
{
    assert (TXT_valid_file (file));

    fprintf (Files [file], "Leaves %d Paths %d Hash %d Total %d\n",
	     Leaf_Malloc, Path_Malloc, Hash_Malloc,
	    (Leaf_Malloc+Path_Malloc+Hash_Malloc));
}

int last=0;

struct leafType *
allocLeaf (void)
/* Return a new pointer to a leaf record */
{
    struct leafType *leaf;

    if (FallenLeaves == NULL)
    {
	Leaf_Malloc += sizeof (struct leafType);
	leaf = (struct leafType *) Malloc (51, sizeof (struct leafType));
	if (leaf == NULL)
	{
	    fprintf (stderr, "Fatal error: out of leaf record space\n" );
	    exit (1);
	}
    }
    else
    { /* use the first record on the dead leaves list */
	leaf = FallenLeaves;
	FallenLeaves = FallenLeaves->L_next;
    }
    return (leaf);
}

struct leafType *
addLeaf (unsigned int transform_model, struct pathType *path, unsigned int pos,
	 unsigned int model, unsigned int context, unsigned int symbol,
	 float codelength)
/* Add a new leaf onto the leaves list, either at the head of it or in
   ascending codelength order (but always after the head) depending on
   the transform models algorithm. */
{
    struct leafType *head, *newleaf, *thisleaf, *thatleaf;

    if (Debug.level1 > 4)
      fprintf (stderr, "Adding leaf: model %d context %d pos %d symbol %d codelength %.3f\n",
	       model, context, pos, symbol, codelength);

    newleaf = allocLeaf ();
    newleaf->L_path = path;
    newleaf->L_next = NULL;
    newleaf->L_prev = NULL;
    newleaf->L_pos = pos;
    newleaf->L_symbol = symbol;
    newleaf->L_model = model;
    newleaf->L_context = context;
    newleaf->L_codelength = codelength;

    head = Transforms [transform_model].Transform_leaves;

    if (Transforms [transform_model].Transform_algorithm == TTM_Viterbi)
      { /* add at the head of the leaves list */
	newleaf->L_next = head;
	if (head != NULL)
	    head->L_prev = newleaf;

	Transforms [transform_model].Transform_leaves = newleaf;
	return (newleaf);
      }

    /* insert the new leaf in sorted order */
    if (head == NULL)
        thisleaf = NULL;
    else
        thisleaf = head->L_next; /* go to next in list */
    thatleaf = head;
    while ((thisleaf != NULL) && (codelength > thisleaf->L_codelength))
    {
	thatleaf = thisleaf;
	thisleaf = thisleaf->L_next;
    }
    /* insert into list */
    newleaf->L_next = thisleaf;
    newleaf->L_prev = thatleaf;
    if (thisleaf != NULL)
	thisleaf->L_prev = newleaf;
    if (thatleaf == NULL)
	head = newleaf;
    else
	thatleaf->L_next = newleaf;
    Transforms [transform_model].Transform_leaves = head;

    return (newleaf);
}

struct leafType *
pruneLeaf (struct leafType *head, struct leafType *leaf)
/* Prunes out the leaf from the leaves list, and returns the new or old head. */
{
    if ((head == NULL) || (leaf == NULL))
        return (head);

    if (leaf->L_context != NIL)
        TLM_release_context (leaf->L_model, leaf->L_context);

    if (leaf->L_prev == NULL) 
        head = leaf->L_next;
    else
        leaf->L_prev->L_next = leaf->L_next;
    if (leaf->L_next != NULL)
        leaf->L_next->L_prev = leaf->L_prev;

    /* add at the head of the FallenLeaves list */
    leaf->L_next = FallenLeaves;
    FallenLeaves = leaf;

    return (head);
}

void
getLeaf (struct leafType *leaf, unsigned int *model, unsigned int *previous_symbol)
/* Gets various information associated with the leaf */
{
    struct pathType *path;

    if (leaf == NIL)
      {
	*model = NIL;
	*previous_symbol = TXT_sentinel_symbol ();
      }
    else
      {
	*model = leaf->L_model;
	path = leaf->L_path;
	if (path == NIL)
	    *previous_symbol = TXT_sentinel_symbol ();
	else
	    *previous_symbol = path->P_symbol;
      }
}

void
prunePath (struct pathType *path)
/* Removes the path node path from the paths trie if it can. Also recursively
   removes any of the path node's ancestors if it is the only thing
   along the path being pointed at. */
{
    struct pathType *child, *parent, *nextnode, *prevnode;

    assert (path != NULL);

    child = path->P_child;
    if (child != NULL)
        return; /* don't prune away if it has children */

    parent = path->P_parent;
    nextnode = path->P_next;
    prevnode = path->P_prev;

    if (nextnode != NULL)
        nextnode->P_prev = prevnode;
    if (prevnode != NULL)
	prevnode->P_next = nextnode;
    else if (parent != NULL)
      {
	parent->P_child = nextnode;
	if (nextnode == NULL) /* parent no longer has any children - remove it */
	    prunePath (parent); 
      };

    Free (52, path, sizeof (struct pathType));
    Path_Malloc -= sizeof (struct pathType);
}

struct leafType *
pruneLeaves (unsigned int transform_model, unsigned int pos)
/* Prune out all the old leaf records for later re-use. */
{
    struct leafType *head, *thisleaf, *nextleaf;
    unsigned int drop, depth, stack_depth, stack_extension;

    head = Transforms [transform_model].Transform_leaves;
    if (Transforms [transform_model].Transform_algorithm != TTM_Stack)
        return (head);

    stack_depth = Transforms [transform_model].Transform_stack_depth;
    stack_extension = Transforms [transform_model].Transform_stack_extension;

    /* prune out old leaf records */
    depth = 0;
    thisleaf = head;
    while (thisleaf != NULL)
    {
        depth++;
	nextleaf = thisleaf->L_next;

	/* Note: the following pruning is sub-optimal */

	drop = FALSE;
	if ((stack_depth != 0) && (depth > stack_depth))
	    drop = TRUE;
	else if ((stack_extension != 0) &&
		 (pos > thisleaf->L_pos + stack_extension))
	    drop = TRUE;

	if (drop)
	{ /* leaf is bad or too old - drop it */
	    prunePath (thisleaf->L_path); /* prune the path as well */
	    head = pruneLeaf (head, thisleaf);
	}

	thisleaf = nextleaf;
    }
    return (head);
}

struct leafType *
releaseLeaves (struct leafType *head)
/* Releases all the leaf records for later re-use. */
{
    struct leafType *thisleaf, *nextleaf;

    /* delete leaf records */
    thisleaf = head;
    while (thisleaf != NULL)
    {
	nextleaf = thisleaf->L_next;
	if (thisleaf->L_context != NIL)
	    TLM_release_context (thisleaf->L_model, thisleaf->L_context);

	/* add at the head of the FallenLeaves list */
	thisleaf->L_next = FallenLeaves;
	FallenLeaves = thisleaf;

	thisleaf = nextleaf;
    }
    return (NIL);
}

void
dumpLeaves (unsigned int file, unsigned int transform_model)
/* Dump the leaves list. */
{
    unsigned int position;
    struct leafType *p;

    assert (TXT_valid_file (file));

    fprintf (Files [file], "Dump of leaves list :\n");
    fprintf (Files [file], "---------------------\n");
    p = Transforms [transform_model].Transform_leaves;
    while (p)
    {
        if (p->L_context == NIL)
	    position = 0;
	else
	    position = TLM_getcontext_position (p->L_model, p->L_context);
	fprintf (Files [file], "%8p %8p pos %7d model %7d cont %7d posn %7d codelength %7.3f\n",
		 (void *) p, (void *) p->L_path, p->L_pos, p->L_model, p->L_context,
		 position, p->L_codelength);
	p = p->L_next;
    }
    fprintf (Files [file], "---------------------\n");
}

int
dumpPathLeaf (unsigned int file, unsigned int transform_model, struct pathType *path)
/* Dump the leaf for the path. */
{
    struct leafType *p;

    assert (TXT_valid_file (file));

    p = Transforms [transform_model].Transform_leaves;
    while (p)
    {
	if (path == p->L_path)
	{
	    fprintf (Files [file], "%8p %7d %7d %7.3f ", (void *) p, p->L_model,
		     p->L_context, p->L_codelength);
	    return (1);
	}
	p = p->L_next;
    }
    return (0);
}

void
dumpPathSymbols (unsigned int file, unsigned int text)
/* Dumps out the path symbols to the file. */
{
  unsigned int symbol, previous_symbol, sentinel_symbol, pos;

  assert (TXT_valid_file (file));

  pos = 0;
  previous_symbol = NIL;
  sentinel_symbol = TXT_sentinel_symbol ();
  while (TXT_get_symbol (text, pos++, &symbol))
    {
      if (previous_symbol != sentinel_symbol)
	  TXT_dump_symbol1 (file, symbol);
      else if (symbol == sentinel_symbol)
	  fprintf (Files [file], "<sentinel>");
      else
	  switch (symbol)
	    {
	    case MODEL_SYMBOL:
	      fprintf (Files [file], "<model>");
	      break;
	    case GHOST_SYMBOL:
	      fprintf (Files [file], "<ghost>");
	      break;
	    case SUSPEND_SYMBOL:
	      fprintf (Files [file], "<suspend>");
	      break;
	    default:
	      if (Transform_dump_symbol_function == NULL)
		  TXT_dump_symbol1 (file, symbol);
	      else
		  Transform_dump_symbol_function (file, symbol);
	    }
      previous_symbol = symbol;
    }
}

void
dumpPath (unsigned int file, unsigned int transform_model, struct pathType *path,
	  unsigned int d, unsigned int pathstring)
/* Dump the path trie node; d is 0 when at the top level. */
{
    struct pathType *child;
    unsigned int sym;

    assert (TXT_valid_file (file));

    while (path != NULL)
    {
	sym = path->P_symbol;
	child = path->P_child;
	if (d > 0)
	  {
	    TXT_setlength_text (pathstring, d-1);
	    TXT_append_symbol (pathstring, sym);
	  }
	if (child != NULL)
	    dumpPath (file, transform_model, child, d+1, pathstring);
	else
	{
	    if (Debug.level1 > 4)
	        fprintf (Files [file], "%8p ", (void *) path);
	    if (dumpPathLeaf (file, transform_model, path) || (Debug.level1 > 4))
	    {
	        fprintf (Files [file], "[" );
		if (Transform_dump_symbols_function == NULL)
		    TXT_dump_text (file, pathstring, NULL);
		else
		    Transform_dump_symbols_function (file,  pathstring);
		fprintf (Files [file], "]\n");
	    }
	}
	path = path->P_next;
    }
}

void
dumpPaths (unsigned int file, unsigned int transform_model)
/* Dump the paths trie. */
{
    unsigned int pathstring;

    assert (TXT_valid_file (file));

    pathstring = TXT_create_text ();

    fprintf (Files [file], "Dump of Paths: \n");
    fprintf (Files [file], "--------------\n");
    dumpPath (file, transform_model, Transforms [transform_model].Transform_paths, 0, pathstring);
    fprintf (Files [file], "--------------\n");
    fprintf (Files [file], "\n");

    TXT_release_text (pathstring);
}

void
dumpConfusion (unsigned int confusion_text)
/* Debug routine for debugging the confusion text. */
{
    
    fputc ('[', stderr);
    /*
    if (Transform_dump_confusion_function == NULL)
        TXT_dump_text (Stderr_File, confusion_text, NULL);
    else
        Transform_dump_confusion_function (Stderr_File, confusion_text);
    */
    dumpPathSymbols (Stderr_File, confusion_text);
    fputc (']', stderr);
    fprintf (stderr, "\n");
}

void
releasePath (struct pathType *path)
/* Releases the path trie node. */
{
    struct pathType *child, *here;

    while (path != NULL)
    {
	child = path->P_child;
	if (child != NULL)
	  {
	    fprintf (stderr, "Releasing path %p child %p\n",
		     (void *) path, (void *) child);
	    releasePath (child);
	  }
	here = path;
	path = path->P_next;
	Free (52, here, sizeof (struct pathType));
	Path_Malloc -= sizeof (struct pathType);
    }
}

struct pathQType *
createPathQ (struct pathType *path)
/* Creates a new pathq record. */
{
    struct pathQType *pathq;

    pathq = (struct pathQType *) malloc (sizeof (struct pathQType));
    assert (pathq != NULL);

    pathq->Q_path = path;
    pathq->Q_next = NULL;

    return (pathq);
}

struct pathQType *
addPathQ (struct pathQType *pathq, struct pathType *path)
/* Appends the path at the head  of the path queue (for latter deletion).
   Return the pointer to the head of the queue (which may change). */
{
    struct pathQType *newq;

    if (path == NULL)
      /* No need to add it to the queue */
        return (pathq);

    newq = createPathQ (path);
    /* Add to head of the queue */
    newq->Q_next = pathq;

    return (newq);
}

struct pathType *
headPathQ (struct pathQType *pathq)
/* Returns the path stored at the head of the path queue. */
{
    if (pathq == NULL)
        return (NULL);
    else
        return (pathq->Q_path);
}

struct pathQType *
popPathQ (struct pathQType *pathq)
/* Pops the first record off the path queue and release the path record
   associated with it. */
{
    struct pathQType *nextq;

    if (pathq == NULL)
        return (NULL);

    nextq = pathq->Q_next;
    free (pathq);

    return (nextq);
}

void
releasePath1 (struct pathType *path)
/* Releases the path trie node. Does not use recursion (to avoid stack size
   problems). */
{
    struct pathQType *pathq = NULL;
    struct pathType *child, *curr;

    pathq = addPathQ (pathq, path);
    while (pathq != NULL)
      {
	/* Free the path record at the head of the queue, then pop it off */
	path = headPathQ (pathq);
	if (path != NULL)
	  {
	    curr = path->P_next;
	    child = path->P_child;
	  }

	/*fprintf (stderr, "Releasing path %p\n", (void *) path);*/
	Free (52, path, sizeof (struct pathType));
	Path_Malloc -= sizeof (struct pathType);

	pathq = popPathQ (pathq);
	pathq = addPathQ (pathq, child);

	while (curr != NULL)
	  {
	    child = curr->P_child;
	    /* Add to queue of paths to be deleted */
	    pathq = addPathQ (pathq, child);

	    curr = curr->P_next;
	  }
      }
}

void
releasePaths (unsigned int transform_model)
/* Frees the paths trie and leaves. */
{
    /* Release the paths trie */
    releasePath1 (Transforms [transform_model].Transform_paths);
    Transforms [transform_model].Transform_paths = NULL;

    Transforms [transform_model].Transform_leaves =
        releaseLeaves (Transforms [transform_model].Transform_leaves);

    printPathsMalloc (Stderr_File);
}


struct pathType *
allocPath (unsigned int symbol, unsigned int model)
/* Return a pointer to the next free path trie record. */
{
    struct pathType *path;

    Path_Malloc += sizeof (struct pathType);
    path = (struct pathType *) Malloc (52, sizeof (struct pathType));
    if (path == NULL)
    {
	fprintf (stderr, "Fatal error: out of path trie space\n" );
	exit (1);
    }
    path->P_child = NULL;
    path->P_parent = NULL;
    path->P_next = NULL;
    path->P_prev = NULL;
    path->P_symbol = symbol;
    path->P_model = model;
    return (path);
}

struct pathType *
addPath (struct pathType *path, unsigned int model, unsigned int symbol)
/* Extend the path by adding the symbol to node path, and
   return a pointer to the new node. */
{
    struct pathType *newpath, *child;

    assert (path != NULL);
    newpath = allocPath (symbol, model);
    newpath->P_parent = path;

    /* add newpath at the head of path's list of children */
    child = path->P_child;
    if (child != NULL)
      {
	newpath->P_next = child;
	child->P_prev = newpath;
      }

    path->P_child = newpath;

    return (newpath);
}

void
startPaths (unsigned int transform_model)
/* Frees the paths trie and leaves. */
{
    releaseHashm (transform_model);
}

void
startPath (unsigned int transform_model, unsigned int transform_type,
	   unsigned int language_model)
/* Starts a new path in the paths trie for the language model. */
{
    struct transformType *transform;
    struct pathType *newpath;
    unsigned int context;

    /*printPathsMalloc (Stderr_File);*/

    assert (TTM_valid_transform (transform_model));
    transform = Transforms + transform_model;

    startHashm (transform_model, transform_type, language_model);

    newpath = transform->Transform_paths;
    if (transform->Transform_paths == NULL)
      { /* Create root of Paths trie */
	/* Set symbol at root of trie to be NIL : */
	transform->Transform_paths = allocPath (NIL, NIL);
      }

    /* Set context operations to return codelength values */
    TLM_set_context_operation (TLM_Get_Codelength);

    /* Set symbol at start of path to be the language model itself: */
    newpath = addPath (transform->Transform_paths, language_model,
		       TXT_sentinel_symbol ());
    newpath = addPath (newpath, language_model, TRANSFORM_MODEL_TYPE);
    newpath = addPath (newpath, language_model, language_model);

    if (transform_type != TTM_transform_multi_context)
        /* NIL context means a single context transform is being used */
        context = NIL;
    else
        context = TLM_create_context (language_model);
    addLeaf (transform_model, newpath, 0, language_model, context, 0, 0.0);

    /*
    fprintf (stderr, "End of startup - dump of paths:\n");
    dumpPaths (Stderr_File, transform_model);
    */
}

float
extendPathSymbol (unsigned int transform_model, unsigned int model,
		  unsigned int context, unsigned int source_symbol,
		  unsigned int symbol, unsigned int special_symbol,
		  unsigned int input_position)
/* Extend the path using the (confusion's) symbol and special symbol and
   returns its codelength. */
{
    float codelength;
    struct hashmType *hashm;

    codelength = 0;
    if (context == NIL)
      { /* NIL context means a single context transform is being used */
	hashm = updateHashm (transform_model, model, input_position,
			     source_symbol);
	if (Debug.level1 > 1)
	    fprintf (stderr,
	      "hashm: model %d pos %d sentinel codelength %.3f codelength %.3f\n",
	       model, input_position, hashm->H_sentinel_codelength,
	       hashm->H_symbol_codelength);

	if (symbol == TXT_sentinel_symbol ())
	    codelength = hashm->H_sentinel_codelength;
	else
	    codelength = hashm->H_symbol_codelength;
      }
    else
      {
	if (!special_symbol)
	  {
	    TLM_update_context (model, context, symbol);
	    codelength = TLM_Codelength;
	  }
	else
	  {
	    if (special_symbol == SUSPEND_SYMBOL)
	      {
		/* Set context operations to return no codelength values i.e. faster */
		TLM_set_context_operation (TLM_Get_Nothing);
		TLM_update_context (model, context, symbol);
		/* Set future context operations to return codelength values */
		TLM_set_context_operation (TLM_Get_Codelength);
	      }
	    /* else just do nothing (for GHOST_SYMBOL) */
	  }
      }
    return (codelength);
}

boolean
extendPath (unsigned int transform_model, struct leafType *leaf,
	    unsigned int source_pos, unsigned int source_symbol,
	    unsigned int context_length, unsigned int confusion_text,
	    float confusion_codelength)
/* Extend the path using the symbols found in the confusion_text. Returns
   FALSE if the path was not extended (i.e. it wasn't as good as another
   path with the same memory). Confusion_codelength is the codelength of the
   confusion probability for these symbols. */
{
    struct pathType *path;
    struct pathType *newpath;
    struct leafType *oldleaf, *newleaf;
    struct hashpType *hashp;
    float codelength, tcodelength;
    unsigned int context, input_position, old_context;
    unsigned int sentinel_symbol, special_symbol, confusion_symbol, previous_symbol;
    unsigned int model, p;
    boolean update, added;
    int context_position;

    path = leaf->L_path;
    old_context = leaf->L_context;
    tcodelength = leaf->L_codelength;
    input_position = leaf->L_pos;
    model = path->P_model;

    if (old_context == NIL)
        context = NIL;
    else /* Make copy of context for future path */
        context = TLM_copy_context (model, old_context);

    codelength = 0.0;
    p = 0;
    special_symbol = NIL;
    previous_symbol = NIL;
    sentinel_symbol = TXT_sentinel_symbol ();

    /*
    fprintf (stderr, "confusion text = ");
    dumpPathSymbols (Stderr_File, confusion_text);
    fprintf (stderr, "\n");
    */

    while (TXT_get_symbol (confusion_text, p++, &confusion_symbol))
    { /* encode each confusion_symbol */
      if (special_symbol == MODEL_SYMBOL)
	{ /* need to switch context into a new model */
	  codelength += extendPathSymbol
	    (transform_model, model, context, source_symbol, sentinel_symbol,
	     NIL, input_position);
	  if (context != NIL)
	      TLM_release_context (model, context);
	  model = confusion_symbol; /* The symbol in the confusion_text contains the model number */

	  assert (TLM_valid_model (model));
	  if (findHashm (transform_model, model))
	    /* NIL context means a single context transform is being used */
	      context = NIL;
	  else
	      context = TLM_create_context (model);
	  special_symbol = NIL;
	}
	else if ((previous_symbol == sentinel_symbol) &&
		 (confusion_symbol != sentinel_symbol))
	    /* means special symbol follows */
	    special_symbol = confusion_symbol;
	else if ((previous_symbol == sentinel_symbol) ||
		 (confusion_symbol != sentinel_symbol))
	  {
	    codelength += extendPathSymbol
	        (transform_model, model, context, source_symbol, confusion_symbol,
		 special_symbol, input_position);
	    special_symbol = NIL;
	  }

	if ((previous_symbol == sentinel_symbol) &&
	    (confusion_symbol == sentinel_symbol))
	    previous_symbol = NIL;
	else
	    previous_symbol = confusion_symbol;
    }

    if (Debug.level1 > 4)
    {
	fprintf (stderr, "tcodelength = %7.3f confusion codelength = %7.3f", tcodelength, confusion_codelength);
    }
    tcodelength += codelength + confusion_codelength;
    if (Debug.level1 > 4)
    {
	fprintf (stderr, " confusion text = ");
	dumpConfusion (confusion_text);
	fprintf (stderr, " codelength = %6.3f tcodelength = %7.3f\n",
		 codelength, tcodelength);
    }

    if (context == NIL)
      /* NIL context means a single context transform is being used */
        context_position = 0;
    else
        context_position = TLM_getcontext_position (model, context);

    if ((Transforms [transform_model].Transform_algorithm == TTM_Viterbi) ||
	((Transforms [transform_model].Transform_algorithm == TTM_Stack) &&
	(Transforms [transform_model].Transform_stack_type == TTM_Stack_type0)))
        hashp = addHashp
	    (transform_model, model, input_position, context_position,
	     tcodelength, codelength, &update, &added);
    else
      {
	hashp = NULL;
        update = TRUE;
	added = TRUE;
      }

    if (Debug.level1 > 4)
      fprintf (stderr, "ExtendPath: model %d context %d context_pos %d update %d added %d\n",
	       model, context, context_position, update, added);

    if (!update)
    { /* Release the copy of the context since it will not be used */
        if (context != NIL)
	    TLM_release_context (model, context);
    }
    else
    {
	newpath = path;
	p = 0;
	while (TXT_get_symbol (confusion_text, p++, &confusion_symbol)) /* add a path for each confusion_symbol */
	    newpath = addPath (newpath, model, confusion_symbol);

	newleaf = addLeaf (transform_model, newpath, source_pos+context_length, model, context,
			   confusion_symbol, tcodelength);
	if (!added)
	  { /* replaced existing leaf - remove it */
	    oldleaf = hashp->H_leaf;
	    if (oldleaf != NULL)
	      { /* Prune out old leaf (and old path if possible) */
		prunePath (oldleaf->L_path);
		Transforms [transform_model].Transform_leaves =
		    pruneLeaf (Transforms [transform_model].Transform_leaves, oldleaf);
	      }
	  }
	if (hashp != NULL)
	    hashp->H_leaf = newleaf;
    }

    return (update);
}

unsigned int ConfusionText = NIL;
unsigned int RangeText = NIL;

void
extendConfusionList (unsigned int transform_model, struct leafType *leaf,
		     struct confusionListType *confusion_list,
		     unsigned int source_text, unsigned int source_pos,
		     unsigned int source_symbol, unsigned int context_length)
/* Extend paths for the leaf using all the possible confusions defined by the
   confusion_list. */
{
    void (*confusion_function) (unsigned int, unsigned int, unsigned int,
				unsigned int, unsigned int, unsigned int);
    unsigned int p, new_model, old_model, sentinel_symbol;
    unsigned int confusion_symbol, previous_symbol, confusion_type;
    unsigned int insert_range_pos, range_pos, range_length, range_symbol;
    unsigned int range_text;
    boolean extended;

    /*fprintf (stderr, "Context length %d\n", context_length);*/

    if (ConfusionText == NIL)
        ConfusionText = TXT_create_text ();

    new_model = NIL;
    sentinel_symbol = TXT_sentinel_symbol ();
    while (confusion_list != NULL)
      {
	range_text = NIL;
	insert_range_pos = 0;

	/* Create the confusion text */
	TXT_setlength_text (ConfusionText, 0);
	p = 0;
	while (TXT_get_symbol (confusion_list->Confusion_type, p, &confusion_type) &&
	       TXT_get_symbol (confusion_list->Confusion, p, &confusion_symbol))
	  {
	    switch (confusion_type)
	      {
	      case TRANSFORM_SYMBOL_TYPE:
		TXT_append_symbol (ConfusionText, confusion_symbol);
		break;
	      case TRANSFORM_MODEL_TYPE:
		TXT_append_symbol (ConfusionText, sentinel_symbol);
		TXT_append_symbol (ConfusionText, MODEL_SYMBOL);
		/* This flags that the next confusion symbol is a model
		   number */
		new_model = confusion_symbol;
		TXT_append_symbol (ConfusionText, new_model);
		break;
	      case TRANSFORM_BOOLEAN_TYPE:
		TXT_append_symbol (ConfusionText, source_symbol);
		break;
	      case TRANSFORM_FUNCTION_TYPE:
		confusion_function =
		  (void (*) (unsigned int, unsigned int, unsigned int, unsigned int,
			     unsigned int, unsigned int))
		  (size_t) confusion_symbol;
		if (RangeText == NIL)
		    RangeText = TXT_create_text (); /* keep on re-using the same range text */
		range_text = RangeText;
		getLeaf (leaf, &old_model, &previous_symbol);
		if (new_model == NIL)
		    new_model = old_model;
		assert (new_model != NIL);

		/* Generate a range of symbols from the confusion function: */
		confusion_function (new_model, source_symbol, previous_symbol,
				    source_text, source_pos, range_text); 
							    
		TXT_append_symbol (ConfusionText, 0); /* Insert a dummy symbol which will be replaced later */
		insert_range_pos = TXT_length_text (ConfusionText) - 1;
		break;
	      case TRANSFORM_WILDCARD_TYPE:
		TXT_append_symbol (ConfusionText, source_symbol);
		break;
	      case TRANSFORM_RANGE_TYPE:
		if (range_text == NIL)
		  {
		    TXT_get_symbol (confusion_list->Confusion, p, &range_text);
		    TXT_append_symbol (ConfusionText, 0); /* Insert a dummy symbol which will be replaced later */
		    insert_range_pos = TXT_length_text (ConfusionText) - 1;
		  }
		break;
	      case TRANSFORM_SENTINEL_TYPE:
		/* Need to append the sentinel symbol twice to indicate
		   a single sentinel symbol occurs */
		TXT_append_symbol (ConfusionText, sentinel_symbol);
		TXT_append_symbol (ConfusionText, sentinel_symbol);
		break;
	      case TRANSFORM_GHOST_TYPE:
		TXT_append_symbol (ConfusionText, sentinel_symbol);
		TXT_append_symbol (ConfusionText, GHOST_SYMBOL);
		/* This flags that a "ghost" operation should be carried
		   out on the next source symbol */
		break;
	      case TRANSFORM_SUSPEND_TYPE:
		TXT_append_symbol (ConfusionText, sentinel_symbol);
		TXT_append_symbol (ConfusionText, SUSPEND_SYMBOL);
		/* This flags that a "suspend" operation should be carried
		   out on the next source symbol */
		break;
	      default:
		break;
	      }
	    p++;
	  }
	range_pos = 0;
	if (range_text == NIL)
	    range_length = 0;
	else
	    range_length = TXT_length_text (range_text);

	for (;;)
	  { /* generate a new confusion path (for each symbol in the range if a range exists) */
	    if (range_text != NIL)
	      {
		if (range_pos >= range_length)
		    break;
		TXT_get_symbol (range_text, range_pos, &range_symbol);
		TXT_put_symbol (ConfusionText, range_symbol, insert_range_pos);
	      }

	    if (Debug.level1 > 4)
	      {
		fprintf (stderr, "Confusing context symbol ");
		if (Transform_dump_confusion_function == NULL)
		    TXT_dump_symbol1 (Stderr_File, source_symbol);
		else
		    Transform_dump_symbol_function (Stderr_File, source_symbol);
		fprintf (stderr, " length %d", context_length);
		if (context_length > 1)
		  {
		    fprintf (stderr, " context [");
		    TXT_dump_text2 (Stderr_File, source_text, source_pos, context_length,
				    TXT_dump_symbol);
		    fprintf (stderr, "]");
		  }

		fprintf (stderr, " with ");

		dumpConfusion (ConfusionText);
	      }

	    extended =
	        extendPath (transform_model, leaf, source_pos, source_symbol,
			    context_length, ConfusionText, confusion_list->Codelength);
	    if (Debug.level1 > 4)
	      {
		fprintf (stderr, "Source symbol ");
		if (Transform_dump_confusion_function == NULL)
		    TXT_dump_symbol1 (Stderr_File, source_symbol);
		else
		    Transform_dump_symbol_function (Stderr_File, source_symbol);
		fprintf (stderr, " extended: ");
		if (extended)
		    fprintf (stderr, "TRUE\n");
		else
		    fprintf (stderr, "FALSE\n");
	      }


	    if (range_text == NIL)
	        break;
	    else
	        range_pos++;
	  }

	confusion_list = confusion_list->Cnext;
      }
}

void
extendConfusionTrie (unsigned int transform_model, struct leafType *leaf,
		     struct confusionTrieType *confusion_node,
		     unsigned int source_text, unsigned int source_pos,
		     unsigned int source_symbol)
/* Extends the transform paths for the confusion node for the current source
   text at position source_pos. */
{
    struct pathType *path;
    struct confusionTrieType *here;
    unsigned int model, previous_symbol;

    assert (leaf != NULL);
    path = leaf->L_path;
    assert (path != NULL);

    here = confusion_node;
    for (;;)
      { 
	if (here == NULL)
	    break;
	getLeaf (leaf, &model, &previous_symbol);
	if (matchConfusionSymbol (model, source_symbol, previous_symbol,
				  source_text, source_pos,
				  here->Csymbol, here->Ctype))
	  {
	    extendConfusionList (transform_model, leaf, here->Confusions,
				 source_text, source_pos, source_symbol,
				 TXT_length_text (here->Context));
	    extendConfusionTrie (transform_model, leaf, here->Cdown,
				 source_text, source_pos, source_symbol);
	  }
	if ((here->Ctype == TRANSFORM_SYMBOL_TYPE) &&
	    (source_symbol < here->Csymbol))
	    break; /* The list is sorted with normal symbols appearing
		      at the end in lexicographical order, so we can
		      break out if we reach them and the symbol being
		      matched is less than the one in the list */
	here = here->Cnext;
      }
}

void
extendPaths (unsigned int transform_model, struct leafType *leaf,
	     struct confusionTrieType *confusions,
	     unsigned int source_text, unsigned int source_pos)
/* Extend the transform paths for the leaf using all the possible confusions
   defined by the transform model. */
{
    unsigned int source_symbol;

    assert (TXT_get_symbol (source_text, source_pos, &source_symbol));

    /* Extend the confusions for the null context */
    extendConfusionList (transform_model, leaf, confusions->Confusions,
			 source_text, source_pos, source_symbol, 1);

    /* Next, extend the confusions for the matching future contexts  */
    extendConfusionTrie (transform_model, leaf, confusions->Cdown,
			 source_text, source_pos, source_symbol);
}

struct leafType *
updateLeaf (unsigned int transform_model, struct leafType *leaf,
	    struct confusionTrieType *confusions, unsigned int source_text,
	    unsigned int source_pos)
/* Updates the transform paths for the current leaf. */
{
    struct leafType *leaves = NULL;

    if (Debug.level1 > 3)
        fprintf (stderr, "Updating leaf at pos %d Lpos %d codelength %.3f\n",
		 source_pos, leaf->L_pos, leaf->L_codelength);

    if (source_pos == leaf->L_pos)
      { /* Only extend the path if the leaf's position matches the source position.
	   A mis-match occurs when multiple characters are matched for the source
	   text in a confusion, and the subsequent text needs to be skipped over. */
	extendPaths (transform_model, leaf, confusions, source_text, leaf->L_pos);
	prunePath (leaf->L_path); /* Prune out the path for the leaf if possible */

	/* Also prune the leaf that was just extended from the list */
	leaves = Transforms [transform_model].Transform_leaves;
	leaves = pruneLeaf (leaves, leaf);
	Transforms [transform_model].Transform_leaves = leaves;
      }

    return (leaves);
}

void
updatePaths (unsigned int transform_model, struct confusionTrieType *confusions,
	     unsigned int source_text, unsigned int source_pos)
/* Update the paths trie structure using the incoming symbols in the source text at
   position pos. */
{
    struct leafType *leaf, *nextleaf, *leaves;

    assert (TTM_valid_transform (transform_model));

    if (source_pos >= TXT_length_text (source_text))
        return; /* we have reached the end of the text */

    initHashp (transform_model);

    if (Debug.level1 > 1)
	fprintf (stderr, "\n\nUpdating paths for source position %d\n", source_pos);
    /*
    if (source_pos == 250)
      {
	Debug.level1 = 6;
	debug_paths ();
      }
    */

    if (Transforms [transform_model].Transform_algorithm == TTM_Viterbi)
      { /* Viterbi algorithm - extend all leaves */
	leaf = Transforms [transform_model].Transform_leaves; /* start at the head of the leaves list */
	while (leaf != NULL)
	  {
	    /* note: new leaves for the Viterbi algorithm will keep on being
	       added at the head, so will not affect the operation of this
	       loop */

	    nextleaf = leaf->L_next; /* save pointer to next leaf */
	    updateLeaf (transform_model, leaf, confusions, source_text,
			source_pos);
	    leaf = nextleaf;
	  }
      }
    else
      {	/* Stack algorithm - prune out all the leaves and then update any
	   potentially good leaves (which have a better codelength than the
	   current best leaf). */

	/* Prune out the old leaves first: */
	leaves = pruneLeaves (transform_model, source_pos);
	for (;;)
	  { /* The head of the leaves list contains the leaf pointing
	       to the best codelength pathway. If this isn't up-to-date
	       (i.e. its position is less than the current position)
	       Then extend it by one position, and repeat process until a
	       minimum codelength leaf is found which is up-to-date. */
	    if (leaves == NULL)
	      break;

	    /* update the head leaf only */
	    leaves = updateLeaf (transform_model, leaves, confusions,
				 source_text, source_pos);

	    if (!leaves || leaves->L_pos > source_pos)
	      break; /* best leaf occurs at current position;
			i.e. there are no more potentially good leaves
			that need to be updated to current position */
	  }
      }


    if (Debug.level1 > 3)
        dumpLeaves (Stderr_File, transform_model);
}

unsigned int
findBestPath (unsigned int transform_model)
/* Returns a pointer to a text record that contains the best path through the paths trie. */
{
    struct pathType *s, *path, *minpath;
    struct leafType *leaf;
    float codelength, mincodelength;
    unsigned int transform_text, symbol, output;  /* Corrected output text that is returned */
    int p;

    mincodelength = 9999.0; /* this is to prevent a pedantic error message; it gets reset
			       first time through anyway */

    transform_text = Transforms [transform_model].Transform_text;
    TXT_setlength_text (transform_text, 0); /* Resets the start of the output text. */

    /* Find path with the minimum codelength */
    minpath = NULL;
    leaf = Transforms [transform_model].Transform_leaves;
    while (leaf != NULL)
    {
	path = leaf->L_path;
	codelength = leaf->L_codelength;
	if ((minpath == NULL) || (codelength < mincodelength))
	{
	    minpath = path;
	    mincodelength = codelength;
	}
	leaf = leaf->L_next;
    }
    if (Debug.level1 > 0)
	fprintf (stderr, "Min codelength = %7.3f\n", mincodelength );

    /* Traverse back up the trie */
    s = minpath;
    while (s != NULL)
    {
	TXT_append_symbol (transform_text, s->P_symbol);
	s = s->P_parent;
    }

    /* Reverse the symbols to reconstruct the original text */
    output = TXT_create_text ();
    for (p=TXT_length_text (transform_text)-2; p>=0; p--)
      {
	TXT_get_symbol (transform_text, p, &symbol);
	TXT_append_symbol (output, symbol);
      }

    return (output);
} /* findBestPath */

unsigned int
transformPaths (unsigned int transform_model, unsigned int source_text,
	     void (*dump_symbol_function) (unsigned int, unsigned int),
	     void (*dump_symbols_function) (unsigned int, unsigned int),
	     void (*dump_confusion_function) (unsigned int, unsigned int))
/* Creates and returns an unsigned integer which provides a reference to a text record
   that contains the input source text corrected according to the transform model. */
{
    unsigned int source_pos, output_text, text_length;

    assert (TTM_valid_transform (transform_model));

    /* Check whether the transform paths and leaves have been initialized by TTM_start_transform () */
    assert (Transforms [transform_model].Transform_paths != NULL);
    assert (Transforms [transform_model].Transform_leaves != NULL);

    text_length = TXT_length_text (source_text);

    source_pos = 0;
    while (source_pos < text_length)
    {
	if ((Debug.progress) && ((source_pos % Debug.progress) == 0))
	{
	    fprintf (stderr, " position %4d... ", source_pos);
	    printPathsMalloc (Stderr_File);
	    /*dump_memory (Stderr_File);*/
	}

	updatePaths (transform_model, Transforms [transform_model].Transform,
		     source_text, source_pos);

	if (Debug.level1 > 1)
	{
	    unsigned int dump_best;

	    fprintf (stderr, "Best path so far up to position %d:\n", source_pos);
	    dump_best = findBestPath (transform_model);

	    if (dump_symbols_function == NULL)
	        TXT_dump_text (Stderr_File, dump_best, NULL);
	    else
	        dump_symbols_function (Stderr_File, dump_best);

	    TXT_release_text (dump_best);
	    fprintf (stderr, "\n");
	}

	if (Debug.level1 > 2)
	  {
	    dumpPaths (Stderr_File, transform_model);
	    dumpHashp (Stderr_File, transform_model);
	    dumpHashm (Stderr_File, transform_model);
	  }
	/*
	if (Debug.level1 > 3)
	{
	    dumpLeaves (Stderr_File, transform_model);
	}
	*/
	source_pos++;
    }
    if (Debug.progress)
    {
	fprintf (stderr, "%4d... ", source_pos);
	printPathsMalloc (Stderr_File);
    }
    output_text = findBestPath (transform_model);

    return (output_text);
}
