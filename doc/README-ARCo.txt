In the 6.0u11 patch the following has changed in ARCo:

1. ARCo now supports Sun Java Web Console (SWC) up to 3.0.2
	
    a) If upgrading from a previous version you need to reinstall ARCo using the 
'inst_reporting' and 'inst_dbwriter scripts'. Files that are particular to each 
SWC version are created during the installation of the reporting module.
	
	b) SWC 3.0 currently only deploys web apps in directory layout format 
(no WAR files). The 'deployment directory' of ARCo is
	
	$SGE_ROOT/$SGE_CELL/arco/reporting
	
This is also the default directory from where the config.xml is read when 
running the arcorun script. During deployment into SWC 3.0 a registration 
notification file reporting.reg is created in:
    
    /etc/webconsole/console/prereg/<application_name>

	c) The SWC 3.0.2 is included with the Solaris 10u3 or it can be downloaded 
from: 

    http://sun.com/downloads
	
For information on installing, unistalling, administering and managing SWC see:

    http://docs.sun.com/app/docs/doc/817-1985/6mhm8o5ke?a=view
    
--------------------------------------------------------------------------------
Note: that all 1.x.x and 2.x.x versions of SWC have been EOV'd (End of Version). 
Sun does no longer offer support via micro releases or patches. The only support
path is to upgrade to a supported version or patch level.
--------------------------------------------------------------------------------

2. The DBWriter insert speed has been improved.

3. During the installation of the reporting module, the user is prompted to 
either update (overwrite) or keep the predefined queries in the spool 
directory. You will see following prompt:

Install predefined queries
--------------------------

query directory /var/spool/arco/oracle/queries already exists
Copy examples queries into /var/spool/arco/oracle/queries
Query AR_Attributes.xml already exists. Overwrite? 
( y = yes, n = no, Y = yes to all, N = no to all ) [n] >>

If you have modified any of these predefined queries and did not save them under
a different name. If you choose the 'y' or 'Y' option all your modifications
will be lost. 

However, we do recommend you update the existing queries as there have been done
improvements/fixes to the existing queries, especially for Oracle. 
See issues:

    http://gridengine.sunsource.net/issues/show_bug.cgi?id=2230
    http://gridengine.sunsource.net/issues/show_bug.cgi?id=2247
    http://gridengine.sunsource.net/issues/show_bug.cgi?id=2266

None of your custom queries will be modified at any time. 

4. The toc file is now created during installation.
	
	
