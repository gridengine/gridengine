#ifndef __MSG_COMMON_H
#define __MSG_COMMON_H
/*___INFO__MARK_BEGIN__*/
/*************************************************************************
 * 
 *  The Contents of this file are made available subject to the terms of
 *  the Sun Industry Standards Source License Version 1.2
 * 
 *  Sun Microsystems Inc., March, 2001
 * 
 * 
 *  Sun Industry Standards Source License Version 1.2
 *  =================================================
 *  The contents of this file are subject to the Sun Industry Standards
 *  Source License Version 1.2 (the "License"); You may not use this file
 *  except in compliance with the License. You may obtain a copy of the
 *  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
 * 
 *  Software provided under this License is provided on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 *  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
 *  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
 *  See the License for the specific provisions governing your rights and
 *  obligations concerning the Software.
 * 
 *   The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 * 
 *   Copyright: 2001 by Sun Microsystems, Inc.
 * 
 *   All Rights Reserved.
 * 
 ************************************************************************/
/*___INFO__MARK_END__*/


#include "basis_types.h"

/*
** global messages
*/

#define MSG_SGETEXT_SGEROOTNOTFOUND_S         _("SGE_ROOT directory "SFQ" doesn't exist\n")
#define MSG_SGETEXT_SGEROOTNOTSET             _("SGE_ROOT is not set\n")

#define MSG_PARSE_TOOMANYOPTIONS              _("ERROR! too many options\n")
#define MSG_UNKNOWN_OBJECT                    _("??? unknown object ???")
#define MSG_NONE                   _("none")
#define MSG_NULL                   _("(NULL)")
#define MSG_SMALLNULL                       _("(null)")

#define MSG_FILE_CANTOPENDIRECTORYX_SS                         _("can't open directory \"%s\": %s\n")
#define MSG_SGETEXT_CANTRESOLVEHOST_S           _("can't resolve hostname "SFQ"\n")
#define MSG_SGETEXT_CANTRESOLVEHOST_SS          _("can't resolve hostname "SFQ": %s\n")
#define MSG_NULLPOINTER            _("NULL pointer received")    
#define MSG_FILE_NOWRITE_SS           _("unable to open %s for writing: %s")
#define MSG_FILE_NOOPEN_SS            _("cant open file %s: %s\n")
#define MSG_FILE_NOWRITE_S            _("cant not write to file %s\n")
#define MSG_ERROR_COULDNOTOPENSTDOUTASFILE                 _("Could not open stdout as file\n")
#define MSG_ERROR_UNABLETODUMPJOBLIST                      _("Unable to dump job list\n")

#define MSG_CONFIG_CONF_VERSIONNOTFOUNDONREADINGSPOOLFILE    _("conf_version not found on reading spool file\n")



#define MSG_ANSWER_SUBGROUPXINGROUPYNOTACCEPTED_SS _("subgroup '%s' in group '%s' not accepted\n")
#define MSG_ANSWER_HOSTXINGROUPYNOTACCEPTED_SS     _("host '%s' in group '%s' not accepted\n")
#define MSG_ANSWER_NOGUILTYSUBGROUPNAME_S    _("'%s' is no guilty sub group name\n")
#define MSG_ANSWER_NOGROUPNAMESPECIFIED      _("no group name specified\n")
#define MSG_ANSWER_XCANTBESUBGROUPOFITSELF_S _("'%s' can't be it's own sub group\n")
#define MSG_ANSWER_CANTGETSUBGROUPELEMX_S    _("can't get sub group element '%s'\n")


