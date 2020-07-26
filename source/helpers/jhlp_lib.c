/***************************************************************************/
/*                                                                         */
/* jhlp_lib.cpp     Version 1.02     Copyright (c) 1999-2000 Jan Mortensen */
/*                                                                         */
/* Converts a jhlp-system to html.                                         */
/*                                                                         */
/* by Jan E. Mortensen     email:  jemor@inet.uni2.dk                      */
/*    Lollandsvej 35 3.tv.                                                 */
/*    DK-2000 Frederiksberg                                                */
/*    Denmark                                                              */
/*                                                                         */
/***************************************************************************/

#ifndef _WIN32
  #include <unistd.h>
#else
  #include <dir.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "jfm_lib.h"

struct jhlp_head_desc
{
  signed short txt_id;
  signed short fchild_id;  /* first child id  */
  signed short fnote_id;   /* first node id   */
  char name[16];           /* identifier      */
  char sname[16];          /* short name      */
  unsigned char flags;     /* see below,      */
  char status;             /* see below ,     */
};
#define JHLP_HEAD_SIZE sizeof(struct jhlp_head_desc)

/* flags is combined of : */
#define JHLP_HF_ORDERED 1
#define JHLP_HF_NOWRITE 2

/* status is one of: */
#define JHLP_HST_EMPTY     0
#define JHLP_HST_WRITTEN   1

struct jhlp_note_desc
{
  signed short fname_id;
  char txt[80];
};
#define JHLP_NOTE_SIZE sizeof(struct jhlp_note_desc)

struct jhlp_label_desc
{
  signed short head_id;
  char name[16];
};
#define JHLP_LABEL_SIZE sizeof(struct jhlp_label_desc)

struct jhlp_index_desc
{
  signed short head_id;
  signed short label_id;
  signed short txt_id;
};
#define JHLP_INDEX_SIZE sizeof(struct jhlp_index_desc)

struct jhlp_glt_desc  /* describes a gloasry, list og table */
{
  signed short fodata_id;
  signed short txt_id;
  char name[16];
  char type;
  char option;  /* = 1: list, table: sorted. glosary: in table-form */
};
#define JHLP_GLT_SIZE sizeof(struct jhlp_glt_desc)
/* where type is one of JHLP_DM_GLOSARY, JHLP_DM_TABLE, JHLP_DM_LIST, */
/*                      JHLP_DM_INCLUDE */


struct jhlp_odata_desc  /* describes an entry in a glosary, list or tabel */
{
  signed short name_id;  /* rowident, lineident or glos-lname  */
  signed short fdata_id;
  signed short vers_id;  /* id version-txt */
};
#define JHLP_ODATA_SIZE sizeof(struct jhlp_odata_desc)

struct jhlp_var_desc
{
  char name[16];
  char value[16];
};
#define JHLP_VAR_SIZE sizeof(struct jhlp_var_desc)

struct jhlp_expr_desc
{
  char name[16];
  char value[16];
  int negate;
  int op;  /* operator */
};
/* where op is one of: */
#define JHLP_EOP_NONE 0
#define JHLP_EOP_EQ   1
#define JHLP_EOP_NEQ  2
#define JHLP_EOP_GT   3
#define JHLP_EOP_LT   4
#define JHLP_EOP_GEQ  5
#define JHLP_EOP_LEQ  6

struct jhlp_expr_desc jhlp_expr;

#define JHLP_CT_EOL     0
#define JHLP_CT_SPACE   1
#define JHLP_CT_LETTER  2
#define JHLP_CT_NEGATE  3
#define JHLP_CT_CMP     4

static int jhlp_fid;  /* list of (full) file-names. */
static int jhlp_cid;  /* list of contents-headers.   */
static int jhlp_iid;  /* list of indexes.           */
static int jhlp_lid;  /* list of labels.            */
static int jhlp_gltid;  /* list of glt's.            */
static int jhlp_vid;  /* list of variables.         */

static int jhlp_jhi_exists = 0;

static int jhlp_contents_c = 0;
static int jhlp_glosary_c = 0;
static int jhlp_index_c = 0;

static FILE *jhlp_sout = NULL;
static FILE *jhlp_ifp = NULL;  /* current jhd- or jhp-file */
static FILE *jhlp_ofp = NULL;  /* current html-file */
static FILE *jhlp_jhc_fp = NULL;  /* current jhc-file */
static FILE *jhlp_jhi_fp = NULL;

#define JHLP_DM_TEXT    0
#define JHLP_DM_GLOS    1
#define JHLP_DM_TABLE   2
#define JHLP_DM_LIST    3
#define JHLP_DM_INCLUDE 4
static int jhlp_dmode;

#define JHLP_GM_HTML 0
#define JHLP_GM_TEXT 1
static int jhlp_gmode = JHLP_GM_HTML;

#define JHLP_HM_OFF   0   /* replace html special-chars with escaped chars, */
#define JHLP_HM_ON    1   /* don't change html special-chars,               */
#define JHLP_HM_FIRST 2   /* replace html spec-chars if char in position 0  */
                          /* is a '<'.                                      */
static int jhlp_html_mode = JHLP_HM_FIRST;

static int jhlp_fase;

static int jhlp_vers_ok = 1; /* 1: ok to add to table, list, text.           */
                             /* 0: don't add because older version.          */
#define JHLP_TXT_LEN 256
static char jhlp_txt[JHLP_TXT_LEN];

static int noget_skrevet = 0;

#define JHLP_ARG_COUNT 32
static char *jhlp_argv[JHLP_ARG_COUNT];
static int jhlp_argc;
static char jhlp_argtxt[JHLP_TXT_LEN];

#define JHLP_PATH_SIZE 32
static int jhlp_path[JHLP_PATH_SIZE];
int jhlp_path_c;

#define JHLP_IFSTACK_SIZE 64
static char jhlp_ifstack[JHLP_IFSTACK_SIZE];
int jhlp_top_ifstack;
#define JHLP_IS_FALSE  0
#define JHLP_IS_TRUE   1
#define JHLP_IS_IFALSE 2

#define JHLP_MAX_PATH 260
static char jhlp_cur_fname[JHLP_MAX_PATH] = "";
static char jhlp_dest_dir[JHLP_MAX_PATH] = ""; /* write html-files to this dir */
static char jhlp_defile_dir[JHLP_MAX_PATH] = ""; /* dir for jhlp_de_fname */
static char jhlp_source_dir[JHLP_MAX_PATH] = ""; /* read files from this dir */
static char jhlp_jhi_fname[JHLP_MAX_PATH] = "";
static char jhlp_jhc_fname[JHLP_MAX_PATH] = "";
static char jhlp_jhd_fname[JHLP_MAX_PATH] = "";
static char jhlp_de_fname[JHLP_MAX_PATH] = ""; /* write print-html-file to this file */
static char jhlp_stylesheet[JHLP_MAX_PATH] = "";
static char jhlp_prhead[JHLP_MAX_PATH] = "";
static int  jhlp_silent = 0;
static int  jhlp_copy_mode = 0;

static int  jhlp_line_no = 0;
static int  jhlp_cur_head_id = -1;
static int  jhlp_cur_glt_id = -1;
static int  jhlp_cur_od_id = -1;

struct jhlp_com_desc {
	int value;
	const char *name;
};

#define JHLP_COM_HEAD    0
#define JHLP_COM_CHEAD   1
#define JHLP_COM_DTAB    2
#define JHLP_COM_TAB     3
#define JHLP_COM_REF     5
#define JHLP_COM_IND     6
#define JHLP_COM_ETC     7
#define JHLP_COM_ETAB    8
#define JHLP_COM_DLIST   9
#define JHLP_COM_LI     10
#define JHLP_COM_ELI    11
#define JHLP_COM_DGLOS  12
#define JHLP_COM_LAB    13
#define JHLP_COM_GLOS   14
#define JHLP_COM_EGLOS  15
#define JHLP_COM_IF     16
#define JHLP_COM_ELSE   17
#define JHLP_COM_ENDIF  18
#define JHLP_COM_DEFINE 19
#define JHLP_COM_UNDEF  20
#define JHLP_COM_TEXT   21
#define JHLP_COM_ETEXT  22
#define JHLP_COM_INCLUDE 23
#define JHLP_COM_HTML    24

#define JHLP_COM_ERROR 9999

static struct jhlp_com_desc jhlp_coms[] =
{ {      JHLP_COM_HEAD,   "!head"},
  {      JHLP_COM_CHEAD,  "!chead"},
  {      JHLP_COM_DTAB,   "!dtab"},
  {      JHLP_COM_ETC,    "!etc"},
  {      JHLP_COM_REF,    "!ref"},
  {      JHLP_COM_IND,    "!ind"},
  {      JHLP_COM_TAB,    "!tab"},
  {      JHLP_COM_ETAB,   "!etab"},
  {      JHLP_COM_DLIST,  "!dlist"},
  {      JHLP_COM_LI,     "!li"},
  {      JHLP_COM_ELI,    "!eli"},
  {      JHLP_COM_DGLOS,  "!dglos"},
  {      JHLP_COM_GLOS,   "!glos"},
  {      JHLP_COM_EGLOS,  "!eglos"},
  {      JHLP_COM_LAB,    "!lab"},
  {      JHLP_COM_IF,     "!if"},
  {      JHLP_COM_ELSE,   "!else"},
  {      JHLP_COM_ENDIF,  "!endif"},
  {      JHLP_COM_DEFINE, "!define"},
  {      JHLP_COM_UNDEF,  "!undef"},
  {      JHLP_COM_TEXT,   "!text"},
  {      JHLP_COM_ETEXT,  "!etext"},
  {      JHLP_COM_INCLUDE,"!include"},
  {      JHLP_COM_HTML,   "!html"},

  {      JHLP_COM_ERROR,  "error"}
};

static char jhlp_t_sorted[] = "sorted";
static char jhlp_t_text[]   = "text";
static char jhlp_t_nul[]    = "0";
static char jhlp_t_nowrite[]= "nowrite";

static char jhlp_h_bpre[]   = "<PRE>";
static char jhlp_h_epre[]   = "</PRE>";
static char jhlp_h_bhtml[]  = "<HTML>";
static char jhlp_h_ehtml[]  = "</HTML>";
static char jhlp_h_bbody[]  = "<BODY>";
static char jhlp_h_ebody[]  = "</BODY>";
static char jhlp_h_btitle[] = "<TITLE>";
static char jhlp_h_etitle[] = "</TITLE>";
static char jhlp_h_bhead[]  = "<HEAD>";
static char jhlp_h_ehead[]  = "</HEAD>";
static char jhlp_h_bh[]        = "<H1>";
static char jhlp_h_eh[]        = "</H1>";

static char jhlp_empty[] = " ";

static int jhlp_error_count = 0;

struct jhlp_err_desc {
	int eno;
	const char *text;
	};

static struct jhlp_err_desc jhlp_err_texts[] =
 { {      0, " "},
   {      1, "Cannot open file:"},
   {      2, "Error reading from file:"},
   {      3, "Error writing to file:"},
   {      6, "Cannot allocate memory to:"},
   {      9, "Illegal number:"},
   {     11, "Unexpected EOF."},
   {   1100, "Not a JHI-file:"},
   {   1101, "Too many words in text."},
   {   1102, "Missing end-qoute in:"},
   {   1103, "Wrong number of arguments to command:"},
   {   1104, "Unknown command:"},
   {   1105, "Unknown head-identifier:"},
   {   1106, "Cannot allocate jfm-memory."},
   {   1107, "Syntax error in var-definition:"},
   {   1108, "Syntax error in definition:"},
   {   1109, "Unknown keyword in head-definition:"},
   {   1110, "Unknown keyword in table-definition:"},
   {   1111, "Unknown keyword in list-definition:"},
   {   1112, "Unknown keyword in gloary-definition:"},
   {   1113, "Unknown label:"},
   {   1114, "No free jfm-node."},
   {   1115, "Too many nested if-statements."},
   {   1116, "!else without !if."},
   {   1117, "!end without !if."},
   {   1118, "Too many contents-levels."},
   {   1119, "Multiple definition of:"},
   {   1120, "Text outside head."},
   {   1121, "Undefined table, list or glosary:"},
   {   1122, "Cannot start definition inside definition:"},
   {   1123, "!etc-comamnd outside !tab."},
   {   1124, "!etab, !eli or !eglos wtihtout start"},
   {   1125, "label defined twice:"},
   {   1126, "Undefined include-text:"},
   {   1127, "Unknown html-mode:"},
   {   9999, "Unknown error!"},
 };

static void jhlp_strcpy(char *dest, const char *source);
static int jhlp_stricmp(const char *a1, const char *a2);
static void jhlp_fclose(FILE *fp);
static int jhlp_error(int eno, const char *name);
static int jhlp_jfm_check(int id);
static void jhlp_set_length(char *txt, unsigned int le);
static void jhlp_splitpath(const char *path, char *dir, char *name, char *ext);
static void jhlp_ext_subst(char *d, const char *e, int forced);
static int jhlp_ext_cmp(const char *fname, const char *ext);
static int jhlp_full_path(char *dfname, char *sfname);
static void jhlp_url(char *url, char *path);
static int jhlp_lcmp(const char *txt, int fid);
static int jhlp_licmp(char *txt, int fid);
static void jhlp_set_filename(char *fname);
static int jhlp_file_copy(char *dfname, char *sfname);
static int jhlp_atof(float *f, char *a);
static int jhlp_readln(FILE *fp);
static int jhlp_command(void);
static int jhlp_str_split(char *sourcetxt);
static int jhlp_f_head(char *name, int fid);
static int jhlp_find_head(char *name);
static int jhlp_fopen(int id);
static int jhlp_path_init(void);
static int jhlp_path_next(void);
static void jhlp_build_path(int tid);
static int jhlp_next_id(int id);
static int jhlp_prev_id(int id);
static int jhlp_init(void);
static int jhlp_load_jhi(char *jhc_fname);
static int jhlp_save_jhi(void);
static int jhlp_head_insert(int fid, struct jhlp_head_desc *hdesc, int sorted);
static int jhlp_create_contents(char *name, char *pname, char *sname,
                               char *lname, int sorted, int nowrite);
