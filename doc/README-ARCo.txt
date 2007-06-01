The following tables, view, queries have been added for the
Grid Engine 6.1 Advanced Reservation:

TABLES
sge_ar
sge_ar_attribute
sge_ar_usage
sge_ar_log
sge_ar_resource_usage

VIEWS
view_ar_attribute
view_ar_log
view_ar_usage
view_ar_resource_usage
view_ar_time_usage

For tables and view definition see the dbdefinition.xml in:

    <dbwriter_inst_dir>/database/<db_type>/dbdefinition.xml

QUERIES
Accounting per AR
Advanced Reservation Attributes
Advanced Reservation by User
Advanced Reservation Log
Advanced Reservation Time Usage
Number of Jobs Completed per AR

For queries definition see the appropriate xml file in:
    <reporting_inst_dir>/database/example_queries/<db_type>

The following tables and views have been modified for the
Grid Engine 6.1 Advanced Reservation:

TABLES
sge_job_usage

VIEWS
view_accounting

DELETION RULES
Following deletion rule has been added to dbwriter.xml file:
  
<!--     Advanced Reservation values    -->
  <delete scope="ar_values" time_range="year" time_amount="1" />

This deletion rule will affect all AR tables listed above.

For more on deletion rules refer to Grid Engine Administration Guide, section 
'Configuring DBWriter', subsection 'Deleting Outdated Records'.



    
     
