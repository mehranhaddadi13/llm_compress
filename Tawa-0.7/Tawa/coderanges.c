/* Routines for keeping track of a sequence of probabilities
   for a particular encoding. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "io.h"
#include "model.h"

#define CODERANGE_SIZE 512 /* Initial max. number of coderange records */

struct coderangeType
{ /* Arithmetic coding range record */
    unsigned int R_lbnd; /* Lower bound to arithmetic coding range */
    unsigned int R_hbnd; /* Upper bound to arithmetic coding range */
    unsigned int R_totl; /* Total of arithmetic doing range */
    unsigned int R_next; /* The next in the doubly linked list of ranges */
    unsigned int R_prev; /* The previous in the doubly linked list of ranges */
};

/* Redefine fields for first record only in the list of coderange records */
#define R_head R_lbnd /* R_lbnd is also used to store head of coderange list */
#define R_tail R_hbnd /* R_hbnd is also used to store tail of coderange list */
#define R_pos  R_totl /* R_totl is also used to store current coderange pos */

struct coderangeType *Coderanges = NULL; /* Linked lists of arithmetic coding ranges*/
unsigned int Coderanges_max_size = 0;/* Current max. size of the coderange array */
unsigned int Coderanges_used = NIL;  /* List of deleted coderange records */
unsigned int Coderanges_unused = NIL;/* Next unused coderange record */

unsigned int
create_coderange (void)
{
    unsigned int coderange;

    if (Coderanges_used != NIL)
    {	/* use the first record on the used list */
	coderange = Coderanges_used;
	Coderanges_used = Coderanges [coderange].R_next;
    }
    else
    {
	if (Coderanges_unused+1 >= Coderanges_max_size)
	{ /* need to extend Coderanges array */
	    if (Coderanges_max_size == 0)
		Coderanges_max_size = CODERANGE_SIZE;
	    else
		Coderanges_max_size = 10*(Coderanges_max_size+50)/9;

	    Coderanges = (struct coderangeType *) realloc (Coderanges, Coderanges_max_size * sizeof (struct coderangeType));

	    if (Coderanges == NULL)
	    {
		fprintf (stderr, "Fatal error: out of coderange record space\n");
		exit (1);
	    }
	}
	Coderanges_unused++;
	coderange = Coderanges_unused;
    }

    if (coderange != NIL)
    {
	Coderanges [coderange].R_lbnd = 0;
	Coderanges [coderange].R_hbnd = 0;
	Coderanges [coderange].R_totl = 0;
	Coderanges [coderange].R_next = NIL;
	Coderanges [coderange].R_prev = NIL;
    }
    return (coderange);
}

void
TLM_append_coderange (unsigned int coderanges, unsigned int lbnd,
		      unsigned int hbnd, unsigned int totl)
/* Append a new coderange record onto the tail of the coderange list */
{
    unsigned int new_coderange, rhead, rtail;

    /* Check for valid range */
    assert (lbnd < hbnd);
    assert (hbnd <= totl);

    assert (coderanges != NIL);
    rhead = Coderanges [coderanges].R_head;
    rtail = Coderanges [coderanges].R_tail;

    new_coderange = create_coderange ();
    assert (new_coderange != NIL);

    Coderanges [new_coderange].R_lbnd = lbnd;
    Coderanges [new_coderange].R_hbnd = hbnd;
    Coderanges [new_coderange].R_totl = totl;

    if (rtail == NIL)
	rhead = new_coderange; /* list is empty - create it */
    else
    {
	Coderanges [rtail].R_next = new_coderange;
	Coderanges [new_coderange].R_prev = rtail;
    }
    rtail = new_coderange;

    Coderanges [coderanges].R_head = rhead;
    Coderanges [coderanges].R_tail = rtail;
}

void
TLM_overwrite_coderange (unsigned int coderanges, unsigned int lbnd,
			 unsigned int hbnd, unsigned int totl)
