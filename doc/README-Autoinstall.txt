       Detailed Information about the new installation script "inst_sge"
       -----------------------------------------------------------------

Content:
--------
1.  Common Info
2.  Preparations
3.  Automatic Installation
4.  Un-installation
4.1 Un-installation Execd
4.2 Un-installation Qmaster
5.  Copyright

1. Common Info:
---------------

   The new install script supports an automatic installation mode. The
   "inst_sge" script can be found in the $SGE_ROOT directory.

   The modules of the installation script and a auto install configuration
   template are in $SGE_ROOT/util/install_modules.

   The combination of csp (increased security) and autoinstall is currently
   not supported.


2. Preparations:
----------------

   Make sure, what kind of installation you want to use (Interactive or
   automatic). For automatic installation you have to configure a
   configuration file.

   A template can be found in $SGE_ROOT/$SGE_CELL/util/install_modules. (For
   details see 3.1 Automatic Installation) If you want to use automatic
   installation, it is mandatory, that root has rsh or ssh access to the
   hosts, which should be installed, because during the installation, the
   script tries to execute itself on the remote hosts. This happens over rsh
   or ssh.

   For Berkeley DB spooling you can decide, if you want to run a separated
   Berkeley DB spooling server or local spooling.

   In case of using a Berkeley DB server you have to choose a host, where
   your Berkeley DB is running on. On this host, rpc services have to be
   installed and configured.

   For local spooling no special configuration have to done. It necessary,
   that the directory, where Berkeley DB spools is local. It's not allowed
   to use a NFS mounted directory -> the spooling will fail! This is a
   limitation of Berkeley DB!

   If you are going to setup a separated Berkeley DB Spooling Server, please
   install the Server on the preferred host with ./inst.sge -db and than
   start the automatic installation.


