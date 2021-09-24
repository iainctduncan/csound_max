/**
	@file
	csound6~: a simple audio object for Max
	original by: jeremy bernstein, jeremy@bootsquad.com
	@ingroup examples
*/

#include "ext.h"			// standard Max include, always required (except in Jitter)
#include "ext_obex.h"		// required for "new" style objects
#include "z_dsp.h"			// required for MSP objects

// below copied from csoundapi
#include <stdio.h>
#if defined(__APPLE__)
#include <CsoundLib64/csound.h>
#else
#include <csound/csound.h>
#endif

#define CS_MAX_CHANS 32
#define MAXMESSTRING 16384

#define MIDI_QUEUE_MAX 1024
#define MIDI_QUEUE_MASK 1023

// ICTD: not sure if I should be using MYFLT or not here
typedef struct _channelname {
  t_symbol *name;
  MYFLT   value;
  struct _channelname *next;
} channelname;

typedef struct _midi_queue {
  int writep;
  int readp;
  unsigned char values[MIDI_QUEUE_MAX];
} midi_queue;

// struct to represent the object's state
typedef struct _csound6 {
	t_pxobject		ob;			// the object itself (t_pxobject in MSP instead of t_object)
	double			offset; 	// the value of a property of our object

  // stuff from csoundapi
  CSOUND   *csound;
  // all the below will eventually need to be active
  float   f;
  //t_sample *outs[CS_MAX_CHANS];
  //t_sample *ins[CS_MAX_CHANS];
  int     vsize;
  int     chans;
  int     pksmps;
  int     pos;
  int     cleanup;
  int     end;
  int     numlets;
  int     result;
  int     run;
  int     ver;
  char    **cmdl;
  int       argnum;
  channelname *iochannels;
  //t_outlet *ctlout;
  //t_outlet *bangout;
  int     messon;
  char     *csmess;
  t_symbol *curdir;
  midi_queue *mq;
  char *orc;

} t_csound6;


// method prototypes
void *csound6_new(t_symbol *s, long argc, t_atom *argv);
void csound6_free(t_csound6 *x);
void csound6_assist(t_csound6 *x, void *b, long m, long a, char *s);
void csound6_float(t_csound6 *x, double f);
void csound6_dsp64(t_csound6 *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void csound6_perform64(t_csound6 *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);


// global class pointer variable
static t_class *csound6_class = NULL;


//***********************************************************************************************

void ext_main(void *r)
{
	// object initialization, note the use of dsp_free for the freemethod, which is required
	// unless you need to free allocated memory, in which case you should call dsp_free from
	// your custom free function.

	t_class *c = class_new("csound6~", (method)csound6_new, (method)dsp_free, (long)sizeof(t_csound6), 0L, A_GIMME, 0);

	class_addmethod(c, (method)csound6_float,		"float",	A_FLOAT, 0);
	class_addmethod(c, (method)csound6_dsp64,		"dsp64",	A_CANT, 0);
	class_addmethod(c, (method)csound6_assist,	"assist",	A_CANT, 0);

	class_dspinit(c);
	class_register(CLASS_BOX, c);
	csound6_class = c;

  {
    int v1, v2, v3;
    v1 = csoundGetVersion();
    v3 = v1 % 10;
    v2 = (v1 / 10) % 100;
    v1 = v1 / 1000;
    post("\ncsound6~ 1.01\n"
           " A Max csound class using the Csound %d.%02d.%d API\n"
           "(c) V Lazzarini, 2005-2007, I Duncan 2021\n", v1, v2, v3);
  }



}

// copied directly from csound_pd, should "just work"
int isCsoundFile(char *in){
    int     len;
    int     i;
    const char *extensions[6] = {".csd", ".orc", ".sco",".CSD",".ORC",".SCO"};
    len = strlen(in);
    for (i = 0; i < len; i++, in++) {
      if (*in == '.') break;
    }
    if (*in == '.') {
      for(i=0; i<6;i++)
        if (strcmp(in,extensions[i])==0) return 1;
    }
    return 0;
}



void *csound6_new(t_symbol *s, long argc, t_atom *argv)
{
	t_csound6 *x = (t_csound6 *)object_alloc(csound6_class);

  // make a csound!
  x->csound = (CSOUND *) csoundCreate(x);
  // stuff from csoundapi
  x->orc = NULL;
  x->numlets = 1;
  x->result = 1;
  x->run = 1;
  x->chans = 1;
  x->cleanup = 0;
  x->cmdl = NULL;
  x->iochannels = NULL;
  x->csmess = malloc(MAXMESSTRING);
  x->messon = 1;
  //x->curdir = canvas_getcurrentdir();

  csoundSetHostImplementedAudioIO(x->csound, 1, 0);
  // stuff from csoundapi that eventually needs to be active
  //csoundSetInputChannelCallback(x->csound, in_channel_value_callback);
  //csoundSetOutputChannelCallback(x->csound, out_channel_value_callback);
  //csoundSetHostImplementedMIDIIO(x->csound, 1);
  //csoundSetExternalMidiInOpenCallback(x->csound, open_midi_callback);
  //csoundSetExternalMidiReadCallback(x->csound, read_midi_callback);
  //csoundSetExternalMidiInCloseCallback(x->csound, close_midi_callback);
  //csoundSetMessageCallback(x->csound, message_callback);
 
  // normally csoundCompile takes argc and argv, so presumably
  // I need to convert from atoms to strings somehow
  //csoundCompile(csound, argc, argv); 
  // from csoundapi:
  //x->result = csoundCompile(x->csound, x->argnum, (const char **)cmdl);
  x->result = csoundCompile(x->csound, 0, NULL);
  


  // from simplemsp~
	if (x) {
		dsp_setup((t_pxobject *)x, 1);	// MSP inlets: arg is # of inlets and is REQUIRED!
		// use 0 if you don't need inlets
		outlet_new(x, "signal"); 		// signal outlet (note "signal" rather than NULL)
		x->offset = 0.0;
	}
	return (x);
}


// NOT CALLED!, we use dsp_free for a generic free function
void csound6_free(t_csound6 *x)
{
	;
}


void csound6_assist(t_csound6 *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { //inlet
		sprintf(s, "I am inlet %ld", a);
	}
	else {	// outlet
		sprintf(s, "I am outlet %ld", a);
	}
}


void csound6_float(t_csound6 *x, double f)
{
	x->offset = f;
}


// registers a function for the signal chain in Max
void csound6_dsp64(t_csound6 *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	post("my sample rate is: %f", samplerate);

	// instead of calling dsp_add(), we send the "dsp_add64" message to the object representing the dsp chain
	// the arguments passed are:
	// 1: the dsp64 object passed-in by the calling function
	// 2: the symbol of the "dsp_add64" message we are sending
	// 3: a pointer to your object
	// 4: a pointer to your 64-bit perform method
	// 5: flags to alter how the signal chain handles your object -- just pass 0
	// 6: a generic pointer that you can use to pass any additional data to your perform method

	object_method(dsp64, gensym("dsp_add64"), x, csound6_perform64, 0, NULL);
}


// this is the 64-bit perform method audio vectors
void csound6_perform64(t_csound6 *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	t_double *inL = ins[0];		// we get audio for each inlet of the object from the **ins argument
	t_double *outL = outs[0];	// we get audio for each outlet of the object from the **outs argument
	int n = sampleframes;

	// this perform method simply copies the input to the output, offsetting the value
	while (n--)
		*outL++ = *inL++ * x->offset;
}

