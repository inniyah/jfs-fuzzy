  /*************************************************************************/
  /*                                                                       */
  /* jffamlib.cpp Version  2.03  Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                                                       */
  /* JFS FAM-creater using a cellular automat.                             */
  /*                                                                       */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                    */
  /*    Lollandsvej 35 3.tv.                                               */
  /*    DK-2000 Frederiksberg                                              */
  /*    Denmark                                                            */
  /*                                                                       */
  /*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "jfr_lib.h"
#include "jfg_lib.h"
#include "jfp_lib.h"
#include "jft_lib.h"
#include "jffamlib.h"

#define JFE_WARNING 0
#define JFE_ERROR   1
#define JFE_FATAL   2

static FILE *da_opfile = NULL;
static FILE *sout = NULL;
static void *jffam_head = NULL;

char jffam_op_fname[256];
char jffam_ru_fname[256];

#define JFRD_VMAX 10

static float jffam_ivalues[JFRD_VMAX];
static float jffam_ovalues[JFRD_VMAX];
static float jffam_expected[JFRD_VMAX];
static float jffam_confidences[JFRD_VMAX];

static int jffam_silent    = 0;
static int jffam_data_mode = 0;  /* 0: input, expected. 1: expected, input */

static int jffam_fixed     = 1; /* 1: input-rules is fixed. */

static int jffam_change_program = 0;

static long jffam_data_size = 5000;
static long jffam_prog_size = 20000;

static char jffam_field_sep[256];
          /* ""   : brug space, tab etc som felt-seperator, */
			       /* andet: kun field_sep er feltsepator.           */

/* conflict resolution-method:                               */
  #define JFFAM_RC_SCORE  0
  #define JFFAM_RC_COUNT  1
static int jffam_res_confl = JFFAM_RC_SCORE;


static int jffam_c_contradictions;  /* number of contradictions. */

/* celular automat rule:                        */
  #define JFFAM_CA_AVG     0
  #define JFFAM_CA_MINMAX  1
  #define JFFAM_CA_DELTA   2
static int jffam_ca_rule;

/* then-part of generated rules:                                */
  #define JFFAM_TT_ADJECTIV   0
  #define JFFAM_TT_CENTER     1
static int jffam_then_type;

static int jffam_use_weight = 0;  /* 1: if-statements of the form 'ifw %<wgt>'    */
static float jffam_weight_val = 0.0;

static int jffam_max_steps = 100; /* max number of generations in cel-automat */

static signed long jffam_ipcount;

/*****************************************************************************/
/* variables to describe the jfs-varibles in the statement:                  */
/* 'extern jfrd input {<var>]  output <var>':                                */

struct jffam_multi_desc       /* describes a variable.                       */
  { signed short var_no;
			 signed short cur;         /* current fzvar. fzvar_no = f_fzvar_no + cur. */
			 signed short adjectiv_c;
			 unsigned short f_fzvar_no;
			 unsigned short f_adjectiv_no;
		};

/* describtion of the input-variables:                                       */
static struct jffam_multi_desc jffam_if_vars[JFRD_VMAX];
static int jffam_ff_if_vars = 0;  /* number of input-variables.              */

static struct jffam_multi_desc jffam_then_var; /* describes the output-var.  */

/****************************************************************************/

#define JFRD_WMAX 100
static const char *jffam_words[JFRD_WMAX];

#define JFRD_MAX_TEXT 512
static char jffam_text[JFRD_MAX_TEXT];

static struct jfg_sprog_desc     jffam_pdesc;
static struct jfg_statement_desc jffam_sdesc;

static char jffam_empty[] = " ";

#define JFRD_TREE_SIZE 100

static struct jfg_tree_desc jffam_tree[JFRD_TREE_SIZE];

/***************************************************************************/
/* Variables used in Wang-Mendel reduction:                                */

#define JFRD_CMP_EQ                 0
#define JFRD_CMP_NEQ                1
#define JFRD_CMP_INDEPENDENT        5
#define JFRD_CMP_DATA_CONTRADICTION 6


static unsigned char *jffam_darea = NULL;   /* memmory to the WM-rules.    */
static long          jffam_ff_darea = 0;    /* first-free WM-rule.         */
static long          jffam_drec_size;       /* size of a WM-rule.          */

static float         jffam_dscore;

	 /* A MW-rule contains:               score (float),               */
	 /*                                   count (short),               */
	 /*                                   conflict (byte),             */
	 /*                                   then_value (byte),           */
	 /*                                   input-var-1 value (byte),    */
	 /*                                   input-var-2 value (byte).    */
	 /*                                     .                          */

static unsigned short jffam_rule_count;
static char jffam_conflict;

/********************************************************************/

static unsigned char *jffam_program_id;

static int jffam_ins_rules;         /* number of rules inserted in program. */


/*************************************************************************/
/* Variables containig the cellular automat:                             */

static unsigned char *cel_source = NULL; /* source cellular automat.      */
static unsigned char *cel_dest = NULL;   /* destination cellular automat. */
			     /* cel[m] < 128: cell contains value for non-fixed rule, */
			     /* cel[m] = 128: cell is empty,                          */
			     /* cel[m] > 128: cell contains value for fixed rule,     */
			     /*               value = cel[m] - 129.                   */

static unsigned long cel_adr_id[JFRD_VMAX]; /* used to adressof cel-arrays */

static int cel_cur[JFRD_VMAX];      /* Current cell                        */
                                    /*<cel_cur>[0] = ip-var-0,             */
                                    /*         [1] = ip-var-1,...          */

static int cel_husk[JFRD_VMAX];     /* remeber center-cell.                */

/**************************************************************************/

static const char jffam_t_jfrd[]     = "jfrd";
static const char jffam_t_input[]    = "input";
static const char jffam_t_output[]   = "output";
static char jffam_da_fname[256];

/**************************************************************************/
/* Error handling                                                         */
/**************************************************************************/

struct jffam_err_desc {
	int eno;
	const char *text;
};

