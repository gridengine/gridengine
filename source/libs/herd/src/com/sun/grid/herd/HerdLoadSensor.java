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

package com.sun.grid.herd;

import com.sun.grid.loadsensor.LoadSensor;
import com.sun.grid.loadsensor.LoadSensorManager;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.logging.Logger;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.conf.Configured;
import org.apache.hadoop.hdfs.DFSClient;
import org.apache.hadoop.hdfs.protocol.DatanodeInfo;
import org.apache.hadoop.hdfs.protocol.FSConstants.DatanodeReportType;
import org.apache.hadoop.hdfs.server.namenode.NameNode;
import org.apache.hadoop.hdfs.server.protocol.BlocksWithLocations;
import org.apache.hadoop.hdfs.server.protocol.NamenodeProtocol;
import org.apache.hadoop.io.retry.RetryPolicies;
import org.apache.hadoop.io.retry.RetryPolicy;
import org.apache.hadoop.io.retry.RetryProxy;
import org.apache.hadoop.ipc.RPC;
import org.apache.hadoop.net.NetUtils;
import org.apache.hadoop.security.UnixUserGroupInformation;
import org.apache.hadoop.security.UserGroupInformation;
import org.apache.hadoop.util.StringUtils;
import org.apache.hadoop.util.Tool;
import org.apache.hadoop.util.ToolRunner;

/**
 *
 */
public class HerdLoadSensor extends Configured implements Tool, LoadSensor {
    private static final String BLOCK_KEY = "hdfs_blk";
    private static final String RACK_KEY1 = "hdfs_primary_rack";
    private static final String RACK_KEY2 = "hdfs_secondary_rack";
    private static final String ZEROS = "0000000000000000";
    private static final Logger log = Logger.getLogger(HerdLoadSensor.class.getName());
    private Configuration conf;
    private DFSClient client = null;
    private NamenodeProtocol namenode = null;
    private DatanodeInfo node = null;
    private String hostName = null;
    private String rackName = null;
    private Map<String,String> blockStrings = null;
    
    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        int exit = 0;

        try {
            exit = ToolRunner.run(new HerdLoadSensor(), args);
        } catch (Exception e) {
            Throwable cause = e;

            while (e.getCause() != null) {
                cause = e.getCause();
            }

            System.err.println("Error while running command: " + cause.getClass().getSimpleName() + " -- " + cause.getMessage());
            e.printStackTrace();
            exit = 13;
        }

        System.exit(exit);
    }

    public int run(String[] args) throws Exception {
        log.info("Started Herd load sensor");
        
        conf = getConf();
        client = new DFSClient(conf);
        namenode = createNamenode(conf);
        hostName = args[0];
        
        findDatanode();

        LoadSensorManager mgr = new LoadSensorManager(this);
        
        mgr.parse();
        
        return 0;
    }

    private void findDatanode() throws IOException {
        DatanodeInfo[] datanodes = client.namenode.getDatanodeReport(DatanodeReportType.LIVE);

        for (DatanodeInfo info : datanodes) {
            if (hostName.equals(info.getHostName())) {
                node = info;
            }
        }
    }

    /* Build a NamenodeProtocol connection to the namenode and
     * set up the retry policy */
    private static NamenodeProtocol createNamenode(Configuration conf)
            throws IOException {
        InetSocketAddress nameNodeAddr = NameNode.getAddress(conf);
        RetryPolicy timeoutPolicy = RetryPolicies.exponentialBackoffRetry(
                5, 200, TimeUnit.MILLISECONDS);
        Map<Class<? extends Exception>, RetryPolicy> exceptionToPolicyMap =
                new HashMap<Class<? extends Exception>, RetryPolicy>();
        RetryPolicy methodPolicy = RetryPolicies.retryByException(
                timeoutPolicy, exceptionToPolicyMap);
        Map<String, RetryPolicy> methodNameToPolicyMap =
                new HashMap<String, RetryPolicy>();
        methodNameToPolicyMap.put("getBlocks", methodPolicy);

        UserGroupInformation ugi;
        try {
            ugi = UnixUserGroupInformation.login(conf);
        } catch (javax.security.auth.login.LoginException e) {
            throw new IOException(StringUtils.stringifyException(e));
        }

        return (NamenodeProtocol) RetryProxy.create(
                NamenodeProtocol.class,
                RPC.getProxy(NamenodeProtocol.class,
                NamenodeProtocol.versionID,
                nameNodeAddr,
                ugi,
                conf,
                NetUtils.getDefaultSocketFactory(conf)),
                methodNameToPolicyMap);
    }

    public int getMeasurementInterval() {
        return MEASURE_ON_DEMAND;
    }

    public void doMeasurement() {
        try {
            rackName = node.getNetworkLocation();

            BlocksWithLocations blocks = namenode.getBlocks(node, Long.MAX_VALUE);

            List<Long> blockIds = new ArrayList<Long>(100);

            for (BlocksWithLocations.BlockWithLocations block : blocks.getBlocks()) {
                blockIds.add(block.getBlock().getBlockId());
            }

            blockStrings = buildBlockStrings(blockIds);

            log.fine("Rack is " + rackName);
            log.fine("Blocks are: " + blockStrings);
        } catch (IOException e) {
            log.warning("load sensor threw I/O exception while communicating with namenode: " + e.getMessage());
        }
    }

    public static Map<String,String> buildBlockStrings(List<Long> blockIds) {
        Map<String,String> ret = new HashMap<String, String>();
        StringBuilder blk = new StringBuilder();
        String previous = "";

        Collections.sort(blockIds);

        for (Long id : blockIds) {
            String idString = Long.toHexString(id);

            // Zero-pad if needed
            idString = ZEROS.substring(idString.length()) + idString;

            if (!idString.substring(0, 2).equals(previous)) {
                if (!previous.equals("")) {
                    blk.append('/');
                    ret.put(BLOCK_KEY + previous, blk.toString());
                }
                
                previous = idString.substring(0, 2);
                blk.setLength(0);
            }
            
            blk.append('/');
            blk.append(idString.substring(2));
        }

        // Clean up last block
        if (!previous.equals("")) {
            blk.append("//");
            ret.put(BLOCK_KEY + previous, blk.toString());
        }

        return ret;
    }

    public Map<String, Map<String, String>> getLoadValues() {
        Map<String, Map<String, String>> ret = null;
        Map<String,String> loadValues = new HashMap<String, String>();

        loadValues.put(RACK_KEY1, rackName);
        loadValues.put(RACK_KEY2, rackName);
        loadValues.putAll(blockStrings);

        ret = Collections.singletonMap(hostName, loadValues);

        log.fine("Returning load values: " + ret);

        return ret;
    }
}
