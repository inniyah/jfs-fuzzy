  /*********************************************************************/
  /*                                                                   */
  /* jfr2wlib.h Version 2.01 Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                                                   */
  /* JFS Inverse Compiler-functions. Converts a JFR-file to a JFW-file. */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#ifndef jfr2wlibH
#define jfr2wlibH

int jfr2w_conv(char *de_fname, char *so_fname, char *sout_fname,
               int out_mode, int ndigits,
   	           int maxtext, int maxtree, int maxstack,
	              int rule_nos, int smode, int sw_comments);

/* Convert the .jfr-file <de_fname> to the .jfw-file <so_fname>.      */
/* sout_fname: Redirect messages, normaly written to stdout, to       */
/*             <sout_fname>. If <sout_fname>==NULL write messages to  */
/*             stdout.                                                */
/* out_mode :  0: overwrite <sout_fname>,                             */
/*             1: append to <sout_fname>.                             */
/* ndigits  :  max number of digits after decimal-point.              */
/*             if <ndigits>==0 then default (5).                      */
/* maxtext  :  Maximal number of chars in a single statement.         */
/*             if <maxtext>==0 then default (512).                    */
/* maxtree  :  Maximal number of nodes in jfg_tree. If <maxtree>==0   */
/*             then default (128).                                    */
/* maxstack :  Size of conversion-stack. If <maxstack>==0 then        */
/*             default-size (64).                                     */
/* rules_nos:  1: write rule-numbers in comment-lines.                */
/* smode    :  0: Write only the nessasary parentheses,               */
/*             1: Write parentheses around all expressions.           */
/* sw_comments: 1: write '|'-comments in case-statements.             */
/*                                                                    */
/* RETURN:                                                            */
/*         0: succes,                                                 */
/*         1: warnings,                                               */
/*         2: errors,                                                 */
/*         3: fatal errors.                                           */


#endif