struct jffam_err_desc jffam_err_texts[] =
{{  0, " "},
	{  1, "Cannot open file:"},
	{  2, "Error reading from file:"},
	{  3, "Error writing to file:"},
	{  4, "Not an jfr-file:"},
	{  5, "Wrong version:"},
	{  6, "Cannot allocate memory to:"},
	{  9, "Illegal number:"},
	{ 10, "Value out of domain-range:"},
	{ 11, "Unexpected EOF."},
	{ 13, "Undefined adjectiv:"},
	{401, "jfp cannot insert this type of statement"},
	{402, "Statement to large."},
	{403, "Not enogh free memory to insert statement"},
	{501, "Illegal create-type:"},
	{504, "Syntax error in jfrd-statement:"},
	{505, "Too many variables in statement (max 50)."},
	{506, "Undefined variable:"},
	{507, "Illegal number of adjectives to variable ([1..126]:"},
	{508, "Out of memory in rule-array"},
	{514, "Tree not large enogh to hold statement"},
	{517, "Cannot create rules from data without a data-file"},
	{519, "Too many words in statement (max 255)."},
	{520, "No 'call jfrd'-statement in program"},
	{9999, "Unknown error!"},
};

static void jffam_close(void);
static int jf_error(int eno, const char *name, int mode);
static int jffam_fl_ip_get(struct jft_data_record *dd);
static int jffam_ip_get();
static int jffam_var_no(const char *text);
static int jffam_get_command(int argc);

static unsigned char *jffam_ins_s_rule(unsigned char *progid);
static unsigned char *jffam_ins_fam_rule(unsigned char *progid, int then_val);

static int jffam_oom(void);
static void jffam_s_get(int dno);
static void jffam_s_put(int dno);
static void jffam_s_copy(int dno, int sno);
static void jffam_s_update(int dno);
static int jffam_ch_cmp(unsigned char ch1, unsigned char ch2);
static int jffam_s_cmp(int dno1, int dno2);
static int jffam_rcheck(unsigned long *cruleno, unsigned long adr);
static void jffam_resolve_contradiction(unsigned long rno1,	unsigned long rno2);
static int closest_adjectiv(int ifvar_no);
static void jffam_call(void);
static void jffam_no_call(void);
static void jffam_ip2rule(unsigned long rno);
static void jffam_red_contra(void);

static int jffam_cel_get(unsigned char *cel);
static void jffam_cel_put(unsigned char *cel, int val);
static int jffam_first_cel(void);
static int jffam_next_cel(void);
static int jffam_first_nabo(int factor);
static int jffam_next_nabo(int factor);
static int jffam_cel_create(void);

static void jffam_sp_write(int ant);
static int jffam_maxlen(int var_no);
static void jffam_li_write(int margen, int maxlen, int dobbel);
static void jffam_skema_write(void);

static int jffam_cel_step(void);
static void jffam_write(void);
static int jffam_create(void);
static int jffam_data(unsigned char *program_id);


static void jffam_close(void)
{
  jfr_close(jffam_head);
  jfr_free();
  jft_close();
  jfg_free();
  jfp_free();
}

static int jf_error(int eno, const char *name, int mode)
{
  int m, v, e;

  e = 0;
  for (v = 0; e == 0; v++)
  { if (jffam_err_texts[v].eno == eno
       	|| jffam_err_texts[v].eno == 9999)
      e = v;
  }
  if (mode == JFE_WARNING)
  { fprintf(sout, "WARNING %d: %s %s\n", eno, jffam_err_texts[e].text, name);
    m = 0;
  }
  else
  { if (eno != 0)
      fprintf(sout, "*** error %d: %s %s\n", eno, jffam_err_texts[e].text, name);
    if (mode == JFE_FATAL)
    { if (eno != 0)
	       fprintf(sout, "\n*** PROGRAM ABORTED! ***\n");
        jffam_close();
    }
    m = -1;
  }
  return m;
}

/************************************************************************/
/* Funktioner til indlaesning fra fil                                   */
/************************************************************************/

static int jffam_fl_ip_get(struct jft_data_record *dd)
{
  int m;
  char txt[540];

  m = jft_getdata(dd);
  if (m != 0)
  { if (m != 11) /* eof */
    { sprintf(txt, " %s in file: %s line %d.",
              jft_error_desc.carg, jffam_da_fname, jft_error_desc.line_no);
      jf_error(jft_error_desc.error_no, txt, JFE_ERROR);
      m = 0;
    }
  }
  return m;
}

static int jffam_ip_get()
{
  int slut, m;
  struct jft_data_record dd;

  slut = 0;
  for (m = 0; slut == 0 && m < jft_dset_desc.record_size; m++)
  { slut = jffam_fl_ip_get(&dd);
    if (slut == 1 && m != 0)
       return jf_error(11, jffam_empty, JFE_ERROR);
    if (slut == 0)
    { if (dd.vtype == JFT_VT_EXPECTED)
        jffam_expected[dd.vno] = dd.farg;
      else
      if (dd.vtype == JFT_VT_INPUT)
      { jffam_ivalues[dd.vno] = dd.farg;
        if (dd.mode == JFT_DM_MISSING)
          jffam_confidences[dd.vno] = 0.0;
        else
          jffam_confidences[dd.vno] = 1.0;
      }
    }
  }
  return slut;
}

/**********************************************************************/
/* funktioner til afkodning af jfrd-statement                         */
/**********************************************************************/

static int jffam_var_no(const char *text)
{
  int m, res;
  struct jfg_var_desc vdesc;

  res = -1;
  for (m = 0; res == -1 && m < jffam_pdesc.var_c; m++)
  { jfg_var(&vdesc, jffam_head, m);
    if (strcmp(vdesc.name, text) == 0)
      res = m;
  }
  return res;
}

