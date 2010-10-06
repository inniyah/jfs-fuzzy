  /*********************************************************************/
  /*                                                                   */
  /* jfr2hlib.c  Version 2.04  Copyright (c) 1999-2000 Jan E. Mortensen*/
  /*                                                                   */
  /* JFS JFR-to-html (javascript) converter.                           */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

/* Global label modes: */
#define JFR2HTM_LM_LEFT     1
#define JFR2HTM_LM_ABOVE    2
#define JFR2HTM_LM_BLABOVE  3
#define JFR2HTM_LM_TABLE    4
#define JFR2HTM_LM_MULTABLE 5

/***********************************************************************/
/* Variable-argument values:                                           */

/* bit 0-1 variable type: */
#define JFR2HTM_VT_STANDARD  0    /* normal input/output variable.       */
#define JFR2HTM_VT_FIELDSET  1    /* starts a field-set no input/output. */
#define JFR2HTM_VT_DB        2    /* read value from database (not       */
                                  /* supported in version 2.04).         */
#define JFR2HTM_VT_DBIP      3    /* database and input variable (not    */
                                  /* supported in version 2.04).         */

/* The other values in variable argument depend on variable type:        */
/*-----------------------------------------------------------------------*/
/* Variable type JFR2HTM_VT_STANDARD:                            */

/* bit 2-5 field-types: */
#define JFR2HTM_FT_DEFAULT    0   /* float,integer uses numeric-text-fields,*/
                                  /* categorical uses pulldown lists.       */
#define JFR2HTM_FT_TEXT_NUM   1   /* (4) text-field numeric.                */
#define JFR2HTM_FT_TEXT_ADJ   2   /* (8) text field adjective (output only).*/
#define JFR2HTM_FT_PULLDOWN   3   /* (12) pull-down list.                   */
#define JFR2HTM_FT_RADIO      4   /* (16) radio buttons, all on same line.  */
#define JFR2HTM_FT_RADIO_M    5   /* (20) radio buttons, one pr line.       */
#define JFR2HTM_FT_CHECKBOX   6   /* (24) checkbox (only input with exactly */
                                  /*     two adjectives. checked == adj 2). */
#define JFR2HTM_FT_RULER      7   /* (28) input the variables value from a  */
                                  /* ruler (not supported in version 2.04). */
#define JFR2HTM_FT_TEXT       8   /* (32) input a text-string instead of a  */
                                  /* variable value (not in version 2.04).  */
#define JFR2HTM_FT_ADJECTIVES 9   /* (36) adjective-values (output only)    */
                                  /*      not supported in version 2.04.    */
                    /*     10-15 reserved for future use.                   */

/* bit 6-16: flags:                                                       */
#define JFR2HTM_VA_COM_LABEL    64 /* use comment as label instead of     */
                                   /* text. */
#define JFR2HTM_VA_HELP_BUTTON 128 /* place a help-button after field     */
                                   /* click displays comment).            */
#define JFR2HTM_VA_CONFIDENCE  256 /* read confidence for variable.       */
#define JFR2HTM_VA_NO_NL       512 /* no new-line after field.            */

/*-----------------------------------------------------------------------*/
/* Variable type JFR2HTM_VT_FIELDSET:                                    */

/* bit 2-5 label-mode: */
#define JFR2HTM_LM_DEFAULT    0 /* use global label mode */
/* other same as global:                                                 */
/* #define JFR2HTM_LM_LEFT     1     (4)                                 */
/* #define JFR2HTM_LM_ABOVE    2     (8)                                 */
/* #define JFR2HTM_LM_BLABOVE  3     (12)                                */
/* #define JFR2HTM_LM_TABLE    4     (16)                                */
/* #define JFR2HTM_LM_MULTABLE 5     (20)                                */

/* bit 6-15: flags: */

/*  #define JFR2HTM_VA_COM_LABEL   64  use comment as field-set label.   */
#define JFR2HTM_VA_NO_LEGEND      128 /* don't write variable's text as  */
                                      /* fieldset ledgend.               */
#define JFR2HTM_VA_NO_FIELDSET    256 /* don't place the block in a HTML */
                                      /* fieldset (, but include         */
                                      /* stylesheet blocks).             */
                                                                              
int jfr2h_conv(char *op_hfname, char *op_jfname,
       	       char *jfr_fname, char *so_fname,
	              int prog_only, int precision, int overwrite, int js_file,
       	       int maxtext, int maxtree, int maxstack, int no_check,
               char *sshet, int use_args, int label_mode, int main_comment,
               char *conf_txt, float conf_max, char *prefix_txt,
               FILE *sout);

/* the function jfr2h_conv converts a compiled jfs-program to a web-page */
/* (a html-document with input/ouptput and a calculate button. The       */
/* jfs-program is converted to Java-script.                              */
/* PARAMETERS:                                                           */
/* op_hfname   : write the html-document to a file with the name         */
/*               <op_fname>.                                             */
/* op_jfname   : if <jfs_file> == 1 then write the Javascript part of    */
/*               the converted program to the file <op_jfname>.          */
/* jfr_fname   : the file-name of the compiled jfs-program, which is to  */
/*               be converted.                                           */
/* so_fname    : if <overwrite>==0 then read the html-file to be updated */
/*               from <so_fname>. <so_fname> has to be different from    */
/*               <op_hfname> (JFR2HTM first copys <op_hfname>.htm to     */
/*               <of_hfname>.old and then sets <so_fname> to             */
/*               <opf_hfname>.old).                                      */
/* prog_only   : 0: update html-forms and Javascript program,            */
/*               1: update only Javascript program.                      */
/* precision   : write floating-point numbers with max <precision>       */
/*               digiges after the decimal-point.                        */
/* overwrite   : 0: update the existing html-file <so_fname> (write to   */
/*                  <op_hfname>) if it exists.                           */
/*               1: create a new html-file overwriting <op_hfname>).     */
/* js_file     : 0: write both forms and Javascript-program to           */
/*                  <op_fname>,                                          */
/*               1: write html-forms to <op_hfname> and Javascript-      */
/*                  program to <op_jfname>.                              */
/* maxtext     : maximum number of characters in statement.              */
/* maxtree     : number of nodes in statement-tree (jfg_lib).            */
/* maxstack    : stack-size in conversion-stack (jfg_lib).               */
/* no_check    : 0: validate numbers with isNaN(),                       */
/*               1: don't validate numbers.                              */
/* ssheet      : if (strlen(ssheet) != 0) include a style-sheet link to  */
/*               <ssheet> in the html-file's header (HREF="<ssheet>").   */
/* use_args    : 1: use the variables argument-value to determine the    */
/*                  input/output-fields format.                          */
/*               0: use default format for all variables.                */
/* label_mode  : Placement of labels. One of the vales JFR2HTM_LM_...    */
/*               defined above.                                          */
/* main_commen : 1: Write the jfs-program's title-comment to the         */
/*                  HTML-file.                                           */
/* conf_txt    : text written before confidence input-fields.            */
/* conf_max    : max-value confidence input.                             */
/* prefix_txt  : prefix all global JavaScript variables and functions    */
/*               with this text.                                         */
/* sout        : write messages, error-messages to the open file <sout>. */
/*                                                                       */
/* RETURN:                                                               */
/*   0: succes,                                                          */
/*   1, -1: error in conversion.                                         */


