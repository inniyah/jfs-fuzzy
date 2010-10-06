  /**************************************************************************/
  /*                                                                        */
  /* jfg_lib.c   Version  2.01  Copyright (c) 1998-2000 Jan E. Mortensen    */
  /*                                                                        */
  /* C-library to get information about a compiled jfs-program.             */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_gen.h"
#include "jfg_lib.h"
#include "jfs_text.h"
#include "jfs_cons.h"

#define JFG_PT_ROUND  0  /* standard parentes:  '('  */
#define JFG_PT_SQUARE 1  /* bracket parentes:   '['  */

static char jfg_t_bpar[]   = "(";
static char jfg_t_epar[]   = ")";
static char jfg_t_then[]   = "then";
static char jfg_t_if[]     = "if";
static char jfg_t_ifw[]    = "ifw";
static char jfg_t_scolon[] = ";";
static char jfg_t_extern[]   = "extern";
static char jfg_t_true[]   = "true";
static char jfg_t_false[]  = "false";
static char jfg_t_end[]    = "end";
static char jfg_t_switch[] = "switch";
static char jfg_t_case[]   = "case";
static char jfg_t_default[]= "default";
static char jfg_t_while[]  = "while";
static char jfg_t_eq[]     = "=";
static char jfg_t_comma[]  = ",";
static char jfg_t_clear[]  = "clear";
static char jfg_t_between[]= "between";
static char jfg_t_and[]    = "and";
static char jfg_t_in[]     = "in ";
static char jfg_t_iif[]    = "iif(";
static char jfg_t_bspar[]  = "[";
static char jfg_t_espar[]  = "]";
static char jfg_t_increase[]="increase";
static char jfg_t_decrease[]="decrease";
static char jfg_t_with[]   = "with";
static char jfg_t_return[] = "return";
static char jfg_t_wset[]   = "wset";


struct jfg_dfunc_desc
        {       const char *name;
                int type;
                int precedence;
        };

/* type in */
#define JFG_DF_FUNC     0
#define JFG_DF_OPERATOR 1

static struct jfg_dfunc_desc
     jfg_t_dfuncs[] = {{ "+",     1, 50},   /*  0 */
                       { "-",     1, 50},   /*  1 */
                       { "*",     1, 60},   /*  2 */
                       { "/",     1, 60},   /*  3 */
                       { "pow(",  0, 0},    /*  4 */
                       { "min(",  0, 0},    /*  5 */
                       { "max(",  0, 0},    /*  6 */
                       { "cut(",  0, 0},    /*  7 */
                       { " ",     0, 0},    /*  8 */
                       { " ",     0, 0},    /*  9 */
                       { "==",    1, 40},   /* 10 */
                       { "!=",    1, 40},   /* 11 */
                       { ">",     1, 40},   /* 12 */
                       { ">=",    1, 40},   /* 13 */
                       { "<",     1, 40},   /* 14 */
                       { "<=",    1, 40}    /* 15 */
                      };

static  unsigned short jfg_fop_assos[] =   /* is operator associativ */
        {
   0,   /*  0: JFG_FOP_NONE */
   1,   /*  1: JFG_FOP_MIN  */
   1,   /*  2: JFG_FOP_MAX  */
   1,   /*  3: JFG_FOP_PROD */
   1,   /*  4: JFG_FOP_PSUM */
   0,   /*  5: JFG_FOP_AVG  */
   1,   /*  6: JFG_FOP_BSUM */
   0,   /*  7: JFG_FOP_NEW  */
   0,   /*  8: JFG_FOP_MXOR */
   0,   /*  9: JFG_FOP_SPTRUE ?  */
   0,   /* 10: JFG_FOP_SPFALSE ? */
   0,   /* 11: JFG_FOP_SMTRUE ?  */
   0,   /* 12: JFG_FOP_SMFALSE ? */
   1,   /* 13: JFG_FOP_R0        */
   0,   /* 14: JFG_FOP_R1        */
   0,   /* 15: JFG_FOP_R2 ?      */
   0,   /* 16: JFG_FOP_R3        */
   0,   /* 17: JFG_FOP_R4 ?      */
   0,   /* 18: JFG_FOP_R5        */
   1,   /* 19: JFG_FOP_R6        */
   0,   /* 20: JFG_FOP_R7        */
   1,   /* 21: JFG_FOP_R8        */
   1,   /* 22: JFG_FOP_R9        */
   1,   /* 23: JFG_FOP_R10       */
   0,   /* 24: JFG_FOP_R11       */
   1,   /* 25: JFG_FOP_R12       */
   0,   /* 26: JFG_FOP_R13 ?     */
   1,   /* 27: JFG_FOP_R14       */
   1,   /* 28: JFG_FOP_R15       */
   1,   /* 29: JFG_FOP_HAMAND    */
   1,   /* 30: JFG_FOP_HAMOR     */
   1,   /* 31: JFG_FOP_YAGERAND  */
   1,   /* 32: JFG_FOP_YAGEROR   */
   1,   /* 33: JFG_FOP_BUNION    */
   0    /* 34: JFG_FOP_SIMILAR ? */
        };

static  int jfg_maxstack = 0;
static  unsigned short *jfg_stack = NULL;
static  int jfg_ff_stack;

static  int jfg_ff_text, jfg_line_pos;
static  int jfg_aspaces;
static  int jfg_maxtext;

static  int jfg_func_no = -1;

static  int jfg_mode = 0; /* 0: Only Parentheses when needed,     */
                          /* 1: Parentheses around all operators. */

static  int jfg_precision = 4;

static  int jfg_err;      /* 301: statement longer than text.  */
                          /* 302: no free nodes in tree.       */
                          /* 303: stack-overflow.              */
                          /* 304: Illegal program-counter.     */

static void jfg_print(char *dtext, const char *text);
static void jfg_p_float(char *dtext, float arg, int learn);
static void jfg_p_fzvar(char *dtext, struct jfr_head_desc *jfr_head,
                        unsigned short fzvar_no);
static void jfg_p_tf(char *dtext, unsigned short tf);
static int jfg_in_test(struct jfr_head_desc *jfr_head,
                       struct jfg_tree_desc *jfg_tree, int ladr);
static void jfg_in_print(char *dtext, struct jfr_head_desc *jfr_head,
                         struct jfg_tree_desc *jfg_tree, int ladr);
static int jfg_precedence(struct jfr_head_desc *jfr_head,
                          int tree_adr, struct jfg_tree_desc *jfg_tree);

static int jfg_parentes(struct jfr_head_desc *jfr_head,
                        int tree_adr, int parent_adr,
                        struct jfg_tree_desc *jfg_tree, int left_leaf);
static void jfg_ic_pr(char *dtext, struct jfr_head_desc *jfr_head,
                      struct jfg_tree_desc *jfg_tree,
                      int tree_adr, int parent_op,
                      int left_leaf);

static unsigned char *jfg_skip_expr(struct jfr_head_desc *jfr_head,
                                    unsigned char *pc);
static void jfg_push(unsigned short val);
static unsigned short jfg_pop(void);

static void jfg_p_expr(char *text, struct jfr_head_desc *jfr_head,
                       struct jfg_tree_desc *tree, unsigned short ff_tree,
                       unsigned char *program_id);

/***********************************************************************/
/* fuctions to write text (called from jfg_t_statement).               */
/***********************************************************************/


