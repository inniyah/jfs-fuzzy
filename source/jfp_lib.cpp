  /**************************************************************************/
  /*                                                                        */
  /* jfp_lib.c   Version  2.00    Copyright (c) 1998-1999 Jan E. Mortensen  */
  /*                                                                        */
  /* JFS (Jans Fuzzy System) library to change a jfr-program.               */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    2000 Frederiksberg                                                  */
  /*    DK Denmark                                                          */
  /*                                                                        */
  /**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "jfr_gen.h"
#include "jfg_lib.h"
#include "jfp_lib.h"
#include "jfs_cons.h"

static int jfp_err = 0;

static int jfp_cstatmax = 0;
static unsigned char *jfp_cstat = NULL; /* holds a single statement */
static unsigned short jfp_ff_cstat;

static int jfp_err_set(int err_no);
static void jfp_base(struct jfr_head_desc *jfr_head, int var_no, int ano);
static void jfp_bases(struct jfr_head_desc *jfr_head, int adjectiv_no);

static void jfp_cadd(unsigned char arg);
static void jfp_fadd(unsigned char op, float f);
static void jfp_progadd(unsigned short arg,
			struct jfr_head_desc *jfr_head);

static int jfp_cstat_ins(struct jfr_head_desc *jfr_head,
			 unsigned char **program_id);

static int jfp_leaf(struct jfr_head_desc *jfr_head,
		    struct jfg_tree_desc *tree,
		    unsigned short leaf_no);


static int jfp_err_set(int err_no)
{
  if (jfp_err == 0)
    jfp_err = err_no;
  return 1;
}

static void jfp_base(struct jfr_head_desc *jfr_head, int var_no, int ano)
{
  /* Update base for adjectiv <ano> if base is calculated.             */

  struct jfr_adjectiv_desc *adjectiv;
  struct jfr_var_desc *var;
  float pre, post;

  var = &(jfr_head->vars[var_no]);
  adjectiv = &(jfr_head->adjectives[ano]);
  if (adjectiv->limit_c == 0
      && (adjectiv->flags & JFS_AF_BASE) == 0)
  { if (ano == var->f_adjectiv_no)
    { if ((jfr_head->domains[var->domain_no].flags & JFS_DF_MINENTER) == 0)
	    pre = adjectiv->center;
      else
	   pre = jfr_head->domains[var->domain_no].dmin;
    }
    else
      pre = jfr_head->adjectives[ano - 1].trapez_end;
    if (ano == var->f_adjectiv_no + var->fzvar_c - 1)
    { if ((jfr_head->domains[var->domain_no].flags & JFS_DF_MAXENTER) == 0)
	    post = adjectiv->center;
      else
        post = jfr_head->domains[var->domain_no].dmax;
    }
    else
      post = jfr_head->adjectives[ano + 1].trapez_start;
    adjectiv->base = post - pre;
  }
}


void jfp_adj_bases(struct jfr_head_desc *jfr_head, int adjectiv_no)
{
  /* After change of center of <adjectiv_no> bases for adjectives      */
  /* next to <adjectiv> is recalcualted (if not the bases are entered).*/
  /* If a variable uses the adjectiv as default-value then it is       */
  /* changed.                                                          */

  struct jfr_var_desc *var;
  int m, changed;

 changed = 0;
  for (m = 0; m < jfr_head->var_c; m++)
  { var = &(jfr_head->vars[m]);
    if (adjectiv_no >= var->f_adjectiv_no
	       && adjectiv_no < var->f_adjectiv_no + var->fzvar_c)
    { if (changed == 0)
      { if (adjectiv_no > var->f_adjectiv_no)
	         jfp_base(jfr_head, m, adjectiv_no - 1);
  	     if (adjectiv_no < var->f_adjectiv_no + var->fzvar_c - 1)
	         jfp_base(jfr_head, m, adjectiv_no + 1);
	       changed = 1;
      }
      if (var->default_type == JFS_VDT_ADJECTIV
	         && var->f_adjectiv_no + var->default_type == adjectiv_no)
	       var->default_val = jfr_head->adjectives[adjectiv_no].center;
    }
  }
}

