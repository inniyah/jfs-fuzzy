  /**************************************************************************/
  /* jfpltlib.h  Version  2.02 Copyright (c) 2000 Jan E. Mortensen          */
  /*                                                                        */
  /* Functions to write plot-info about a compiled jfs-program to GNU-plot. */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#ifndef jfpltlibH
#define jfpltlibH

struct jfplt_error_rec
{ int error_mode;
  int error_no;
  char argument[256];
};
#define JPLT_EM_NONE    0
#define JPLT_EM_ERROR   2

extern struct jfplt_error_rec jfplt_error_desc;


struct jfplt_param_desc
{
  char *opfname;
  char *ipfname;
  char *initfname;
  char *term_name;
  char *op_extension;
  char *op_dir;
  int plt_hedges;
  int plt_relations;
  int plt_operators;
  int plt_fuz;
  int plt_defuz;
  int digits;
  int samples;
  char **data;
  int data_c;
  FILE *sout;
};

/* maxtree      : The number of nodes in the jfg-tree used to hold a        */
/*                statement (128 is a typical value).                       */
/* maxstack     : Size of jfg-conversion-stack (64 is typcial value).       */
/* sout         : Write messages to this open file (set <sout> =stdout      */
/*                for output to screen).                                    */

int jfplt_plot(struct jfplt_param_desc *params);


#endif