static void jfg_print(char *dtext, const char *text)

   /*  concatenate text to dtext. If len(dtext) > 78 add newline, tab. */
{
  char prev_char, this_char;
  int len, m, nl, extra_space;


  len = strlen(text);
  if (len + ((int) strlen(dtext)) + jfg_aspaces + 5 >= jfg_maxtext)
  { jfg_err = 301;
    return;
  }
  nl = 0;
  if (text[0] == '\n' || text[0] == 10 || text[0] == 13)
    nl = 1;
  if (jfg_line_pos + len > 78)
  { dtext[jfg_ff_text++] = '\n';
    nl = 1;
  }
  if (nl == 1)
  { jfg_line_pos = 0;
    jfg_aspaces += 2;
  }
  while (jfg_line_pos < jfg_aspaces)
  { dtext[jfg_ff_text] = ' ';
    jfg_ff_text++;
    jfg_line_pos++;
  }
  if (jfg_ff_text != 0)
  { prev_char = dtext[jfg_ff_text - 1];
    this_char = text[0];
    extra_space = -1;
    if (prev_char == ' ' || this_char == ' ')
      extra_space = 0;
    if (extra_space == -1 && (prev_char == '(' || prev_char == '['))
      extra_space = 0;
    if (extra_space == -1 && prev_char == ',')
      extra_space = 1;
    if (extra_space == -1 && prev_char == '%')
      extra_space = 0;
    if (extra_space == -1 && this_char == ';')
      extra_space = 0;
    if (extra_space == -1 && this_char == ',')
      extra_space = 0;
    if (extra_space == -1 && (this_char == ')' || this_char == ']'))
      extra_space = 0;
    if (extra_space == -1)
      extra_space = 1;
    if (extra_space == 1)
    { dtext[jfg_ff_text++] = ' ';
      jfg_line_pos++;
    }
  }
  for (m = 0; m < len; m++)
  { dtext[jfg_ff_text] = text[m];
    jfg_ff_text++;
    jfg_line_pos++;
  }
  dtext[jfg_ff_text] = '\0';
}

static void jfg_tp_print(char *dtext, char *text, int par_type)
{
   /* prints <text> concatenats with '(' or '[' on dtext */

  char lt[80];

  strcpy(lt, text);
  if (par_type == JFG_PT_ROUND)
    strcat(lt, jfg_t_bpar);
  else
    strcat(lt, jfg_t_bspar);
  jfg_print(dtext, lt);
}

static void jfg_p_float(char *dtext, float arg, int learn)
{
  /* Concatenate the floating-point-number <arg> to <dtext>   */

  char from[50];
  char to[50];
  int ft, tt;

  ft = 0;
  tt = learn;
  to[0] = '%';

  switch (jfg_precision)
  { case 0:
      sprintf(from, "%20.0f ", arg);
      break;
    case 1:
      sprintf(from, "%20.1f ", arg);
      break;
    case 2:
      sprintf(from, "%20.2f ", arg);
      break;
    case 3:
      sprintf(from, "%20.3f ", arg);
      break;
    case 4:
      sprintf(from, "%20.4f ", arg);
      break;
    case 5:
      sprintf(from, "%20.5f ", arg);
      break;
    case 6:
      sprintf(from, "%20.6f ", arg);
      break;
    case 7:
      sprintf(from, "%20.7f ", arg);
      break;
    case 8:
      sprintf(from, "%20.8f ", arg);
      break;
    default:
      sprintf(from, "%20.9f ", arg);
      break;

  }
  while (from[ft] == ' ')
    ft++;
  while (from[ft] != ' ' && ft < 30)
  { to[tt] = from[ft];
    tt++; ft++;
  }
  to[tt] = '\0';

  /* Remove scaling zeros */
  tt--;
  while (tt > 0)
  { if (to[tt] == '0')
      to[tt] = '\0';
    else
    if (to[tt] == '.')
    { to[tt] = '\0';
      break;

    }
    else
      break;
    tt--;
  }
  jfg_print(dtext, to);
}

static void jfg_p_fzvar(char *dtext, struct jfr_head_desc *jfr_head,
   unsigned short fzvar_no)
{
  /* Concatenae the name of the fuzzy-variable <fzvar_no> to <dtext>. */
  unsigned short vno, m, ano;

  vno = jfr_head->fzvars[fzvar_no].var_no;
  jfg_print(dtext, jfr_head->vars[vno].name);
  m = fzvar_no - jfr_head->vars[vno].f_fzvar_no;
  ano = jfr_head->vars[vno].f_adjectiv_no + m;
  jfg_print(dtext, jfr_head->adjectives[ano].name);
}

static void jfg_p_funcarg(char *dtext, struct jfr_head_desc *jfr_head,
     unsigned short arg_no)
{
  int a;

  a = jfr_head->functions[jfg_func_no].f_arg_no + arg_no;
  jfg_print(dtext, jfr_head->func_args[a].name);
}

static void jfg_p_tf(char *dtext, unsigned short tf)
{
  /* Concatenate 'true' or 'false' to <dtext> */

  if (tf == JFG_TT_TRUE)
    jfg_print(dtext, jfg_t_true);
  else
  if (tf == JFG_TT_FALSE)
    jfg_print(dtext, jfg_t_false);
}

static int jfg_in_test(struct jfr_head_desc *jfr_head,
         struct jfg_tree_desc *jfg_tree, int ladr)
{
  /* Tests if the subtree starting in <ladr> can be reduced to a    */
  /* in-expresion. Returns the variable-number if it can be reduced */
  /* and -1 if not.                                                 */

  struct jfg_tree_desc *leaf;
  int in_var, t;

  in_var = -1;
  leaf = &(jfg_tree[ladr]);
  if (leaf->type == JFG_TT_FZVAR)
    in_var = jfr_head->fzvars[leaf->sarg_1].var_no;
  else
  if (leaf->type == JFG_TT_OP)
  { if (leaf->op == JFS_ONO_OR)
    { in_var = jfg_in_test(jfr_head, jfg_tree, leaf->sarg_1);
      if (in_var != -1)
      { t = jfg_in_test(jfr_head, jfg_tree, leaf->sarg_2);
        if (in_var != t)
          in_var = -1;
      }
    }
  }

  return in_var;
}

static void jfg_in_print(char *dtext, struct jfr_head_desc *jfr_head,
    struct jfg_tree_desc *jfg_tree, int ladr)
{
  /* Concatenates the inner part of the in-expresion starting in    */
  /* <ladr> to <dtext>.                                             */

  struct jfg_tree_desc *leaf;
  int in_var, adj_no;

  leaf = &(jfg_tree[ladr]);
  if (leaf->type == JFG_TT_FZVAR)
  { in_var = jfr_head->fzvars[leaf->sarg_1].var_no;
    adj_no = jfr_head->vars[in_var].f_adjectiv_no
             + leaf->sarg_1 - jfr_head->vars[in_var].f_fzvar_no;
    jfg_print(dtext, jfr_head->adjectives[adj_no].name);
  }
  else
  { jfg_in_print(dtext, jfr_head, jfg_tree, leaf->sarg_1);
    jfg_print(dtext, jfg_t_comma);
    jfg_in_print(dtext, jfr_head, jfg_tree, leaf->sarg_2);
  }
}

static int jfg_precedence(struct jfr_head_desc *jfr_head,
   int tree_adr, struct jfg_tree_desc *jfg_tree)
{
  /* Returns precedence of the operator in <tree_adr>.              */

  struct jfg_tree_desc *leaf;
  int precedence;

  precedence = -1;
  leaf = &(jfg_tree[tree_adr]);
  if (leaf->type == JFG_TT_OP)
    precedence = jfr_head->operators[leaf->op].precedence;
  else
  if (leaf->type == JFG_TT_DFUNC)
  { if (jfg_t_dfuncs[leaf->op].type == JFG_DF_OPERATOR)
      precedence = jfg_t_dfuncs[leaf->op].precedence;
  }
  else
  if (leaf->type == JFG_TT_UREL)
    precedence = 40;
  return precedence;
}

