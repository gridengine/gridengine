/*******************************************************************
* Copyright © 2002 Sun Microsystems, Inc.  All rights reserved.
* Use is subject to license terms.
*
* This program scans an exacct file and prints the 
* information about processes associated with the given
* taskid.  
* If no task id is given, the current taskid is used.
* If no exacct file is given, /usr/adm/exacct/proc is used.
* This program skips processes that spent less than 0.5 seconds
* of CPU time.
*
* This program was create to be used in the epilog script of
* a Sun[TM] One Grid Engine queue, but can be used as a standalone tool.
*
* To compile this program use: cc -o proclist proclist.c -lexacct
*
* Paulo Tiberio M. Bulhoes, 
* Sun Microsystems, Inc.
* October 2002.
*******************************************************************/
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/task.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <exacct.h>
#include <strings.h>


#define PROC_FILE "/usr/adm/exacct/proc"
#define TASK_FILE "/usr/adm/exacct/task"
#define FILE_CREATOR "SunOS"
#define TIME_LEN 128

/*
 * Program to print a summary at the end of a Sun One Grid Engine job Flags :
 * -t TASKID -f exacct file
 */

static void     dump_records(ea_file_t * ea, taskid_t taskid, FILE * out);
static void     dump_process(ea_object_t * obp, taskid_t taskid, FILE * out);

int
main(int argc, char *argv[], char *envp[])
{
	taskid_t        taskid = (-1);
	char           *fname = (char *) NULL;
	char           *default_fname = PROC_FILE;
	ea_file_t       eaf;
	int             cmd_line_error = 0;
	extern char    *optarg;
	int             opt;

	/** Parse command line flags **/
	while ((opt = getopt(argc, argv, "t:f:")) != EOF)
		switch (opt) {
		case 't':
			if (taskid != (-1))
				cmd_line_error++;
			else {
				if ((taskid = atol(optarg)) == 0)
					cmd_line_error++;
			};
			break;
		case 'f':
			if (fname != (char *) NULL)
				cmd_line_error++;
			else
				fname = optarg;
			break;
		case '?':
			cmd_line_error++;
		};
	if (cmd_line_error) {
		(void) fprintf(stderr,
		     "usage: %s [-t taskid] [-f <exacct file>]\n", argv[0]);
		exit(2);
	}
	if (taskid == (-1)) {
		/* Task id not given, use the current one */
		taskid = gettaskid();
	};
	if (fname == (char *) NULL) {
		/* No file given, use the default file name */
		fname = default_fname;
	};

	/* Open the exacct file */

	if (ea_open(&eaf, fname, FILE_CREATOR, EO_HEAD, O_RDONLY, 0) != 0) {
		(void) fprintf(stderr,
			       "Error opening exacct file %s.\n", fname);
		exit(2);
	}

	/* File is open, now look for processes associated with taskid */
	/*	fprintf(stdout,"File:%s\nCreator:%s\nHostname:%s\n", 
		fname, ea_get_creator(&eaf), ea_get_hostname(&eaf)); */
	dump_records(&eaf, taskid, (FILE *) stdout);

	(void) ea_close(&eaf);
	exit(0);
}

static void
dump_records(ea_file_t * ea, taskid_t taskid, FILE * out)
{
	ea_object_t    *obp;
	int             record_count = 0;

	/* fprintf(out, "Dumping records for taskid %d\n", taskid); */
	/* Print Header */
	(void)fprintf(out,
		"Start Time        Finish Time       Command      ProjID   TaskID       ucpu     scpu wstatus\n");

	/* Loop over the records in the file */
	while ((obp = ea_get_object_tree(ea, 1)) != (ea_object_t *) NULL) {
		record_count++;	/* This is just for statistics */
		if ((obp->eo_type == EO_GROUP) &&
		    ((obp->eo_catalog & EXD_DATA_MASK) == EXD_GROUP_PROC)) {
			/* We are only interested in processes */
			dump_process(obp, taskid, out);
		};
		ea_free_object(obp, EUP_ALLOC);

	};

	/* If we get here, ea_get_object_tree failed */
	if (ea_error() == EXR_EOF) {
		/* printf("End of file reached, number of records read=%d\n", record_count); */
		return;
	} else {
		(void)fprintf(stderr, "Found a bad record, aborting\n");
		exit(2);
	}
}

