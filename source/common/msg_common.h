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

#define MSG_SGETEXT_SGEROOTNOTFOUND_S         _MESSAGE(23000, _("SGE_ROOT directory "SFQ" doesn't exist\n"))
#define MSG_SGETEXT_SGEROOTNOTSET             _MESSAGE(23001, _("SGE_ROOT is not set\n"))

#define MSG_PARSE_TOOMANYOPTIONS              _MESSAGE(23002, _("ERROR! too many options\n"))
#define MSG_UNKNOWN_OBJECT                    _MESSAGE(23003, _("??? unknown object ???"))
#define MSG_NONE                   _MESSAGE(23004, _("none"))
#define MSG_NULL                   _MESSAGE(23005, _("(NULL)"))
#define MSG_SMALLNULL                       _MESSAGE(23006, _("(null)"))

#define MSG_SGETEXT_NOQMASTER_NOPORT_NOSERVICE_SS   _MESSAGE(23007, _("unable to contact qmaster via "SFQ" commd - can't resolve service "SFQ"\n"))
#define MSG_SGETEXT_NOQMASTER_PORT_ENV_SI           _MESSAGE(23008, _("unable to contact qmaster via "SFQ" commd using port %d\n"))
#define MSG_SGETEXT_NOQMASTER_PORT_SERVICE_ENV_SIS  _MESSAGE(23009, _("unable to contact qmaster via "SFQ" commd using port %d (service "SFQ")\n"))
#define MSG_SGETEXT_NOQMASTER_REACHABLE             _MESSAGE(23010, _("unable to contact qmaster\n"))
#define MSG_SGETEXT_NOQMASTER_REACHABLE_COMMLIB_SS  _MESSAGE(23011, _("unable to contact qmaster via "SFQ" commd - commlib error: "SFQ"\n"))
#define MSG_SGETEXT_NOQMASTER_SUBSCR_AT_COMMD_S     _MESSAGE(23012, _("unable to contact qmaster via "SFQ" commd - qmaster not enrolled at commd\n"))
#define MSG_SGETEXT_NOQMASTER_RESOLVING_ERROR_S     _MESSAGE(23013, _("unable to contact qmaster via "SFQ" commd - hostname resolving error\n"))

#define MSG_FILE_CANTOPENDIRECTORYX_SS                         _MESSAGE(23014, _("can't open directory \"%s\": %s\n"))
#define MSG_SGETEXT_CANTRESOLVEHOST_S           _MESSAGE(23015, _("can't resolve hostname "SFQ"\n"))
#define MSG_SGETEXT_CANTRESOLVEHOST_SS          _MESSAGE(23016, _("can't resolve hostname "SFQ": %s\n"))
#define MSG_NULLPOINTER            _MESSAGE(23017, _("NULL pointer received") )    
#define MSG_FILE_NOWRITE_SS           _MESSAGE(23018, _("unable to open %s for writing: %s"))
#define MSG_FILE_NOOPEN_SS            _MESSAGE(23019, _("cant open file %s: %s\n"))
#define MSG_FILE_NOWRITE_S            _MESSAGE(23020, _("cant not write to file %s\n"))
#define MSG_ERROR_COULDNOTOPENSTDOUTASFILE                 _MESSAGE(23021, _("Could not open stdout as file\n"))
#define MSG_ERROR_UNABLETODUMPJOBLIST                      _MESSAGE(23022, _("Unable to dump job list\n"))

#define MSG_CONFIG_CONF_ERROROPENINGSPOOLFILE_SS    _MESSAGE(23023, _("error opening the configuration spool file "SFN": %s\n"))
#define MSG_CONFIG_CONF_VERSIONNOTFOUNDONREADINGSPOOLFILE    _MESSAGE(23024, _("conf_version not found on reading spool file\n"))
#define MSG_CONFIG_CONF_NOVALUEFORCONFIGATTRIB_S       _MESSAGE(23025, _("no value given for configuration attribute "SFQ"\n"))
#define MSG_CONFIG_CONF_INCORRECTVALUEFORCONFIGATTRIB_SS       _MESSAGE(23026, _("incorrect value "SFQ" given for configuration attribute "SFQ"\n"))
#define MSG_CONFIG_CONF_GIDRANGELESSTHANNOTALLOWED_I  _MESSAGE(23027, _("minimum group id in gid_range may not be less than %d in cluster configuration\n"))
#define MSG_CONFIG_CONF_ONLYSINGLEVALUEFORCONFIGATTRIB_S       _MESSAGE(23028, _("only a single value is allowed for configuration attribute "SFQ"\n"))
#define MSG_CONFIG_CONF_ERRORSTORINGCONFIGVALUE_S       _MESSAGE(23029, _("error storing configuration attribute "SFQ"\n"))



#define MSG_ANSWER_SUBGROUPXINGROUPYNOTACCEPTED_SS _MESSAGE(23030, _("subgroup '%s' in group '%s' not accepted\n"))
#define MSG_ANSWER_HOSTXINGROUPYNOTACCEPTED_SS     _MESSAGE(23031, _("host '%s' in group '%s' not accepted\n"))
#define MSG_ANSWER_NOGUILTYSUBGROUPNAME_S    _MESSAGE(23032, _("'%s' is no guilty sub group name\n"))
#define MSG_ANSWER_NOGROUPNAMESPECIFIED      _MESSAGE(23033, _("no group name specified\n"))
#define MSG_ANSWER_XCANTBESUBGROUPOFITSELF_S _MESSAGE(23034, _("'%s' can't be it's own sub group\n"))
#define MSG_ANSWER_CANTGETSUBGROUPELEMX_S    _MESSAGE(23035, _("can't get sub group element '%s'\n"))