static int jfg_parentes(struct jfr_head_desc *jfr_head,
   int tree_adr, int parent_adr,
   struct jfg_tree_desc *jfg_tree, int left_leaf)
{
  /* Test if parenteses around <tree_adr> with parent <parent_adr> */

  int pri, lpri, parentes;
  struct jfg_tree_desc *leaf;
  struct jfg_tree_desc *pleaf;
  struct jfr_operator_desc *op;

  pri = jfg_precedence(jfr_head, tree_adr, jfg_tree);
  lpri = jfg_precedence(jfr_head, parent_adr, jfg_tree);

  parentes = 0;
  if (pri >= 0 && lpri >= 0 && (jfg_mode == 1 || pri < lpri))
    parentes = 1;
  else
  if (pri >= 0 && lpri >= 0 && pri == lpri && left_leaf == 0)
  { /* if same priority and rigth-leave it depends on the operator */
    pleaf = &(jfg_tree[parent_adr]);
    leaf = &(jfg_tree[tree_adr]);
    if (leaf->type != pleaf->type || leaf->op != pleaf->op)
      parentes = 1;
    else
    { switch (leaf->type)
      { case JFG_TT_DFUNC:
          if (leaf->op == JFS_DFU_PLUS || leaf->op == JFS_DFU_PROD)
            parentes = 0;
          else
            parentes = 1;
          break;
        case JFG_TT_OP:
          op = &(jfr_head->operators[leaf->op]);
          if (op->op_2 != JFS_FOP_NONE)
            parentes = 1;
          else
            parentes = 1 - jfg_fop_assos[op->op_1];
                   /* parentes if not asociativ */
          break;
        default:  /* JFG_TT_UREL */
          parentes = 1;
          break;
      }
    }
  }
  return parentes;
}

static void jfg_ic_pr(char *dtext,
       struct jfr_head_desc *jfr_head,
       struct jfg_tree_desc *jfg_tree,
       int tree_adr, int parent_adr,
       int left_leaf)
{
  unsigned short vno, bpar;
  struct jfg_tree_desc *leaf;
  int in_var;

  leaf = &(jfg_tree[tree_adr]);
  in_var = -1;
  if (leaf->type == JFG_TT_OP)
  { if (leaf->op == JFS_ONO_OR)
    { in_var = jfg_in_test(jfr_head, jfg_tree, tree_adr);
      if (in_var != -1)
      { jfg_print(dtext, jfr_head->vars[in_var].name);
        jfg_print(dtext, jfg_t_in);
        jfg_print(dtext, jfg_t_bpar);
        jfg_in_print(dtext, jfr_head, jfg_tree, tree_adr);
        jfg_print(dtext, jfg_t_epar);
      }
    }
  }
  if (in_var == -1)
  { bpar = jfg_parentes(jfr_head, tree_adr, parent_adr, jfg_tree, left_leaf);
    if (bpar == 1)
      jfg_print(dtext, jfg_t_bpar);
    switch (leaf->type)
    { case JFG_TT_OP:
        jfg_ic_pr(dtext, jfr_head, jfg_tree,
                  leaf->sarg_1, tree_adr, 1);
        jfg_print(dtext, jfr_head->operators[leaf->op].name);
        jfg_ic_pr(dtext, jfr_head,
                  jfg_tree, leaf->sarg_2, tree_adr, 0);
        break;
      case JFG_TT_HEDGE:
        jfg_tp_print(dtext, jfr_head->hedges[leaf->op].name, JFG_PT_ROUND);
        jfg_ic_pr(dtext, jfr_head, jfg_tree,
                  leaf->sarg_1, tree_adr, 1);
        jfg_print(dtext, jfg_t_epar);
        break;
      case JFG_TT_UREL: /* user defined relation */
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_1,
                  tree_adr, 1);
        jfg_print(dtext, jfr_head->relations[leaf->op].name);
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_2,
                  tree_adr, 0);
        break;
      case JFG_TT_SFUNC: /* single arg-function */
        jfg_tp_print(dtext, jfs_t_sfus[leaf->op], JFG_PT_ROUND);
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_1, tree_adr, 1);
        jfg_print(dtext, jfg_t_epar);
        break;
      case JFG_TT_DFUNC:  /* double function */
        if (jfg_t_dfuncs[leaf->op].type == JFG_DF_FUNC)
        { jfg_print(dtext, jfg_t_dfuncs[leaf->op].name);
          jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_1, tree_adr, 1);
          jfg_print(dtext, jfg_t_comma);
          jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_2, tree_adr, 0);
          jfg_print(dtext, jfg_t_epar);
        }
        else  /* predefined operator */
        { jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_1, tree_adr, 1);
          jfg_print(dtext, jfg_t_dfuncs[leaf->op].name);
          jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_2, tree_adr, 0);
        }
        break;
      case JFG_TT_CONST:
        jfg_p_float(dtext, leaf->farg, leaf->op);
        break;
      case JFG_TT_VAR:
        jfg_print(dtext, jfr_head->vars[leaf->sarg_1].name);
        break;
      case JFG_TT_FZVAR:
        jfg_p_fzvar(dtext, jfr_head, leaf->sarg_1);
        break;
      case JFG_TT_TRUE:
      case JFG_TT_FALSE:
        jfg_p_tf(dtext, leaf->type);
        break;
      case JFG_TT_BETWEEN:
        vno = leaf->sarg_1;
        jfg_print(dtext, jfr_head->vars[vno].name);
        jfg_print(dtext, jfg_t_between);
        jfg_print(dtext,
                  jfr_head->adjectives[jfr_head->vars[vno].f_adjectiv_no
                  + leaf->sarg_2].name);
        jfg_print(dtext, jfg_t_and);
        jfg_print(dtext,
                  jfr_head->adjectives[jfr_head->vars[vno].f_adjectiv_no
                          + (short) leaf->op].name);
        break;
      case JFG_TT_VFUNC: /* var function */
        jfg_tp_print(dtext, jfs_t_vfus[leaf->op], JFG_PT_ROUND);
        jfg_print(dtext, jfr_head->vars[leaf->sarg_1].name);
        jfg_print(dtext, jfg_t_epar);
        break;
      case JFG_TT_UFUNC: /* user-defined function */
        jfg_tp_print(dtext, jfr_head->functions[leaf->op].name, JFG_PT_ROUND);
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_1, tree_adr, 0);
        jfg_print(dtext, jfg_t_epar);
        break;
      case JFG_TT_ARGLIST: /* argument liste til user-function */
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_1, tree_adr, 0);
        jfg_print(dtext, jfg_t_comma);
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_2, tree_adr, 0);
        break;
      case JFG_TT_UF_VAR:  /* user-function variable            */
        jfg_p_funcarg(dtext, jfr_head, leaf->sarg_1);
        break;
      case JFG_TT_IIF:
        jfg_print(dtext, jfg_t_iif);
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_1, tree_adr, 0);
        jfg_print(dtext, jfg_t_comma);
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_2, tree_adr, 0);
        jfg_print(dtext, jfg_t_epar);
        break;
      case JFG_TT_ARVAL:
        jfg_tp_print(dtext, jfr_head->arrays[leaf->op].name, JFG_PT_SQUARE);
        jfg_ic_pr(dtext, jfr_head, jfg_tree, leaf->sarg_1, tree_adr, 0);
        jfg_print(dtext, jfg_t_espar);
        break;
    }
    if (bpar == 1)
      jfg_print(dtext, jfg_t_epar);
  }
}


/***********************************************************************/
/* functions used in parsing of statements                             */
/***********************************************************************/

