/***
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

// global class pointer variable
static t_class *csound6_class = NULL;

// linked list struct for iochannels, with name/value pairs
typedef struct _channelname {
  t_symbol *name;
  MYFLT   value;
  struct _channelname *next;
} channelname;

typedef struct _csound6 {
	t_pxobject		ob;			
	double			offset; 	
 
  char *csd_file; 
  char *csd_fullpath; 
  char *orc_file; 
  char *orc_fullpath; 
  char *sco_file; 
  char *sco_fullpath; 

  // stuff from csound6
  CSOUND   *csound;
  float   f;
  // the below were type t_sample in the pd version
  double *outs[CS_MAX_CHANS];
  double *ins[CS_MAX_CHANS];
  bool    compiled;
  bool    ready;      // all checks pass
  bool    running;
  int     vsize;
  int     chans;
  int     ksmps;
  int     kpass_in_vector;
  int     kpasses_per_vector;
  int     max_vector_size;
  int     pos;
  int     cleanup;
  int     end;
  int     numlets;
  int     ver;
  int     num_cs_args;
  char    **cs_cmdl;
  int     argnum;
  channelname *iochannels;  
  //t_outlet *ctlout;
  //t_outlet *bangout;
 
  int     post_messages;
  char    *cs_message;

  // TODO dynamic orchestra compilation
  //char *orc;

} t_csound6;


// method prototypes
static void *csound6_new(t_symbol *s, long argc, t_atom *argv);
static void csound6_free(t_csound6 *x);
static void csound6_dsp64(t_csound6 *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
static void csound6_perform64(t_csound6 *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);

static void csound6_init_csound(t_csound6 *x);
static bool csound6_ready(t_csound6 *x);

static void csound6_reset(t_csound6 *x);
static void csound6_bang(t_csound6 *x);
static void csound6_start(t_csound6 *x);
static void csound6_stop(t_csound6 *x);
static void csound6_rewind(t_csound6 *x);
static void csound6_offset(t_csound6 *x, double arg);
static int csound6_csound_start(t_csound6 *x);
static void csound6_event(t_csound6 *x, t_symbol *s, int argc, t_atom *argv);

// channel functions ported from Pd version (in prog)
static channelname *create_channel(channelname *ch, char *channel);
static MYFLT get_channel_value(t_csound6 *x, char *channel);
static int set_channel_value(t_csound6 *x, t_symbol *channel, MYFLT value);
static void destroy_channels(channelname *ch);
static void csound6_register_controls(t_csound6 *x, t_symbol *s, int argc, t_atom *argv);
static void in_channel_value_callback(CSOUND *csound, const char *name, void *val, const void *channelType);
//static void out_channel_value_callback(CSOUND *csound, const char *name, void *val, const void *channelType);
static void csound6_control(t_csound6 *x, t_symbol *s, double f);
static void csound6_set_channel(t_csound6 *x, t_symbol *s, int argc, t_atom *argv);


//***********************************************************************************************

void ext_main(void *r){
  // XXX: need to fix this to make it call free??
	t_class *c = class_new("csound6~", (method)csound6_new, (method)dsp_free, 
                         (long)sizeof(t_csound6), 0L, A_GIMME, 0);

  class_addmethod(c, (method) csound6_bang, "bang", NULL, 0);
  class_addmethod(c, (method) csound6_reset, "reset", NULL, 0);
  class_addmethod(c, (method) csound6_start, "start", NULL, 0);
  class_addmethod(c, (method) csound6_stop, "stop", NULL, 0);
  class_addmethod(c, (method) csound6_rewind, "rewind", NULL, 0);
  class_addmethod(c, (method) csound6_offset, "offset", A_FLOAT, 0);
  class_addmethod(c, (method) csound6_event, "event", A_GIMME, 0);
  class_addmethod(c, (method) csound6_set_channel, "chnset", A_GIMME, 0);

  // LEGACY control stuff, NB this was registered for the 'set' message in the PD version 
  // and called csoundapi_channel
  class_addmethod(c, (method) csound6_register_controls, "controls", A_GIMME, 0);
  class_addmethod(c, (method) csound6_control, "control", A_DEFSYM, A_DEFFLOAT, 0);

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
           "(c) Iain C.T. Duncan, 2021, Victor Lazzarini, 2005-2007\n", v1, v2, v3);
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

  // set up internal state
  x->csound = (CSOUND *) csoundCreate(x);
  // x->orc = NULL;
  x->numlets = 1;
  x->compiled = 0;
  x->running = false;
  x->kpass_in_vector = 0;
  x->chans = 1;
  x->cleanup = 0;
  x->cs_cmdl = NULL;
  x->iochannels = NULL;
  x->cs_message = sysmem_newptr(MAXMESSTRING);
  x->post_messages = 1;
  x->csd_file = NULL;
  x->orc_file = NULL;
  x->sco_file = NULL;

  csoundSetHostImplementedAudioIO(x->csound, 1, 0);
  csoundSetInputChannelCallback(x->csound, in_channel_value_callback);
  //csoundSetOutputChannelCallback(x->csound, out_channel_value_callback);

  // stuff from csound6 that eventually needs to be active
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
  if( x->csd_fullpath ){
    //const char * csd_path = "/Users/iainduncan/Documents/max-code/csound/csound-1.csd";
    post("compiling %s", x->csd_fullpath);
    const char *cs_cmdl[] = { "csound", x->csd_fullpath};
    if( csoundCompile(x->csound, 2, (const char **)cs_cmdl) ){
      post("csound6: error compiling %s", x->csd_fullpath);
    }else 
      x->compiled = true;
  }else{
    post("ERROR orc/sco pairs not yet implemented");
  }

  if( x->compiled ){
    post("csound6~ compiled", x->csd_file);
    x->end = 0;
    x->cleanup = 1;
    x->chans = csoundGetNchnls(x->csound);
    x->ksmps = csoundGetKsmps(x->csound);
    x->numlets = x->chans;
    post("x->chans: %i, x->ksmps %i, x->numlets: %i", x->chans, x->ksmps, x->numlets);

    // create a signal inlet and outlet for each csound channel
	  dsp_setup((t_pxobject *)x, x->numlets);	  // MSP inlets: arg is # of inlets and is REQUIRED!
    for (int i = 0; i < x->numlets && i < CS_MAX_CHANS; i++){
      outlet_new(x, "signal");
    }
    x->pos = 0;
  }
  else
    post("csound6~ error: could not compile");

	return (x);
}


static void csound6_init_csound(t_csound6 *x){
  post("csound6_init_csound()");

  // PD version
  //if (x->end && x->cleanup) {
  //  csoundCleanup(x->csound);
  //  x->cleanup = 0;
  //}
  if(x->compiled){
    csoundCleanup(x->csound);
  }
  csoundReset(x->csound);
  csoundSetHostImplementedAudioIO(x->csound, 1, 0);
  //csoundSetExternalMidiInOpenCallback(x->csound, open_midi_callback);
  //csoundSetExternalMidiReadCallback(x->csound, read_midi_callback);
  //csoundSetExternalMidiInCloseCallback(x->csound, close_midi_callback);

  if(!x->csd_fullpath){
    post("csound6 error: no csd file set for compiling");
    return;
  }else{
    post("compiling %s", x->csd_fullpath);
  }

  const char *cs_cmdl[] = { "csound", x->csd_fullpath};
  if( !csoundCompile(x->csound, 2, (const char **)cs_cmdl) ){
    post("csound6: compiled %s", x->csd_fullpath);
    x->ksmps = csoundGetKsmps(x->csound);
    x->compiled = true;
  }else{
    post("ERROR re-initializing csound");
  }
}

static void csound6_reset(t_csound6 *x){
  post("csound6_reset()");
  // stop playback so perform function won't try to render
  x->running = false;
  csound6_init_csound(x); 
}

// handler for the "start" message
static void csound6_start(t_csound6 *x){
  post("csound6_start()");
  csound6_csound_start(x);
}

// handler for the bang message
static void csound6_bang(t_csound6 *x){
  post("csound6_bang");
  csound6_csound_start(x);
}

static void csound6_stop(t_csound6 *x){
  post("csound6_stop()");
  x->running = false;
}

static void csound6_rewind(t_csound6 *x){
  post("csound6_rewind()");
  if(x->compiled){
    csoundSetScoreOffsetSeconds(x->csound, (MYFLT) 0);
    csoundRewindScore(x->csound);
    csoundSetScorePending(x->csound, 1);
  }
}

static void csound6_offset(t_csound6 *x, double arg){
  post("csound6_offset(), %5.2f", arg);
  if(x->compiled){
    csoundSetScoreOffsetSeconds(x->csound, (MYFLT) arg);
    csoundRewindScore(x->csound);
    csoundSetScorePending(x->csound, 1);
  }
}


// sanity check of sr, ksmps, channels
// should be run after any compile
// has side effect of setting max_vector_size and kpasses_per_vector 
static bool csound6_ready(t_csound6 *x){
  post("csound6_ready() checking rates");
  x->max_vector_size = sys_getblksize();
  if(!x->compiled){
    post("csound6~ error: csound is not compiled");
    return false;
  }
  if( sys_getsr() != csoundGetSr(x->csound) ){
    post("csound6~ error: csound sr must match Max");
    return false;
  }
  if(x->chans != csoundGetNchnls(x->csound) ){
    post("csound6~ error: number of channels can only be changed on object creation file load.");
    return false;
  }
  if( x->max_vector_size % x->ksmps != 0 ){
    post("csound6~ error: ksmps must be even divisor of Max signal vector size");
    return false;
  }else{
    x->kpasses_per_vector = x->max_vector_size / x->ksmps;
  }
  // all checks passed, csound is ready to play
  return true; 
}

static int csound6_csound_start(t_csound6 *x){
  post("csound6_csound_start()");
  // if csound is ready, set the running flag which will kick off performance
  x->running = csound6_ready(x); 
  post("  - x->running now: %i", x->running);
}

static void csound6_event(t_csound6 *x, t_symbol *s, int argc, t_atom *argv){
    if( x->compiled == false ){
      post("csound6~ error in csound6_event: csound not compiled");
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
static void csound6_free(t_csound6 *x){
    post("csound6_free()");	
}


// registers a function for the signal chain in Max
static void csound6_dsp64(t_csound6 *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags){
  //post("csound6_dsp64()");
	//post("my sample rate is: %f", samplerate);
  object_method(dsp64, gensym("dsp_add64"), x, csound6_perform64, 0, NULL);
}


// IN PROG: WORKING but only for ksmps == vector size
static void csound6_perform64(t_csound6 *x, t_object *dsp64, double **ins, long numins, 
          double **outs, long numouts, long sampleframes, long flags, void *userparam){
  //post("csound6_perform65");

  MYFLT  *csout, *csin;
  // get the csound working buffers
  csout = csoundGetSpout(x->csound);
  csin = csoundGetSpin(x->csound);

  // TODO: allow multiple kpasses

  // do nothing if we aren't running
  if( x->running ){
    // render a ksmp vector, which updates the csout and csin pointers
    if( x->end = csoundPerformKsmps(x->csound) ){
      // todo: pd version sends a bang when done
      x->running = 0;
    }else{
      for (int i=0; i < x->numlets; i++){
        for(int s=0; s < sampleframes; s++){
          outs[i][s] = csoundGetSpoutSample(x->csound, s, i);
        }
      }
    }
  }
  // if not running hold output at 0 - not sure if this is necessary
  else{
    for (int i=0; i < x->numlets; i++){
      for(int s=0; s < sampleframes; s++) outs[i][s] = 0.0; 
    }
  }
}

// LEGACY invalue system - deprecate
// all this does is find the channel and set the value pointer
// returns 1 on success, 0 if named channel not found
static int set_channel_value(t_csound6 *x, t_symbol *channel, MYFLT value){
    //post("set_channel_value() chan: %s val: %.2f", channel->s_name, value);
    channelname *ch = x->iochannels;
    // traverse the linked list of channels to find the right channel and set it's value
    if (ch == NULL) return 0; 
    while (strcmp(ch->name->s_name, channel->s_name)) {
      ch = ch->next;
      if (ch == NULL) return 0;  
    }
    ch->value = value;
    //post(" - found channel, has set %s to %.2f", ch->name->s_name, ch->value);
    return 1;
}

// LEGACY invalue system - deprecate
static MYFLT get_channel_value(t_csound6 *x, char *channel){
    //post("get_channel_value() chan: '%s'", channel);
    channelname *ch;
    ch = x->iochannels;
    if (ch == NULL) return (MYFLT) 0; 
    while (strcmp(ch->name->s_name, channel)) {
      ch = ch->next;
      if (ch == NULL) return (MYFLT) 0; 
    }
    //post("  - returning: %.2f", ch->value);
    return ch->value;
}

// LEGACY invalue system - deprecate
// create a new channel and add to the channel linked list
static channelname *create_channel(channelname *ch, char *channel){
    //post("create_channel %s", channel);
    channelname *tmp = ch, *newch = (channelname *) sysmem_newptr( sizeof(channelname) );
    newch->name = gensym(channel);
    newch->value = 0.f;
    newch->next = tmp;
    ch = newch;
    return ch;
}

// LEGACY invalue system - deprecate
static void destroy_channels(channelname *ch){
    channelname *tmp = ch;
    while (ch != NULL) {
      tmp = ch->next;
      sysmem_freeptr(ch);
      ch = tmp;
    }
}

// LEGACY invalue system - deprecate
// called from the "controls" message, creates named channels
static void csound6_register_controls(t_csound6 *x, t_symbol *s, int argc, t_atom *argv){
    post("csound6_register_controls()");
    int     i;
    char    chan_name[128];
    for (i = 0; i < argc; i++) {
      // is the below even necessary??
      strcpy( chan_name, atom_getsym(argv+i)->s_name );
      // PD had the below
      // XXX: we should be checking the channel doesn't exist already first!
      //post(" registering %s", chan_name);
      x->iochannels = create_channel(x->iochannels, chan_name);
    }
}

// LEGACY invalue system - deprecate
static void csound6_control(t_csound6 *x, t_symbol *s, double f){
    post("csound6_control, setting %s to %.2f", s->s_name, f);
    if (!set_channel_value(x, s, f))
      post("csound6~ error: channel not found");
}

// LEGACY invalue system - deprecate
// callback fired by csound engine, ported directly from Pd version
static void in_channel_value_callback(CSOUND *csound, const char *name, void *valp, const void *channelType) {
    //post("in_channel_value_callback, chan name: %s", name);
    t_csound6 *x = (t_csound6 *) csoundGetHostData(csound);
    MYFLT *vp = (MYFLT *) valp;
    *vp = get_channel_value(x, (char *) name);
}

// callback fired by csound engine, adapted from Pd version
// allows sending control values from Csound
//static void out_channel_value_callback(CSOUND *csound, const char *name, 
//                                      void *valp, const void *channelType) {
//    t_atom  at[2];
//    t_csound6 *x = (t_csound6 *) csoundGetHostData(csound);
//    MYFLT val = *((MYFLT *) valp);
//    set the symbol and value for the output key/val pair
//    SETFLOAT(&at[1], (t_float) val);
//    SETSYMBOL(&at[0], gensym((char *) name));
//    outlet_list(x->ctlout, gensym("list"), 2, at);
//}

// called on the 'chnset' input message, series of key value pairs
// working
static void csound6_set_channel(t_csound6 *x, t_symbol *s, int argc, t_atom *argv){
  post("csound6_set_channel()");
  CSOUND *csound = x->csound;
  int i;
  char chan_name[128]; // temp string for channel name
  MYFLT chan_val;

  // loop through the key/val pairs
  for(i=0; i < argc; i+=2){
    // copy channel name from key of this pair
    // Pd: atom_string(&argv[i], chn, 64);
    if( atom_gettype( &argv[i] ) != A_SYM ){
      post("csound6~ error: chnset keys must be symbols");
      return;
    }else{
      // ??? not sure if the below is necessary...
      strcpy( chan_name, atom_getsym(argv+i)->s_name );
    }
    if(i+1 < argc){
      // set the value to be either a string of number
      switch (atom_gettype( &argv[i+1])) {
        case A_SYM: 
          csoundSetStringChannel(csound, chan_name, atom_getsym( &argv[i+1] )->s_name);
          break;
        case A_LONG:
        case A_FLOAT:
          chan_val = atom_getfloat( &argv[i+1] );
          //post("setting %s to %.2f", chan_name, chan_val);
          csoundSetControlChannel(csound, chan_name, chan_val);
          break;
      }
    }
  }
}


