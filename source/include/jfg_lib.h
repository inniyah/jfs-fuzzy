  /**************************************************************************/
  /*                                                                        */
  /* jfg_lib.h   Version  2.01    Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                                                        */
  /* C-library to get information about a compiled jfs-program.             */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/
#ifndef jfglibH
#define jfglibH

#include "jfs_cons.h"

struct jfg_sprog_desc  /* Static information about a jfs-program.  */
{
    unsigned long  asize;       /* Program-size in bytes.          */
    char           title[60];   /* program title.                  */
    signed short comment_no;    /* comment-no main-comment.        */
                                /* == -1: no comment.              */
    unsigned short comment_c;   /* number of comments.             */
    unsigned short domain_c;    /* Number of domains.              */
    unsigned short adjectiv_c;  /* Number of adjectives.           */
    unsigned short f_ivar_no;   /* Variable-no first input-var.    */
    unsigned short f_ovar_no;   /* Variable-no first output-var.   */
    unsigned short f_lvar_no;   /* Variable-no first local-var.    */
    unsigned short ivar_c;      /* Number of input variables.      */
    unsigned short ovar_c;      /* Number of output variables.     */
    unsigned short lvar_c;      /* Number of local varaibles.      */
    unsigned short var_c;       /* Number of (domain)-variables.   */
    unsigned short fzvar_c;     /* Number of fuzzy-variables.      */
    unsigned short array_c;     /* Number of arrays.               */
    unsigned short hedge_c;     /* Number of hedges.               */
    unsigned short relation_c;  /* Number of user-relations.       */
    unsigned short operator_c;  /* Number of operators.            */
    unsigned short function_c;  /* Number of user-functions.       */
    unsigned char  *pc_start;   /* Program-address first statement */
                                /* in main-program.                */
};


struct jfg_domain_desc /* Information about a domain:                  */
{
 	float          dmin;          /* domain's minimum-value.       */
	 float          dmax;          /* domain's maximum-value.       */
  unsigned short f_adjectiv_no; /* Adjectiv-no first adjectiv.   */
	 unsigned short adjectiv_c;    /* Number of adjectives.         */
	 signed short   comment_no;    /* comment_no.                   */
	 char           name[16];
	 char           unit[40];      /* unit-text.                    */
	 unsigned char type;           /* Domain-type (se below).       */
	 unsigned char flags;          /* Flags (se below).             */
};

/* <type> is one of:
        JFS_DT_FLOAT
        JFS_DT_INTEGER
        JFS_DT_CATEGORICAL
*/

/* <flags> is combined of:
        JFS_DF_MINENTER         Minimum-value entered.
        JFS_DF_MAXENTER         Maximum-value entered.
*/

/* Domain-no for predefined domains:
        JFS_DNO_FBOOL
        JFS_DNO_FLOAT
*/


struct jfg_adjectiv_desc /* Information about an adjective.           */
{
	float          center;		      /* Center value.                       */
	float          base;          /* Base value.                         */
	float          trapez_start;  /* Trapez start (x-value),             */
	float          trapez_end;    /* Trapez end   (x-value).             */
	char           name[16];
	signed short   comment_no;
	unsigned char  limit_c;       /* Number of points in pl-function.    */
	unsigned char  domain_no;
	unsigned char  h1_no;         /* Hedge-no first hedge.               */
	unsigned char  h2_no;         /* Hedge-no second hedge.              */
	unsigned char  flags;         /* Flags (se below).                   */
};

/* <flags> is combined of:
        JFS_AF_ICENTER      '%' before center value.
        JFS_AF_BASE         base entered.
        JFS_AF_IBASE        '%' before base value.
        JFS_AF_CENTER       center entered.
        JFS_AF_TRAPEZ       trapez entered.
        JFS_AF_HEDGE        use hedge.
        JFS_AF_ISTRAPEZ     '%' before trapez start-point.
        JFS_AF_IETRAPEZ     '%' before trapez end-point.
*/