static int jffam_get_command(int argc)
{
  int m, vno;
  struct jfg_var_desc vdesc;

  int state;   /*  0: start                       */
               /*  1: jfrd                        */
               /*  2: input                       */
               /*  3: output                      */
               /*  4: slut                        */

  jffam_ff_if_vars = 0;

  m = -1;
  state = 0;

  while (state != -1)
  { m++;
    if (m >= argc)
    { if (state != 4)
       	return jf_error(504, jffam_empty, JFE_ERROR);
    }
    switch (state)
    { case 0:
        if (strcmp(jffam_words[m], jffam_t_jfrd) == 0)
          state = 1;
        else
          return jf_error(502, jffam_words[m], JFE_ERROR);
        break;
      case 1:  /* jfrd */
        if (strcmp(jffam_words[m], jffam_t_input) == 0)
          state = 2;
        else
          return jf_error(504, jffam_words[m], JFE_ERROR);
        break;
      case 2:  /* input */
         if (strcmp(jffam_words[m], jffam_t_output) == 0)
           state = 3;
         else
         if (jffam_words[m][0] == '-')
           m++;
         else
         { vno = jffam_var_no(jffam_words[m]);
           if (vno == -1)
             return jf_error(506, jffam_words[m], JFE_ERROR);
           if (jffam_ff_if_vars >= JFRD_VMAX)
             return jf_error(505, jffam_words[m], JFE_ERROR);

           jffam_if_vars[jffam_ff_if_vars].var_no = vno;
           jfg_var(&vdesc, jffam_head, vno);
           jffam_if_vars[jffam_ff_if_vars].adjectiv_c = vdesc.fzvar_c;
           jffam_if_vars[jffam_ff_if_vars].f_fzvar_no = vdesc.f_fzvar_no;
           jffam_if_vars[jffam_ff_if_vars].f_adjectiv_no = vdesc.f_adjectiv_no;
           if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 254)
             return jf_error(507, jffam_words[m], JFE_ERROR);
           jffam_ff_if_vars++;
         }
         break;
      case 3:
         if (jffam_words[m][0] == '-')
           m++;
         else
         { vno = jffam_var_no(jffam_words[m]);
           if (vno == -1)
             return jf_error(506, jffam_words[m], JFE_ERROR);
           jffam_then_var.var_no = vno;
           jfg_var(&vdesc, jffam_head, vno);
           jffam_then_var.adjectiv_c = vdesc.fzvar_c;
           jffam_then_var.f_fzvar_no = vdesc.f_fzvar_no;
           jffam_then_var.f_adjectiv_no = vdesc.f_adjectiv_no;
           if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 126)
             return jf_error(507, jffam_words[m], JFE_ERROR);
           state = 4;
         }
         break;
      case 4:
        state = -1; /* slut */
        if (m < argc)
          return jf_error(504, jffam_words[m], JFE_ERROR);
        break;
    }
  }
  if (jffam_ff_if_vars == 0)
  { for (jffam_ff_if_vars = 0; jffam_ff_if_vars < jffam_pdesc.ivar_c;
         jffam_ff_if_vars++)
    { vno = jffam_pdesc.f_ivar_no + jffam_ff_if_vars;
      if (jffam_ff_if_vars >= JFRD_VMAX)
        return jf_error(505, jffam_words[m], JFE_ERROR);
      jffam_if_vars[jffam_ff_if_vars].var_no = vno;
      jfg_var(&vdesc, jffam_head, vno);
      jffam_if_vars[jffam_ff_if_vars].adjectiv_c = vdesc.fzvar_c;
      jffam_if_vars[jffam_ff_if_vars].f_fzvar_no = vdesc.f_fzvar_no;
      jffam_if_vars[jffam_ff_if_vars].f_adjectiv_no = vdesc.f_adjectiv_no;
      if (vdesc.fzvar_c == 0 || vdesc.fzvar_c > 254)
        return jf_error(507, jffam_words[m], JFE_ERROR);
    }
  }
  return 0;
}

/***********************************************************************/
/* Indsaet statement                                                   */
/***********************************************************************/

static unsigned char *jffam_ins_s_rule(unsigned char *progid)
/* temporary insert current a rule in program  */
/* (used to find the rule witch best matches correct output). */
{
  int res, ff_tree, v, first;
  unsigned char *pc;

  pc = progid;
  ff_tree = 0;
  first = 1;
  for (v = 0; v < jffam_ff_if_vars; v++)
  { if (jffam_if_vars[v].cur != 255)
    { if (ff_tree + 2 >= JFRD_TREE_SIZE)
      { jf_error(514, jffam_empty, JFE_ERROR);
       	return pc;
      }
      jffam_tree[ff_tree].type = JFG_TT_FZVAR;
      jffam_tree[ff_tree].sarg_1 = jffam_if_vars[v].cur
				                              + jffam_if_vars[v].f_fzvar_no;
      ff_tree++;
      if (first == 0)
      { jffam_tree[ff_tree].type = JFG_TT_OP;
       	jffam_tree[ff_tree].op = JFS_ONO_AND;
        jffam_tree[ff_tree].sarg_1 = ff_tree - 2;
        jffam_tree[ff_tree].sarg_2 = ff_tree - 1;
        ff_tree++;
      }
      first = 0;
    }
  }
  if (first == 1)                           /* No if-arguments!  */
  { jffam_tree[ff_tree].type = JFG_TT_TRUE;
    ff_tree++;
  }
  jffam_sdesc.type = JFG_ST_IF;
  jffam_sdesc.sec_type = JFG_SST_FZVAR;
  jffam_sdesc.flags = 0;
  if (jffam_use_weight == 1)
  { jffam_sdesc.flags = 3;
    jffam_sdesc.farg = jffam_weight_val;
  }
  jffam_sdesc.sarg_1 = jffam_then_var.f_fzvar_no + jffam_then_var.cur;

  res = jfp_i_tree(jffam_head, &pc, &jffam_sdesc,
              		   jffam_tree, ff_tree - 1, 0, 0,
                   NULL, 0);
  if (res != 0)
    jf_error(res, jffam_empty, JFE_ERROR);
  return pc;
}


