Tutorial: How to perform an upgrade to new SGE 6.1
--------------------------------------------------

Upgrades to 6.1 are only possible from 6.0u2 or higher.
Upgrades from version 5.3 or version 6.0/6.0u1 are not supported anymore.
If you are still running 5.3 or 6.0/6.0u1 Versions, it is recommended to upgrade
to 6.0u4 or higher and then to 6.1.

ATTENTION: Before starting with upgrades, it is recommended that the cluster 
holds no running and pending jobs. After the upgrade all jobs will be gone.
It is also recommended to backup the cluster configuration. This can be done by
using the backup functionality of the SGE installer (inst_sge -bup).

IMPORTANT INFORMATION: (This is only necessary, if your Berkeley DB (BDB) RPC
Server host is a sol-sparc64 machine). For Grid Engine 6.0, a 64bit BDB RPC
Server was not available, instead the 32bit server was used. It is also
necessary to backup the BDB database with the 32bit db_dump and db_load
binaries from the sol-sparc package. The backup functionality of the SGE
installer takes care of using the correct binaries.
Due to an issue in u10, the backup will will fail because of the missing
libdb-4.2.so file. This error is not critical and the workaround is to execute
the following command:

   # touch $SGE_ROOT/lib/sol-sparc/libdb-4.2.so 

Necessary steps to perform the upgrade
--------------------------------------

Please note, <db_home> is the directory where the database files are stored. 
This directory path was selected by the administrator at qmaster installation
time in case of BDB spooling.

1. Shutdown the whole cluster using the

   # qconf -ke all -ks -km

   command. If you're running a BDB RPC Server installation you also have to
   shutdown the BDB RPC server.

2. Create the cluster system backup by executing

   # inst_sge -bup

   (More information how to perform a backup can be found in the Installation
   Guide or Administration Guide)

3. (Skip this step in case of classic spooling)
   Because of a BDB version update the internal BDB database structures changed.
   To adjust the structures execute the following command as sgeadmin user:

   # $SGE_ROOT/utilbin/<arch>/db_dump -f /tmp/dump.out -h <db_home> sge

   Execute this command on qmaster host. If you are using a BDB RPC Server, then
   you must execute this command on the RPC Server host.

   Please verify that the command was executed correctly and the file
   /tmp/dump.out is not empty. 

   If the command succeeded remove the previous BDB database files by
   executing the following command as sgeadmin user:

   # rm -f on your <db_home>/* 

   Please do not delete the whole directory, only the 
   files within your <db_home>.

4. Unpack the new binaries and the common package to your SGE_ROOT directory

5. (Skip this step in case of classic spooling)
   Now restore the database. Execute as sgeadmin user the following command:

   # $SGE_ROOT/utilbin/<arch>/db_load -f /tmp/dump.out -h <db_home> sge

   This command must be executed on the on qmaster host or in case of BDB RPC Server
   on the RPC server host. 

6. The final step is to start the SGE installer upgrade routine as user root
   on the qmaster host with:

   # ./inst_sge -upd 

   The upgrade will create new settings and rc-script files, the old files
   will be saved under the same filename with a time stamp attached. The
   upgrade procedure starts the qmaster and scheduler daemon automatically,
   execution host daemons needs to be started manually.

   After the upgrade, please check the newly created settings files and copy 
   the new rc-scripts to your startup location (eg. /etc/init.d) 
