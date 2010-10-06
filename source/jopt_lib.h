  /**********************************************************************/
  /*                                                                    */
  /* jfopt_lib.h   Version  1.00   Copyright (c) 2000 Jan E. Mortensen  */
  /*                                                                    */
  /* Functions to decode an option-string                               */
  /*                                                                    */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                 */
  /*    Lollandsvej 35 3.tv.                                            */
  /*    DK-2000 Frederiksberg                                           */
  /*    Denmark                                                         */
  /*                                                                    */
  /**********************************************************************/

#ifndef jfoptlibH
#define jfoptlibH

struct jopt_desc
{
  char *txt;
  unsigned short value;
  unsigned short min_argc;
  unsigned short max_argc;
};

struct jopt_error_rec
{ int error_mode;
  int error_no;
  char argument[256];
};
#define JOPT_EM_NONE    0
#define JOPT_EM_WARNING 1
#define JOPT_EM_ERROR   2
#define JOPT_EM_FATAL   3

extern struct jopt_error_rec jopt_error_desc;

/***********************************************************************/
/* functions:                                                          */

int jopt_init(struct jopt_desc *options, int opt_c,
              char *argv[], int argc);
/*                                                                     */
/* initialises the option-system, the posible options are in           */
/* <options>[<opt_c>]. The string to be decoded is in <argv[<argc>].   */

int jopt_get(unsigned short *value, char *aargv[], int *aargc);
/*                                                                     */
/* get the next option and its arguments. Option-value is returned in  */
/* <value>, arguments in <argv>[<argc>].                               */
/* RETURN:                                                             */
/*         0:    succes,                                               */
/*         1:    no-more-options,                                      */
/*        other: error (see <jopt_error> for details.                  */

#endif
