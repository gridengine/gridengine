
struct uti_state_t {
   /* sge_prog.c */
   char        *sge_formal_prog_name;      /* taken from prognames[] */
   char        *qualified_hostname;
   char        *unqualified_hostname;
   u_long32    who;                        /* Qxxx defines         QUSERDEFINED  */
   u_long32    uid;
   u_long32    gid;
   int         daemonized;
   char        *user_name;
   char        *default_cell;
   sge_exit_func_t exit_func;
   int         exit_on_error;
};


#if defined(SGE_MT)
void uti_state_init(struct uti_state_t* state);
#else
extern struct uti_state_t *uti_state;
#endif