struct jfg_limit_desc /* Information about a single point in a         */
                      /* pl-funtion (x:y).                             */
{
	float limit;             /* x-value limit.                         */
	float value;             /* y-value limit.                         */
	unsigned char exclusiv;  /* 1: exclusiv x-value.                   */
	unsigned char flags;
};

/* <flags> is combined of:
        JFS_LF_IX              '%' before x-value.
        JFS_LF_IY              '%' befoer y-value.
*/



struct jfg_var_desc   /* Information about a domain-variable           */
{
    float          acut;        /* Alfacut value.                */
    float          no_arg;      /* Nomalize value (if normalise  */
            				                /* and no_arg == 0 then 'normal  */
				                            /* conf').                       */
	float          defuz_arg;      /* Defuz-function argument.      */
	float          default_val;    /* (see 'default_type').         */
	unsigned short f_adjectiv_no;  /* Adjectiv-no first adjectiv.   */
	unsigned short f_fzvar_no;     /* Fuzzy-var-no first fzvar.     */
	unsigned short fzvar_c;        /* Number of fzvars (=number of  */
				                            /* adjectives).                  */
	signed short   comment_no;
	unsigned short argument;
	unsigned short default_no;
	char           name[16];
	char           text[60];
	unsigned char  vtype;          /* variable-type: (see below).   */
	unsigned char  default_type;   /* 0: no default entered,        */
                                /* 1: default value entered,     */
                                /* 2: adjective-default:         */
                                /* default_no + f_adjectiv_no    */
                                /* = adjectiv_no.                */
	char           f_comp;         /* fuzzy composite-operator-no.  */
	char           d_comp;         /* domain composite operator (see*/
				                            /* below).                       */
	char           defuz_1;        /* First defuz-func (see below). */
	char           defuz_2;        /* Second defuz-func (see below).*/
	unsigned char  domain_no;
	unsigned char  flags;          /* Flags (see below).            */
};

/* <vtype is one of:
        JFS_VT_INPUT
        JFS_VT_OUTPUT
        JFS_VT_LOCAL
*/

/* <flags> is combined of:
        JFS_VF_IACUT      '%' before acut-value.
        JFS_VF_IDEFUZ     '%' before defuz argument.
        JFS_VF_INORMAL    '%' before normal argument.
        JFS_VF_NORMAL     normalise.
*/

/* <defuz_1>, <defuz_2> is one of:
        JFS_VD_CENTROID
        JFS_VD_CMAX         Center-of-maximas.
        JFS_VD_AVG
        JFS_VD_FIRST
        JFS_VD_FMAX          firstmax
        JFS_VD_LMAX          lastmax
*/

/* <d_comp> is one of:
        JFS_VCF_NEW
        JFS_VCF_AVG
        JFS_VCF_MAX
*/



struct jfg_fzvar_desc /* Information about a fuzzy-variable            */
{
	unsigned short var_no;      /* domain-var-no                   */
	unsigned short adjectiv_no;
};



struct jfg_array_desc /* Information about an array                    */
{
	unsigned short array_c;  /* array-size.                        */
	signed short   comment_no;
	char           name[16];
};



struct jfg_hedge_desc /* Information about a hedge                     */
{
	float          hedge_arg;  /* Hedge argument.                      */
	signed short   comment_no;
	char           name[16];
	unsigned char  limit_c;    /* Number of points in pl-function.     */
	unsigned char  type;       /* Hedge-type (see below).              */
	unsigned char  flags;      /* Flags (see below).                   */
};

/* <type> is one of:
        JFS_HT_NEGATE
        JFS_HT_POWER
        JFS_HT_SIGMOID
        JFS_HT_ROUND
        JFS_HT_YNOT         yager_not
        JFS_HT_BELL
        JFS_HT_TCUT
        JFS_HT_BCUT
        JFS_HT_LIMITS       hedge defined by pl-function.
*/

/* <flags> is combined of:
       JFS_HF_IARG         '%' before hedge argument.
*/

/* Predefined hedges:
       JFS_HNO_NOT         "not"
*/




