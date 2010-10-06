
  /**************************************************************************/
  /*                                                                        */
  /* jfr_lib.c   Version  2.03    Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                                                        */
  /* JFS load/run-functions.                                                */
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
#include "jfr_lib.h"
#include "jfs_cons.h"

struct jfr_bl_stack_desc
      { /* stak-record to switch/while-blocks  */
        float          weight;
        float          sum;
        unsigned char *pc;
        unsigned short rule_no;
      };
#define JFR_BL_STACK_SIZE sizeof(struct jfr_bl_stack_desc)

struct jfr_fu_stack_desc
      { /* stak-record to function-calls        */
          float          weight;
          float          cond_value;
          float          rm_weight;
          unsigned char  *pc;
          unsigned char  *program_id;  /* pc calling statement (log).  */
          unsigned char  *stack_id;
          int            function_no;  /* function-no calling function */
                                       /* -1:main, only used in log.   */
          unsigned short rule_no;
          unsigned char  fuarg_c;      /* no args actual function      */
          unsigned char  dummy;
      };
#define JFR_FU_STACK_SIZE sizeof(struct jfr_fu_stack_desc)


struct jfr_run_head_desc
  {   unsigned char  *pc;          /* program counter.         */
      float          weight;       /* calculated weight.       */
      float          rm_weight;    /* weight before statement  */
                                   /* started.                 */
      unsigned char           *stack;
      unsigned char           *ff_stack;
      unsigned char           *max_stack;
      unsigned char           *stack_id;

      /* used to log-information */
      float          cond_value;   /* rule-weight.             */
      float          expr_value;   /* value last then-expr.    */
      float          index_value;  /* array-indeks-value.      */
      unsigned char  *program_id;  /* Program-id act.statement */
      unsigned short rule_no;
      int            err_code;
      int            changed;      /* Did last stmt  change    */
      int            function_no;  /* -1: main                 */
  };

static struct jfr_run_head_desc jfrr;

static struct jfr_head_desc     *jfr_head;

static void   (*jfr_call)(void);
static void   (*jfr_mcall)(int place);
static void   (*jfr_uvget)(int vno);

static int    jfr_status = 0;  /* 0: not initialised (jfr_Init()),        */
                               /* 1: initialised.                         */


void (*jfr_operators[256-JFR_OP_FIRST])(void);

static void jfr_err_set(int eno);
static void jfr_fpop(void *dest);
static void jfr_fpush(void *source);

static float jfr_fzround(float f);
static void jfr_vround(unsigned short var_no);

static float jfr_limit_calc(float ip, int first, int count);
static float jfr_ihedge_calc(int hedge_no, float val);

static void jfr_set_prog_id(void);

static float jfr_fuzificate(int fzvar_no, float value);
static void jfr_defuz(unsigned short var_no);
static void jfr_normal(unsigned short var_no);

static void jfr_ifzvput(unsigned short fzvar_no, float conf);
static void jfr_ifzvupd(unsigned short fzvar_no, float conf);
static void jfr_ivput(unsigned short var_no, float value, float conf);
static void jfr_ivupd(unsigned short var_no, float value, float conf);

static int jfr_indeks(int ano, float fid);
static void jfr_iaset(int ano, float id, float val);
static float jfr_iaget(int ano, float fid);
static float jfr_ivget(unsigned short var_no);
static float jfr_ifzvget(unsigned short fzvar_no);
static void jfr_ivincdec(unsigned short var_no, float value, float conf);
static void jfr_op_vincdec(int sign);
static void jfr_iclear(unsigned short var_no, int fzvclear);
static unsigned short jfr_fzvno(void);

static float jfr_ivfunc_calc(unsigned short fno,
                             unsigned short vno);
static float jfr_ibetween_calc(unsigned short vno, unsigned short c1,
                               unsigned short c2);
static float jfr_idfunc_calc(unsigned short fno, float a1, float a2);
static float jfr_irel_calc(unsigned short rno, float v1, float v2);
static float jfr_isfunc_calc(unsigned short fno, float a);
static float jfr_iop_calc(unsigned short opno, float arg_1, float arg_2);

static void jfr_op_wset(  void);
static void jfr_op_spush(void);
static void jfr_op_iif(void);
static void jfr_op_array(void);
static void jfr_op_freturn(void);
static void jfr_op_endfunc(void);
static void jfr_op_userfunc(void);
static void jfr_op_apop(void);
static void jfr_op_spop(void);
static void jfr_op_comment(void);
static void jfr_op_vswitch(void);
static void jfr_op_vfunc(  void);
static void jfr_op_between(void);
static void jfr_op_clear(  void);
static void jfr_op_switch( void);
static void jfr_op_default(void);
static void jfr_op_case(   void);
static void jfr_op_while(  void);
static void jfr_op_vpop(   void);
static void jfr_op_vincrease(void);
static void jfr_op_vdecrease(void);
static void jfr_op_fzvpop( void);
static void jfr_op_endswitch(void);
static void jfr_op_endwhile(void);
static void jfr_op_true(   void);
static void jfr_op_false(  void);
static void jfr_op_expr(   void);
static void jfr_op_wexpr(  void);
static void jfr_op_var(    void);
static void jfr_op_const(  void);
static void jfr_op_dfunc(  void);
static void jfr_op_urel(   void);
static void jfr_op_sfunc(  void);
static void jfr_op_hedge(  void);
static void jfr_op_op(     void);
static void jfr_op_then(   void);
static void jfr_op_call(   void);
static void jfr_op_eop(void);
static void jfr_op_err(void);

static int jfr_p_init(void *head);
static void jfr_program_handle(void);
static int jfr_iload(void **head, char *fname, unsigned short esize);
static int jfr_check(void *head);

/**************************************************************************/
/* Basic functions                                                        */
/**************************************************************************/

static void jfr_err_set(int eno)
{
  /* sets err_code to errno (if err_code == 0) */

  if (eno == 206) /* stack overflow */
  { jfr_head->status = 1;  /* stop */
    jfrr.err_code = 206;
  }
  else
  { if (jfrr.err_code == 0)
      jfrr.err_code = eno;
  }
}

static void jfr_fpop(void *dest)
{
  jfrr.ff_stack -= sizeof(float);
  memcpy(dest, jfrr.ff_stack, sizeof(float));
}

static void jfr_fpush(void *source)
{
  if (jfrr.ff_stack + sizeof(float) >= jfrr.max_stack)
    jfr_err_set(206);
  else
  { memcpy(jfrr.ff_stack, source, sizeof(float));
    jfrr.ff_stack += sizeof(float);
  }
}

static float jfr_fzround(float f)
{
  /* f is rounded to a number in [0.0, 1.0] */
  float res;

  if (f < 0.0)
    res = 0.0;
  else
  if (f > 1.0)
    res = 1.0;
  else
    res = f;
  return res;
}

static void jfr_vround(unsigned short var_no)
{
  /* rounding of a variable's value to legal domain-values */

  float dist, bdist, res;
  double t, r;
  int m, ano;
  struct jfr_domain_desc *domain;
  struct jfr_var_desc *var;

  var = &(jfr_head->vars[var_no]);
  domain = &(jfr_head->domains[var->domain_no]);

  switch (domain->type)
  { case JFS_DT_FLOAT:
      res = var->value;
      break;
    case JFS_DT_INTEGER:
      t = modf((double) var->value, &r);
      if (t >= 0.5)
        r = r + 1.0;
      else
      if (t <= -0.5)
        r = r - 1.0;
      res = (float) r;
      break;
    case JFS_DT_CATEGORICAL:
      ano = var->f_adjectiv_no;
      res = var->default_val;  /* if no fuzzy vars bound to var */
      bdist = 0.0;
      for (m = 0; m < var->fzvar_c; m++, ano++)
      { dist = fabs(var->value - jfr_head->adjectives[ano].center);
        if (m == 0 || dist < bdist)
        { res = jfr_head->adjectives[ano].center;
          bdist = dist;
        }
      }
      break;
  }
  if ((domain->flags & JFS_DF_MINENTER))
  { if (res < domain->dmin)
    { res = domain->dmin;
      jfr_err_set(202);
    }
  }
  if ((domain->flags & JFS_DF_MAXENTER))
  { if (res > domain->dmax)
    { res = domain->dmax;
      jfr_err_set(202);
    }
  }
  var->value = res;
}

static float jfr_limit_calc(float ip, int first, int count)
{
  /* calculates pl_function(<ip>), where the pl-function starts in    */
  /* limits[<first>] and has <count> points.                          */
  float res;
  int i, m;

  m = first;
  for (i = 0; i < count; i++, m++)
  { if (ip < jfr_head->limits[m].limit
        || (jfr_head->limits[m].exclusiv == 0
            && ip == jfr_head->limits[m].limit))
       break;
  }
  if (i == count)
  { m--;
    res =  jfr_head->limits[m].a * jfr_head->limits[m].limit
         + jfr_head->limits[m].b;
  }
  else
    res =   jfr_head->limits[m].a * ip + jfr_head->limits[m].b;
  res = jfr_fzround(res);
  return res;
}