unsigned char *jfg_skip_expr(struct jfr_head_desc *jfr_head,
      unsigned char *progid)
{
  /* program_id is pc of expr-statement after weigth-part.              */
  /* the function returns pc of pop-step (vpop, fzvpop, apop, while or  */
  /* case). */

  unsigned char op;
  int fundet;
  unsigned char *pc;

  pc = progid;
  fundet = 0;
  while (fundet == 0)
  { op = *pc;
    switch (op)
    { case JFR_OP_CONST:
      case JFR_OP_ICONST:
        pc++;
        pc += sizeof(float);
        break;
      case JFR_OP_HEDGE:
      case JFR_OP_DFUNC:
      case JFR_OP_SFUNC:
      case JFR_OP_UREL:
      case JFR_OP_OP:
      case JFR_OP_SPUSH:
        pc++;
        pc++;
        break;
      case JFR_OP_USERFUNC:
        pc++;
        if (jfr_head->functions[*pc].type == JFS_FT_PROCEDURE)
        { pc--;
          fundet = 1;
        }
        else
          pc++;
        break;
      case JFR_OP_VAR:
      case JFR_OP_ARRAY:
        pc++;
        pc++;
        break;
      case JFR_OP_TRUE:
      case JFR_OP_FALSE:
      case JFR_OP_THENEXPR:
      case JFR_OP_IIF:
        pc++;
     break;
      case JFR_OP_WSET:
      case JFR_OP_CASE:
      case JFR_OP_VCASE:
      case JFR_OP_WHILE:
      case JFR_OP_FZVPOP:
      case JFR_OP_VPOP:
      case JFR_OP_APOP:
      case JFR_OP_SPOP:
      case JFR_OP_VINCREASE:
      case JFR_OP_VDECREASE:
      case JFR_OP_FRETURN:
      case JFR_OP_ENDFUNC:
      case JFR_OP_CLEAR:
      case JFR_OP_EXTERN:
        fundet = 1;
        break;
      case JFR_OP_BETWEEN:
        pc++;
        pc++; /* var */
        pc++; /* cur_1 */
        pc++; /* cur_2 */
        break;
      case JFR_OP_VFUNC:
        pc++;
        pc++; /* functype */
        if (*pc == 255)
          pc++;
        pc++;
        break;
      default:   /* fzvar */
        pc++;
        if (jfr_head->vbytes == 2)
          pc++;
        break;
    }
  }
  return pc;
}

static void jfg_push(unsigned short val)
{
  if (jfg_ff_stack >= jfg_maxstack)
    jfg_err = 303;
  else
  { jfg_stack[jfg_ff_stack] = val;
    jfg_ff_stack++;
  }
}

static unsigned short jfg_pop(void)
{
  unsigned short res = 0;

  if (jfg_ff_stack > 0)
  { jfg_ff_stack--;
    res = jfg_stack[jfg_ff_stack];
  }
  return res;
}

/***********************************************************************/
/* External functions                                                  */
/***********************************************************************/

int  jfg_init(int pmode, int stack_size, int precision)
{
  jfg_mode = pmode;
  if (stack_size <= 0)
    jfg_maxstack = 64;
  else
    jfg_maxstack = stack_size;
  if ((jfg_stack = (unsigned short *)
                   malloc(jfg_maxstack * sizeof(short))) == NULL)
  { jfg_maxstack = 0;
    return 6;
  }
  if (precision > 0)
    jfg_precision = precision;
  return 0;
}


void jfg_sprg(struct jfg_sprog_desc *sprg, void *head)
{
  struct jfr_head_desc *jfr_head;

  jfr_head = (struct jfr_head_desc *) head;
  sprg->asize = jfr_head->a_size;
  strcpy(sprg->title, jfr_head->title);
  sprg->comment_no = jfr_head->comment_no;
  sprg->comment_c  = jfr_head->comment_c;
  sprg->domain_c   = jfr_head->domain_c;
  sprg->adjectiv_c = jfr_head->adjectiv_c;
  sprg->f_ivar_no  = jfr_head->f_ivar_no;
  sprg->f_ovar_no  = jfr_head->f_ovar_no;
  sprg->f_lvar_no  = jfr_head->f_lvar_no;
  sprg->ivar_c     = jfr_head->ivar_c;
  sprg->ovar_c     = jfr_head->ovar_c;
  sprg->lvar_c     = jfr_head->lvar_c;
  sprg->var_c      = jfr_head->var_c;
  sprg->fzvar_c    = jfr_head->fzvar_c;
  sprg->array_c    = jfr_head->array_c;
  sprg->hedge_c    = jfr_head->hedge_c;
  sprg->relation_c = jfr_head->relation_c;
  sprg->operator_c = jfr_head->operator_c;
  sprg->function_c = jfr_head->function_c;
  sprg->pc_start   = jfr_head->program_code;
}


void jfg_domain(struct jfg_domain_desc *ddesc, void *head,
  unsigned short domain_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_domain_desc *domain;

  jfr_head = (struct jfr_head_desc *) head;
  domain = &(jfr_head->domains[domain_no]);

  ddesc->dmin      = domain->dmin;
  ddesc->dmax      = domain->dmax;
  ddesc->f_adjectiv_no  = domain->f_adjectiv_no;
  ddesc->adjectiv_c     = domain->adjectiv_c;
  ddesc->comment_no     = domain->comment_no;
  ddesc->type           = domain->type;
  ddesc->flags          = domain->flags;
  strcpy(ddesc->name, domain->name);
  strcpy(ddesc->unit, domain->unit);
}

void jfg_adjectiv(struct jfg_adjectiv_desc *adesc, void *head,
   unsigned short adjectiv_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_adjectiv_desc *adjectiv;

  jfr_head = (struct jfr_head_desc *) head;
  adjectiv = &(jfr_head->adjectives[adjectiv_no]);

  adesc->center         = adjectiv->center;
  adesc->base           = adjectiv->base;
  adesc->trapez_start   = adjectiv->trapez_start;
  adesc->trapez_end     = adjectiv->trapez_end;
  adesc->domain_no      = adjectiv->domain_no;
  strcpy(adesc->name, adjectiv->name);
  adesc->limit_c        = adjectiv->limit_c;
  adesc->comment_no     = adjectiv->comment_no;
  adesc->h1_no          = adjectiv->h1_no;
  adesc->h2_no          = adjectiv->h2_no;
  adesc->flags          = adjectiv->flags;
}

void jfg_alimits(struct jfg_limit_desc *ldescs, void *head,
  unsigned short adjectiv_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_adjectiv_desc *adjectiv;
  struct jfr_limit_desc *limit;
  int m;

  jfr_head = (struct jfr_head_desc *) head;
  adjectiv = &(jfr_head->adjectives[adjectiv_no]);
  for (m = 0; m < adjectiv->limit_c; m++)
  { limit = &(jfr_head->limits[adjectiv->f_limit_no + m]);
    ldescs[m].limit = limit->limit;
    ldescs[m].value = limit->limit * limit->a + limit->b;
    if (ldescs[m].value < 0.0)
      ldescs[m].value = 0.0;
    else
    if (ldescs[m].value > 1.0)
      ldescs[m].value = 1.0;
    ldescs[m].exclusiv = limit->exclusiv;
    ldescs[m].flags    = limit->flags;
  }
}

void jfg_var(struct jfg_var_desc *vdesc, void *head,
     unsigned short var_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_var_desc *v;

  jfr_head = (struct jfr_head_desc *) head;
  v = &(jfr_head->vars[var_no]);

  vdesc->acut          = v->acut;
  vdesc->no_arg        = v->no_arg;
  vdesc->defuz_arg     = v->defuz_arg;
  vdesc->default_val   = v->default_val;
  vdesc->domain_no     = v->domain_no;
  vdesc->f_adjectiv_no = v->f_adjectiv_no;
  vdesc->f_fzvar_no    = v->f_fzvar_no;
  vdesc->fzvar_c       = v->fzvar_c;
  vdesc->comment_no    = v->comment_no;
  vdesc->default_type  = v->default_type;
  vdesc->default_no    = v->default_no;
  strcpy(vdesc->name, v->name);
  strcpy(vdesc->text, v->text);
  vdesc->vtype         = v->vtype;
  vdesc->defuz_1       = v->defuz_1;
  vdesc->defuz_2       = v->defuz_2;
  vdesc->f_comp        = v->f_comp;
  vdesc->d_comp        = v->d_comp;
  vdesc->flags         = v->flags;
  vdesc->argument      = v->argument;
}

