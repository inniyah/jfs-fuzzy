/***************************************************************************/
/* Global error messages to JFR.                                           */
/***************************************************************************/

char *jf_err_texts[] =
		    { " ",                                        /*  0 */
		      "Error opening file:",                      /*  1 */
		      "Error reading from file:",                 /*  2 */
		      "Error writing to file:",                   /*  3 */
		      "Not a jfr-file:",                          /*  4 */
		      "Wrong version.",                           /*  5 */
		      "Cannot allocate memory to:",               /*  6 */
		      "Syntax error in:",                         /*  7 */
		      "Out of memory",                            /*  8 */
		      "Illegal number:",                          /*  9 */
		      "Value out of domain range:",               /* 10 */
		      "Unexpected EOF.",                          /* 11 */
		      "Unknown error.",                           /* 12 */
                      "Undefined adjectiv:",                      /* 13 */
                      "Missing start/end of interval.",           /* 14 */
                      "Too many tokens in first data-line (max 256).",  /* 15 */
                      "No value for variable in first line.",     /* 16 */
                      "Illegal data-file mode.",                  /* 17 */
                      "Token to long (max 255 chars).",           /* 18 */
                      "Penalty-matrix and more than one output-var." /* 19 */
                      "Too many penalty values (max 64).",         /* 20 */
                      "No values in first data-line.",            /* 21 */
		      " "
		     };

/* error numbers:   100..199:  jfc-errors,          */
/*                  200..299:  jfr_lib-errors,      */
/*                  300..399:  jfg_lib-errors,      */
/*                  400..499:  jfp_lib-errors,      */
/*                  500..599:  jfrd-errors,         */
/*                  600..699:  jfr-errors,          */
/*                  700..799:  jfr2s-errors,        */
/*                  800..899:  jfi-errors,          */
/*                  900..999:  jfgp-errors,         */
/*                1000..1099:  jfol_errors,         */
/*                1100..1199:  jhlp-errors,         */
/*                1200..1299:  jfqr-errors,         */
/*                1300..1399:  jfrplt-errors,       */