static float jfr_ihedge_calc(int hedge_no, float val)
{
  /* Internal function. Calculates <hedge_no>(<val>).  */
  struct jfr_hedge_desc *hedge;
  float f, res;

  hedge = &(jfr_head->hedges[hedge_no]);

  if (hedge->type != JFS_HT_LIMITS)
    f = jfr_fzround(val);
  else
    f = val;

  switch (hedge->type)
  { case JFS_HT_NEGATE:
      res = 1.0 - f;
      break;
    case JFS_HT_POWER:
      res = pow(f, hedge->hedge_arg);
      break;
    case JFS_HT_SIGMOID:
      /* normaliseres saa 0.0->-10.0, 1.0->10.0 foer beregning */
      res = 1.0 / (1.0 + pow(2.71828,
                             -(20.0 * f - 10.0) * hedge->hedge_arg));
      break;
    case JFS_HT_ROUND:
      if (f >= hedge->hedge_arg)
        res = 1.0;
      else
        res = 0.0;
      break;
    case JFS_HT_YNOT:
      res = pow(1.0 - pow(f, hedge->hedge_arg),
                1.0 / hedge->hedge_arg);
      break;
    case JFS_HT_BELL:
      if (f <= hedge->hedge_arg)
        res = pow(f, 2.0 - (f / hedge->hedge_arg));
      else
        res = pow(f, (f + hedge->hedge_arg - 2.0)
                    / (2 * hedge->hedge_arg - 2.0));
   break;
    case JFS_HT_TCUT:
      if (f > hedge->hedge_arg)
        res = 1.0;
      else
        res = f;
      break;
    case JFS_HT_BCUT:
      if (f < hedge->hedge_arg)
        res = 0.0;
      else
        res = f;
      break;
    case JFS_HT_LIMITS:
      res = jfr_limit_calc(f, hedge->f_limit_no, hedge->limit_c);
      break;
  }
  return res;
}

static void jfr_set_prog_id(void)
{
  /* remember program counter etc.          */

  jfrr.program_id = jfrr.pc - 1;
  jfrr.rule_no++;
  jfrr.changed = 0;
}

static float jfr_fuzificate(int fzvar_no, float value)
{
  /* calculates the value of fuzzification(<value>) for <fzvar_no>. */
  /* Do not change the value of <fz_var> (returns the value).       */

  struct jfr_var_desc *var;
  struct jfr_adjectiv_desc *adjectiv;
  struct jfr_fzvar_desc *fzvar;
  int adj_no;
  float res, pre, post;

  fzvar = &(jfr_head->fzvars[fzvar_no]);
  var = &(jfr_head->vars[fzvar->var_no]);
  adj_no = var->f_adjectiv_no + fzvar_no - var->f_fzvar_no;
  adjectiv = &(jfr_head->adjectives[adj_no]);
  if (adjectiv->limit_c == 0)     /* No pl-function, calculate from */
                                  /* trapez.                        */
  { if (value < adjectiv->trapez_start)
    { if (fzvar_no == var->f_fzvar_no)
        res = 1.0;
      else
      { pre = jfr_head->adjectives[adj_no - 1].trapez_end;
        if (value <= pre)
          res = 0.0;
        else
          res = (value - pre) / (adjectiv->trapez_start - pre);
      }
    }
    else
    if (value > adjectiv->trapez_end)
    { if (fzvar_no == var->f_fzvar_no + var->fzvar_c - 1)
        res = 1.0;
      else
      { post = jfr_head->adjectives[adj_no + 1].trapez_start;
        if (value >= post)
          res = 0.0;
        else
          res = (post - value) / (post - adjectiv->trapez_end);
      }
    }
    else  /* val in [trapez_start, trapez_end] */
      res = 1.0;
  }
  else  /* multivalue adjectiv */
    res = jfr_limit_calc(value,
                         adjectiv->f_limit_no, adjectiv->limit_c);

  if (adjectiv->flags & JFS_AF_HEDGE)
  { if (value <= adjectiv->center)
      res = jfr_ihedge_calc(adjectiv->h1_no, res);
    else
      res = jfr_ihedge_calc(adjectiv->h2_no, res);
  }
  return res;
}

static void jfr_defuz(unsigned short var_no)
{
   /* Calculates the value of <var_no> by defuzzification.      */
   int fv, m, defuz_func;
   float sum, f, e, b, tael, naevn, res, res_1, max_conf;
   float values[256];
   struct jfr_var_desc *ovar;
   struct jfr_domain_desc *domain;
   struct jfr_adjectiv_desc *adjectiv;

   ovar = &(jfr_head->vars[var_no]);

   /* normalise af fzvars */
   sum = 0.0;
   max_conf = 0.0;
   for (fv = 0; fv < ovar->fzvar_c; fv++)
   { if (jfr_head->fzvars[ovar->f_fzvar_no + fv].value < ovar->acut)
       values[fv] = 0.0;
     else
       values[fv] = jfr_head->fzvars[ovar->f_fzvar_no + fv].value;
     if (values[fv] > max_conf)
       max_conf = values[fv];
     sum += values[fv];
   }
   if (sum == 0.0)      /* Normalisation imposible */
   { ovar->value = ovar->default_val;
     ovar->conf = 0.0;
     jfr_err_set(204);
     return ;                     /* RETURN */
   }

   for (fv = 0; fv < ovar->fzvar_c; fv++)
     values[fv] = values[fv] / sum;

   defuz_func = ovar->defuz_1;
   for (m = 0; m < 2; m++)
   { switch (defuz_func)
     { case JFS_VD_CENTROID:
         tael = naevn = 0.0;
         for (fv = 0; fv < ovar->fzvar_c; fv++)
         { /* arealet af 3-kant = .5 * h›jde * grundlinie */
           adjectiv = &(jfr_head->adjectives[ovar->f_adjectiv_no + fv]);
           f = 0.5 * adjectiv->base * values[fv];
           tael += f * adjectiv->center;
           naevn += f;
         }
         if (naevn == 0.0)
           res = 0.0;
         else
           res = tael / naevn;
         break;
       case JFS_VD_CMAX:
       case JFS_VD_FMAX:
       case JFS_VD_LMAX:
         for (fv = 0; fv < ovar->fzvar_c; fv++)
         { adjectiv = &(jfr_head->adjectives[ovar->f_adjectiv_no + fv]);
           if (fv == 0 || values[fv] > f)
           { b = e = adjectiv->center;
             f = values[fv];
           }
           else
           if (values[fv] == f)
           { e = adjectiv->center;
           }
         }
         if (defuz_func == JFS_VD_CMAX)
           res = (b + e) / 2.0;       /* midtpunktet */
         else
         if (defuz_func == JFS_VD_FMAX)
           res = b;
         else
         if (defuz_func == JFS_VD_LMAX)
           res = e;
         break;
       case JFS_VD_AVG:
         res = 0.0;
         for (fv = 0; fv < ovar->fzvar_c; fv++)
           res += values[fv]
                  * jfr_head->adjectives[ovar->f_adjectiv_no + fv].center;
         break;
       case JFS_VD_FIRST:
         domain = &(jfr_head->domains[ovar->domain_no]);
         if ((domain->flags & JFS_DF_MINENTER) == 0
             || (domain->flags & JFS_DF_MAXENTER) == 0)
           res = jfr_head->fzvars[ovar->f_fzvar_no].value;
         else
           res = (domain->dmax - domain->dmin) *
              jfr_head->fzvars[ovar->f_fzvar_no].value + domain->dmin;
         break;
     }
     if (m == 0)
     { res_1 = res;
       defuz_func = ovar->defuz_2;
       if (ovar->defuz_1 == ovar->defuz_2)
      break;  /* m = 7; */
     }
     else
       res = (1.0 - ovar->defuz_arg) * res_1 + ovar->defuz_arg * res;
   } /* for (m */

   ovar->value = res;
   ovar->conf = max_conf;
   jfr_vround(var_no);
}

static void jfr_normal(unsigned short var_no)
{
  /* Normalise(<var_no>) (part of fuzzification).                      */
  struct jfr_var_desc *var;
  int m;
  float sum, na;

  var = &(jfr_head->vars[var_no]);
  if ((var->flags & JFS_VF_NORMAL))
  { sum = 0.0;
    for (m = 0; m < var->fzvar_c; m++)
      sum += jfr_head->fzvars[var->f_fzvar_no + m].value;
    if (sum == 0.0)
      jfr_err_set(201);
    else
    { if (var->no_arg > 0.0)
        na = var->no_arg;
      else
        na = var->conf;
      sum = na / sum;
      for (m = 0; m < var->fzvar_c; m++)
        jfr_head->fzvars[var->f_fzvar_no + m].value *= sum;
    }
  }
}

