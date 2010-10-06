  /*********************************************************************/
  /*                                                                   */
  /* jfs_text.h   Version  2.00   Copyright (c) 1999 Jan E. Mortensen  */
  /*                                                                   */
  /* JFS constant-texts.                                               */
  /*                                                                   */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                */
  /*    Lollandsvej 35 3.tv.                                           */
  /*    DK-2000 Frederiksberg                                          */
  /*    Denmark                                                        */
  /*                                                                   */
  /*********************************************************************/

#ifndef jfstextH
#define jfstextH


/***********************************************************************/
/* Domains:                                                            */

/* domain-types:                                                       */
extern char *jfs_t_dts[];

/***********************************************************************/


/***********************************************************************/
/* Variables:                                                          */

extern char *jfs_t_vts[];

/* defuz-functions:                                                    */
extern char *jfs_t_vds[];

/* domain-composite function-types:                                    */
extern char *jfs_t_vcfs[];

/***********************************************************************/


/***********************************************************************/
/* Hedges:                                                             */

/* hedge-types:                                                        */
extern char *jfs_t_hts[];

/***********************************************************************/


/***********************************************************************/
/* Operators:                                                          */


/* fuzzy operator types:                                               */
extern char *jfs_t_fops[];

extern char *jfs_t_oph_modes[];

/***********************************************************************/


/***********************************************************************/
/* Reserved names:                                                     */

extern char *jfs_t_reserved[];

/***********************************************************************/


/***********************************************************************/
/* Statement elements:                                                 */


/* single-argument functions:                                          */
extern char *jfs_t_sfus[];

/* variable functions:                                                 */
extern char *jfs_t_vfus[];

/* operators/relations/2-arg-functions                                 */
extern char *jfs_t_dfus[];

/***********************************************************************/
/* functions:                                                          */

int jfst_txt_find(char *txts[], int count, char *name);
int jfst_rname_find(char *name);  // find reserved word
int jfst_htype_find(char *name);  // find hedge-type
int jfst_dtype_find(char *name);  // find domain-type
int jfst_defuz_find(char *name);  // find defuz
int jfst_otype_find(char *name);  // find operator-type
int jfst_ohmode_find(char *name);// find operator_hedge_mode
int jfst_vtype_find(char *name);  // find variable-type
int jfst_dctype_find(char *name); // find domain-composite-type
int jfst_sfunc_find(char *name);  // find single-arg-function
int jfst_dfunc_find(char *name);  // find opertaor/double-funktion
int jfst_vfunc_find(char *name);  // find variable-function

#endif