struct jfg_relation_desc  /* Information about a relation.             */
{
	signed short   comment_no;
	char           name[16];
	unsigned char  limit_c;     /* number of points in pl-function.   */
	unsigned char  hedge_no;
	unsigned char  flags;       /* See below.                         */
};

/* <flags> is combined of:
     JFS_RF_HEDGE          Hedge entererd.
*/




struct jfg_operator_desc /* Information about a operator.              */
{
	float          op_arg;        /* Argument operator.                */
	signed short   comment_no;
	char           name[16];
	unsigned char  op_1;          /* Operator 1 (see below).           */
	unsigned char  op_2;          /* Operator 2 (see below).           */
                               /* op_1 == op_2: only one operator.  */
 unsigned char  hedge_mode;
	unsigned char  hedge_no;
	unsigned char  precedence;
	unsigned char  flags;
};

/* <op_1> and <op_2> is one of:
        JFS_FOP_NONE
        JFS_FOP_MIN
        JFS_FOP_MAX
        JFS_FOP_PROD
        JFS_FOP_PSUM
        JFS_FOP_AVG
        JFS_FOP_BSUM
        JFS_FOP_NEW
        JFS_FOP_MXOR
        JFS_FOP_SPTRUE
        JFS_FOP_SPFALSE
        JFS_FOP_SMTRUE
        JFS_FOP_SMFALSE
        JFS_FOP_R0
        JFS_FOP_R1
        JFS_FOP_R2
        JFS_FOP_R3
        JFS_FOP_R4
        JFS_FOP_R5
        JFS_FOP_R6
        JFS_FOP_R7
        JFS_FOP_R8
        JFS_FOP_R9
        JFS_FOP_R10
        JFS_FOP_R11
        JFS_FOP_R12
        JFS_FOP_R13
        JFS_FOP_R14
        JFS_FOP_R15
        JFS_FOP_HAMAND
        JFS_FOP_HAMOR
        JFS_FOP_YAGERAND
        JFS_FOP_YAGEROR
        JFS_FOP_BUNION
        JFS_FOP_SIMILAR
*/

/* hedge_mode is one of:
        JFS_OHM_NONE
        JFS_OHM_ARG1      op(h(a),b),
        JFS_OHM_ARG2      op(a,h(b)),
        JFS_OHM_ARG12     op(h(a),h(b)),
        JFS_OHM_POST      h(op(a,b)).
*/

/* <flags> is combined of:
        JFS_OF_IARG         '%' before operator argument.
*/

/* Predefined operators:
        JFS_ONO_CASEOP            operator-number "caseop",
        JFS_ONO_WEIGHTOP                          "weightop",
        JFS_ONO_AND                               "and",
        JFS_ONO_OR                                "or",
        JFS_ONO_WHILEOP                           "whileop".
*/



struct jfg_function_desc    /* describes a user-defined function or */
                            /* procedure.                           */
{
	unsigned short arg_c;      /* Number of arguments to function.     */
	signed short   comment_no;
	char           name[16];
	unsigned char  *pc;        /* program-address first statement.     */
	unsigned char  type;       /* see below.                           */
};

/* <type> is one of:
         JFS_FT_PROCEDURE
         JFS_FT_FUNCTION
*/


struct jfg_func_arg_desc
{
	char          name[16];
};

struct jfg_statement_desc /* Information about a statement.            */
{
	float          farg;       /* Depends on <type> (see below).       */
	unsigned short sarg_1;     /* -"-                                  */
	unsigned short sarg_2;     /* -"-                                  */
	unsigned char  *n_pc;      /* Program-address next statement.      */
	signed   short comment_no;
	char           type;       /* Statement-type (see below).          */
	char           sec_type;   /* secondair statement-type (see below).*/
	char           flags;      /* See below.                           */
};

/* <type> is one of:                                                    */
#define JFG_ST_IF      0 /* 'ifw <rweight> <cond> then <then_expr>;' or */
			 /* 'if <cond> then <then_expr;'.               */
			 /* <farg>=<rweight>,                           */
			 /* <flags> bit 0 = 1: ifw-statement (else      */
			 /*                    if-statement.            */
			 /*             1 = 1: '%' before rule-weight.  */

	/* <sec_type> is one of:                                        */