static int jhlp_negate(int ibool);
static void jhlp_cappend(char *s, int c, unsigned int ml);
static int jhlp_cmp_op(int c1, int c2);
static int jhlp_find_var(char *name);
static int jhlp_expr_split(char *expr);
static int jhlp_expr_handle(char *expr);
static void jhlp_dexpr_handle(char *dexpr);
static int jhlp_jhd_readln(void);
static int jhlp_write_line(void);
static int jhlp_chap_no(char *chap_txt, int f_pid, int sid);
static void jhlp_write_t_idline(int id, int label_id, char *text);
static void jhlp_write_idline(int id, int label_id);
static int jhlp_rred_contents(int fid);
static int jhlp_reduce_contents(void);
static void jhlp_contents_list(int hid);
static int jhlp_write_contents(void);
static void jhlp_write_navigation(int hid, int invers);
static void jhlp_write_bottom(int cid);
static void jhlp_write_bottoms(int hid);
static int jhlp_write_head(char *hname);
static int jhlp_find_glt(int glttype, char *name);
static int jhlp_find_odata(int gltid, char *name);
static int jhlp_dglt(int type, char *name, char *lname, int sorted);
static int jhlp_odata_ins(int glt_id, char *name, char *vers_name);
static int jhlp_vers_cmp(char *vers_name, signed short oid);
static int jhlp_start_glt(int glttype);
static int jhlp_etc(void);
static int jhlp_end_glt(int type);
static int jhlp_write_table(char *name);
static int jhlp_write_list(char *name);
static int jhlp_write_glosary(const char *name);
static int jhlp_write_include(const char *name);
static int jhlp_find_label(int hid, const char *name);
static int jhlp_lab(void);
static int jhlp_insert_index(void);
static int jhlp_handle_command(void);
static int jhlp_handle_txtline(void);
static int jhlp_handle_line(void);
static int jhlp_jhc_handle(void);
static int jhlp_jhp_handle(void);
static int jhlp_handle(void);
static int jhlp_ins_note(void);
static int jhlp_read_notes(void);
static int jhlp_write_index(void);
static int jhlp_write_glob_glosary(void);
static void jhlp_prlist(int fid, int prhid, int write, int lev, char *chap);
static void jhlp_del_tmp(int fid);
static int jhlp_write_prtext(void);

static int jhlp_stricmp(const char *a1, const char *a2)
{
  int m, res;

  res = -2;
  for (m = 0; res == -2; m++)
  { if (toupper(a1[m]) > toupper(a2[m]))
      res = 1;
    else
    if (toupper(a1[m]) < toupper(a2[m]))
      res = -1;
    else
    if (a1[m] == '\0')
      res = 0;
  }
  return res;
}

static void jhlp_fclose(FILE *fp)
{
  if (fp != NULL)
    fclose(fp);
  fp = NULL;
}

static void jhlp_strcpy(char *dest, const char *source)
{
  if (dest != NULL && source != NULL)
    strcpy(dest, source);
}

static int jhlp_error(int eno, const char *name)
{
  int m, v, e;

  e = 0;
  m = 0;
  for (v = 0; e == 0; v++)
  { if (jhlp_err_texts[v].eno == eno
        || jhlp_err_texts[v].eno == -eno
       	|| jhlp_err_texts[v].eno == 9999)
      e = v;
  }
  if (eno != 0)
  { fprintf(jhlp_sout,
            "*** error %d: %s %s\n", eno, jhlp_err_texts[e].text, name);
    if (strlen(jhlp_cur_fname) > 0)
      fprintf(jhlp_sout,
              "    in line %d in file %s\n", jhlp_line_no, jhlp_cur_fname);
    if (noget_skrevet == 1)
      m = 2;
    else
      m = 1;
    jhlp_error_count++;
  }
  return m;
}

static int jhlp_jfm_check(int id)
{
  /* check if a jfm-call resulted in an error. If error then call jhlp_error */
  /* and return res.                                                         */

  if (id == -6)
    return jhlp_error(1106, jhlp_empty);
  else
  if (id == -14)
    return jhlp_error(1114, jhlp_empty);
  return 0;
}

static void jhlp_set_length(char *txt, unsigned int le)
{
  /* reduces the length of txt to <le> (no change if length(txt) < le.  */

  if (strlen(txt) > le)
    txt[le] = '\0';
}

static void jhlp_splitpath(const char *path, char *dir, char *name, char *ext)
{
  char txt[JHLP_MAX_PATH];
  int m;
  char empty[2];

  empty[0] = '\0';
  strcpy(txt, path);
  m = strlen(txt) - 1;
  while (m >= 0 && txt[m] != '.')
   m--;
  if (m < 0)
    jhlp_strcpy(ext, empty);
  else
  { jhlp_strcpy(ext, &(txt[m]));
    txt[m] = '\0';
  }
  m = strlen(txt) - 1;
  while (m >= 0 && (txt[m] != '\\' && txt[m] != '/'))
    m --;
  if (m < 0)
  { jhlp_strcpy(name, txt);
    jhlp_strcpy(dir, empty);
  }
  else
  { m++;
    jhlp_strcpy(name, &(txt[m]));
    txt[m] = '\0';
    jhlp_strcpy(dir, txt);
  }
}

static void jhlp_ext_subst(char *d, const char *e, int forced)
{
  int m, fundet;
  char punkt[] = ".";

  fundet = 0;
  for (m = strlen(d) - 1; m >= 0 && fundet == 0 ; m--)
  { if (d[m] == '.')
    { fundet = 1;
      if (forced == 1)
        d[m] = '\0';
    }
  }
  if (fundet == 0 || forced == 1)
  { if (strlen(e) != 0)
      strcat(d, punkt);
    strcat(d, e);
  }
}

static int jhlp_ext_cmp(const char *fname, const char *ext)
{
  /* returns 0 if the extension of <fname> is equal to <ext>  */
  /* ('.' is part extension).                                 */

  int res;
  char fext[16];

  jhlp_splitpath(fname, NULL, NULL, fext);
  res = jhlp_stricmp(ext, fext);
  return res;
}

static int jhlp_full_path(char *dfname, char *sfname)
{
  /* Copies the full file-name of <sfname> to <dfname>. If <sfname>   */
  /* already contains a full path returns 1, else return 0.           */
  int res;
#ifndef _WIN32
  int m, i;
  char txt[JHLP_MAX_PATH];
#endif

  res = 0;
  if (sfname[0] == '\\' || sfname[1] == ':' || sfname[0] == '/')
  { res = 1;
    strcpy(dfname, sfname);
  }
  else
  {
#ifndef _WIN32
    getcwd(txt, JHLP_MAX_PATH);
    m = 0;
    while (sfname[m] == '.')
    { if (sfname[m + 1] == '/' && txt[m] != '\\')
        m += 2;
      else
      if (sfname[m + 1] == '.')
      { m += 3;
        i = strlen(txt) - 1;
        while (i > 0 && txt[i] != '/' && txt[i] != '\\')
          i--;
        txt[i] = '\0';
      }
    }
    strcpy(dfname, txt);
    strcat(dfname, "/");
    strcat(dfname, &(sfname[m]));
#else
    _fullpath(dfname, sfname, JHLP_MAX_PATH);
#endif
  }
  return res;
}

static void jhlp_url(char *url, char *path)
{
  /* converts the DOS-file-path in <path> to a url                        */
  /* ('D:\windowm\jfs.gif' is written to <url> as '//D:/windows/jfs.gif). */

  int s, d;

  s = d = 0;
  if (path[1] == ':')
  { url[0] = '/';
    d += 1;
  }
  while (path[s] != '\0')
  { if (path[s] == '\\')
      url[d] = '/';
    else
      url[d] = path[s];
    s++;
    d++;
  }
  url[d] = '\0';
}

static int jhlp_lcmp(const char *txt, int fid)
{
  /* compares a char-string to a list of strings. If it matches one in  */
  /* list return <id> for this, else return -1.                         */

  int res, id;

  res = -1;
  id = fid;
  while (res == -1 && id != -1)
  { if (strcmp(txt, (char *) jfm_data_adr(id)) == 0)
      res = id;
    id = jfm_next(id);
    if (id == fid)
      id = -1;
  }
  return res;
}

static int jhlp_licmp(char *txt, int fid)
{
  /* compares a char-string to a list of strings ignoring upper/lower-cases*/
  /* If it matches one in the list return <id> for this, else return -1.                         */

  int res, id;

  res = -1;
  id = fid;
  while (res == -1 && id != -1)
  { if (jhlp_stricmp(txt, (char *) jfm_data_adr(id)) == 0)
      res = id;
    id = jfm_next(id);
    if (id == fid)
      id = -1;
  }
  return res;
}

static void jhlp_set_filename(char *fname)
{
  /* changes the current file-name to fname (and initialises line_no). */

  strcpy(jhlp_cur_fname, fname);
  jhlp_line_no = 0;
}

static int jhlp_file_copy(char *dfname, char *sfname)
{
  int res, m, t;
  FILE *ip;
  FILE *op;
  char buf[300];

  res = 0;
  ip = fopen(sfname, "rb");
  if (ip == NULL)
    return jhlp_error(1, sfname);
  op = fopen(dfname, "wb");
  if (op == NULL)
  { fclose(ip);
    res = jhlp_error(1, dfname);
  }
  m = 1;
  while (m > 0 && res == 0)
  { m = fread(buf, 1, 255, ip);
    if (m > 0)
    { t = fwrite(buf, 1, m, op);
      if (t != m)
        res = jhlp_error(3, dfname);
    }
  }
  fclose(op);
  fclose(ip);
  return res;
}

static int jhlp_atof(float *f, char *a)
{
  int m;
  char c;
  int state;  /* 0: before sign,                    */
              /* 1: afersign, before decimal-point, */
              /* 2: after decimal point.            */

  state = 0;
  *f = 0.0;
  for (m = 0; a[m] != '\0'; m++)
  { if (a[m] == ',')  /* Both comma and point is decimal-char */
      a[m] = '.';
    c = a[m];
    switch (state)
    { case 0:
        if (c != ' ')
        { if (c == '-')
            state = 1;
          else
          if (c == '.')
            state = 2;
          else
          if (isdigit(c))
            state = 1;
          else
            return 9;
         }
         break;
      case 1:
        if (!isdigit(c))
        { if (c == '.')
            state = 2;
          else
            return 9;
        }
        break;
      case 2:
        if (!isdigit(c))
          return 9;
        break;
    }
  }
  if (state == 0)
    *f = 0.0;
  else
    *f = atof(a);
  return 0;
}

static int jhlp_readln(FILE *fp)
{
  /* reads a line from the ascii-file fp. Return 0 if ok, -1:if eof.   */

  int res;

  if (fgets(jhlp_txt, JHLP_TXT_LEN, fp) != NULL)
  { jhlp_line_no++;
    res = 0;
  }
  else
    res = -1;
  return res;
}

static int jhlp_command(void)
{
  int res, m;

  res = -1;
  for (m = 0; res == -1; m++)
  { if (strcmp(jhlp_argv[0], jhlp_coms[m].name) == 0)
      res = jhlp_coms[m].value;
    else
    if (jhlp_coms[m].value == JHLP_COM_ERROR)
    { jhlp_error(1104, jhlp_argv[0]);
      break;
    }
  }
  return res;
}

static int jhlp_str_split(char *sourcetxt)
{
  /* Bruges til opsplitning af en linie <txt> i tokens. */

  int tilst;  /* 0: space, 1: text, 2:qoute             */
  char c;
  int t;
  char *txt;

  tilst = 0;
  jhlp_argc = 0;
  t = 0;
  strcpy(jhlp_argtxt, sourcetxt);
  txt = jhlp_argtxt;
  while (txt[t] != '\0' && txt[t] != '#')
  { c = txt[t];
    switch (tilst)
    { case 0:  /* space */
        if (!(isspace(c)))
        { if (c == '"')
          { t++;
            jhlp_argv[jhlp_argc] = &(txt[t]);
            tilst = 2;
          }
          else
          { jhlp_argv[jhlp_argc] = &(txt[t]);
            tilst = 1;
            t++;
          }
        }
        else
          t++;
        break;
      case 1:  /* text */
        if (isspace(c))
        { txt[t] = '\0';
          tilst = 0;
          jhlp_argc++;
          if (jhlp_argc >= JHLP_ARG_COUNT)
            return jhlp_error(1101, jhlp_empty);
          t++;
        }
        else
        if (c == '"')
        { txt[t] = '\0';
          tilst = 2;
          jhlp_argc++;
          t++;
          jhlp_argv[jhlp_argc] = &(txt[t]);
        }
        else
          t++;
        break;
      case 2:  /* text i qoute */
        if (txt[t] == '"')
        { txt[t] = '\0';
          jhlp_argc++;
          tilst = 0;
          if (jhlp_argc >= JHLP_ARG_COUNT)
            return jhlp_error(1101, jhlp_empty);
        }
        t++;
        break;
    }
  }
  if (txt[t] == '#')
    txt[t] = '\0';
  if (tilst == 1)
    jhlp_argc++;
  if (tilst == 2)
    return jhlp_error(1102, jhlp_empty);
  return jhlp_argc;
}

static int jhlp_f_head(char *name, int fid)
{
  int id, m;
  struct jhlp_head_desc *hdesc;

  /* Help-function to jhlp_find_head()*/
  id = fid;
  while (id != -1)
  { hdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
    if (strcmp(hdesc->name, name) == 0)
      return id;
    else
    { m = jhlp_f_head(name, hdesc->fchild_id);
      if (m != -1)
        return m;
    }
    id = jfm_next(id);
    if (id == fid)
      id = -1;
  }
  return id;
}

static int jhlp_find_head(char *name)
{
  /*returns id for the head with name=<name> (-1 if none). */

  return jhlp_f_head(name, jhlp_cid);
}

static int jhlp_fopen(int id)
{
  char fname[256];
  int res;
  struct jhlp_head_desc *hdesc;

  res = 0;
  jhlp_fclose(jhlp_ofp);
  jhlp_ofp = NULL;
  hdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
  strcpy(fname, jhlp_dest_dir);
  strcat(fname, hdesc->name);
  if (jhlp_gmode == JHLP_GM_HTML)
    strcat(fname, ".htm");
  else
    strcat(fname, ".tmp");
  if (hdesc->status == JHLP_HST_EMPTY)
    jhlp_ofp = fopen(fname, "w");
  else
    jhlp_ofp = fopen(fname, "a");
  if (jhlp_ofp == NULL)
    res = jhlp_error(1, fname);
  return res;
}
/****************************************************************************/
/* Functions to handling of the path-array:                                 */

static int jhlp_path_init(void)
{
  jhlp_path[0] = jhlp_cid;
  jhlp_path_c = 1;
  return jhlp_cid;
}