/*
	This routine loops over the items on a process group,
	gathers the information we are looking for and 
	prints it  
*/
	
static void
dump_process(ea_object_t * obj, taskid_t taskid, FILE * out)
{
	int             found_task = 0;
	projid_t        projid = 0;
	taskid_t        task = 0;
	static char     start_time[TIME_LEN];
	static char     finish_time[TIME_LEN];
	static char     wstat_buf[TIME_LEN];
	static char     cmd[PATH_MAX];
	uint32_t        wstat = 0;
	time_t          otime = 0;
	double          ucpu = 0.0;
	double          ucpuns = 0.0;
	double          scpu = 0.0;
	double          scpuns = 0.0;
	ea_object_t    *tmp;

	if ((obj->eo_catalog & EXD_DATA_MASK) != EXD_GROUP_PROC) {
		(void)fprintf(stderr, "dump_process: object is not a process group!\n");
		abort();
	}

	tmp = obj->eo_group.eg_objs;
	for (tmp = obj->eo_group.eg_objs;
	     tmp != (ea_object_t *) NULL;
	     tmp = tmp->eo_next) {
		switch (tmp->eo_catalog & EXD_DATA_MASK) {
		case EXD_PROC_TASKID:
			task = tmp->eo_item.ei_uint32;
			found_task = 1;
			if (task != taskid) {
				return;	/* Ignore other tasks */
			};
			break;
		case EXD_PROC_PROJID:
			projid = tmp->eo_item.ei_uint32;
			break;
		case EXD_PROC_COMMAND:
			(void)strncpy(cmd, tmp->eo_item.ei_string, PATH_MAX);
			break;
		case EXD_PROC_START_SEC:
			otime = (time_t) tmp->eo_item.ei_uint64;
			(void) ascftime(start_time, "%D %T", localtime(&otime));

			break;
		case EXD_PROC_FINISH_SEC:
			otime = tmp->eo_item.ei_uint64;
			(void) ascftime(finish_time, "%D %T", localtime(&otime));
			break;
		case EXD_PROC_CPU_USER_SEC:
			ucpu = ((double) tmp->eo_item.ei_uint64);
			break;
		case EXD_PROC_CPU_USER_NSEC:
			ucpuns = ((double) tmp->eo_item.ei_uint64) * 1.0e-9;
			break;
		case EXD_PROC_CPU_SYS_SEC:
			scpu = ((double) tmp->eo_item.ei_uint64);
			break;
		case EXD_PROC_CPU_SYS_NSEC:
			scpuns = ((double) tmp->eo_item.ei_uint64) * 1.0e-9;
			break;
		case EXD_PROC_WAIT_STATUS:
			wstat = tmp->eo_item.ei_uint32;

			if (WIFEXITED(wstat))
				(void) sprintf(wstat_buf, "0x%-08x exit", WEXITSTATUS(wstat));
			else if (WIFSIGNALED(wstat))
				(void) sprintf(wstat_buf, "0x%08x signal", WTERMSIG(wstat));
			else
				(void) sprintf(wstat_buf, "%d", wstat);
			break;
		default:;
		}
	}
	if (!found_task)
		return;

	/*
	 * Uncomment the line below if you wish to ommit the records of very
	 * short processes
	 */
	if ((ucpu + ucpuns + scpu + scpuns) < 0.5) return; 
	/* Print the process record */
	(void)fprintf(out, "%s %s %-12s (%6u) (%6u) %8.2f %8.2f %s\n",
		start_time, finish_time, cmd, projid, task,
		ucpu + ucpuns, scpu + scpuns, wstat_buf);
}