/*
** parse_job_cull.c
*/
#define MSG_PARSE_NULLPOINTERRECEIVED       _("NULL pointer received\n")
#define MSG_MEM_MEMORYALLOCFAILED           _("memory allocation failed\n")
#define MSG_ANSWER_GETCWDFAILED             _("getcwd() failed\n")
#define MSG_ANSER_CANTDELETEHGRPXREFERENCEDINUSERMAPPINGFORCLUSTERUSERY_SS _("can't delete host group '%s' - referenced in user mapping for cluster user '%s'\n")
#define MSG_ANSWER_HELPNOTALLOWEDINCONTEXT  _("-help not allowed in this context\n")
#define MSG_ANSWER_UNKOWNOPTIONX_S          _("Unknown option %s")
#define MSG_ANSWER_CANTPROCESSNULLLIST      _("can't process NULL list")
#define MSG_FILE_ERROROPENINGXY_SS          _("error opening %s: %s\n")
#define MSG_ANSWER_ERRORREADINGFROMFILEX_S  _("error reading from file %s\n")
#define MSG_ANSWER_ERRORREADINGFROMSTDIN    _("error reading from stdin\n")
#define MSG_ANSWER_SUBMITBINARIESDENIED     _("denied: may not submit binaries\n")
#define MSG_FILE_ERROROPENFILEXFORWRITING_S _("error opening file \"%s\" for writing\n")
#define MSG_FILE_ERRORWRITETOFILEX_S        _("error writing to file \"%s\"\n")
#define MSG_ANSWER_ARGUMENTMISSINGFORX_S    _("argument missing for \"%s\"\n")
#define MSG_USER_INVALIDNAMEX_S                      _("invalid user name \"%s\"\n")
#define MSG_USER_NOHOMEDIRFORUSERX_S                 _("missing home directory for user \"%s\"\n")
#define MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S  _("getuniquehostname() failed resolving: %s\n")
#define MSG_OBJ_UNABLE2FINDQ_S        _("unable to find queue \"%s\"\n")
#define MSG_OBJ_USER                  _("user")
#define MSG_OBJ_GROUP                 _("group")
#define MSG_OBJ_USERPRJ               _("user/project")
#define MSG_OBJ_SHARETREE             _("sharetree")
#define MSG_OBJ_USERSET               _("userset")
#define MSG_JOB_PROJECT               _("project")
#define MSG_SGETEXT_DOESNOTEXIST_SS             _("denied: "SFN" "SFQ" does not exist\n")
#define MSG_SGETEXT_MUSTBEMANAGER_S             _("denied: "SFQ" must be manager for this operation\n")
#define MSG_SGETEXT_MUSTBEOPERATOR_S            _("denied: "SFQ" must be operator for this operation\n")



/*
** parse.c
*/
#define MSG_PARSE_XISNOTAVALIDOPTION_S      _("\"%s\" is not a valid option\n")
#define MSG_PARSE_NOOPTIONARGPROVIDEDTOX_S  _("no option argument provided to \"%s\"")
#define MSG_SMALLNULL                       _("(null)")
#define MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S _("ERROR! %s option must have argument\n")
#define MSG_JOB_XISINVALIDJOBTASKID_S       _("ERROR! %s is a invalid job-task identifier\n")


/*
** parse_qlist.c
*/
#define MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S        _("\"%s\" option has already been set, overriding previous setting\n")
#define MSG_PARSE_WORNGOBJLISTFORMATXSPECIFIEDTOYOPTION_SS    _("ERROR! Wrong object list format \"%s\" specified to %s option\n")
#define MSG_PARSE_DOPTIONMUSTHAVEFILEARGUMENT                 _("ERROR! -d option must have file argument\n")
#define MSG_PARSE_WRONGFIELDLISTFORMATXSPECIFIEDFORFOPTION_S  _("ERROR! Wrong field list format \"%s\" specified to -f option\n")
#define MSG_PARSE_INVALIDOPTIONARGUMENTX_S                    _("ERROR! invalid option argument \"%s\"\n")