static void jfr_ifzvput(unsigned short fzvar_no, float conf)
{
  /* Assigns the fuzzy-var <fzvar_no> the value <conf>.           */

  struct jfr_fzvar_desc *fzvar;
  struct jfr_var_desc *var;
  float ov;

  fzvar = &(jfr_head->fzvars[fzvar_no]);
  var = &(jfr_head->vars[fzvar->var_no]);

  ov = jfr_ifzvget(fzvar_no);
  fzvar->value = jfr_fzround(conf);

  if (fzvar->value != ov || var->status == JFS_VST_UNDEFINED)
  { var->status = JFS_VST_FUZCHANGED;
    jfrr.changed = 1;
  }
}

static int jfr_indeks(int ano, float fid)
{
  struct jfr_array_desc *adesc;
  int id;
  double r, t;

  adesc = &(jfr_head->arrays[ano]);
  t = modf((double) fid, &r);
  if (t >= 0.5)
    r = r + 1.0;
  id = (int) r;
  if (id < 0)
    id = 0;
  else
  if (id >= adesc->array_c)
    id = adesc->array_c - 1;
  return id;
}

static void jfr_iaset(int ano, float id, float val)
{
  int i;
  struct jfr_array_desc *adesc;

  adesc = &(jfr_head->arrays[ano]);
  i = jfr_indeks(ano, id) + adesc->f_array_id;
  if (jfr_head->array_vals[i] != val)
  { jfr_head->array_vals[i] = val;
    jfrr.changed = 1;
  }
}

static float jfr_iaget(int ano, float fid)
{
  int i;
  struct jfr_array_desc *adesc;
  float res;

  adesc = &(jfr_head->arrays[ano]);
  i = jfr_indeks(ano, fid);
  res = jfr_head->array_vals[adesc->f_array_id + i];
  return res;
}

static void jfr_ifzvupd(unsigned short fzvar_no, float conf)
{
  float c, ov;
  struct jfr_fzvar_desc *fzvar;
  struct jfr_var_desc *var;

  c = conf;
  if (c != 0.0)
  { fzvar = &(jfr_head->fzvars[fzvar_no]);
    var = &(jfr_head->vars[fzvar->var_no]);
    ov = jfr_ifzvget(fzvar_no);
    fzvar->value = jfr_iop_calc(var->f_comp, fzvar->value, c);
    if (fzvar->value != ov || var->status == JFS_VST_UNDEFINED)
    { var->status = JFS_VST_FUZCHANGED;
      jfrr.changed = 1;
    }
  }
}

static void jfr_ivput(unsigned short var_no, float value, float conf)
{
  /* Assigns the domain_var <var_no> with the value <value>:<conf>. */

  struct jfr_var_desc *var;
  float ov, oc;

  if (conf < 0.0)
  { if (conf == -1.0)
      jfr_iclear(var_no, JFR_FCLR_ONE);
    else
    if (conf == -2.0)
      jfr_iclear(var_no, JFR_FCLR_AVG);
    else
      jfr_iclear(var_no, JFR_FCLR_ZERO);
  }
  else
  { var = &(jfr_head->vars[var_no]);
    ov = jfr_ivget(var_no);
    oc = var->conf;
    var->conf = jfr_fzround(conf);
    var->value = value;
    var->conf_sum = conf;

    jfr_vround(var_no);
    if (var->value != ov || var->conf != oc || var->status == JFS_VST_UNDEFINED)
    { var->status = JFS_VST_DOMCHANGED;
      jfrr.changed = 1;
    }
  }
}

static void jfr_ivupd(unsigned short var_no, float value, float conf)
{
  /* Updates the domain_var <var_no> with the value <value>:<conf>. */
  /* NB Ignores weight, if-hedge, etc (see jfr_vpop, jfr_vupd).     */

  struct jfr_var_desc *var;
  float c, oc, ov, f;

  var = &(jfr_head->vars[var_no]);
  ov = jfr_ivget(var_no);
  oc = var->conf;
  c = jfr_fzround(conf);
  if (c != 0.0)
  { switch (var->d_comp)
    { case JFS_VCF_NEW:
        var->value = value;
        var->conf = c;
        break;
      case JFS_VCF_AVG:
        f = var->value * var->conf_sum + value * c;
        var->conf_sum += c;
        var->value = f / var->conf_sum;
        if (c > var->conf)
          var->conf = c;
        break;
      case JFS_VCF_MAX:
        if (c > var->conf)
        { var->value = value;
          var->conf = c;
        }
        break;
    }
    jfr_vround(var_no);
    if (var->value != ov || var->conf != oc || var->status == JFS_VST_UNDEFINED)
    { var->status = JFS_VST_DOMCHANGED;
      jfrr.changed = 1;
    }
  }
}

static void jfr_ivincdec(unsigned short var_no, float value, float conf)
{
  /* Increases the value of <var_no> with <value> * <conf>. */

  struct jfr_var_desc *var;
  float c, oc, ov;

  var = &(jfr_head->vars[var_no]);
  ov = jfr_ivget(var_no);
  oc = var->conf;
  c = jfr_fzround(conf);
  if (c != 0.0)
  {
    var->value += c * value;
    if (c > var->conf)
      var->conf = c;
    jfr_vround(var_no);
  }
  if (var->value != ov || var->conf != oc || var->status == JFS_VST_UNDEFINED)
  { var->status = JFS_VST_DOMCHANGED;
    jfrr.changed = 1;
  }
}

static float jfr_ivget(unsigned short var_no)
{
  /* Returns the value of the domain-var <var_no>. If any of the bound */
  /* fzvars has changed, the value is recalcuated (defuz).             */

  struct jfr_var_desc *var;

  var = &(jfr_head->vars[var_no]);
  if (var->status == JFS_VST_UNDEFINED)
  { var->value = var->default_val;
  }
  else
  { if (var->status == JFS_VST_FUZCHANGED)
    { jfr_defuz(var_no);
      var->status = 0;
    }
  }
  return var->value;
}


static void jfr_op_vincdec(int sign)
{
  /* increase/decrease operation (called from jfr_op_ic, jfr_op_decr */
  unsigned short var_no;
  float value;

  var_no = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&value);
  jfrr.expr_value = value;
  jfr_ivincdec(var_no, ((float) sign) * value, jfrr.weight);
  if (jfr_mcall != NULL && jfrr.weight != 0.0)
    jfr_mcall(1);
  jfrr.weight = jfrr.rm_weight;
}


static float jfr_ifzvget(unsigned short fzvar_no)
{
  /* Returns the value of the fuzzy-variable <fzvar_no>. If the domain- */
  /* var <fzvar_no> is bound to has chaged the value are recalculated.  */

  struct jfr_fzvar_desc *fzvar;
  struct jfr_var_desc *var;
  int m;

  fzvar = &(jfr_head->fzvars[fzvar_no]);
  var = &(jfr_head->vars[fzvar->var_no]);
  if (var->status == JFS_VST_DOMCHANGED)
  { for (m = 0; m < var->fzvar_c; m++)
      jfr_head->fzvars[var->f_fzvar_no + m].value
        = jfr_fuzificate(var->f_fzvar_no + m, var->value);
    jfr_normal(fzvar->var_no);
    var->status = 0;
  }
  return fzvar->value;
}

static void jfr_iclear(unsigned short var_no, int fzvclear)
{
  /* Clears the domain-variable <var_no>.                           */
  struct jfr_var_desc *var;
  float clearval;
  int m;

  var = &(jfr_head->vars[var_no]);
  var->conf = 0.0;
  var->conf_sum = 0.0;
  var->value = var->default_val;
  var->status = JFS_VST_UNDEFINED;
  jfrr.changed = 1;

  clearval = 0.0;
  if (fzvclear == JFR_FCLR_ONE)
    clearval = 1.0;
  else
  if (fzvclear == JFR_FCLR_AVG && var->fzvar_c > 0)
    clearval = 1.0 / ((float) var->fzvar_c);
  for (m = 0; m < var->fzvar_c; m++)
    jfr_head->fzvars[var->f_fzvar_no + m].value = clearval;
}

static unsigned short jfr_fzvno(void)
{
  unsigned short fzvno;
  /* reads a fzvar-number from program.          */

  fzvno = *jfrr.pc;
  jfrr.pc++;
  if (jfr_head->vbytes == 2)
  { fzvno = 256 * fzvno + *jfrr.pc;
    jfrr.pc++;
  }
  return fzvno;
}

static float jfr_ivfunc_calc(unsigned short fno, unsigned short vno)
{
  /* internal variable-function calculation.    */
  float fval, res, vv;
  struct jfr_var_desc *var;
  struct jfr_domain_desc *domain;
  unsigned short m;

  var = &(jfr_head->vars[vno]);
  vv = jfr_ivget(vno);

  switch (fno)
  { case JFS_VFU_DNORMAL:
      domain = &(jfr_head->domains[var->domain_no]);
      if ((domain->flags & JFS_DF_MINENTER) == 0
          || (domain->flags & JFS_DF_MAXENTER) == 0)
        res = vv;
      else
      { if (domain->dmax != domain->dmin)
          res = (vv - domain->dmin) / (domain->dmax - domain->dmin);
        else
          res = domain->dmin;
      }
      break;
    case JFS_VFU_M_FZVAR:
      for (m = 0; m < var->fzvar_c; m++)
      { fval = jfr_ifzvget(var->f_fzvar_no + m);
        if (m == 0 || fval > res)
          res = fval;
      }
      break;
    case JFS_VFU_S_FZVAR:
      res = 0.0;
      for (m = 0; m < var->fzvar_c; m++)
        res += jfr_ifzvget(var->f_fzvar_no + m);
      break;
    case JFS_VFU_DEFAULT:
      res = var->default_val;
      break;
    case JFS_VFU_CONFIDENCE:
      res = var->conf;
      break;
  }
  return res;
}