void jfg_fzvar(struct jfg_fzvar_desc *fzvdesc, void *head,
       unsigned short fzvar_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_var_desc *var;

  jfr_head = (struct jfr_head_desc *) head;
  fzvdesc->var_no       = jfr_head->fzvars[fzvar_no].var_no;
  var = &(jfr_head->vars[fzvdesc->var_no]);
  fzvdesc->adjectiv_no  = var->f_adjectiv_no + fzvar_no - var->f_fzvar_no;
}

void jfg_array(struct jfg_array_desc *arrdesc, void *head,
        unsigned short array_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_array_desc *arr;

  jfr_head = (struct jfr_head_desc *) head;
  arr = &(jfr_head->arrays[array_no]);
  arrdesc->array_c = arr->array_c;
  arrdesc->comment_no = arr->comment_no;
  strcpy(arrdesc->name, arr->name);
}

void jfg_hedge(struct jfg_hedge_desc *hdesc, void *head,
       unsigned short hedge_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_hedge_desc *hedge;

  jfr_head = (struct jfr_head_desc *) head;
  hedge = &(jfr_head->hedges[hedge_no]);

  hdesc->hedge_arg     = hedge->hedge_arg;
  strcpy(hdesc->name, hedge->name);
  hdesc->type          = hedge->type;
  hdesc->limit_c       = hedge->limit_c;
  hdesc->comment_no    = hedge->comment_no;
  hdesc->flags         = hedge->flags;
}

void jfg_hlimits(struct jfg_limit_desc *ldescs, void *head,
  unsigned short hedge_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_hedge_desc *hedge;
  struct jfr_limit_desc *limit;
  int m;

  jfr_head = (struct jfr_head_desc *) head;
  hedge = &(jfr_head->hedges[hedge_no]);
  for (m = 0; m < hedge->limit_c; m++)
  { limit = &(jfr_head->limits[hedge->f_limit_no + m]);
    ldescs[m].limit = limit->limit;
    ldescs[m].value = limit->limit * limit->a + limit->b;
    if (ldescs[m].value < 0.0)
      ldescs[m].value = 0.0;
    else
    if (ldescs[m].value > 1.0)
      ldescs[m].value = 1.0;
    ldescs[m].exclusiv = limit->exclusiv;
    ldescs[m].flags = limit->flags;
  }
}

void jfg_relation(struct jfg_relation_desc *rdesc, void *head,
   unsigned short relation_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_relation_desc *rel;

  jfr_head = (struct jfr_head_desc *) head;
  rel = &(jfr_head->relations[relation_no]);

  rdesc->comment_no  = rel->comment_no;
  rdesc->limit_c     = rel->limit_c;
  strcpy(rdesc->name, rel->name);
  rdesc->hedge_no    = rel->hedge_no;
  rdesc->flags       = rel->flags;
}

void jfg_rlimits(struct jfg_limit_desc *ldescs, void *head,
  unsigned short relation_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_relation_desc *rel;
  struct jfr_limit_desc *limit;
  int m;

  jfr_head = (struct jfr_head_desc *) head;
  rel = &(jfr_head->relations[relation_no]);
  for (m = 0; m < rel->limit_c; m++)
  { limit = &(jfr_head->limits[rel->f_limit_no + m]);
    ldescs[m].limit = limit->limit;
    ldescs[m].value = limit->limit * limit->a + limit->b;
    if (ldescs[m].value < 0.0)
      ldescs[m].value = 0.0;
    else
    if (ldescs[m].value > 1.0)
      ldescs[m].value = 1.0;
    ldescs[m].exclusiv = limit->exclusiv;
    ldescs[m].flags    = limit->flags;
  }
}

void jfg_operator(struct jfg_operator_desc *odesc, void *head,
   unsigned short op_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_operator_desc *ope;

  jfr_head = (struct jfr_head_desc *) head;
  ope = &(jfr_head->operators[op_no]);

  odesc->op_arg      = ope->op_arg;
  strcpy(odesc->name, ope->name);
  odesc->op_1           = ope->op_1;
  odesc->op_2           = ope->op_2;
  odesc->hedge_mode     = ope->hedge_mode;
  odesc->hedge_no       = ope->hedge_no;
  odesc->comment_no     = ope->comment_no;
  odesc->precedence     = ope->precedence;
  odesc->flags          = ope->flags;
}

void jfg_func_arg(struct jfg_func_arg_desc *fadesc, void *head,
    unsigned short fno, unsigned short ano)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_function_desc *func;
  struct jfr_funcarg_desc *farg;

  jfr_head = (struct jfr_head_desc *) head;
  func = &(jfr_head->functions[fno]);
  farg = &(jfr_head->func_args[func->f_arg_no + ano]);
  strcpy(fadesc->name, farg->name);
}

void jfg_function(struct jfg_function_desc *fdesc, void *head,
    unsigned short function_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_function_desc *func;

  jfr_head = (struct jfr_head_desc *) head;
  func = &(jfr_head->functions[function_no]);
  fdesc->arg_c = func->arg_c;
  fdesc->comment_no = func->comment_no;
  fdesc->pc = jfr_head->function_code + func->pc;
  fdesc->type = func->type;
  strcpy(fdesc->name, func->name);
}

int jfg_comment(char *comment, int max_text,
                void *head, unsigned short comment_no)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_comment_desc *com;
  int m, res;

  jfr_head = (struct jfr_head_desc *) head;
  res = 0;
  com = &(jfr_head->comments[comment_no]);
  for (m = 0; res == 0 && m < com->comment_c; m++)
  { if (m >= max_text - 1)
      res = 301;
    else
      comment[m] = jfr_head->comment_block[com->f_comment_id + m];
  }
  comment[m] = '\0';
  return res;
}