static int jhlp_path_next(void)
{
  int m, id, fundet;
  struct jhlp_head_desc *h1;

  id = -1;
  fundet = 0;
  if (jhlp_path_c <= 0)
    fundet = 1;
  if (fundet == 0)
  { m = jhlp_path_c - 1;
    h1 = (struct jhlp_head_desc *) jfm_data_adr(jhlp_path[m]);
    if (h1->fchild_id != -1)
    { jhlp_path[jhlp_path_c] = h1->fchild_id;
      if (jhlp_path_c < JHLP_PATH_SIZE)
      { jhlp_path_c++;
        id = h1->fchild_id;
        fundet = 1;
      }
      else
      { id = -1;
        jhlp_error(1118, jhlp_empty);
        return -1;
      }
    }
  }
  while (fundet == 0)
  { if (jhlp_path_c < 1)
      fundet = 1;  /* current == last */
    else
    { m = jhlp_path_c - 1;
      jhlp_path[m] = jfm_next(jhlp_path[m]);
      if (m > 0)
      { h1 = (struct jhlp_head_desc *) jfm_data_adr(jhlp_path[m - 1]);
        if (jhlp_path[m] != h1->fchild_id)
        { id = jhlp_path[m];
          fundet = 1;
        }
        else
         jhlp_path_c--;
      }
      else
      { if (jhlp_path[m] != jhlp_cid)
        { id = jhlp_path[m];
          fundet = 1;
        }
        else
          jhlp_path_c--;
      }
    }
  }
  return id;
}

static void jhlp_build_path(int tid)
{
  int id;

  id = jhlp_path_init();
  while (id != -1 && id != tid)
    id = jhlp_path_next();
}

static int jhlp_next_id(int id)
{
  int nid;

  jhlp_build_path(id);
  nid = jhlp_path_next();
  return nid;
}

static int jhlp_prev_id(int id)
{
  int tid, prev_id;

  tid = jhlp_path_init();
  if (id == tid)
    return -1; /* first */
  while (tid != id)
  { prev_id = tid;
    tid = jhlp_path_next();
  }
  return prev_id;
}

/**********************************************************************/
/* level 1 functions (functions called from jfh_convert) with sub-functions: */

static int jhlp_init(void)
{
  int res, id;
  struct jhlp_glt_desc gltdesc;
  struct jhlp_var_desc vdesc;

  res = 0;
  jhlp_fid = -1;
  jhlp_cid = -1;
  jhlp_iid = -1;
  jhlp_lid = -1;
  jhlp_vid = -1;

  gltdesc.fodata_id = -1;
  strcpy(gltdesc.name, "glosary");
  gltdesc.type = JHLP_DM_GLOS;
  gltdesc.option = 0;
  jhlp_gltid = jfm_pre_insert(-1, &gltdesc, JHLP_GLT_SIZE);
  res = jhlp_jfm_check(jhlp_gltid);
  if (res == 0)
  { strcpy(gltdesc.name, "text");
    gltdesc.type = JHLP_DM_INCLUDE;
    id = jfm_pre_insert(jhlp_gltid, &gltdesc, JHLP_GLT_SIZE);
    res = jhlp_jfm_check(id);
  }
  if (res == 0)
  { if (jhlp_gmode == JHLP_GM_TEXT)
    { strcpy(vdesc.name, "paper");
      vdesc.value[0] = '\0';
      id = jfm_pre_insert(jhlp_vid, &vdesc, JHLP_VAR_SIZE);
      res = jhlp_jfm_check(id);
      if (res == 0)
      { if (jhlp_vid == -1)
          jhlp_vid = id;
      }
    }
  }
  return res;
}

static int jhlp_load_jhi(char *jhc_fname)
{
  int res, id;
  char name[JHLP_MAX_PATH];
  FILE *fp;

  res = 0;
  if (jhlp_silent != 1)
    fprintf(jhlp_sout, "  Loading jhi-file: %s...\n", jhlp_jhi_fname);
  jhlp_set_filename(jhlp_jhi_fname);
  fp = fopen(jhlp_jhi_fname, "r");
  if (fp == NULL)
    jhlp_jhi_exists = 0;
  else
  { jhlp_jhi_exists = 1;
    while (res == 0 && jhlp_readln(fp) == 0)
    { if (jhlp_str_split(jhlp_txt) > 0)
      { id = jfm_pre_insert(jhlp_fid, jhlp_argv[0], strlen(jhlp_argv[0]) + 1);
        res = jhlp_jfm_check(id);
        if (res == 0)
        { if (jhlp_fid == -1)
            jhlp_fid = id;
        }
      }
    }
    fclose(fp);
  }
  jhlp_full_path(name, jhc_fname);
  jhlp_ext_subst(name, "jhc", 1);
  if (jhlp_licmp(name, jhlp_fid) == -1)
  { id = jfm_pre_insert(jhlp_fid, name, strlen(name) + 1);
    res = jhlp_jfm_check(id);
    if (res == 0)
    { if (jhlp_fid == -1)
        jhlp_fid = id;
    }
  }
  return res;
}

static int jhlp_save_jhi(void)
{
  FILE *fp;
  int id, res;
  char cp_fname[JHLP_MAX_PATH];

  res = 0;
  strcpy(cp_fname, jhlp_jhi_fname);
  jhlp_ext_subst(cp_fname, "old", 1);
  if (jhlp_jhi_exists == 1)
    res = jhlp_file_copy(cp_fname, jhlp_jhi_fname);
  if (res == 0)
  { if (jhlp_silent != 1)
      fprintf(jhlp_sout, "  Saving jhi-file: %s\n", jhlp_jhi_fname);
    fp = fopen(jhlp_jhi_fname, "w");
    noget_skrevet = 1;
    if (fp == NULL)
      return jhlp_error(1, jhlp_jhi_fname);
    id = jhlp_fid;
    while (id != -1)
    { fprintf(fp, "\"%s\"\n", (char *) jfm_data_adr(id));
      id = jfm_next(id);
      if (id == jhlp_fid)
        id = -1;
    }
    fclose(fp);
  }
  while (jhlp_fid != -1)
    jhlp_fid = jfm_delete(jhlp_fid);
  return res;
}

/**************************************************************************/
/* jhlp_create_contents:                                                  */

static int jhlp_head_insert(int fid, struct jhlp_head_desc *hdesc, int sorted)
{
  /* inserts hdesc in the list starting in <fid>. Returns the new  */
  /* starting-id in list. Sets jhlp_cur_head.                      */

  int rid, id, place, fundet, c;
  struct jhlp_head_desc *hp;
  char *txt;

  rid = fid;
  txt = (char *) jfm_data_adr(hdesc->txt_id);
  place = 0; /* before first */
  id = fid;
  if (sorted == 1)
    fundet = 0;
  else
  { fundet = 1;
    place = 2; /* after last */
  }
  while (id != -1 && fundet == 0)
  { hp = (struct jhlp_head_desc *) jfm_data_adr(id);
    c = strcmp(txt, (char *) jfm_data_adr(hp->txt_id));
    if (c < 0)
      fundet = 1;
    if (fundet == 0)
    { place = 1; /* after first */
      id = jfm_next(id);
      if (id == fid)
      { id = -1;
        place = 2; /* after last */
      }
    }
  }
  if (place == 0) /* before first */
  {  id = jfm_pre_insert(fid, hdesc, JHLP_HEAD_SIZE);
     jhlp_jfm_check(id);
     rid = id;
  }
  else
  if (place == 1) /* between first and last */
  { id = jfm_pre_insert(id, hdesc, JHLP_HEAD_SIZE);
    jhlp_jfm_check(id);
  }
  else /* after last */
  {  id = jfm_pre_insert(fid, hdesc, JHLP_HEAD_SIZE);
     jhlp_jfm_check(id);
     if (id != -1 && rid == -1)
       rid = id;
  }
  if (id >= 0)
    jhlp_cur_head_id = id;
  return rid;
}

static int jhlp_create_contents(char *name, char *pname, char *sname,
                               char *lname, int sorted, int nowrite)
{
  int id, pid, fid = 0, res, psorted;
  struct jhlp_head_desc *phead;
  struct jhlp_head_desc chead;

  res = 0;
  jhlp_set_length(name, 15);
  jhlp_set_length(pname, 15);
  jhlp_set_length(sname, 15);
  /* first check if the head is already declared: */
  id = jhlp_find_head(name);
  if (id >= 0)
    jhlp_cur_head_id = id;
  else
  { chead.fchild_id = -1;
    chead.fnote_id = -1;
    chead.txt_id = jfm_pre_insert(-1, lname, strlen(lname) + 1);
    jhlp_jfm_check(chead.txt_id);
    if (chead.txt_id < 0)
      res = -1;
    strcpy(chead.name, name);
    strcpy(chead.sname, sname);
    chead.flags = 0;
    if (sorted == 1)
      chead.flags += JHLP_HF_ORDERED;
    if (nowrite == 1)
      chead.flags += JHLP_HF_NOWRITE;
    chead.status = JHLP_HST_EMPTY;

    /* find parent-head */
    psorted = 0;
    if (strcmp(pname, "NULL") == 0)
    { pid = -1;
      fid = jhlp_cid;
    }
    else
    { pid = jhlp_find_head(pname);
      if (pid == -1)
        res = jhlp_error(1105, pname);
      else
      { phead = (struct jhlp_head_desc *) jfm_data_adr(pid);
        fid = phead->fchild_id;
        if (phead->flags & JHLP_HF_ORDERED)
          psorted = 1;
      }
    }
    if (res == 0)
    { id = jhlp_head_insert(fid, &chead, psorted); /* id:=new-first-child */
      if (id >= 0)
      { if (id != fid && pid != -1)
        { phead = (struct jhlp_head_desc *) jfm_data_adr(pid);
          phead->fchild_id = id;
        }
        if (jhlp_cid == -1)
          jhlp_cid = id;
      }
      else
        res = 1;
    }
    if (res == 0)
      jhlp_contents_c++;
  }
  return res;
}

/************************************************************************/
/* Help-functions to command-handling:                                  */

/* expr-handling :                                                      */

static int jhlp_negate(int ibool)
{
  int res;

  if (ibool == JHLP_IS_TRUE)
    res = JHLP_IS_FALSE;
  else
  if (ibool == JHLP_IS_FALSE)
    res = JHLP_IS_TRUE;
  else
    res = JHLP_IS_IFALSE;
  return res;
}

static void jhlp_cappend(char *s, int c, unsigned int ml)
{
  /* tilfoejer tegnet <c> til strengen <s> hvis s[ml] er en lovlig      */
  /* streng.                                                            */
  char a[2];

  if (strlen(s) + 1 < ml)
  { a[0] = c;
    a[1] = '\0';
    strcat(s, a);
  }
}

static int jhlp_cmp_op(int c1, int c2)
{
  int res;

  res = -6;
  switch (c1)
  { case '=':
      if (c2 == ' ' || c2 == '=')
        res = JHLP_EOP_EQ;
      break;
    case '>':
      if (c2 == ' ')
        res = JHLP_EOP_GT;
      else
      if (c2 == '=')
        res = JHLP_EOP_GEQ;
      break;
    case '<':
      if (c2 == ' ')
        res = JHLP_EOP_LT;
      else
      if (c2 == '=')
        res = JHLP_EOP_LEQ;
      break;
  }
  return res;
}

static int jhlp_find_var(char *name)
{
  int id, rid;
  struct jhlp_var_desc *vdesc;

  id = jhlp_vid;
  rid = -1;
  while (rid == -1 && id != -1)
  { vdesc = (struct jhlp_var_desc *) jfm_data_adr(id);
    if (strcmp(vdesc->name, name) == 0)
      rid = id;
    else
    { id = jfm_next(id);
      if (id == jhlp_vid)
        id = -1;
    }
  }
  return rid;
}

static int jhlp_expr_split(char *expr)
{
  int res, stop, m, c, cmp_c = 0, ctype, t;
  int state;  /* states:  0       2      4     7     11    */
              /*           [!] <name> [<op> <value>]       */
  res = 0;
  state = 0;
  stop = 0;
  m = 0;
  jhlp_expr.name[0] = '\0';
  jhlp_expr.value[0] = '\0';
  jhlp_expr.negate = 0;
  jhlp_expr.op = JHLP_EOP_NONE;

  while (res == 0 && stop == 0)
  { c = expr[m];
    if (c == '\0')
      ctype = JHLP_CT_EOL;
    else
    if (isspace(c))
      ctype = JHLP_CT_SPACE;
    else
    if (c == '!')
      ctype = JHLP_CT_NEGATE;
    else
    if (c == '=' || c == '>' || c == '<')
      ctype = JHLP_CT_CMP;
    else
      ctype = JHLP_CT_LETTER;

    switch(state)
    { case 0: /* before first letter */
        switch (ctype)
        { case JHLP_CT_NEGATE:
            jhlp_expr.negate = 1;
            state = 1;
            break;
          case JHLP_CT_LETTER:
            jhlp_cappend(jhlp_expr.name, c, 16);
            state = 2;
            break;
          case JHLP_CT_SPACE:
            break;
          default:
            res = 6;
            break;
        }
        break;
      case 1: /* after '!' : */
        switch (ctype)
        { case JHLP_CT_LETTER:
            jhlp_cappend(jhlp_expr.name, c, 16);
            state = 2;
            break;
          case JHLP_CT_SPACE:
            break;
          default:
            res = 6;
            break;
        }
        break;
      case 2: /* in <name>: */
        switch (ctype)
        { case JHLP_CT_LETTER:
            jhlp_cappend(jhlp_expr.name, c, 16);
            break;
          case JHLP_CT_SPACE:
            state = 3;
            break;
          case JHLP_CT_NEGATE:
            state = 8;
            break;
          case JHLP_CT_CMP:
            cmp_c = c;
            state = 4;
            break;
          case JHLP_CT_EOL:
            stop = 1;
            break;
          default:
            res = 6;
            break;
        }
        break;
      case 3: /* after <name> <space> */
        switch (ctype)
        { case JHLP_CT_CMP:
            cmp_c = c;
            state = 4;
            break;
          case JHLP_CT_SPACE:
            break;
          case JHLP_CT_EOL:
            stop = 1;
            break;
          default:
            res = 6;
            break;
        }
        break;
      case 4: /* after <name> <cmp> */
        switch (ctype)
        { case JHLP_CT_LETTER:
            jhlp_cappend(jhlp_expr.value, c, 16);
            state = 7;
            break;
          case JHLP_CT_SPACE:
          case JHLP_CT_CMP:
            t = jhlp_cmp_op(cmp_c, c);
            if (t >= 0)
              jhlp_expr.op = t;
            else
              res = 6;
            state = 5;
            break;
          case JHLP_CT_EOL:
            t = jhlp_cmp_op(cmp_c, ' ');
            if (t >= 0)
              jhlp_expr.op = t;
            else
              res = 6;
            stop = 1;
            break;
          default:
            res = 6;
            break;
        }
        break;
      case 5: /* after <name> <cmp> <space */
        switch (ctype)
        { case JHLP_CT_LETTER:
            jhlp_cappend(jhlp_expr.value, c, 16);
            state = 7;
            break;
          case JHLP_CT_SPACE:
            break;
          case JHLP_CT_EOL:
            stop = 1;
            break;
          default:
            res = 6;
            break;
        }
        break;
      case 7: /* in <value> */
        switch (ctype)
        { case JHLP_CT_LETTER:
            jhlp_cappend(jhlp_expr.value, c, 16);
            break;
          case JHLP_CT_SPACE:
          case JHLP_CT_EOL:
            stop = 1;
            break;
          default:
            res = 6;
            break;
        }
        break;
      case 8: /* after <name> ! */
        if (ctype == '=')
        { jhlp_expr.op = JHLP_EOP_NEQ;
          state = 7;
        }
        else
          res = 6;
        break;
    }
    m++;
  }
  if (jhlp_expr.negate == 1 && jhlp_expr.op != JHLP_EOP_NONE)
    res = 6;
  if (res != 0)
    jhlp_error(1108, expr);
  return res;
}

