Upgrading from a Previous Release of Grid Engine Software

This chapter describes the steps necessary to upgrade your existing software to this release. You can upgrade from either Sun ONE Grid Engine 5.3 or Sun ONE Grid Engine, Enterprise Edition, 5.3. 

About Upgrading the Software 

The upgrade procedure is non destructive. The upgrade procedure installs the N1 Grid Engine 6 software on the master host by using the cluster configuration information from the older version of the software. The older version of the software will not be removed or modified in any way.   

How to Upgrade the Software 

Plan the Installation  
This procedure assumes that you have already extracted the grid engine software
1. Log in to the master host as root. 
2. Load the distribution files. 
3. Ensure that you have set the $SGE_ROOT environment variable by typing: 

# echo $SGE_ROOT   If the $SGE_ROOT environment variable is not set, set it now, by typing: 

# SGE_ROOT=sge-root; export SGE_ROOT 

4. Change to the installation directory, sge-root.   
If the directory where the installation files reside is visible from the master host, change directories (cd) to the installation directory sge-root, and then proceed to Step 4.   
If the directory is not visible and cannot be made visible, do the following: 
a. Create a local installation directory, sge-root, on the master host. 
b. Copy the installation files to the local installation directory sge-root across the network (for example, by using ftp or rcp). 
c. Change directories (cd) to the local sge-root directory. 
5. Run the upgrade command on the master host, and respond to the prompts. This command starts the master host installation procedure. 
You are asked several questions, and you might be required to run some administrative actions. 

The syntax of the command is inst_sge -upd <5.3_sge-root_directory> <5.3_SGE_CELL_name>. 
In the following example, the 5.3 sge-root directory is /sge/gridware and the cell name is default. 

# ./inst_sge -upd /sge/gridware default 

7. Verify the sge-root directory setting. 
8. Set up the TCP/IP services for the grid engine software. 
9. Enter the name of your cell(s). 
10. Specify a spool directory. 
11. Set the correct file permissions. 

12. Specify whether all of your grid engine system hosts are located in a single DNS domain.
13. Specify whether you want to use classic spooling or Berkeley DB. 14. Enter a group ID range 
15. Verify the spooling directory for the execution daemon. 
16. Enter the email address of the user who should receive problem reports.

Once you answer this question, the installation process is nearly complete. 

Several screens of information will be displayed before the script exits. 
The commands that are noted in those screens are also documented in the N1GE6 Installation Guide. The upgrade process uses your existing configuration to customize the installation. 

You will see output similar to the following: 

Creating >act_qmaster< file 
Creating >sgemaster< script 
Creating >sgeexecd< script 
creating directory: /tmp/centry 
Reading in complex attributes. 
Reading in administrative hosts. 
Reading in execution hosts. 
Reading in submit hosts. 
Reading in users: 
User "as114086". 
User "md121042". 
Reading in usersets: 
Userset "defaultdepartment". 
Userset "deadlineusers". 
Userset "admin". 
Userset "bchem1". 
Reading in calendars: 
Calendar "always_disabled". 
Calendar "always_suspend". 
Reading in projects: Project "ap1". Project "ap2". 
Reading in parallel environments: PE "bench_tight". PE "make". 
Creating settings files for >.profile/.cshrc< 

Caution   Do not rename any of the binaries of the distribution. 
If you use any scripts or tools in your cluster that monitor the daemons, make sure to check for the new names.
Eg: loadsensor scripts. The path's to this scripts will be upgraded,
so if you have a configured loadsensor in you old cluster. It will
be also configured in the new cluster. But the script still lies in your old installation. In case of a deinstallation of the old cluster the loadsensor script will be also deleted. Make sure to copy you loadsensors to the new installation.

18. Create the environment variables for use with the grid engine software. 

Note   For the following examples, if you have only a single grid engine system cluster, the value of cell is default.   If you are using a C shell, type the following command: 

% source sge-root/cell/common/settings.csh   

If you are using a Bourne shell or Korn shell, type the following command: 

$ . sge-root/cell/common/settings.sh 

19. Install or upgrade the execution hosts. 
There are two ways that you can install the N1 Grid Engine 6 software on your execution hosts: installation or upgrade. 

You need to log into each execution host, and run the following command: 

# sge-root/inst_sge -x -upd   Install the software on the execution host. 

a. If you only have a few execution hosts, you can install them interactively. You need to log into each execution host, and run the following command: 

# sge-root/inst_sge -x -upd 

This will install a execution host without creating a local configuration for this execution host. This is useful, to make sure, that the upgraded hostconfiguration won't be overwritten.
If you want to overwrite the host configurations (which was take by the upgrade) you can use this command:

#  sge-root/inst_sge -x

b. If you have a large number of execution hosts, you should consider installing them non-interactively. 

CAUTION: If you use the automatic execution installation, then your upgraded hostconfiguration will be overritten. To make sure, that nothing will be overwritten, please do not use the automatic installation. Use: inst_sge -x -upd

If you have configured load sensors on your execution hosts, you will need to copy these load sensors to the new directory location. 

21. Check your complexes. 
Both the structure of complexes and the rules for configuring complexes have changed. You can use qconf -sc to list your complexes. 
Review the log file that was generated during the master host upgrade,
in qmaster spool directory. 
If necessary, you can use qconf -mc to reconfigure your complexes. 

22. Reconfigure your queues. During the upgrade process, a single default cluster queue is created. 
Within this queue you will find all of your installed execution hosts. It is recommended that you reconfigure your queues.