int jfg_statement(struct jfg_statement_desc *stat, void *head,
                  unsigned char *program_id)
{
  struct jfr_head_desc *jfr_head;
  unsigned char *pc;
  int m, op, res;

  res = 0;
  jfr_head = (struct jfr_head_desc *) head;
  pc = program_id;
  op = *pc;
  pc++;
  stat->sec_type = 0;
  stat->flags = 0;
  stat->farg = 1.0;
  stat->comment_no = -1;
  switch (op)
  {
    case JFR_OP_WEXPR:
    case JFR_OP_AWEXPR:
      memcpy(&(stat->farg), pc, sizeof(float));
      pc += sizeof(float);
      stat->flags |= 1; /* rule weight */
      if (op == JFR_OP_AWEXPR)
        stat->flags |= 2; /* weight improve */
    case JFR_OP_EXPR:
      pc = jfg_skip_expr(jfr_head, pc);
      op = *pc;
      pc++;
      switch (op)
      { case JFR_OP_WSET:
          stat->type = JFG_ST_WSET;
          break;
        case JFR_OP_CASE:
          stat->type = JFG_ST_CASE;
          break;
        case JFR_OP_VCASE:
          stat->type = JFG_ST_CASE;
          stat->flags |= 1;
          break;
        case JFR_OP_WHILE:
          stat->type = JFG_ST_WHILE;
          break;
        case JFR_OP_USERFUNC:
          stat->type = JFG_ST_IF;
          stat->sec_type = JFG_SST_PROCEDURE;
          stat->sarg_1 = *pc;
          pc++;
          break;
        case JFR_OP_VPOP:
        case JFR_OP_CLEAR:
        case JFR_OP_VINCREASE:
        case JFR_OP_VDECREASE:
        case JFR_OP_APOP:
          stat->type = JFG_ST_IF;
          if (op == JFR_OP_VPOP)
            stat->sec_type = JFG_SST_VAR;
          else if (op == JFR_OP_CLEAR)
            stat->sec_type = JFG_SST_CLEAR;
          else if (op == JFR_OP_VINCREASE)
            stat->sec_type = JFG_SST_INC;
          else if (op == JFR_OP_VDECREASE)
          { stat->sec_type = JFG_SST_INC;
            stat->flags |= 4;
          }
          else if (op == JFR_OP_APOP)
            stat->sec_type = JFG_SST_ARR;
          stat->sarg_1 = *pc;
          pc++;
          break;
        case JFR_OP_SPOP:
          stat->type = JFG_ST_IF;
          stat->sec_type = JFG_SST_FUARG;
          m = *pc;
          stat->sarg_1 = m;
          pc++;
          break;
        case JFR_OP_FZVPOP:
          stat->type = JFG_ST_IF;
          stat->sec_type = JFG_SST_FZVAR;
          m = *pc;
          pc++;
          if (jfr_head->vbytes == 2)
          { m = 256 * m + (int) *pc;
            pc++;
          }
          stat->sarg_1 = m;
          break;
        case JFR_OP_EXTERN:
          m = 256 * (int) (*pc);
          pc++;
          m += *pc;
          pc++;
          stat->type = JFG_ST_IF;
          stat->sec_type = JFG_SST_EXTERN;
          stat->sarg_1 = *pc; /* argc */
          pc++;
          pc += m;
          break;
        case JFR_OP_FRETURN:
          stat->type = JFG_ST_IF;
          stat->sec_type = JFG_SST_RETURN;
          break;
      }
      break;
    case JFR_OP_DEFAULT:
      stat->type = JFG_ST_DEFAULT;
      stat->farg = 0.0;
      break;
    case JFR_OP_ENDWHILE:
      stat->type = JFG_ST_STEND;
      stat->sec_type = 1;
      break;
    case JFR_OP_ENDSWITCH:
      stat->type = JFG_ST_STEND;
      break;
    case JFR_OP_ENDFUNC:
      stat->type = JFG_ST_STEND;
      stat->sec_type = 2;
      break;
    case JFR_OP_EOP:
      stat->type = JFG_ST_EOP;
      break;
    case JFR_OP_SWITCH:
      stat->type = JFG_ST_SWITCH;
      stat->flags = 0;
      break;
    case JFR_OP_VSWITCH:
      stat->type = JFG_ST_SWITCH;
      stat->flags = 1;
      stat->sarg_1 = *pc;
      pc++;
      break;
    default:
      res = 304; /* ulovlig program id */
      break;
  } /* switch */
  if (*pc == JFR_OP_COMMENT)
  { pc++;
    m = *pc;
    pc++;
    stat->comment_no = 256 * m + (int) *pc;
    pc++;
  }
  stat->n_pc = pc;
  return res;
}


int jfg_if_tree(struct jfg_tree_desc *jfg_tree,
                unsigned short max_tree,
                unsigned short *cond_tree,
                unsigned short *index_tree,
                unsigned short *expr_tree,
                void *head, unsigned char *program_id)
{
  struct jfr_head_desc *jfr_head;
  unsigned char *pc;
  int m, first, husk, a;
  unsigned short jfg_ff_tree;
  int res;
  struct jfg_tree_desc *leaf;
  unsigned char op;

  jfr_head = (struct jfr_head_desc *) head;
  jfg_err = 0;
  pc = program_id;
  op = *pc;
  husk = 0;
  if (op == JFR_OP_EXPR || op == JFR_OP_WEXPR || op == JFR_OP_AWEXPR)
  { pc++;
    if (op == JFR_OP_WEXPR || op == JFR_OP_AWEXPR)
      pc += sizeof(float);
  }
  res = 1;
  jfg_ff_stack = 0;
  jfg_ff_tree = 0;
  while (res != 0)
  { op = *pc;
    leaf = &(jfg_tree[jfg_ff_tree]);
    if (jfg_ff_tree  >= max_tree)
    { jfg_err = 302;
      return jfg_err;
    }
    leaf->op = 0;
    pc++;
    switch (op)
    { case JFR_OP_VAR:
        leaf->type = JFG_TT_VAR;
        leaf->sarg_1 = *pc;
        pc++;
        break;
      case JFR_OP_ARRAY:
        leaf->type = JFG_TT_ARVAL;
        leaf->op = *pc;
        pc++;
        leaf->sarg_1 = jfg_pop();
        break;
      case JFR_OP_SPUSH:
        leaf->type = JFG_TT_UF_VAR;
        leaf->sarg_1 = *pc;
        pc++;
        break;
      case JFR_OP_IIF:
        leaf->type = JFG_TT_ARGLIST;
        leaf->sarg_2 = jfg_pop();
        leaf->sarg_1 = jfg_pop();
        m = jfg_ff_tree;
        jfg_ff_tree++;
        if (jfg_ff_tree >= max_tree)
        { jfg_err = 302;
          return jfg_err;
        }
        leaf = &(jfg_tree[jfg_ff_tree]);
        leaf->type = JFG_TT_IIF;
        leaf->sarg_1 = jfg_pop();
        leaf->sarg_2 = m;
        break;
      case JFR_OP_TRUE:
        leaf->type = JFG_TT_TRUE;
        break;
      case JFR_OP_FALSE:
        leaf->type = JFG_TT_FALSE;
        break;
      case JFR_OP_CONST:
      case JFR_OP_ICONST:
        memcpy(&(leaf->farg), pc, sizeof(float));
        pc += sizeof(float);
        leaf->type = JFG_TT_CONST;
        if (op == JFR_OP_CONST)
          leaf->op = 0;
        else
         leaf->op = 1;
        break;
      case JFR_OP_UREL:
        leaf->type = JFG_TT_UREL;
        leaf->op = *pc;
        pc++;
        leaf->sarg_2 = jfg_pop();
        leaf->sarg_1 = jfg_pop();
        break;
      case JFR_OP_DFUNC:
        leaf->type = JFG_TT_DFUNC;
        leaf->op = *pc;
        pc++;
        leaf->sarg_2 = jfg_pop();
        leaf->sarg_1 = jfg_pop();
        break;
      case JFR_OP_SFUNC:
        leaf->type = JFG_TT_SFUNC;
        leaf->op = *pc;
        pc++;
        leaf->sarg_1 = jfg_pop();
        break;
      case JFR_OP_USERFUNC:
        m = *pc;
        pc++;
        a = jfr_head->functions[m].arg_c;
        if (a == 1)
        { leaf->type = JFG_TT_UFUNC;
          leaf->op = m;
          leaf->sarg_1 = jfg_pop();
        }
        else
        { first = 1;
          while (a > 0)
          { leaf->type = JFG_TT_ARGLIST;
            if (first == 1)  /* sidste leaf i argliste */
            { leaf->sarg_2 = jfg_pop();
              leaf->sarg_1 = jfg_pop();
              first = 0;
              a -= 2;
            }
            else
            { leaf->sarg_1 = jfg_pop();
              leaf->sarg_2 = husk;
              a--;
            }
            husk = jfg_ff_tree;
            jfg_ff_tree++;
            if (jfg_ff_tree  >= max_tree)
            { jfg_err = 302;
              return jfg_err;
            }
            leaf = &(jfg_tree[jfg_ff_tree]);
          }
          leaf->type = JFG_TT_UFUNC;
          leaf->op = m;
          leaf->sarg_1 = husk;
        }
        if (jfr_head->functions[m].type == JFS_FT_PROCEDURE)
        { res = 0;
          jfg_push(jfg_ff_tree);
          jfg_ff_tree++;
          *expr_tree = jfg_stack[0];
        }
        break;
      case JFR_OP_HEDGE:
        leaf->type = JFG_TT_HEDGE;
        leaf->op   = *pc;
        pc++;
        leaf->sarg_1 = jfg_pop();
        break;
      case JFR_OP_OP:
        leaf->type = JFG_TT_OP;
        leaf->sarg_2 = jfg_pop();;
        leaf->sarg_1 = jfg_pop();
        leaf->op = *pc;
        pc++;
        break;
      case JFR_OP_CASE:
      case JFR_OP_WSET:
      case JFR_OP_WHILE:
      case JFR_OP_FZVPOP:
      case JFR_OP_VCASE:
      case JFR_OP_CLEAR:
      case JFR_OP_EXTERN:
        res = 0;
        *cond_tree = jfg_stack[0];
        break;
      case JFR_OP_THENEXPR:
        *cond_tree = jfg_stack[0];
        jfg_ff_stack = 0;
        break;
      case JFR_OP_VPOP:
      case JFR_OP_SPOP:
      case JFR_OP_VINCREASE:
      case JFR_OP_VDECREASE:
        *expr_tree = jfg_stack[0];
        res = 0;
        break;
      case JFR_OP_APOP:
        *expr_tree = jfg_stack[1];
        *index_tree = jfg_stack[0];
        res = 0;
        break;
      case JFR_OP_FRETURN:
        *expr_tree = jfg_stack[0];
        res = 0;
        break;
      case JFR_OP_BETWEEN:
        leaf->type = JFG_TT_BETWEEN;
        m = *pc;
        pc++;
        leaf->sarg_1 = m;
        leaf->sarg_2 = *pc;
        pc++;
        leaf->op = *pc;
        pc++;
        break;
      case JFR_OP_VFUNC:
        leaf->type = JFG_TT_VFUNC;
        leaf->op = *pc;
        pc++;
        leaf->sarg_1 = *pc;
        pc++;
        break;
      default:   /* fzvar */
        leaf->type = JFG_TT_FZVAR;
        m = op;
        if (jfr_head->vbytes == 2)
        { m = m * 256 + (int) *pc;
          pc++;
        }
        leaf->sarg_1 = m;
        op = 0;
        break;
    }
    if (op != JFR_OP_THENEXPR && res != 0)
    { jfg_push(jfg_ff_tree);
      jfg_ff_tree++;
    }
  }
  jfg_ff_stack = 0;
  return jfg_err;
}

