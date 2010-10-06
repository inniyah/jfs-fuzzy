  /*********************************************************************/
  /*                                                                   */
  /* jfr_gen.h    Version  2.00   Copyright (c) 1999 Jan E. Mortensen  */
  /*                                                                   */
  /* Internal JFR-structures.                                          */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#ifndef jfrgenH
#define jfrgenH

/* JFR-commands: */

#define JFR_OP_FIRST  200  /* first number used as a command.            */
                           /* 200..214 reserved for future extensions.   */

#define JFR_OP_WSET      215  /* Weight-set (pop weight).                */                           
#define JFR_OP_SPOP      216  /* pop to stack[stack_id + <id(b)>].       */
#define JFR_OP_ENDFUNC   217  /* end a function/procedure.               */
#define JFR_OP_VDECREASE 218  /* decrease <var(b)>.                      */
#define JFR_OP_VINCREASE 219  /* increase <var(b)>.                      */
#define JFR_OP_VCASE     220  /* case til 'switch var'.                  */
#define JFR_OP_VSWITCH   221  /* switch <var(b)>.                        */
#define JFR_OP_SPUSH     222  /* push from stak[stack_id + <id(b)>].     */
#define JFR_OP_IIF       223  /* iif-function.                           */
#define JFR_OP_APOP      224  /* pop <array(b)>.                         */
#define JFR_OP_COMMENT   225  /* comment <commentno(b2)>.                */
#define JFR_OP_ICONST    226  /* %push <constant(f)>.                    */
#define JFR_OP_VPOP      227  /* pop <var(b)>.                           */
#define JFR_OP_FZVPOP    228  /* pop <fzvar(b12)>.                       */
#define JFR_OP_ENDSWITCH 229  /* end a switch block.                     */
#define JFR_OP_TRUE      230  /* push true (1).                          */
#define JFR_OP_FALSE     231  /* push false (0).                         */
#define JFR_OP_ENDWHILE  232  /* end a while-block.                      */
#define JFR_OP_EXPR      233  /* start expresion.                        */
#define JFR_OP_WEXPR     234  /* <lweight(f)> start expresion.           */
#define JFR_OP_AWEXPR    235  /* %<lweight(f)> start expresion.          */
#define JFR_OP_THENEXPR  236  /* start then-expresion.                   */
#define JFR_OP_VAR       237  /* push <var(b)>.                          */
#define JFR_OP_CONST     238  /* push <constant(f)>.                     */
#define JFR_OP_DFUNC     239  /* <func(b)>(a,b). (+, -, >, max etc).     */
#define JFR_OP_UREL      240  /* compare <defined relation(b)>.          */
#define JFR_OP_SFUNC     241  /* <func(b)>(a). (cos, sin, floor etc).    */
#define JFR_OP_ARRAY     242  /* push <array(b)>.                        */
#define JFR_OP_VFUNC     243  /* var-func <fno(b)>(<var(b)>).            */
#define JFR_OP_BETWEEN   244  /* between <var(b)> <cno(b)> <cno(b)>.     */
#define JFR_OP_CLEAR     245  /* clear <var(b)>.                         */
#define JFR_OP_SWITCH    246  /* switch-statement.                       */
#define JFR_OP_CASE      247  /* case (end of case).                     */
#define JFR_OP_OP        248  /* operator <opno(b)> .                    */
#define JFR_OP_FRETURN   249  /* return-statement in user-func.          */
#define JFR_OP_USERFUNC  250  /* user-func <fno(b)>.                     */
#define JFR_OP_DEFAULT   251  /* default-stmt.                           */
#define JFR_OP_EOP       252  /* end (end of program).                   */
#define JFR_OP_HEDGE     253  /* <hedge_no(b)>.                          */
#define JFR_OP_EXTERN    254  /* extern <bytes(b2)> <argc(b)> {<arg>}.   */
#define JFR_OP_WHILE     255  /* the statement while.                    */

struct jfr_comment_desc { unsigned short f_comment_id;
                          unsigned short comment_c;
                          /* the comment is in                           */
                          /* comment_block[f_comment_id...               */
                          /*               f_comment_id + comment_c-1].  */
			            };
