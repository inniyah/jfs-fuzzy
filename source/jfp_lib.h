  /**************************************************************************/
  /*                                                                        */
  /* jfp_lib.h   Version  2.00    Copyright (c) 1998-1999 Jan E. Mortensen  */
  /*                                                                        */
  /* Functions to change a compiled jfs-program.                            */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

/* jfp-errors:  401: jfp cannot insert this type of statement.         */
/*              402: Statement to large.                               */
/*              403: Not enogh extra memory in jfs-program.            */
/*              404: To many args to call-statement (max 255).         */

/* Data-structures are described in 'jfg_lib.h'.                       */

int jfp_init(int statsize);

/* Initialises the jfp-library. Reserves memory to a single compiled   */
/* statement. The amount of memory is <statsize>. If <statsize> == 0,  */
/* 512 bytes are reserved.                                             */
/* return: 0: ok,                                                      */
/*         6: cannot allocate memory.                                  */

void jfp_adjectiv(void *head, unsigned short adjectiv_no,
               		 struct jfg_adjectiv_desc *adesc);

/* Change adjectiv number <adjectiv_no> in the jfs-program <head>.     */
/* h1_no  := <adesc>.h1_no,                                            */
/* h2_no  := <adesc>.h2_no.                                            */
/* center := <adesc>.center (if JFS_AF_CENTER).                        */
/* base   := <adesc>.base (if JFS_AF_BASE).                            */
/* trapez_start := <adesc>.trapez_start,                               */
/* trapez_end   := <adesc>.trapez_end (if JFS_TRAPEZ).                 */



void jfp_alimits(void *head, unsigned short adjectiv_no,
               		struct jfg_limit_desc *ldescs);

/* Change the limit-values of adjectiv number <adjectiv_no> in the     */
/* jfs-program <head>.                                                 */
/* limits[0].x := <ldesc>[0].x, limits[0].y := <ldesc>[0].y,           */
/* limits[0].flags := <ldescs[0].flags,                                */
/* limits[0].exclusiv := <ldescs>[0].exclusiv.                         */
/*    .               .            .               .                   */
/*    .               .            .               .                   */
/* limits[adjectiv.limit_c - 1].x := <ldesc>[adjectiv.limit_c - 1].x,  */
/* limits[adjectiv.limit_c - 1].y := <ldesc>[adjectiv.limit_c - 1].y.  */
/* limits[adjectiv.limit_c - 1].flags := <ldescs[adjectiv.limit_c -1].flags */
/* limits[adjectiv.limit_c - 1].exclusiv :=                             */
/*         <ldescs>[adjectiv.limit_c - 1].exclusiv.                     */




void jfp_var(void *head, unsigned short var_no,
        	    struct jfg_var_desc *vdesc);

/* Change domain-variable number <var_no> in the  jfs-program <head>.  */
/* acut        := <vdesc>.acut,                                        */
/* no_arg      := <vdesc>.no_arg,                                      */
/* defuz_arg   := <vdesc>.defuz_arg,                                   */
/* default_val := <vdesc>.default_val,                                 */
/* defuz_1     := <vdesc>.defuz_1,                                     */
/* defuz_2     := <vdesc>.defuz_2,                                     */
/* f_comp      := <vdesc>.f_comp,                                      */
/* d_comp      := <vdesc>.d_comp,                                      */
/* flags       := <vdesc>.flags,                                       */
/* argument    := <vdesc>.argument                                     */




void jfp_hedge(void *head, unsigned short hedge_no,
        	      struct jfg_hedge_desc *hdesc);

/* Change hedge number <hedge_no> in the jfs-program <head>.           */
/* hedge_arg  := <hdesc>.hedge_arg,                                    */
/* hedge_type := <hdesc>.type (if hedge_type != JFG_HT_LIMITS),        */
/* hedge_flags:= <hdesc>.flags.                                        */




void jfp_hlimits(void *head, unsigned short hedge_no,
              		 struct jfg_limit_desc *ldescs);