static unsigned char *jffam_ins_fam_rule(unsigned char *progid, int then_val)
/* insert a rule from current cell in cellular automat. */
{
  int res, ff_tree, v, first;
  unsigned char *pc;
  unsigned short cond_node, expr_node;
  struct jfg_adjectiv_desc adesc;

  pc = progid;
  ff_tree = 0;
  first = 1;
  for (v = 0; v < jffam_ff_if_vars; v++)
  { if (ff_tree + 5 >= JFRD_TREE_SIZE)
    { jf_error(514, jffam_empty, JFE_ERROR);
      return pc;
    }
    jffam_tree[ff_tree].type = JFG_TT_FZVAR;
    jffam_tree[ff_tree].sarg_1 = cel_cur[v]
				  + jffam_if_vars[v].f_fzvar_no;
    ff_tree++;
    if (first == 0)
    { jffam_tree[ff_tree].type = JFG_TT_OP;
      jffam_tree[ff_tree].op = JFS_ONO_AND;
      jffam_tree[ff_tree].sarg_1 = ff_tree - 2;
      jffam_tree[ff_tree].sarg_2 = ff_tree - 1;
      ff_tree++;
    }
    first = 0;
  }
  if (first == 1)                           /* Ingen if-arguments!  */
  { jffam_tree[ff_tree].type = JFG_TT_TRUE;
    ff_tree++;
  }
  cond_node = ff_tree - 1;
  expr_node = 0;

  jffam_sdesc.type = JFG_ST_IF;
  jffam_sdesc.flags = 0;
  if (jffam_then_type == JFFAM_TT_ADJECTIV)
  { jffam_sdesc.sec_type = JFG_SST_FZVAR;
    jffam_sdesc.sarg_1 = jffam_then_var.f_fzvar_no + then_val;
  }
  else
  if (jffam_then_type == JFFAM_TT_CENTER)
  { jffam_sdesc.sec_type = JFG_SST_VAR;
    jffam_sdesc.sarg_1 = jffam_then_var.var_no;
    jfg_adjectiv(&adesc, jffam_head, jffam_then_var.f_adjectiv_no + then_val);
    jffam_tree[ff_tree].type = JFG_TT_CONST;
    jffam_tree[ff_tree].op = 1;
    jffam_tree[ff_tree].farg = adesc.center;
    expr_node = ff_tree;
  }
  if (jffam_use_weight == 1)
  { jffam_sdesc.flags += 3;
    jffam_sdesc.farg = jffam_weight_val;
  }

  res = jfp_i_tree(jffam_head, &pc, &jffam_sdesc,
              		   jffam_tree, cond_node, 0, expr_node,
                   NULL, 0);
  if (res != 0)
    jf_error(res, jffam_empty, JFE_ERROR);
  return pc;
}


/*************************************************************************/
/* Wang-Mendel-discover                                                  */
/*************************************************************************/

static int jffam_oom(void)  /* return 1 if out-of-memory darea */
{
  int res;

  if ((jffam_ff_darea + 5) * jffam_drec_size > jffam_data_size)
    res = 1;
  else
    res = 0;
  return res;
}

static void jffam_s_get(int dno) /* copies s-rule to jfrd-if_vars etc   */
{
  unsigned long adr;
  int m;

  adr = ((unsigned long) dno) * jffam_drec_size;

  memcpy((char *) &jffam_dscore, (char *) &(jffam_darea[adr]), sizeof(float));
  adr += sizeof(float);


  memcpy((char *) &jffam_rule_count,
	         (char *) &(jffam_darea[adr]), sizeof(short));
  adr += sizeof(short);

  jffam_conflict = jffam_darea[adr];
  adr++;

  jffam_then_var.cur = jffam_darea[adr];
  adr++;
  for (m = 0; m < jffam_ff_if_vars; m++)
  { jffam_if_vars[m].cur = jffam_darea[adr];
    adr++;
  }
}

static void jffam_s_put(int dno)   /* copies if_vars-rule til s_rule <dno>. */
{
  unsigned long adr;
  int m;

  adr = ((unsigned long) dno) * jffam_drec_size;

  memcpy((char *) &(jffam_darea[adr]),
	         (char *) &jffam_dscore, sizeof(float));
  adr += sizeof(float);

  memcpy((char *) &(jffam_darea[adr]),
	         (char *) &jffam_rule_count, sizeof(short));
  adr += sizeof(short);

  jffam_darea[adr] = jffam_conflict;
  adr++;
  jffam_darea[adr] = jffam_then_var.cur;
  adr++;
  for (m = 0; m < jffam_ff_if_vars; m++)
  { jffam_darea[adr] = jffam_if_vars[m].cur;
    adr++;
  }
}

static void jffam_s_copy(int dno, int sno)
{
  jffam_s_get(sno);
  jffam_s_put(dno);
}

static void jffam_s_update(int dno)
{
  unsigned long adr;
  float f;
  short s;

  /* Updates score, count fro rule dno from current var-rule. */

  adr = dno * jffam_drec_size;
  memcpy((char *) &f,
	         (char *) &(jffam_darea[adr]), sizeof(float));
  if (jffam_dscore < f)
    memcpy((char *) &(jffam_darea[adr]),
	           (char *) &jffam_dscore, sizeof(float));
  adr += sizeof(float);
  memcpy((char *) &s,
	         (char *) &(jffam_darea[adr]), sizeof(short));
  if (jffam_res_confl == JFFAM_RC_COUNT)
    s += jffam_rule_count;
  memcpy((char *) &(jffam_darea[adr]),
	         (char *) &s, sizeof(short));
}

static int jffam_ch_cmp(unsigned char ch1, unsigned char ch2)
{
  int res;

  res = JFRD_CMP_NEQ;
  if (ch1 == ch2)
    res = JFRD_CMP_EQ;
  return res;
}

static int jffam_s_cmp(int dno1, int dno2)
{
  unsigned long adr1, adr2;
  int m, res;
  unsigned char then1, then2;

  adr1 = (((unsigned long) dno1) * jffam_drec_size) + sizeof(float);
  adr1 += (unsigned long) (sizeof(short) + 1);
  adr2 = (((unsigned long) dno2) * jffam_drec_size) + sizeof(float);
  adr2 += (unsigned long) (sizeof(short) + 1);
  then1 = jffam_darea[adr1];
  then2 = jffam_darea[adr2];
  adr1++;
  adr2++;
  res = JFRD_CMP_EQ;
  for (m = 0; res != JFRD_CMP_NEQ && m < jffam_ff_if_vars; m++)
  { res = jffam_ch_cmp(jffam_darea[adr1], jffam_darea[adr2]);
    adr1++;
    adr2++;
  }

  if (then1 == then2)
  { if (res == JFRD_CMP_NEQ)
      res = JFRD_CMP_INDEPENDENT;
  }
  else
  { if (res == JFRD_CMP_NEQ)
      res = JFRD_CMP_INDEPENDENT;
    else
      res = JFRD_CMP_DATA_CONTRADICTION;
  }
  return res;
}

