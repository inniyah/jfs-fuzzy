
  /**************************************************************************/
  /*                                                                        */
  /* jfr_lib.h   Version  2.03    Copyright (c) 1998-2000 Jan E. Mortensen  */
  /*                                                                        */
  /* Functions to run a compiled JFS-program.                               */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

#ifndef jfrlibH
#define jfrlibH

/* ------------------------------------------------------------------- */
/* Basic load/run-functions                                            */
/* ------------------------------------------------------------------- */


int jfr_init(unsigned short stack_size);

   /* Initialises the jfr-system. If not called explicit, then jfr_init() */
   /* is called from jfr_load(). If <stack_size> == 0 then stack gets     */
   /* default-size (1024 bytes).                                          */
   /* Return:                                                             */
   /* 0: ok,                                                              */
   /* 6: Cannot allocate memory to stack.                                 */
   
int jfr_load(void **jfr_head, char *fname);

   /* Loads a compiled jfs-program from the file <fname>.   */
   /* <jfr_head>  points to the loaded program.             */
   /* Return:                                               */
   /* 0: ok,                                                */
   /* 1: cannot open file,                                  */
   /* 2: Error reading from file,                           */
   /* 4: The file does not contain a jfs-program,           */
   /* 5: A newer version of jfr_lib is needed,              */
   /* 6: Cannot allocate enough memory.                     */


void jfr_run(float *op, void *jfr_head, float *ip);

   /* Activates and executes the jfs-program <jfr_head>.    */
   /* Input is taken from the array <ip>, and output is     */
   /* written to the array <op>.                            */
   /* <ip>[0] is value to first input variable, <ip>[1] is  */
   /* value to second input variable and so on. After       */
   /* execution of jfr_run()  <op>[0] is value of first     */
   /* output variable and so on.                            */


int jfr_close(void *jfr_head);

   /* release the memory allocated to the jfs-program       */
   /* <jfr_head>. Return:                                   */
   /* 0: ok,                                                */
   /* 4: <jfr_head> does not point to a jfs-program.        */

int jfr_free(void);

   /* release memory allocated by the jfr-system.           */


/* ------------------------------------------------------------------- */
/* Extended load/run-functions                                         */
/* ------------------------------------------------------------------- */

int jfr_aload(void **jfr_head, char *fname, unsigned short e_psize);


    /* Like jfr_load, but with allocation of extra memory.           */
    /* Allocate <e_psize> extra bytes to program.                    */
    /* Return: see jfr_load.                                         */


void jfr_arun(float *op, void *jfr_head, float *ip, float *confidence,
              void (*call)(void),
              void (*mcall)(int place),
              void (*uvget)(int vno));

   /* Like jfr_run, but with extra parameters. Activate and run the  */
   /* jfs-program <jfr_head> using the function <call> to execute    */
   /* extern-statements. If <call> == NULL, then extern-statements   */
   /* are ignored. The function <mcall> is called before the first   */
   /* statements is executed with the argument '0' (<place>==0). It  */
   /* is called after the executing of each statements with the      */
   /* argument '1', and finally after the last statement are         */
   /* executed and the output variables calculated, with the         */
   /* argument '2'. If <mcall>==NULL no calls are made.  The         */
   /* function <uvget> is called every time a undefined variable is  */
   /* used in an expression. The number of the undefined variable    */
   /* is <vno>. If <uvget>==NULL no calls are made.                  */
   /* The confidence-value of the first input-variable are set to    */
   /* <confidence>[0], the second is set to <confidence>[1] and so   */
   /* on. If <confidence> == NULL then confidence is set to 1.0 for  */
   /* all input variables. A confidence < 0.0 sets the variable to   */
   /* undefined. If conficence == -1.0 then all fuzzy variables      */
   /* bound to <vno> are set to 1.0. If confindence == -2.0 all fuzzy*/
   /* variables aer set to 1.0 / number-of-fuzzy-varibables. If      */
   /* confidence is another negative number (-3.0 for example) all   */
   /* fuzzy variables bound to <vno> are set to 0.0.                 */


/* ------------------------------------------------------------------- */
/* Execution control/change functions.                                 */
/* ------------------------------------------------------------------- */

void jfr_activate(void *jfr_head,
                  void (*call)(void),
                  void (*mcall)(int place),
                  void (*uvget)(int vno));

    /* Makes <jfr_head> the active jfs-program and initialises all     */
    /* variables etc.                                                  */


int jfr_error(void);

    /* returns the first error-code since last call of jfr_error.   */
    /* The call clears the error-state (sets error-code to 0).      */
    /* Posible return values:                                       */
    /*   0: no errors,                                              */
    /* 201: Fuzzification error: All fzvars == 0, cannot normalise. */
    /* 202: Variable-value out of domain range.                     */
    /* 203: Illegal operation.                                      */
    /* 204: Cannot defuzzificate, all fuzzy-variables == 0.         */
    /* 205: Function argument out of range.                         */
    /* 206: Stack overflow .                                        */


