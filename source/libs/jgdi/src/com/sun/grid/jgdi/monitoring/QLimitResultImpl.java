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
package com.sun.grid.jgdi.monitoring;
import com.sun.grid.jgdi.monitoring.LimitRuleInfoImpl;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Default Implemenation of the {@link QLimitResult} interface.
 *
 */
public class QLimitResultImpl implements QLimitResult, Serializable {
   
   private Map limitRuleInfoMap = new HashMap();
   private List limitRuleInfoList = new ArrayList();
   
   public Set getLimitRuleNames() {
      return limitRuleInfoMap.keySet();
   }
   
   public LimitRuleInfo createLimitRuleInfo(String limitRuleName) {
      LimitRuleInfoImpl ret = new LimitRuleInfoImpl(limitRuleName);
      limitRuleInfoMap.put(ret.getLimitRuleName(), ret);
      return ret;
   }
   
   public void addLimitRuleInfo(LimitRuleInfo limitRuleInfo) {
      limitRuleInfoMap.put(limitRuleInfo.getLimitRuleName(), limitRuleInfo);
   }
   
   public LimitRuleInfo getLimitRuleInfo(String limitRuleName) {
      return (LimitRuleInfo)limitRuleInfoMap.get(limitRuleName);
   }
   
   public List getLimitRules() {
      return new ArrayList(limitRuleInfoMap.values());
   }
   
}
