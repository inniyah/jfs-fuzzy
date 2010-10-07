  /*********************************************************************/
  /*                                                                   */
  /* jfs_cons.h   Version  2.00   Copyright (c) 1999 Jan E. Mortensen  */
  /*                                                                   */
  /* JFS (defined) constants.                                          */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#ifndef jfsconsH
#define jfsconsH


/***********************************************************************/
/* Domains:                                                            */

/* domain-types:                                                       */
#define JFS_DT_FLOAT       0
#define JFS_DT_INTEGER     1
#define JFS_DT_CATEGORICAL 2

#define JFS_DT_COUNT 3  /* Number of domain-types.                     */

/* domain-flags:                                                       */
#define JFS_DF_MINENTER 1      /* Minimum-value entered.               */
#define JFS_DF_MAXENTER 2      /* Maximum-value entered.               */

/* Domain-no for predefined domains:                                   */
#define JFS_DNO_FBOOL   0
#define JFS_DNO_FLOAT   1

/***********************************************************************/


/***********************************************************************/
/* Adjectives:                                                         */

/* adjectiv-flags:                                                     */
#define JFS_AF_ICENTER      1   /* '%' before center value.            */
#define JFS_AF_BASE         2   /* base entered.                       */
#define JFS_AF_IBASE        4   /* '%' before base value.              */
#define JFS_AF_CENTER       8   /* center entered.                     */
#define JFS_AF_TRAPEZ      16   /* trapez entered.                     */
#define JFS_AF_HEDGE       32   /* use hedge.                          */
#define JFS_AF_ISTRAPEZ    64   /* '%' before trapez start-point.      */
#define JFS_AF_IETRAPEZ   128   /* '%' before trapez end-point.        */

/***********************************************************************/


/***********************************************************************/
/* Limits (plfs):                                                      */

/* Limit-flags:                                                        */
#define JFS_LF_IX     1         /* '%' before x-value.                 */
#define JFS_LF_IY     2         /* '%' befoer y-value.                 */



/***********************************************************************/
/* Variables:                                                          */

/* Variable-types:                                                     */
#define JFS_VT_INPUT    0
#define JFS_VT_OUTPUT   1
#define JFS_VT_LOCAL    2

#define JFS_VT_COUNT    3 /* Number of variable-types                  */

/* Variable-flags:                                                     */
#define JFS_VF_IACUT    1 /* '%' before acut-value.                    */
#define JFS_VF_IDEFUZ   2 /* '%' before defuz argument.                */
#define JFS_VF_INORMAL  4 /* '%' before normal argument.               */
#define JFS_VF_NORMAL   8 /* normalise.                                */

/* defuz-function types:                                               */
#define JFS_VD_CENTROID 0
#define JFS_VD_CMAX     1 /* Center-of-maximas.                        */
#define JFS_VD_AVG      2
#define JFS_VD_FIRST    3
#define JFS_VD_FMAX     4 /* firstmax                                  */
#define JFS_VD_LMAX     5 /* lastmax                                   */

#define JFS_VD_COUNT 6 /* Number of defuz-function-types.              */

/* domain composite-operator types:                                    */
#define JFS_VCF_NEW        0
#define JFS_VCF_AVG        1
#define JFS_VCF_MAX        2

#define JFS_VCF_COUNT 3 /* Numer of domain composite operator-types.   */

/* default-types:                                                      */
#define JFS_VDT_NONE       0   /* no default entered.                  */
#define JFS_VDT_VALUE      1   /* default-value entered.               */
#define JFS_VDT_ADJECTIV   2   /* default-valus is adjectiv value.     */

#define JFS_VDT_COUNT 3 /* number of default-types.                    */

/* Variable-statuses:                                                  */
#define JFS_VST_OK         0
#define JFS_VST_UNDEFINED  1
#define JFS_VST_FUZCHANGED 2
#define JFS_VST_DOMCHANGED 3

/***********************************************************************/


/***********************************************************************/
/* Hedges:                                                             */

/* hedge-types:                                                        */
#define JFS_HT_NEGATE  0
#define JFS_HT_POWER   1
#define JFS_HT_SIGMOID 2
#define JFS_HT_ROUND   3
#define JFS_HT_YNOT    4 /* yager_not */
#define JFS_HT_BELL    5
#define JFS_HT_TCUT    6
#define JFS_HT_BCUT    7
#define JFS_HT_LIMITS  8 /* hedge defined by pl-function.              */

#define JFS_HT_COUNT 9 /* Number of hedge-types.                       */

/* hedge-flags:                                                        */
#define JFS_HF_IARG 1           /* '%' before hedge argument.          */

/* Predefined hedges:                                                  */
#define JFS_HNO_NOT        0    /*              "not",                 */

/***********************************************************************/



/***********************************************************************/
/* Relations:                                                          */

/* relation-flags:                                                     */
#define JFS_RF_HEDGE 1    /* Hedge entererd.                           */

/***********************************************************************/



/***********************************************************************/
/* Operators:                                                          */