/*
** parse_qsub.c
*/
#define MSG_ANSWER_WRONGTIMEFORMATEXSPECIFIEDTOAOPTION_S  _("ERROR! Wrong date/time format \"%s\" specified to -a option\n")
#define MSG_PARSE_ACOPTIONMUSTHAVECONTEXTLISTLISTARGUMENT _("ERROR! -ac option must have a context_list list argument\n")
#define MSG_ANSWER_WRONGCONTEXTLISTFORMATAC_S              _("ERROR! Wrong context list format -ac \"%s\"\n")
#define MSG_PARSE_CARGUMENTINVALID                         _("ERROR! -c argument invalid\n")
#define MSG_PARSE_CSPECIFIERINVALID                        _("ERROR! -c specifier invalid\n")
#define MSG_PARSE_DCOPTIONMUSTHAVESIMPLECONTEXTLISTARGUMENT  _("ERROR! -dc option must have a simple_context_list list argument\n")
#define MSG_PARSE_WRONGCONTEXTLISTFORMATDC_S              _("ERROR! Wrong context list format -dc \"%s\"\n")
#define MSG_PARSE_DISPLAYOPTIONMUSTHAVEARGUMENT              _("ERROR! -display option must have argument\n")
#define MSG_PARSE_WRONGTIMEFORMATXSPECTODLOPTION_S             _("ERROR! Wrong date/time format \"%s\" specified to -dl option\n")
#define MSG_PARSE_WRONGPATHLISTFORMATXSPECTOEOPTION_S                 _("ERROR! Wrong path list format \"%s\" specified to -e option\n")
#define MSG_PARSE_UNKNOWNHOLDLISTXSPECTOHOPTION_S              _("ERROR! Unknown hold list \"%s\" specified to -h option\n")
#define MSG_PARSE_WRONGJIDHOLDLISTFORMATXSPECTOHOLDJIDOPTION_S              _("ERROR! Wrong jid_hold list format \"%s\" specified to -hold_jid option\n")
#define MSG_PARSE_INVALIDOPTIONARGUMENTJX_S              _("invalid option argument \"-j %s\"\n")
#define MSG_PARSE_WRONGRESOURCELISTFORMATXSPECTOLOPTION_S              _("ERROR! Wrong resource list format \"%s\" specified to -l option\n")
#define MSG_PARSE_WRONGMAILOPTIONSLISTFORMATXSPECTOMOPTION_S              _("ERROR! Wrong mail options list format \"%s\" specified to -m option\n")
#define MSG_PARSE_WRONGMAILLISTFORMATXSPECTOMOPTION_S              _("ERROR! Wrong mail list format \"%s\" specified to -M option\n")
#define MSG_PARSE_ARGUMENTTONOPTIONMUSTNOTCONTAINBSL              _("ERROR! argument to -N option must not contain / \n")
#define MSG_PARSE_EMPTYSTRINGARGUMENTTONOPTIONINVALID              _("ERROR! empty string argument to -N option invalid\n")
#define MSG_PARSE_INVALIDOPTIONARGUMENTNOW_S              _("invalid option argument \"-now %s\"\n")
#define MSG_PARSE_WRONGSTDOUTPATHLISTFORMATXSPECTOOOPTION_S              _("ERROR! Wrong stdout path list format \"%s\" specified to -o option\n")
#define MSG_PARSE_PEOPTIONMUSTHAVEPENAMEARGUMENT              _("ERROR! -pe option must have pe_name argument\n")
#define MSG_PARSE_PEOPTIONMUSTHAVERANGEAS2NDARGUMENT              _("ERROR! -pe option must have range as 2nd argument\n")
#define MSG_PARSE_QOPTIONMUSTHAVEDESTIDLISTARGUMENT              _("ERROR! -q option must have destination identifier list argument\n")
#define MSG_PARSE_WRONGDESTIDLISTFORMATXSPECTOQOPTION_S              _("ERROR! Wrong destination identifier list format \"%s\" specified to -q option\n")
#define MSG_PARSE_QSARGSOPTIONMUSTBETERMINATEDWITHQSEND              _("ERROR! -qs_args option must be terminated with -qs_end\n")
#define MSG_PARSE_INVALIDOPTIONARGUMENTRX_S              _("invalid option argument \"-r %s\"\n")
#define MSG_PARSE_SCOPTIONMUSTHAVECONTEXTLISTARGUMENT              _("ERROR! -sc option must have context list argument\n")
#define MSG_PARSE_WRONGCONTEXTLISTFORMATSCX_S              _("ERROR! Wrong context list format -sc \"%s\"\n")
#define MSG_PARSE_SOPTIONMUSTHAVEPATHNAMEARGUMENT              _("ERROR! -S option must have path_name argument\n")
#define MSG_PARSE_WRONGSHELLLISTFORMATXSPECTOSOPTION_S              _("ERROR! Wrong shell list format \"%s\" specified to -S option\n")
#define MSG_PARSE_TOPTIONMUSTHAVEALISTOFTASKIDRANGES              _("ERROR! -t option must have a list of task id ranges\n")
#define MSG_PARSE_UOPTMUSTHAVEALISTUSERNAMES              _("ERROR! -u option must have a list usernames\n")
#define MSG_PARSE_VOPTMUSTHAVEVARIABLELISTARGUMENT              _("ERROR! -v option must have variable list argument\n")
#define MSG_PARSE_WRONGVARIABLELISTFORMATVORENVIRONMENTVARIABLENOTSET_S              _("ERROR! Wrong variable list format -v \"%s\" or environment variable not set\n")
#define MSG_PARSE_COULDNOTPARSEENVIRIONMENT              _("ERROR! Could not parse environment\n")
#define MSG_PARSE_INVALIDOPTIONARGUMENTWX_S              _("invalid option argument \"-w %s\"\n")
#define MSG_PARSE_ATSIGNOPTIONMUSTHAVEFILEARGUMENT       _("ERROR! -@ option must have file argument\n")
#define MSG_PARSE_INVALIDOPTIONARGUMENTX_S              _("ERROR! invalid option argument \"%s\"\n")
#define MSG_PARSE_NOJOBIDGIVENBEFORESEPARATOR              _("ERROR! no job id given before -- separator\n")
#define MSG_PARSE_OPTIONMUSTBEFOLLOWEDBYJOBARGUMENTS              _("ERROR! -- option must be followed by job arguments\n")
#define MSG_PARSE_WRONGJOBIDLISTFORMATXSPECIFIED_S              _("ERROR! Wrong job id list format \"%s\" specified\n")
#define MSG_PARSE_INVALIDPRIORITYMUSTBEINNEG1023TO1024              _("ERROR! invalid priority, must be an integer from -1023 to 1024\n")


