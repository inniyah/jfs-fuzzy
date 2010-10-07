  /*********************************************************************/
  /*                                                                   */
  /* jfw2slib.h  Version 2.01  Copyright (c) 1999-2000 Jan E. Mortensen*/
  /*                                                                   */
  /* JFS Inverse Compiler-functions. Converts a JFW-file to a JFS-file. */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#ifndef jfw2slibH
#define jfw2slibH

int jfw2s_conv(char *de_fname, char *so_fname, char *err_fname,
               int ndigits, int maxtext, int out_mode,
	              int var_nos, int op_mode);


/* Convert the .jfw-file <de_fname> to the .jfs-file <so_fname>.      */
/* err_fname:  write error-messages to err_fname, if NULL write to    */
/*             stdout.                                                */
/* ndigits  :  max number of digits after decimal-point.              */
/*             if <ndigits>==0 then default (5).                      */
/* outmode  :  0: ovewrite error-file,                                */
/*             1: append to error-file.                               */
/* maxtext  :  Maximal number of chars in a single statement.         */
/*             if <maxtext>==0 then default (512).                    */
/* var_nos  :  1: write variable-numbers in comment-lines.            */
/* op_mode  :  0: declarations in compact form,                       */
/*             1: write declarations in full form.                    */

/* RETURN:     0: succes,                                             */
/*             1: warnings,                                           */
/*             2:errors,                                              */
/*             3:fatal error.                                         */

#endif