/*********************************************************************/
/* Eksterne funktioner                                               */
/*********************************************************************/

int jfp_init(int cstatsize)
{
  if (cstatsize <= 0)
    jfp_cstatmax = 512;
  else
    jfp_cstatmax = cstatsize;
  if ((jfp_cstat = (unsigned char *) malloc(jfp_cstatmax)) == NULL)
  { jfp_cstatmax = 0;
    return 6;
  }
  return 0;
}

void jfp_adjectiv(void *head, unsigned short adjectiv_no,
               		 struct jfg_adjectiv_desc *adesc)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_adjectiv_desc *adjectiv;
  float hcenter;

  jfr_head = (struct jfr_head_desc *) head;
  adjectiv = &(jfr_head->adjectives[adjectiv_no]);
  hcenter = adjectiv->center;
  /* adjectiv->flags = adesc->flags;  FJERNET V 2.0 */
  adjectiv->h1_no = adesc->h1_no;
  adjectiv->h2_no = adesc->h2_no;
  if (adjectiv->flags & JFS_AF_CENTER)
  { adjectiv->center = adesc->center;
    if (!(adjectiv->flags & JFS_AF_TRAPEZ))
    { adjectiv->trapez_start = adjectiv->center;
      adjectiv->trapez_end = adjectiv->center;
    }
  }
  if (adjectiv->flags & JFS_AF_TRAPEZ)
  { adjectiv->trapez_start = adesc->trapez_start;
    adjectiv->trapez_end = adesc->trapez_end;
    if (!(adjectiv->flags & JFS_AF_CENTER))
      adjectiv->center = (adjectiv->trapez_start + adjectiv->trapez_end) / 2.0;
  }
  if (adjectiv->flags & JFS_AF_BASE)
    adjectiv->base   = adesc->base;
  if (!(adjectiv->flags & JFS_AF_BASE) && adjectiv->center != hcenter)
    jfp_adj_bases(jfr_head, adjectiv_no);
}

void jfp_alimits(void *head, unsigned short adjectiv_no,
               		struct jfg_limit_desc *ldescs)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_adjectiv_desc *adjectiv;
  struct jfr_limit_desc *limit;
  int m;
  float x, y, hx, hy, cg_tael, cg_naevn;

  jfr_head = (struct jfr_head_desc *) head;
  adjectiv = &(jfr_head->adjectives[adjectiv_no]);
  x = y = 0.0;
  cg_tael = cg_naevn = 0.0;
  for (m = 0; m < adjectiv->limit_c; m++)
  { hx = x;
    hy = y;
    x = ldescs[m].limit;
    y = ldescs[m].value;
    limit = &(jfr_head->limits[adjectiv->f_limit_no + m]);
    limit->limit = x;
    limit->exclusiv = ldescs[m].exclusiv;
    limit->flags = ldescs[m].flags;
    if (m == 0 || x == hx)
    { limit->a = 0.0;
      limit->b = y;
    }
    else
    { limit->a = (y - hy) / (x - hx);
      limit->b = y - limit->a * x;

      if ((adjectiv->flags & JFS_AF_CENTER) == 0)
      { /* optael center of gravity  =                          */
	    /* sum(intgral(x * (ax + b), x1, x2)) / sum(intgral(ax + b, x1, x2)) */
	    cg_tael += 1.0 / 3.0 * limit->a * (x * x * x - hx * hx * hx)
		           + 0.5 * limit->b * (x * x - hx * hx);
	    cg_naevn += 0.5 * limit->a * (x * x - hx * hx)
		            + limit->b * (x - hx);
      }
    }
  }

  if ((adjectiv->flags & JFS_AF_BASE) == 0)
    adjectiv->base = ldescs[adjectiv->limit_c - 1].limit
                     - ldescs[0].limit;

  if ((adjectiv->flags & JFS_AF_CENTER) == 0)
  { if (cg_naevn != 0.0)
      adjectiv->center = cg_tael / cg_naevn;
    else
      adjectiv->center = 0.0;
    adjectiv->trapez_start = adjectiv->trapez_end = adjectiv->center;
    jfp_adj_bases(jfr_head, adjectiv_no);
  }
}