#define JFG_SST_FZVAR 0

			 /* then_expr = <fzvar>.                        */
			 /* <sarg_1>=<fzvar>-number.                    */

#define JFG_SST_VAR   1
			 /* then expr = '<var> = <expr>',               */
			 /* <sarg_1>=<var>-number.                      */


#define JFG_SST_ARR   2
			 /*  then_expr = <arr>(<expr)=<expr>'.          */
			 /*   <sarg_1>=<array>-number.                  */


#define JFG_SST_INC   3
			 /* then_expr = 'increase <var> with <expr>',   */
			 /* or 'decrease <var> with <expr>'.            */
			 /* <flags> bit 2 = 0: increase-statement.      */
			 /*                 1: decrease-statemnet.      */
			 /*   <sarg_1>=<var>-number.                    */

#define JFG_SST_EXTERN  4
			 /* then_expr = 'extern <text>'.                */

#define JFG_SST_CLEAR   5
			 /* then_expr = 'clear <var>'.                  */
			 /*   <sarg_1>=<var>-number.                    */

#define JFG_SST_PROCEDURE 6
			 /* then_expr = '<proc>(<arg>, ..., <arg>)'     */
			 /* <sarg_1>=<proc>-number.                     */

#define JFG_SST_RETURN 7
			 /* then_expr = 'return <expr>'.                */

#define JFG_SST_FUARG   8
			 /* then expr = '<fu_arg> = <expr>',            */
			 /* <sarg_1>=<funarg_no>-number (in function).  */

#define JFG_ST_EOP     1 /* end of program.                             */

#define JFG_ST_CASE    2 /* case statement.                             */
			 /* flags bit 0 = 1: case from: 'switch <var>:' */

#define JFG_ST_STEND   3 /* 'end;'-statement.                           */
			 /* sec_type = 0: end-switch,                   */
			 /*            1: end-while,                    */
			 /*            2: end-function/procedure.       */

#define JFG_ST_SWITCH  4 /* switch statement.                           */
			 /* 'switch:' or 'switch <var>:'-statement'.    */
			 /* <flags> bit 0 = 1: 'switch <var>:'statement.*/
			 /* <sarg_1>=<var_no> (if 'switch <var>:').     */

#define JFG_ST_DEFAULT 5 /* Default statement.                          */

#define JFG_ST_WHILE   6 /* while statement.                            */

#define JFG_ST_WSET    7 /* wset-statement.                            */


struct jfg_tree_desc        /* Information about a node in a        */
{	                          /* statement-tree.                      */
 float          farg;       /* Depends on <type> (see below).       */
	unsigned short sarg_1;     /* -"-                                  */
	unsigned short sarg_2;     /* -"-                                  */
	char           type;       /* See below.                           */
	unsigned char  op;         /* Depends on <type> (see below).       */
};

/* <type> is one of:                                                   */

#define JFG_TT_OP    0 /* user-defined operator:         */
                       /* <expr_1> <op> <expr_2>         */
		       /* <op>     = operator-number.                  */
		       /* <sarg_1> = node_no <expr_1>.                 */
		       /* <sarg_2> = node_no <expr_2>.                 */

#define JFG_TT_HEDGE 1 /* hedge:  h(<expr>).                           */
		       /* <op>     = hedge_no.                         */
		       /* <sarg_1> = node_no <expr>.                   */

#define JFG_TT_UREL  2 /* user-defined relation:         */
                       /* <expr_1> <rel> <expr_2>        */
		       /* <op>     = relation_no.                      */
		       /* <sarg_1> = node_no <expr_1>.                 */
		       /* <sarg_2> = node_no <expr_2>.                 */