static int jhlp_expr_handle(char *expr)
{
  float f1, f2;
  int res, id, found, t;
  struct jhlp_var_desc vdesc;

  res = 0;
  if (jhlp_expr_split(expr) == 0)
  { id = jhlp_vid;
    found = 0;
    while (found == 0 && id != -1)
    { memcpy(&vdesc, jfm_data_adr(id), JHLP_VAR_SIZE);
      if (strcmp(vdesc.name, jhlp_expr.name) == 0)
        found = 1;
      else
      { id = jfm_next(id);
        if (id == jhlp_vid)
          id = -1;
      }
    }
    if (found == 0)
    { if (jhlp_expr.op == JHLP_EOP_NONE && jhlp_expr.negate == 1)
        res = 1;
    }
    else
    { t = -2;
      if (jhlp_atof(&f1, vdesc.value) == 0)
      { if (jhlp_atof(&f2, jhlp_expr.value) == 0)
        { if (f1 == f2)
            t = 0;
          else
          if (f1 > f2)
            t = 1;
          else
            t = -1;
        }
      }
      if (t == -2)  /* not numbers */
        t = strcmp(vdesc.value, jhlp_expr.value);
      switch (jhlp_expr.op)
      { case JHLP_EOP_NONE:
          res = 1;
          break;
        case JHLP_EOP_EQ:
          if (t == 0)
            res = 1;
          break;
        case JHLP_EOP_NEQ:
          if (t != 0)
            res = 1;
         break;
        case JHLP_EOP_GT:
          if (t == 1)
            res = 1;
          break;
        case JHLP_EOP_GEQ:
          if (t >= 0)
            res = 1;
          break;
        case JHLP_EOP_LT:
          if (t < 0)
            res = 1;
          break;
        case JHLP_EOP_LEQ:
          if (t <= 0)
            res = 1;
          break;
      }
      if (jhlp_expr.negate == 1)
      { if (res == 0)
          res = 1;
        else
          res = 0;
      }
    }
  }
  return res;
}

static void jhlp_dexpr_handle(char *dexpr)
{
  int id, found, r;
  struct jhlp_var_desc vdesc;

  if (jhlp_expr_split(dexpr) == 0)
  { if (jhlp_expr.negate == 1
        || (jhlp_expr.op != JHLP_EOP_EQ && jhlp_expr.op != JHLP_EOP_NONE))
      jhlp_error(1107, jhlp_expr.name);
    else
    { id = jhlp_vid;
      found = 0;
      while (found == 0 && id != -1)
      { memcpy(&vdesc, jfm_data_adr(id), JHLP_VAR_SIZE);
        if (strcmp(vdesc.name, jhlp_expr.name) == 0)
          found = 1;
        else
        { id = jfm_next(id);
          if (id == jhlp_vid)
            id = -1;
        }
      }
      if (found == 1)
      { strcpy(vdesc.value, jhlp_expr.value);
        r = jfm_update(id, &vdesc, JHLP_VAR_SIZE);
        jhlp_jfm_check(r);
      }
      else
      { strcpy(vdesc.name, jhlp_expr.name);
        strcpy(vdesc.value, jhlp_expr.value);
        id = jfm_pre_insert(jhlp_vid, &vdesc, JHLP_VAR_SIZE);
        if (jhlp_jfm_check(id) == 0)
        { if (jhlp_vid == -1)
            jhlp_vid = id;
        }
      }
    }
  }
}

/************************************************************************/
/* jhlp_write_body():                                                   */


static int jhlp_jhd_readln(void)
{
  /* reads the next jhd-line into jhlp_txt. Opens a new  jhd-file if needed. */
  int res;
  /* char txt[JHLP_MAX_PATH]; */

  res = 2;
  while (res == 2)
  { if (jhlp_jhi_fp == NULL)
    { jhlp_jhi_fp = fopen(jhlp_jhi_fname, "r");
      if (jhlp_jhi_fp == NULL)
        return jhlp_error(1, jhlp_jhi_fname);
    }
    else
    { if (jhlp_jhc_fp == NULL)
      { if (jhlp_readln(jhlp_jhi_fp) != 0)
        { res = 1; /* eofiles */
          jhlp_fclose(jhlp_jhi_fp);
          jhlp_jhi_fp = NULL;
        }
        else
        { if (jhlp_str_split(jhlp_txt) > 0)
          { strcpy(jhlp_jhc_fname, jhlp_argv[0]);
            jhlp_splitpath(jhlp_jhc_fname, jhlp_source_dir, NULL, NULL);
            if (jhlp_silent != 1)
              fprintf(jhlp_sout, "      %s\n", jhlp_jhc_fname);
            jhlp_jhc_fp = fopen(jhlp_jhc_fname, "r");
            if (jhlp_jhc_fp == NULL)
              return jhlp_error(1, jhlp_jhc_fname);
          }
        }
      }
      else /* jhlp_jhc_fp not NULL */
      { if (jhlp_ifp == NULL)
        { if (jhlp_readln(jhlp_jhc_fp) != 0)
          { jhlp_fclose(jhlp_jhc_fp);
            jhlp_jhc_fp = NULL;
          }
          else
          { if (jhlp_str_split(jhlp_txt) > 0)
            { if (jhlp_ext_cmp(jhlp_argv[0], ".jhd") == 0)
              { strcpy(jhlp_jhd_fname, jhlp_source_dir);
                strcat(jhlp_jhd_fname, jhlp_argv[0]);
                jhlp_ifp = fopen(jhlp_jhd_fname, "r");
                if (jhlp_ifp == NULL)
                  return jhlp_error(1, jhlp_jhd_fname);
                jhlp_set_filename(jhlp_jhd_fname);
                if (jhlp_silent != 1)
                  fprintf(jhlp_sout, "        %s\n",  jhlp_jhd_fname);
              }
            }
          }
        }
        else /* jhlp_ifp != NULL */
        { if (jhlp_readln(jhlp_ifp) != 0)
          { jhlp_fclose(jhlp_ifp);
            jhlp_ifp = NULL;
          }
          else
            res = 0;
        }
      }
    }
  }
  return res;
}

static int jhlp_write_line(void)
{
  int res, m, le, c, mode;
  char txt[JHLP_MAX_PATH];
  char otxt[JHLP_MAX_PATH];

  res = 0;
  otxt[0] = '\0';
  le = strlen(jhlp_txt);
  if (jhlp_html_mode == JHLP_HM_FIRST)
  { if (jhlp_txt[0] == '<')
      mode = JHLP_HM_ON; /* html-line */
    else
      mode = JHLP_HM_OFF;
  }
  else
    mode = jhlp_html_mode;
  for (m = 0; m < le; m++)
  { c = jhlp_txt[m];
    txt[0] = c;
    txt[1] = '\0';
    if (mode == JHLP_HM_OFF)
    { if (c == '<')
        strcpy(txt, "&lt;");
      else
      if (c == '>')
        strcpy(txt, "&gt;");
      else
      if (c == '&')
        strcpy(txt, "&amp;");
      else
      if (c == '"')
        strcpy(txt, "&quot;");
    }
    if (c == '%' && jhlp_txt[m + 1] == '%')
    { if (jhlp_copy_mode == 1)
        txt[0] = '\0';
      else
        jhlp_url(txt, jhlp_source_dir);
      m++;
    }
    else
    if (c == '\xE6' ||c == '\x91') /* ae */
      strcpy(txt, "&aelig;");
    else
    if (c == '\xF8' || c == '\x9B') /* oe */
      strcpy(txt, "&oslash;");
    else
    if (c == '\xE5' || c == '\x86')  /* aa */
      strcpy(txt, "&aring;");
    else
    if (c == '\xC6' || c == '\x92')  /* AE */
      strcpy(txt, "&Aelig;");
    else
    if (c == '\xD8' || c == '\x9D')  /* OE */
      strcpy(txt, "&Oeslash;");
    else
    if (c == '\xC5' || c == '\x8F')    /* AA */
      strcpy(txt, "&Aring;");
    else
    if (c == '\n' || c == '\r')
      txt[0] = '\0';
    strcat(otxt, txt);
  }
  fprintf(jhlp_ofp, "%s\n", otxt);
  return res;
}

static int jhlp_chap_no(char *chap_txt, int f_pid, int sid)
{
  int lcno, fundet, id, rm_end;
  struct jhlp_head_desc *hdesc;
  char ntxt[16];

  lcno = 1;
  fundet = 0;
  id = f_pid;
  while (id != -1 && fundet == 0)
  { rm_end = strlen(chap_txt);
    if (rm_end != 0)
      strcat(chap_txt, ".");
    sprintf(ntxt, "%d", lcno);
    strcat(chap_txt, ntxt);
    if (sid == id)
      fundet = 1;
    else
    { hdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
      fundet = jhlp_chap_no(chap_txt, hdesc->fchild_id, sid);
    }
    if (fundet == 0)
    { lcno++;
      chap_txt[rm_end] = '\0';
      id = jfm_next(id);
      if (id == f_pid)
        id = -1;
    }
  }
  return fundet;
}

static void jhlp_write_t_idline(int id, int label_id, char *text)
{
  char *t;
  struct jhlp_head_desc hdesc;
  struct jhlp_head_desc *thdesc;
  struct jhlp_label_desc *ldesc;
  char chap_txt[256];
  int med, tid;

  memcpy(&hdesc, jfm_data_adr(id), JHLP_HEAD_SIZE);
  if (text == NULL)
    t = (char *) jfm_data_adr(hdesc.txt_id);
  else
    t = text;
  if (jhlp_gmode == JHLP_GM_HTML)
  { if (label_id == -1)
      fprintf(jhlp_ofp, "<A HREF=\"%s.htm\">%s</A>\n",
                        hdesc.name, t);
    else
    { ldesc = (struct jhlp_label_desc *) jfm_data_adr(label_id);
      fprintf(jhlp_ofp, "<A HREF=\"%s.htm#%s\">%s</A>\n",
                        hdesc.name, ldesc->name, t);

    }
  }
  else
  { tid = jhlp_find_head(jhlp_prhead);
    thdesc = (struct jhlp_head_desc *) jfm_data_adr(tid);
    chap_txt[0] = '\0';
    if (id == tid)
      med = 1;
    else
      med = jhlp_chap_no(chap_txt, thdesc->fchild_id, id);
    if (med == 0)
      fprintf(jhlp_ofp, "<U>ONLINE:%s</U>", (const char *) jfm_data_adr(hdesc.txt_id));
    else
      fprintf(jhlp_ofp, "<U>%s: %s</U> ", chap_txt, (const char *) jfm_data_adr(hdesc.txt_id));
  }
}

static void jhlp_write_idline(int id, int label_id)
{
  jhlp_write_t_idline(id, label_id, NULL);
}

/*************************************************************************/
/* reduce-contents:                                                       */

static int jhlp_rred_contents(int fid)
{
  /* removes empty heads and subhead for all heads in the list */
  /* starting in <fid>. Returns (new) first id.                */
  int newfid, id, nid;
  struct jhlp_head_desc hdesc;

  newfid = fid;
  id = fid;
  while (id != -1)
  { memcpy(&hdesc, jfm_data_adr(id), JHLP_HEAD_SIZE);
    hdesc.fchild_id = jhlp_rred_contents(hdesc.fchild_id);
    if (hdesc.fchild_id == -1 && hdesc.status == JHLP_HST_EMPTY)
    { nid = jfm_delete(id);
      if (id == newfid) /* if first id in list is deleted */
      { newfid = nid;
        id = nid;
      }
      else
      { if (nid == newfid) /* last is deleted */
          id = -1;
        else
          id = nid;
      }
      jhlp_contents_c--;
    }
    else
    { hdesc.status = JHLP_HST_EMPTY;
      jfm_update(id, &hdesc, JHLP_HEAD_SIZE);
      id = jfm_next(id);
      if (id == newfid)
        id = -1;
    }
  }
  return newfid;
}

static int jhlp_reduce_contents(void)
{
  if (jhlp_silent != 1)
    fprintf(jhlp_sout, "  Reducing contents..\n");
  jhlp_cid = jhlp_rred_contents(jhlp_cid);
  return 0;
}

/************************************************************************/
/* write_contents:                                                       */

static void jhlp_contents_list(int hid)
{
  /* writes a list of index-lines for hid with sub-headers */

  int id, cid;
  struct jhlp_head_desc *hdesc;

  if (hid != -1)
  { id = hid;
    while (id != -1)
    { fprintf(jhlp_ofp, "<LI>");
      jhlp_write_idline(id, -1);
      hdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
      cid = hdesc->fchild_id;
      if (cid != -1)
      { fprintf(jhlp_ofp, "<UL>\n");
        jhlp_contents_list(cid);
        fprintf(jhlp_ofp, "</UL>\n");
      }
      id = jfm_next(id);
      if (id == hid)
        id = -1;
    }
  }
}

static int jhlp_write_contents(void)
{
  int res;
  char fname[256];

  res = 0;
  if (jhlp_contents_c <= 1 || jhlp_gmode == JHLP_GM_TEXT)
    return 0;
  strcpy(fname, jhlp_dest_dir);
  strcat(fname, "contents.htm");
  jhlp_ofp = fopen(fname, "w");
  if (jhlp_ofp == NULL)
    return jhlp_error(1, fname);

   /* write head */
  fprintf(jhlp_ofp, "%s\n", jhlp_h_bhtml);
  fprintf(jhlp_ofp, "%s\n", jhlp_h_bhead);
  fprintf(jhlp_ofp, "%sContents%s\n", jhlp_h_btitle, jhlp_h_etitle);
  if (strlen(jhlp_stylesheet) != 0)
#ifndef _WIN32
    fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"%s\">\n",
                      jhlp_stylesheet);