/*
** parse_qconf.c
*/
#define MSG_QCONF_CANTCHANGEQUEUENAME_S           _("%s: cannot change queuename\n")
#define MSG_QCONF_CANTCHANGEHOSTNAME_S           _("%s: cannot change hostname\n")  
#define MSG_FILE_NOFILEARGUMENTGIVEN           _("no file argument given\n")
#define MSG_PARSE_EDITFAILED                   _("edit failed\n")
#define MSG_FILE_FILEUNCHANGED                 _("file unchanged\n")
#define MSG_FILE_ERRORREADINGINFILE            _("error reading in file\n")
#define MSG_EXEC_XISNOEXECHOST_S               _("%s is no exec host\n")
#define MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S    _("added host %s to exec host list\n")
#define MSG_ANSWER_INVALIDFORMAT               _("invalid format\n")
#define MSG_QUEUE_XISNOTAQUEUENAME_S           _("%s is not a queuename\n")
#define MSG_QUEUE_UNABLETOWRITETEMPLATEQUEUE   _("unable to write template queue")
#define MSG_TREE_CHANGEDSHARETREE              _("changed sharetree\n")
#define MSG_TREE_NOSHARETREE                   _("No sharetree\n")
#define MSG_TREE_CANTADDNODEXISNONUNIQUE_S     _("Could not add node %s to sharetree due to non-unique path\n")
#define MSG_TREE_SETTING                       _("setting ")
#define MSG_TREE_REMOVING                      _("removing ") 
#define MSG_ANSWER_XISNOTVALIDSEENODESHARESLIST_S _("%s is not a valid argument, see node_shares_list format\n")
#define MSG_TREE_MODIFIEDSHARETREE             _("modified sharetree\n")
#define MSG_TREE_NOMIDIFIEDSHARETREE           _("no modifications to sharetree\n")
#define MSG_ANSWER_NOLISTNAMEPROVIDEDTOAUX_S   _("no list_name provided to \"-au %s\"")
#define MSG_ANSWER_NOLISTNAMEPROVIDEDTODUX_S   _("no list_name provided to \"-du %s\"")
#define MSG_TREE_CANTDELROOTNODE               _("can't delete root node\n")
#define MSG_TREE_CANTDELNODESWITHCHILD         _("can't delete nodes with children\n")
#define MSG_ANSWER_XISNOTAVALIDOPTIONY_SU      _("\"%s\" is not a valid option " U32CFormat "\n")
#define MSG_COMPLEX_NEEDACOMPLEXNAME           _("Need a complex Name")
#define MSG_CALENDAR_XISNOTACALENDAR_S         _("%s is not a calendar\n")
#define MSG_CKPT_XISNOTCHKPINTERFACEDEF_S      _("%s is not a checkpointing interface definition\n")
#define MSG_EXEC_XISNOTANEXECUTIONHOST_S       _("%s is not an execution host\n")
#define MSG_EXEC_HOSTENTRYOFXCHANGEDINEXECLIST_S     _("changed entry of host %s in execution host list\n")
#define MSG_PARALLEL_XNOTAPARALLELEVIRONMENT_S     _("%s is not a parallel environment\n")
#define MSG_QUEUE_XISNOTAQUEUENAME_S           _("%s is not a queuename\n")
#define MSG_QUEUE_UNABLETOWRITEOLDQUEUE        _("unable to write old queue")
#define MSG_ANSWER_MISSINGFILENAMEASOPTIONARG_S   _("%s: missing filename as option argument\n")
#define MSG_QCONF_XISNOTAOBJECTATTRIB_SSS         _("%s: \"%s\" is not a %s attribute\n")
#define MSG_QCONF_XISNOTAQUEUEATTRIB_SS           _("%s: \"%s\" is not a queue attribute\n")
#define MSG_QCONF_XISNOTAEXECHOSTATTRIB_SS           _("%s: \"%s\" is not a exec host attribute\n")
#define MSG_QCONF_CANTCHANGEOBJECTNAME_SS         _("%s: cannot change %s\n")
#define MSG_QCONF_INTERNALFAILURE_S               _("%s: internal failure\n")
#define MSG_QCONF_MQATTR_MISSINGOBJECTLIST_S       _("%s: missing object list\n")
#define MSG_QCONF_MQATTR_MISSINGQUEUELIST_S       _("%s: missing queue list\n")
#define MSG_QCONF_MQATTR_MISSINGHOSTLIST_S       _("%s: missing host list\n")        
#define MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION  _("changed scheduler configuration\n")
#define MSG_USER_XISNOKNOWNUSER_S                 _("%s is not known as user\n")
#define MSG_PROJECT_XISNOKNWOWNPROJECT_S          _("%s is not known as project\n")
#define MSG_ANSWER_NEEDHOSTNAMETODELLOCALCONFIG     _("Need hostname to delete local configuration\n")
#define MSG_ANSWER_INVALIDOPTIONARGX_S            _("invalid option argument \"%s\"")
#define MSG_SRC_X_HELP_USAGE_S                    _("Usage: %s -help\n")
#define MSG_FILE_EDITFILEXDOESNOTEXIST_S          _("edit file %s does not exist")
#define MSG_QCONF_EDITOREXITEDWITHERROR_I         _("editor exited with error %d")
#define MSG_QCONF_EDITFILEXNOLONGEREXISTS_S       _("edit file %s no longer exists")
#define MSG_QCONF_EDITORWASTERMINATEDBYSIGX_I     _("editor was terminated by a signal %d\n")
#define MSG_QCONF_CANTSTARTEDITORX_S              _("can't start editor %s")
#define MSG_QCONF_XADDEDTOYLIST_SS                _("%s added to %s list\n")
#define MSG_FILE_CANTCREATETEMPFILE               _("could not generate temporary filename\n")
#define MSG_QCONF_CANTREADCONFIG_S                _("can't read configuration: %s\n")
#define MSG_QCONF_CANTREADX_S                     _("cant read %s\n")
#define MSG_QCONF_CANTREADSHARETREEX_S            _("cant read sharetree: %s\n")
#define MSG_QCONF_NOXDEFINED_S                    _("no %s defined\n")
#define MSG_TABLE_HOST                            "HOST"
#define MSG_TABLE_PROCESSORS                      "PROCESSORS"
#define MSG_TABLE_ARCH                            "ARCH"
#define MSG_TABLE_SUM_F                           "SUM"
#define MSG_TABLE_EV_ID                           "ID"
#define MSG_TABLE_EV_NAME                         "NAME"
#define MSG_QCONF_NOEXECUTIONHOSTSDEFINED         _("no execution hosts defined\n")
#define MSG_QCONF_NOEVENTCLIENTSREGISTERED        _("no event clients registered\n")
#define MSG_QCONF_XISNOVALIDCOMPLEXNAME_S         _("%s is no valid complex name\n")
#define MSG_ANSWER_MALFORMEDREQUEST               _("malformed request\n")
#define MSG_ANSWER_COMPLEXXALREADYEXISTS_S        _("complex %s already exists\n")
#define MSG_ANSWER_USERMAPENTRYALREADYEXISTS_S    _("user mapping for %s already exists\n")
#define MSG_ANSWER_HOSTGROUPENTRYALLREADYEXISTS_S _("host group '%s' already exists\n")
#define MSG_ANSWER_CLUSTERUNAMEXDIFFFROMY_SS      _("\"cluster_user\" name entry '%s' does not match filename '%s'\n")
#define MSG_UM_NOCLUSTERUSERENTRYINFILEX_S        _("no \"cluster_user\" name entry in file '%s'\n")
#define MSG_ANSWER_HOSTGROUPNAMEXDIFFFROMY_SS     _("\"group_name\" entry '%s' does not match filename '%s'\n")
#define MSG_HGRP_NOHOSTGROUPENTRYINFILEX_S        _("no \"group_name\" name entry in file '%s'\n")
#define MSG_ANSWER_UNKNOWNHOSTORGROUPNAME_S       _("unknown host or group name '%s'\n")
#define MSG_ANSWER_UNKNOWNHOSTNAME_S              _("unknown host name '%s'\n")
#define MSG_ANSWER_UNKNOWNGROUPNAME_S             _("unknown group name '%s'\n")
#define MSG_ANSWER_SUBGROUPHASNOSUPERGROUP_SS      _("subgroup '%s' has no supergroup '%s'\n")
#define MSG_ANSWER_SUBGROUPXHASLINKTOGROUPY_SS       _("sub group '%s' has sub group link to group '%s' (deadlock)\n")
#define MSG_ANSWER_SUPERGROUPHASNOSUBGROUP_SS      _("supergroup '%s' has no subgroup '%s'\n")
#define MSG_ANSWER_CLUSTERUNAMEXNOTAMBIGUOUSMAPFORYATZ_SSS _("cluster_user name entry '%s' has not ambiguous mappings for '%s' at '%s'\n")
#define MSG_ANSWER_DUPLICATEDMAPPINGENTRY_SSS     _("mapping entry as user '%s' from '%s' already exists for cluster user '%s'\n")
#define MSG_ANSWER_IGNORINGMAPPINGFOR_S           _("usermapping for '%s' not accepted\n")
#define MSG_ANSWER_IGNORINGHOSTGROUP_S            _("host group list '%s' not accepted\n")
#define MSG_ANSWER_USERMAPENTRYXNOTFOUND_S        _("user mapping entry for user '%s' not found\n")
#define MSG_ANSWER_HOSTGROUPENTRYXNOTFOUND_S      _("host group entry for group '%s' not found\n")
#define MSG_ANSWER_COMPLEXXDOESNOTEXIST_S         _("complex %s does not exist.\n")
#define MSG_MULTIPLY_MODIFIEDIN                   _("modified in")
#define MSG_MULTIPLY_ADDEDTO                      _("added to")
#define MSG_MULTIPLY_DELETEDFROM                  _("deleted from")
#define MSG_ANSWER_XYCOMPLEXLIST_SS               _("%s %s complex list\n")
#define MSG_FILE_ERRORWRITINGUSERSETTOFILE        _("error writing userset to file\n")
#define MSG_FILE_ERRORREADINGUSERSETFROMFILE_S    _("error reading userset from file %s\n")
#define MSG_COMPLEX_COMPLEXXNOTDEFINED_S          _("complex %s not defined\n")
#define MSG_ERROR_USERXNOTDEFINED_S               _("user %s not defined\n")
#define MSG_ERROR_GROUPXNOTDEFINED_S              _("host group '%s' is not defined\n")
#define MSG_ANSWER_CONFIGXNOTDEFINED_S            _("configuration %s not defined\n")
#define MSG_ANSWER_CONFIGXALREADYEXISTS_S         _("configuration %s already exists\n")
#define MSG_ANSWER_CONFIGXDOESNOTEXIST_S          _("configuration %s does not exist\n")
#define MSG_ANSWER_CONFIGUNCHANGED                _("configuration unchanged\n")
#define MSG_ANSWER_ERRORREADINGTEMPFILE           _("error reading temp file\n")
#define MSG_ANSWER_ERRORREADINGCONFIGFROMFILEX_S  _("error reading configuration from file %s\n")
#define MSG_ANSWER_DENIEDHOSTXISNOADMINHOST_S     _("denied: host \"%s\" is no admin host\n")