static int jffam_rcheck(unsigned long *cruleno, unsigned long adr)
{
  int res, r;
  unsigned long m;

  res = JFRD_CMP_INDEPENDENT;
  for (m = 0; m < jffam_ff_darea; m++)
  { if (m != adr)
    { r = jffam_s_cmp(adr, m);
      if (r == JFRD_CMP_EQ)
      { res = r;
        *cruleno = m;
      }
      else
      if (r == JFRD_CMP_DATA_CONTRADICTION && res != JFRD_CMP_EQ)
      { res = r;
       	*cruleno = m;
      }
    }
  }
  return res;
}

static void jffam_resolve_contradiction(unsigned long rno1,	unsigned long rno2)
{
  unsigned long adr1, adr2;
  float score_1, score_2;
  short count_1, count_2;

  adr1 = ((unsigned long) rno1) * jffam_drec_size;
  memcpy((char *) &score_1,
	        (char *) &(jffam_darea[adr1]), sizeof(float));
  adr1 += sizeof(float);
  memcpy((char *) &count_1,
	        (char *) &(jffam_darea[adr1]), sizeof(short));
  adr1 += sizeof(short);

  adr2 = ((unsigned long) rno2) * jffam_drec_size;
  memcpy((char *) &score_2,
	        (char *) &(jffam_darea[adr2]), sizeof(float));
  adr2 += sizeof(float);
  memcpy((char *) &count_2,
	        (char *) &(jffam_darea[adr2]), sizeof(short));
  adr2 += sizeof(short);

  if (jffam_res_confl == JFFAM_RC_SCORE)
  { if (score_1 <= score_2)
    { jffam_s_copy(rno2, jffam_ff_darea - 1);
      jffam_ff_darea--;
    }
    else
    { jffam_s_copy(rno1, rno2);
      jffam_ff_darea--;
    }
  }
  else
  { jffam_darea[adr1] = 1;
    jffam_darea[adr2] = 1;
  }
  jffam_c_contradictions++;
}

static int closest_adjectiv(int ifvar_no)
{
  int m, best_cur = 0;
  float best_value, value;
  struct jfg_var_desc vdesc;

  best_value = 0;
  jfg_var(&vdesc, jffam_head, jffam_if_vars[ifvar_no].var_no);
  for (m = 0; m < jffam_if_vars[ifvar_no].adjectiv_c; m++)
  { value = jfr_fzvget(jffam_if_vars[ifvar_no].f_fzvar_no + m);
    if (m == 0 || value > best_value)
    { best_value = value;
      best_cur = m;
    }
  }
  jffam_dscore += (1.0 - best_value);
  return best_cur;
}


static void jffam_call(void)
{
  int m;

  jffam_dscore = 0.0;
  for (m = 0; m < jffam_ff_if_vars; m++)
  { jffam_if_vars[m].cur = closest_adjectiv(m);
  }
}

static void jffam_no_call(void)
{
  return ;
}

static void jffam_ip2rule(unsigned long rno)
{
  float dist, best_dist;
  int m, v, best_no = 0;

  best_dist = 0.0;
  /* run the program with jffam_ivalues. The rules if-part              */
  /* is set by jffam_call().                                            */
  jfr_arun(jffam_ovalues, jffam_head, jffam_ivalues, jffam_confidences,
	          jffam_call, NULL, NULL);

  /* for each adjecitvi to the output insert the rule with found if-part */
  /* and the curent adjectiv. Find adjectiv which gives output closest   */
  /* the expected value.                                                 */
  for (m = 0; m < jffam_then_var.adjectiv_c; m++)
  { jffam_then_var.cur = m;
    jffam_ins_s_rule(jffam_program_id);
    jfr_arun(jffam_ovalues, jffam_head, jffam_ivalues, jffam_confidences,
	            jffam_no_call, NULL, NULL);
    dist = 0.0;
    for (v = 0; v < jffam_pdesc.ovar_c; v++)
      dist += fabs(jffam_ovalues[v] - jffam_expected[v]);
    if (m == 0 || dist < best_dist)
    { best_no = m;
      best_dist = dist;
    }
    jfp_d_statement(jffam_head, jffam_program_id);
  }
  jffam_then_var.cur = best_no;
  jffam_dscore += best_dist;
  jffam_conflict = 0;
  if (jffam_res_confl == JFFAM_RC_SCORE)
    jffam_rule_count = jffam_ipcount;
  else
    jffam_rule_count = 1;
  jffam_s_put(rno);
}

static void jffam_red_contra(void)
{
  unsigned long n1, n2;
  unsigned short n1_count;
  int rm_rules;
  int ens;

  rm_rules = 0;
  if (jffam_res_confl == JFFAM_RC_COUNT)
  { for (n1 = 0; n1 < jffam_ff_darea; n1++)
    { jffam_s_get(n1);
      n1_count = jffam_rule_count;
      if (jffam_conflict == 1)
      { for (n2 = n1 + 1; n2 < jffam_ff_darea;  )
        { jffam_s_get(n2);
          if (jffam_conflict == 1)
          { ens = jffam_s_cmp(n1, n2);
            if (ens == JFRD_CMP_DATA_CONTRADICTION)
            { if (jffam_rule_count > n1_count)
              { n1_count = jffam_rule_count;
                jffam_s_copy(n1, n2);
              }
              jffam_s_copy(n2, jffam_ff_darea - 1);
              jffam_ff_darea--;
              rm_rules++;
            }
            else
              n2++;
          }
          else
            n2++;
        }
      }
    }
  }
  if (jffam_silent == 0)
    fprintf(sout, "\n    Contradictions resolved. %d rules removed.\n",
	          rm_rules);
}


/************************************************************************/
/* Cellulaer Automat                                                    */
/************************************************************************/

static int jffam_cel_get(unsigned char *cel)
{
  int m, res;
  long adr;

  adr = 0;
  for (m = 0; m < jffam_ff_if_vars; m++)
    adr += ((long) cel_cur[m]) * ((long) cel_adr_id[m]);
  res = (int) cel[adr];
  return res;
}

static void jffam_cel_put(unsigned char *cel, int val)
{
  int m;
  long adr;

  adr = 0;
  for (m = 0; m < jffam_ff_if_vars; m++)
    adr += ((long) cel_cur[m]) * ((long) cel_adr_id[m]);
  cel[adr] = (signed char) val;
}