#define JFG_TT_SFUNC 3 /* single-argument predefined function: */
                       /*   f(<expr>)                          */
              		       /* <sarg_1> = node_no <expr>            */
              		       /* <op> is one of:
                             JFS_SFU_COS
                             JFS_SFU_SIN
                             JFS_SFU_TAN
                             JFS_SFU_ACOS
                             JFS_SFU_ASIN
                             JFS_SFU_ATAN
                             JFS_SFU_LOG
                             JFS_SFU_FABS
                             JFS_SFU_FLOOR
                             JFS_SFU_CEIL
                             JFS_SFU_NEGATE
                             JFS_SFU_RANDOM
                             JFS_SFU_SQR
                             JFS_SFU_SQRT
*/

#define JFG_TT_DFUNC 4 /* 2-arg predefined function:     */
         /* 'f(<expr_1>, <expr_2>)' or                   */
		       /* prediefined operator: <expr_1> <op> <expr_2>.*/
		       /* <sarg_1> = node_no <expr_1>.                 */
		       /* <sarg_2> = node_no <expr_2>.                 */
		       /* <op> is one of:
               JFS_DFU_PLUS
               JFS_DFU_MINUS
               JFS_DFU_PROD
               JFS_DFU_DIV
               JFS_DFU_POW
               JFS_DFU_MIN
               JFS_DFU_MAX
               JFS_DFU_CUT
               JFS_ROP_EQ
               JFS_ROP_NEQ
               JFS_ROP_GT
               JFS_ROP_GEQ
               JFS_ROP_LT
               JFS_ROP_LEQ
*/

#define JFG_TT_CONST 5 /* constant                             */
		       /* <farg> = constant-value                      */
		       /* <op> = 0: no '%',                            */
		       /*      = 1: '%' in front of constant.          */

#define JFG_TT_VAR   6 /* Domain-variable.                             */
		       /* <sarg_1> = variable-number.                  */

#define JFG_TT_FZVAR 7 /* fuzzy-variable.                              */
		       /* <sarg_1> = fuzzy-variable-number.            */

#define JFG_TT_TRUE 11 /* The constant 'true'.                         */

#define JFG_TT_FALSE 12   /* The constant 'false'.                     */


#define JFG_TT_BETWEEN 14 /* between:                                  */
			  /* <var> between <adj_1> and <adj_2>         */
			  /* <sarg_1> = domain-var_no,                 */
			  /* <sarg_2> = Relativ adjectiv-no <adj_1>:   */
			  /*           adj1_no = var.f_adj_no + arg_2. */
			  /* <op>    = Relativ adjectiv-no <adj_2>:    */
			  /*           adj2_no = var.f_adj_no + op.    */

#define JFG_TT_VFUNC 15   /* var-function  f(var)                      */
			  /* <sarg_1> = domain-var-no.                 */
			  /* <op>  is one of:
			    JFS_VFU_DNORMAL
			    JFS_VFU_M_FZVAR
			    JFS_VFU_S_FZVAR
			    JFS_VFU_DEFAULT
			    JFS_VFU_CONFIDENCE
*/

#define JFG_TT_UFUNC 16   /* user-function f(<expr>, <expr>,...)       */
                     			  /* <op>     = function-no.                   */
                     			  /* <sarg_1> = arglist.                       */

#define JFG_TT_ARGLIST 17 /* argumentliste:                            */
                     			  /*      <sarg_1> = node_no <expr>,           */
                     			  /*      <sarg_2> = node_no <arglist>.        */

#define JFG_TT_UF_VAR  18 /* userfunction variable:                    */
                     			  /* <sarg_1>      = arg_no (in function).     */

#define JFG_TT_IIF     19 /* iif(<cl>, <e1>, <e2>):                    */
                     			  /* <sarg_1> = cl,                            */
                     			  /* <sarg_2> = arglist (sarg_1=e1,            */
                       	  /*                     sarg_2=e2).           */

#define JFG_TT_ARVAL   21 /* array-value <arr>[<expr>]:                */
                     			  /* <op>     = <array_no>,                    */
                     			  /* <sarg_1> = node_no <expr>.                */

#define JFG_TT_EOE     22 /* not used in jfg_tree_desc, but in         */
			  /* jfg_oc_desc (see below).                  */

#define JFG_TT_EMPTY   23 /* not used by jfg_lib.                      */