/*
** sge_sharetree.c
*/
#define MSG_TREE_UNABLETOLACATEXINSHARETREE_S     _("Unable to locate %s in sharetree\n")
#define MSG_OBJ_NOSTREEELEM           _("no sharetree element")
#define MSG_STREE_UNEXPECTEDNAMEFIELD  _("unexpected name field")
#define MSG_STREE_UNEXPECTEDTYPEFIELD  _("unexpected type field")
#define MSG_STREE_UNEXPECTEDVERSIONFIELD  _("unexpected version field")
#define MSG_STREE_UNEXPECTEDSHARESFIELD  _("unexpected shares field")
#define MSG_STREE_UNEXPECTEDCHILDNODEFIELD  _("unexpected childnodes field")
#define MSG_STREE_NAMETWICE_I         _("double defined name in line %d")
#define MSG_STREE_TYPETWICE_I         _("double defined type in line %d")
#define MSG_STREE_SHARESTWICE_I       _("double defined shares in line %d")
#define MSG_STREE_CHILDNODETWICE_I     _("double defined child node list in line %d")
#define MSG_STREE_NOFATHERNODE_U      _("could not find father node for node "U32CFormat)
#define MSG_STREE_NOPARSECHILDNODES   _("error parsing child node list")
#define MSG_STREE_NOPARSELINE_I       _("error parsing line %d")
#define MSG_STREE_NOVALIDNODEREF_U    _("found reference to node " U32CFormat " but no specification")
#define MSG_FILE_FILEEMPTY            _("empty file")