/*
** parse_job_cull.c
*/
#define MSG_PARSE_NULLPOINTERRECEIVED       _MESSAGE(23036, _("NULL pointer received\n"))
#define MSG_MEM_MEMORYALLOCFAILED           _MESSAGE(23037, _("memory allocation failed\n"))
#define MSG_ANSWER_GETCWDFAILED             _MESSAGE(23038, _("getcwd() failed\n"))
#define MSG_ANSER_CANTDELETEHGRPXREFERENCEDINUSERMAPPINGFORCLUSTERUSERY_SS _MESSAGE(23039, _("can't delete host group '%s' - referenced in user mapping for cluster user '%s'\n"))
#define MSG_ANSWER_HELPNOTALLOWEDINCONTEXT  _MESSAGE(23040, _("-help not allowed in this context\n"))
#define MSG_ANSWER_UNKOWNOPTIONX_S          _MESSAGE(23041, _("Unknown option %s"))
#define MSG_ANSWER_CANTPROCESSNULLLIST      _MESSAGE(23042, _("can't process NULL list"))
#define MSG_FILE_ERROROPENINGXY_SS          _MESSAGE(23043, _("error opening %s: %s\n"))
#define MSG_ANSWER_ERRORREADINGFROMFILEX_S  _MESSAGE(23044, _("error reading from file %s\n"))
#define MSG_ANSWER_ERRORREADINGFROMSTDIN    _MESSAGE(23045, _("error reading from stdin\n"))
#define MSG_ANSWER_SUBMITBINARIESDENIED     _MESSAGE(23046, _("denied: may not submit binaries\n"))
#define MSG_FILE_ERROROPENFILEXFORWRITING_S _MESSAGE(23047, _("error opening file \"%s\" for writing\n"))
#define MSG_FILE_ERRORWRITETOFILEX_S        _MESSAGE(23048, _("error writing to file \"%s\"\n"))
#define MSG_ANSWER_ARGUMENTMISSINGFORX_S    _MESSAGE(23049, _("argument missing for \"%s\"\n"))
#define MSG_USER_INVALIDNAMEX_S                      _MESSAGE(23050, _("invalid user name \"%s\"\n"))
#define MSG_USER_NOHOMEDIRFORUSERX_S                 _MESSAGE(23051, _("missing home directory for user \"%s\"\n"))
#define MSG_ANSWER_GETUNIQUEHNFAILEDRESX_S  _MESSAGE(23052, _("getuniquehostname() failed resolving: %s\n"))
#define MSG_OBJ_UNABLE2FINDQ_S        _MESSAGE(23053, _("unable to find queue \"%s\"\n"))
#define MSG_OBJ_USER                  _MESSAGE(23054, _("user"))
#define MSG_OBJ_GROUP                 _MESSAGE(23055, _("group"))
#define MSG_OBJ_USERPRJ               _MESSAGE(23056, _("user/project"))
#define MSG_OBJ_SHARETREE             _MESSAGE(23057, _("sharetree"))
#define MSG_OBJ_USERSET               _MESSAGE(23058, _("userset"))
#define MSG_JOB_PROJECT               _MESSAGE(23059, _("project"))
#define MSG_SGETEXT_DOESNOTEXIST_SS             _MESSAGE(23060, _("denied: "SFN" "SFQ" does not exist\n"))
#define MSG_SGETEXT_MUSTBEMANAGER_S             _MESSAGE(23061, _("denied: "SFQ" must be manager for this operation\n"))
#define MSG_SGETEXT_MUSTBEOPERATOR_S            _MESSAGE(23062, _("denied: "SFQ" must be operator for this operation\n"))



/*
** parse.c
*/
#define MSG_PARSE_NOOPTIONARGPROVIDEDTOX_S  _MESSAGE(23064, _("no option argument provided to \"%s\""))
#define MSG_PARSE_XOPTIONMUSTHAVEARGUMENT_S _MESSAGE(23066, _("ERROR! %s option must have argument\n"))


/*
** parse_qlist.c
*/
#define MSG_PARSE_XOPTIONALREADYSETOVERWRITINGSETING_S        _MESSAGE(23068, _("\"%s\" option has already been set, overriding previous setting\n"))
#define MSG_PARSE_WORNGOBJLISTFORMATXSPECIFIEDTOYOPTION_SS    _MESSAGE(23069, _("ERROR! Wrong object list format \"%s\" specified to %s option\n"))
#define MSG_PARSE_DOPTIONMUSTHAVEFILEARGUMENT                 _MESSAGE(23070, _("ERROR! -d option must have file argument\n"))
#define MSG_PARSE_WRONGFIELDLISTFORMATXSPECIFIEDFORFOPTION_S  _MESSAGE(23071, _("ERROR! Wrong field list format \"%s\" specified to -f option\n"))
#define MSG_PARSE_INVALIDOPTIONARGUMENTX_S                    _MESSAGE(23072, _("ERROR! invalid option argument \"%s\"\n"))