struct jfg_oc_desc  /* low-level information about a single command in */
              		    /* an expression.                                  */
{
  float farg;
  unsigned char    *n_pc;      /* program-address next command.        */
  int               type;      /* command-type (see below).            */
  int               op;        /* depends on <type> (see below).       */
  unsigned short    sarg_1;    /*    -"-                               */
  unsigned short    sarg_2;    /*    -"-                               */
};


/* where <type> is one of:                                             */

/* JFG_TT_OP              operator:                                    */
		       /* <op>     = operator-number.                  */

/* JFG_TT_HEDGE           hedge:                                       */
		       /* <op>     = hedge_no.                         */

/* JFG_TT_UREL            user-relation:                               */
		       /* <op>     = relation_no.                      */

/* JFG_TT_SFUNC           single-argument function:                    */
		       /* <op> is one of: JFG_SFU_COS..JFG_SFU_RANDOM  */

/* JFG_TT_DFUNC           2-arg function or predefinde operator:       */
		       /* <op> is one of: JFG_DFU_PLUS..JFG_ROP_LEQ    */

/* JFG_TT_CONST           constant:                                    */
		       /* <farg> = constant-value                      */
		       /* <op> = 0: no '%',                            */
		       /*      = 1: '%' in front of constant.          */

/* JFG_TT_VAR             Domain-variable:                             */
		       /* <sarg_1> = variable-number.                  */

/* JFG_TT_FZVAR           fuzzy-variable:                              */
		       /* <sarg_1> = fuzzy-variable-number.            */

/* JFG_TT_TRUE            The constant 'true'.                         */

/* JFG_TT_FALSE           The constant 'false'.                        */


/* JFG_TT_BETWEEN         between:                                     */
			  /* <var> between <adj_1> and <adj_2>         */
			  /* <sarg_1> = var_no,                        */
			  /* <sarg_2> = Relativ adjectiv-no <adj_1>:   */
			  /*           adj1_no = var.f_adj_no + arg_2. */
			  /* <op>    = Relativ adjectiv-no <adj_2>:    */
			  /*           adj2_no = var.f_adj_no + op.    */

/* JFG_TT_VFUNC              var-function:                             */
			  /* <sarg_1> = domain-var-no.                 */
			  /* <op>  is one of: JFG_VFU_DNORMAL..        */

/* JFG_TT_UFUNC              user-function:                            */
			  /* <op>     = function-no.                   */

/* JFG_TT_UF_VAR             userfunction variable:                    */
			  /* <sarg_1>      = arg_no (in function).     */

/* JFG_TT_IIF                iif                                       */


/* JFG_TT_ARVAL              array-value:                              */
			  /* <op>     = <array>.                       */


/* JFG_TT_EOE                end of expression.                        */


/***********************************************************************/
/* funktioner                                                          */
/***********************************************************************/

int  jfg_init(int pmode, int stack_size, int precision);

/* Initialises the jfg_library. Should be called before the other      */
/* functions in the library is called. The arguments are:              */
/* <pmode>  : parentes-mode (when writing a statement as text). One of:*/
	      #define JFG_PM_NORMAL 0 /* only parentheses if needed,   */
	      #define JFG_PM_ALL    1 /* parentheses around all        */
                     				      /* expresions.                   */
/* <stack_size>: Size of the local conversion-stack (== 0: default     */
/*               value (64)).                                          */
/* <precision>:  The maximum number of digits after the decimal-point  */
/*               in statements converted to text (== 0: default (4)).  */
/* RETURNS:      0: ok,                                                */
/*               6: Cannot allocate memory to local-stack.             */

void jfg_sprg(struct jfg_sprog_desc *sprg, void *head);

/* return general information about the jfs-program identified by      */
/* <head>. The information is returned in the structure referenced by  */
/* <sprg>.                                                             */

void jfg_domain(struct jfg_domain_desc *ddesc, void *head,
                unsigned short domain_no);

/* Returns information about domain number <domain_no> in the          */
/* jfs-program <head>. The information is returned in the structure    */
/* refereced by <ddesc>.                                               */