static int jffam_first_cel(void)
{
  int m;

  for (m = 0; m < jffam_ff_if_vars; m++)
    cel_cur[m] = 0;
  return 0;
}

static int jffam_next_cel(void)
{
  int m;

  for (m = jffam_ff_if_vars - 1; m >= 0; m--)
  { cel_cur[m]++;
    if (cel_cur[m] >= jffam_if_vars[m].adjectiv_c)
      cel_cur[m] = 0;
    else
      return 0; /* found */
  }
  return 1;  /* end */
}

static int jffam_first_nabo(int factor)
{
  int m;

  for (m = 0; m < jffam_ff_if_vars; m++)
  { cel_husk[m] = cel_cur[m];
    cel_cur[m] = cel_husk[m] - factor;
    if (cel_cur[m] < 0)
      cel_cur[m] = 0;
  }
  return 0;
}

static int jffam_next_nabo(int factor)
{
  int m;

  for (m = jffam_ff_if_vars - 1; m >= 0; m--)
  { cel_cur[m] += factor;
    if (cel_cur[m] > cel_husk[m] + factor
       	|| cel_cur[m] >= jffam_if_vars[m].adjectiv_c)
    { cel_cur[m] = cel_husk[m] - factor;
      if (cel_cur[m] < 0)
        cel_cur[m] = 0;
    }
    else
      return 0;
  }
  return 1; /* slut */
}

static int jffam_cel_create(void)
{
  unsigned short size;
  int m, slut;
  unsigned long n1;

  size = 1;
  for (m = 0; m < jffam_ff_if_vars; m++)
  { if (m == 0)
      cel_adr_id[m] = 1;
    else
      cel_adr_id[m] = cel_adr_id[m - 1] * jffam_if_vars[m - 1].adjectiv_c;
    size *= jffam_if_vars[m].adjectiv_c;
  }
  cel_source = (unsigned char *) malloc(size);
  if (cel_source == NULL)
    return jf_error(6, jffam_empty, JFE_FATAL);
  cel_dest = (unsigned char *) malloc(size);
  if (cel_dest == NULL)
    return jf_error(6, jffam_empty, JFE_FATAL);

  /* Initiliser cel */
  slut = jffam_first_cel();
  while (slut == 0)
  { jffam_cel_put(cel_source, 128);
    slut = jffam_next_cel();
  }

  for (n1 = 0; n1 < jffam_ff_darea; n1++)
  { jffam_s_get(n1);
    for (m = 0; m < jffam_ff_if_vars; m++)
      cel_cur[m] = jffam_if_vars[m].cur;
    if (jffam_fixed == 1)
      jffam_cel_put(cel_source, 129 + jffam_then_var.cur);
    else
      jffam_cel_put(cel_source, jffam_then_var.cur);
  }
  return 0;
}

/* functions to print of FAM-comment */

static void jffam_sp_write(int ant)
{
  int m;

  for (m = 0; m < ant; m++)
    fprintf(da_opfile, " ");
}

static int jffam_maxlen(int var_no)
{
  int le, m;
  struct jfg_adjectiv_desc adesc;
  struct jfg_var_desc vdesc;

  le = 0;
  jfg_var(&vdesc, jffam_head, var_no);
  for (m = 0; m < vdesc.fzvar_c; m++)
  { jfg_adjectiv(&adesc, jffam_head, vdesc.f_adjectiv_no + m);
    if (strlen(adesc.name) > le)
      le = strlen(adesc.name);
  }
  return le;
}

static void jffam_li_write(int margen, int maxlen, int dobbel)
{
  int m, a;

  fprintf(da_opfile, "#");
  jffam_sp_write(margen);
  for (a = 0; a < jffam_if_vars[1].adjectiv_c + 1; a++)
  { for (m = 0; m < maxlen + 2; m++)
    { if (dobbel == 1)
       	fprintf(da_opfile, "=");
      else
       	fprintf(da_opfile, "-");
    }
    fprintf(da_opfile, "|");
    if (a == 0)
      fprintf(da_opfile, "|");
  }
  fprintf(da_opfile, "\n");
}

static void jffam_skema_write(void)
{
  int m, v, maxlen, ml, y, margen, nskrevet;
  struct jfg_var_desc vdesc;
  struct jfg_adjectiv_desc adesc;

  nskrevet = 0;
  maxlen = 0;
  for (m = 0; m < 2; m++)
  { ml = jffam_maxlen(jffam_if_vars[m].var_no);
    if (ml > maxlen)
      maxlen = ml;
  }
  ml = jffam_maxlen(jffam_then_var.var_no);
  if (ml > maxlen)
    maxlen = ml;
  jfg_var(&vdesc, jffam_head, jffam_if_vars[0].var_no);
  margen = 2 + strlen(vdesc.name);
  jfg_var(&vdesc, jffam_head, jffam_if_vars[1].var_no);

  /* skriv v2's navn */
  fprintf(da_opfile, "#\n#");
  jffam_sp_write(margen);
  m = ((jffam_if_vars[1].adjectiv_c + 1) * (maxlen + 3)) / 2;
  jffam_sp_write(m);
  fprintf(da_opfile, "%s\n", vdesc.name);

  /* skriv v2's adjectiver */
  fprintf(da_opfile, "#");
  jffam_sp_write(margen + maxlen + 2);
  fprintf(da_opfile, "||");
  for (m = 0; m < jffam_if_vars[1].adjectiv_c; m++)
  { jfg_adjectiv(&adesc, jffam_head, jffam_if_vars[1].f_adjectiv_no + m);
    fprintf(da_opfile, " %s ", adesc.name);
    jffam_sp_write(maxlen - strlen(adesc.name));
    fprintf(da_opfile, "|");
  }
  fprintf(da_opfile, "\n");
  jffam_li_write(margen, maxlen, 1);

  for (m = 0; m < jffam_if_vars[0].adjectiv_c; m++)
  { fprintf(da_opfile, "#");
    if (nskrevet == 0 && m >= jffam_if_vars[0].adjectiv_c / 2)
    { nskrevet = 1;
      jfg_var(&vdesc, jffam_head, jffam_if_vars[0].var_no);
      fprintf(da_opfile, " %s ", vdesc.name);
    }
    else
      jffam_sp_write(margen);
    jfg_adjectiv(&adesc, jffam_head, jffam_if_vars[0].f_adjectiv_no + m);
    fprintf(da_opfile, " %s ", adesc.name);
    jffam_sp_write(maxlen - strlen(adesc.name));
    fprintf(da_opfile, "||");

    for (y = 0; y < jffam_if_vars[1].adjectiv_c; y++)
    { cel_cur[0] = m;
      cel_cur[1] = y;
      v = jffam_cel_get(cel_dest);
      if (v != 128)
      { if (v > 128)
          v -= 129;
        jfg_adjectiv(&adesc, jffam_head, jffam_then_var.f_adjectiv_no + v);
        fprintf(da_opfile, " %s ", adesc.name);
        jffam_sp_write(maxlen - strlen(adesc.name));
      }
      else
       	jffam_sp_write(maxlen);
      fprintf(da_opfile, "|");
    }
    fprintf(da_opfile, "\n");
    jffam_li_write(margen, maxlen, 0);
  }
  fprintf(da_opfile, "#\n");
}