void jfp_var(void *head, unsigned short var_no,
             struct jfg_var_desc *vdesc)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_var_desc *var;

  jfr_head = (struct jfr_head_desc *) head;
  var = &(jfr_head->vars[var_no]);
  var->acut 	    	= vdesc->acut;
  var->no_arg 	   = vdesc->no_arg;
  var->defuz_arg 	= vdesc->defuz_arg;
  if (var->default_type == JFS_VDT_VALUE)
    var->default_val = vdesc->default_val;
  var->defuz_1 		 = vdesc->defuz_1;
  var->defuz_2		  = vdesc->defuz_2;
  var->f_comp     = vdesc->f_comp;
  var->d_comp     = vdesc->d_comp;
  var->flags		    = vdesc->flags;
  var->argument   = vdesc->argument;
}

void jfp_hedge(void *head, unsigned short hedge_no,
               struct jfg_hedge_desc *hdesc)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_hedge_desc *hedge;

  jfr_head = (struct jfr_head_desc *) head;
  hedge = &(jfr_head->hedges[hedge_no]);
  hedge->hedge_arg	= hdesc->hedge_arg;
  if (hedge->type != JFS_HT_LIMITS)
    hedge->type		= hdesc->type;
  hedge->flags		= hdesc->flags;
}

void jfp_hlimits(void *head, unsigned short hedge_no,
               		struct jfg_limit_desc *ldescs)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_hedge_desc *hedge;
  struct jfr_limit_desc *limit;
  int m;
  float x, y, hx, hy;

  jfr_head = (struct jfr_head_desc *) head;
  hedge = &(jfr_head->hedges[hedge_no]);
  x = y = 0.0;
  for (m = 0; m < hedge->limit_c; m++)
  { hx = x;
    hy = y;
    x = ldescs[m].limit;
    y = ldescs[m].value;
    limit = &(jfr_head->limits[hedge->f_limit_no + m]);
    limit->limit = x;
    limit->exclusiv = ldescs[m].exclusiv;
    limit->flags = ldescs[m].flags;
    if (m == 0 || x == hx)
    { limit->a = 0.0;
      limit->b = y;
    }
    else
    { limit->a = (y - hy) / (x - hx);
      limit->b = y - limit->a * x;
    }
  }
}

void jfp_relation(void *head, unsigned short relation_no,
               		 struct jfg_relation_desc *rdesc)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_relation_desc *relation;

  jfr_head = (struct jfr_head_desc *) head;
  relation = &(jfr_head->relations[relation_no]);
  relation->hedge_no  = rdesc->hedge_no;
  relation->flags	    = rdesc->flags;
}

void jfp_rlimits(void *head, unsigned short relation_no,
               		struct jfg_limit_desc *ldescs)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_relation_desc *relation;
  struct jfr_limit_desc *limit;
  int m;
  float x, y, hx, hy;

  jfr_head = (struct jfr_head_desc *) head;
  relation = &(jfr_head->relations[relation_no]);
  x = y = 0.0;
  for (m = 0; m < relation->limit_c; m++)
  { hx = x;
    hy = y;
    x = ldescs[m].limit;
    y = ldescs[m].value;
    limit = &(jfr_head->limits[relation->f_limit_no + m]);
    limit->limit = x;
    limit->exclusiv = ldescs[m].exclusiv;
    limit->flags = ldescs[m].flags;
    if (m == 0 || x == hx)
    { limit->a = 0.0;
      limit->b = y;
    }
    else
    { limit->a = (y - hy) / (x - hx);
      limit->b = y - limit->a * x;
    }
  }
}

void jfp_operator(void *head, unsigned short operator_no,
              		  struct jfg_operator_desc *odesc)
{
  struct jfr_head_desc *jfr_head;
  struct jfr_operator_desc *op;

