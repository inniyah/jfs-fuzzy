  /**************************************************************************/
  /* jfr2clib.h  Version  2.03 Copyright (c) 1999-2000 Jan E. Mortensen     */
  /*                                                                        */
  /* Functions to convert a compiled jfs-program to C-sourcecode.           */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#ifndef jfr2clibH
#define jfr2clibH


int jfr2c_conv(char *opfname, char *func_name, char *ipfname,
               int precision, int non_protected, int non_rounded,
               int use_relations, int use_minmax, int use_inline,
               int optimize, int conf_func,
               int use_double,
       	       int maxtree, int maxstack, FILE *sout);

/* Converts a compiled jfs-program to C-sourcecode. Arguments:              */
/* opfname      : Write the C-sourcecode program to the file <opfname>.     */
/*                Write a header-file to the file <on>.h, where <on> is     */
/*                equal to <opfname> without its extension.                 */
/* ipfname      : The filename of the file containing the compiled          */
/*                jfs-program.                                              */
/* precision    : Write max the specified number of digits after the        */
/*                decimal-point in floating-point numbers.                  */
/* non_protected: 0: use standard (protected) jfs-functions,                */
/*                1: use non-protected version of log(), acos(), / etc.     */
/* non_rounded  : 0: standard jfs rounding of expressions and variables,    */
/*                1: no rounding of expressions, operator-args, variables.  */
/* use_relations: 0: use standard jfs-versions of  logical operators        */
/*                   (==, >, etc),                                          */
/*                1: use C's logical operators (returns integers).          */
/* use_minmax   : 0: define functions to calculate minimum and maximum,     */
/*                1: use the functions min(), max() defined in stdlib in    */
/*                   some C-compilers to calculate min and max.             */
/* use_inline   : 0: don't use inline-functions,                            */
/*                1: define small common-functions as inline-functions.     */
/* optimize     : 0: optimize for speed,                                    */
/*                1: optimize for space.                                    */
/* conf_func    : 0: don't include confidence-values in the C-function      */
/*                   which runs the jfs-program (jfr(op[], ip[]);),         */
/*                1: include confinde-values in C-function:                 */
/*                   jfr(op[], ip[], conf[]);.                              */
/* use_double   : 0: create C-variables of the type 'float',                */
/*                1: create C-variables of the type 'double'.               */
/* maxtree      : The number of nodes in the jfg-tree used to hold a        */
/*                statement (128 is a typical value).                       */
/* maxstack     : Size of jfg-conversion-stack (64 is typcial value).       */
/* sout         : Write messages to this open file (set <sout> =stdout      */
/*                for output to screen).                                    */

#endif
