  /*********************************************************************/
  /*                                                                   */
  /* jft_lib.h   Version 2.03  Copyright (c) 1999-2000 Jan E. Mortensen*/
  /*                                                                   */
  /* JFS-Tokeniser. Functions to read jfs-values from an ascii-file    */
  /* or from text-strings.                                             */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#ifndef jftlibH
#define jftlibtH

/* Character types */

#define JFT_T_IGNORE    0
#define JFT_T_SPACE     1
#define JFT_T_LETTER    2
#define JFT_T_COMMENT   3   /* comment. Rest of line is ignored         */
#define JFT_T_QOUTE     4
#define JFT_T_NL        6   /* New line                                 */
#define JFT_T_EOF       7   /* End Of File.                             */
#define JFT_T_DEFAULT 127   /* No chartype (don't use).                 */

/* Token types     */

#define JFT_DM_NUMBER     0   /* Token contains a number.              */
#define JFT_DM_ADJECTIV   1   /* Token is an adjective name.           */
#define JFT_DM_INTERVAL   2   /* Token is a '*[:count]'.               */
#define JFT_DM_MMINTERVAL 3   /* Token is '*:count:min:max,.           */
#define JFT_DM_MISSING    4   /* Token is the character '?'.           */
#define JFT_DM_ERROR      5   /* Token does not cotain a meaningsfull  */
                     			      /* value.                                */
#define JFT_DM_END        6   /* End Of File.                          */

/* variable types */
#define JFT_VT_INPUT      0   /* the value of an input-variable,          */
#define JFT_VT_EXPECTED   1   /* the expected value of an output-var,     */
#define JFT_VT_LOCAL      2   /* local variable (NEVER!),                 */
#define JFT_VT_KEY        3   /* a non-variable token, key-value for      */
                              /* data-set,                                */
#define JFT_VT_ARG        4   /* a field with an argument other than <K>  */
                              /* (key).                                   */
#define JFT_VT_IGNORE     5   /* a non-variable token.                    */


struct jft_data_record
{                      float farg; /* data value.                         */
                       float conf; /* confidence.                         */
                       float imin; /* minimum-value if JFT_DM_MMINTERVAL. */
                       float imax; /* maximum-value if JFT_DM_MMINTERVAL. */
                       int   sarg; /* mode=JFT_DM_ADJECTIVE:adjective no  */
                                   /*                       (relative).   */
                                   /*      JFT_DM_MMINTERVAL,             */
                                   /*      JFT_DM_INTERVAL:count (-1,: no */
                                   /*        (count-value, -2: '*m').     */
                       int  mode;  /* one of token types JFT_DM_          */
                       char token[256];
                                   /* text-value                          */
                       int  vtype; /* one of JFT_VT_                      */
                       char vt_arg[8]; /* vt-argument (if JFT_VT_ARG).    */
                       int  vno;   /* variable-number (relative).         */

};

struct jft_error_record
{
  int error_no;
  int line_no;
  char carg[256];
};
extern struct jft_error_record jft_error_desc; /* error last jft-call.  */

struct jft_dset_record
{
  int record_size;     /* number of values pr record (incl key and ignore). */
  int record_count;    /* number of records in file.                        */
  int record_no;       /* current record-number (first is 1).               */
  int line_no;         /* current line-number (first is 1).                 */
  int field_no;        /* current field-no (the field just read).           */
  int input;           /* 1: input-data in record.                          */
  int expected;        /* 1: expected-data in record.                       */
  int key;             /* 1: key-field in record.                           */
  float max_value;     /* max float-value for input, expected variables.    */
};
extern struct jft_dset_record jft_dset_desc; /* describes the opened file */


/* possible error_no values:
{   0, "no error."},
{   1, "cannot open file: "},
{   2, "Error rewinding/reading from file."},
{   4, "Not a jfr-file."},
{   9, "Illegal number :"},
{  10, "value out of domain-range: "},
{  11, "Unexpected EOF."},
{  13, "Undefined adjective: "},
{  14, "Missing start/end of interval."},
{  15, "No value for variable:"},
{  16, "Too many values in a record (max 255)."},
{  17, "Illegal jft-file-mode."},
{  18, "Token to long (max 255 chars)."},
{  19, "Penalty-matrix and more than one output-variable."},
{  20, "Too many penalty-values (max 64)."},
{  21, "No values in first data-line."},
*/

/**********************************************************************/
/* Functions                                                          */
/**********************************************************************/

void jft_init(void *jf_head);

/* Initalises the tokeniser. This functions has to be called before    */
/* any of the other functions in jft_lib are called.                   */
/* The character types are initilised:                                 */
/* '#' = JFT_DM_COMMENT,                                               */
/* '\n' = JFT_DM_NL,                                                   */
/* '"', ''', = JFT_DM_QOUTE,                                           */
/* isspace(x): x = JFT_DM_SPACE,                                       */
/* all other characters are set to JFT_T_LETTER.                       */