  jfr_head = (struct jfr_head_desc *) head;
  op = &(jfr_head->operators[operator_no]);
  op->op_arg = odesc->op_arg;
  op->op_1   = odesc->op_1;
  op->op_2   = odesc->op_2;
  op->hedge_mode = odesc->hedge_mode;
  op->hedge_no = odesc->hedge_no;
  op->flags    = odesc->flags;
}

void jfp_statement(unsigned char *program_id,
               		  struct jfg_statement_desc *stat)
{
  unsigned char op;
  unsigned char *pc;

  pc = program_id;
  op = *pc;
  pc++;
  if (stat->type == JFG_ST_IF)
  { if (op == JFR_OP_WEXPR  || op == JFR_OP_AWEXPR)
    { memcpy(pc, &(stat->farg), sizeof(float));
    }
  }
}


void jfp_d_statement(void *head, unsigned char *program_id)
{
  struct jfr_head_desc *jfr_head;
  struct jfg_statement_desc stdesc;
  unsigned short ant;
  unsigned char *pc;
  unsigned char *dpc;
  int m, slut;

  jfr_head = (struct jfr_head_desc *) head;
  jfg_statement(&stdesc, head, program_id);
  ant = stdesc.n_pc - program_id;
  pc = stdesc.n_pc;
  dpc = program_id;
  for (m = pc - jfr_head->function_code;
       m < jfr_head->funccode_c + jfr_head->program_c; m++)
  { *dpc = *pc;
    pc++; dpc++;
  }
  jfr_head->a_size -= ant;
  if (jfr_head->program_code > program_id)  /* slettet fra funktion */
  { jfr_head->program_code -= ant;
    jfr_head->funccode_c -= ant;
    slut = 0;
    for (m = jfr_head->function_c - 1; slut == 0 && m >= 0; m--)
    { if (jfr_head->function_code + jfr_head->functions[m].pc > program_id)
        jfr_head->functions[m].pc -= ant;
      else
        slut = 1;
    }
  }
  else
    jfr_head->program_c -= ant;
}

static void jfp_cadd(unsigned char arg)
{
  if (jfp_ff_cstat >= jfp_cstatmax)
    jfp_err_set(402);
  else
  { jfp_cstat[jfp_ff_cstat] = arg;
    jfp_ff_cstat++;
  }
}

static void jfp_fadd(unsigned char op, float f)
{
  jfp_cadd(op);
  if (jfp_ff_cstat + ((int) sizeof(float)) >= jfp_cstatmax)
    jfp_err_set(402);
  else
  { memcpy(&(jfp_cstat[jfp_ff_cstat]), &f, sizeof(float));
    jfp_ff_cstat += sizeof(float);
  }
}

static void jfp_progadd(unsigned short arg,
			struct jfr_head_desc *jfr_head)
{
  if (jfr_head->vbytes == 2)
  { jfp_cadd(arg / 256);
    jfp_cadd(arg % 256);
  }
  else
    jfp_cadd(arg);
}


static int jfp_cstat_ins(struct jfr_head_desc *jfr_head,
		  unsigned char **program_id)
{
  int m, slut;
  signed long pid;
  unsigned char *prog_id;

  if (jfp_err != 0)
    return 1;
  prog_id = *program_id;
  if (jfr_head->a_size + jfp_ff_cstat >= jfr_head->rsize)
    return jfp_err_set(403);
  for (pid = jfr_head->funccode_c + jfr_head->program_c - 1;
       pid >= prog_id - jfr_head->function_code;
       pid--)
    jfr_head->function_code[pid + jfp_ff_cstat]
	 = jfr_head->function_code[pid];
  for (m = 0; m < jfp_ff_cstat; m++)
  { *prog_id = jfp_cstat[m];
    prog_id++;
  }
  jfr_head->a_size += jfp_ff_cstat;

  if (jfr_head->program_code > *program_id)  /* indsaetter i funktion */
  { jfr_head->program_code += jfp_ff_cstat;
    jfr_head->funccode_c += jfp_ff_cstat;
    slut = 0;
    for (m = jfr_head->function_c - 1; slut == 0 && m >= 0; m--)
    { if (jfr_head->function_code + jfr_head->functions[m].pc > *program_id)
	    jfr_head->functions[m].pc += jfp_ff_cstat;
      else
	    slut = 1;
    }
  }
  else
    jfr_head->program_c += jfp_ff_cstat;
  *program_id = prog_id;
  return 0;
}