/**********************************************************************/

static int jffam_cel_step(void)
{
  int m, changed, slut, val = 0, dval = 0, glval, ant, sum, slut2;
  int vmin, vmax, t, n, ant2 = 0, sum2 = 0;

  changed = 0;
  slut = jffam_first_cel();
  while (slut == 0)
  { glval = jffam_cel_get(cel_source);
    if (glval > 128)
      jffam_cel_put(cel_dest, glval);
    else
    { sum = 0; ant = 0; vmin = glval; vmax = glval;
      jffam_cel_put(cel_source, 128); /* change temporary to empty */
      slut2 = jffam_first_nabo(1);
      while (slut2 == 0)
      { val = jffam_cel_get(cel_source);
        if (val != 128)
        { if (val > 128)
            val = val - 129;
          sum += val;
          if (ant == 0 || val < vmin)
            vmin = val;
          if (ant == 0 || val > vmax)
            vmax = val;
          ant++;
        }
        slut2 = jffam_next_nabo(1);
      }
      for (m = 0; m < jffam_ff_if_vars; m++)
       	cel_cur[m] = cel_husk[m];

      if (jffam_ca_rule == JFFAM_CA_DELTA)
      { ant2 = 0; sum2 = 0;
        slut2 = jffam_first_nabo(2);
        while (slut2 == 0)
        { val = jffam_cel_get(cel_source);
          if (val != 128)
          { if (val > 128)
            val = val - 129;
            sum2 += val;
            ant2++;
          }
          slut2 = jffam_next_nabo(2);
        }
        for (m = 0; m < jffam_ff_if_vars; m++)
          cel_cur[m] = cel_husk[m];
      }
      if (ant == 0)
       	dval = glval;
      else
      { if (jffam_ca_rule == JFFAM_CA_AVG)
        { dval = sum / ant;
          if ((sum % ant) * 2 >= ant)
            dval++;
        }
        else
        if (jffam_ca_rule == JFFAM_CA_MINMAX)
        { dval = (vmin + vmax) / 2;
          if ((vmin + vmax) % 2 == 1)
            dval++;
        }
        else
        if (jffam_ca_rule == JFFAM_CA_DELTA)
        { /* formlen er udledt fra (2*sum-sum2)/ant              */
          /* for at konpensere for punkter i kanten (ant!=ant2): */
          /* (2*sum-(sum2+(sum1*(ant-ant2)/ant))/ant             */
          t = (ant * sum - ant * sum2 + ant2 * sum);
          n = ant * ant;
          dval = t / n;
          if ((t % n) * 2 > n)
            dval++;
        }
        if (dval < 0)
          dval = 0;
        if (dval >= jffam_then_var.adjectiv_c)
          dval = jffam_then_var.adjectiv_c - 1;
      }
      jffam_cel_put(cel_dest, dval);
      if (glval != dval)
       	changed = 1;
      jffam_cel_put(cel_source, glval); /* ret tilbage */
    }
    slut = jffam_next_cel();
  }
  return changed;
}

static void jffam_write(void)
{
  int ant, slut, m, val;
  unsigned char *pc;
  struct jfg_adjectiv_desc adesc;

  ant = 0;
  if (da_opfile != NULL)
  { if (jffam_ff_if_vars == 2)
      jffam_skema_write();
    slut = jffam_first_cel();
    while (slut == 0)
    { val = jffam_cel_get(cel_dest);
      if (val != 128)
      { if (val > 128)
       	  val -= 129;
        for (m = 0; m < jffam_ff_if_vars; m++)
        { jfg_adjectiv(&adesc, jffam_head,
                       jffam_if_vars[m].f_adjectiv_no + cel_cur[m]);
          fprintf(da_opfile, "%s ", adesc.name);
        }
        jfg_adjectiv(&adesc, jffam_head,
                     jffam_then_var.f_adjectiv_no + val);
        fprintf(da_opfile, "%s\n", adesc.name);
        ant++;
      }
      slut = jffam_next_cel();
    }
    fclose(da_opfile);
    da_opfile = NULL;
    if (jffam_silent == 0)
      fprintf(sout, "\n    %d rules written to the data file: %s\n",
              ant, jffam_ru_fname);
  }
  if (jffam_change_program == 1)
  { ant = 0;
    pc = jffam_program_id;
    jffam_ins_rules = 0;
    slut = jffam_first_cel();
    while (slut == 0)
    { val = jffam_cel_get(cel_dest);
      if (val != 128)
      { if (val >128)
	         val -= 129;
	       pc = jffam_ins_fam_rule(pc, val);
	       ant++;
      }
      slut = jffam_next_cel();
    }
    if (jffam_silent == 0)
      fprintf(sout, "\n    %d rules inserted in the program: %s\n",
              ant, jffam_op_fname);
  }
}

static int jffam_create(void)
{
  int m, res, changed;
  unsigned char *tmp_cel;

  res = jffam_cel_create();
  if (res == 0)
  { changed = 1;
    fprintf(sout, "\n    Cellular automat started...\n");
    for (m = 0; changed == 1 && m < jffam_max_steps; m++)
    { if (m != 0)
      { tmp_cel = cel_source;
        cel_source = cel_dest;
        cel_dest = tmp_cel;
      }
      changed = jffam_cel_step();
    }
      fprintf(sout, "    Cellular automat stopped after %d steps.\n", m);

    jffam_write();
  }
  return res;
}