int jfg_oc(struct jfg_oc_desc *oc,
        void *head, unsigned char *program_id)
{
  unsigned char *pc;
  struct jfr_head_desc *jfr_head;
  unsigned char op;
  int m;

  pc = program_id;
  jfr_head = (struct jfr_head_desc *) head;
  op = *pc;
  if (op == JFR_OP_EXPR || op == JFR_OP_WEXPR || op == JFR_OP_AWEXPR
      || op == JFR_OP_THENEXPR)
  { pc++;
    if (op == JFR_OP_WEXPR || op == JFR_OP_AWEXPR)
      pc += sizeof(float);
  }
  op = *pc;
  pc++;
  switch (op)
  { case JFR_OP_VAR:
      oc->type = JFG_TT_VAR;
      oc->sarg_1 = *pc;
      pc++;
      break;
    case JFR_OP_ARRAY:
      oc->type = JFG_TT_ARVAL;
      oc->op = *pc;
      pc++;
      break;
    case JFR_OP_SPUSH:
      oc->type = JFG_TT_UF_VAR;
      oc->sarg_1 = *pc;
      pc++;
      break;
    case JFR_OP_IIF:
      oc->type = JFG_TT_ARGLIST;
      break;
    case JFR_OP_TRUE:
   oc->type = JFG_TT_TRUE;
   break;
    case JFR_OP_FALSE:
   oc->type = JFG_TT_FALSE;
   break;
    case JFR_OP_CONST:
    case JFR_OP_ICONST:
      memcpy(&(oc->farg), pc, sizeof(float));
      pc += sizeof(float);
      oc->type = JFG_TT_CONST;
      if (op == JFR_OP_CONST)
        oc->op = 0;
      else
        oc->op = 1;
      break;
    case JFR_OP_UREL:
      oc->type = JFG_TT_UREL;
      oc->op = *pc;
      pc++;
      break;
    case JFR_OP_DFUNC:
      oc->type = JFG_TT_DFUNC;
      oc->op = *pc;
      pc++;
      break;
    case JFR_OP_SFUNC:
      oc->type = JFG_TT_SFUNC;
      oc->op = *pc;
      pc++;
      break;
    case JFR_OP_USERFUNC:
      oc->type = JFG_TT_UFUNC;
      oc->op = *pc;
      pc++;
      break;
    case JFR_OP_HEDGE:
      oc->type = JFG_TT_HEDGE;
      oc->op   = *pc;
      pc++;
      break;
    case JFR_OP_OP:
      oc->type = JFG_TT_OP;
      oc->op = *pc;
      pc++;
      break;
    case JFR_OP_CASE:
    case JFR_OP_WSET:
    case JFR_OP_VCASE:
    case JFR_OP_WHILE:
    case JFR_OP_FRETURN:
    case JFR_OP_ENDFUNC:
      oc->type = JFG_TT_EOE;
      break;
    case JFR_OP_EXTERN:
      oc->type = JFG_TT_EOE;
      m = 256 * (int) (*pc);
      pc++;
      m += *pc;
      pc++;
      pc++;   /* argc */
      pc += m;
      break;
    case JFR_OP_FZVPOP:
      oc->type = JFG_TT_EOE;
      pc++;
      if (jfr_head->vbytes == 2)
        pc++;
      break;
    case JFR_OP_VPOP:
    case JFR_OP_APOP:
    case JFR_OP_SPOP:
    case JFR_OP_VINCREASE:
    case JFR_OP_VDECREASE:
    case JFR_OP_CLEAR:
      oc->type = JFG_TT_EOE;
      pc++;
      break;
    case JFR_OP_BETWEEN:
      oc->type = JFG_TT_BETWEEN;
      oc->sarg_1 = *pc;
      pc++;
      oc->sarg_2 = *pc;
      pc++;
      oc->op = *pc;
      pc++;
      break;
    case JFR_OP_VFUNC:
      oc->type = JFG_TT_VFUNC;
      oc->op = *pc;
      pc++;
      oc->sarg_1 = *pc;
      pc++;
      break;
    default:   /* fzvar */
      oc->type = JFG_TT_FZVAR;
      m = op;
      if (jfr_head->vbytes == 2)
      { m = m * 256 + (int) *pc;
        pc++;
      }
      oc->sarg_1 = m;
      break;
    }
    oc->n_pc = pc;
    return 0;
}

