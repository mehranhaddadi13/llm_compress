/* Works out similarity metrics for a file. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
#define MAX_TAG 64        /* Maximum length of a tag string */
#define MAX_FILENAME 256  /* Maximum length of a filename */
#define MAX_MODELS 10000  /* Maximum number of models */

int Args_max_order;                 /* Maximum order of the models */
unsigned int Args_alphabet_size;    /* The models' alphabet size */
boolean Args_performs_full_excls;   /* Performs full exclusions or not */
boolean Args_performs_update_excls; /* Performs update exclusions or not */
unsigned int Args_escape_method;    /* The model's escape method e.g. C or D */

unsigned int Collection_model = NIL;

unsigned int Topic_models [MAX_MODELS];
unsigned int Topic_models_count = 0;

float Cross_Entropy;
float Divergence1, Divergence2, Radius;
float Divergence3, Divergence4, Radius1;
float Min_Cross_Entropy = 0.0;
float Min_Divergence1 = 0.0, Min_Divergence2 = 0.0, Min_Radius = 0.0;
float Min_Divergence3 = 0.0, Min_Divergence4 = 0.0, Min_Radius1 = 0.0;
unsigned int Pos_Cross_Entropy = 1;
unsigned int Pos_Divergence1 = 0, Pos_Divergence2 = 0, Pos_Radius = 0;
unsigned int Pos_Divergence3 = 0, Pos_Divergence4 = 0, Pos_Radius1 = 0;

FILE *Testfiles_fp;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: radius_file [options]\n"
	     "\n"
	     "options:\n"
	     "  -a n\tsize of alphabet for document model=n\n"
	     "  -e n\tescape method of document model=n\n"
	     "  -o n\tmax order of document model=n\n"
	     "  -c fn\tcollection-model-filename\n"
	     "  -m fn\tmodel-filename\n"
	     "  -t fn\ttest-list-of-filenames\n"
	     "\n"
	     );
    exit (2);
}

float absolute (float value)
{
    if (value < 0)
        return (- value);
    else
        return (value);
}