/* Overwrite the record at the head of the coderange list */
{
    unsigned int rhead;

    /* Check for valid range */
    assert (lbnd < hbnd);
    assert (hbnd <= totl);

    assert (coderanges != NIL);
    rhead = Coderanges [coderanges].R_head;
    assert (rhead != NIL);

    Coderanges [rhead].R_lbnd = lbnd;
    Coderanges [rhead].R_hbnd = hbnd;
    Coderanges [rhead].R_totl = totl;
}

void
TLM_release_coderanges (unsigned int coderanges)
/* Release the coderange list to the used list */
{
    unsigned int rhead, rtail;

    if (coderanges == NIL)
        return;

    rhead = Coderanges [coderanges].R_head;
    rtail = Coderanges [coderanges].R_tail;
    /* add at the head of the Coderanges_used list */
    if (rtail != NIL)
    {
	assert (rhead != NIL);

	Coderanges [rtail].R_next = Coderanges_used;
	Coderanges_used = rhead;
    }

    Coderanges [coderanges].R_head = NIL;
    Coderanges [coderanges].R_tail = NIL;
    Coderanges [coderanges].R_pos = NIL;
}

unsigned int
TLM_create_coderanges (void)
/* Return a new pointer to a coderange record */
{
    return (create_coderange ());
}

void
TLM_reset_coderanges (unsigned int coderanges)
/* Resets the position in the list of coderanges associated with the current symbol.
   The next call to TLM_next_coderange will return the first coderanges on the list. */
{
    assert (coderanges != NIL);

    Coderanges [coderanges].R_pos = Coderanges [coderanges].R_head;
}

boolean
TLM_next_coderange (unsigned int coderanges, unsigned int *lbnd,
		    unsigned int *hbnd, unsigned int *totl)
/* Places the current coderange in lbnd, hbnd and totl
   (see documentation for more complete description). Update the coderange record so
   that the current coderange becomes the next coderange in the list of
   coderange associated with the current symbol. If there are no more
   coderange in the list then return NIL otherwise some non-NIL value. */
{
    unsigned int p;

    assert (coderanges != NIL);

    p = Coderanges [coderanges].R_pos;

    if (p == NIL)
      { /* end of list */
	*lbnd = 0;
	*hbnd = 0;
	*totl = 0;
      }
    else
      {
	*lbnd = Coderanges [p].R_lbnd;
	*hbnd = Coderanges [p].R_hbnd;
	*totl = Coderanges [p].R_totl;
        Coderanges [coderanges].R_pos = Coderanges [p].R_next;
      }

    return (p);
}

unsigned int
TLM_length_coderanges (unsigned int coderanges)
/* Returns the code length of the coderanges list. */
{
    unsigned int len, p;

    assert (coderanges != NIL);

    len = 0;

    p = Coderanges [coderanges].R_head;
    while (p)
    {
      len++;
      p = Coderanges [p].R_next;
    }

    return (len);
}

float
TLM_codelength_coderanges (unsigned int coderanges)
/* Returns the code length of the current symbol's coderange in bits. It does this without
   altering the current symbol or the current coderange. */
{
    unsigned int p;
    float codelength;

    assert (coderanges != NIL);

    codelength = 0.0;

    p = Coderanges [coderanges].R_head;
    while (p)
    {
	codelength += Codelength (Coderanges [p].R_lbnd, Coderanges [p].R_hbnd,
				  Coderanges [p].R_totl);
	p = Coderanges [p].R_next;
    }
    return (codelength);
}

void
TLM_dump_coderanges (unsigned int file, unsigned int coderanges)
/* Prints the coderange list for the current symbol in a human readable form.
   It does this without altering the current position in the coderange. list as determined
   by the functions TLM_reset_coderanges or TLM_next_coderange. */
{
    unsigned int p;

    assert (TXT_valid_file (file));
    assert (coderanges != NIL);

    fprintf (Files [file], "Dump of coderange list :");

    p = Coderanges [coderanges].R_head;
    while (p)
    {
	fprintf (Files [file], " (%d,%d,%d)", Coderanges [p].R_lbnd,
		 Coderanges [p].R_hbnd, Coderanges [p].R_totl);
	p = Coderanges [p].R_next;
    }

    fprintf (Files [file], "\n");
}