/*
** parse_qsub.c
*/
#define MSG_ANSWER_WRONGTIMEFORMATEXSPECIFIEDTOAOPTION_S  _MESSAGE(23073, _("ERROR! Wrong date/time format \"%s\" specified to -a option\n"))
#define MSG_PARSE_ACOPTIONMUSTHAVECONTEXTLISTLISTARGUMENT _MESSAGE(23074, _("ERROR! -ac option must have a context_list list argument\n"))
#define MSG_ANSWER_WRONGCONTEXTLISTFORMATAC_S              _MESSAGE(23075, _("ERROR! Wrong context list format -ac \"%s\"\n"))
#define MSG_PARSE_CARGUMENTINVALID                         _MESSAGE(23076, _("ERROR! -c argument invalid\n"))
#define MSG_PARSE_CSPECIFIERINVALID                        _MESSAGE(23077, _("ERROR! -c specifier invalid\n"))
#define MSG_PARSE_DCOPTIONMUSTHAVESIMPLECONTEXTLISTARGUMENT  _MESSAGE(23078, _("ERROR! -dc option must have a simple_context_list list argument\n"))
#define MSG_PARSE_WRONGCONTEXTLISTFORMATDC_S              _MESSAGE(23079, _("ERROR! Wrong context list format -dc \"%s\"\n"))
#define MSG_PARSE_DISPLAYOPTIONMUSTHAVEARGUMENT              _MESSAGE(23080, _("ERROR! -display option must have argument\n"))
#define MSG_PARSE_WRONGTIMEFORMATXSPECTODLOPTION_S             _MESSAGE(23081, _("ERROR! Wrong date/time format \"%s\" specified to -dl option\n"))
#define MSG_PARSE_WRONGPATHLISTFORMATXSPECTOEOPTION_S                 _MESSAGE(23082, _("ERROR! Wrong path list format \"%s\" specified to -e option\n"))
#define MSG_PARSE_UNKNOWNHOLDLISTXSPECTOHOPTION_S              _MESSAGE(23083, _("ERROR! Unknown hold list \"%s\" specified to -h option\n"))
#define MSG_PARSE_WRONGJIDHOLDLISTFORMATXSPECTOHOLDJIDOPTION_S              _MESSAGE(23084, _("ERROR! Wrong jid_hold list format \"%s\" specified to -hold_jid option\n"))
#define MSG_PARSE_INVALIDOPTIONARGUMENTJX_S              _MESSAGE(23085, _("invalid option argument \"-j %s\"\n"))
#define MSG_PARSE_WRONGRESOURCELISTFORMATXSPECTOLOPTION_S              _MESSAGE(23086, _("ERROR! Wrong resource list format \"%s\" specified to -l option\n"))
#define MSG_PARSE_WRONGMAILOPTIONSLISTFORMATXSPECTOMOPTION_S              _MESSAGE(23087, _("ERROR! Wrong mail options list format \"%s\" specified to -m option\n"))
#define MSG_PARSE_WRONGMAILLISTFORMATXSPECTOMOPTION_S              _MESSAGE(23088, _("ERROR! Wrong mail list format \"%s\" specified to -M option\n"))
#define MSG_PARSE_ARGUMENTTONOPTIONMUSTNOTCONTAINBSL              _MESSAGE(23089, _("ERROR! argument to -N option must not contain / \n"))
#define MSG_PARSE_EMPTYSTRINGARGUMENTTONOPTIONINVALID              _MESSAGE(23090, _("ERROR! empty string argument to -N option invalid\n"))
#define MSG_PARSE_INVALIDOPTIONARGUMENTNOW_S              _MESSAGE(23091, _("invalid option argument \"-now %s\"\n"))
#define MSG_PARSE_WRONGSTDOUTPATHLISTFORMATXSPECTOOOPTION_S              _MESSAGE(23092, _("ERROR! Wrong stdout path list format \"%s\" specified to -o option\n"))
#define MSG_PARSE_PEOPTIONMUSTHAVEPENAMEARGUMENT              _MESSAGE(23093, _("ERROR! -pe option must have pe_name argument\n"))
#define MSG_PARSE_PEOPTIONMUSTHAVERANGEAS2NDARGUMENT              _MESSAGE(23094, _("ERROR! -pe option must have range as 2nd argument\n"))
#define MSG_PARSE_QOPTIONMUSTHAVEDESTIDLISTARGUMENT              _MESSAGE(23095, _("ERROR! -q option must have destination identifier list argument\n"))
#define MSG_PARSE_WRONGDESTIDLISTFORMATXSPECTOQOPTION_S              _MESSAGE(23096, _("ERROR! Wrong destination identifier list format \"%s\" specified to -q option\n"))
#define MSG_PARSE_QSARGSOPTIONMUSTBETERMINATEDWITHQSEND              _MESSAGE(23097, _("ERROR! -qs_args option must be terminated with -qs_end\n"))
#define MSG_PARSE_INVALIDOPTIONARGUMENTRX_S              _MESSAGE(23098, _("invalid option argument \"-r %s\"\n"))
#define MSG_PARSE_SCOPTIONMUSTHAVECONTEXTLISTARGUMENT              _MESSAGE(23099, _("ERROR! -sc option must have context list argument\n"))
#define MSG_PARSE_WRONGCONTEXTLISTFORMATSCX_S              _MESSAGE(23100, _("ERROR! Wrong context list format -sc \"%s\"\n"))
#define MSG_PARSE_SOPTIONMUSTHAVEPATHNAMEARGUMENT              _MESSAGE(23101, _("ERROR! -S option must have path_name argument\n"))
#define MSG_PARSE_WRONGSHELLLISTFORMATXSPECTOSOPTION_S              _MESSAGE(23102, _("ERROR! Wrong shell list format \"%s\" specified to -S option\n"))
#define MSG_PARSE_TOPTIONMUSTHAVEALISTOFTASKIDRANGES              _MESSAGE(23103, _("ERROR! -t option must have a list of task id ranges\n"))
#define MSG_PARSE_UOPTMUSTHAVEALISTUSERNAMES              _MESSAGE(23104, _("ERROR! -u option must have a list usernames\n"))
#define MSG_PARSE_VOPTMUSTHAVEVARIABLELISTARGUMENT              _MESSAGE(23105, _("ERROR! -v option must have variable list argument\n"))
#define MSG_PARSE_WRONGVARIABLELISTFORMATVORENVIRONMENTVARIABLENOTSET_S              _MESSAGE(23106, _("ERROR! Wrong variable list format -v \"%s\" or environment variable not set\n"))
#define MSG_PARSE_COULDNOTPARSEENVIRIONMENT              _MESSAGE(23107, _("ERROR! Could not parse environment\n"))
#define MSG_PARSE_INVALIDOPTIONARGUMENTWX_S              _MESSAGE(23108, _("invalid option argument \"-w %s\"\n"))
#define MSG_PARSE_ATSIGNOPTIONMUSTHAVEFILEARGUMENT       _MESSAGE(23109, _("ERROR! -@ option must have file argument\n"))
#define MSG_PARSE_NOJOBIDGIVENBEFORESEPARATOR              _MESSAGE(23111, _("ERROR! no job id given before -- separator\n"))
#define MSG_PARSE_OPTIONMUSTBEFOLLOWEDBYJOBARGUMENTS              _MESSAGE(23112, _("ERROR! -- option must be followed by job arguments\n"))
#define MSG_PARSE_WRONGJOBIDLISTFORMATXSPECIFIED_S              _MESSAGE(23113, _("ERROR! Wrong job id list format \"%s\" specified\n"))
#define MSG_PARSE_INVALIDPRIORITYMUSTBEINNEG1023TO1024              _MESSAGE(23114, _("ERROR! invalid priority, must be an integer from -1023 to 1024\n"))


