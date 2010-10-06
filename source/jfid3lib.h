
 /***************************************************************************/
  /*                                                                        */
  /* jfid3lib.h   Version  2.03  Copyright (c) 1998-2000 Jan E. Mortensen   */
  /*                                                                        */
  /* JFS Ruled discover using ID3.                                          */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#ifndef jfid3libH
#define jfid3libH

int jfid3_run(char *op_fname, char *ip_fname, char *da_fname, char *field_sep,
              int data_mode, long prog_size, long data_size, int res_confl,
              char *hfile_name, int hdests, int hrules, float min_score,
              char *sout_fname, int append, int batch);

/* jfid3_run load a compiled jfs-program from <ip_fname>, replaces a statement*/
/* of the form 'extern jfrd input {<vno>} output <vno>;' with rules           */
/* generated from the asci-data-file <da_fname>. The changed program is       */
/* written to the file <op_fname>. Parameters:                                */
/* sout_fname    : If <sout_fname>!="" then write run-messages etc to this    */
/*                 file, instead of writting to stdout.                       */
/* data_mode     : dataorder in data-file. One of:                            */
/*                 JFT_FM_INPUT_EXPECTED, JFT_FM_INPUT_EXPECTED_KEY,          */
/*                 JFT_FM_EXPECTED_INPUT, JFT_FM_EXPECTED_INPUT_KEY,          */
/*                 JFT_FM_KEY_INPUT_EXPECTED, JFT_FM_KEY_EXPECTED_INPUT,      */
/*                 JFT_FM_FIRST_LINE.                                         */
/* field_sep     : if != "" then use the chars in <field_sep> as seperators   */
/*                 in the data-file (instead of spaces, tabs).                */
/* prog_size     : Allocate <prog_size> extra bytes to program (to rules).    */
/* data_size     : Allocate <data_size> bytes to data.                        */
/* res_confl     : conflict resolve method:                                   */
/*                 0: score,                                                  */
/*                 1: count.                                                  */
/* append        : 0: overwrite <sout_fname>-file,                            */
/*                 1: append to file.                                         */
/* batch         : 0: Wait for the user to press RETURN before exit.          */
/* hfile_name    : filename history-file.                                     */
/* hdsets        : 0: don't write data-set-info to history-file,              */
/*                 1: write data-set-infó to history-file.                    */
/* hrules        : 0: don't write rule-info to history-file,                  */
/*                 1: write normal rule-info to history-file,                 */
/*                 2: write extended rule-info to history-file.               */
/* min_score     : Remove all rules with a score below <min_score>.           */

#endif
