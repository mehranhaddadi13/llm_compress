/* Evaluates the effectiveness of the segmentation program by calculating
   the recall/precision between two files. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILENAME_SIZE 100

unsigned int Use_Numbers = 0;
unsigned int progress = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: segment_eval [options]\n"
	     "\n"
	     "options:\n"
	     "  -p debug progress every n numbers\n"
	     "  -g fn\tground truth (correct) text\n"
	     "  -N\ttext files are a sequence of unsigned numbers\n"
	     "  -s n\tsegmented text (output from segmentation program)\n"
	);
    exit (2);
}

int
get_symbol (FILE *fp, unsigned int *symbol)
{
    int result;

    *symbol = 0;
    if (!Use_Numbers)
      {
	result = getc (fp);
	if (result != EOF)
	    *symbol = result;
      }
    else
      {
        result = fscanf (fp, "%u", symbol);
	switch (result)
	  {
	  case 1: /* one number read successfully */
	    break;
	  case EOF: /* eof found */
	    break;
	  case 0:
	    fprintf (stderr, "Formatting error in file\n");
	    break;
	  default:
	    fprintf (stderr, "Unknown error (%i) reading file\n", result);
	    exit (1);
	  }
      }

    return (result);
}

int
get_symbols (FILE *fp, unsigned int skip_spaces,
	     unsigned int *symbol, unsigned int *count)
{
    int result;

    for (;;)
      {
	result = get_symbol (fp, symbol);
	if (result == EOF)
	    break;
	if (!skip_spaces || (*symbol != ' '))
	    break;
      }

    /*
    fprintf (stderr, "symbol = %d\n", *symbol);
    */

    if (!skip_spaces && (*symbol == ' '))
        (*count)++;

    return (result);
}

void
process_files (FILE *gfp, FILE *sfp)
{
    int gres, sres;
    unsigned int gsym, ssym, gcount, scount, gcount1, scount1, cnt;

    cnt = 0;
    gcount = 0;
    scount = 0;
    gcount1 = 0;
    scount1 = 0;
    gres = get_symbols (gfp, 1, &gsym, &gcount);
    sres = get_symbols (sfp, 1, &ssym, &scount);
    for (;;)
      {
	cnt++;
	if ((progress != 0) && ((cnt % progress) == 0))
	    fprintf (stderr, "Processed %d steps (%d,%d)\n", cnt, gsym, ssym);

	/*
	fprintf (stderr, "gres = %d sres = %d\n", gres, sres);
	fprintf (stderr, "gsym = %d ssym = %d\n", gsym, ssym);
	fprintf (stderr, "gcount %d gcount1 %d scount %d scount1 %d\n",
		 gcount, gcount1, scount, scount1);
	*/

	if (gsym == ssym)
	  {
	    if (gsym == ' ')
	      {
		gcount1++;
		scount1++;
	      }
	    if (gres == EOF)
	        break;

	    gres = get_symbols (gfp, (gsym == ' '), &gsym, &gcount);
	    sres = get_symbols (sfp, (ssym == ' '), &ssym, &scount);
	  }
	else if (gsym == ' ')
	  {
	    gres = get_symbols (gfp, 1, &gsym, &gcount);
	  }
	else if (ssym == ' ')
	  {
	    sres = get_symbols (sfp, 1, &ssym, &scount);
	  }
	else
	  {
	    /*
	    fprintf (stderr, "gsym = %d ssym = %d\n", gsym, ssym);
	    fprintf (stderr, "gres = %d sres = %d\n", gres, sres);
	    */
	    assert ((gres == EOF) || (sres == EOF));
	  }
      }

    fprintf (stderr, "gcount %d gcount1 %d scount %d scount1 %d\n",
	     gcount, gcount1, scount, scount1);
    fprintf (stderr, "recall %.2f precision %.2f\n",
	     100 * (float) gcount1 / gcount,
	     100 * (float) scount1 / scount);

}

int
main (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    char gfilename [FILENAME_SIZE];
    char sfilename [FILENAME_SIZE];
    FILE *gfp, *sfp;

    int opt;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "g:Np:s:")) != -1)
	switch (opt)
	{
	case 'g':
	  strcpy (gfilename, optarg);
	  gfp = fopen (gfilename, "r");
	  if (gfp == NULL)
	    {
	      fprintf (stderr, "cannot open file %s\n", gfilename);
	      exit (1);
	    }
	  break;
	case 'N':
	  Use_Numbers = 1;
	  break;
	case 'p':
	    progress = atoi (optarg);
	    break;
	case 's':
	  strcpy (sfilename, optarg);
	  sfp = fopen (sfilename, "r");
	  if (sfp == NULL)
	    {
	      fprintf (stderr, "cannot open file %s\n", sfilename);
	      exit (1);
	    }
	  break;
	default:
	    usage ();
	    break;
	}

    for (; optind < argc; optind++)
	usage ();

    process_files (gfp, sfp);

    exit (0);
}