static float jfr_ibetween_calc(unsigned short vno, unsigned short c1,
                               unsigned short c2)
{
  float vv, res;
  struct jfr_var_desc *var;

  var = &(jfr_head->vars[vno]);
  vv = jfr_ivget(vno);
  if (vv < jfr_head->adjectives[var->f_adjectiv_no + c1].trapez_start)
    res = jfr_fuzificate(var->f_fzvar_no + c1, vv);
  else
  if (vv > jfr_head->adjectives[var->f_adjectiv_no + c2].trapez_end)
    res = jfr_fuzificate(var->f_fzvar_no + c2, vv);
  else
    res = 1.0;
  return res;
}

static float jfr_idfunc_calc(unsigned short fno, float a1, float a2)
{
  /* Internal 2-arg-function calc (<fno>(<a1>,<a2>)). Also calc of  */
  /* predefined operators and relations.                            */

  float res;

  res = 0.0;
  switch (fno)
  { case JFS_DFU_PLUS:
      res = a1 + a2;
      break;
    case JFS_DFU_MINUS:
      res = a1 - a2;
      break;
    case JFS_DFU_PROD:
      res = a1 * a2;
      break;
    case JFS_DFU_DIV:
      if (a2 == 0.0)
     jfr_err_set(205);
      else
     res = a1 / a2;
      break;
    case JFS_DFU_POW:
      if ((a1 == 0.0 && a2 < 0.0)
          || (a1 < 0.0 && a2 != floor(a2)))
        jfr_err_set(205);
      else
        res = pow(a1, a2);
      break;
    case JFS_DFU_MIN:
      if (a1 < a2)
        res = a1;
      else
        res = a2;
      break;
    case JFS_DFU_MAX:
      if (a1 > a2)
        res = a1;
      else
        res = a2;
      break;
    case JFS_DFU_CUT:
      if (a1 > a2)
        res = a1;
      else
        res = 0.0;
      break;
    case JFS_ROP_EQ:
      if (a1 == a2)
        res = 1.0;
      break;
    case JFS_ROP_NEQ:
      if (a1 != a2)
        res = 1.0;
      break;
    case JFS_ROP_GT:
      if (a1 > a2)
        res = 1.0;
      break;
    case JFS_ROP_GEQ:
      if (a1 >= a2)
        res = 1.0;
      break;
    case JFS_ROP_LT:
      if (a1 < a2)
        res = 1.0;
      break;
    case JFS_ROP_LEQ:
      if (a1 <= a2)
        res = 1.0;
      break;
  }
  return res;
}

static float jfr_irel_calc(unsigned short rno, float v1, float v2)
{
  /* Internal relation-calc.                                    */
  float r;
  struct jfr_relation_desc *rdesc;

  rdesc = &(jfr_head->relations[rno]);
  r = jfr_limit_calc(v1 - v2, rdesc->f_limit_no, rdesc->limit_c);
  if (rdesc->flags & JFS_RF_HEDGE)
    r = jfr_ihedge_calc(rdesc->hedge_no, r);

  return r;
}

static float jfr_isfunc_calc(unsigned short fno, float a)
{
  /* Internal single-argument-function calculation (<fno>(<a>)).    */
  float r;

  r = 0.0;
  switch (fno)
  { case JFS_SFU_COS:
      r = cos(a);
      break;
    case JFS_SFU_SIN:
      r = sin(a);
      break;
    case JFS_SFU_TAN:
      r = cos(a);
      if (r == 0.0)
        jfr_err_set(205);
      else
        r = sin(a) / r;
      break;
    case JFS_SFU_ACOS:
      if (a < -1.0 || a > 1.0)
        jfr_err_set(205);
      else
        r = acos(a);
      break;
    case JFS_SFU_ASIN:
      if (a < -1.0 || a > 1.0)
        jfr_err_set(205);
      else
        r = asin(a);
      break;
    case JFS_SFU_ATAN:
      r = atan(a);
      break;
    case JFS_SFU_LOG:
      if (a <= 0.0)
        jfr_err_set(205);
      else
        r = log(a);
      break;
    case JFS_SFU_FABS:
      r = fabs(a);
      break;
    case JFS_SFU_FLOOR:
      r = floor(a);
      break;
    case JFS_SFU_CEIL:
      r = ceil(a);
      break;
    case JFS_SFU_NEGATE:
      r = -a;
      break;
    case JFS_SFU_RANDOM:
      r = (((float) rand()) / ((float) RAND_MAX)) * a;
      break;
    case JFS_SFU_SQR:
      r = a * a;
      break;
    case JFS_SFU_SQRT:
      if (a >= 0)
        r = sqrt(a);
      else
        jfr_err_set(205);
      break;
    case JFS_SFU_WGET:
      if (jfrr.weight > a)
        r = jfrr.weight;
      else
        r = 0.0;
      break;
  }
  return r;
}

static float jfr_iop_calc(unsigned short opno, float a_1, float a_2)
{
  /* Internal fuzzy-operator calc (<arg_1> <opno> <arg_2>).           */
  float tres, res, arg_1, arg_2;
  int m, op_no;
  struct jfr_operator_desc *ope;

  ope = &(jfr_head->operators[opno]);
  op_no = ope->op_1;
  if (ope->hedge_mode == JFS_OHM_ARG1 || ope->hedge_mode == JFS_OHM_ARG12)
    arg_1 = jfr_ihedge_calc(ope->hedge_no, a_1);
  else
    arg_1 = a_1;
  if (ope->hedge_mode == JFS_OHM_ARG2 || ope->hedge_mode == JFS_OHM_ARG12)
    arg_2 = jfr_ihedge_calc(ope->hedge_no, a_2);
  else
    arg_2 = a_2;
  res = 0.0;
  for (m = 0; m < 2; m++)
  { switch (op_no)
    { case JFS_FOP_MIN:              /* min */
        if (arg_1 < arg_2)
          res = arg_1;
        else
          res = arg_2;
        break;
      case JFS_FOP_MAX:              /* max */
        if (arg_1 > arg_2)
          res = arg_1;
        else
          res = arg_2;
        break;
      case JFS_FOP_PROD:              /* prod */
        res = arg_1 * arg_2;
        break;
      case JFS_FOP_PSUM:              /* psum */
        res = arg_1 + arg_2 - arg_1 * arg_2;
        break;
      case JFS_FOP_AVG:              /* avg NB: IKKE comp-avg */
        res = (arg_1 + arg_2) / 2.0;
        break;
      case JFS_FOP_BSUM:              /* bsum */
        res = arg_1 + arg_2;
        if (res > 1.0)
          res = 1.0;
        break;
      case JFS_FOP_BUNION:            /* Bold union */
        res = arg_1 + arg_2 -1.0;
        if (res < 0.0)
          res = 0.0;
        break;
      case JFS_FOP_SIMILAR:
        if (arg_1 == arg_2)
          res = 1.0;
        else
        { if (arg_1 < arg_2)
            res = arg_1 / arg_2;
          else
            res = arg_2 / arg_1;
        }
        break;
      case JFS_FOP_NEW:               /* new */
        res = arg_2;
        break;
      case JFS_FOP_MXOR:
        if (arg_1 >= arg_2)
          res = arg_1 - arg_2;
        else
          res = arg_2 - arg_1;
        break;
      case JFS_FOP_SPTRUE:
        res = (2.0 * arg_1 - 1.0) * (2.0 * arg_1 - 1.0) *
              (2.0 * arg_2 - 1.0) * (2.0 * arg_2 - 1.0);
        break;
      case JFS_FOP_SPFALSE:
        res = 1.0 - ((2.0 * arg_1 - 1.0) * (2.0 * arg_1 - 1.0) *
                     (2.0 * arg_2 - 1.0) * (2.0 * arg_2 - 1.0));
        break;
      case JFS_FOP_SMTRUE:
        res = 1.0 - 16.0 * (arg_1 - arg_1 * arg_1) * (arg_2 - arg_2 * arg_2);
        break;
      case JFS_FOP_SMFALSE:
        res = 16.0 * (arg_1 - arg_1 * arg_1) * (arg_2 - arg_2 * arg_2);
        break;
      case JFS_FOP_R0:
        res = 0.0;
        break;
      case JFS_FOP_R1:
        res = arg_1 * arg_2 - arg_1 - arg_2 + 1;
        break;
      case JFS_FOP_R2:
        res = arg_2 - arg_1 * arg_2;
        break;
      case JFS_FOP_R3:
        res = 1.0 - arg_1;
        break;
      case JFS_FOP_R4:
        res = arg_1 - arg_1 * arg_2;
        break;
      case JFS_FOP_R5:
        res = 1.0 - arg_2;
        break;
      case JFS_FOP_R6:
        res = arg_1 + arg_2 - 2.0 * arg_1 * arg_2;
        break;
      case JFS_FOP_R7:
        res = 1.0 - arg_1 * arg_2;
        break;
      case JFS_FOP_R8:
        res = arg_1 * arg_2;
        break;
      case JFS_FOP_R9:
        res = 1.0 - arg_1 - arg_2 + 2.0 * arg_1 * arg_2;
        break;
      case JFS_FOP_R10:
        res = arg_2;
        break;
      case JFS_FOP_R11:
        res = 1.0 - arg_1 + arg_1 * arg_2;
        break;
      case JFS_FOP_R12:
        res = arg_1;
        break;
      case JFS_FOP_R13:
        res = arg_1 * arg_2 - arg_2 + 1.0;
        break;
      case JFS_FOP_R14:
        res = arg_1 + arg_2 - arg_1 * arg_2;
        break;
      case JFS_FOP_R15:
        res = 1.0;
        break;

      case JFS_FOP_HAMAND:                /* hamand */
        res = (arg_1 * arg_2)
             / (ope->op_arg + (1.0 - ope->op_arg)
             * (arg_1 + arg_2 - arg_1 * arg_2));
        break;
      case JFS_FOP_HAMOR:               /* hamor */
        res =   (arg_1 + arg_2 - (2.0 - ope->op_arg) * arg_1 * arg_2)
              / (1.0 - (1.0 - ope->op_arg) * arg_1 * arg_2);
        break;
      case JFS_FOP_YAGERAND:               /* yager and */
        res = pow(pow(1.0 - arg_1, ope->op_arg)
             + pow(1.0 - arg_2, ope->op_arg),
                   1.0 / ope->op_arg);
        if (res > 1.0)
          res = 0.0;
        else
          res = 1.0 - res;
        break;
      case JFS_FOP_YAGEROR:               /* yager or */
        res = pow(pow(arg_1, ope->op_arg) + pow(arg_2, ope->op_arg),
                  1.0 / ope->op_arg);
        if (res > 1.0)
          res = 1.0;
        break;
    }
    if (m == 0)
    { tres = res;
      if (ope->op_1 == ope->op_2)
        break;
      else
        op_no = ope->op_2;
    }
    else
      tres = tres * (1.0 - ope->op_arg) + res * ope->op_arg;
  }  /* for */
  if (ope->hedge_mode == JFS_OHM_POST)
    tres = jfr_ihedge_calc(ope->hedge_no, tres);
  return tres;
}