/*
** sched_conf.c
*/
#define MSG_SCHEDCONF_ALPPISNULLNOSCHEDULERCONFIGURATION    _("alpp == NULL - no scheduler configuration")
#define MSG_ANSWER_NOSCHEDULERCONFIGURATION    _("no scheduler configuration")
#define MSG_SCHEDCONF_CANTOPENSCHEDULERCONFIGURATIONFILE_SS    _("cant open scheduler configuration file \"%s\": %s\n")
#define MSG_SCHEDCONF_INVALIDVALUEXFORQUEUESORTMETHOD_S    _("invalid value \"%s\" for queue_sort_method")
#define MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION    _("can't create scheduler configuration\n")

/* read_write_job.c */
#define MSG_JOB_CANNOT_REMOVE_SS       _("can not remove "SFN": "SFN"\n")
#define MSG_JOB_TASK_SPOOL_FILE        _("task spool file")
#define MSG_JOB_JOB_SPOOL_FILE         _("job spool file")
#define MSG_JOB_JOB_SPOOL_DIRECTORY    _("job spool directory")


/* read_write_ume.c */
#define MSG_UM_CONFIGTEXT1 _("# configuration file for user mapping\n")
#define MSG_UM_CONFIGTEXT2 _("#\n")
#define MSG_UM_CONFIGTEXT3 _("# cluster_user      NAME -> unique cluster user name\n")
#define MSG_UM_CONFIGTEXT4 _("# MAPPING_USER           -> User mapping for specified hosts/host groups\n")
#define MSG_UM_CONFIGTEXT5 _("# MAPPINGHOST-/GROUPLIST -> [,] separated host or host group names\n")
#define MSG_UM_CONFIGTEXT6 _("# MAPPING_USER      MAPPINGHOST-/GROUPLIST\n")
/* read_write_host_group.c */
#define MSG_HOSTGROUP_CONFIGTEXT1 _("# configuration file for host groups\n")
#define MSG_HOSTGROUP_CONFIGTEXT2 _("#\n")
#define MSG_HOSTGROUP_CONFIGTEXT3 _("# group_name      NAME -> unique host group name\n")
#define MSG_HOSTGROUP_CONFIGTEXT4 _("# each line contains a hostname\n")
#define MSG_HOSTGROUP_CONFIGTEXT5 _("# if the first character is a '@' the following name\n")
#define MSG_HOSTGROUP_CONFIGTEXT6 _("# is resolved as subgroup name\n")



