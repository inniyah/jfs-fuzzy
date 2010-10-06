
  /**************************************************************************/
  /*                                                                        */
  /* jfw2rlib.h   Version  2.03    Copyright (c) 1999-2000 Jan E. Mortensen */
  /*                                                                        */
  /* JFS converter. Converts a JFW-file to a JFR-file.                      */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/



int jfw2r_convert(char *de_fname, char *so_fname,
                  char *er_fname, int er_append,
                  int maxstack, int maxargline, int margc,
                  int err_message_mode,
                  int err_line_mode);

/* Convert the jfw-file <so_fname> to the jfr-file <de_fname>.            */

/* ARGUMENTS:                                                             */
/* de_fname    : The compiled program is written to the file <de_fname>.  */
/* so_fname    : The name of the file containing the jfw-program.         */
/* er_fname    : If <er_fname> == NULL then error-messages are written to */
/*               stdout, else error-messages are written to <er_fname>.   */
/* er_append   : <er_append> == 0: open <er_fname> for writing; discard   */
/*                                 previus contens if any.                */
/*                           == 1: append error-messages to <er_fname>.   */
/* max_stack   : The size of the expression-stack. If <max_stack>==0 then */
/*               the size of the expression-stack is set to 100.          */
/* maxargline  : The maximal number of characters in a statement. If      */
/*               <maxargline>==0 then the maximal number of characters    */
/*               are 1024.                                                */
/* margc       : The maximal number of tokens in a statement. If          */
/*               <margc>==0 then the maximal number of tokens is 256.     */
/* err_message_mode : if <err_message_mode> != 0 then error-messages are  */
/*               written in compact form, else error-messages are writen  */
/*               in normal form.                                          */
/* err_line_mode : 0: start with line number 1,                           */
/*                 1: start with line number jfw_head.f_code_line_no.     */

/* RETURN:                                                                */
/*         0: succes. No errors in program.                               */
/*         1: failure. Errors in program. Nothing written to <de_fname>.  */
/*         2: failure. Error while writting to destination. <de_fname> is */
/*                     damaged.                                           */