unsigned int
TLM_copy_coderanges (unsigned int coderanges)
/* Creates a copy of the list of coderanges and returns a pointer to it. */
{
    unsigned int new_coderanges, p, pos;

    assert (coderanges != NIL);

    new_coderanges = TLM_create_coderanges ();

    p = Coderanges [coderanges].R_head;
    pos = Coderanges [coderanges].R_pos;
    while (p)
    {
        TLM_append_coderange (new_coderanges, Coderanges [p].R_lbnd,
			      Coderanges [p].R_hbnd, Coderanges [p].R_totl);
	if (p == pos)
	    Coderanges [new_coderanges].R_pos = Coderanges [new_coderanges].R_tail;
	p = Coderanges [p].R_next;
    }

    return (new_coderanges);
}

void
TLM_betail_coderanges (unsigned int coderanges)
/* Removes the tail of the coderange list. (This is useful for keeping the escape part of the list
   of coderanges intact.) */
{
    unsigned int rhead, rtail, rprev;

    if (coderanges == NIL)
        return;

    rhead = Coderanges [coderanges].R_head;
    rtail = Coderanges [coderanges].R_tail;

    if ((rhead == NIL) || (rtail == NIL))
        return;
    else if (rhead == rtail)
      { /* List contains a single record */
	Coderanges [coderanges].R_head = NIL;
	Coderanges [coderanges].R_tail = NIL;
	Coderanges [coderanges].R_pos = NIL;
      }
    else
      { /* Chop off the tail */
	rprev = Coderanges [rtail].R_prev;
	assert (rprev  != NIL);
	Coderanges [rprev].R_next = NIL;
	Coderanges [coderanges].R_tail = rprev;

	/* add the tail record at the head of the Coderanges_used list */
	Coderanges [rtail].R_next = Coderanges_used;
	Coderanges_used = rtail;
      }
}

/* Scaffolding for coderange module:

#define LN2 0.69314718055994530941

float
log_two (float x)
{
    return (log (x) / LN2);
}

float
Codelength (unsigned int lbnd, unsigned int hbnd, unsigned int totl)
{
    return (-log_two ((float) (hbnd - lbnd) / (totl)));
}

int
main ()
{
    unsigned int coderanges, new_coderanges, lbnd, hbnd, totl;
    float codelength;

    for (;;)
      {
	coderanges = TLM_create_coderanges ();
	for (;;)
	  {
	    printf ("lbnd? ");
	    scanf ("%d", &lbnd);
	    
	    printf ("hbnd? ");
	    scanf ("%d", &hbnd);
	    
	    printf ("totl? ");
	    scanf ("%d", &totl);
	    
	    if (!totl)
	      break;
	    
	    TLM_append_coderange (coderanges, lbnd, hbnd, totl);
	    
	    TLM_dump_coderanges (stdout, coderanges);
	    
	    TLM_reset_coderanges (coderanges);
	    while (TLM_next_coderange (coderanges, &lbnd, &hbnd, &totl))
	      printf ("lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	    
	    codelength = TLM_codelength_coderanges (coderanges);
	    printf ("codelength = %.3f\n", codelength);
	  }

	new_coderanges = TLM_copy_coderanges (coderanges);
	TLM_dump_coderanges (stdout, new_coderanges);
	
	printf ("Betail 1:\n");
	TLM_betail_coderanges (new_coderanges);
	TLM_dump_coderanges (stdout, new_coderanges);

	printf ("Betail 2:\n");
	TLM_betail_coderanges (new_coderanges);
	TLM_dump_coderanges (stdout, new_coderanges);

	printf ("Betail 3:\n");
	TLM_betail_coderanges (new_coderanges);
	TLM_dump_coderanges (stdout, new_coderanges);


	TLM_release_coderanges (coderanges);
	TLM_release_coderanges (new_coderanges);
      }
}
*/