/*
** parse_qconf.c
*/
#define MSG_QCONF_CANTCHANGEQUEUENAME_S           _MESSAGE(23115, _("%s: cannot change queuename\n"))
#define MSG_QCONF_CANTCHANGEHOSTNAME_S           _MESSAGE(23116, _("%s: cannot change hostname\n") )  
#define MSG_FILE_NOFILEARGUMENTGIVEN           _MESSAGE(23117, _("no file argument given\n"))
#define MSG_PARSE_EDITFAILED                   _MESSAGE(23118, _("edit failed\n"))
#define MSG_FILE_FILEUNCHANGED                 _MESSAGE(23119, _("file unchanged\n"))
#define MSG_FILE_ERRORREADINGINFILE            _MESSAGE(23120, _("error reading in file\n"))
#define MSG_EXEC_XISNOEXECHOST_S               _MESSAGE(23121, _("%s is no exec host\n"))
#define MSG_EXEC_ADDEDHOSTXTOEXECHOSTLIST_S    _MESSAGE(23122, _("added host %s to exec host list\n"))
#define MSG_ANSWER_INVALIDFORMAT               _MESSAGE(23123, _("invalid format\n"))
#define MSG_QUEUE_XISNOTAQUEUENAME_S           _MESSAGE(23124, _("%s is not a queuename\n"))
#define MSG_QUEUE_UNABLETOWRITETEMPLATEQUEUE   _MESSAGE(23125, _("unable to write template queue"))
#define MSG_TREE_CHANGEDSHARETREE              _MESSAGE(23126, _("changed sharetree\n"))
#define MSG_TREE_NOSHARETREE                   _MESSAGE(23127, _("No sharetree\n"))
#define MSG_TREE_CANTADDNODEXISNONUNIQUE_S     _MESSAGE(23128, _("Could not add node %s to sharetree due to non-unique path\n"))
#define MSG_TREE_SETTING                       _MESSAGE(23129, _("setting "))
#define MSG_TREE_REMOVING                      _MESSAGE(23130, _("removing ") ) 
#define MSG_ANSWER_XISNOTVALIDSEENODESHARESLIST_S _MESSAGE(23131, _("%s is not a valid argument, see node_shares_list format\n"))
#define MSG_TREE_MODIFIEDSHARETREE             _MESSAGE(23132, _("modified sharetree\n"))
#define MSG_TREE_NOMIDIFIEDSHARETREE           _MESSAGE(23133, _("no modifications to sharetree\n"))
#define MSG_ANSWER_NOLISTNAMEPROVIDEDTOAUX_S   _MESSAGE(23134, _("no list_name provided to \"-au %s\""))
#define MSG_ANSWER_NOLISTNAMEPROVIDEDTODUX_S   _MESSAGE(23135, _("no list_name provided to \"-du %s\""))
#define MSG_TREE_CANTDELROOTNODE               _MESSAGE(23136, _("can't delete root node\n"))
#define MSG_TREE_CANTDELNODESWITHCHILD         _MESSAGE(23137, _("can't delete nodes with children\n"))
#define MSG_ANSWER_XISNOTAVALIDOPTIONY_SU      _MESSAGE(23138, _("\"%s\" is not a valid option " U32CFormat "\n"))
#define MSG_COMPLEX_NEEDACOMPLEXNAME           _MESSAGE(23139, _("Need a complex Name"))
#define MSG_CALENDAR_XISNOTACALENDAR_S         _MESSAGE(23140, _("%s is not a calendar\n"))
#define MSG_CKPT_XISNOTCHKPINTERFACEDEF_S      _MESSAGE(23141, _("%s is not a checkpointing interface definition\n"))
#define MSG_EXEC_XISNOTANEXECUTIONHOST_S       _MESSAGE(23142, _("%s is not an execution host\n"))
#define MSG_EXEC_HOSTENTRYOFXCHANGEDINEXECLIST_S     _MESSAGE(23143, _("changed entry of host %s in execution host list\n"))
#define MSG_PARALLEL_XNOTAPARALLELEVIRONMENT_S     _MESSAGE(23144, _("%s is not a parallel environment\n"))
#define MSG_QUEUE_UNABLETOWRITEOLDQUEUE        _MESSAGE(23146, _("unable to write old queue"))
#define MSG_ANSWER_MISSINGFILENAMEASOPTIONARG_S   _MESSAGE(23147, _("%s: missing filename as option argument\n"))
#define MSG_QCONF_XISNOTAOBJECTATTRIB_SSS         _MESSAGE(23148, _("%s: \"%s\" is not a %s attribute\n"))
#define MSG_QCONF_XISNOTAQUEUEATTRIB_SS           _MESSAGE(23149, _("%s: \"%s\" is not a queue attribute\n"))
#define MSG_QCONF_XISNOTAEXECHOSTATTRIB_SS           _MESSAGE(23150, _("%s: \"%s\" is not a exec host attribute\n"))
#define MSG_QCONF_CANTCHANGEOBJECTNAME_SS         _MESSAGE(23151, _("%s: cannot change %s\n"))
#define MSG_QCONF_INTERNALFAILURE_S               _MESSAGE(23152, _("%s: internal failure\n"))
#define MSG_QCONF_MQATTR_MISSINGOBJECTLIST_S       _MESSAGE(23153, _("%s: missing object list\n"))
#define MSG_QCONF_MQATTR_MISSINGQUEUELIST_S       _MESSAGE(23154, _("%s: missing queue list\n"))
#define MSG_QCONF_MQATTR_MISSINGHOSTLIST_S       _MESSAGE(23155, _("%s: missing host list\n") )        
#define MSG_SCHEDD_CHANGEDSCHEDULERCONFIGURATION  _MESSAGE(23156, _("changed scheduler configuration\n"))
#define MSG_USER_XISNOKNOWNUSER_S                 _MESSAGE(23157, _("%s is not known as user\n"))
#define MSG_PROJECT_XISNOKNWOWNPROJECT_S          _MESSAGE(23158, _("%s is not known as project\n"))
#define MSG_ANSWER_NEEDHOSTNAMETODELLOCALCONFIG     _MESSAGE(23159, _("Need hostname to delete local configuration\n"))
#define MSG_ANSWER_INVALIDOPTIONARGX_S            _MESSAGE(23160, _("invalid option argument \"%s\"\n"))
#define MSG_SRC_X_HELP_USAGE_S                    _MESSAGE(23161, _("Usage: %s -help\n"))
#define MSG_FILE_EDITFILEXDOESNOTEXIST_S          _MESSAGE(23162, _("edit file %s does not exist"))
#define MSG_QCONF_EDITOREXITEDWITHERROR_I         _MESSAGE(23163, _("editor exited with error %d"))
#define MSG_QCONF_EDITFILEXNOLONGEREXISTS_S       _MESSAGE(23164, _("edit file %s no longer exists"))
#define MSG_QCONF_EDITORWASTERMINATEDBYSIGX_I     _MESSAGE(23165, _("editor was terminated by a signal %d\n"))
#define MSG_QCONF_CANTSTARTEDITORX_S              _MESSAGE(23166, _("can't start editor %s"))
#define MSG_QCONF_XADDEDTOYLIST_SS                _MESSAGE(23167, _("%s added to %s list\n"))
#define MSG_FILE_CANTCREATETEMPFILE               _MESSAGE(23168, _("could not generate temporary filename\n"))
#define MSG_QCONF_CANTREADCONFIG_S                _MESSAGE(23169, _("can't read configuration: %s\n"))
#define MSG_QCONF_CANTREADX_S                     _MESSAGE(23170, _("cant read %s\n"))
#define MSG_QCONF_CANTREADSHARETREEX_S            _MESSAGE(23171, _("cant read sharetree: %s\n"))
#define MSG_QCONF_NOXDEFINED_S                    _MESSAGE(23172, _("no %s defined\n"))
#define MSG_TABLE_HOST                            "HOST"
#define MSG_TABLE_PROCESSORS                      "PROCESSORS"
#define MSG_TABLE_ARCH                            "ARCH"
#define MSG_TABLE_SUM_F                           "SUM"
#define MSG_TABLE_EV_ID                           "ID"
#define MSG_TABLE_EV_NAME                         "NAME"
#define MSG_QCONF_NOEXECUTIONHOSTSDEFINED         _MESSAGE(23173, _("no execution hosts defined\n"))
#define MSG_QCONF_NOEVENTCLIENTSREGISTERED        _MESSAGE(23174, _("no event clients registered\n"))
#define MSG_QCONF_XISNOVALIDCOMPLEXNAME_S         _MESSAGE(23175, _("%s is no valid complex name\n"))
#define MSG_ANSWER_MALFORMEDREQUEST               _MESSAGE(23176, _("malformed request\n"))
#define MSG_ANSWER_COMPLEXXALREADYEXISTS_S        _MESSAGE(23177, _("complex %s already exists\n"))
#define MSG_ANSWER_USERMAPENTRYALREADYEXISTS_S    _MESSAGE(23178, _("user mapping for %s already exists\n"))
#define MSG_ANSWER_HOSTGROUPENTRYALLREADYEXISTS_S _MESSAGE(23179, _("host group '%s' already exists\n"))
#define MSG_ANSWER_CLUSTERUNAMEXDIFFFROMY_SS      _MESSAGE(23180, _("\"cluster_user\" name entry '%s' does not match filename '%s'\n"))
#define MSG_UM_NOCLUSTERUSERENTRYINFILEX_S        _MESSAGE(23181, _("no \"cluster_user\" name entry in file '%s'\n"))
#define MSG_ANSWER_HOSTGROUPNAMEXDIFFFROMY_SS     _MESSAGE(23182, _("\"group_name\" entry '%s' does not match filename '%s'\n"))
#define MSG_HGRP_NOHOSTGROUPENTRYINFILEX_S        _MESSAGE(23183, _("no \"group_name\" name entry in file '%s'\n"))
#define MSG_ANSWER_UNKNOWNHOSTORGROUPNAME_S       _MESSAGE(23184, _("unknown host or group name '%s'\n"))
#define MSG_ANSWER_UNKNOWNHOSTNAME_S              _MESSAGE(23185, _("unknown host name '%s'\n"))
#define MSG_ANSWER_UNKNOWNGROUPNAME_S             _MESSAGE(23186, _("unknown group name '%s'\n"))
#define MSG_ANSWER_SUBGROUPHASNOSUPERGROUP_SS      _MESSAGE(23187, _("subgroup '%s' has no supergroup '%s'\n"))
#define MSG_ANSWER_SUBGROUPXHASLINKTOGROUPY_SS       _MESSAGE(23188, _("sub group '%s' has sub group link to group '%s' (deadlock)\n"))
#define MSG_ANSWER_SUPERGROUPHASNOSUBGROUP_SS      _MESSAGE(23189, _("supergroup '%s' has no subgroup '%s'\n"))
#define MSG_ANSWER_CLUSTERUNAMEXNOTAMBIGUOUSMAPFORYATZ_SSS _MESSAGE(23190, _("cluster_user name entry '%s' has not ambiguous mappings for '%s' at '%s'\n"))
#define MSG_ANSWER_DUPLICATEDMAPPINGENTRY_SSS     _MESSAGE(23191, _("mapping entry as user '%s' from '%s' already exists for cluster user '%s'\n"))
#define MSG_ANSWER_IGNORINGMAPPINGFOR_S           _MESSAGE(23192, _("usermapping for '%s' not accepted\n"))
#define MSG_ANSWER_IGNORINGHOSTGROUP_S            _MESSAGE(23193, _("host group list '%s' not accepted\n"))
#define MSG_ANSWER_USERMAPENTRYXNOTFOUND_S        _MESSAGE(23194, _("user mapping entry for user '%s' not found\n"))
#define MSG_ANSWER_HOSTGROUPENTRYXNOTFOUND_S      _MESSAGE(23195, _("host group entry for group '%s' not found\n"))
#define MSG_ANSWER_COMPLEXXDOESNOTEXIST_S         _MESSAGE(23196, _("complex %s does not exist.\n"))
#define MSG_MULTIPLY_MODIFIEDIN                   _MESSAGE(23197, _("modified in"))
#define MSG_MULTIPLY_ADDEDTO                      _MESSAGE(23198, _("added to"))
#define MSG_MULTIPLY_DELETEDFROM                  _MESSAGE(23199, _("deleted from"))
#define MSG_ANSWER_XYCOMPLEXLIST_SS               _MESSAGE(23200, _("%s %s complex list\n"))
#define MSG_FILE_ERRORWRITINGUSERSETTOFILE        _MESSAGE(23201, _("error writing userset to file\n"))
#define MSG_FILE_ERRORREADINGUSERSETFROMFILE_S    _MESSAGE(23202, _("error reading userset from file %s\n"))
#define MSG_COMPLEX_COMPLEXXNOTDEFINED_S          _MESSAGE(23203, _("complex %s not defined\n"))
#define MSG_ERROR_USERXNOTDEFINED_S               _MESSAGE(23204, _("user %s not defined\n"))
#define MSG_ERROR_GROUPXNOTDEFINED_S              _MESSAGE(23205, _("host group '%s' is not defined\n"))
#define MSG_ANSWER_CONFIGXNOTDEFINED_S            _MESSAGE(23206, _("configuration %s not defined\n"))
#define MSG_ANSWER_CONFIGXALREADYEXISTS_S         _MESSAGE(23207, _("configuration %s already exists\n"))
#define MSG_ANSWER_CONFIGXDOESNOTEXIST_S          _MESSAGE(23208, _("configuration %s does not exist\n"))
#define MSG_ANSWER_CONFIGUNCHANGED                _MESSAGE(23209, _("configuration unchanged\n"))
#define MSG_ANSWER_ERRORREADINGTEMPFILE           _MESSAGE(23210, _("error reading temp file\n"))
#define MSG_ANSWER_ERRORREADINGCONFIGFROMFILEX_S  _MESSAGE(23211, _("error reading configuration from file %s\n"))
#define MSG_ANSWER_DENIEDHOSTXISNOADMINHOST_S     _MESSAGE(23212, _("denied: host \"%s\" is no admin host\n"))