#else
    fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"/%s\">\n",
                      jhlp_stylesheet);
#endif
  fprintf(jhlp_ofp, "%s\n", jhlp_h_ehead);

  fprintf(jhlp_ofp, "%s\n", jhlp_h_bbody);
  fprintf(jhlp_ofp, "%sContents%s\n", jhlp_h_bh, jhlp_h_eh);

  fprintf(jhlp_ofp, "<UL>\n");
  jhlp_contents_list(jhlp_cid);
  if (jhlp_glosary_c > 0)
  { fprintf(jhlp_ofp, "<LI>");
    fprintf(jhlp_ofp, "<A HREF=\"glosary.htm\">Glosary</A>\n");
  }
  if (jhlp_index_c > 1)
  { fprintf(jhlp_ofp, "<LI>");
    fprintf(jhlp_ofp, "<A HREF=\"index.htm\">Index</A>\n");
  }
  fprintf(jhlp_ofp, "</UL>\n");
  fprintf(jhlp_ofp, "%s\n%s\n", jhlp_h_ebody, jhlp_h_ehtml);
  jhlp_fclose(jhlp_ofp);
  jhlp_ofp = NULL;
  return res;
}

static void jhlp_write_navigation(int hid, int invers)
{
  struct jhlp_head_desc *h2desc;
  int nid, m, n;

  for (n = 0; n < 2; n++)
  { if (n == 1)
      fprintf(jhlp_ofp, "<BR>\n");
    if ((n == 0 && invers == 0) || (n == 1 && invers == 1))
    { /* write navigation-line: */
      fprintf(jhlp_ofp, "Jump:");
      if (jhlp_contents_c > 1)
        fprintf(jhlp_ofp,
                "[<A HREF=\"contents.htm\" ACCESSKEY=\"c\">Contents</A>]\n");
      if (jhlp_index_c > 1)
        fprintf(jhlp_ofp, "[<A HREF=\"index.htm\" ACCESSKEY=\"i\">Index</A>]\n");
      nid = jhlp_prev_id(hid);
      if (nid != -1)
      { h2desc = (struct jhlp_head_desc *) jfm_data_adr(nid);
        fprintf(jhlp_ofp, "[<A HREF=\"%s.htm\" ACCESSKEY=\"p\">Prev</A>]\n",
                          h2desc->name);
      }
      nid = jhlp_next_id(hid);
      if (nid != -1)
      { h2desc = (struct jhlp_head_desc *) jfm_data_adr(nid);
        fprintf(jhlp_ofp, "[<A HREF=\"%s.htm\" ACCESSKEY=\"n\" >Next</A>]\n",
                 h2desc->name);
      }
    }
    else
    { /* write path-line: */
      if (hid != jhlp_cid)
      { jhlp_build_path(hid);
        if (jhlp_path_c > 1)
        { for (m = 0; m < jhlp_path_c ; m++)
          { h2desc = (struct jhlp_head_desc *) jfm_data_adr(jhlp_path[m]);
            if (m == 0)
              fprintf(jhlp_ofp, "Path: <TT>");
            else
              fprintf(jhlp_ofp, " \\ ");
            fprintf(jhlp_ofp, "<A HREF=\"%s.htm\">%s</A>\n",
                              h2desc->name, h2desc->sname);
          }
          fprintf(jhlp_ofp, "</TT>\n");
        }
      }
    }
  }
}

static void jhlp_write_bottom(int cid)
{
  struct jhlp_head_desc hdesc;
  struct jhlp_head_desc *chdesc;
  struct jhlp_note_desc *ndesc;
  int id;
  char *t;

  if (cid == -1)
    return;
  memcpy(&hdesc, jfm_data_adr(cid), JHLP_HEAD_SIZE);
  if (jhlp_fopen(cid) == 0)
  { if (jhlp_gmode == JHLP_GM_HTML)
    { fprintf(jhlp_ofp, "%s\n", jhlp_h_bpre);
      id = hdesc.fchild_id;
      while (id != -1)
      { chdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
        if ((chdesc->flags & JHLP_HF_NOWRITE) == 0)
          jhlp_write_idline(id, -1);
        id = jfm_next(id);
        if (id == hdesc.fchild_id)
          id = -1;
      }
      fprintf(jhlp_ofp, "%s\n", jhlp_h_epre);
    }
    /* write notes */
    if (hdesc.fnote_id != -1)
    { fprintf(jhlp_ofp, "<HR>\n");
      fprintf(jhlp_ofp, "<SMALL><B>Notes:</B><BR>");
      id = hdesc.fnote_id;
      while (id != -1)
      { ndesc = (struct jhlp_note_desc *) jfm_data_adr(id);
        t = (char *) jfm_data_adr(ndesc->fname_id);
        fprintf(jhlp_ofp, "<A HREF=\"%s\" TARGET=_blank>%s</A><BR>\n",
                            t, ndesc->txt);
        id = jfm_next(id);
        if (id == hdesc.fnote_id)
          id = -1;
      }
      fprintf(jhlp_ofp, "</SMALL>");
    }
    if (jhlp_gmode == JHLP_GM_HTML)
    { fprintf(jhlp_ofp, "<DIV CLASS=\"bnavigation\">\n");
      fprintf(jhlp_ofp, "<HR>\n");
      jhlp_write_navigation(cid, 1);
      fprintf(jhlp_ofp, "</DIV>\n");
      fprintf(jhlp_ofp, "%s\n%s\n", jhlp_h_ebody, jhlp_h_ehtml);
    }
    jhlp_fclose(jhlp_ofp);
    jhlp_ofp = NULL;
  }
}

static void jhlp_write_bottoms(int hid)
{
  /* writes bottom-information to all html-files */

  int id, cid;
  struct jhlp_head_desc *hdesc;

  if (hid != -1)
  { id = hid;
    while (id != -1)
    { jhlp_write_bottom(id);
      hdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
      cid = hdesc->fchild_id;
      jhlp_write_bottoms(cid);
      id = jfm_next(id);
      if (id == hid)
        id = -1;
    }
  }
}

static int jhlp_write_head(char *hname)
{
  struct jhlp_head_desc *hdesc;
  char *htxt;
  int res, id;

  jhlp_set_length(hname, 15);
  id = jhlp_find_head(hname);
  if (id == -1)
    return 0;  /* head fjernet af check-contents */

  jhlp_cur_head_id = id;
  res = jhlp_fopen(id);
  if (res != 0)
    return res;

  hdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
  htxt = (char *) jfm_data_adr(hdesc->txt_id);
  if (hdesc->status == JHLP_HST_WRITTEN)
     return 0;
  hdesc->status = JHLP_HST_WRITTEN;

  if (jhlp_gmode == JHLP_GM_TEXT)
    return 0;

  /* write head: */
  fprintf(jhlp_ofp, "%s\n", jhlp_h_bhtml);
  fprintf(jhlp_ofp, "%s\n", jhlp_h_bhead);
  fprintf(jhlp_ofp, "%s%s%s\n", jhlp_h_btitle, htxt, jhlp_h_etitle);
  if (strlen(jhlp_stylesheet) != 0)
#ifndef _WIN32
    fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"%s\">\n",
                      jhlp_stylesheet);
#else
    fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"/%s\">\n",
                      jhlp_stylesheet);
#endif
  fprintf(jhlp_ofp, "%s\n", jhlp_h_ehead);

  fprintf(jhlp_ofp, "%s\n", jhlp_h_bbody);
  fprintf(jhlp_ofp, "%s%s%s\n", jhlp_h_bh, htxt, jhlp_h_eh);

  fprintf(jhlp_ofp, "<DIV CLASS=\"navigation\">\n");

  jhlp_write_navigation(id, 0);
  fprintf(jhlp_ofp, "<HR>\n");
  fprintf(jhlp_ofp, "</DIV>\n");
  return res;
}

static int jhlp_find_glt(int glttype, char *name)
{
  int id;
  struct jhlp_glt_desc *gd;

  id = jhlp_gltid;
  while (id != -1)
  { gd = (struct jhlp_glt_desc *) jfm_data_adr(id);
    if (strcmp(gd->name, name) == 0 && gd->type == glttype)
      break;
    id = jfm_next(id);
    if (id == jhlp_gltid)
      id = -1;
  }
  return id;
}

static int jhlp_find_odata(int gltid, char *name)
{
  int id, rid;
  struct jhlp_glt_desc *gltdesc;
  struct jhlp_odata_desc *odesc;

  rid = -1;
  gltdesc = (struct jhlp_glt_desc *) jfm_data_adr(gltid);
  id = gltdesc->fodata_id;
  while (rid == -1 && id != -1)
  { odesc = (struct jhlp_odata_desc *) jfm_data_adr(id);
    if (strcmp((char *) jfm_data_adr(odesc->name_id), name) == 0)
      rid = id;
    id = jfm_next(id);
    if (id == gltdesc->fodata_id)
      id = -1;
  }
  return rid;
}

static int jhlp_dglt(int type, char *name, char *lname, int sorted)
{
  /* define a glt (!dtab, !dlist eller !dglos */
  int id, res;
  struct jhlp_glt_desc gltdesc;
  struct jhlp_head_desc *hdesc;

  res = 0;
  jhlp_set_length(name, 15);
  id = jhlp_find_glt(type, name);
  if (id >= 0)
    res = jhlp_error(1119, name);

  if (res == 0)
  { strcpy(gltdesc.name, name);
    gltdesc.txt_id = -1;
    if (lname != NULL)
    { gltdesc.txt_id = jfm_pre_insert(-1, lname, strlen(lname) + 1);
      res = jhlp_jfm_check(gltdesc.txt_id);
    }
    gltdesc.fodata_id = -1;
    gltdesc.type = type;
    gltdesc.option = sorted;
    if (res == 0)
    { id = jfm_pre_insert(jhlp_gltid, &gltdesc, JHLP_GLT_SIZE);
      res = jhlp_jfm_check(id);
      if (res == 0)
      { if (jhlp_gltid == -1)
          jhlp_gltid = id;
        if (jhlp_cur_head_id == -1)
          res = jhlp_error(1120, jhlp_empty);
        if (res == 0)
        { hdesc = (struct jhlp_head_desc *) jfm_data_adr(jhlp_cur_head_id);
          hdesc->status = JHLP_HST_WRITTEN; /* Boer rettes, saa kun glt */
                                            /* med data skrives.        */
        }
      }
    }
  }
  return res;
}

static int jhlp_odata_ins(int glt_id, char *name, char *vers_name)
{
  int fid, sorted, res, id, place, fundet, c;
  struct jhlp_glt_desc gltdesc;
  struct jhlp_odata_desc odesc;
  struct jhlp_odata_desc *od;

  memcpy(&gltdesc, jfm_data_adr(glt_id), JHLP_GLT_SIZE);
  if (gltdesc.type == JHLP_DM_GLOS)
    sorted = 1;
  else
    sorted = gltdesc.option;
  fid = gltdesc.fodata_id;
  odesc.fdata_id = -1;
  odesc.name_id = jfm_pre_insert(-1, name, strlen(name) + 1);
  res = jhlp_jfm_check(odesc.name_id);
  if (res != 0)
    return res;
  odesc.vers_id = jfm_pre_insert(-1, vers_name, strlen(vers_name) + 1);
  res = jhlp_jfm_check(odesc.vers_id);
  if (res != 0)
    return res;

  place = 0; /* before first */
  id = fid;
  if (sorted == 1)
    fundet = 0;
  else
  { fundet = 1;
    if (gltdesc.fodata_id == -1)
      place = 0; /* before first */
    else
      place = 2; /* after last */
  }
  while (id != -1 && fundet == 0)
  { od = (struct jhlp_odata_desc *) jfm_data_adr(id);
    c = jhlp_stricmp(name, (char *) jfm_data_adr(od->name_id));
    if (c < 0)
      fundet = 1;
    if (fundet == 0)
    { place = 1; /* after first */
      id = jfm_next(id);
      if (id == fid)
      { id = -1;
        place = 2; /* after last */
      }
    }
  }
  if (place == 0) /* before first */
  {  id = jfm_pre_insert(fid, &odesc, JHLP_ODATA_SIZE);
     if (jhlp_jfm_check(id) == 0)
     { gltdesc.fodata_id = id;
       jfm_update(glt_id, &gltdesc, JHLP_GLT_SIZE);
     }
  }
  else
  if (place == 1) /* between first and last */
  { id = jfm_pre_insert(id, &odesc, JHLP_ODATA_SIZE);
    jhlp_jfm_check(id);
  }
  else /* after last */
  {  id = jfm_pre_insert(fid, &odesc, JHLP_ODATA_SIZE);
     jhlp_jfm_check(id);
  }
  return id;
}

static int jhlp_vers_cmp(char *vers_name, signed short oid)
{
  struct jhlp_odata_desc *odesc;
  int vers_ok, t;
  signed short id;
  float f1, f2;
  char *gl_name;

  vers_ok = 0;
  if (strcmp(vers_name, "append") == 0)
    vers_ok = 1;
  else
  { odesc = (struct jhlp_odata_desc *) jfm_data_adr(oid);
    gl_name = (char *) jfm_data_adr(odesc->vers_id);
    t = -2;
    if (jhlp_atof(&f1, vers_name) == 0)
    { if (jhlp_atof(&f2, gl_name) == 0)
      { if (f1 == f2)
          t = 0;
        else
        if (f1 > f2)
          t = 1;
        else
          t = -1;
      }
    }
    if (t == -2)  /* not numbers */
      t = strcmp(vers_name, gl_name);
   if (t == 1)
     vers_ok = 1;
   if (vers_ok)  /* fjern indhold af odata */
   {  id = odesc->fdata_id;
      while (id != -1)
        id = jfm_delete(id);
     odesc = (struct jhlp_odata_desc *) jfm_data_adr(oid);
     odesc->fdata_id = -1;
   }
  }
  return vers_ok;
}

static int jhlp_start_glt(int glttype)
{
  int res, oid;
  char *glt_name;
  char *oid_name;
  char *vers_name;

  res = 0;
  if (jhlp_dmode != JHLP_DM_TEXT)
    return jhlp_error(1122, jhlp_empty);
  if (glttype == JHLP_DM_INCLUDE)
  { glt_name = jhlp_t_text;
    oid_name = jhlp_argv[1];
    if (jhlp_argc == 3)
      vers_name = jhlp_argv[2];
    else
      vers_name = jhlp_t_nul;
  }
  else
  { glt_name = jhlp_argv[1];
    oid_name = jhlp_argv[2];
    if (jhlp_argc == 4)
      vers_name = jhlp_argv[3];
    else
      vers_name = jhlp_t_nul;
  }
  jhlp_set_length(glt_name, 15);
  jhlp_cur_glt_id = jhlp_find_glt(glttype, glt_name);
  jhlp_dmode = glttype;
  if (jhlp_cur_glt_id == -1)
    return jhlp_error(1121, glt_name);
  oid = jhlp_find_odata(jhlp_cur_glt_id, oid_name);
  jhlp_vers_ok = 1;
  if (jhlp_fase == 1)
  { if (oid == -1)
      oid = jhlp_odata_ins(jhlp_cur_glt_id, oid_name, vers_name);
    else
      jhlp_vers_ok = jhlp_vers_cmp(vers_name, oid);
  }
  jhlp_cur_od_id = oid;
  if (glttype == JHLP_DM_GLOS && strcmp(glt_name, "glosary") == 0
      && jhlp_fase == 1)
    jhlp_glosary_c++;
  return res;
}