struct jfr_stat_desc /* Information about last executed statement:    */
{
   float weight;     /* Value of global weight after execution of */
                     /* last statement.                           */
   float rm_weight;  /* Value of global weight before exection of */
                     /* last statement.                           */
   float cond_value; /* value of condition-expression last        */
                     /* statement.                                */
   float expr_value; /* value of last then-expression.            */
   float index_value;/* if last statement was an array-assignment,*/
                     /* then array-index-value.                   */
   int changed;      /* Did last statement change a variable or   */
                     /* the global weight.                        */
                     /* 1:yes, 0:no.                              */
   int rule_no;      /* Statement-number last statement (in       */
                     /* function/main-program).                   */
   int function_no;  /* >=0: number actual function,              */
                     /*  -1: in main-program.                     */
   unsigned char *pc;/* Program-address last statement (if called */
                     /* from an extern-statement <pc> is address  */
                     /* of extern-statement).                     */
};

void jfr_statement_info(struct jfr_stat_desc *sprg);

    /* returns information about the last executed statement in     */
    /* <sprg> (to be called from mcall()).                          */

/* -------------------------------------------------------------------- */
/* Variable-handling funtions                                           */
/* -------------------------------------------------------------------- */

void jfr_clear(unsigned short var_no, int fzvclear);

   /* sets the value of variable <var_no> in active jfs to          */
   /* default-value with confidence=0. The variables status is set  */
   /* to undefined. The value of fzvars bound to <var_no> is set    */
   /*depending on  <fzvclear>:                                      */
   /* JFR_FCLR_ZERO: values of fzvars are set to 0.0,               */
   /* JFR_FCLR_ONE: values of fzvars are set to 1.0,                */
   /* JFR_FCLR_AVG: values are set to 1.0 / number-of-fzvars.       */

#define JFR_FCLR_ZERO    0
#define JFR_FCLR_ONE     1
#define JFR_FCLR_AVG     2
#define JFR_FCLR_DEFAULT 3

float jfr_vget(unsigned short var_no);

   /* returns the value of domain variable number <var_no> in        */
   /* the active jfs.                                                */



float jfr_cget(unsigned short var_no);

   /* Returns the confidence-value of domain variable number        */
   /* <var_no>.                                                     */



float jfr_fzvget(unsigned short fzvar_no);

   /* return the value of fuzzy variable number <fzvar_no>.         */



float jfr_aget(unsigned short ar_no, unsigned short id);

  /* return the value of the array  <ar_no>[<id>].                 */



void jfr_vput(unsigned short var_no, float value, float confidence);

   /* Changes the value of domain variable number <var_no> to       */
   /* <value>, and sets its confidence to <confidence>.             */



void jfr_vupd(unsigned short var_no, float value, float confidence);

   /* Updates the value of domain variable number <var_no> with     */
   /* <value> and <confidence>. The call of jfr_vupd has the same   */
   /* effect as the jfs-statement:                                  */
   /* 'if <confidence> then <var_name> = <value>;'                  */
   /* (where <var_name> is the name of variable number <var_no>).   */



void  jfr_fzvput(unsigned short fzvar_no, float value);

   /* Changes the value of fuzzy variable number <fzvar_no> to      */
   /* <value>.                                                      */



void jfr_aput(unsigned short ar_no, unsigned short id, float val);

   /* Changes the value of the array <ar_no>[<id>] to <val>.     */



void  jfr_fzvupd(unsigned short fzvar_no, float value);

   /* updated the value of <fzvar_no>. Updates it with <value>.     */
   /* The call is equal to the jfs-statement:                       */
   /* 'if <value> then <var_name> <adj_name>';                      */
   /* (where <var_name> <adj_name> is the name of fuzzy variable    */
   /* number <fzvar_no>).                                           */



/* -------------------------------------------------------------------- */
/* Expresion-calculating                                                */
/* -------------------------------------------------------------------- */


float jfr_op_calc(unsigned short op_no, float v1, float v2);

   /* Calculates the result of using user-defined operator number   */
   /* <op_no> on 2 arguments <v1> and <v2>. Equal to the            */
   /* jfs-expresion: '<v1> <op_name> <v2>'                          */
   /* (where <op_name> is the name of operator number <op_no>).     */



float jfr_hedge_calc(unsigned short hno, float v);

  /* calculates the result of using hedge number <hno> on <v>:      */
  /* '<hname>(<v>)'                                                 */
  /* (where <hname> is the name of hedge number <hno>).             */



float jfr_rel_calc(unsigned short rel_no, float val1, float val2);

  /* calculates the result of using userdefined relation number     */
  /* <rel_no> on <val1> and <val2>. Equal to the jfs-expression:    */
  /* '<val1> <rel_name> <val2>'                                     */
  /* (where <rel_name> is the name of relation number <rel_no>).    */



#endif