/*
** complex.c
*/
#define MSG_COMPLEX_STARTSCOMMENTBUTNOSAVE _("#--- # starts a comment but comments are not saved across edits -----------------------\n")
#define MSG_PARSE_CANTPARSECPLX_S     _("parse error in file %s\n")
#define MSG_PARSE_CANTPARSECPLX_SI    _("parse error in file %s in line %d\n")
#define MSG_PARSE_INVALIDCPLXTYPE_SS    _("parse error in file %s, unknown type %s\n")
#define MSG_PARSE_INVALIDCPLXRELOP_SS   _("parse error in file %s, invalid relation operator %s\n")
#define MSG_PARSE_INVALIDCPLXCONSUM_SSS _("error in file %s: attribute %s may not be consumable because it is of type %s\n")
#define MSG_PARSE_INVALIDCPLXENTRY_SSS  _("parse error in file %s, invalid %s entry %s\n")
#define MSG_PARSE_INVALIDCPLXREQ_SS     _("parse error in file %s, invalid requestable entry %s\n")
#define MSG_COM_COMMDLOCKED           _("may be commd is locked:\n")
#define MSG_CPLX_VALUEMISSING_S       _("denied: missing value for request "SFQ"\n")
#define MSG_CPLX_WRONGTYPE_SSS        _("wrong type in \"%s=%s\" - %s expected\n")
#define MSG_SGETEXT_INVALIDHOST_S               _("invalid hostname "SFQ"\n")
#define MSG_CPLX_ATTRIBISNEG_S        _("attribute "SFQ" is consumable but has a negative value\n")
#define MSG_SGETEXT_UNKNOWN_RESOURCE_S          _("unknown resource "SFQ"\n")
#define MSG_SGETEXT_RESOURCE_NOT_REQUESTABLE_S        _("resource "SFQ" configured as non requestable\n")    
#define MSG_SGETEXT_UNKNOWN_ATTR_TYPE_U         _("unknown complex attribute type " U32CFormat  "\n")