/* Change the limit-values of hedge number <hedge_no> in the           */
/* jfs-program <head>.                                                 */
/* limits[0].x := <ldesc>[0].x, limits[0].y := <ldesc>[0].y,           */
/* limits[0].flags := <ldescs[0].flags,                                */
/* limits[0].exclusiv := <ldescs>[0].exclusiv.                         */
/*    .               .            .               .                   */
/*    .               .            .               .                   */
/* limits[adjectiv.limit_c - 1].x := <ldesc>[adjectiv.limit_c - 1].x,  */
/* limits[adjectiv.limit_c - 1].y := <ldesc>[adjectiv.limit_c - 1].y.  */
/* limits[adjectiv.limit_c - 1].flags := <ldescs[adjectiv.limit_c -1].flags */
/* limits[adjectiv.limit_c - 1].exclusiv :=                             */
/*         <ldescs>[adjectiv.limit_c - 1].exclusiv.                     */




void jfp_relation(void *head, unsigned short relation_no,
               		 struct jfg_relation_desc *rdesc);

/* Change relation number <relation_no> in the jfs-program <head>.     */
/* hedge_no := <rdesc>.hedge_no,                                       */
/* flags    := <rdesc>.flags.                                          */




void jfp_rlimits(void *head, unsigned short relation_no,
               		struct jfg_limit_desc *ldescs);

/* Change the limit-values of relation number <relation_no> in the     */
/* jfs-program <head>.                                                 */
/* limits[0].x := <ldesc>[0].x, limits[0].y := <ldesc>[0].y,           */
/* limits[0].flags := <ldescs[0].flags,                                */
/* limits[0].exclusiv := <ldescs>[0].exclusiv.                         */
/*    .               .            .               .                   */
/*    .               .            .               .                   */
/* limits[adjectiv.limit_c - 1].x := <ldesc>[adjectiv.limit_c - 1].x,  */
/* limits[adjectiv.limit_c - 1].y := <ldesc>[adjectiv.limit_c - 1].y.  */
/* limits[adjectiv.limit_c - 1].flags := <ldescs[adjectiv.limit_c -1].flags */
/* limits[adjectiv.limit_c - 1].exclusiv :=                             */
/*         <ldescs>[adjectiv.limit_c - 1].exclusiv.                     */




void jfp_operator(void *head, unsigned short operator_no,
              		  struct jfg_operator_desc *odesc);

/* Change operator number <operator_no> in the jfs-program <head>.     */
/* op_arg   := <odesc>.op_arg,                                         */
/* op_1     := <odesc>.op_1,                                           */
/* op_2     := <odesc>.op_2,                                           */
/* hedge_mode := <odesc>.hedge_mode,                                   */
/* hedge_no := <odesc>.hedge_no,                                       */
/* flags    := <odesc>.flags.                                          */




void jfp_statement(unsigned char *pc,
               		  struct jfg_statement_desc *stat);

/* Change the ifw-statement starting at program_address <pc>.          */
/* rule-weight := <stat>.farg.                                         */




void jfp_d_statement(void *head, unsigned char *pc);

/* deletes the statement starting at program-address <pc> in the       */
/* jfs-program <head>.                                                 */





int jfp_i_tree(void *head, unsigned char **pc,
              	struct jfg_statement_desc *stat,
             		struct jfg_tree_desc *tree,
               unsigned short cond_no,
               unsigned short index_no,
               unsigned short expr_no,
               char *argv[], int argc);