void
loadModels (FILE *fp)
/* Load the models from file FP. */
{
    char tag [MAX_TAG], model_filename [MAX_FILENAME];
    unsigned int model;

    while ((fscanf (fp, "%s %s", tag, model_filename) != EOF))
    {
        model = TLM_read_model (model_filename,
				"Loading training model from file",
				"Classify_file: can't open training file");

	TLM_set_tag (model, tag);
	Topic_models [Topic_models_count] = model;
	Topic_models_count++;
    }
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int models_found;
    unsigned int testfiles_found;
    int escape;
    FILE *fp;
    int opt;

    Args_alphabet_size = MAX_ALPHABET;
    Args_max_order = MAX_ORDER;
    Args_escape_method = TLM_PPM_Method_D;
    Args_performs_full_excls = TRUE;
    Args_performs_update_excls = TRUE;

    /* get the argument options */
    models_found = 0;
    testfiles_found = 0;
    while ((opt = getopt (argc, argv, "a:e:o:c:m:t:")) != -1)
	switch (opt)
	{
        case 'a':
	    Args_alphabet_size = atoi (optarg);
	    break;
        case 'e':
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Args_escape_method = escape;
	    break;
	case 'o':
	    Args_max_order = atoi (optarg);
	    break;
	case 'c':
	    Collection_model = TLM_read_model
	        (optarg, "Loading collection model from file",
		 "Classify_file: can't open collection file");
	    models_found = 1;
	    break;
	case 'm':
	    fprintf (stderr, "Loading models from file %s\n",
		    optarg);
	    if ((fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open models file %s\n",
			 optarg);
		exit (1);
	    }
	    loadModels (fp);
	    models_found = 1;
	    break;
	case 't':
	    fprintf (stderr, "Loading test files from file %s\n",
		    optarg);
	    if ((Testfiles_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open testfiles file %s\n",
			 optarg);
		exit (1);
	    }
	    testfiles_found = 1;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found || !testfiles_found)
    {
        fprintf (stderr, "\nFatal error: missing models or test files\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
      usage ();
}

int
getline( FILE *fp, char *s, int max )
/* Read line from FP into S; return its length (maximum length = MAX). */
{
    int i;
    int cc;

    i = 0;
    cc = '\0';
    while ((--max > 0) && ((cc = getc(fp)) != EOF) && (cc != '\n'))
        s [i++] = cc;
    s [i] = 0;
    if (cc == EOF)
        return (EOF);
    else
        return( i );
}

void
classifyText (unsigned int text, unsigned int topic_model,
	      unsigned int collection_model)
/* Classifies the text using the models. */
{
    unsigned int document_model, pos, symbol;
    unsigned int document_context, topic_context, collection_context;
    float document_codelength, topic_codelength, collection_codelength;

    Cross_Entropy = 0.0;
    Divergence1 = 0.0;
    Divergence2 = 0.0;
    Radius = 0.0;

    Divergence3 = 0.0;
    Divergence4 = 0.0;
    Radius1 = 0.0;

    document_model = TLM_create_model
      (TLM_PPM_Model, "Test", Args_alphabet_size, Args_max_order,
       Args_escape_method, Args_performs_full_excls, Args_performs_update_excls);

    TLM_set_context_type (TLM_Get_Codelength);

    document_context = TLM_create_context (document_model);
    topic_context = TLM_create_context (topic_model);
    collection_context = TLM_create_context (collection_model);

    /* Now compute metrics for each symbol */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
	TLM_update_context (document_model, document_context, symbol);
	document_codelength = TLM_Codelength;
	TLM_update_context (topic_model, topic_context, symbol);
	topic_codelength = TLM_Codelength;
	TLM_update_context (collection_model, collection_context, symbol);
	collection_codelength = TLM_Codelength;
	/*fprintf (stderr, "doc %.3f topic %.3f coll %.3f\n",
	  document_codelength, topic_codelength, collection_codelength);*/

	Cross_Entropy += topic_codelength;

	Divergence1 += absolute (document_codelength - topic_codelength);
	Divergence2 += absolute (topic_codelength - collection_codelength);
	Radius += absolute (document_codelength + topic_codelength -
			    2 * collection_codelength);

	Divergence3 += document_codelength - topic_codelength;
	Divergence4 += topic_codelength - collection_codelength;
	Radius1 += (document_codelength + topic_codelength -
		    2 * collection_codelength);
    }

    TLM_release_context (document_model, document_context);
    TLM_release_context (topic_model, topic_context);
    TLM_release_context (collection_model, collection_context);

    TLM_release_model (document_model);
}

void
classifyTestfiles (void)
/* Classify the test files. */
{
    char test_filename [256];
    unsigned int file, test_text, m;
    int len;

    while ((len = getline (Testfiles_fp, test_filename, 256)) != EOF)
    {
        file = TXT_open_file (test_filename, "r", NULL,
			      "Classify_file: can't open testing file");
	test_text = TXT_load_text (file);
	TXT_close_file (file);

	Min_Cross_Entropy = 0.0;
	Pos_Cross_Entropy = 0;
	Min_Divergence1 = 0.0;
	Pos_Divergence1 = 0;
	Min_Divergence2 = 0.0;
	Pos_Divergence2 = 0;
	Min_Radius = 0.0;
	Pos_Radius = 0;

	Min_Divergence3 = 0.0;
	Pos_Divergence3 = 0;
	Min_Divergence4 = 0.0;
	Pos_Divergence4 = 0;
	Min_Radius1 = 0.0;
	Pos_Radius1 = 0;

	fprintf (stderr, "\nClassifying test file %s:\n", test_filename);
	for (m = 0; m < Topic_models_count; m++)
	  {
	    classifyText (test_text, Topic_models [m], Collection_model);
	    fprintf (stderr, "%-30s: cross %9.3f div1 %9.3f div2 %9.3f rad %9.3f div3 %9.3f div4 %9.3f rad1 %9.3f\n",
		     TLM_get_tag (Topic_models [m]), Cross_Entropy,
		     Divergence1, Divergence2, Radius,
		     Divergence3, Divergence4, Radius1);
	    if (!m)
	      {
		Min_Cross_Entropy = Cross_Entropy;

		Min_Divergence1 = Divergence1;
		Min_Divergence2 = Divergence2;
		Min_Radius = Radius;

		Min_Divergence3 = Divergence3;
		Min_Divergence4 = Divergence4;
		Min_Radius1 = Radius1;
	      }
	    else
	    {
	      if (Cross_Entropy < Min_Cross_Entropy)
		{
		  Pos_Cross_Entropy = m;
		  Min_Cross_Entropy = Cross_Entropy;
		}

	      if (Divergence1 < Min_Divergence1)
		{
		  Pos_Divergence1 = m;
		  Min_Divergence1 = Divergence1;
		}
	      if (Divergence2 < Min_Divergence2)
		{
		  Pos_Divergence2 = m;
		  Min_Divergence2 = Divergence2;
		}
	      if (Radius < Min_Radius)
		{
		  Pos_Radius = m;
		  Min_Radius = Radius;
		}

	      if (Divergence3 < Min_Divergence3)
		{
		  Pos_Divergence3 = m;
		  Min_Divergence3 = Divergence3;
		}
	      if (Divergence4 < Min_Divergence4)
		{
		  Pos_Divergence4 = m;
		  Min_Divergence4 = Divergence4;
		}
	      if (Radius1 < Min_Radius1)
		{
		  Pos_Radius1 = m;
		  Min_Radius1 = Radius1;
		}
	    }
	  }

	fprintf (stderr, "Smallest cross_entropy for %s = %.3f\n",
		 TLM_get_tag (Topic_models [Pos_Cross_Entropy]),
		 Min_Cross_Entropy);
	fprintf (stderr, "Smallest divergence 1 for %s = %.3f\n",
		 TLM_get_tag (Topic_models [Pos_Divergence1]),
		 Min_Divergence1);
	fprintf (stderr, "Smallest divergence 2 for %s = %.3f\n",
		 TLM_get_tag (Topic_models [Pos_Divergence2]),
		 Min_Divergence2);
	fprintf (stderr, "Smallest radius for %s = %.3f\n",
		 TLM_get_tag (Topic_models [Pos_Radius]),
		 Min_Radius);

	fprintf (stderr, "Smallest divergence 3 for %s = %.3f\n",
		 TLM_get_tag (Topic_models [Pos_Divergence3]),
		 Min_Divergence3);
	fprintf (stderr, "Smallest divergence 4 for %s = %.3f\n",
		 TLM_get_tag (Topic_models [Pos_Divergence4]),
		 Min_Divergence4);
	fprintf (stderr, "Smallest radius 1 for %s = %.3f\n",
		 TLM_get_tag (Topic_models [Pos_Radius1]),
		 Min_Radius1);
    }
}

int
main (int argc, char *argv[])
{
    fprintf (stderr, "Remember to check that all models are static!!\n");

    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    classifyTestfiles ();

    return 0;
}