/*************************************************************************/
/* operator functions                                                    */
/*************************************************************************/

static void jfr_op_wset(void)
{
  float f;

  jfr_fpop(&f);
  f = jfr_fzround(f);
  jfrr.expr_value = f;
  if (jfrr.weight != 0.0)
    jfrr.weight = f;
  if (jfr_mcall != NULL)
    jfr_mcall(1);
}

static void jfr_op_spush(void)
{
  /* push a function value onto the stack */
  unsigned char *lv;
  int lvno;

  lvno = *jfrr.pc;
  jfrr.pc++;
  lv = jfrr.stack_id + lvno * sizeof(float);
  jfr_fpush(lv);
}


static void jfr_op_iif(void)
{
  float r, r1, r2, e;

  jfr_fpop(&r2);
  jfr_fpop(&r1);
  jfr_fpop(&e);
  e = jfr_fzround(e);
  r = e * r1 + (1.0 - e) * r2;
  jfr_fpush(&r);
}


static void jfr_op_array(void)
{
  unsigned char ano;
  float fid, val;

  ano = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&fid);
  val = jfr_iaget(ano, fid);
  jfr_fpush(&val);
}

static void jfr_op_freturn(void)
{
  /* function-return */
  float res;
  struct jfr_fu_stack_desc *fsdesc;
  struct jfr_function_desc *fudesc;
  /* temporary pop the return-value */
  jfr_fpop(&res);
  jfrr.expr_value = res;
  if (jfrr.weight != 0.0)
  { if (jfr_mcall != NULL)
      jfr_mcall(1);
    /* pop jfrr-values from stack:*/
    /* jfrr.ff_stack -= JFR_FU_STACK_SIZE; WRONG !!! return in switch-block */
    fudesc = &(jfr_head->functions[jfrr.function_no]);
    jfrr.ff_stack = jfrr.stack_id + fudesc->arg_c * sizeof(float);

    fsdesc = (struct jfr_fu_stack_desc *) jfrr.ff_stack;
    jfrr.weight      = fsdesc->weight;
    jfrr.cond_value  = fsdesc->cond_value;
    jfrr.rm_weight   = fsdesc->rm_weight;
    jfrr.pc          = fsdesc->pc;
    jfrr.program_id  = fsdesc->program_id;
    jfrr.stack_id    = fsdesc->stack_id;
    jfrr.function_no = fsdesc->function_no;
    jfrr.rule_no     = fsdesc->rule_no;
    /* pop (remove) function-args from stack: */
    jfrr.ff_stack -= fsdesc->fuarg_c * sizeof(float);

    /* put the return-value back on the stack: */
    jfr_fpush(&res);
  }
  else
      jfrr.weight = jfrr.rm_weight;
  /* put the return-value back on the stack: */
  /* jfr_fpush(&res);        */
}

static void jfr_op_endfunc(void)
{
  /* procedure-end or function-end with no return */
  struct jfr_fu_stack_desc *fudesc;
  float r;
  int husktype;

  jfr_set_prog_id();
  /* pop jfr-values from stack: */
  jfrr.ff_stack -= JFR_FU_STACK_SIZE;
  fudesc = (struct jfr_fu_stack_desc *) jfrr.ff_stack;
  husktype = jfr_head->functions[jfrr.function_no].type;
  jfrr.weight      = fudesc->weight;
  jfrr.cond_value  = fudesc->cond_value;
  jfrr.rm_weight   = fudesc->rm_weight;
  jfrr.pc          = fudesc->pc;
  jfrr.program_id  = fudesc->program_id;
  jfrr.stack_id    = fudesc->stack_id;
  jfrr.rule_no     = fudesc->rule_no;
  jfrr.function_no = fudesc->function_no;
  /* pop (remove) funcarg-values from stack */
  jfrr.ff_stack -= fudesc->fuarg_c * sizeof(float);

  if (husktype == JFS_FT_FUNCTION)
  { /* function without return */
    r = 0.0;
    jfr_fpush(&r);
  }
  else
  { /* end the 'if <e> then proc();'-statement: */
    jfrr.weight = jfrr.rm_weight;
  }
}

static void jfr_op_userfunc(void)
{
  unsigned char fno;
  struct jfr_function_desc *fudesc;
  struct jfr_fu_stack_desc *fsdesc;

  fno = *jfrr.pc;
  fudesc = &(jfr_head->functions[fno]);
  jfrr.pc++;
  if (jfrr.ff_stack + JFR_FU_STACK_SIZE >= jfrr.max_stack)
    jfr_err_set(206);
  else
  { fsdesc = (struct jfr_fu_stack_desc *) jfrr.ff_stack;
    fsdesc->weight      = jfrr.weight;
    fsdesc->rm_weight   = jfrr.rm_weight;
    fsdesc->cond_value  = jfrr.cond_value;
    fsdesc->pc          = jfrr.pc;
    fsdesc->program_id  = jfrr.program_id;
    fsdesc->stack_id    = jfrr.stack_id;
    fsdesc->fuarg_c     = fudesc->arg_c;
    fsdesc->function_no = jfrr.function_no;
    fsdesc->rule_no     = jfrr.rule_no;
    jfrr.pc = jfr_head->function_code + fudesc->pc;
    jfrr.stack_id = jfrr.ff_stack - fudesc->arg_c * sizeof(float);
    jfrr.ff_stack += JFR_FU_STACK_SIZE;
    jfrr.rule_no = 0;
    jfrr.function_no = fno;
  }
}

static void jfr_op_apop(void)
{
  /* pop an array-value */
  int ano;
  float val, id;

  ano = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&val);
  jfr_fpop(&id);

  jfrr.expr_value = val;
  jfrr.index_value = id;
  if (jfrr.weight > 0)
  { jfr_iaset(ano, id, val);
    if (jfr_mcall != NULL)
      jfr_mcall(1);
  }
  jfrr.weight = jfrr.rm_weight;
}

static void jfr_op_spop(void)
{
  unsigned char *lv;
  int lvno;
  float r;

  /* pop a value into a function-variable */
  lvno = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&r);
  jfrr.expr_value = r;
  if (jfrr.weight > 0)
  { lv = jfrr.stack_id + lvno * sizeof(float);
    memcpy(lv, (unsigned char *) &r, sizeof(float));
    if (jfr_mcall != NULL)
      jfr_mcall(1);
  }
  jfrr.weight = jfrr.rm_weight;
}