#define JFR_COMMENT_SIZE sizeof(struct jfr_comment_desc)


struct jfw_synonym_desc { /* Only used in jfw-files.                      */
                          char name[32];
                          char value[96];
                    	};
#define JFW_SYNONYM_SIZE sizeof(struct jfw_synonym_desc)


struct jfr_domain_desc { float dmin;
                         float dmax;
                         char name[16];
                         char unit[40];
                         unsigned short f_adjectiv_no;
                         unsigned short adjectiv_c;
                         signed short   comment_no;  /* -1: no comment */
                         unsigned char type;    /* 0:float,        */
                                                /* 1:integer,      */
                                                /* 2: categorical. */
                         unsigned char flags;
                              /* bit 0 (1) = 1: dmin entered,      */
                              /* bit 1 (2) = 1: dmax entered,      */
		              };
#define JFR_DOMAIN_SIZE sizeof(struct jfr_domain_desc)

struct jfr_adjectiv_desc { float center;
                           float base;
                           float trapez_start;
                           float trapez_end;
                           unsigned short f_limit_no;
                           signed short comment_no;
                           char name[16];
                           unsigned char limit_c;
                           unsigned char domain_no;
                           unsigned char h1_no;
                           unsigned char h2_no;
                           unsigned char var_no;  /* kun hvis var-defined*/
                           unsigned char flags;
                                 /* bit 0 (1) = 1: center learn, */
                                 /* bit 1 (2) = 1: base entered, */
                                 /* bit 2 (4) = 1: base learn,   */
                                 /* bit 3 (8) = 1: center enter, */
                                 /* bit 4 (16)= 1: trapez enter, */
                                 /* bit 5 (32)= 1: use hedge,    */
                                 /* bit 6 (64)= 1: trap-st learn,*/
                                 /* bit 7 (128)=1: trap-end learn*/
                           unsigned char dum1;
                           unsigned char dum2;
		                 };
#define JFR_ADJECTIV_SIZE  sizeof(struct jfr_adjectiv_desc)

struct jfr_var_desc
      { float value;
		      float conf;
		      float conf_sum;
		      float no_arg;     /* normalise argument.        */
		      float acut;       /* alfa cut value.            */
		      float defuz_arg;
		      float default_val;
		      char name[16];
		      char text[60];
		      unsigned short f_adjectiv_no;
		      unsigned short f_fzvar_no;
		      unsigned short fzvar_c;   /* = adjectiv_count     */
		      signed short comment_no;
		      unsigned short argument;
		      unsigned char vtype;      /* variable-type.       */
		      unsigned char default_type;
		      unsigned char default_no; /* default = adjs[      */
            						                /* f_adj + default].cent*/
		      char f_comp;       /* fuzzy comp-operator.        */
		      char d_comp;       /* domain comp-operator        */
		      char defuz_1;
		      char defuz_2;
		      unsigned char domain_no;
		      unsigned char status;       /* 0: ok,                   */
                                    /* 1: undefined,            */
                                    /* 2: fuzzy-val changed.    */
                                    /* 3: domain-val changed.   */
		      unsigned char flags;
                                    /* bit 0 (1)  = 1: acut learn,         */
                                    /* bit 1 (2)  = 1: defuzarg learn.     */
                                    /* bit 2 (4)  = 1: no_arg learn.       */
                                    /* bit 3 (8) =  1: normalise.          */
  };
#define JFR_VAR_SIZE sizeof(struct jfr_var_desc)

struct jfr_fzvar_desc { float value;
                        unsigned short  var_no;
                        unsigned short  adjectiv_no;
                      };
#define JFR_FZVAR_SIZE sizeof(struct jfr_fzvar_desc)

struct jfr_array_desc { char name[16];
                        unsigned short array_c;
                        unsigned short f_array_id;
                        /* array is in array_vals[f_array_id...       */
                        /* ..f_array_id + array_c-1].                 */
                        signed short comment_no;
                        unsigned char domain_no;   /* not used in 2.0 */
                        unsigned char dummy;
		             };