/*
** sge_sharetree.c
*/
#define MSG_TREE_UNABLETOLACATEXINSHARETREE_S     _MESSAGE(23213, _("Unable to locate %s in sharetree\n"))
#define MSG_OBJ_NOSTREEELEM           _MESSAGE(23214, _("no sharetree element"))
#define MSG_STREE_UNEXPECTEDNAMEFIELD  _MESSAGE(23215, _("unexpected name field"))
#define MSG_STREE_UNEXPECTEDTYPEFIELD  _MESSAGE(23216, _("unexpected type field"))
#define MSG_STREE_UNEXPECTEDVERSIONFIELD  _MESSAGE(23217, _("unexpected version field"))
#define MSG_STREE_UNEXPECTEDSHARESFIELD  _MESSAGE(23218, _("unexpected shares field"))
#define MSG_STREE_UNEXPECTEDCHILDNODEFIELD  _MESSAGE(23219, _("unexpected childnodes field"))
#define MSG_STREE_NAMETWICE_I         _MESSAGE(23220, _("double defined name in line %d"))
#define MSG_STREE_TYPETWICE_I         _MESSAGE(23221, _("double defined type in line %d"))
#define MSG_STREE_SHARESTWICE_I       _MESSAGE(23222, _("double defined shares in line %d"))
#define MSG_STREE_CHILDNODETWICE_I     _MESSAGE(23223, _("double defined child node list in line %d"))
#define MSG_STREE_NOFATHERNODE_U      _MESSAGE(23224, _("could not find father node for node "U32CFormat))
#define MSG_STREE_NOPARSECHILDNODES   _MESSAGE(23225, _("error parsing child node list"))
#define MSG_STREE_NOPARSELINE_I       _MESSAGE(23226, _("error parsing line %d"))
#define MSG_STREE_NOVALIDNODEREF_U    _MESSAGE(23227, _("found reference to node " U32CFormat " but no specification"))
#define MSG_FILE_FILEEMPTY            _MESSAGE(23228, _("empty file"))