int jft_fopen(char *fname, int fmode, int count_records);

/* Opens the file <fname> for reading values to the jfs-program         */
/* <jf_head>. <fmode> describes the contents of the file. See below for */
/* the possible values. If this call is succesfull, data can be         */
/* read from the file by jft_gettoken(), jft_getvar() etc.              */
/* The fields in <jft_data_desc> is assigned values.                    */
/* jft_data_desc.record_c is only assigned a value if <count_records>==1*/
/* return   0: ok                                                       */
/*          -1: error (see jft_error_desc for details).                 */

#define JFT_FM_NONE                      0
#define JFT_FM_INPUT                     100
#define JFT_FM_INPUT_EXPECTED            120
#define JFT_FM_INPUT_EXPECTED_KEY        123
#define JFT_FM_INPUT_KEY                 130
#define JFT_FM_EXPECTED                  200
#define JFT_FM_EXPECTED_INPUT            210
#define JFT_FM_EXPECTED_INPUT_KEY        213
#define JFT_FM_EXPECTED_KEY              230
#define JFT_FM_KEY_INPUT                 310
#define JFT_FM_KEY_EXPECTED              320
#define JFT_FM_KEY_INPUT_EXPECTED        312
#define JFT_FM_KEY_EXPECTED_INPUT        321
#define JFT_FM_FIRST_LINE                999

int jft_rewind(void);

/* Rewinds the file opened by jft_fopen.                               */
/* return   0: ok,                                                     */
/*         -1: error (see jft_error_desc for details).                 */



void jft_char_type(int char_no, int ctype);

/* Changes the character type of <char_no>. Character type is set to   */
/* <ctpye>, where <ctype> in JFT_T_IGNORE..JFT_T_DEFAULT.              */



void jft_close(void);

/* Closes the file opened by jft_fopen().                              */



int jft_atof(float *f, const char *a);

/* Converts the character-string in <a> to a floating-point number     */
/* in <f> (cannot handle exponential notation).                        */
/* return   0: succes,                                                 */
/*         -1: Illegal number.                                         */




int jft_atov(struct jft_data_record *dd, int var_no, char *a);

/* Converts the string <a> to a variable-value-describtion <dd>.       */
/* <a> is assumed to be a value for the variable <var_no> in the       */
/* jfs-program <jfr_head>.                                             */
/* return   0: succes,                                                 */
/*         -1: error (see jft_error_desc for details).                 */


void jft_dd_copy(struct jft_data_record *dest,
                 struct jft_data_record *source);

/* Copy <source> to <dest>.                                            */



int jft_gettoken(char *t);

/* Reads a token from the file opened by jft_fopen().                  */
/* return   0: succes,                                                 */
/*         11: EOF.                                                    */

int jft_getvar(struct jft_data_record *dd, int var_no);

/* Reads a variable-value from the file opened by jft_fopen() into the */
/* variable-value-describtion <dd>. The values is assumed to be value  */
/* for the variable number <var_no> (absolute number).                 */
/* return   0: succes,                                                 */
/*         11: Eof,                                                    */
/*         -1: error (see jft_error_desc for details).                 */


int jft_getdata(struct jft_data_record *dd);

/* Reads the next value from the file opened by jft_fopen() into the    */
/* data-description <dd>.                                               */
/* return  0: succes,                                                   */
/*        11: eof,                                                      */
/*        -1: error, see <jft_error_desc> for details.                  */


int jft_getrecord(float *ipvalues, float *ipconfs, float *expvalues,
                  char *key);

/* Reads the next record into <ipvalues>, <ipconfs> (confidences input),*/
/* <exp_values> (if expected) and the text-value of key-field is        */
/* written to <key>.                                                    */
/* Nothing is read into a variable with the value NULL.                 */
/* return   0: sucees,                                                  */
/*         11: eof,                                                     */
/*         -1: error (see jft_error_desc for details).                  */


int jft_penalty_read(char *fname);

/* Reads a penalty-matrix from the file with the name <fname>.          */
/* NOTE: jft_penalty_read() uses jft_fopen(), jft_get_token() etc, i.e. */
/* jft_penalty_read() has to be used after jft_init() is called and not */
/* between jft_fopen(), jft_close() for a data-file.                    */
/* Return:  >0: number of entries in penalty-matrix,                    */
/*          -1: error, see jft_error_desc.                              */

float jft_penalty_calc(float op_value, float exp_value);

/* Returns the penalty-score for output-values <op_value> and           */
/* expected-value <exp_value> using the penalty-matrix loaded by        */
/* jft_penalty_read().                                                  */



#endif

