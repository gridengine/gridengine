Tutorial: How to perform an upgrade to new N1GE 6.1
---------------------------------------------------

Upgrade to 6.1 are only possible from 6.0u2 or higher.
Upgrades from Version 5.3 or Version 6.0/6.0u1 are not supported anymore.
If you are still running 5.3 or 6.0/6.0u1 Versions, it is recommend to upgrade
to 6.0u10 and then to 6.1.

Attention: We recommend to upgrade "empty" clusters only, if possible ensure,
that no jobs are running or in pending state!

There are 2 upgrade scenarios.

1. Upgrade to 6.1 using classic/berkeley db spooling
2. Upgrade to 6.1 using BDB server spooling.

Due to an issue in Berkeley DB Server, it was not possible to run a 64 bit BDB server
on 64 bit hosts. The 6.0ux Version used a 32bit BDB Server for spooling.
This needs an additional step in the upgrade procedure, which will be described later.

1. Steps for upgrading from 6.0u2 or higher to 6.1 using classic/berkeley db spooling

   - do a complete backup of your cluster.
     (see. Admin Guide for further info, backup)
   - shutdown the cluster using the command: qconf -ke all -ks -km
   - extract the new binaries to SGE_ROOT, exchange the old with new one
   - execute ./inst_sge -upd, and follow the instructions. This update will
     recreate the rc-files and settings files.
   - restart the cluster, by using the sgemaster and sgeexecd rc files
   - upgrade will be complete now


2. Steps for upgrading from 6.0u2 or higher to 6.1 using berkeley db server.

   Attention:(This is only necessary, if you Berkeley DB Server host is a
sol-sparc64 machine). For 64 bit Solaris a 64 bit Berkeley DB Server was not
available. The 32 bit Server was used in stead. For doing a backup or a
restore the 32 bit db_dump and db_load binaries are recommended.
The solaris 32 bit packages have to be installed also. For all patch-levels
less then or equal to u9 you may easily use the ./inst_sge -bup command for doing a backup.
Due to an issue in u10, the backup will still look for the libdb-4.2.so file, which
is not available. This leads to an backup failure. The workaround, making the backup work is the following
command: touch $SGE_ROOT/lib/sol-sparc/libdb-4.2.so 
The backup will work now.

   - do a complete backup of your cluster.
     (see. Admin Guide for further info, backup)
   - shutdown the cluster using the command: qconf -ke all -ks -km
   - shutdown the BDB Server, using the sgebdb stop script
   - extract the new binaries to SGE_ROOT, exchange the old with new one
   - do a restore (see: Admin Guide for restoring cluster configuration)
   - execute ./inst_sge -upd, and follow the instructions. This update will
     recreate the rc-files and settings files.
   - restart BDB Server, Qmaster and Execd's
   - upgrade will be complete now.