static void jfr_op_comment(void)
{
  jfrr.pc += 2;
}

static void jfr_op_vfunc(void)
{
  float res;
  unsigned char fno;
  unsigned char var_no;

  /* varaible function */
  fno = *jfrr.pc;
  jfrr.pc++;
  var_no = *jfrr.pc;
  jfrr.pc++;
  res = jfr_ivfunc_calc(fno, var_no);
  jfr_fpush(&res);
}

static void jfr_op_between(void)
{
  float res;
  unsigned short var_no;
  unsigned short cu_no_1, cu_no_2;

  var_no = *jfrr.pc;
  jfrr.pc++;
  cu_no_1 = *jfrr.pc;
  jfrr.pc++;
  cu_no_2 = *jfrr.pc;
  jfrr.pc++;
  res = jfr_ibetween_calc(var_no, cu_no_1, cu_no_2);
  jfr_fpush(&res);
}

static void jfr_op_clear(void)
{
  int var_no;

  var_no = *jfrr.pc;
  jfrr.pc++;
  if (jfrr.weight != 0.0)
  { jfr_iclear(var_no, JFR_FCLR_ZERO);
    if (jfr_mcall != NULL)
      jfr_mcall(1);
  }
  jfrr.weight = jfrr.rm_weight;
}

static void jfr_op_switch(void)
{
  struct jfr_bl_stack_desc *bl;

  jfr_set_prog_id();
  if (jfrr.ff_stack + JFR_BL_STACK_SIZE >= jfrr.max_stack)
    jfr_err_set(206);
  else
  { bl = (struct jfr_bl_stack_desc *) jfrr.ff_stack;
    bl->weight = jfrr.weight;
    bl->sum = 0.0;
    jfrr.ff_stack += JFR_BL_STACK_SIZE;
  }
}

static void jfr_op_vswitch(void)
{
  struct jfr_bl_stack_desc *bl;

  jfr_set_prog_id();
  jfrr.pc++; /* ignore varno */
  if (jfrr.ff_stack + JFR_BL_STACK_SIZE >= jfrr.max_stack)
    jfr_err_set(206);
  else
  { bl = (struct jfr_bl_stack_desc *) jfrr.ff_stack;
    bl->weight = jfrr.weight;
    bl->sum = 0.0;
    jfrr.ff_stack += JFR_BL_STACK_SIZE;
  }
}

static void jfr_op_default(void)
{
  float f;
  struct jfr_bl_stack_desc *bl;

  jfr_set_prog_id();
  bl = (struct jfr_bl_stack_desc *) (jfrr.ff_stack - JFR_BL_STACK_SIZE);
  f = 1.0 - bl->sum;
  if (f < 0.0)
    f = 0.0;
  jfrr.cond_value = f;

  f = jfr_iop_calc(JFS_ONO_CASEOP, bl->weight, f);

  if (jfrr.weight != f)
  { jfrr.changed = 1;
    jfrr.weight = f;
  }
  if (jfrr.weight != 0.0 && jfr_mcall != NULL)
    jfr_mcall(1);
}

static void jfr_op_case(void)
{
  float f, res;
  struct jfr_bl_stack_desc *bl;

  jfr_fpop(&f);
  f = jfr_fzround(f);
  jfrr.cond_value = f;

  bl = (struct jfr_bl_stack_desc *) (jfrr.ff_stack - JFR_BL_STACK_SIZE);
  bl->sum += f;
  res = jfr_iop_calc(JFS_ONO_CASEOP, bl->weight, f);
  if (jfrr.weight != res)
  { jfrr.changed = 1;
    jfrr.weight = res;
  }
  if (jfrr.weight != 0.0 && jfr_mcall != NULL)
    jfr_mcall(1);
}

static void jfr_op_while(void)
{
  float f, r;
  struct jfr_bl_stack_desc *bl;

  jfr_fpop(&f);
  f = jfr_fzround(f);
  jfrr.cond_value = f;
  bl = (struct jfr_bl_stack_desc *) jfrr.ff_stack;
  if (jfrr.ff_stack + JFR_BL_STACK_SIZE >= jfrr.max_stack)
    jfr_err_set(206);
  else
  { bl->weight = jfrr.weight;
    bl->rule_no = jfrr.rule_no;
    bl->pc = jfrr.program_id;    /* NB jump back to begining of while-stat */
                                 /* (not to pc).                           */
    jfrr.ff_stack += JFR_BL_STACK_SIZE;
    r = jfr_iop_calc(JFS_ONO_WHILEOP, jfrr.weight, f);
    if (r != jfrr.weight)
    { jfrr.changed = 1;
      jfrr.weight = r;
    }
    bl->sum = r;
    if (jfr_mcall != NULL)
      jfr_mcall(1);
  }
}

static void jfr_op_then(void)
{
  float c;

  jfr_fpop(&c);
  c = jfr_fzround(c);
  jfrr.cond_value = c;
  jfrr.weight *= c;
}

static void jfr_op_vpop(void)
{
  unsigned short var_no;
  float value;

  var_no = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&value);
  jfrr.expr_value = value;
  jfr_ivupd(var_no, value, jfrr.weight);
  if (jfrr.weight != 0.0 && jfr_mcall != NULL)
    jfr_mcall(1);
  jfrr.weight = jfrr.rm_weight;
}

static void jfr_op_vincrease(void)
{
  jfr_op_vincdec(1);
}

static void jfr_op_vdecrease(void)
{
  jfr_op_vincdec(-1);
}

static void jfr_op_fzvpop(void)

{
  unsigned short fzvar_no;

  fzvar_no = jfr_fzvno();
  jfr_ifzvupd(fzvar_no, jfrr.weight);
  if (jfr_mcall != NULL && jfrr.weight != 0.0)
    jfr_mcall(1);
  jfrr.weight = jfrr.rm_weight;
}

static void jfr_op_endswitch(void)
{
  struct jfr_bl_stack_desc *bl;

  jfr_set_prog_id();
  jfrr.ff_stack -= JFR_BL_STACK_SIZE;
  bl = (struct jfr_bl_stack_desc *) jfrr.ff_stack;
  if (jfr_mcall != NULL && jfrr.weight != 0.0)
    jfr_mcall(1);
  jfrr.weight = bl->weight;
}

static void jfr_op_endwhile(void)
{
  struct jfr_bl_stack_desc *bl;

  jfr_set_prog_id();
  jfrr.ff_stack -= JFR_BL_STACK_SIZE;
  bl = (struct jfr_bl_stack_desc *) jfrr.ff_stack;
  if (jfrr.weight > 0.0)
  { jfrr.pc = bl->pc;
    jfrr.rule_no = bl->rule_no - 1;
    jfrr.weight = bl->weight;
  }
  else
    jfrr.weight = bl->weight;
}


static void jfr_op_true(void)
{
  float r;

  r = 1.0;
  jfr_fpush(&r);
}

static void jfr_op_false(void)
{
  float r;

  r = 0.0;
  jfr_fpush(&r);
}

static void jfr_op_expr(void)
{
  jfr_set_prog_id();
  jfrr.rm_weight = jfrr.weight;
}

static void jfr_op_wexpr(void)
{
  float f;

  jfr_set_prog_id();
  jfrr.rm_weight = jfrr.weight;
  memcpy(((char *) &f),
      jfrr.pc, sizeof(float));
  jfrr.pc += sizeof(float);
  jfrr.weight = jfr_iop_calc(JFS_ONO_WEIGHTOP, jfrr.weight, f);
}

static void jfr_op_var(void)
{
  float res;
  unsigned short var_no;
  struct jfr_var_desc *vdesc;

  var_no = *jfrr.pc;
  jfrr.pc++;
  if (jfrr.weight > 0.0)
  { vdesc = (struct jfr_var_desc *) &(jfr_head->vars[var_no]);
    if (vdesc->status == JFS_VST_UNDEFINED)
    { vdesc->status = 0; /* ok */
      if (jfr_uvget != NULL)
        jfr_uvget(var_no);
    }
  }
  res = jfr_ivget(var_no);
  jfr_fpush(&res);
}

static void jfr_op_const(void)
{
  float const_value;

  memcpy(((char *) &(const_value)),
  jfrr.pc, sizeof(float));
  jfrr.pc += sizeof(float);
  jfr_fpush(&const_value);
}

static void jfr_op_dfunc(void)
{
  int rel;
  float a1, a2, res;

  rel = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&a2);
  jfr_fpop(&a1);
  res = jfr_idfunc_calc(rel, a1, a2);
  jfr_fpush(&res);
}

static void jfr_op_urel(void)
{
  int rel;
  float r, a1, a2;

  rel = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&a2);
  jfr_fpop(&a1);

  r = jfr_irel_calc(rel, a1, a2);
  jfr_fpush(&r);
}

static void jfr_op_sfunc(void)
{
  int func;
  float r, a;

  func = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&a);
  r = jfr_isfunc_calc(func, a);
  jfr_fpush(&r);
}

