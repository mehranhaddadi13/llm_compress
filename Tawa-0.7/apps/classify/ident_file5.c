/* Works out the language of a string of text. Writes the results to a JSON file. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "getopt.h"

#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
FILE *Test_fp;

boolean Entropy = FALSE;
boolean DynamicModelRequired = FALSE;
unsigned int DynamicAlphabetSize = 256;
unsigned int DynamicMaxOrder = 5;
unsigned int DynamicEscapeMethod = TLM_PPM_Method_D;
unsigned int MinLength = 0;
unsigned int Load_Numbers = 0;
unsigned int OutputText = 0;

int num_models, m;
int num_files;
int current_file_counter;
void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -C\tcalculate cross-entropy rather than codelength\n"
	     "  -D\tinclude dynamic model in list of models\n"
	     "  -L n\tminimum length of data file=n\n"
	     "  -N\tinput text is a sequence of unsigned numbers\n"
	     "  -a n\tsize of alphabet for dynamic model=n\n"
	     "  -e n\tescape method for model=c\n"
	     "  -m fn\tlist of models filename=fn (required)\n"
	     "  -o n\tmax order of dynamic model=n\n"
	     "  -t fn\tlist of test data files filename=fn (required)\n"
         "  -F\tFormat output as Json\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean models_found;
    boolean data_found;
    int opt, escape;

    /* get the argument options */

    models_found = FALSE;
    data_found = FALSE;
    while ((opt = getopt (argc, argv, "CDL:Na:e:m:o:t:")) != -1)
	switch (opt)
	{
	case 'C':
	    Entropy = TRUE;
	    break;
	case 'D':
	    DynamicModelRequired = TRUE;
	    break;
	case 'L':
	    MinLength = atoi (optarg);
	    break;
	case 'N':
	    Load_Numbers = TRUE;
	    break;
        case 'a':
	    DynamicAlphabetSize = atoi (optarg);
	    break;
	case 'e' :
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    DynamicEscapeMethod = escape;
	    break;
	case 'o':
	    DynamicMaxOrder = atoi (optarg);
	    break;
	case 'm':
	    TLM_load_models (optarg);
	    models_found = TRUE;
	    break;
	case 't':
	    fprintf (stderr, "Loading data files from file %s\n",
		     optarg);
	    if ((Test_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open test file %s\n",
			 optarg);
		exit (1);
	    }
	    data_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found)
    {
        fprintf (stderr, "\nFatal error: missing models filename\n\n");
        usage ();
    }
    if (!data_found)
    {
        fprintf (stderr, "\nFatal error: missing data filename\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

float
codelengthText (unsigned int model, unsigned int text)
/* Returns the codelength for encoding the text using the PPM model. */
{
    unsigned int context, pos, symbol;
    float codelength;

    TLM_set_context_operation (TLM_Get_Codelength);

    codelength = 0.0;
    context = TLM_create_context (model);

    /* Now encode each symbol */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
	TLM_update_context (model, context, symbol);
	codelength += TLM_Codelength;
    }
    TLM_release_context (model, context);

    return (codelength);
}

float *model_bits=NULL;
unsigned int *model_precision=NULL;
unsigned int *model_precision1=NULL;
unsigned int *model_recall=NULL;
unsigned int *model_recall1=NULL;
unsigned int Precision = 0;
unsigned int Precision1 = 0;

void
identifyBestModel (unsigned int file_model, char file_tag [])
{
    unsigned int model, m, bestmodel = 0, nextbestmodel = 0;
    float low=0, nextlow=0;

    TLM_reset_modelno ();

    m = 0;
    while ((model = TLM_next_modelno ()))
    {
        fprintf (stderr, "%6.3f %s\n", model_bits[m], TLM_get_tag (model));
	if ((bestmodel==0) || (model_bits[m]<low))
	  {
	    if (bestmodel!=0){
	        nextlow=low;
		nextbestmodel = bestmodel;
	    }
	    low=model_bits[m];
	    bestmodel = model;
	  }
	else if ((nextbestmodel==0) || (model_bits[m]<nextlow))
	  {
	    nextlow=model_bits[m];
	    nextbestmodel = model;
	  }

	m++;
    }
    if (!strcmp (file_tag, TLM_get_tag (bestmodel)))
      {
	Precision++;
	model_precision [bestmodel]++;
	model_recall [file_model]++;
      }
    else
      {
        Precision1++;
	model_precision1 [bestmodel]++;
	model_recall1 [file_model]++;
      }

    if (Entropy)
        fprintf (stderr,"The lowest cross-entropy of ");
    else
        fprintf (stderr,"The lowest codelength of ");
    fprintf (stderr,"%.3f on file with label %s is for tag %s model named \"%s\"\n",
	     low, file_tag, TLM_get_tag (bestmodel),
	     TLM_get_title (bestmodel));
    if (TLM_valid_model (nextbestmodel))
      {
	if (Entropy)
	    fprintf (stderr,"The next lowest cross-entropy of ");
	else
	    fprintf (stderr,"The next lowest codelength of ");
	fprintf (stderr,"%.3f is for tag %s model named \"%s\"\n",
		 nextlow, TLM_get_tag (nextbestmodel),
		 TLM_get_title (nextbestmodel));
      }
}

void
identifyModels (unsigned int text, unsigned int textlen,
		unsigned int file_model, char file_tag [])
/* Returns the number of bits required to compress each model. */
{
    int model, m;
    unsigned int DynamicModel = NIL;

    if (DynamicModelRequired)
        DynamicModel = TLM_create_model
	   (TLM_PPM_Model, "Dynamic", DynamicAlphabetSize, DynamicMaxOrder,
	    DynamicEscapeMethod, TRUE);

    TLM_reset_modelno ();
    m = 0;
    while ((model = TLM_next_modelno ()))
    {
        model_bits[m] = (codelengthText (model, text));
	if (Entropy)
	    model_bits[m] /= textlen;
	m++;
    }

    identifyBestModel (file_model, file_tag);

    if (DynamicModelRequired)
        TLM_release_model (DynamicModel);
}

void simpleIdentifyModels(unsigned int text, unsigned int textlen,
                          unsigned int file_model, char file_tag[])
/* Helper function for JSONOutput
   Returns the number of bits required to compress each model with json formatting */
{
    int m;
    int model;
    char bits_str [50];

    TXT_append_string (OutputText, "\"");
    TXT_append_string (OutputText, file_tag);
    TXT_append_string (OutputText, "\": {\n");
    TLM_reset_modelno();
    m = 0;
    while((model = TLM_next_modelno()))
    {
        model_bits[m] = codelengthText(model, text);
	TXT_append_string (OutputText, "\"");
	TXT_append_string (OutputText, TLM_get_tag(model));
	TXT_append_string (OutputText, "\": ");
	sprintf (bits_str, "%f", model_bits[m]);
	TXT_append_string (OutputText, bits_str);
        m++;
        if (m < num_models)
	    TXT_append_string (OutputText, ",\n");
        else
	    TXT_append_string (OutputText, "\n");
    }
    current_file_counter++;
    if (current_file_counter < num_files)
        TXT_append_string (OutputText, "},\n");
    else
        TXT_append_string (OutputText, "}\n");
}

void JSONOutput(FILE *fp)
/* Prints codelength of all model/file combinations in json format */
{
    char filename [128];
    char file_tag [128];
    unsigned int file, file_model, text, textlen;

    current_file_counter = 0;
    TXT_append_string (OutputText, "{\n");
    num_files = 0;
    while ((fscanf (fp, "%s %s", file_tag, filename) != EOF))
    {
        num_files++;
    }
    rewind(fp);
    while ((fscanf (fp, "%s %s", file_tag, filename) != EOF))
    {
        file_model = TLM_getmodel_tag(file_tag);
        file = TXT_open_file(filename, "r", NULL, "Ident_file can't open data file");
        text = TXT_load_text(file);
        textlen= TXT_length_text(text);
        if(!MinLength || (textlen >= MinLength))
        {
            simpleIdentifyModels(text, textlen, file_model, file_tag);
        }
        TXT_release_text(text);
        TXT_close_file(file);
    }
    TXT_append_string (OutputText, "}\n");
}


void
identifyFiles (FILE * fp)
/* Load the test data files from file FP. */
{
    char filename [128];
    char file_tag [128];
    unsigned int file, file_model, text, textlen;

    while ((fscanf (fp, "%s %s", file_tag, filename) != EOF))
    {
      file_model = TLM_getmodel_tag (file_tag);
      /*fprintf (stderr, "Model for tag %s = %d\n", file_tag, file_model);*/
      /*dump_memory (stderr);*/

        file = TXT_open_file (filename, "r", NULL,
			      "Ident_file: can't open data file");
	if (Load_Numbers)
	    text = TXT_load_numbers (file);
	else
	    text = TXT_load_text (file);

	textlen = TXT_length_text (text);
	if (!MinLength || (textlen >= MinLength))
	  {
	    fprintf (stderr, "Loading data file from file %s\n",
		     filename);
	    identifyModels (text, textlen, file_model, file_tag);
	  }
	TXT_release_text (text);
	TXT_close_file (file);
    }
}

int
main (int argc, char *argv[])
{
    /* float rec, total_rec; */
    /* float prec, total_prec; */

    init_arguments (argc, argv);

    num_models = TLM_numberof_models();

    model_bits = (float *) calloc (num_models+1, sizeof(float));
    model_recall = (unsigned int *) calloc (num_models+1, sizeof(unsigned int));
    model_recall1 = (unsigned int *) calloc (num_models+1, sizeof(unsigned int));
    model_precision = (unsigned int *) calloc (num_models+1, sizeof(unsigned int));
    model_precision1 = (unsigned int *) calloc (num_models+1, sizeof(unsigned int));

    OutputText = TXT_create_text();

    JSONOutput(Test_fp);

    TXT_dump_text (Stdout_File, OutputText, NULL);

    free(model_bits);
    free(model_precision);
    free(model_precision1);

    return 0;
}