static int jhlp_etc(void)
{
  int oid;
  struct jhlp_odata_desc *odesc;
  char txt[JHLP_MAX_PATH];
  char vers_txt[JHLP_MAX_PATH];

  if (jhlp_dmode != JHLP_DM_TABLE)
    return jhlp_error(1123, jhlp_empty);
  if (jhlp_cur_od_id != -1)
  { odesc = (struct jhlp_odata_desc *) jfm_data_adr(jhlp_cur_od_id);
    strcpy(txt, (char *) jfm_data_adr(odesc->name_id));
    strcpy(vers_txt, (char *) jfm_data_adr(odesc->vers_id));
    if (jhlp_fase == 1)
      oid = jhlp_odata_ins(jhlp_cur_glt_id, txt, vers_txt);
    else
      oid = jfm_next(jhlp_cur_od_id);  
    if (oid != -1)
      jhlp_cur_od_id = oid;
   }
   return 0;
}

static int jhlp_end_glt(int type)
{
  if (jhlp_dmode != type)
    return jhlp_error(1124, jhlp_argv[0]);
  jhlp_cur_od_id = -1;
  jhlp_cur_glt_id = -1;
  jhlp_dmode = JHLP_DM_TEXT;
  return 0;
}

static int jhlp_write_table(char *name)
{
  int res, id, odid, did, first, cur_rowid;
  struct jhlp_glt_desc *gdesc;
  struct jhlp_odata_desc *odesc;

  res = 0;
  jhlp_set_length(name, 15);
  id = jhlp_find_glt(JHLP_DM_TABLE, name);
  gdesc = (struct jhlp_glt_desc *) jfm_data_adr(id);
  fprintf(jhlp_ofp, "<TABLE BORDER>\n");
  if (gdesc->txt_id != -1)
    fprintf(jhlp_ofp, "<CAPTION><STRONG>%s</STRONG></CAPTION>\n",
                      (char *) jfm_data_adr(gdesc->txt_id));
  first = 1;
  odid = gdesc->fodata_id;
  while (odid != -1)
  { odesc = (struct jhlp_odata_desc *) jfm_data_adr(odid);
    if (first == 1)
    { cur_rowid = odesc->name_id;
      first = 0;
      fprintf(jhlp_ofp, "<TR>\n");
    }
    if (strcmp((char *) jfm_data_adr(cur_rowid),
               (char *) jfm_data_adr(odesc->name_id)) != 0)
    { /* new row: */
      cur_rowid = odesc->name_id;
      fprintf(jhlp_ofp, "</TR>\n<TR>\n");
      fprintf(jhlp_ofp, "<TD NOWRAP>\n");
    }
    else
      fprintf(jhlp_ofp, "<TD>\n");
    did = odesc->fdata_id;
    while (did != -1)
    { strcpy(jhlp_txt, (char *) jfm_data_adr(did));
      jhlp_handle_line();
      did = jfm_next(did);
      if (did == odesc->fdata_id)
        did = -1;
    }
    fprintf(jhlp_ofp, "</TD>\n");
    odid = jfm_next(odid);
    if (odid == gdesc->fodata_id)
      odid = -1;
  }
  if (first == 0)
    fprintf(jhlp_ofp, "</TR>\n");
  fprintf(jhlp_ofp, "</TABLE>\n");
  return res;
}

static int jhlp_write_list(char *name)
{
  int res, id, odid, did;
  struct jhlp_glt_desc *gdesc;
  struct jhlp_odata_desc *odesc;

  res = 0;
  jhlp_set_length(name, 15);
  id = jhlp_find_glt(JHLP_DM_LIST, name);
  if (id == -1)
    return id;
  gdesc = (struct jhlp_glt_desc *) jfm_data_adr(id);
  fprintf(jhlp_ofp, "<UL>");
  odid = gdesc->fodata_id;
  while (odid != -1)
  { odesc = (struct jhlp_odata_desc *) jfm_data_adr(odid);
    fprintf(jhlp_ofp, "<LI>\n");

    did = odesc->fdata_id;
    while (did != -1)
    { strcpy(jhlp_txt, (char *) jfm_data_adr(did));
      jhlp_handle_line();
      did = jfm_next(did);
      if (did == odesc->fdata_id)
        did = -1;
    }
    fprintf(jhlp_ofp, "</LI>\n");
    odid = jfm_next(odid);
    if (odid == gdesc->fodata_id)
      odid = -1;
  }
  fprintf(jhlp_ofp, "</UL>\n");
  return res;
}

static int jhlp_write_glosary(const char *_name)
{
  int res, id, odid, did;
  struct jhlp_glt_desc *gdesc;
  struct jhlp_odata_desc *odesc;
  char *name;

  name = (char *)(malloc (strlen(_name) + 1));
  if (name != NULL) strcpy(name, _name);
  res = 0;
  jhlp_set_length(name, 15);
  id = jhlp_find_glt(JHLP_DM_GLOS, name);
  if (id == -1)
  {
    free(name);
    return -1;
  }
  gdesc = (struct jhlp_glt_desc *) jfm_data_adr(id);
  fprintf(jhlp_ofp, "<DL>\n");

  odid = gdesc->fodata_id;
  while (odid != -1)
  { odesc = (struct jhlp_odata_desc *) jfm_data_adr(odid);
    fprintf(jhlp_ofp, "<DT>\n");
    fprintf(jhlp_ofp, "%s", (char *) jfm_data_adr(odesc->name_id));

    did = odesc->fdata_id;
    fprintf(jhlp_ofp, "<DD>\n");
    while (did != -1)
    { strcpy(jhlp_txt, (char *) jfm_data_adr(did));
      jhlp_handle_line();
      did = jfm_next(did);
      if (did == odesc->fdata_id)
        did = -1;
    }
    odid = jfm_next(odid);
    if (odid == gdesc->fodata_id)
      odid = -1;
  }
  fprintf(jhlp_ofp, "</DL>\n");
  free(name);
  return res;
}

static int jhlp_write_include(const char *_name)
{
  int res, id, odid, did, skrevet;
  struct jhlp_glt_desc *gdesc;
  struct jhlp_odata_desc *odesc;
  char *name;

  name = (char *)(malloc (strlen(_name) + 1));
  if (name != NULL) strcpy(name, _name);
  res = 0;
  jhlp_set_length(name, 15);
  id = jhlp_find_glt(JHLP_DM_INCLUDE, jhlp_t_text);
  if (id == -1)
  {
    free (name);
    return id;
  }
  gdesc = (struct jhlp_glt_desc *) jfm_data_adr(id);
  odid = gdesc->fodata_id;
  skrevet = 0;
  while (skrevet == 0 && odid != -1)
  { odesc = (struct jhlp_odata_desc *) jfm_data_adr(odid);
    if (strcmp((char *) jfm_data_adr(odesc->name_id), name) == 0)
    { skrevet = 1;
      did = odesc->fdata_id;
      while (did != -1)
      { strcpy(jhlp_txt, (char *) jfm_data_adr(did));
        jhlp_handle_line();
        did = jfm_next(did);
        if (did == odesc->fdata_id)
          did = -1;
      }
    }
    else
    { odid = jfm_next(odid);
      if (odid == gdesc->fodata_id)
        odid = -1;
    }
    if (skrevet == 0)
      res = jhlp_error(1126, name);
  }
  free (name);
  return res;
}

static int jhlp_find_label(int hid, const char *name)
{
  int rid, id;
  struct jhlp_label_desc *ld;

  rid = -1;
  id = jhlp_lid;
  while (rid == -1 && id != -1)
  { ld = (struct jhlp_label_desc *) jfm_data_adr(id);
    if (hid == ld->head_id && strcmp(name, ld->name) == 0)
      rid = id;
    id = jfm_next(id);
    if (id == jhlp_lid)
      id = -1;
  }
  return rid;
}

static int jhlp_lab(void)
{
  int id;
  struct jhlp_label_desc ldesc;
  struct jhlp_label_desc *ld;

  if (jhlp_cur_head_id == -1)
    return jhlp_error(1120, jhlp_argv[0]);
  jhlp_set_length(jhlp_argv[1], 15);
  id = jhlp_find_label(jhlp_cur_head_id, jhlp_argv[1]);
  if (jhlp_fase == 1)
  { if (id != -1)
      return jhlp_error(1125, jhlp_argv[1]);
    ldesc.head_id = jhlp_cur_head_id;
    strcpy(ldesc.name, jhlp_argv[1]);
    id = jfm_pre_insert(jhlp_lid, &ldesc, JHLP_LABEL_SIZE);
    if (jhlp_jfm_check(id) == 0)
    { if (jhlp_lid == -1)
        jhlp_lid = id;
    }
  }
  else
  { if (id != -1)
    { ld = (struct jhlp_label_desc *) jfm_data_adr(id);
      fprintf(jhlp_ofp, "<A NAME=\"%s\"</A>\n", ld->name);
    }
  }
  return 0;
}

static int jhlp_insert_index(void)
{
  int res, id, place, fundet, c;
  struct jhlp_index_desc idesc;
  struct jhlp_index_desc *ip;
  char txt[JHLP_MAX_PATH];

  res = 0;
  idesc.head_id = jhlp_cur_head_id;
  idesc.label_id = -1;
  strcpy(txt, jhlp_argv[1]);
  txt[0] = toupper(txt[0]);
  idesc.txt_id = jfm_pre_insert(-1, txt, strlen(txt) + 1);
  res = jhlp_jfm_check(idesc.txt_id);
  if (res == 0 && jhlp_argc == 3)
  { idesc.label_id = jhlp_find_label(jhlp_cur_head_id, jhlp_argv[2]);
    if (idesc.label_id == -1)
      res = jhlp_error(1113, jhlp_argv[2]);
  }
  if (res == 0)
  { place = 0; /* before first */
    id = jhlp_iid;
    fundet = 0;
    while (id != -1 && fundet == 0)
    { ip = (struct jhlp_index_desc *) jfm_data_adr(id);
      c = jhlp_stricmp(txt, (char *) jfm_data_adr(ip->txt_id));
      if (c < 0)
        fundet = 1;
      if (fundet == 0)
      { place = 1; /* after first */
        id = jfm_next(id);
        if (id == jhlp_iid)
        { id = -1;
          place = 2; /* after last */
        }
      }
    }
    if (place == 0) /* before first */
    {  id = jfm_pre_insert(jhlp_iid, &idesc, JHLP_INDEX_SIZE);
       res = jhlp_jfm_check(id);
       jhlp_iid = id;
    }
    else
    if (place == 1) /* between first and last */
    { id = jfm_pre_insert(id, &idesc, JHLP_INDEX_SIZE);
      res = jhlp_jfm_check(id);
    }
    else /* after last */
    {  id = jfm_pre_insert(jhlp_iid, &idesc, JHLP_INDEX_SIZE);
       res = jhlp_jfm_check(id);
    }
  }
  jhlp_index_c++;
  return res;
}