static void jfg_p_expr(char *text, struct jfr_head_desc *jfr_head,
                       struct jfg_tree_desc *tree, unsigned short max_tree,
                       unsigned char *program_id)
{
  unsigned char *pc;
  unsigned short m;
  int wlearn, res, test_pr, vno, ano, scase, argc, t, fuargno, break_stat;
  char t_tmp[256];
  float farg;
  unsigned short cond_tree, index_tree, expr_tree;
  struct jfg_statement_desc stat;

  pc = program_id;
  test_pr = 0;
  scase = 0;
  break_stat = 0;
  jfg_statement(&stat, jfr_head, program_id);
  switch (stat.type)
  { case JFG_ST_CASE:
      jfg_print(text, jfg_t_case);
      if ((stat.flags & 1) != 0)
        scase = 1;
      pc++;
      break;
    case JFG_ST_WSET:
      jfg_print(text, jfg_t_wset);
      break_stat = 1;
      pc++;
      break;
    case JFG_ST_WHILE:
      jfg_print(text, jfg_t_while);
      pc++;
      break;
    case JFG_ST_IF:
      if ((stat.flags & 1) == 0)
        test_pr = 1;  /* if-statemet or assign-statement */
      else
      { jfg_print(text, jfg_t_ifw);
        pc++;
        memcpy(&farg, pc, sizeof(float));
        pc += sizeof(float);
        if ((stat.flags & 2) != 0)
          wlearn = 1;
        else
          wlearn = 0;
        jfg_p_float(text, farg, wlearn);
      }
      break;
  }

  cond_tree = 0;
  expr_tree = 0;
  res = jfg_if_tree(tree, max_tree, &cond_tree, &index_tree, &expr_tree,
                    jfr_head, program_id);

  if (res == 0)
  { if (test_pr == 1)
    { if (tree[cond_tree].type != JFG_TT_TRUE)
      { test_pr = 0;
        jfg_print(text, jfg_t_if);
      }
    }
    if (test_pr == 0)
    { if (scase == 1)  /* case til en 'switch var'-statement */
      { vno = jfr_head->fzvars[tree[cond_tree].sarg_1].var_no;
        ano = jfr_head->vars[vno].f_adjectiv_no
           + tree[cond_tree].sarg_1 - jfr_head->vars[vno].f_fzvar_no;
        jfg_print(text, jfr_head->adjectives[ano].name);
      }
      else
      if (break_stat == 1)
      { if (tree[cond_tree].type != JFG_TT_FALSE)
           jfg_ic_pr(text, jfr_head, tree, cond_tree, cond_tree, 1);
      }
      else
        jfg_ic_pr(text, jfr_head, tree, cond_tree, cond_tree, 1);
    }
    if (stat.type == JFG_ST_IF)
    { if (test_pr == 0)
        jfg_print(text, jfg_t_then);
      if (stat.sec_type == JFG_SST_VAR)
      { jfg_print(text, jfr_head->vars[stat.sarg_1].name);
        jfg_print(text, jfg_t_eq);
        jfg_ic_pr(text, jfr_head, tree, expr_tree, expr_tree, 1);
      }
      else
      if (stat.sec_type == JFG_SST_FUARG)
      { fuargno = jfr_head->functions[jfg_func_no].f_arg_no + stat.sarg_1;
        jfg_print(text, jfr_head->func_args[fuargno].name);
        jfg_print(text, jfg_t_eq);
        jfg_ic_pr(text, jfr_head, tree, expr_tree, expr_tree, 1);
      }
      else
      if (stat.sec_type == JFG_SST_PROCEDURE)
      {  jfg_ic_pr(text, jfr_head, tree, expr_tree, expr_tree, 1);
      }
      else
      if (stat.sec_type == JFG_SST_INC)
      { if (stat.flags & 4)
          jfg_print(text, jfg_t_decrease);
        else
          jfg_print(text, jfg_t_increase);
        jfg_print(text, jfr_head->vars[stat.sarg_1].name);
        if (tree[expr_tree].type != JFG_TT_CONST
            || tree[expr_tree].farg != 1.0
            || tree[expr_tree].op != 0)
        { jfg_print(text, jfg_t_with);
          jfg_ic_pr(text, jfr_head, tree, expr_tree, expr_tree, 1);
        }
      }
      else
      if (stat.sec_type == JFG_SST_ARR)
      { jfg_tp_print(text, jfr_head->arrays[stat.sarg_1].name, JFG_PT_SQUARE);
        jfg_ic_pr(text, jfr_head, tree, index_tree, index_tree, 1);
        jfg_print(text, jfg_t_espar);
        jfg_print(text, jfg_t_eq);
        jfg_ic_pr(text, jfr_head, tree, expr_tree, expr_tree, 1);
      }
      else
      if (stat.sec_type == JFG_SST_CLEAR)
      { jfg_print(text, jfg_t_clear);
        jfg_print(text, jfr_head->vars[stat.sarg_1].name);
      }
      else
      if (stat.sec_type == JFG_SST_PROCEDURE)
      { jfg_ic_pr(text, jfr_head, tree, expr_tree, expr_tree, 1);
      }
      else
      if (stat.sec_type == JFG_SST_RETURN)
      { jfg_print(text, jfg_t_return);
        jfg_ic_pr(text, jfr_head, tree, expr_tree, expr_tree, 1);
      }
      else
      if (stat.sec_type == JFG_SST_EXTERN)
      { jfg_print(text, jfg_t_extern);
        pc = program_id;
        if (*pc == JFR_OP_WEXPR || *pc == JFR_OP_AWEXPR)
          pc += sizeof(float);
        pc++;
        pc = jfg_skip_expr(jfr_head, pc);
        pc += 3;
        argc = *pc;
        pc++;
        for (m = 0; m < argc; m++)
        { t = 0;
          while (*pc != '\0')
          { t_tmp[t] = *pc;
            t++;
            pc++;
          }
          t_tmp[t] = '\0';
          jfg_print(text, t_tmp);
          pc++;
        }
      }
      else
     jfg_p_fzvar(text, jfr_head, stat.sarg_1);
    }
  }
}


int jfg_t_statement(char *text, int maxtext, int aspaces,
                    struct jfg_tree_desc *tmp_tree, unsigned short max_tree,
                    void *head, int func_no, unsigned char *program_id)
{
  unsigned char *pc;
  struct jfr_head_desc *jfr_head;
  unsigned char op;

  jfg_maxtext = maxtext;
  jfg_err = 0;
  jfg_ff_text = jfg_line_pos = 0;
  jfg_aspaces = aspaces;
  text[0] = '\0';
  jfr_head = (struct jfr_head_desc *) head;
  jfg_func_no = func_no;
  pc = program_id;
  op = *pc;
  pc++;
  switch (op)
  { case JFR_OP_EXPR:
    case JFR_OP_WEXPR:
    case JFR_OP_AWEXPR:
      jfg_p_expr(text, jfr_head, tmp_tree, max_tree, pc - 1);
      break;
    case JFR_OP_SWITCH:
      jfg_print(text, jfg_t_switch);
      break;
    case JFR_OP_VSWITCH:
      jfg_print(text, jfg_t_switch);
      jfg_print(text, jfr_head->vars[*pc].name);
      break;
    case JFR_OP_DEFAULT:
      jfg_print(text, jfg_t_default);
      break;
    case JFR_OP_ENDSWITCH:
    case JFR_OP_ENDWHILE:
    case JFR_OP_ENDFUNC:
      jfg_print(text, jfg_t_end);
      break;
  }
  jfg_print(text, jfg_t_scolon);
  return jfg_err;
}

int jfg_a_statement(const char *argv[], int maxargc,
      void *head, unsigned char *pc)
{
  int argc, m;
  unsigned char *ipc;
  struct jfr_head_desc *jfr_head;

  jfr_head = (struct jfr_head_desc *) head;
  ipc = pc;
  if (*ipc == JFR_OP_WEXPR || *ipc == JFR_OP_AWEXPR)
    ipc += sizeof(float);
  ipc++;
  ipc = jfg_skip_expr(jfr_head, ipc);
  ipc += 3;
  argc = *ipc;
  if (argc > maxargc)
    return -1;
  ipc++;
  for (m = 0; m < argc; m++)
  { argv[m] = (char *) ipc;
    while (*ipc != '\0')
      ipc++;
    ipc++;
  }
  return argc;
}

void jfg_free(void)
{
  jfg_maxstack = 0;
  if (jfg_stack != NULL)
    free(jfg_stack);
  jfg_stack = NULL;
}