void jfg_adjectiv(struct jfg_adjectiv_desc *adesc, void *head,
                  unsigned short adjectiv_no);

/* Returns information about adjectiv number <adjectiv_no> in the      */
/* jfs-program <head>. The information is returned in the structure    */
/* referenced by <adesc>.                                              */



void jfg_alimits(struct jfg_limit_desc *ldescs, void *head,
                 unsigned short adjectiv_no);

/* Returns information about the pl-function bound to an adjective.    */
/* <Adjectiv_no> is the adjectiv's number in the jfs-program <head>.   */
/* The information is placed in the array referenced by <adescs>       */
/* (the array-size must be greater than ajectiv.limit_c).              */



void jfg_var(struct jfg_var_desc *vdesc, void *head,
             unsigned short var_no);

/* Returns information about Domain-variable number <var_no> in the    */
/* jfs-program <head>. The information is returned in the structure    */
/* referenced by <vdesc>.                                              */



void jfg_fzvar(struct jfg_fzvar_desc *fzvdesc, void *head,
               unsigned short fzvar_no);

/* Returns information about fuzzy-variable number <fzvar_no> in the   */
/* jfs-program <head>. The information is returned in the structure    */
/* referenced by <fzvdesc>.                                            */



void jfg_array(struct jfg_array_desc *arrdesc, void *head,
               unsigned short array_no);

/* Returns information about array number <array_no> in th jfs-program */
/* <head>. The information is return in the structure referenced by    */
/* <arrdesc>.                                                          */



void jfg_hedge(struct jfg_hedge_desc *hdesc, void *head,
               unsigned short hedge_no);

/* Returns information about hedge number <hedge_no> in the            */
/* jfs-program <head>. The information is returned in the structure    */
/* referenced by <hdesc>.                                              */



void jfg_hlimits(struct jfg_limit_desc *ldescs, void *head,
                 unsigned short hedge_no);

/* Returns information about the pl-function bound to a hedge.         */
/* <hedge_no> is the number of the hedge in the jfs-program <head>.    */
/* The information is placed in the array referenced by <ldescs>       */
/* (the array-size must be greater than hedge.limit_c).                */



void jfg_relation(struct jfg_relation_desc *rdesc, void *head,
                  unsigned short relation_no);

/* Returns information about user-defined relation number              */
/* <relation_no> in the jfs-program <head>. The information is         */
/* returned in the structure referenced by <rdesc>.                    */



void jfg_rlimits(struct jfg_limit_desc *ldescs, void *head,
                 unsigned short relation_no);

/* Returns information about a relation's pl-function.                 */
/* <relation_no> is the reltaion's number in the jfs-program <head>.   */
/* The information is placed in the array referenced by <rdescs>       */
/* (the array-size must be greater than realtions.limit_c).            */



void jfg_operator(struct jfg_operator_desc *odesc, void *head,
                  unsigned short operator_no);

/* Returns information about user-defined operator number              */
/* <operator_no> in the jfs-program <head>. The information is         */
/* returned in the structure referenced by <odesc>.                    */



void jfg_function(struct jfg_function_desc *fdesc, void *head,
                  unsigned short function_no);

/* Returns information about user-defined function number              */
/* <function_no> in the jfs-program <head>. The information is         */
/* returned in the structure referenced by <fdesc>.                    */



void jfg_func_arg(struct jfg_func_arg_desc *farg, void *head,
                  unsigned short function_no, unsigned short arg_no);

/* Returns information about function-argument number <argument_no>    */
/* in function number <function_no>. The informationis returned in the */
/* structure <farg>.                                                   */



int jfg_comment(char *comment, int max_text,
                void *head, unsigned short comment_no);

/* Copy the text of comment number <comment_no> to the text-array      */
/* <comment>. Max <max_text> characters are copied.                    */
/* Return:   0: succes,                                                */
/*         301: comment longer than <max_text>                         */



int jfg_statement(struct jfg_statement_desc *stat, void *head,
                  unsigned char *pc);

