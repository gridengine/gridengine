--___INFO__MARK_BEGIN__
-----------------------------------------------------------------------------
--
--  The Contents of this file are made available subject to the terms of
--  the Sun Industry Standards Source License Version 1.2
--
--  Sun Microsystems Inc., March, 2001
--
--
--  Sun Industry Standards Source License Version 1.2
--  =================================================
--  The contents of this file are subject to the Sun Industry Standards
--  Source License Version 1.2 (the "License"); You may not use this file
--  except in compliance with the License. You may obtain a copy of the
--  License at http://gridengine.sunsource.net/Gridengine_SISSL_license.html
--
--  Software provided under this License is provided on an "AS IS" basis,
--  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
--  WITHOUT LIMITATION, WARRANTIES THAT THE SOFTWARE IS FREE OF DEFECTS,
--  MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE, OR NON-INFRINGING.
--  See the License for the specific provisions governing your rights and
--  obligations concerning the Software.
--
--  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
--
--  Copyright: 2001 by Sun Microsystems, Inc.
--
--  All Rights Reserved.
--
-----------------------------------------------------------------------------
--___INFO__MARK_END__
-- 
-- Initialize Sun Grid Engine Database
--
-- maintain information about the database:
--    timestamp of last change to database structure
--    SGE version
--    storage with/without history
CREATE TABLE sge_info (
   last_change TIMESTAMP WITH TIME ZONE NOT NULL,
   version VARCHAR NOT NULL, 
   with_history BOOLEAN NOT NULL
);
-- this info should better be passed from install script via psql
INSERT INTO sge_info VALUES ('now', 'SGEEE pre6.0 (Maintrunk)', false);

--
-- top level tables have additional info:
--    XX__id: internal id number
--    XX__valid: is record still valid (for spooling with history)
--    XX__created: timestamp, when record was created
--    XX__deleted: timestamp, when record became invalid
--
-- tables for sublists have in addition:
--    XX__parent: id of parent object
--
-- standard indexes:
--    XX__id, XX__valid, XX__created (unique)
--    XX__valid [,XX__parent]
--    XX__parent (non unique)
--


-- admin host
CREATE SEQUENCE sge_adminhost_seq;
CREATE TABLE sge_adminhost (
   AH__id INTEGER NOT NULL,
   AH__valid BOOLEAN NOT NULL DEFAULT TRUE,
   AH__created TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT 'now', 
   AH__deleted TIMESTAMP WITH TIME ZONE, 
   AH_name VARCHAR NOT NULL
);
CREATE UNIQUE INDEX sge_adminhost_idx1 ON sge_adminhost (AH__id, AH__valid, AH__created);
CREATE INDEX sge_adminhost_idx2 ON sge_adminhost (AH__valid);
-- exec host
CREATE SEQUENCE sge_exechost_seq;
CREATE TABLE sge_exechost (
   EH__id INTEGER NOT NULL,
   EH__valid BOOLEAN NOT NULL DEFAULT TRUE,
   EH__created TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT 'now', 
   EH__deleted TIMESTAMP WITH TIME ZONE, 
   EH_name VARCHAR NOT NULL,
   EH_processors INTEGER NOT NULL,
   EH_resource_capability_factor DOUBLE PRECISION NOT NULL
);
CREATE UNIQUE INDEX sge_exechost_idx1 ON sge_exechost (EH__id, EH__valid, EH__created);
CREATE INDEX sge_exechost_idx2 ON sge_exechost (EH__valid);

CREATE SEQUENCE sge_exechost_load_seq;
CREATE TABLE sge_exechost_load (
   HL__id INTEGER NOT NULL,
   HL__valid BOOLEAN NOT NULL DEFAULT TRUE,
   HL__created TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT 'now', 
   HL__deleted TIMESTAMP WITH TIME ZONE, 
   HL__parent INTEGER NOT NULL,
   HL_name VARCHAR NOT NULL,
   HL_value VARCHAR NOT NULL
);
CREATE UNIQUE INDEX sge_exechost_load_idx1 ON sge_exechost_load (HL__id, HL__valid, HL__created);
CREATE INDEX sge_exechost_load_idx2 ON sge_exechost_load (HL__valid, HL__parent);

CREATE SEQUENCE sge_exechost_load_scaling_seq;
CREATE TABLE sge_exechost_load_scaling (
   HS__id INTEGER NOT NULL,
   HS__valid BOOLEAN NOT NULL DEFAULT TRUE,
   HS__created TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT 'now', 
   HS__deleted TIMESTAMP WITH TIME ZONE, 
   HS__parent INTEGER NOT NULL,
   HS_name VARCHAR NOT NULL,
   HS_value VARCHAR NOT NULL
);
CREATE UNIQUE INDEX sge_exechost_load_scaling_idx1 ON sge_exechost_load_scaling (HS__id, HS__valid, HS__created);
CREATE INDEX sge_exechost_load_scaling_idx2 ON sge_exechost_load_scaling (HS__valid, HS__parent);