static int jffam_data(unsigned char *program_id)
{
  int rule_type, res;
  unsigned long cruleno = 0;

  jffam_program_id = program_id;
  if ((jffam_darea = (unsigned char *) malloc(jffam_data_size)) == NULL)
    return jf_error(6, jffam_empty, JFE_FATAL);
  jffam_ipcount = 0;
  jffam_ff_darea = 0;
  jffam_ins_rules = 0;

  jffam_drec_size = sizeof(float) + sizeof(short) +
		   sizeof(unsigned char) * (jffam_ff_if_vars + 1 + 1);
  if (jffam_drec_size % 2 == 1)
    jffam_drec_size++;

  while (jffam_ip_get() == 0)
  { jffam_ipcount++;
    if (jffam_oom() == 0)
    { jffam_ip2rule(jffam_ff_darea);
      jffam_ff_darea++;
      rule_type = jffam_rcheck(&cruleno, jffam_ff_darea - 1);
      if (rule_type == JFRD_CMP_DATA_CONTRADICTION)
      { jffam_resolve_contradiction(cruleno, jffam_ff_darea - 1);
      }
      else
      if (rule_type == JFRD_CMP_EQ)
      { jffam_s_update(cruleno);
       	jffam_ff_darea--;
      }
      /* else the rule is inserted */
      if (jffam_oom() == 1)
       	return jf_error(6, jffam_empty, JFE_FATAL);
    }
  } /* while */
  fprintf(sout, "\n    %d fixed rules created from %d data-sets.\n",
	               (int) jffam_ff_darea, (int) jffam_ipcount);
  jfp_d_statement(jffam_head, program_id);
  jffam_red_contra();

  res = jffam_create();
  if (res == 0)
  { free(jffam_darea);
    jffam_darea = NULL;
    jffam_ff_darea = 0;
  }
  return res;
}

int jffam_run(char *op_fn, char *ip_fn, char *ru_fn, char *da_fn,
              char *field_sep, int data_mode, long prog_size, long data_size,
              int res_confl, int ca_rule, float weight_val, int then_type,
              int fixed, int steps,
              char *sout_fn, int append, int batch)
{
  int m;
  unsigned char *pc;
  int slut, res = 0;

  sout = stdout;
  if (sout_fn != NULL && strlen(sout_fn) != 0)
  { if (append == 1)
      sout = fopen(sout_fn, "a");
    else
      sout = fopen(sout_fn, "w");
    if (sout == NULL)
    { sout = stdout;
      jf_error(1, sout_fn, JFE_ERROR);
    }
  }
  strcpy(jffam_field_sep, field_sep);
  strcpy(jffam_ru_fname, ru_fn);
  strcpy(jffam_op_fname, op_fn);
  strcpy(jffam_da_fname, da_fn);
  jffam_data_mode = data_mode; jffam_prog_size = prog_size;
  jffam_data_size = data_size; jffam_res_confl = res_confl;
  jffam_ca_rule = ca_rule; jffam_weight_val = weight_val; jffam_fixed = fixed;
  jffam_then_type = then_type;
  jffam_max_steps = steps;
  if (jffam_weight_val > 0.0)
    jffam_use_weight = 1;
  if (ru_fn != NULL && strlen(ru_fn) != 0)
  { if ((da_opfile = fopen(ru_fn, "w")) == NULL)
      return jf_error(1, ru_fn, JFE_FATAL);
  }

  if (op_fn != NULL && strlen(op_fn) != 0)
    jffam_change_program = 1;

  m = jfr_aload(&jffam_head, ip_fn, jffam_prog_size);
  if (m != 0)
    return jf_error(m, ip_fn, JFE_FATAL);
  m = jfg_init(JFG_PM_NORMAL, 64, 4);
  if (m != 0)
    return jf_error(m, jffam_empty, JFE_FATAL);
  m = jfp_init(0);
  if (m != 0)
    return jf_error(m, jffam_empty, JFE_FATAL);

  jft_init(jffam_head);
  for (m = 0; m < strlen(jffam_field_sep); m++)
    jft_char_type(jffam_field_sep[m], JFT_T_SPACE);
  m = jft_fopen(da_fn, data_mode, 0);
  if (m != 0)
    return jf_error(m, da_fn, JFE_FATAL);

  jfg_sprg(&jffam_pdesc, jffam_head);

  fprintf(sout, "\n  FAM creation startet\n");
  pc = jffam_pdesc.pc_start; slut = 0;
  jfg_statement(&jffam_sdesc, jffam_head, pc);
  while (slut == 0 && jffam_sdesc.type != JFG_ST_EOP)
  { if (jffam_sdesc.type == JFG_ST_IF
        && jffam_sdesc.sec_type == JFG_SST_EXTERN)
    { jfg_t_statement(jffam_text, JFRD_MAX_TEXT, 2,
		                    jffam_tree, JFRD_TREE_SIZE,
                      jffam_head, -1, pc);
      if (jffam_silent == 0)
	       fprintf(sout, "\n  Now handling statement:\n  %s\n", jffam_text);
      m = jfg_a_statement(jffam_words, JFRD_WMAX, jffam_head, pc);
      if (m < 0)
       	return jf_error(519, jffam_text, JFE_FATAL);
      if (jffam_get_command(m) == 0)
      {	res = jffam_data(pc);
        slut = 1;
      }
      else
       	pc = jffam_sdesc.n_pc;
    }
    else
      pc = jffam_sdesc.n_pc;
    jfg_statement(&jffam_sdesc, jffam_head, pc);
  }

  if (res == 0)
  { if (slut == 0)
      return jf_error(520, jffam_empty, JFE_FATAL);

    if (jffam_change_program == 1)
    { m = jfp_save(op_fn, jffam_head);
      if (m != 0)
        return jf_error(m, op_fn, JFE_FATAL);
    }

    fprintf(sout, "\n  FAM creation completed\n");
  }
  jffam_close();
  if (batch == 0)
  { printf("Press RETURN ...");
    fgets(jffam_ru_fname, 78, stdin);  /* dummy get */
  }
  return 0;
}

