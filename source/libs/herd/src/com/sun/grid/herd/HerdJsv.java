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

import com.sun.grid.jsv.JobDescription;
import com.sun.grid.jsv.Jsv;
import com.sun.grid.jsv.JsvManager;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.logging.Logger;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.conf.Configured;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hdfs.DFSClient;
import org.apache.hadoop.hdfs.protocol.DatanodeInfo;
import org.apache.hadoop.hdfs.protocol.LocatedBlock;
import org.apache.hadoop.util.StringUtils;
import org.apache.hadoop.util.Tool;
import org.apache.hadoop.util.ToolRunner;

/**
 *
 * @author dant
 */
public class HerdJsv extends Configured implements Tool, Jsv {
    private static final String BLOCK_KEY = "hdfs_B";
    private static final String PATH_KEY = "hdfs_input";
    private static final String PATH_KEY_SHORT = "hdfs_in";
    private static final String PRIMARY_RACK_KEY = "hdfs_R";
    private static final String SECONDARY_RACK_KEY = "hdfs_r";
    private static final String ZEROS = "0000000000000000";
    private static final Logger log = Logger.getLogger(HerdJsv.class.getName());

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        int exit = 0;

        try {
            exit = ToolRunner.run(new HerdJsv(), args);
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

    public int run(String[] arg0) throws Exception {
        log.info("Started Herd JSV");

        new JsvManager().parse(this);

        return 0;
    }

    public void onStart(JsvManager jsv) {
        Logger.getLogger("com.sun.grid.Jsv").info("Starting HerdJsv");
    }

    public void onVerify(JsvManager jsv) {
        JobDescription job = jsv.getJobDescription();
        Map<String,String> res = job.getHardResourceRequirements();
        String path = null;

        if ((res != null) && res.containsKey(PATH_KEY)) {
            path = res.get(PATH_KEY);
        } else if ((res != null) && res.containsKey(PATH_KEY_SHORT)) {
            path = res.get(PATH_KEY_SHORT);
        }

        if (path != null) {
            Map<String,String> soft = job.getSoftResourceRequirements();

            try {
                List<LocatedBlock> blocks = getBlocks(path, getConf());

                // Add new requests
                soft.putAll(buildRackRequests(blocks));
                soft.putAll(buildBlockRequests(blocks));
                job.setSoftResourceRequirements(soft);

                // Remove old request
                res.remove(PATH_KEY);
                res.remove(PATH_KEY_SHORT);
                job.setHardResourceRequirements(res);
            } catch (FileNotFoundException e) {
                jsv.reject("The requested data path does not exist: " + path);
            } catch (IOException e) {
                jsv.log(JsvManager.LogLevel.ERROR, "Unable to contact Nodename: " + StringUtils.stringifyException(e));
                jsv.rejectWait("Unable to process Hadoop jobs at this time.");
            }
        }
    }

    private static Map<String,String> buildBlockRequests(List<LocatedBlock> blocks) {
        Map<String,String> ret = new HashMap<String, String>();
        StringBuilder request = new StringBuilder();
        String previous = "";

        Collections.sort(blocks, new Comparator<LocatedBlock>() {
            public int compare(LocatedBlock o1, LocatedBlock o2) {
                return o1.getBlock().compareTo(o2.getBlock());
            }
        });

        for (LocatedBlock block : blocks) {
            String idString = toIdString(block.getBlock().getBlockId());

            if (!idString.substring(0, 2).equals(previous)) {
                if (!previous.equals("")) {
                    request.append("*");
                    ret.put(BLOCK_KEY + previous, request.toString());
                }
                
                previous = idString.substring(0, 2);
                request.setLength(0);
            }

            request.append('*');
            request.append(idString.substring(2));
        }

        if (!previous.equals("")) {
            request.append("*");
            ret.put(BLOCK_KEY + previous, request.toString());
        }

        return ret;
    }

    private static String toIdString(long id) {
        String idString = Long.toHexString(id);

        // Zero-pad if needed
        idString = ZEROS.substring(idString.length()) + idString;

        return idString;
    }

    private static Map<String,String> buildRackRequests(List<LocatedBlock> blocks) {
        Map<String,String> ret = new HashMap<String, String>();
        StringBuilder primary = new StringBuilder();
        StringBuilder secondary = new StringBuilder();

        Map<String, Integer> racks = collateRacks(blocks);
        // Sort the racks by number of blocks in each rack
        List<String> rackNames = getSortedRacks(racks);

        // Build primary from the top 20% and secondary from 100%
        int count = 0;
        int top = (int)Math.ceil(rackNames.size() * 0.2f);
        
        for (String rack : rackNames) {
            secondary.append('|');
            secondary.append(rack);

            if (count < top) {
                primary.append('|');
                primary.append(rack);
            }

            count++;
        }

        ret.put(PRIMARY_RACK_KEY, primary.toString().substring(1));
        ret.put(SECONDARY_RACK_KEY, secondary.toString().substring(1));

        return ret;
    }

    private static Map<String,Integer> collateRacks(List<LocatedBlock> blocks) {
        Map<String, Integer> racks = new HashMap<String, Integer>();

        for (LocatedBlock block : blocks) {
            for (DatanodeInfo node : block.getLocations()) {
                String loc = node.getNetworkLocation();

                if (racks.containsKey(loc)) {
                    racks.put(loc, racks.get(loc) + 1);
                } else {
                    racks.put(loc, 1);
                }
            }
        }

        return racks;
    }

    private static List<String> getSortedRacks(final Map<String,Integer> racks) {
        List<String> rackNames = new ArrayList<String>(racks.keySet());

        Collections.sort(rackNames, new Comparator<String>() {
            public int compare(String o1, String o2) {
                return racks.get(o1).compareTo(racks.get(o2));
            }
        });

        return rackNames;
    }

    private static List<LocatedBlock> getBlocks(String path, Configuration conf) throws IOException {
        List<LocatedBlock> blocks = new LinkedList<LocatedBlock>();

        FileStatus s = FileSystem.get(conf).getFileStatus(new Path(path));
        DFSClient dfs = new DFSClient(conf);

        if (!s.isDir()) {
            blocks.addAll(dfs.namenode.getBlockLocations(path, 0,
                    Long.MAX_VALUE).getLocatedBlocks());
        } else {
            for (FileStatus fs : dfs.listPaths(path)) {
                blocks.addAll(getBlocks(fs.getPath().toString(), conf));
            }
        }
        
        dfs.close();

        return blocks;
    }
}
