
/***************************************************************************/
/*                                                                         */
/* jhlp_lib.h      Version 1.02      Copyright (c) 1999-2000 Jan Mortensen */
/*                                                                         */
/* Converts a jhlp-system to html.                                         */
/*                                                                         */
/* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                      */
/*    Lollandsvej 35 3.tv.                                                 */
/*    DK-2000 Frederiksberg                                                */
/*    Denmark                                                              */
/*                                                                         */
/***************************************************************************/

int jhlp_convert(char *de_fname, char *jhi_fname, char *new_so_fname,
                 char *stylesheet, char *head_name,
                 char *er_fname, int append_mode, int copy_mode,
                 long nodes, long mem, int silent);

/* de_fname    : name of destination html-file (only used if writing to    */
/*               a single file). If <de_fname> == "" then destination-dir  */
/*               is the dir of <jfi_fname>.                                */
/* new_so_fname: jhc-file to be added to jhi-file. If "" then the          */
/*               help-system is generated without adding new jhc-files.    */
/* stylesheet  : If <stylesheet> != "" then all generated html-files link  */
/*               to the stylesheet-file <stylesheet>.                      */
/* head_name   : If <head_name> != "" and <de_fname> != "" then only       */
/*               the section <head_name> and sub-sectiosn are written to   */
/*               <de_fname>.                                               */
/* er_fname    : If <er_fname> != "" then messages are written to          */
/*               the file <er_fname>.                                      */
/* append_mode : 1: append messages to <er_fname>.                         */
/* copy_mode   : 1: copy files in jhc-files with extension != '.jhd','.jhp'*/
/*                  to html-directory.                                     */ 
/* nodes       : allocate <nodes> to tempory data. If <nodes>==0 then      */
/*               allocate 4000 nodes.                                      */
/* mem         : allocate <mem> bytes to temporary data. If <mem>==0 then  */
/*               allocate 100.000 bytes.                                   */
/* silent      : 1: don't write messages to stdout.                        */


