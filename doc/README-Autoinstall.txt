Detailed Information about the new installation script: inst_sge

Content:

1.      Common Info
2.      Preparations
3.      Installation
3.1     Automatic Installation
4.      Uninstallation
4.1     Uninstallation Execd
4.2     Uninstallation Qmaster


1. Common Info:

The new install script supports an automatic installation mode


The inst_sge script can be found in $SGE_ROOT.
The modules of installation script and a autoinstall configuration template are in $SGE_ROOT/util/install_modules.

The combination of csp (increased security) and autoinstall is not supported


2. Preparations:

Make sure, what kind of installation you want to use. (Interactive or automatic)

For automatic installation you have to configure a configuration file.
A template can be found in $SGE_ROOT/util/inst_sge_modules. (For details see 3.1 Automatic Installation)
If you want to use automatic installation, it is mandatory, that root has rsh or ssh access to the hosts, which
should be installed, because during the installation, the script tries to execute itself on the remote hosts.
This happens over rsh or ssh.

For Berkeley DB spooling you can decide, if you want to run a separated Berkeley DB spooling server or
local spooling.

In case of using a Berkeley DB server you have to choose a host, where your Berkeley DB is running on.
On this host, rpc services have to be installed and configured.

For local spooling no special configuration have to done. It necessary, that the directory,
where Berkeley DB spools is local. It's not allowed to use a NFS mounted directory -> the spooling will fail!
This is a limitation of Berkeley DB!

If you are going to setup a seperated Berkeley DB Spooling Server, please install the Server on
the prefered host with ./inst.sge -db and than start the automatic installation.


3. Installation


3.1 Automatic Installation

During the automatic installation, the user is not asked for any question. No message will be displayed on the terminal
After the installation you will find a log file in $SGE_ROOT/$SGE_CELL/common. The name is: install<combination of date time>.log
Here you can find information about the used configuration and about errors during installation.

For a automatic installation it is recommended, that root has the right to connect via rsh/ssh to the different hosts,
without asking for a password. If this is not allowed in your network, the automatic installation won't work.

Another point is the configuration file. Before the installation is started, the configuration file has to be adapted to
your requirements. A template of this file can be found in $SGE_ROOT/$SGE_CELL/util/install_modules/inst_template.conf

Please copy this template and make your changes.




The template looks like this:

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

# The directory, where qmaster spools (eg act_qmaster file) 
QMASTER_SPOOL_DIR="Please, enter spooldir"

# The directory, where the execd spools (active jobs)
EXECD_SPOOL_DIR="Please, enter spooldir"

# The directory, where the execd spools (local configuration)
EXECD_SPOOL_DIR_LOCAL="Please, enter spooldir"

# Name of the Server, where the Spooling DB is running on, in
#case of no server please enter "none"
DB_SPOOLING_SERVER="servername"

# The directory, where the DB spools. This must be a local directory
DB_SPOOLING_DIR="path/to/spooldb/dir"

# If true, the domainnames will be ignored, during the hostname resolving
HOSTNAME_RESOLVING="true"

# Shell, which should be used for remote installation (rsh/ssh)
SHELL_NAME="rsh"

# Enter your default domain, if you are using /etc/hosts or NIS configuration
DEFAULT_DOMAIN="none"

# Please enter your Group id range here (e.g. 16000-16100)
GID_RANGE="Please, enter GID range"

# If a job stops, fails, finish, you can send a mail to this address
ADMIN_MAIL="none"

# If true, the rc scripts (sgemaster, sgeexecd, sgebdb) will be added,
# to start automatically during boottime
ADD_TO_RC="false"

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


After writing a valid configuration file you can start the installation with this
options:

inst_sge -m -x -auto path/to/configfile       This installs master locally, execution host locally and remotely as configured
                                              in EXEC_HOST_LIST

inst_sge -m -x -sm -auto path/to/configfile   This installs a additional Shadow Master Host as configured in SHADOW_HOST



4. Uninstallation

An other new feature is the uninstallation of sge.
It is also possible to do this automatic.
It is recommended to uninstall the execution hosts first.
If you try to uninstall the qmaster first, you have to uninstall all executionhosts by hand.


To make sure, that you have a clean environment, please always source the settings.csh file,
which is located in $SGE_ROOT/$SGE_CELL/common

Using tcsh:  source $SGE_ROOT/$SGE_CELL/common/settings.csh  

Using bourne shell: . $SGE_ROOT/$SGE_CELL/common/settings.sh


4.1 Exechost uninstallation

During the execution host uninstallation all hostconfiguration will be removed.
So if you are not sure about the the uninstallation than please make a backup of
your configuration.

The uninstallation goes step by step and tries to stop the exechosts in a gentle way.
First of all, the queue which are concerned of the uninstallation will be disabled.
So we can be sure, that no new jobs will be scheduled. Than, the script tries to
checkpoint, reschedule, forced rescheduling running jobs.
After this action, the queue should be empty. 
The execution host will be shutdown and the configuration, global spooldirectory,
local directory will be removed.


To start the automatic uninstallation of execution hosts enter the following command:

./inst_sge -ux -auto "path/to/filename"

During the automatic uninstallation no terminal output will be produced.
Be sure to have a well configured config file (see. 3.1)
The entry:

# Remove this execution hosts in automatic mode
EXEC_HOST_LIST_RM="host1 host2 host3 host4"

is used for the uninstallation. Every host, which is in the EXEC_HOST_LIST_RM will be automatically
removed from the cluster.  
 


4.2 Masterhost uninstallation

The masterhost uninstallation will remove all of our configuration of the cluster.
After this procedure only the binary installation will be left.

Please be careful by using this functionality. To make sure, that no configuration will become lost,
make backups.

The masterhost uninstallation also supports the interactive and automatic mode.


To start the automatic uninstallation please enter the following command:

inst_sge -um -auto /path/to/config

This mode is the same as in interactive mode. But the user won't be asked anymore.
If the uninstall process is started, it can't be stopped anymore.
All terminal output will be suppressed!