/*
** sched_conf.c
*/
#define MSG_SCHEDCONF_ALPPISNULLNOSCHEDULERCONFIGURATION    _MESSAGE(23229, _("alpp == NULL - no scheduler configuration"))
#define MSG_ANSWER_NOSCHEDULERCONFIGURATION    _MESSAGE(23230, _("no scheduler configuration"))
#define MSG_SCHEDCONF_CANTOPENSCHEDULERCONFIGURATIONFILE_SS    _MESSAGE(23231, _("cant open scheduler configuration file \"%s\": %s\n"))
#define MSG_SCHEDCONF_INVALIDVALUEXFORQUEUESORTMETHOD_S    _MESSAGE(23232, _("invalid value \"%s\" for queue_sort_method"))
#define MSG_SCHEDCONF_CANTCREATESCHEDULERCONFIGURATION    _MESSAGE(23233, _("can't create scheduler configuration\n"))

/* read_write_job.c */
#define MSG_JOB_CANNOT_REMOVE_SS       _MESSAGE(23234, _("can not remove "SFN": "SFN"\n"))
#define MSG_JOB_TASK_SPOOL_FILE        _MESSAGE(23235, _("task spool file"))
#define MSG_JOB_JOB_SPOOL_FILE         _MESSAGE(23236, _("job spool file"))
#define MSG_JOB_JOB_SPOOL_DIRECTORY    _MESSAGE(23237, _("job spool directory"))