/* Inserts the statement described by <stat> and <tree> at            */
/* program_address <pc>. <pc> is changed to program-address of next   */
/* statement (after insert). The use of the other arguments           */
/* depend on the statement-type (<stat>.type and <stat>.sec_type:     */
/* <stat>.type == JFS_ST_IF:                                          */
/*     insert a statement of the type: 'if <cond> then <thexpr>;'     */
/*     <tree>[<cond_no>] is first node in cond. The use of the        */
/*     other arguments depend on <stat>.sec_type:                     */
/*        <stat>.sec_type == JFG_SST_FZVAR:                           */
/*              inserts the statement 'if <cond> then <fzvar>' where  */
/*              <fzvar> = <stat>.sarg_1.                              */
/*        <stat>.sec_type == JFG_SST_VAR:                             */
/*                        == JFG_SST_FUARG                            */
/*              insert the statement 'if <cond> then <var>=<expr>'    */
/*              where <stat>.sarg_1 = var-no and <tree>[<expr_no] is  */
/*              first node in <expr>.                                 */
/*        <stat>.sec_type == JFG_SST_ARR:                             */
/*              inserts the statement:                                */
/*              'if <cond> then <arr>[<e1>]=<e2>;' where              */
/*              <stat>.sarg_1=array-number of <arr>,                  */
/*              <tree>[index_no] is start of <e1> and                 */
/*              <tree>{expr_no] is start  of <e2>.                    */
/*        <stat>.sec_type == JFG_SST_INC:                             */
/*               inserts the statement                                */
/*               'if <cond> then increase/decrease <var> with <expr>;'*/
/*               where <stat>.sarg_1 = domain-var-number <var>,       */
/*               <tree>[<expr_no>] is first node in <expr>.           */
/*        <stat>.sec_type == JFG_SST_EXTERN:                          */
/*               inserts the statement                                */
/*               'if <cond> then extern <a1> <a2> ... <an>;' where    */
/*               <argc> is the number of arguments (n) and            */
/*               <argv>[0]=<a1>, <argv>[1]=<a2> and so on.            */
/*        <stat>.sec_type == JFG_SST_CLEAR:                           */
/*               inserts the statement 'if <cond> then clear <var>;'  */
/*               where <stat>.sarg_1 = domain-var number of <var>.    */
/*        <stat>.sec_type == JFG_SST_PROCEDURE:                       */
/*               inserts the statement                                */
/*               'if <cond> then <proc>(<e1>, <e2>, ..., <en>);'      */
/*               where <stat>.sarg_1 = funtion-no <proc> and          */
/*               <tree>[expr_no] is first node in the argument-list   */
/*               <e1>, <e2> ... <en> (connected by tree-nodes of the  */
/*               type JFG_TT_ARGLIST).                                */
/*        <stat>.sec_type == JFG_SST_RETURN:                          */
/*               insert a statement of the type:                      */
/*               'if <cond> then return <expr>;' where                */
/*               <tree>[<expr_no>] si first node of <expr>.           */
/* <stat>.type == JFG_ST_CASE:                                        */
/*        inserts a statement of the type 'case <expr>;' or           */
/*        'case <adjectiv>;' (depending on <stat>.flags) where        */
/*        <tree>[<expr_no>] is first node in <expr> (if case-type:    */
/*        'case <adjectiv> then <tree>[<expr_no>] should be a node of */
/*        type: JFG_TT_FZVAR).                                        */
/* <stat>.type == JFG_ST_STEND:                                       */
/*        inserts a end-statement. Depending on flags inserts an      */
/*        end-switch, end-while, end-function statement. (no check in */
/*        program of matching "begin-ends").                          */
/* <stat>.type == JFG_ST_SWITCH:                                      */
/*        inserts a 'switch:' or 'switch <var>;' statement (depending */
/*        on <stat>.flags). If 'switch <var>;' then <stat>.sarg_1 is  */
/*        domain-var number of <var>.                                 */
/* <stat>.type == JFG_ST_DEFAULT:                                     */
/*        inserts a 'default;'-statement (no check of surounding      */
/*        switch-block).                                              */
/* <stat>.type == JFG_ST_WHILE:                                       */
/*        inserts the statement 'while <cond>;' where                 */
/*        <tree>[<cond_no>] is first no of <cond>.                    */
/* <stat>.type == JFG_ST_WSET:                                        */
/*        inserts the statement: 'wset <cond>;' where                 */
/*        <tree>[<cond_no>] is first node of <cond> (NOTE: <cond_no>  */
/*        NOT <expr_no>).                                             */
/* tree[<cond_no>] is the root of the conditional expression. If the  */
/* statement is  of the type 'if <cond> then <v>=<vexp>;', then       */
/* tree[<expr_no>] is the root of <vexp>. The statement is inserted   */
/* at positione <pc>. If the statement is of the type 'if <e> then    */
/* extern <arg1> <arg2> ...;', then the arguments are in <argv> with  */
/* <argc>=the number of arguments. <pc> is changed to program-address */
/* of next statement.                                                 */
/* return: 0: ok,                                                     */
/*       401: cannot insert this type of statement (call or rem-      */
/*            statement).                                             */
/*       402: statement to long.                                      */
/*       403: not enogh extra memory in jfs-program.                  */




void jfp_u_oc(void *head, unsigned char *pc,
	      struct jfg_oc_desc *oc);

/* Updates the low-level command in program-address <pc> from <oc>.   */



int jfp_save(char *fname, void *head);

/* Saves the jfs-program <head> in the file <fname>.                  */
/* return:  0: succes,                                                */
/*          1: cannot open file for writing,                          */
/*          3: error writing to file.                                 */


void jfp_free(void);

/* Frees the memory allocated by jfp_init().                          */


