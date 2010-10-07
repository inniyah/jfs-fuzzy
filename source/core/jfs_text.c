  /*********************************************************************/
  /*                                                                   */
  /* jfs_text.cpp Version  2.00   Copyright (c) 1999 Jan E. Mortensen  */
  /*                                                                   */
  /* JFS constant-texts.                                               */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#include <stdio.h>
#include <string.h>
#include "jfs_cons.h"

/***********************************************************************/
/* Domains:                                                            */

/* domain-types:                                                       */
const char *jfs_t_dts[] =
			  { "float",          /* 0 */
			    "integer",        /* 1 */
			    "categorical"     /* 2 */
			  };

/***********************************************************************/


/***********************************************************************/
/* Variables:                                                          */

/* variable-types                                                      */
const char *jfs_t_vts[] =
               { "input",         /* 0 */
                 "output",        /* 1 */
                 "local"          /* 2 */
               };

/* defuz-functions:                                                    */
const char *jfs_t_vds[] =
			  { "centroid",        /* 0 */
			    "cmax",            /* 1 */
			    "avg",             /* 2 */
			    "first",           /* 3 */
			    "firstmax",        /* 4 */
			    "lastmax"          /* 5 */
			  };

/* domain-composite function-types:                                    */
const char *jfs_t_vcfs[] =
			  { "new",           /* 0 */
			    "avg",           /* 1 */
			    "max"            /* 2 */
			  };

/***********************************************************************/


/***********************************************************************/
/* Hedges:                                                             */

/* hedge-types:                                                        */
const char *jfs_t_hts[] =
			  { "negate",        /* 0 */
			    "pow",           /* 1 */
			    "sigmoid",       /* 2 */
			    "round",         /* 3 */
			    "yager_not",     /* 4 */
			    "bell",          /* 5 */
			    "tcut",          /* 6 */
			    "bcut",          /* 7 */
			    "plf"            /* 8 */
			  };


/***********************************************************************/


/***********************************************************************/
/* Operators:                                                          */


/* fuzzy operator types:                                               */
const char *jfs_t_fops[] =
			{ "#",               /*  0 = op_one */
			  "min",             /*  1 */
			  "max",             /*  2 */
			  "prod",            /*  3 */
			  "psum",            /*  4 */
			  "avg",             /*  5 */
			  "bsum",            /*  6 */
			  "new",             /*  7 */
			  "mxor",            /*  8 */
			  "sptrue",          /*  9 */
			  "spfalse",         /* 10 */
			  "smtrue",          /* 11 */
			  "smfalse",         /* 12 */
			  "r0",              /* 13 */
			  "r1",              /* 14 */
			  "r2",              /* 15 */
			  "r3",              /* 16 */
			  "r4",              /* 17 */
			  "r5",              /* 18 */
			  "r6",              /* 19 */
			  "r7",              /* 20 */
			  "r8",              /* 21 */
			  "r9",              /* 22 */
			  "r10",             /* 23 */
			  "r11",             /* 24 */
			  "r12",             /* 25 */
			  "r13",             /* 26 */
			  "r14",             /* 27 */
			  "r15",             /* 28 */
			  "ham_and",         /* 29 */
			  "ham_or",          /* 30 */
			  "yager_and",       /* 31 */
			  "yager_or",        /* 32 */
			  "bunion",          /* 33 */
     "similar"          /* 34 */
  };

const char *jfs_t_oph_modes[] =
              { "none",          /* 0  */
                "arg1",          /* 1  */
                "arg2",          /* 2  */
                "arg12",         /* 3  */
                "post"           /* 4  */
              };

/***********************************************************************/

/***********************************************************************/
/* Reserved names:                                                     */

const char *jfs_t_reserved[] =
              { "fbool",      /* 0  domains  */
                "float",      /* 1           */
                "weight",     /* 2  variables*/
                "not",        /* 3  hedges   */
                "caseop",     /* 4  operators*/
                "weightop",   /* 5           */
                "and",        /* 6           */
                "or",         /* 7           */
                "whileop"     /* 8           */
              };

/***********************************************************************/


/***********************************************************************/
/* Statement elements:                                                 */


/* single-argument functions:                                          */
const char *jfs_t_sfus[] =
{ "cos",      /*  0 */
			"sin",      /*  1 */
			"tan",      /*  2 */
			"acos",     /*  3 */
			"asin",     /*  4 */
			"atan",     /*  5 */
			"log",      /*  6 */
			"fabs",     /*  7 */
			"floor",    /*  8 */
			"ceil",     /*  9 */
			"-",        /* 10 */
			"random",   /* 11 */
			"sqr",      /* 12 */
			"sqrt",     /* 13 */
   "wget",     /* 14 */
};


/* variable functions:                                                 */
const char *jfs_t_vfus[] =
 { "dnormal",    /*  0 */
			"m_fzvar",    /*  1 */
			"s_fzvar",    /*  2 */
			"default",    /*  3 */
			"confidence"  /*  4 */
 };


/* operators/relations/2-arg-functions                                 */

const char *jfs_t_dfus[] =
 { "+",        /*  0 */
			"-",        /*  1 */
			"*",        /*  2 */
			"/",        /*  3 */
			"pow",      /*  4 */
			"min",      /*  5 */
			"max",      /*  6 */
			"cut",      /*  7 */
			" ",        /*  8 */
			" ",        /*  9 */
			"==",       /* 10 */
			"!=",       /* 11 */
			">",        /* 12 */
			">=",       /* 13 */
			"<",        /* 14 */
			"<=",       /* 15 */
  };

int jfst_txt_find(const char *txts[], int count, const char *name)
{
  int m;

  for (m = 0; m < count; m++)
  { if (strcmp(txts[m], name) == 0)
      return m;
  }
  return -1;
}

int jfst_rname_find(const char *name)  /* find reserved word */
{
  return jfst_txt_find(jfs_t_reserved, JFS_RES_COUNT, name);
}

int jfst_htype_find(const char *name)  /* find hedge-type */
{
  return jfst_txt_find(jfs_t_hts, JFS_HT_COUNT, name);
}

int jfst_dtype_find(const char *name)  /* find domain-type */
{
  return jfst_txt_find(jfs_t_dts, JFS_DT_COUNT, name);
}

int jfst_defuz_find(const char *name)  /* find defuz */
{
  return jfst_txt_find(jfs_t_vds, JFS_VD_COUNT, name);
}

int jfst_otype_find(const char *name)  /* find operator-type */
{
  return jfst_txt_find(jfs_t_fops, JFS_FOP_COUNT, name);
}

int jfst_ohmode_find(const char *name) /* find operator_hedge_mode */
{
  return jfst_txt_find(jfs_t_oph_modes, JFS_OHM_COUNT, name);
}

int jfst_vtype_find(const char *name)  /* find variable-type */
{
  return jfst_txt_find(jfs_t_vts, JFS_VT_COUNT, name);
}

int jfst_dctype_find(const char *name)  /* find domain-composite-type */
{
  return jfst_txt_find(jfs_t_vcfs, JFS_VCF_COUNT, name);
}

int jfst_sfunc_find(const char *name)  /* find single-arg-function */
{
  return jfst_txt_find(jfs_t_sfus, JFS_SFU_COUNT, name);
}

int jfst_dfunc_find(const char *name) /* find opertaor/double-funktion */
{
  return jfst_txt_find(jfs_t_dfus, JFS_DFU_COUNT, name);
}

int jfst_vfunc_find(const char *name) /* find variable-function */
{
  return jfst_txt_find(jfs_t_vfus, JFS_VFU_COUNT, name);
}