/* read_write_ume.c */
#define MSG_UM_CONFIGTEXT1 _MESSAGE(23238, _("# configuration file for user mapping\n"))
#define MSG_UM_CONFIGTEXT2 _MESSAGE(23239, _("#\n"))
#define MSG_UM_CONFIGTEXT3 _MESSAGE(23240, _("# cluster_user      NAME -> unique cluster user name\n"))
#define MSG_UM_CONFIGTEXT4 _MESSAGE(23241, _("# MAPPING_USER           -> User mapping for specified hosts/host groups\n"))
#define MSG_UM_CONFIGTEXT5 _MESSAGE(23242, _("# MAPPINGHOST-/GROUPLIST -> [,] separated host or host group names\n"))
#define MSG_UM_CONFIGTEXT6 _MESSAGE(23243, _("# MAPPING_USER      MAPPINGHOST-/GROUPLIST\n"))
/* read_write_host_group.c */
#define MSG_HOSTGROUP_CONFIGTEXT1 _MESSAGE(23244, _("# configuration file for host groups\n"))
#define MSG_HOSTGROUP_CONFIGTEXT2 _MESSAGE(23245, _("#\n"))
#define MSG_HOSTGROUP_CONFIGTEXT3 _MESSAGE(23246, _("# group_name      NAME -> unique host group name\n"))
#define MSG_HOSTGROUP_CONFIGTEXT4 _MESSAGE(23247, _("# each line contains a hostname\n"))
#define MSG_HOSTGROUP_CONFIGTEXT5 _MESSAGE(23248, _("# if the first character is a '@' the following name\n"))
#define MSG_HOSTGROUP_CONFIGTEXT6 _MESSAGE(23249, _("# is resolved as subgroup name\n"))