static int jhlp_handle_command(void)
{
  int res, id, nid, lab_id, it, e, command, sorted, tabel, m, nowrite;
  char *p;
  char *tref;

  res = 0;
  if (jhlp_top_ifstack < 0)
    it = 1;
  else
  { if (jhlp_ifstack[jhlp_top_ifstack] == JHLP_IS_TRUE)
      it = 1;
    else
      it = 0;
  }
  command = jhlp_command();
  switch (command)
  { case JHLP_COM_HEAD:
      if (jhlp_argc < 5)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
      { sorted = 0;
        nowrite = 0;
        for (m = 5; m < jhlp_argc; m++)
        { if (strcmp(jhlp_argv[m], jhlp_t_sorted) == 0)
            sorted = 1;
          else
          if (strcmp(jhlp_argv[m], jhlp_t_nowrite) == 0)
            nowrite = 1;
          else
            jhlp_error(1109, jhlp_argv[5]);
        }
        if (jhlp_fase == 1)
          res = jhlp_create_contents(jhlp_argv[1], jhlp_argv[2], jhlp_argv[3],
                                    jhlp_argv[4], sorted, nowrite);
        else
          res = jhlp_write_head(jhlp_argv[1]);
      }
      break;
    case JHLP_COM_CHEAD:
      if (jhlp_argc < 7)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
      { sorted = 0;
        nowrite = 1;
        for (m = 7; m < jhlp_argc; m++)
        { if (strcmp(jhlp_argv[m], jhlp_t_sorted) == 0)
            sorted = 1;
          else
          if (strcmp(jhlp_argv[m], jhlp_t_nowrite) == 0)
            nowrite = 0;
          else
            jhlp_error(1109, jhlp_argv[7]);
        }
        e = jhlp_expr_handle(jhlp_argv[2]);
        if (e == 1)
          p = jhlp_argv[3];  /* p1-id */
        else
          p = jhlp_argv[4];  /* p2-id */
        if (jhlp_fase == 1)
          res = jhlp_create_contents(jhlp_argv[1], p, jhlp_argv[5],
                                    jhlp_argv[6], sorted, nowrite);
        else
          res = jhlp_write_head(jhlp_argv[1]);
      }
      break;
    case JHLP_COM_DTAB:
      if (jhlp_argc < 3 || jhlp_argc > 4)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
      { sorted = 0;
        if (jhlp_argc == 4)
        { if (strcmp(jhlp_argv[3], jhlp_t_sorted) == 0)
            sorted = 1;
          else
            jhlp_error(1110, jhlp_argv[3]);
        }
        if (jhlp_fase == 1)
          res = jhlp_dglt(JHLP_DM_TABLE, jhlp_argv[1], jhlp_argv[2], sorted);
        else
          res = jhlp_write_table(jhlp_argv[1]);
      }
      break;
    case JHLP_COM_TAB:
      if (jhlp_argc < 3 || jhlp_argc > 4)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_start_glt(JHLP_DM_TABLE);
      break;
    case JHLP_COM_ETC:
      if (jhlp_argc != 1)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_etc();
      break;
    case JHLP_COM_ETAB:
      if (jhlp_argc != 1)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_end_glt(JHLP_DM_TABLE);
      break;
    case JHLP_COM_DLIST:
      if (jhlp_argc < 2 || jhlp_argc > 3)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
      { sorted = 0;
        if (jhlp_argc == 3)
        { if (strcmp(jhlp_argv[2], jhlp_t_sorted) == 0)
            sorted = 1;
          else
            jhlp_error(1111, jhlp_argv[2]);
        }
        if (jhlp_fase == 1)
          res = jhlp_dglt(JHLP_DM_LIST, jhlp_argv[1], NULL, sorted);
        else
          res = jhlp_write_list(jhlp_argv[1]);
      }
      break;
    case JHLP_COM_LI:
      if (jhlp_argc < 3 || jhlp_argc > 4)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_start_glt(JHLP_DM_LIST);
      break;
    case JHLP_COM_ELI:
      if (jhlp_argc != 1)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_end_glt(JHLP_DM_LIST);
      break;
    case JHLP_COM_DGLOS:
      if (jhlp_argc < 2 || jhlp_argc > 3)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
      { tabel = 0;
        if (jhlp_argc == 3)
        { if (strcmp(jhlp_argv[2], "table") == 0)
            tabel = 1;
          else
            res = jhlp_error(1112, jhlp_argv[2]);
        }
        if (jhlp_fase == 1)
          res = jhlp_dglt(JHLP_DM_GLOS, jhlp_argv[1], NULL, tabel);
        else
          res = jhlp_write_glosary(jhlp_argv[1]);
      }
      break;
    case JHLP_COM_GLOS:
      if (jhlp_argc < 3 || jhlp_argc > 4)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_start_glt(JHLP_DM_GLOS);
      break;
    case JHLP_COM_EGLOS:
      if (jhlp_argc != 1)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_end_glt(JHLP_DM_GLOS);
      break;

    case JHLP_COM_TEXT:
      if (jhlp_argc < 2 || jhlp_argc > 3)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_start_glt(JHLP_DM_INCLUDE);
      break;
    case JHLP_COM_ETEXT:
      if (jhlp_argc != 1)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_end_glt(JHLP_DM_INCLUDE);
      break;
    case JHLP_COM_INCLUDE:
      if (jhlp_argc != 2)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
      { if (jhlp_fase == 1)
           res = jhlp_handle_txtline();
        else
          res = jhlp_write_include(jhlp_argv[1]);
      }
      break;
    case JHLP_COM_LAB:
      if (jhlp_argc != 2)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        res = jhlp_lab();
      break;
    case JHLP_COM_REF:
      if (jhlp_argc < 2 || jhlp_argc > 4)
        res = jhlp_error(1103, jhlp_argv[0]);
      tref = NULL;
      jhlp_set_length(jhlp_argv[1], 15);
      if (res == 0 && it == 1)
      { if (jhlp_fase == 1)
        { res = jhlp_handle_txtline();
        }
        else
        { id = jhlp_find_head(jhlp_argv[1]);
          if (id == -1)
            res = jhlp_error(1105, jhlp_argv[1]);
          if (res == 0)
          { lab_id = -1;
            if (jhlp_argc == 3)  /* format: !ref <ident> <text> */
            { tref = jhlp_argv[2];
            }
            if (jhlp_argc == 4)  /* format: !ref <ident> <label> <text> */
            { jhlp_set_length(jhlp_argv[2], 15);
              lab_id = jhlp_find_label(id, jhlp_argv[2]);
              if (lab_id == -1)
                res = jhlp_error(1113, jhlp_argv[2]);
              tref = jhlp_argv[3];
            }
          }
          if (res == 0 && jhlp_dmode == JHLP_DM_TEXT)
            jhlp_write_t_idline(id, lab_id, tref);
        }
      }
      break;
    case JHLP_COM_IND:
      if (jhlp_argc < 2 || jhlp_argc > 3)
        res = jhlp_error(1103, jhlp_argv[0]);
      else
      if (it == 1 && jhlp_fase == 1)
        res = jhlp_insert_index();
      break;
    case JHLP_COM_IF:
      if (jhlp_argc != 2)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0)
      { if (it == 1)
          it = jhlp_expr_handle(jhlp_argv[1]);
        else
          it = JHLP_IS_IFALSE;
        if (jhlp_top_ifstack + 1 >= JHLP_IFSTACK_SIZE)
          res = jhlp_error(1115, jhlp_empty);
        else
        { jhlp_top_ifstack++;
          jhlp_ifstack[jhlp_top_ifstack] = it;
        }
      }
      break;
    case JHLP_COM_ELSE:
      if (jhlp_argc != 1)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (jhlp_top_ifstack < 0)
        res = jhlp_error(1116, jhlp_empty);
      if (res == 0)
      { jhlp_ifstack[jhlp_top_ifstack]
            = jhlp_negate(jhlp_ifstack[jhlp_top_ifstack]);
      }
      break;
    case JHLP_COM_ENDIF:
      if (jhlp_argc != 1)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (jhlp_top_ifstack < 0)
        res = jhlp_error(1117, jhlp_empty);
      if (res == 0)
        jhlp_top_ifstack--;
      break;
    case JHLP_COM_DEFINE:
      if (jhlp_argc != 2)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
        jhlp_dexpr_handle(jhlp_argv[1]);
      break;
    case JHLP_COM_UNDEF:
      if (jhlp_argc != 2)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0 && it == 1)
      { id = jhlp_find_var(jhlp_argv[1]);
        if (id != -1)
        { nid = jfm_delete(id);
          if (jhlp_vid == id)
            jhlp_vid = nid;
        }
      }      
      break;
    case JHLP_COM_HTML:
      if (jhlp_argc != 2)
        res = jhlp_error(1103, jhlp_argv[0]);
      if (res == 0)
      { if (strcmp(jhlp_argv[1], "off") == 0)
          jhlp_html_mode = JHLP_HM_OFF;
        else
        if (strcmp(jhlp_argv[1], "on") == 0)
          jhlp_html_mode = JHLP_HM_ON;
        else
        if (strcmp(jhlp_argv[1], "first") == 0)
          jhlp_html_mode = JHLP_HM_FIRST;
        else
          res = jhlp_error(1127, jhlp_argv[1]);
      }
      break;
  }
  return res;
}

static int jhlp_handle_txtline(void)
{
  int res, id;
  struct jhlp_head_desc *hdesc;
  struct jhlp_odata_desc oddesc;

  res = 0;
  if (jhlp_top_ifstack >= 0)
  { if (jhlp_ifstack[jhlp_top_ifstack] != JHLP_IS_TRUE)
      return 0;
  }
  if (jhlp_cur_head_id == -1)
    return jhlp_error(1120, jhlp_txt);
  hdesc = (struct jhlp_head_desc *) jfm_data_adr(jhlp_cur_head_id);
  if (jhlp_fase == 1)
  { switch (jhlp_dmode)
    { case JHLP_DM_TEXT:
        hdesc->status = JHLP_HST_WRITTEN;
        break;
      case JHLP_DM_GLOS:
      case JHLP_DM_TABLE:
      case JHLP_DM_LIST:
      case JHLP_DM_INCLUDE:
        if (jhlp_vers_ok == 1 &&
              jhlp_cur_od_id != -1) /* only == -1 in error-situations */
        { memcpy(&oddesc, jfm_data_adr(jhlp_cur_od_id), JHLP_ODATA_SIZE);
          id = jfm_pre_insert(oddesc.fdata_id, jhlp_txt, strlen(jhlp_txt) + 1);
          if (oddesc.fdata_id == -1)
          { oddesc.fdata_id = id;
            jfm_update(jhlp_cur_od_id, &oddesc, JHLP_ODATA_SIZE);
          }
        }
        break;
    }
  }
  else /* fase == 2 */
  { switch(jhlp_dmode)
    { case JHLP_DM_TEXT:
        res = jhlp_write_line();
        break;
      case JHLP_DM_GLOS:
      case JHLP_DM_TABLE:
      case JHLP_DM_LIST:
      case JHLP_DM_INCLUDE:
        break;
    }
  }
  return res;
}

static int jhlp_handle_line(void)
{
  int res;

  if (jhlp_txt[0] == '!')
  { jhlp_str_split(jhlp_txt);
    res = jhlp_handle_command();
  }
  else
    res = jhlp_handle_txtline();
  return res;
}

static int jhlp_jhc_handle(void)
{
  int e, res;

  res = 0;
  if (jhlp_silent != 1)
    fprintf(jhlp_sout, "    Handling jhd-files...\n");
  jhlp_top_ifstack = -1;
  jhlp_ifp = jhlp_ofp = NULL;
  jhlp_jhi_fp = jhlp_jhc_fp = NULL;
  e = jhlp_jhd_readln();
  while (res == 0 && e == 0)
  { res = jhlp_handle_line();
    e = jhlp_jhd_readln();
  }
  if (jhlp_ofp != NULL)
  { jhlp_fclose(jhlp_ofp);
    jhlp_ofp = NULL;
  }
  if (res == 0 && jhlp_fase == 2)
    jhlp_write_bottoms(jhlp_cid);
  return res;
}

static int jhlp_jhp_handle(void)
{
  int res;
  FILE *jhp_fp;
  /* char txt[JHLP_MAX_PATH]; */
  char jhp_fname[JHLP_MAX_PATH];
  char dname[JHLP_MAX_PATH];
  char sname[JHLP_MAX_PATH];

  res = 0;
  while (jhlp_vid != -1)
    jhlp_vid = jfm_delete(jhlp_vid);
  if (jhlp_silent != 1)
    fprintf(jhlp_sout, "    Handling jhp-files..\n");
  jhlp_jhi_fp = fopen(jhlp_jhi_fname, "r");
  if (jhlp_jhi_fp == NULL)
    return jhlp_error(1, jhlp_jhi_fname);

  /* for each jhc-file in the jhi-file :*/
  while (res == 0 && jhlp_readln(jhlp_jhi_fp) == 0)
  { if (jhlp_str_split(jhlp_txt) > 0)
    { strcpy(jhlp_jhc_fname, jhlp_argv[0]);
      jhlp_splitpath(jhlp_jhc_fname, jhlp_source_dir, NULL, NULL);
      if (jhlp_silent != 1)
        fprintf(jhlp_sout, "      %s\n", jhlp_jhc_fname);
      jhlp_jhc_fp = fopen(jhlp_jhc_fname, "r");
      if (jhlp_jhc_fp == NULL)
         return jhlp_error(1, jhlp_jhc_fname);
      /* for each jhp-file in the jhc-file: */
      while (res == 0 && jhlp_readln(jhlp_jhc_fp) == 0)
      { if (jhlp_str_split(jhlp_txt) > 0)
        { if (jhlp_ext_cmp(jhlp_argv[0], ".jhp") == 0)
          { strcpy(jhp_fname, jhlp_source_dir);
            strcat(jhp_fname, jhlp_argv[0]);
            if (jhlp_silent != 1)
              fprintf(jhlp_sout, "        %s\n", jhp_fname);
            jhp_fp = fopen(jhp_fname, "r");
            if (jhp_fp == NULL)
              return jhlp_error(1, jhp_fname);
            jhlp_set_filename(jhp_fname);
            while (res == 0 && jhlp_readln(jhp_fp) == 0)
            { if (jhlp_str_split(jhlp_txt) > 0)
                jhlp_dexpr_handle(jhlp_argv[0]);
            }
            jhlp_fclose(jhp_fp);
          }
          else
          { if (jhlp_ext_cmp(jhlp_argv[0], ".jhd") != 0
                && jhlp_ext_cmp(jhlp_argv[0], ".jhn") != 0)
            { if (jhlp_copy_mode == 1 && jhlp_fase == 2)
              { /* copy image-files etc. Not perfect, copy even if file
                   is not needed by generated html */
                strcpy(sname, jhlp_source_dir);
                strcat(sname, jhlp_argv[0]);
                if (jhlp_gmode == JHLP_GM_TEXT)
                  strcpy(dname, jhlp_defile_dir);
                else
                  strcpy(dname, jhlp_dest_dir);
                strcat(dname, jhlp_argv[0]);
                jhlp_file_copy(dname, sname);
              }
            }
          }
        }
      }
      jhlp_fclose(jhlp_jhc_fp);
      jhlp_jhc_fp = NULL;
    }
  }
  jhlp_fclose(jhlp_jhi_fp);
  jhlp_jhi_fp = NULL;
  return res;
}

static int jhlp_handle(void)
{
  int res;

  jhlp_ifp = jhlp_ofp = NULL;
  jhlp_html_mode = JHLP_HM_FIRST;
  res = jhlp_jhp_handle();
  if (res == 0)
    res = jhlp_jhc_handle();
  return res;
}

static int jhlp_ins_note(void)
{
  int res, fnid, nid, hid;
  struct jhlp_note_desc ndesc;
  struct jhlp_head_desc *hdesc;

  res = 0;
  if (jhlp_argc != 3)
    return jhlp_error(7, jhlp_argv[0]);
  hid = jhlp_find_head(jhlp_argv[0]);
  if (hid < 0)
    return jhlp_error(1105, jhlp_argv[0]);
  hdesc = (struct jhlp_head_desc *) jfm_data_adr(hid);
  fnid = hdesc->fnote_id;
  ndesc.fname_id = jfm_pre_insert(-1, jhlp_argv[2], strlen(jhlp_argv[2]) + 1);
  jhlp_jfm_check(ndesc.fname_id);
  if (ndesc.fname_id < 0)
    return 1;
  jhlp_set_length(jhlp_argv[1], 78);
  strcpy(ndesc.txt, jhlp_argv[1]);
  nid = jfm_pre_insert(fnid, &ndesc, JHLP_NOTE_SIZE);
  jhlp_jfm_check(nid);
  if (fnid == -1)
  { hdesc = (struct jhlp_head_desc *) jfm_data_adr(hid);
    hdesc->fnote_id = nid;
  }
  return res;
}