static void jfr_op_hedge(void)
{
  int hno;
  float a, r;
  hno = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&a);
  r = jfr_ihedge_calc(hno, a);
  jfr_fpush(&r);
}

static void jfr_op_op(void)
{
  float res, arg_1, arg_2;
  int opno;

  opno = *jfrr.pc;
  jfrr.pc++;
  jfr_fpop(&arg_2);
  arg_2 = jfr_fzround(arg_2);
  jfr_fpop(&arg_1);
  arg_1 = jfr_fzround(arg_1);
  res = jfr_iop_calc(opno, arg_1, arg_2);
  jfr_fpush(&res);
}

static void jfr_op_call(void)
{
  unsigned short skip;

  skip = *jfrr.pc;
  jfrr.pc++;
  skip = 256 * skip + *jfrr.pc;
  jfrr.pc++;
  jfrr.pc++; /* argc */
  jfrr.pc += skip;
  if (jfrr.weight != 0.0 && jfr_call != NULL)
    jfr_call();
  jfrr.weight = jfrr.rm_weight;
}

static void jfr_op_eop(void)
{
  jfr_set_prog_id();
  jfr_head->status = 1;
}

static void jfr_op_err(void)
{
  jfr_err_set(203);
}

/*******************************************************************/
/* Run program                                                     */
/*******************************************************************/

static void jfr_program_handle(void)
{
  unsigned char oc;
  unsigned short fzvar_no, var_no;
  float v;
  struct jfr_var_desc *vdesc;

  if (jfr_mcall != NULL)
    jfr_mcall(0);
  jfr_head->status = 2;
  while (jfr_head->status == 2)
  {
    oc = *jfrr.pc; /* get command or fzvar_no */

    if (oc < JFR_OP_FIRST)     /* oc == fzvar_no */
    { fzvar_no = jfr_fzvno();
      if (jfrr.weight > 0.0)
      { var_no = jfr_head->fzvars[fzvar_no].var_no;
        vdesc = (struct jfr_var_desc *) &(jfr_head->vars[var_no]);
        if (vdesc->status == 1)  /* undefined */
        { vdesc->status = 0; /* ok */
          if (jfr_uvget != NULL)
            jfr_uvget(var_no);
        }
      }
      v = jfr_ifzvget(fzvar_no);
      jfr_fpush(&v);
    }
    else
    { jfrr.pc++;
      (*jfr_operators[oc - JFR_OP_FIRST])();
    }
  }
  if (jfr_mcall != NULL)
    jfr_mcall(2);
}

/**************************************************************************/
/* Eksternal functions                                                    */
/**************************************************************************/

int jfr_load(void **jfr_head, char *fname)
{
  return jfr_aload(jfr_head, fname, 0);
}

void jfr_run(float *op, void *head, float *ip)
{
  jfr_arun(op, head, ip, NULL, NULL, NULL, NULL);
}

int jfr_close(void *head)
{
  if (jfr_check(head) != 0)
    return 4;
  free(head);
  return 0;
}

int jfr_free(void)
{
  if (jfrr.stack != NULL)
    free(jfrr.stack);
  jfrr.stack = NULL;
  jfr_status = 0;
  return 0;
}

static int jfr_iload(void **head, char *fname, unsigned short esize)
{
  FILE *fh;
  int ar, m;
  unsigned long memsize, rsize;
  char *dest;
  char *c;
  struct jfr_ehead_desc jfh;
  struct jfr_head_desc *jfr_head;

  if ((fh = fopen(fname, "rb")) == NULL)
    return 1;
  ar = fread(&jfh, 1, JFR_EHEAD_SIZE, fh);
  if (ar < 0)
  { fclose(fh);
    return 2;
  }
  if (ar == JFR_EHEAD_SIZE)
  { if ((m = jfr_check(&jfh)) != 0)
      return m;
    memsize = jfh.a_size + JFR_HEAD_SIZE + 258 + esize;
  }
  else
    return 4;
  if (memsize > 64000)
    return 6;
  if ((c = (char *) malloc(memsize)) == NULL)
    return 6;
  dest = c + JFR_HEAD_SIZE;

  rsize = JFR_HEAD_SIZE;
  while ((ar = fread(dest, 1, 256, fh)) > 0)
  { dest += ar;
    rsize += ar;
    if (rsize + 256 >= memsize)
    { fclose(fh);
      free(c);
      return 2;
    }
  }
  fclose(fh);
  if (ar < 0)
  { free(c);
    return 2;
  }
  memcpy(c, (char *) &jfh, JFR_EHEAD_SIZE);
  *head = c;
  jfr_head = (struct jfr_head_desc *) c;
  jfr_head->status = 0;
  jfr_head->rsize = memsize - JFR_HEAD_SIZE;
  jfr_head->flags = 0;
  return 0;
}

int jfr_aload(void **head, char *fname, unsigned short esize)
{
  int err;

  err = jfr_iload(head, fname, esize);
  if (err == 0)
    err = jfr_p_init(*head);
  return err;
}

static int jfr_p_init(void *head)
{
  /* Initialisation at loading time */
  unsigned char *ff_jfs;
  struct jfr_head_desc *jfr_head;

  if (jfr_status == 0)
    jfr_init(0);
  jfr_head = (struct jfr_head_desc *) head;
  ff_jfs =  (unsigned char *) head;
  ff_jfs += JFR_HEAD_SIZE;
  jfr_head->domains = (struct jfr_domain_desc *) ff_jfs;
  ff_jfs += jfr_head->domain_c * JFR_DOMAIN_SIZE;
  jfr_head->adjectives = (struct jfr_adjectiv_desc *) ff_jfs;
  ff_jfs += jfr_head->adjectiv_c * JFR_ADJECTIV_SIZE;
  jfr_head->vars = (struct jfr_var_desc *) ff_jfs;
  ff_jfs += jfr_head->var_c * JFR_VAR_SIZE;
  jfr_head->arrays = (struct jfr_array_desc *) ff_jfs;
  ff_jfs += jfr_head->array_c * JFR_ARRAY_SIZE;
  jfr_head->limits = (struct jfr_limit_desc *) ff_jfs;
  ff_jfs += jfr_head->limit_c * JFR_LIMIT_SIZE;
  jfr_head->hedges = (struct jfr_hedge_desc *) ff_jfs;
  ff_jfs += jfr_head->hedge_c * JFR_HEDGE_SIZE;
  jfr_head->relations = (struct jfr_relation_desc *) ff_jfs;
  ff_jfs += jfr_head->relation_c * JFR_RELATION_SIZE;
  jfr_head->operators = (struct jfr_operator_desc *) ff_jfs;
  ff_jfs += jfr_head->operator_c * JFR_OPERATOR_SIZE;
  jfr_head->comments = (struct jfr_comment_desc *) ff_jfs;
  ff_jfs += jfr_head->comment_c * JFR_COMMENT_SIZE;
  jfr_head->functions = (struct jfr_function_desc *) ff_jfs;
  ff_jfs += jfr_head->function_c * JFR_FUNCTION_SIZE;
  jfr_head->func_args = (struct jfr_funcarg_desc *) ff_jfs;
  ff_jfs += jfr_head->funcarg_c * JFR_FUNCARG_SIZE;
  jfr_head->comment_block = (unsigned char *) ff_jfs;
  ff_jfs += jfr_head->com_block_c;
  jfr_head->fzvars = (struct jfr_fzvar_desc *) ff_jfs;
  ff_jfs += jfr_head->fzvar_c * JFR_FZVAR_SIZE;
  jfr_head->array_vals = (float *) ff_jfs;
  ff_jfs += jfr_head->arrayval_c * sizeof(float);
  jfr_head->function_code = (unsigned char *) ff_jfs;
  ff_jfs += jfr_head->funccode_c;
  jfr_head->program_code = ff_jfs;

  jfr_head->status = 1;
  return 0;
}