static int jfp_leaf(struct jfr_head_desc *jfr_head,
		    struct jfg_tree_desc *tree, unsigned short leaf_no)
{
  struct jfg_tree_desc *leaf;

  leaf = &(tree[leaf_no]);
  switch (leaf->type)
  { case JFG_TT_OP:   /* and, or */
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_leaf(jfr_head, tree, leaf->sarg_2);
      jfp_cadd(JFR_OP_OP);
      jfp_cadd(leaf->op);
      break;
    case JFG_TT_HEDGE:   /* hedge  */
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_cadd(JFR_OP_HEDGE);
      jfp_cadd(leaf->op);
      break;
    case JFG_TT_UREL:  /* cmp */
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_leaf(jfr_head, tree, leaf->sarg_2);
      jfp_cadd(JFR_OP_UREL);
      jfp_cadd(leaf->op);
      break;
    case JFG_TT_SFUNC:
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_cadd(JFR_OP_SFUNC);
      jfp_cadd(leaf->op);
      break;
    case JFG_TT_DFUNC:
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_leaf(jfr_head, tree, leaf->sarg_2);
      jfp_cadd(JFR_OP_DFUNC);
      jfp_cadd(leaf->op);
      break;
    case JFG_TT_CONST: /* const */
      if (leaf->op == 0)
       	jfp_fadd(JFR_OP_CONST, leaf->farg);
      else
       	jfp_fadd(JFR_OP_ICONST, leaf->farg);
      break;
    case JFG_TT_VAR:
      jfp_cadd(JFR_OP_VAR);
      jfp_cadd(leaf->sarg_1);
      break;
    case JFG_TT_UF_VAR:
      jfp_cadd(JFR_OP_SPUSH);
      jfp_cadd(leaf->sarg_1);
      break;
    case JFG_TT_FZVAR: /* fzvar_no */
      jfp_progadd(leaf->sarg_1, jfr_head);
      break;
    case JFG_TT_TRUE:
      jfp_cadd(JFR_OP_TRUE);
      break;
    case JFG_TT_FALSE:
      jfp_cadd(JFR_OP_FALSE);
      break;
    case JFG_TT_BETWEEN:
      jfp_cadd(JFR_OP_BETWEEN);
      jfp_cadd(leaf->sarg_1);
      jfp_cadd(leaf->sarg_2);
      jfp_cadd(leaf->op);
      break;
    case JFG_TT_VFUNC:
      jfp_cadd(JFR_OP_VFUNC);
      jfp_cadd(leaf->op);
      jfp_cadd(leaf->sarg_1);
      break;
    case JFG_TT_UFUNC:
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_cadd(JFR_OP_USERFUNC);
      jfp_cadd(leaf->op);
      break;
    case JFG_TT_ARGLIST:
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_leaf(jfr_head, tree, leaf->sarg_2);
      break;
    case JFG_TT_IIF:
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_leaf(jfr_head, tree, leaf->sarg_2);
      jfp_cadd(JFR_OP_IIF);
      break;
    case JFG_TT_ARVAL:
      jfp_leaf(jfr_head, tree, leaf->sarg_1);
      jfp_cadd(JFR_OP_ARRAY);
      jfp_cadd(leaf->op);
      break;
  }
  return 0;
}


