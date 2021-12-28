/**
	csound6~: a port of the csound
*/

#include "ext.h"			// standard Max include, always required (except in Jitter)
#include "ext_obex.h"		// required for "new" style objects
#include "ext_common.h"
#include "ext_strings.h"
#include "z_dsp.h"			// required for MSP objects
#include "common/commonsyms.c"
#include "string.h"


// below copied from csound6
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
 
  char *csd_file; 
  char *csd_fullpath; 
  char *orc_file; 
  char *orc_fullpath; 
  char *sco_file; 
  char *sco_fullpath; 

  // stuff from csound6
  CSOUND   *csound;
  // all the below will eventually need to be active
  float   f;
  // the below were type t_sample in the pd version
  double *outs[CS_MAX_CHANS];
  double *ins[CS_MAX_CHANS];
  int     compiled;
  int     vsize;
  int     chans;
  int     pksmps;
  int     pos;
  int     cleanup;
  int     end;
  int     numlets;
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
void csound6_dsp64(t_csound6 *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void csound6_perform64(t_csound6 *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);

void csound6_bang(t_csound6 *x);
int csound6_csound_start(t_csound6 *x);

static void csound6_event(t_csound6 *x, t_symbol *s, int argc, t_atom *argv);

// global class pointer variable
static t_class *csound6_class = NULL;


//***********************************************************************************************

void ext_main(void *r)
{
	// object initialization, note the use of dsp_free for the freemethod, which is required
	// unless you need to free allocated memory, in which case you should call dsp_free from
	// your custom free function.

  // XXX: need to fix this to make it call free??
	t_class *c = class_new("csound6~", (method)csound6_new, (method)dsp_free, (long)sizeof(t_csound6), 0L, A_GIMME, 0);

  class_addmethod(c, (method)csound6_bang, "bang", NULL, 0);
  class_addmethod(c, (method) csound6_event, "event", A_GIMME, 0);

	class_addmethod(c, (method)csound6_dsp64,	"dsp64",A_CANT, 0);
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
           " A Max Csound class using the Csound %d.%02d.%d API\n"
           "(c) Victor Lazzarini, 2005-2007, Iain C.T. Duncan 2021\n", v1, v2, v3);
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

// get full path for a source file
// is_main_source_file is whether it's the box argument 
const char * csound6_get_fullpath(const char *file_arg){
    //post("csound6_get_fullpath(), file_arg %s", file_arg);
    t_fourcc filetype = 'TEXT', outtype;
    char filename[MAX_PATH_CHARS];
    short path_id;
    strcpy(filename, file_arg);    
    if (locatefile_extended(filename, &path_id, &outtype, &filetype, 1)) { // non-zero: not found
        post("csound6~: file %s not found", file_arg);
        return NULL;
    }
    // we have a file and a path short, need to convert it to abs path 
    char *full_path = sysmem_newptr( sizeof(char) * 1024); 
    path_toabsolutesystempath(path_id, filename, full_path);
    //post("  - full_path: %s", full_path);
    return full_path;
}
 


void *csound6_new(t_symbol *s, long argc, t_atom *argv){
  post("cound6_new()");
	t_csound6 *x = (t_csound6 *)object_alloc(csound6_class);

  // stuff from csound6
  x->csound = (CSOUND *) csoundCreate(x);
  x->orc = NULL;
  x->numlets = 1;
  x->run = 1;
  x->chans = 1;
  x->cleanup = 0;
  x->cmdl = NULL;
  x->iochannels = NULL;
  x->csmess = malloc(MAXMESSTRING);
  x->messon = 1;
  x->compiled = 0;

  x->csd_file = NULL;
  x->orc_file = NULL;
  x->sco_file = NULL;

  csoundSetHostImplementedAudioIO(x->csound, 1, 0);
  // stuff from csound6 that eventually needs to be active
  //csoundSetInputChannelCallback(x->csound, in_channel_value_callback);
  //csoundSetOutputChannelCallback(x->csound, out_channel_value_callback);
  //csoundSetHostImplementedMIDIIO(x->csound, 1);
  //csoundSetExternalMidiInOpenCallback(x->csound, open_midi_callback);
  //csoundSetExternalMidiReadCallback(x->csound, read_midi_callback);
  //csoundSetExternalMidiInCloseCallback(x->csound, close_midi_callback);
  //csoundSetMessageCallback(x->csound, message_callback);

  // expects args to be either csd file, orc file, or pair of orc and sco
  // this needs to be smarter later to allow passing in attrs as they appear
  // in the argc and argc list, annoyingly
  post("csound6~ new, arc: %i", argc);
  if(argc == 1){
      x->csd_file = atom_getsym(argv)->s_name;
      post("csd: %s", x->csd_file);
      x->csd_fullpath = csound6_get_fullpath(x->csd_file);
  }else if (argc == 2){
      x->orc_file = atom_getsym(argv)->s_name;
      x->sco_file = atom_getsym(argv+1)->s_name;
      post("orc: %s sco: %s", x->orc_file, x->sco_file);
  }

  // if doing csd, compile 
  if( x->csd_file ){
    //const char * csd_path = "/Users/iainduncan/Documents/max-code/csound/csound-1.csd";
    post("compiling %s", x->csd_fullpath);
    const char *cs_cmdl[] = { "csound", x->csd_fullpath};
    x->compiled = csoundCompile(x->csound, 2, (const char **)cs_cmdl);
  }else{
    post("ERROR orc/sco pairs not yet implemented");
  }

  x->run = 0;
  if (!x->compiled) {
    post("compiled, starting %s", x->csd_file);
    x->compiled = 1;
    x->end = 0;
    x->cleanup = 1;
    x->chans = csoundGetNchnls(x->csound);
    x->pksmps = csoundGetKsmps(x->csound);
    x->numlets = x->chans;
    post("x->chans: %i, x->pksmps %i, x->numlets: %i", x->chans, x->pksmps, x->numlets);

    // create a signal inlet and outlet for each csound channel
	  dsp_setup((t_pxobject *)x, x->numlets);	  // MSP inlets: arg is # of inlets and is REQUIRED!
    for (int i = 0; i < x->numlets && i < CS_MAX_CHANS; i++){
      outlet_new(x, "signal");
    }
    x->pos = 0;
    x->run = 0;   // in PD, it starts playback automatically (this is what the pd one does)
  }
  else
    post("csound6~ error: could not compile");

	return (x);
}

void csound6_bang(t_csound6 *x){
  post("csound6_bang, attempting to start playback");
  if ( csound6_csound_start(x) ){
    post("error starting playback");
  }else{
    post("starting csound score");
  }
}

int csound6_csound_start(t_csound6 *x){
  post("csound6_csound_start()");
  float max_sr = sys_getsr();
  int max_vector_size = sys_getblksize();
  if( x->pksmps != max_vector_size ){
    post("error, csound ksmps must match Max signal vector size (for now)");
    x->run = 0;
    return 1;
  }else{
    post("... setting x->run to 1");
    x->run = 1;
    return 0; 
  }
}

static void csound6_event(t_csound6 *x, t_symbol *s, int argc, t_atom *argv){
    if( x->compiled == 0 ){
      post("csound6~ error: csound not compiled");
      return;
    }
    t_symbol *evt_type = atom_getsym(argv);
    if( evt_type != gensym("i") && evt_type != gensym("f") && evt_type != gensym("e") ){
      post("csound6~ error: valid event types are i, f, and e");
      return;
    }
    // make an array of floats for the pfields arguments
    MYFLT   *pfields;
    int num_pfields = argc - 1;
    pfields  = (MYFLT *) sysmem_newptr( num_pfields * sizeof(MYFLT) );
    for (int i=1; i<argc; i++) pfields[i-1] = atom_getfloat( &argv[i] );

    int res = csoundScoreEvent(x->csound, evt_type->s_name[0], pfields, 5);
    // not sure about these two, were in the PD version
    x->cleanup = 1;
    x->end = 0;
    
    sysmem_freeptr(pfields);
}


// NOT CALLED!, we use dsp_free for a generic free function
void csound6_free(t_csound6 *x){
    post("csound6_free()");	
}


// registers a function for the signal chain in Max
void csound6_dsp64(t_csound6 *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags){
  //post("csound6_dsp64()");
	//post("my sample rate is: %f", samplerate);
  object_method(dsp64, gensym("dsp_add64"), x, csound6_perform64, 0, NULL);
}


// IN PROG:
void csound6_perform64(t_csound6 *x, t_object *dsp64, double **ins, long numins, 
          double **outs, long numouts, long sampleframes, long flags, void *userparam){
  //post("csound6_perform65");

  MYFLT  *csout, *csin;
  // get the csound working buffers
  csout = csoundGetSpout(x->csound);
  csin = csoundGetSpin(x->csound);

  // do nothing if we aren't running
  if( x->run ){

    // render a ksmp vector, which updates the csout and csin pointers
    if( x->end = csoundPerformKsmps(x->csound) ){
      // todo: pd version sends a bang when done
      x->run = 0;
    }else{
      for (int i=0; i < x->numlets; i++){
        for(int s=0; s < sampleframes; s++){
          outs[i][s] = csoundGetSpoutSample(x->csound, s, i);
        }
      }
    }
  }else{
    // if not running hold output at 0 - not sure if this is necessary
    for (int i=0; i < x->numlets; i++){
      for(int s=0; s < sampleframes; s++) outs[i][s] = 0.0; 
    }
  }

}


