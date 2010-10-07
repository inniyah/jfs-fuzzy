  /**************************************************************************/
  /*                                                                        */
  /* jfea_lib.h  Version  2.02    Copyright (c) 1999-2000 Jan E. Mortensen  */
  /*                                                                        */
  /* JFS  rule creator using evolutionary programing.                       */
  /*                                                                        */
  /* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                     */
  /*    Lollandsvej 35 3.tv.                                                */
  /*    DK-2000 Frederiksberg                                               */
  /*    Denmark                                                             */
  /*                                                                        */
  /**************************************************************************/

/************************************************************************/
/* Statistical informartion about the population and the last created   */
/* individual is placed in the structure-variable  jfea_stat:.          */

struct jfea_stat_rec
{
    float worst_score;    /* score worst individual in population.      */
    float median_score;   /* score median-individual in population.     */
    float old_score;      /* score of the individual, which was deleted */
                          /* to create new individual.                  */
    float p1_score;       /* score parent-1.                            */
    float p2_score;       /* score parent-2.                            */
    int method;           /* Genetic operator used to create new        */
                          /* individual:                                */
                          /* 0: crosover,                               */
                          /* 1: sumcros,                                */
                          /* 2: mutation,                               */
                          /* 3: creation,                               */
                          /* 4: point-crosover,                         */
                          /* 5: rule-crosover,                          */
                          /* 6: step-mutation.                          */
                          /* 7: maxcros,                                */
                          /* 8: mincros                                 */
		     };
extern struct jfea_stat_rec jfea_stat;

/***********************************************************************/
/* Information about last error is found in the structure-variable     */
/* jfea_error_rec:                                                     */

struct jfea_error_rec
{
  int error_no;
  char argument[256];         /* if the error occured in an object     */
                              /* (variable, filename, word etc), then  */
                              /* the name of the object is found here. */
};
extern struct jfea_error_rec jfea_error_desc;

/* Meaning of <error-no>-values:                                        */
/*    1, Cannot open file:" <argument>.                                 */
/*    2, Error reading from file: <argument>.                           */
/*    3, Error writing to file: <argument>.                             */
/*    6, Cannot allocate memory to: <argument>.                         */
/*  504, Syntax error in jfrd-statement: <argument>.                    */
/*  505, Too many variables in statement (max 64).                       */
/*  506, Undefined variable: <argument>.                                */
/*  519, Too many words in statement (max 255).                          */
/*  520, No 'extern jfrd'-statement in program.                         */
/*  522, 'No default'-output, but default not defined.                  */
/*  550, No min/max domain-values for variable: <argument>.             */
/*  551, No adjectives bound to: <argument>.                            */
/*  552, Option 'a' or 'v' has to be specified for: <argument>.         */
/*  553, Too many adjectives (max 24) for 'i'-option to: <argument>.     */
/*  554, 'i'-option without 'a'-option for variable: <argument>.        */
/*  555, No legal option for variable: <argument>.                      */
/*  556, No Input-variables.                                            */
/*  557, 'R'-option, but no fuzzy relations.                            */
/*  560, Only one adjective bound to:<argument>.                        */

int jfea_random(int sup);

/************************************************************************/
/* Special functions:                                                   */

long jfea_protect(void);
/* protects the actual selected individual (it can no longer be changed */
/* by mutated or replaced by another individal created by genetic       */
/* operators). RETURN: a individual-identifier, which can be used by    */
/* jfea_un_protect() and jfea_ind2jfr().                                */

void jfea_un_protect(long ind_id);
/* Removes the protection on individual <ind_id> (<ind_id> is the       */
/* individual-identifier, returned by jfea_protect).                    */

void jfea_ind2jfr(long ind_id);
/* Writes the individual identified by <ind_i> to the jfs-program       */
/* <jfr_head> (specified in jfea_init()).                               */


/************************************************************************/
/* Functions to create, initialise, run, stop and delete the            */
/* jfea-system:                                                         */


int jfea_init(
/* Initialises the jfea-system. Reserves memory to individuals,         */
/* finds and analyses the 'extern jfrd'-statement in the loaded         */
/* jfs-program <jfr_head>. Sets up option-values.                       */

              void *jfr_head,  /* the jfs-program.                      */
              int ind_c,       /* Number of individs in population.     */
              int rule_c,      /* Number of rules (0: adjectiv_c rules).*/
              int fixed_rules, /* 1: first rule: 'then <adjectiv_1>',   */
                               /*    sec rule:   'then <adjectiv_2>'    */
                               /* etc.                                  */
              int no_default   /* 1: don't create 'then <adj>'-rules    */
                               /*    if <adj> is default-adjectiv.      */
             );
/* Return codes: 0: succes, other: error (see jfea_error_rec for        */
/* error details.                                                       */



int jfea_run(
/* Creates teh initial population. Judges each individual by writing    */
/* the indiviual to jfr_head (specified in jfea_init()) and calling     */
/* the function <judge> (the score of an individual is the              */
/* return-value from the call). Then start creating new individuals,    */
/* calling the judge-function after each creation. Stops when the       */
/* function jfea_stop() (see below) is called (from <judge>).           */

             float (*judge)(void),       /* judge-function.             */
    	        int maximize                /* 1: biggest score is best,   */
                                         /* 0: smallest score is best.  */
            );




void jfea_stop(void);

/* Stops the creation of individuals (typical called from the             */
/* judge-function specified in jfea_run()). Copy best individ to          */
/* jfr_head (specified in jfea_init().                                    */



void jfea_free(void);  /* free the memory reserved by jfea_init(). */