/*
** complex.c
*/
#define MSG_COMPLEX_STARTSCOMMENTBUTNOSAVE _MESSAGE(23250, _("#--- # starts a comment but comments are not saved across edits -----------------------\n"))
#define MSG_PARSE_CANTPARSECPLX_S     _MESSAGE(23251, _("parse error in file %s\n"))
#define MSG_PARSE_CANTPARSECPLX_SI    _MESSAGE(23252, _("parse error in file %s in line %d\n"))
#define MSG_PARSE_INVALIDCPLXTYPE_SS    _MESSAGE(23253, _("parse error in file %s, unknown type %s\n"))
#define MSG_PARSE_INVALIDCPLXRELOP_SS   _MESSAGE(23254, _("parse error in file %s, invalid relation operator %s\n"))
#define MSG_PARSE_INVALIDCPLXCONSUM_SSS _MESSAGE(23255, _("error in file %s: attribute %s may not be consumable because it is of type %s\n"))
#define MSG_PARSE_INVALIDCPLXENTRY_SSS  _MESSAGE(23256, _("parse error in file %s, invalid %s entry %s\n"))
#define MSG_PARSE_INVALIDCPLXREQ_SS     _MESSAGE(23257, _("parse error in file %s, invalid requestable entry %s\n"))
#define MSG_COM_COMMDLOCKED           _MESSAGE(23258, _("may be commd is locked:\n"))
#define MSG_CPLX_VALUEMISSING_S       _MESSAGE(23259, _("denied: missing value for request "SFQ"\n"))
#define MSG_CPLX_WRONGTYPE_SSS        _MESSAGE(23260, _("wrong type in \"%s=%s\" - %s expected\n"))
#define MSG_SGETEXT_INVALIDHOST_S               _MESSAGE(23261, _("invalid hostname "SFQ"\n"))
#define MSG_CPLX_ATTRIBISNEG_S        _MESSAGE(23262, _("attribute "SFQ" is consumable but has a negative value\n"))
#define MSG_SGETEXT_UNKNOWN_RESOURCE_S          _MESSAGE(23263, _("unknown resource "SFQ"\n"))
#define MSG_SGETEXT_RESOURCE_NOT_REQUESTABLE_S        _MESSAGE(23264, _("resource "SFQ" configured as non requestable\n") )    
#define MSG_SGETEXT_UNKNOWN_ATTR_TYPE_U         _MESSAGE(23265, _("unknown complex attribute type " U32CFormat  "\n"))


/*
** read_object.c
*/
#define MSG_MEMORY_CANTMALLOCBUFFERFORXOFFILEY_SS    _MESSAGE(23266, _("can't malloc buffer for %s of file \"%s\""))
#define MSG_FILE_CANTDETERMINESIZEFORXOFFILEY_SS    _MESSAGE(23267, _("can't determine size for %s of file \"%s\""))
#define MSG_SGETEXT_UNKNOWN_CONFIG_VALUE_SSS          _MESSAGE(23268, _("unknown attribute "SFQ" in "SFN" configuration in file "SFN"\n") ) 

/*
** read_write_cal.c
*/
#define MSG_TMPNAM_GENERATINGTMPNAM    _MESSAGE(23269, _("generating tmpnam()"))
#define MSG_FILE_ERRORWRITING_S        _MESSAGE(23270, _("error writing %s\n"))

/*
** read_write_host.c
*/
#define MSG_FILE_ERRORWRITING_SS    _MESSAGE(23271, _("error writing %s, %s\n"))
#define MSG_FILE_ERRORWRITINGHOSTNAME    _MESSAGE(23272, _("error writing hostname\n"))


/*
** read_write_queue.c
*/
#define MSG_ANSWER_GETUNIQUEHNFAILEDRESX_SS  _MESSAGE(23274, _("getuniquehostname() failed resolving %s: %s\n"))

/*
** read_write_userprj.c
*/
#define MSG_RWUSERPRJ_EPISNULLNOUSERORPROJECTELEMENT    _MESSAGE(23275, _("ep == NULL - no user or project element"))
#define MSG_PROJECT_FOUNDPROJECTXTWICE_S    _MESSAGE(23276, _("found project %s twice"))
#define MSG_PROJECT_INVALIDPROJECTX_S    _MESSAGE(23277, _("invalid project %s"))
#define MSG_JOB_FOUNDJOBWITHWRONGKEY_S    _MESSAGE(23278, _("found job with wrong key \"%s\""))
#define MSG_JOB_FOUNDJOBXTWICE_U    _MESSAGE(23279, _("found job "U32CFormat" twice"))

/*
** read_write_userset.c
*/
#define MSG_USERSET_NOUSERETELEMENT    _MESSAGE(23280, _("no userset element"))


/*
** get_conf.c
*/
#define MSG_CONF_GETCONF_S            _MESSAGE(23281, _("getting configuration: %s"))
#define MSG_CONF_REQCONF_II           _MESSAGE(23282, _("requested %d configurations, got %d"))
#define MSG_CONF_NOGLOBAL             _MESSAGE(23283, _("global configuration not defined"))
#define MSG_CONF_NOLOCAL_S            _MESSAGE(23284, _("local configuration %s not defined - using global configuration"))
#define MSG_CONF_NOCONFBG             _MESSAGE(23285, _("can't get configuration from qmaster -- backgrounding"))
#define MSG_CONF_NOCONFSLEEP          _MESSAGE(23286, _("can't get configuration from qmaster -- sleep(10)"))
#define MSG_CONF_NOCONFSTILL          _MESSAGE(23287, _("still can't get configuration from qmaster -- trying further"))
#define MSG_CONF_NOREADCONF_IS        _MESSAGE(23288, _("Error %d reading configuration \"%s\"\n"))
#define MSG_CONF_NOMERGECONF_IS       _MESSAGE(23289, _("Error %d merging configuration \"%s\"\n"))


/*
** sge_conf.c
*/
#define MSG_GDI_USING_SS              _MESSAGE(23290, _("using \"%s\" for %s\n"))
#define MSG_GDI_NOCONFIGFROMMASTER    _MESSAGE(23291, _("could not get configuration from qmaster - using defaults\n"))
#define MSG_GDI_NEITHERSGECODGRDSETTINGSGE _MESSAGE(23292, _("neither SGE nor COD nor GRD environment for jobs configured, setting SGE environment\n"))
#define MSG_GDI_INVALIDPOLICYSTRING   _MESSAGE(23293, _("Invalid policy hierachy string. Disabling policy hierachy.\n"))


#endif /* __MSG_COMMON_H */