3. Automatic Installation
-------------------------

   During the automatic installation, the user is not asked for any
   question.  No message will be displayed on the terminal After the
   installation you will find a log file in $SGE_ROOT/$SGE_CELL/spool. The
   name is: install_hostname_<combination of date time>.log Here you can
   find information about the used configuration and about errors during
   installation. In case of serious errors, it can be impossible for the
   installation script to move the log file into the spool directory, then
   please have a look into the /tmp/ dir. There you can also find the
   install logs

   For a automatic installation it is recommended, that root has the right
   to connect via rsh/ssh to the different hosts, without asking for a
   password.  If this is not allowed in your network, the automatic
   installation won't work remotely. In this case it is possible to login to
   the host and start the automatic installation again.

   Example:

   On master host:

      % ./inst_sge -m -auto /tmp/install_config_file.conf

   Then login to the execution host and enter

      % ./inst_sge -x -noremote -auto /tmp/install_config_file.conf

   First, the master host will be installed automatically, using the config
   file Second, the execution host will be installed automatically using the
   config file. The -noremote switch ensures, that the install script
   doesn't try the rsh/ssh'ing to other host's which are configured within
   the config file. (e.g. EXECD_HOST_LIST="host1 host2 ...."

   Another point is the configuration file. Before the installation is
   started, the configuration file has to be adapted to your requirements. A
   template of this file can be found in

      $SGE_ROOT/$SGE_CELL/util/install_modules/inst_template.conf

   Please copy this template and make your changes.

   The template files looks like this:

#-------------------------------------------------
# SGE default configuration file
#-------------------------------------------------

# SGE_ROOT Path, this is basic information
SGE_ROOT="Please enter path"

# SGE_QMASTER_PORT is used by qmaster for communication
SGE_QMASTER_PORT="Please enter port"

# SGE_EXECD_PORT is used by execd for communication
SGE_EXECD_PORT="Please enter port"

# CELL_NAME, will be a dir in SGE_ROOT, contains the common dir
CELL_NAME="default"

# The dir, where qmaster spools this parts, which are not spooled by DB
QMASTER_SPOOL_DIR="Please, enter spooldir"

# The dir, where the execd spools (active jobs)
EXECD_SPOOL_DIR="Please, enter spooldir"

# The dir, where the execd spools (local configuration)
EXECD_SPOOL_DIR_LOCAL="Please, enter spooldir"

# If SGE is compiled with -spool-dynamic, you have to enter here, which
# spooling method should be used. (classic or berkeleydb)
SPOOLING_METHOD="berkeleydb"

# Name of the Server, where the Spooling DB is running on
DB_SPOOLING_SERVER="none"

# The dir, where the DB spools
DB_SPOOLING_DIR="spooldb"

# If true, the domain names will be ignored, during the hostname resolving
HOSTNAME_RESOLVING="true"

# Shell, which should be used for remote installation (rsh/ssh)
SHELL_NAME="ssh"

# Enter your default domain, if you are using /etc/hosts or NIS configuration
DEFAULT_DOMAIN="none"

# I don't know any description at the moment (eg. 16000-16100)
GID_RANGE="Please, enter GID range"

# If a job stops, fails, finish, you can send a mail to this address
ADMIN_MAIL="none"

# If true, the rc scripts (sgemaster, sgeexecd, sgebdb) will be added,
# to start automatically during boot time
ADD_TO_RC="false"

#If this is "true" the file permissions of executables will be set to 755
#and of ordinary file to 644.  
SET_FILE_PERMS="true"

# This option is not fully implemented, yet.
# When a exec host should be un-installed, the running jobs will be rescheduled
RESCHEDULE_JOBS="wait"

# Enter a one of the three distributed scheduler tuning configuration sets
# (1=normal, 2=high, 3=max)
SCHEDD_CONF="1"

# A List of Host which should become admin hosts
ADMIN_HOST_LIST="host1 host2 host3 host4"

# A List of Host which should become submit hosts
SUBMIT_HOST_LIST="host1 host2 host3 host4"

# A List of Host which should become exec hosts
EXEC_HOST_LIST="host1 host2 host3 host4"

# The name of the shadow host. This host must have read/write permission
# to the qmaster spool directory
SHADOW_HOST="hostname"

# Remove this execution hosts in automatic mode
EXEC_HOST_LIST_RM="host1 host2 host3 host4"



   After writing a valid configuration file you can start the installation
   with this options:

   % inst_sge -m -x -auto path/to/configfile       
      --> This installs master locally, execution host locally and remotely
          as configured in EXEC_HOST_LIST

   % inst_sge -m -x -sm -auto path/to/configfile   
      --> This installs a additional Shadow Master Host as configured in
          SHADOW_HOST

4. Un-installation
------------------

   An other new feature is the un-installation of Grid Engine It is also
   possible to do this automatic mode. It is recommended to un-install the
   execution hosts first. If you try to un-install the qmaster first, you
   have to un-install all execution hosts manually.

   To make sure, that you have a clean environment, please always source the
   settings.csh file, which is located in $SGE_ROOT/$SGE_CELL/common

   Using tcsh:

      % source $SGE_ROOT/$SGE_CELL/common/settings.csh

   Using the Bourne shell:

      % . $SGE_ROOT/$SGE_CELL/common/settings.sh


4.1 Execution host un-installation
----------------------------------

   During the execution host un-installation all host configuration will be
   removed. So if you are not sure about the the un-installation than please
   make a backup of your configuration.

   The un-installation goes step by step and tries to stop the exec hosts in
   a gentle way. First of all, the queue which are concerned of the
   un-installation will be disabled. So we can be sure, that no new jobs
   will be scheduled. Than, the script tries to checkpoint, reschedule,
   forced rescheduling running jobs. After this action, the queue should be
   empty.  The execution host will be shutdown and the configuration, global
   spool directory, local directory will be removed.

   To start the automatic un-installation of execution hosts enter the
   following command:

      % ./inst_sge -ux -auto "path/to/filename"

   During the automatic un-installation no terminal output will be produced.
   Be sure to have a well configured config file (see. 3.1). The entry:

   # Remove this execution hosts in automatic mode EXEC_HOST_LIST_RM="host1
   host2 host3 host4"

   is used for the un-installation. Every host, which is in the
   EXEC_HOST_LIST_RM will be automatically removed from the cluster.
 

4.2 Master host un-installation
-------------------------------

   The master host un-installation will remove all of our configuration of
   the cluster. After this procedure only the binary installation will be
   left.

   Please be careful by using this functionality. To make sure, that no
   configuration will become lost, make backups. The master host
   un-installation also supports the interactive and automatic mode.

   To start the automatic un-installation please enter the following
   command:

      % inst_sge -um -auto /path/to/config

   This mode is the same as in interactive mode. But the user won't be asked
   anymore. If the uninstall process is started, it can't be stopped
   anymore.  All terminal output will be suppressed!

5. Copyright
------------
___INFO__MARK_BEGIN__                                                       
The Contents of this file are made available subject to the terms of the Sun
Industry Standards Source License Version 1.2

Sun Microsystems Inc., March, 2001

Sun Industry Standards Source License Version 1.2
=================================================

The contents of this file are subject to the Sun Industry Standards Source
License Version 1.2 (the "License"); You may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://gridengine.sunsource.net/Gridengine_SISSL_license.html             

Software provided under this License is provided on an "AS IS" basis,   
WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.

See the License for the specific provisions governing your rights and
obligations concerning the Software.                                 

The Initial Developer of the Original Code is: Sun Microsystems, Inc.

Copyright: 2001 by Sun Microsystems, Inc.                     

All Rights Reserved.
___INFO__MARK_END__                                                  