int jfp_i_tree(void *head, unsigned char **program_id,
	       struct jfg_statement_desc *stat,
	       struct jfg_tree_desc *tree,
	       unsigned short cond_no,
	       unsigned short index_no,
	       unsigned short expr_no,
	       char *argv[], int argc)
{
  struct jfr_head_desc *jfr_head;
  unsigned char op;
  int m, s, a;
  unsigned char c;
  char *as;

  jfr_head = (struct jfr_head_desc *) head;
  jfp_ff_cstat = 0;
  jfp_err = 0;
  switch (stat->type)
  { case JFG_ST_IF:
      if ((stat->flags & 1) != 0) /* rule-weight */
      { if ((stat->flags & 2) != 0)   /* learn weight */
          op = JFR_OP_AWEXPR;
        else
          op = JFR_OP_WEXPR;
        jfp_fadd(op, stat->farg);
      }
      else
        jfp_cadd(JFR_OP_EXPR);
      jfp_leaf(jfr_head, tree, cond_no);
      if (stat->sec_type == JFG_SST_FZVAR)
      { jfp_cadd(JFR_OP_THENEXPR);
        jfp_cadd(JFR_OP_FZVPOP);
        jfp_progadd(stat->sarg_1, jfr_head);
      }
      else
      if (stat->sec_type == JFG_SST_VAR)
      { jfp_cadd(JFR_OP_THENEXPR);
        jfp_leaf(jfr_head, tree, expr_no);
        jfp_cadd(JFR_OP_VPOP);
        jfp_cadd(stat->sarg_1);
      }
      else
      if (stat->sec_type == JFG_SST_FUARG)
      { jfp_cadd(JFR_OP_THENEXPR);
        jfp_leaf(jfr_head, tree, expr_no);
        jfp_cadd(JFR_OP_SPOP);
        jfp_cadd(stat->sarg_1);
      }
      else
      if (stat->sec_type == JFG_SST_RETURN)
      { jfp_cadd(JFR_OP_THENEXPR);
        jfp_leaf(jfr_head, tree, expr_no);
        jfp_cadd(JFR_OP_FRETURN);
      }
      else
      if (stat->sec_type == JFG_SST_ARR)
      { jfp_leaf(jfr_head, tree, index_no);
        jfp_cadd(JFR_OP_THENEXPR);
        jfp_leaf(jfr_head, tree, expr_no);
        jfp_cadd(JFR_OP_APOP);
        jfp_cadd(stat->sarg_1);
      }
      else
      if (stat->sec_type == JFG_SST_PROCEDURE)
      { jfp_cadd(JFR_OP_THENEXPR);
        jfp_leaf(jfr_head, tree, expr_no);
      }
      else
      if (stat->sec_type == JFG_SST_INC)
      { jfp_cadd(JFR_OP_THENEXPR);
        jfp_leaf(jfr_head, tree, expr_no);
        if (stat->flags & 4)
          jfp_cadd(JFR_OP_VDECREASE);
        else
          jfp_cadd(JFR_OP_VINCREASE);
        jfp_cadd(stat->sarg_1);
      }
      else
      if (stat->sec_type == JFG_SST_CLEAR)
      { jfp_cadd(JFR_OP_THENEXPR);
        jfp_cadd(JFR_OP_CLEAR);
        jfp_cadd(stat->sarg_1);
      }
      else
      if (stat->sec_type == JFG_SST_EXTERN)
      { jfp_cadd(JFR_OP_THENEXPR);
        jfp_cadd(JFR_OP_EXTERN);
        s = 0;
        for (m = 0; m < argc; m++)
          s += strlen(argv[m]) + 1;
        c = (unsigned char) (s / 256);
        jfp_cadd(c);
        c = (unsigned char) (s - ((int) c) * 256);
        jfp_cadd(c);
        c = (unsigned char) argc;
        jfp_cadd(c);
        for (m = 0; m < argc; m++)
        { s = strlen(argv[m]);
          as = argv[m];
          for (a = 0; a < s; a++)
           jfp_cadd(as[a]);
         jfp_cadd('\0');
        }
      }
      break;
    case JFG_ST_CASE:
      jfp_cadd(JFR_OP_EXPR);
      jfp_leaf(jfr_head, tree, cond_no);
      if (stat->flags & 1)
        jfp_cadd(JFR_OP_VCASE);
      else
        jfp_cadd(JFR_OP_CASE);
      break;
    case JFG_ST_WSET:
      jfp_cadd(JFR_OP_EXPR);
      jfp_leaf(jfr_head, tree, cond_no);
      jfp_cadd(JFR_OP_WSET);
      break;
    case JFG_ST_STEND:
      if (stat->sec_type == 0)
        jfp_cadd(JFR_OP_ENDSWITCH);
      else
      if (stat->sec_type == 1)
        jfp_cadd(JFR_OP_ENDWHILE);
      else
        jfp_cadd(JFR_OP_ENDFUNC);
      break;
    case JFG_ST_SWITCH:
      if (stat->flags & 1)
      { jfp_cadd(JFR_OP_VSWITCH);
        jfp_cadd(stat->sarg_1);
      }
      else
        jfp_cadd(JFR_OP_SWITCH);
      break;
    case JFG_ST_DEFAULT:
      jfp_cadd(JFR_OP_DEFAULT);
      break;
    case JFG_ST_WHILE:
      jfp_cadd(JFR_OP_EXPR);
      jfp_leaf(jfr_head, tree, cond_no);
      jfp_cadd(JFR_OP_WHILE);
      break;
    default:
      jfp_err_set(401);
      break;
  }
  jfp_cstat_ins(jfr_head, program_id);
  return jfp_err;
}