static int jhlp_read_notes(void)
{
  int res;
  FILE *jhn_fp;
  /* char txt[JHLP_MAX_PATH]; */
  char jhn_fname[JHLP_MAX_PATH];

  res = 0;
  if (jhlp_silent != 1)
    fprintf(jhlp_sout, "    Reading notes..\n");
  jhlp_jhi_fp = fopen(jhlp_jhi_fname, "r");
  if (jhlp_jhi_fp == NULL)
    return jhlp_error(1, jhlp_jhi_fname);

  /* for each jhc-file in the jhi-file :*/
  while (res == 0 && jhlp_readln(jhlp_jhi_fp) == 0)
  { if (jhlp_str_split(jhlp_txt) > 0)
    { strcpy(jhlp_jhc_fname, jhlp_argv[0]);
      jhlp_splitpath(jhlp_jhc_fname, jhlp_source_dir, NULL, NULL);
      if (jhlp_silent != 1)
        fprintf(jhlp_sout, "      %s\n", jhlp_jhc_fname);
      jhlp_jhc_fp = fopen(jhlp_jhc_fname, "r");
      if (jhlp_jhc_fp == NULL)
         return jhlp_error(1, jhlp_jhc_fname);
      /* for each jhn-file in the jhc-file: */
      while (res == 0 && jhlp_readln(jhlp_jhc_fp) == 0)
      { if (jhlp_str_split(jhlp_txt) > 0)
        { if (jhlp_ext_cmp(jhlp_argv[0], ".jhn") == 0)
          { strcpy(jhn_fname, jhlp_source_dir);
            strcat(jhn_fname, jhlp_argv[0]);
            if (jhlp_silent != 1)
              fprintf(jhlp_sout, "        %s\n", jhn_fname);
            jhn_fp = fopen(jhn_fname, "r");
            if (jhn_fp == NULL)
              return jhlp_error(1, jhn_fname);
            jhlp_set_filename(jhn_fname);
            while (res == 0 && jhlp_readln(jhn_fp) == 0)
            { if (jhlp_str_split(jhlp_txt) > 0)
                jhlp_ins_note();
            }
            jhlp_fclose(jhn_fp);
          }
        }
      }
      jhlp_fclose(jhlp_jhc_fp);
      jhlp_jhc_fp = NULL;
    }
  }
  jhlp_fclose(jhlp_jhi_fp);
  jhlp_jhi_fp = NULL;
  return res;
}

static int jhlp_write_index(void)
{
  int res, m, cur_letter, id;
  char *dt;
  char fname[256];
  char txt[16];
  struct jhlp_head_desc *hdesc;
  struct jhlp_index_desc *idesc;
  struct jhlp_label_desc *labdesc;

  res = 0;
  if (jhlp_index_c <= 1 || jhlp_gmode == JHLP_GM_TEXT)
     return 0;
  strcpy(fname, jhlp_dest_dir);
  strcat(fname, "index.htm");
  jhlp_ofp = fopen(fname, "w");
  if (jhlp_ofp == NULL)
    return jhlp_error(1, fname);

   /* write head */
  fprintf(jhlp_ofp, "%s\n", jhlp_h_bhtml);
  fprintf(jhlp_ofp, "%s\n", jhlp_h_bhead);
  fprintf(jhlp_ofp, "%sIndex%s\n", jhlp_h_btitle, jhlp_h_etitle);
  if (strlen(jhlp_stylesheet) != 0)
#ifndef _WIN32
    fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"%s\">\n",
                      jhlp_stylesheet);
#else
    fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"/%s\">\n",
                      jhlp_stylesheet);
#endif
  fprintf(jhlp_ofp, "%s\n", jhlp_h_ehead);

  fprintf(jhlp_ofp, "%s\n", jhlp_h_bbody);
  fprintf(jhlp_ofp, "%sIndex%s\n", jhlp_h_bh, jhlp_h_eh);
  txt[1] = '\0';
  for (m = 'A'; m <= 'Z'; m++)
  { txt[0] = m;
    fprintf(jhlp_ofp, "<A HREF=\"#%s\">%s</A>\n", txt, txt);
  }
  fprintf(jhlp_ofp, "<HR>\n");
  fprintf(jhlp_ofp, "<DL>\n");
  id = jhlp_iid;
  cur_letter = 'A' - 1;
  while (id != -1)
  { idesc = (struct jhlp_index_desc *) jfm_data_adr(id);
    hdesc = (struct jhlp_head_desc *) jfm_data_adr(idesc->head_id);
    dt = (char *) jfm_data_adr(idesc->txt_id);
    if (dt[0] > cur_letter)
    { fprintf(jhlp_ofp, "</DL>\n");
      while (cur_letter < dt[0])
      { cur_letter++;
        txt[0] = cur_letter;
        if (cur_letter < dt[0])
          fprintf(jhlp_ofp, "<A NAME=\"%s\"></A>\n", txt);
        else
        { fprintf(jhlp_ofp, "<H2><A NAME=\"%s\">%s</A></H2>\n", txt, txt);
          fprintf(jhlp_ofp, "<DL>\n");
        }
      }
    }

    if (jhlp_gmode == JHLP_GM_HTML)
    { if (idesc->label_id == -1)
        fprintf(jhlp_ofp, "<DT><A HREF=\"%s.htm\">%s</A>\n",
                          hdesc->name, dt);
      else
      { labdesc = (struct jhlp_label_desc *) jfm_data_adr(idesc->label_id);
        fprintf(jhlp_ofp, "<DT><A HREF=\"%s.htm#%s\">%s</A>\n",
                          hdesc->name, labdesc->name, dt);
      }
    }
    id = jfm_next(id);
    if (id == jhlp_iid)
      id = -1;
  }
  fprintf(jhlp_ofp, "</DL>\n");
  while (cur_letter < 'Z')
  { cur_letter++;
    txt[0] = cur_letter;
    fprintf(jhlp_ofp, "<A NAME=\"%s\"></A>\n", txt);
  }

  fprintf(jhlp_ofp, "%s\n%s\n", jhlp_h_ebody, jhlp_h_ehtml);
  jhlp_fclose(jhlp_ofp);
  jhlp_ofp = NULL;
  return res;
}

static int jhlp_write_glob_glosary(void)
{
  int res;
  char fname[JHLP_MAX_PATH];

  res = 0;
  if (jhlp_glosary_c == 0 || jhlp_gmode == JHLP_GM_TEXT)
    return 0;
  strcpy(fname, jhlp_dest_dir);
  strcat(fname, "glosary.htm");
  jhlp_ofp = fopen(fname, "w");
  if (jhlp_ofp == NULL)
    return jhlp_error(1, fname);

   /* write head */
  fprintf(jhlp_ofp, "%s\n", jhlp_h_bhtml);
  fprintf(jhlp_ofp, "%s\n", jhlp_h_bhead);
  fprintf(jhlp_ofp, "%sGlosary%s\n", jhlp_h_btitle, jhlp_h_etitle);
  if (strlen(jhlp_stylesheet) != 0)
#ifndef _WIN32
    fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"%s\">\n",
                      jhlp_stylesheet);
#else
    fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"/%s\">\n",
                      jhlp_stylesheet);
#endif
  fprintf(jhlp_ofp, "%s\n", jhlp_h_ehead);

  fprintf(jhlp_ofp, "%s\n", jhlp_h_bbody);
  fprintf(jhlp_ofp, "%sGlosary%s\n", jhlp_h_bh, jhlp_h_eh);

  jhlp_write_glosary("glosary");

  fprintf(jhlp_ofp, "%s\n%s\n", jhlp_h_ebody, jhlp_h_ehtml);
  jhlp_fclose(jhlp_ofp);
  jhlp_ofp = NULL;
  return res;
}

static void jhlp_prlist(int fid, int prhid, int write, int lev, char *chap)
{
  int id, w, c, clev;
  FILE *ip;
  char fname[JHLP_MAX_PATH];
  char kap[256];
  char txt[16];
  struct jhlp_head_desc *hdesc;

  id = fid;
  c = 0;
  while (id != -1)
  { hdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
    strcpy(kap, chap);
    clev = lev;
    w = write;
    if (id == prhid || write == 1)
    { strcpy(fname, jhlp_dest_dir);
      strcat(fname, hdesc->name);
      strcat(fname, ".tmp");
      ip = fopen(fname, "r");
      if (ip == NULL)
        jhlp_error(1, fname);
      else
      { c++;
        if (clev != 0)
        { sprintf(txt, "%d", c);
          if (strlen(kap) != 0)
          { strcat(kap, ".");
            strcat(kap, txt);
            txt[0] = '\0';
          }
          else
          { strcat(kap, txt);
            txt[0] = '.';
            txt[1] = '\0';
          }
        }
        if (clev < 5)
          clev++;
        fprintf(jhlp_ofp, "<H%d>%s%s  %s</H%d>\n",
                          clev, kap, txt,
                          (char *) jfm_data_adr(hdesc->txt_id), clev);
        while (jhlp_readln(ip) == 0)
          fprintf(jhlp_ofp, "%s", jhlp_txt);
        fclose(ip);
      }
      w = 1;
    }
    jhlp_prlist(hdesc->fchild_id, prhid, w, clev, kap);
    id = jfm_next(id);
    if (id == fid)
      id = -1;
  }
}

static void jhlp_del_tmp(int fid)
{
  int id;
  struct jhlp_head_desc *hdesc;
  char fname[JHLP_MAX_PATH];

  id = fid;
  while (id != -1)
  { hdesc = (struct jhlp_head_desc *) jfm_data_adr(id);
    strcpy(fname, jhlp_dest_dir);
    strcat(fname, hdesc->name);
    strcat(fname, ".tmp");
    remove(fname);
    jhlp_del_tmp(hdesc->fchild_id);
    id = jfm_next(id);
    if (id == fid)
      id = -1;
  }
}

static int jhlp_write_prtext(void)
{
  int res, hid;
  struct jhlp_head_desc *prhead;
  char fname[JHLP_MAX_PATH];
  char empty[10];

  empty[0] = '\0';
  res = 0;
  hid = jhlp_find_head(jhlp_prhead);
  if (hid == -1)
    res = jhlp_error(1105, jhlp_prhead);
  if (res == 0)
  { prhead = (struct jhlp_head_desc *) jfm_data_adr(hid);
    strcpy(fname, jhlp_de_fname);
    jhlp_ofp = fopen(jhlp_de_fname, "w");
    if (jhlp_ofp == NULL)
      return jhlp_error(1, jhlp_de_fname);

    /* write head */
    fprintf(jhlp_ofp, "%s\n", jhlp_h_bhtml);
    fprintf(jhlp_ofp, "%s\n", jhlp_h_bhead);
    fprintf(jhlp_ofp, "%s%s%s\n", jhlp_h_btitle,
                     (char *) jfm_data_adr(prhead->txt_id),jhlp_h_etitle);
    if (strlen(jhlp_stylesheet) != 0)
#ifndef _WIN32
      fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"%s\">\n",
                      jhlp_stylesheet);
#else
      fprintf(jhlp_ofp, "<LINK REL=STYLESHEET TYPE=TEXT/CSS HREF=\"/%s\">\n",
                      jhlp_stylesheet);
#endif
    fprintf(jhlp_ofp, "%s\n", jhlp_h_ehead);

    fprintf(jhlp_ofp, "%s\n", jhlp_h_bbody);

    jhlp_prlist(jhlp_cid, hid, 0, 0, empty);

    fprintf(jhlp_ofp, "%s\n%s\n", jhlp_h_ebody, jhlp_h_ehtml);
    jhlp_fclose(jhlp_ofp);
    jhlp_ofp = NULL;
  }
  jhlp_del_tmp(jhlp_cid);
  return res;
}

/*************************************************************************/
/* Main-function:                                                        */
/*************************************************************************/

int jhlp_convert(char *de_fname, char *jhi_fname, char *jhc_fname,
                 char *stylesheet, char *head_name,
                 char *sout_fname, int append_mode, int copy_mode,
                 long nodes, long mem, int silent)
{
  int res;
  int m;
  /* char txt[JHLP_MAX_PATH]; */
  char name[JHLP_MAX_PATH];
  long inodes, imem;

  res = 0;
  noget_skrevet = 0;
  jhlp_sout = stdout;
  if (strlen(sout_fname) != 0)
  { if (append_mode == 0)
      jhlp_sout = fopen(sout_fname, "w");
    else
      jhlp_sout = fopen(sout_fname, "a");
    if (jhlp_sout == NULL)
    { printf("Cannot open file %s.\n", sout_fname);
      return 1;
    }
  }
  if (strlen(head_name) != 0)
  { jhlp_gmode = JHLP_GM_TEXT;
    strcpy(jhlp_prhead, head_name);
    if (strlen(de_fname) != 0)
      strcpy(jhlp_de_fname, de_fname);
    else
      strcpy(jhlp_de_fname, "jfspr.htm");
    jhlp_full_path(name, jhlp_de_fname);
    jhlp_splitpath(name, jhlp_defile_dir, NULL, NULL);
  }

  strcpy(jhlp_jhi_fname, jhi_fname);
  jhlp_ext_subst(jhlp_jhi_fname, "jhi", 1);

  jhlp_full_path(name, jhlp_jhi_fname);
  jhlp_splitpath(name, jhlp_dest_dir, NULL, NULL);

  if (strlen(stylesheet) > 0)
    jhlp_full_path(jhlp_stylesheet, stylesheet);

  imem = mem; inodes = nodes;
  if (imem <= 0)
    imem = 204800;
  if (inodes <= 0)
    inodes = 10000;
  jhlp_copy_mode = copy_mode;
  jhlp_silent = silent;
  if (jhlp_silent != 1)
    fprintf(jhlp_sout, "  Allocating memory. %ld nodes, %ld K\n",
                       inodes, imem / 1024);
  m = jfm_create(inodes, imem);
  if (m != 0)
  { jhlp_error(6, "jfm-data structure");
    res = 1;
  }
  else
  { res = jhlp_init();
    if (res == 0 && strlen(jhc_fname) != 0)
    {  res = jhlp_load_jhi(jhc_fname);
       if (res == 0)
         res = jhlp_save_jhi();
    }
    jhlp_fase = 1;
    if (jhlp_silent != 1)
      fprintf(jhlp_sout, "  Creating contents..\n");
    if (res == 0)
      res = jhlp_handle();
    if (jhlp_error_count > 0)
    { if (noget_skrevet == 1)
        res = 2;
      else
        res = 1;
    }
    if (res == 0)
      res = jhlp_reduce_contents();
    if (res == 0)
      res = jhlp_write_contents();
    if (res == 0)
      jhlp_read_notes();
    jhlp_fase = 2;
    if (jhlp_silent != 1)
      fprintf(jhlp_sout, "  Writing html-files..\n");
    if (res == 0)
      res = jhlp_handle();
    if (res == 0)
      res = jhlp_write_glob_glosary();
    if (res == 0)
      res = jhlp_write_index();
    if (jhlp_sout != stdout)
    { jhlp_fclose(jhlp_sout);
      jhlp_sout = NULL;
    }
    if (res == 0 && jhlp_gmode == JHLP_GM_TEXT)
      res = jhlp_write_prtext();

    jfm_free();
  }
  if (res == 0 && jhlp_error_count > 0)
  { if (noget_skrevet)
      res = 2;
    else
      res = 1;
  }
  return res;
}