/*
** read_object.c
*/
#define MSG_MEMORY_CANTMALLOCBUFFERFORXOFFILEY_SS    _("can't malloc buffer for %s of file \"%s\"")
#define MSG_FILE_CANTDETERMINESIZEFORXOFFILEY_SS    _("can't determine size for %s of file \"%s\"")
#define MSG_SGETEXT_UNKNOWN_CONFIG_VALUE_SSS          _("unknown attribute "SFQ" in "SFN" configuration in file "SFN"\n") 

/*
** read_write_cal.c
*/
#define MSG_TMPNAM_GENERATINGTMPNAM    _("generating tmpnam()")
#define MSG_FILE_ERRORWRITING_S        _("error writing %s\n")

/*
** read_write_host.c
*/
#define MSG_FILE_ERRORWRITING_SS    _("error writing %s, %s\n")
#define MSG_FILE_ERRORWRITINGHOSTNAME    _("error writing hostname\n")


/*
** read_write_manop.c
*/
#define MSG_FILE_ERROROPENINGX_S    _("error opening %s\n")

/*
** read_write_queue.c
*/
#define MSG_ANSWER_GETUNIQUEHNFAILEDRESX_SS  _("getuniquehostname() failed resolving %s: %s\n")

/*
** read_write_userprj.c
*/
#define MSG_RWUSERPRJ_EPISNULLNOUSERORPROJECTELEMENT    _("ep == NULL - no user or project element")
#define MSG_PROJECT_FOUNDPROJECTXTWICE_S    _("found project %s twice")
#define MSG_PROJECT_INVALIDPROJECTX_S    _("invalid project %s")
#define MSG_JOB_FOUNDJOBWITHWRONGKEY_S    _("found job with wrong key \"%s\"")
#define MSG_JOB_FOUNDJOBXTWICE_U    _("found job "U32CFormat" twice")

/*
** read_write_userset.c
*/
#define MSG_USERSET_NOUSERETELEMENT    _("no userset element")


/*
** get_conf.c
*/
#define MSG_CONF_GETCONF_S            _("getting configuration: %s")
#define MSG_CONF_REQCONF_II           _("requested %d configurations, got %d")
#define MSG_CONF_NOGLOBAL             _("global configuration not defined")
#define MSG_CONF_NOLOCAL_S            _("local configuration %s not defined - using global configuration")
#define MSG_CONF_NOCONFBG             _("can't get configuration from qmaster -- backgrounding")
#define MSG_CONF_NOCONFSLEEP          _("can't get configuration from qmaster -- sleep(10)")
#define MSG_CONF_NOCONFSTILL          _("still can't get configuration from qmaster -- trying further")
#define MSG_CONF_NOREADCONF_IS        _("Error %d reading configuration \"%s\"\n")
#define MSG_CONF_NOMERGECONF_IS       _("Error %d merging configuration \"%s\"\n")


/*
** sge_conf.c
*/
#define MSG_GDI_USING_SS              _("using \"%s\" for %s\n")
#define MSG_GDI_NOCONFIGFROMMASTER    _("could not get configuration from qmaster - using defaults\n")
#define MSG_GDI_NEITHERSGECODGRDSETTINGSGE _("neither SGE nor COD nor GRD environment for jobs configured, setting SGE environment\n")


#endif /* __MSG_COMMON_H */

