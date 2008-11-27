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
package com.sun.grid.installer.gui;

import com.izforge.izpack.gui.ButtonFactory;
import com.izforge.izpack.gui.FlowLayout;
import com.izforge.izpack.gui.IzPanelLayout;
import com.izforge.izpack.gui.LabelFactory;
import com.izforge.izpack.installer.InstallData;
import com.izforge.izpack.installer.InstallerFrame;
import com.izforge.izpack.installer.IzPanel;
import com.izforge.izpack.util.VariableSubstitutor;
import com.izforge.izpack.util.Debug;
import com.sun.grid.installer.util.Config;
import com.sun.grid.installer.util.FileHandler;
import com.sun.grid.installer.util.Util;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.print.PageFormat;
import java.awt.print.Printable;
import java.awt.print.PrinterException;
import java.awt.print.PrinterJob;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Date;
import javax.print.PrintService;
import javax.print.PrintServiceLookup;
import javax.swing.*;

public class ResultPanel extends IzPanel implements Printable, Config {
	/**
	 * The info string.
	 */
	private JEditorPane editorPane =null;
	
	private String readmeTemplatePath = "";
	private String readmePath = "";

	/**
	 * The constructor.
	 *
	 * @param parent The parent window.
	 * @param idata  The installation data.
	 */
	public ResultPanel(InstallerFrame parent, InstallData idata) {
		super(parent, idata, new IzPanelLayout());

		VariableSubstitutor vs = new VariableSubstitutor(idata.getVariables());
		
		readmeTemplatePath = vs.substituteMultiple(idata.getVariable(VAR_README_TEMP_FILE), null);
		readmePath = vs.substituteMultiple(idata.getVariable(VAR_README_FILE), null);
        readmePath = insertTimeStampToFileName(readmePath);

		// The info label.
		add(LabelFactory.create(parent.langpack.getString("result.info.label"),
				parent.icons.getImageIcon("edit"), LEADING), NEXT_LINE);
		// The text area which shows the info.
		editorPane = new JEditorPane();
		editorPane.setCaretPosition(0);
		editorPane.setContentType("text/html");
		editorPane.setEditable(false);
        editorPane.setBackground(Color.white);
		JScrollPane scroller = new JScrollPane(editorPane);

        JButton saveButton = ButtonFactory.createButton(parent.langpack.getString("button.save.label"), parent.icons.getImageIcon("save"), idata.buttonsHColor);
        saveButton.setToolTipText(parent.langpack.getString("button.save.tooltip"));
        saveButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                final JFileChooser fc = new JFileChooser();
                fc.setDialogType(JFileChooser.SAVE_DIALOG);
                fc.setMultiSelectionEnabled(false);
                int ret = fc.showOpenDialog(ResultPanel.this.getParent());
                if (ret == JFileChooser.APPROVE_OPTION) {
                    try {
                        File f = fc.getSelectedFile();
                        ArrayList<String> content = FileHandler.readFileContent(readmePath, true);

                        FileHandler.generateFile(content, f.getAbsolutePath());
                    } catch (FileNotFoundException ex) {
                        Debug.error(ex);
                    } catch (IOException ex) {
                        Debug.error(ex);
                    }
                    
                }
            }
        });

        JButton printButton = ButtonFactory.createButton(parent.langpack.getString("button.print.label"), parent.icons.getImageIcon("print"), idata.buttonsHColor);
        printButton.setToolTipText(parent.langpack.getString("button.print.tooltip"));
        printButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                PrinterJob job = PrinterJob.getPrinterJob();
                job.setPrintable(ResultPanel.this);
                if (job.printDialog()) {
                    try {
                        job.print();
                    } catch (PrinterException pe) {
                        Debug.error(pe);
                    }
                }
            }
        });

        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 10, 0));
        buttonPanel.add(saveButton);
        buttonPanel.add(printButton);

        add(scroller, NEXT_LINE);
        add(buttonPanel, NEXT_LINE);

		// At end of layouting we should call the completeLayout method also they do nothing.
		getLayoutHelper().completeLayout();
	}

	/**
	 * Loads the info text.
	 */
	private void loadResult() {
		try {
			Util.fillUpTemplate(readmeTemplatePath, readmePath, idata.getVariables());
			editorPane.setPage("file://" + readmePath);
		} catch (Exception e) {
			Debug.error(e);
		}
	}
	
	@Override
	public void panelActivate() {
		parent.lockNextButton();
        parent.lockPrevButton();
        parent.setQuitButtonText(parent.langpack.getString("FinishPanel.done"));
        parent.setQuitButtonIcon("done");
        
        loadResult();
	}

	/**
	 * Indicates wether the panel has been validated or not.
	 *
	 * @return Always true.
	 */
    @Override
	public boolean isValidated() {
		return true;
	}

    public int print(Graphics graphics, PageFormat pageFormat, int pageIndex) throws PrinterException {
        if (pageIndex != 0) {
            return Printable.NO_SUCH_PAGE;
        } else {
            Graphics2D g2d = (Graphics2D) graphics;
            g2d.translate(pageFormat.getImageableX(),
                    pageFormat.getImageableY());
            int cw = editorPane.getWidth();
            int ch = editorPane.getHeight();
            double pw = pageFormat.getImageableWidth();
            double ph = pageFormat.getImageableHeight();
            if (cw > pw || ch > ph) {
                double scaleX = pw / cw;
                double scaleY = ph / ch;
                double scale = scaleX < scaleY ? scaleX : scaleY;
                g2d.scale(scale, scale);
            }
            
            editorPane.printAll((Graphics) g2d);
            return Printable.PAGE_EXISTS;
        }
    }

    private String insertTimeStampToFileName(String fileName) {
        String[] splittedFileName = fileName.split(".");

        for (int i = 0; i < splittedFileName.length; i++) {
            fileName += splittedFileName[i];

            if (i == splittedFileName.length - 1) {
                fileName += generateTimeStamp();
            }
        }

        return fileName;
    }

    private String generateTimeStamp() {
        return new Date().toString();
    }
}