void jfp_u_oc(void *head, unsigned char *program_id,
       	      struct jfg_oc_desc *oc)
{
  unsigned char *pc;
  struct jfr_head_desc *jfr_head;
  unsigned char op;

  jfr_head = (struct jfr_head_desc *) head;
  pc = program_id;
  op = *pc;
  if (op == JFR_OP_EXPR || op == JFR_OP_WEXPR || op == JFR_OP_AWEXPR
      || op == JFR_OP_THENEXPR)
  { pc++;
    if (op == JFR_OP_WEXPR || op == JFR_OP_AWEXPR)
      pc += sizeof(float);
  }
  if (oc->type != JFG_TT_FZVAR)
    pc++;
  switch (oc->type)
  { case JFG_TT_OP:
    case JFG_TT_HEDGE:
    case JFG_TT_UREL:
    case JFG_TT_SFUNC:
    case JFG_TT_DFUNC:
      *pc = oc->op;
      break;
    case JFG_TT_CONST:
      memcpy(pc, (unsigned char *) &(oc->farg), sizeof(float));
      break;
    case JFG_TT_VAR:
    case JFG_TT_UF_VAR:
      *pc = oc->sarg_1;
      break;
    case JFG_TT_FZVAR:
      if (jfr_head->vbytes == 2)
      { *pc = oc->sarg_1 / 256;
        pc++;
        *pc = oc->sarg_1 % 256;
      }
      else
        *pc = (unsigned char) oc->sarg_1;
      break;
    case JFG_TT_BETWEEN:
      pc++;
      *pc = (unsigned char) oc->sarg_1;   /* adjectiv 1 */
      pc++;
      *pc = oc->op;                       /* adjectiv 2 */
      break;

  }
}


int jfp_save(char *fname, void *head)
{
  struct jfr_ehead_desc jf_ehead;
  struct jfr_head_desc *jfr_head;
  FILE *dest;

  jfr_head = (struct jfr_head_desc *) head;
  memcpy(&jf_ehead, jfr_head, JFR_EHEAD_SIZE);

  if ((dest = fopen(fname, "wb")) == NULL)
    return 1;
  if (fwrite((char *) &jf_ehead, JFR_EHEAD_SIZE, 1, dest) != 1)
    return 3;
  if (fwrite((char *) jfr_head->domains,
	         jfr_head->a_size, 1, dest) != 1)
    return 3;
  fclose(dest);
  return 0;
}

void jfp_free(void)
{
  if (jfp_cstat != NULL)
    free(jfp_cstat);
}


