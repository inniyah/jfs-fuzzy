  /**************************************************************************/
  /*                                                                        */
  /* jfrd_lib.h   Version  2.03  Copyright (c) 1998-2000 Jan E. Mortensen   */
  /*                                                                        */
  /* JFS to create a fuzzy system from data-sets using the Wang-Mendel      */
  /* method combined with an ad-hoc rule reduction method.                  */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#ifndef jfrdlibH
#define jfrdlibH

int jfrd_run(char *op_fname, char *ip_fname, char *sout_fname, char *da_fname,
             int data_mode, char *field_sep, long prog_size, long data_size,
             int max_time, int red_mode, int red_weight, float weight_value,
             int red_case, int res_confl_mode, int red_order, int def_fzvar,
             int append_mode, int silent, int batch);

/* jfrd_run load a compiled jfs-program from <ip_fname>, replaces a statement */
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
/* max_time      : Stop rewinding after <max_time> minutes.                   */
/* red_mode      : Data-reduction after Wang-Mendell:                         */
/*                 0: no reduction,                                           */
/*                 1: all-reduction,                                          */
/*                 2: between-reduction,                                      */
/*                 3: in-reduction,                                           */
/*                 4: in-between-reduction.                                   */
/* red_weight    : 1: create 'ifw %<wgt>'-statements.                         */
/* weight_val    : <wgt>-value in 'ifw %<wgt>'-statements.                    */
/* red_case      : 1: reduce to case-statements.                              */
/* res_confl_mode: conflict resolve method:                                   */
/*                 0: score,                                                  */
/*                 1: count.                                                  */
/* red_order     : 0: calculate order,                                        */
/*                 1: entered order.                                          */
/* def_fzvar     : 1: remove rules with default-output.                       */
/* append_mode   : 0: overwrite <sout_fname>-file,                            */
/*                 1: append to file.                                         */
/* silent        : 1: Don't write messages to stdout.                         */
/* batch         : 0: Wait for the user to press RETURN before exit.          */

#endif