#define JFR_ARRAY_SIZE sizeof(struct jfr_array_desc)

struct jfr_limit_desc { float limit;
                        float a;    /* f(x) = a * x + b  for x <= limit */
                        float b;
                        unsigned char exclusiv;
                        unsigned char flags;  /* bit 0 (1) = 1: learn  x */
                                              /* bit 1 (2) = 1: learn  y */
                        unsigned char dum1;
                        unsigned char dum2;
		              };
#define JFR_LIMIT_SIZE sizeof(struct jfr_limit_desc)

struct jfr_hedge_desc { float hedge_arg;
                        unsigned short f_limit_no;
                        signed short comment_no;
                        char   name[16];
                        unsigned char  limit_c;
                        unsigned char  type;  /* 0: negate,            */
                                              /* 1: power,             */
                                              /* 2: sigmoid,           */
                                              /* 3: round,             */
                                              /* 4: yager-not,         */
                                              /* 5: bell,              */
                                              /* 6: tcut,              */
                                              /* 7: bcut,              */
                                              /* 10: limit-func.       */
                        unsigned char flags;
                                       /* bit 0 = 1: learn hedge_arg.  */
                        char   dummy;
		               };
#define JFR_HEDGE_SIZE sizeof(struct jfr_hedge_desc)

struct jfr_relation_desc { unsigned short f_limit_no;
                           signed short comment_no;
                           char name[16];
                           unsigned char limit_c;
                           unsigned char hedge_no;
                           unsigned char flags;
                                         /* bit 0 (1) = 1 : hedge entered.   */
                           unsigned char dummy;
			             };
#define JFR_RELATION_SIZE sizeof(struct jfr_relation_desc)

struct jfr_function_desc   /* userdefined functions/procedures.           */
			 { unsigned short pc;
			   signed short comment_no;
			   unsigned short f_arg_no;  /* first func_arg_no */
			   unsigned char  arg_c;
			   unsigned char  type;  /* 0: procedure,         */
         						             /* 1: function.          */
			   char name[16];
			 };
#define JFR_FUNCTION_SIZE sizeof(struct jfr_function_desc)

struct jfr_funcarg_desc { char name[16];
                          unsigned char function_no;
                          unsigned char argtype;
                          unsigned char dum1;
                          unsigned char dum2;
			            };
#define JFR_FUNCARG_SIZE sizeof(struct jfr_funcarg_desc)

struct jfr_operator_desc { float op_arg;
                           char name[16];
                           signed short comment_no;
                           unsigned char op_1;
                           unsigned char op_2;
                           unsigned char hedge_mode;
                                         /* 0: no hedge,             */
                                         /* 1: arg1:  op(h(a), b)),  */
                                         /* 2: arg2:  op(a, h(b),    */
                                         /* 3: arg12: op(h(a), h(b)),*/
                                         /* 4: post:  h(op(a, b)).   */
                           unsigned char hedge_no;
                           unsigned char precedence;
                           unsigned char flags;
                                /* bit 0 (1) = 1: learn op_arg,     */
                                /* bit 1 (2) = 1: hedge entered,    */
                                /* bit 2 (4) = 1: ?? precedence ??  */
			             };
#define JFR_OPERATOR_SIZE sizeof(struct jfr_operator_desc)

/* extern header. Used in jfr-file. (Identical with first bytes of  */
/* jfr_head_desc.                                                   */
/*                                                                  */
struct jfr_ehead_desc
    { char check[4];               /* jfr                       */
      short version;               /* 200                       */
      short flags;
      unsigned long a_size;       /* prog.size excl head.      */
      char title[60];
      signed short comment_no;
      short vbytes;                /* no of bytes pr fzvar.     */
      unsigned short        s_size;    /* stack-size.           */
      unsigned short        domain_c;
      unsigned short        adjectiv_c;
      unsigned short        f_ivar_no;
      unsigned short        ivar_c;
      unsigned short        f_ovar_no;
      unsigned short        ovar_c;
      unsigned short        f_lvar_no;
      unsigned short        lvar_c;
      unsigned short        var_c;
      unsigned short        fzvar_c;
      unsigned short        array_c;
      unsigned short        arrayval_c;
      unsigned short        limit_c;
      unsigned short        hedge_c;
      unsigned short        relation_c;
      unsigned short        operator_c;
      unsigned short        comment_c;
      unsigned short        f_prog_comment; /* first comment program-blk*/
      unsigned short        com_block_c;
      unsigned short        function_c;
      unsigned short        funcarg_c;
      unsigned short        funccode_c;    /* function-block size    */
      unsigned short        program_c;     /* program-block size     */
					                                      /* (excl. function-block) */
      unsigned short        arg_1_c;
      unsigned short        arg_2_c;
   };
