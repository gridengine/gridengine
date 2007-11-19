Tutorial: How to perform an upgrade to new N1GE 6.1
---------------------------------------------------

Upgrades to 6.1 are only possible from 6.0u2 or higher.
Upgrades from version 5.3 or version 6.0/6.0u1 are not supported anymore.
If you are still running 5.3 or 6.0/6.0u1 Versions, it is recommend to upgrade
to 6.0u4 or higher and then to 6.1.

ATTENTION: Before starting with upgrades, it is recommended that the cluster 
holds no running and pending jobs. After the upgrade all jobs, which resisting in
the cluster, will be gone. It is also recommended to do a backup of the
system. This can be done by using the backup functionality of the SGE installer 
(inst_sge -bup)

IMPORTANT INFORMATION:(This is only necessary, if your Berkeley DB Server host is a
sol-sparc64 machine). For Grid Engine 6.0, a 64 bit Berkeley DB Server was not
available. The 32 bit Server was used in stead. For doing a backup or a
restore the 32 bit db_dump and db_load binaries are recommended.
The solaris 32 bit packages have to be installed also. For all patch-levels
less then or equal to u9 you may easily use the ./inst_sge -bup command for doing 
a backup.
Due to an issue in u10, the backup will still look for the libdb-4.2.so file,
which is not available. This leads to an backup failure. The workaround, making 
the backup work is the following
command: touch $SGE_ROOT/lib/sol-sparc/libdb-4.2.so 
The backup will work now.

Upgrade procedure for installations with local Berkeley DB Spooling and 
Berkeley DB RPC Server

Do the following steps for a secure and a working upgrade:

Note:

<db_home> is the directory where your database files are created. During the 
first installation of the qmaster host, you were asked for a Berkeley DB spooling
directory. This directory is meant with <db_home>.

   1. Shutdown the complete cluster using the

      qconf -ke all -ks -km

      command. If you're running a Berkeley DB RPC Server installation you also 
      have to shutdown the Berkeley DB RPC service.

   2. Do a system backup, using the

      inst_sge -bup

      (More information how to perfom a backup can be found in the Admin or 
      Installation Guide)

   3. Due to a Berkeley DB upgrade the database stucture has been changed and some
      additional steps have to be done manually. Execute as adminuser the 
      following command:

      $SGE_ROOT/utilbin/<arch>/db_dump -f /tmp/dump.out -h <your db_home> sge

      Execute this command on master host. If you are using a BDB RPC Server, then
      you must call this command on RPC Server.

   4. Check, if the dump.out file is not empty (use more command or vi, but do not
      change anything), if necessary then do the dump again.

   5. Unpack the new binaries and the common package to your SGE_ROOT directory

   6. Do a

      rm -f on your <db_home>/* 

      directory as adminuser. Please do not delete the whole directory, only the 
      files within your <db_home>.

   7. Execute as adminuser the following command:

      $SGE_ROOT/utilbin/<arch>/db_load -f /tmp/dump.out -h <your db_home> sge

      execute this command on master host. If you are using a BDB RPC Server, then
      you must execute this command on RPC Server.

   8. Execute on the master (also for BDB RPC server installation, this step has 
      to be performed on the master host)

      ./inst_sge -upd 

      (execute this command as user root and follow the instructions)

   9. The upgrade will create new settings and rc-script files, the old files will
      be renamed with scriptname and timestamp. The upgrade procedure restarts the
      qmaster daemon and scheduler. Execution host daemons have to be restarted 
      manually.

  10. After the upgrade, please check the newly created settings files and copy 
      the new rc-scripts to your startup location (eg. /etc/init.d) 




Steps for upgrading from 6.0u2 or higher to 6.1 using classic spooling

   - do a complete backup of your cluster.
     (see. Admin Guide for further info, backup)
   - shutdown the cluster using the command: qconf -ke all -ks -km
   - extract the new binaries to SGE_ROOT, exchange the old with new one
   - execute ./inst_sge -upd, and follow the instructions. This update will
     recreate the rc-files and settings files.
   - the upgrade restarts the qmaster daemon and scheduler. Execution host daemons 
     have to be restarted manually, using the sgeexecd rc file.
   - upgrade will be complete now