/* returns information about the statement starting in                 */
/* program-address <pc> in the jfs-program <head>. The information is  */
/* returned in the structure referenced by <stat>.                     */
/* RETURN:    0: ok,                                                   */
/*          304: Illegal value of pc.                                  */



int jfg_t_statement(char *text, int maxtext, int aspaces,
                    struct jfg_tree_desc *tmp_tree,
                    unsigned short max_tree,
                    void *head, int func_no, unsigned char *pc);

/* Returns the text of a statement.                                    */
/* INPUT:                                                              */
/*    head	: pointer to the jfs-program.                               */
/*    func_no   : statement is placed in this function,                */
/*                = -1: main-program.                                  */
/*    pc        : The statement's starting program-address in the      */
/*                jfs-program <head>.                                  */
/*    text      : The char-array in which the text is placed.          */
/*    maxtext   : The maximum size of the text (=sizeof(<text>).       */
/*    aspaces   : Number of spaces in front of text in each line.      */
/*    tmp_tree  : An array of tree-desc used in conversion.            */
/*    max_tree  : The size of tmp-tree (100 is large).                 */
/* OUTPUT:                                                             */
/*    text      : The statement converted to text.                     */
/* RETURN:                                                             */
/*     0: no errors.                                                   */
/*   301: statement longer than <maxtext>.                             */
/*   302: <tmp_tree> is to small to hold the statement.                */
/*   303: stack-overflow (stack larger than <stack-size> in jfg_init() */
/*        needed).                                                     */
/*   304: <pc> is not the start of a statement.                        */




int jfg_a_statement(const char *argv[], int maxargc,
                    void *head, unsigned char *pc);

/* returns the arguments from the extern-part of an if-statement of    */
/* the type 'if <expr> then extern <arglist>; starting at              */
/* program-address <pc> in the jfs-program <head>. The arguments are   */
/* returned in the array <argv>, where <maxargc> is the size of the    */
/* array.                                                              */
/* RETURN:                                                             */
/*    >= 0: The number of arguments to the extern-statement.           */
/*      -1: Number of arguments larger than <maxargc>.                 */




int jfg_if_tree(struct jfg_tree_desc *jfg_tree,
                unsigned short max_tree,
                unsigned short *cond_tree,
                unsigned short *index_tree,
                unsigned short *expr_tree,
                void *head, unsigned char *pc);

/* Returns the expresions of the if/case/while/wset-statement starting */
/* at program-addresss <pc> in the program <head>. The expressions are */
/* returned as trees placed in the array <jfg_tree>, where <max_tree>  */
/* is the size of the array. <cond_tree> returns the root of the       */
/* conditional-part of the  statement (tree[cond_tree] = root). If the */
/* statement is of the type 'if <cond> then <var>=<expr>;' then        */
/* <expr_tree> returns the root of the expr-tree. If the statement is  */
/* of the type 'if <cond> then <arr>[<idexpr>]=<expr>;' then           */
/* <index_tree> returns the root of the <idexpr>-expression.           */
/* If the statement is of the type 'if <cond> then increase <v> with   */
/* <expr>', then <cond_tree> returns index of root of <cond> and       */
/* <expr_tree> returns index of <expr>. If the statement is of the     */
/* type 'if <cond> then return <expr>' then <cond_tree> returns index  */
/* of the root of <cond> and <expr_tree> index of <expr>.              */
/* RETURN:                                                             */
/*     0: no errors.                                                   */
/*   302: <jfg_tree> is to small to hold the expressions.              */
/*   303: stack-overflow (stack larger than <stack-size> in jfg_init() */
/*        needed).                                                     */
/*   304: <pc> is not the start of a statement or it is a              */
/*        extern-statement.                                            */


int jfg_oc(struct jfg_oc_desc *oc,
           void *head,
	       unsigned char *pc);

/* Returns the low-level command starting in program-adrress <pc> in   */
/* the jfs-program <head>. The command is placed in <oc>.              */


void jfg_free(void);

/* Frees memory allocated by jfg_init().                               */

#endif