#define JFR_EHEAD_SIZE sizeof(struct jfr_ehead_desc)

/* Extern header. Used in jfw-files.                            */
/*                                                              */
struct jfw_head_desc
    { char check[4];               /* jfw                       */
      short version;               /* 200                       */
      short flags;
      unsigned long a_size;       /* not used.                 */
      char title[60];
      signed short comment_no;
      unsigned short        f_prog_line_no;  /* lineno first line in */
               					                         /* program-block.       */
      unsigned short        synonym_c;
      unsigned short        domain_c;
      unsigned short        adjectiv_c;
      unsigned short        ivar_c;
      unsigned short        ovar_c;
      unsigned short        lvar_c;
      unsigned short        var_c;
      unsigned short        array_c;
      unsigned short        limit_c;
      unsigned short        hedge_c;
      unsigned short        relation_c;
      unsigned short        operator_c;
      unsigned short        comment_c;     /* Only comments in declare */
					                                      /* blocks are included.     */
      unsigned short        com_block_c;
      unsigned short        arg_1_c;
      unsigned short        arg_2_c;
   };
#define JFW_HEAD_SIZE sizeof(struct jfw_head_desc)



struct jfr_head_desc
    { char check[4];               /* jfr                       */
      short version;               /* 200                       */
      short flags;
      unsigned long a_size;        /* prog.size excl head.      */
      char title[60];
      signed short comment_no;
      short vbytes;                /* no of bytes pr fzvar.     */
      unsigned short        s_size;    /* stack-size.           */
      unsigned short        domain_c;
      unsigned short        adjectiv_c;
      unsigned short        f_ivar_no;
      unsigned short        ivar_c;
      unsigned short        f_ovar_no;
      unsigned short        ovar_c;
      unsigned short        f_lvar_no;
      unsigned short        lvar_c;
      unsigned short        var_c;
      unsigned short        fzvar_c;
      unsigned short        array_c;
      unsigned short        arrayval_c;
      unsigned short        limit_c;
      unsigned short        hedge_c;
      unsigned short        relation_c;
      unsigned short        operator_c;
      unsigned short        comment_c;
      unsigned short        f_prog_comment; /* first comment program-blk*/
      unsigned short        com_block_c;
      unsigned short        function_c;
      unsigned short        funcarg_c;
      unsigned short        funccode_c;    /* function-block size */
      unsigned short        program_c;     /* program size (excl  */
					                                      /* function-block).    */
      unsigned short        arg_1_c;
      unsigned short        arg_2_c;

      short status;              /* 0: loaded,            */
                                 /* 1: initialised,       */
                                 /* 2: running.           */
      short dummy;
      struct jfr_comment_desc  *comments;
      struct jfr_domain_desc   *domains;
      struct jfr_adjectiv_desc *adjectives;
      struct jfr_var_desc      *vars;
      struct jfr_array_desc    *arrays;
      struct jfr_fzvar_desc    *fzvars;
      struct jfr_limit_desc    *limits;
      struct jfr_hedge_desc    *hedges;
      struct jfr_relation_desc *relations;
      struct jfr_operator_desc *operators;
      struct jfr_function_desc *functions;
      struct jfr_funcarg_desc  *func_args;
      float                    *array_vals;
      unsigned char            *function_code;
      unsigned char            *program_code;
      unsigned char            *comment_block;

      unsigned long         rsize;   /* reserved prog.size excl head */
  };
#define JFR_HEAD_SIZE sizeof(struct jfr_head_desc)


#endif