/* operator-types:                                                     */
#define JFS_FOP_NONE       0
#define JFS_FOP_MIN        1
#define JFS_FOP_MAX        2
#define JFS_FOP_PROD       3
#define JFS_FOP_PSUM       4
#define JFS_FOP_AVG        5
#define JFS_FOP_BSUM       6
#define JFS_FOP_NEW        7
#define JFS_FOP_MXOR       8
#define JFS_FOP_SPTRUE     9
#define JFS_FOP_SPFALSE   10
#define JFS_FOP_SMTRUE    11
#define JFS_FOP_SMFALSE   12
#define JFS_FOP_R0        13
#define JFS_FOP_R1        14
#define JFS_FOP_R2        15
#define JFS_FOP_R3        16
#define JFS_FOP_R4        17
#define JFS_FOP_R5        18
#define JFS_FOP_R6        19
#define JFS_FOP_R7        20
#define JFS_FOP_R8        21
#define JFS_FOP_R9        22
#define JFS_FOP_R10       23
#define JFS_FOP_R11       24
#define JFS_FOP_R12       25
#define JFS_FOP_R13       26
#define JFS_FOP_R14       27
#define JFS_FOP_R15       28
#define JFS_FOP_HAMAND    29
#define JFS_FOP_HAMOR     30
#define JFS_FOP_YAGERAND  31
#define JFS_FOP_YAGEROR   32
#define JFS_FOP_BUNION    33
#define JFS_FOP_SIMILAR   34

#define JFS_FOP_COUNT 35 /* Number of operator-types.                  */

/* operator-hedge-modes:                                               */
#define JFS_OHM_NONE      0  /* no hedge,                              */
#define JFS_OHM_ARG1      1  /* op(h(a),b),                            */
#define JFS_OHM_ARG2      2  /* op(a,h(b)),                            */
#define JFS_OHM_ARG12     3  /* op(h(a), h(b)),                        */
#define JFS_OHM_POST      4  /* h(op(a,b)).                            */

#define JFS_OHM_COUNT     5 /* Number of hedge-modes.                  */

/* operator-flags:                                                     */
#define JFS_OF_IARG       1  /* '%' before operator argument.          */
/* #define JFS_OF_HEDGE      2  Hedge entered.                         */

/* Predefined operators:                                               */
#define JFS_ONO_CASEOP     0   /* operator-number "caseop",            */
#define JFS_ONO_WEIGHTOP   1   /*                 "weightop",          */
#define JFS_ONO_AND        2   /*                 "and",               */
#define JFS_ONO_OR         3   /*                 "or",                */
#define JFS_ONO_WHILEOP    4   /*                 "whileop".           */

/***********************************************************************/

/***********************************************************************/
/* Reserved names:                                                     */

#define JFS_RES_COUNT   9

/***********************************************************************/
/* Functions:                                                          */

/* Function-types:                                                     */
#define JFS_FT_PROCEDURE 0
#define JFS_FT_FUNCTION  1

#define JFS_FT_COUNT 2 /* Number of function-types.                    */

/***********************************************************************/



/***********************************************************************/
/* Statement-elements:                                                 */

/* predefined single-argument functions:                               */
#define JFS_SFU_COS        0
#define JFS_SFU_SIN        1
#define JFS_SFU_TAN        2
#define JFS_SFU_ACOS       3
#define JFS_SFU_ASIN       4
#define JFS_SFU_ATAN       5
#define JFS_SFU_LOG        6
#define JFS_SFU_FABS       7
#define JFS_SFU_FLOOR      8
#define JFS_SFU_CEIL       9
#define JFS_SFU_NEGATE    10
#define JFS_SFU_RANDOM    11
#define JFS_SFU_SQR       12
#define JFS_SFU_SQRT      13
#define JFS_SFU_WGET      14

#define JFS_SFU_COUNT 15 /* umber of single-arg-functions.             */


/* predefined operators and relations:                                 */
#define JFS_DFU_PLUS       0
#define JFS_DFU_MINUS      1
#define JFS_DFU_PROD       2
#define JFS_DFU_DIV        3
#define JFS_DFU_POW        4
#define JFS_DFU_MIN        5
#define JFS_DFU_MAX        6
#define JFS_DFU_CUT        7  /* MISSING 8..9 */
#define JFS_ROP_EQ        10
#define JFS_ROP_NEQ       11
#define JFS_ROP_GT        12
#define JFS_ROP_GEQ       13
#define JFS_ROP_LT        14
#define JFS_ROP_LEQ       15

#define JFS_DFU_COUNT     16
/* predefined variable-functions:                                      */
#define JFS_VFU_DNORMAL    0
#define JFS_VFU_M_FZVAR    1   /* max(fzvars).                         */
#define JFS_VFU_S_FZVAR    2   /* sum(fzvars).                         */
#define JFS_VFU_DEFAULT    3
#define JFS_VFU_CONFIDENCE 4

#define JFS_VFU_COUNT      5

#endif