int  jfr_init(unsigned short stack_size)
{
  int m;
  unsigned short s;
  unsigned char *c;

  for (m = 0; m < 255 - JFR_OP_FIRST; m++)
   jfr_operators[m] = jfr_op_err;

  jfr_operators[JFR_OP_SPOP    - JFR_OP_FIRST]  = jfr_op_spop;
  jfr_operators[JFR_OP_ENDFUNC - JFR_OP_FIRST]  = jfr_op_endfunc;
  jfr_operators[JFR_OP_VDECREASE-JFR_OP_FIRST]  = jfr_op_vdecrease;
  jfr_operators[JFR_OP_VINCREASE-JFR_OP_FIRST]  = jfr_op_vincrease;
  jfr_operators[JFR_OP_VCASE   - JFR_OP_FIRST]  = jfr_op_case;
  jfr_operators[JFR_OP_WSET    - JFR_OP_FIRST]  = jfr_op_wset;
  jfr_operators[JFR_OP_VSWITCH - JFR_OP_FIRST]  = jfr_op_vswitch;
  jfr_operators[JFR_OP_SPUSH   - JFR_OP_FIRST]  = jfr_op_spush;
  jfr_operators[JFR_OP_IIF     - JFR_OP_FIRST]  = jfr_op_iif;
  jfr_operators[JFR_OP_APOP    - JFR_OP_FIRST]  = jfr_op_apop;
  jfr_operators[JFR_OP_COMMENT - JFR_OP_FIRST]  = jfr_op_comment;
  jfr_operators[JFR_OP_ICONST  - JFR_OP_FIRST]  = jfr_op_const;
  jfr_operators[JFR_OP_DEFAULT - JFR_OP_FIRST]  = jfr_op_default;
  jfr_operators[JFR_OP_VPOP    - JFR_OP_FIRST]  = jfr_op_vpop;
  jfr_operators[JFR_OP_FZVPOP  - JFR_OP_FIRST]  = jfr_op_fzvpop;
  jfr_operators[JFR_OP_ENDSWITCH-JFR_OP_FIRST]  = jfr_op_endswitch;
  jfr_operators[JFR_OP_TRUE    - JFR_OP_FIRST]  = jfr_op_true;
  jfr_operators[JFR_OP_FALSE   - JFR_OP_FIRST]  = jfr_op_false;
  jfr_operators[JFR_OP_ENDWHILE- JFR_OP_FIRST]  = jfr_op_endwhile;
  jfr_operators[JFR_OP_EXPR    - JFR_OP_FIRST]  = jfr_op_expr;
  jfr_operators[JFR_OP_WEXPR   - JFR_OP_FIRST]  = jfr_op_wexpr;
  jfr_operators[JFR_OP_AWEXPR  - JFR_OP_FIRST]  = jfr_op_wexpr;
  jfr_operators[JFR_OP_THENEXPR- JFR_OP_FIRST]  = jfr_op_then;
  jfr_operators[JFR_OP_VAR     - JFR_OP_FIRST]  = jfr_op_var;
  jfr_operators[JFR_OP_CONST   - JFR_OP_FIRST]  = jfr_op_const;
  jfr_operators[JFR_OP_DFUNC   - JFR_OP_FIRST]  = jfr_op_dfunc;
  jfr_operators[JFR_OP_UREL    - JFR_OP_FIRST]  = jfr_op_urel;
  jfr_operators[JFR_OP_SFUNC   - JFR_OP_FIRST]  = jfr_op_sfunc;
  jfr_operators[JFR_OP_ARRAY   - JFR_OP_FIRST]  = jfr_op_array;
  jfr_operators[JFR_OP_VFUNC   - JFR_OP_FIRST]  = jfr_op_vfunc;
  jfr_operators[JFR_OP_BETWEEN - JFR_OP_FIRST]  = jfr_op_between;
  jfr_operators[JFR_OP_CLEAR   - JFR_OP_FIRST]  = jfr_op_clear;
  jfr_operators[JFR_OP_SWITCH  - JFR_OP_FIRST]  = jfr_op_switch;
  jfr_operators[JFR_OP_CASE    - JFR_OP_FIRST]  = jfr_op_case;
  jfr_operators[JFR_OP_OP      - JFR_OP_FIRST]  = jfr_op_op;
  jfr_operators[JFR_OP_FRETURN - JFR_OP_FIRST]  = jfr_op_freturn;
  jfr_operators[JFR_OP_USERFUNC- JFR_OP_FIRST]  = jfr_op_userfunc;
  jfr_operators[JFR_OP_EOP     - JFR_OP_FIRST]  = jfr_op_eop;
  jfr_operators[JFR_OP_HEDGE   - JFR_OP_FIRST]  = jfr_op_hedge;
  jfr_operators[JFR_OP_EXTERN  - JFR_OP_FIRST]  = jfr_op_call;
  jfr_operators[JFR_OP_WHILE   - JFR_OP_FIRST]  = jfr_op_while;

  if (stack_size > 0)
    s = stack_size;
  else
    s = 1024;
  if ((c = (unsigned char *) malloc(s)) == NULL)
    return 6;
  jfrr.stack = c;
  jfrr.max_stack = c + s;
  jfr_status = 1;
  return 0;
}

void jfr_activate(void *head,
                  void (*call)(void),
                  void (*mcall)(int place),
                  void (*uvget)(int vno))
{
  int m;

  jfr_head = (struct jfr_head_desc *) head;
  jfrr.err_code = 0;
  jfrr.pc = jfrr.program_id = jfr_head->program_code;
  jfr_call = call;
  jfr_mcall = mcall;
  jfr_uvget = uvget;
  jfrr.weight = 1.0;
  jfrr.cond_value = jfrr.expr_value = jfrr.rm_weight = jfrr.index_value = 0.0;
  jfrr.pc = jfr_head->program_code;
  jfrr.program_id = jfrr.pc;
  jfrr.rule_no = 0;
  jfrr.ff_stack = jfrr.stack;
  jfrr.stack_id = jfrr.stack;
  jfrr.changed = 1;
  jfrr.function_no = -1;    /* main */


  for (m = 0; m < jfr_head->var_c; m++)
  { jfr_head->vars[m].status = 1;
    jfr_head->vars[m].conf = 0.0;
    jfr_head->vars[m].conf_sum = 0.0;
    jfr_head->vars[m].value = jfr_head->vars[m].default_val;
  }

  for (m = 0; m < jfr_head->fzvar_c; m++)
   jfr_head->fzvars[m].value = 0.0;

  for (m = 0; m < jfr_head->arrayval_c; m++)
   jfr_head->array_vals[m] = 0.0;

}

void jfr_arun(float *op, void *head, float *ip, float *confidence,
      void (*call)(void),
      void (*mcall)(int place),
      void (*uvget)(int vno))
{
  int m;

  jfr_activate(head, call, mcall, uvget);
  for (m = 0; m < jfr_head->ivar_c; m++)
  { if (confidence != NULL)
      jfr_ivput(jfr_head->f_ivar_no + m, ip[m], confidence[m]);
    else
      jfr_ivput(jfr_head->f_ivar_no + m, ip[m], 1.0);
  }

  jfr_program_handle();

  for (m = 0; m < jfr_head->ovar_c; m++)
    op[m] = jfr_ivget(jfr_head->f_ovar_no + m);
}


static int jfr_check(void *head)
{
  struct jfr_head_desc *jfr_head;

  if (head == NULL)
    return 4;
  jfr_head = (struct jfr_head_desc *) head;
  if (jfr_head->check[0] != 'j' ||
      jfr_head->check[1] != 'f' ||
      (jfr_head->check[2] != 'r' && jfr_head->check[2] != 'x'))
    return 4;
  if (jfr_head->version / 100 != 2)
    return 5;
  return 0;
}

int jfr_error(void)
{
  int e;

  e = jfrr.err_code;
  jfrr.err_code = 0;
  return e;
}

void jfr_statement_info(struct jfr_stat_desc *sprg)
{
  sprg->weight    = jfrr.weight;
  sprg->rm_weight    = jfrr.rm_weight;
  sprg->cond_value = jfrr.cond_value;
  sprg->expr_value = jfrr.expr_value;
  sprg->index_value = jfrr.index_value;
  sprg->changed    = jfrr.changed;
  sprg->rule_no    = jfrr.rule_no;
  sprg->pc         = jfrr.program_id;
  sprg->function_no = jfrr.function_no;
}

void jfr_clear(unsigned short var_no, int fzvclear)
{
  jfr_iclear(var_no, fzvclear);
}

void jfr_vput(unsigned short var_no, float value, float confidence)
{
  jfr_ivput(var_no, value, confidence);
}

void jfr_vupd(unsigned short var_no, float value, float confidence)
{
  float c;

  c = jfr_fzround(confidence);
  jfr_ivupd(var_no, value, c);
}


float jfr_vget(unsigned short var_no)
{
  float res;

  res = jfr_ivget(var_no);
  return res;
}

float jfr_cget(unsigned short var_no)
{
  float res;

  jfr_ivget(var_no);
  res = jfr_head->vars[var_no].conf;
  return res;
}

float jfr_aget(unsigned short ar_no, unsigned short id)
{
  float res;

  res = jfr_iaget(ar_no, id);
  return res;
}

void jfr_aput(unsigned short ar_no, unsigned short id, float val)
{
  jfr_iaset(ar_no, id, val);
}

void jfr_fzvput(unsigned short fzvar_no, float value)
{
  jfr_ifzvput(fzvar_no, value);
}

void jfr_fzvudp(unsigned short fzvar_no, float value)
{
  jfr_ifzvupd(fzvar_no, value);
}

float jfr_fzvget(unsigned short fzvar_no)
{
  float res;

  res = jfr_ifzvget(fzvar_no);
  return res;
}

float jfr_op_calc(unsigned short op_no, float v1, float v2)
{
  float res;

  res = jfr_iop_calc(op_no, v1, v2);
  return res;
}

float jfr_hedge_calc(unsigned short hno, float v)
{
  float res;

  res = jfr_ihedge_calc(hno, v);
  return res;
}

float jfr_rel_calc(unsigned short rel_no, float v1, float v2)
{
  float res;

  res = jfr_irel_calc(rel_no, v1, v2);
  return res;
}

