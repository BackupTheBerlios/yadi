/*
$Id: boot.cpp,v 1.1 2004/10/07 11:58:04 essu Exp $
#
# Copyright (c) 2004 Steff Ulbrich, Germany. All rights reserved.
# Author: Steff Ulbrich
# Mail: essu@yadi.org
# aktuelle Versionen gibt es hier:
# $Source: /home/xubuntu/berlios_backup/github/tmp-cvs/yadi/Repository/plugins/boot/boot.cpp,v $
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 675 Mass Ave, Cambridge MA 02139, USA.

a small enigma plugin. to choose between different boot options
*/
#include <plugin.h>
#include <stdio.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/gui/listbox.h>

	// our plugin entry point, declared to use C calling convention
extern "C" int plugin_exec( PluginParam *par );

	// our small demo dialog. based on a eWindow and thus is toplevel and has decoration.
class eDemoDialog: public eWindow
{
		// our listbox we like to use.
	eListBox<eListBoxEntryText> *listbox;
		// two buttons, one for abort and another one for accept.
	eButton *bt_abort, *bt_ok;
		// and some label to display status information.
	eLabel *lb_selected;
		// our callback if the user selects something:
	void selectedItem(eListBoxEntryText *item);
	void selectionChanged(eListBoxEntryText *item);
	
	int counter;
public:
		// the constructor.
	eDemoDialog();
		// the destructor.
	~eDemoDialog();
};

bool fileExists(const char* filename)
{
  FILE* test;
  test = fopen(filename,"r");
  if (test != NULL) {
    fclose(test);
    return 1;
  } else {
    return 0;
  }
}

	// our entry point.
int plugin_exec( PluginParam *par )
{
		// our demo dialog instance.
	eDemoDialog dlg;
		// show the dialog...
	dlg.show();
		// give control to dialog.. (the dialog is modal!)
	int result=dlg.exec();
		// and after it, hide it again.
	dlg.hide();
	
		// if the result of the dialog is non-zero, show this warning.
	if (result)
	{
		eMessageBox msg("Einstellungen werden beim naechsten Booten uebernommen", "OK", eMessageBox::iconWarning|eMessageBox::btOK);
		msg.show(); msg.exec(); msg.hide();
	}
	else
	{
		system("reboot");
	}
	return 0;
}


eDemoDialog::eDemoDialog(): eWindow(1)
{
		// move our dialog to 100.100...
	cmove(ePoint(210, 160));
		// ...and make it fill the screen.
	cresize(eSize(300, 256));
		// set a title.
	setText("Boot-Optionen einstellen");
	
		// create our listbox. it's a child of our window, thus we give "this" as parent.
	listbox=new eListBox<eListBoxEntryText>(this);
		// move it into our widget. all positions are relative to the parent widget.
	listbox->move(ePoint(10, 10));
		// leave some space for the buttons (thus height()-100)
	listbox->resize(eSize(clientrect.width()-20, clientrect.height()-90));
	listbox->loadDeco();

		// create the ok button
	bt_ok=new eButton(this);
	bt_ok->move(ePoint(clientrect.width()-110, clientrect.height()-40));
	bt_ok->resize(eSize(100, 30));
		// set the shortcut action. in this case, it's the green button.
	bt_ok->setShortcut("green");
	bt_ok->setShortcutPixmap("green");
		// load some decoration (border)
	bt_ok->loadDeco();
		// set Text
	bt_ok->setText("reboot");
		// "connect" the button. if the user pressed this button (either by selecting it and pressing
		// OK or by pressing green) the button should do something:
		// in this case, this will connect bt_ok->selected to our "accept dialog".
		// accept is just a close(0); - we will return with zero as return code.
	CONNECT(bt_ok->selected, eWidget::accept);
	
	lb_selected=new eLabel(this);
	lb_selected->move(ePoint(20, clientrect.height()-80));
	lb_selected->resize(eSize(clientrect.width()-20, 30)); // leave space for both buttons
	lb_selected->setText("Option waehlen");
	
		// create the abort button	
	bt_abort=new eButton(this);
	bt_abort->move(ePoint(10, clientrect.height()-40)); // listbox goes to height()-40
	bt_abort->resize(eSize(100, 30));
	bt_abort->setShortcut("red");
	bt_abort->setShortcutPixmap("red");
	bt_abort->loadDeco();
	bt_abort->setText("abort");
		// reject is close(1) - we will return with nonzero return code.
	CONNECT(bt_abort->selected, eWidget::reject);
	
		// create some dummy listbox entries:
	new eListBoxEntryText(listbox, "Enigma Direktstart", (void*)0);
	new eListBoxEntryText(listbox, "Boot-Info zeigen", (void*)1);
	new eListBoxEntryText(listbox, "HW-Sections verwenden", (void*)2);
	new eListBoxEntryText(listbox, "SPTS-Mode verwenden", (void*)3);
	new eListBoxEntryText(listbox, "Nokia-Kabeltreiber", (void*)4);
	new eListBoxEntryText(listbox, "spezial Philipstreiber", (void*)5);

		// if the user selects an item, we want to be notified:
	CONNECT(listbox->selected, eDemoDialog::selectedItem);

		// if the user selects an item, we want to be notified:
	CONNECT(listbox->selchanged, eDemoDialog::selectionChanged);
	
		// and we set the focus to the listbox:
	setFocus(listbox);
	
	counter=0;
}

eDemoDialog::~eDemoDialog()
{
	// we have to do almost nothing here. all widgets are automatically removed
	// since they are child of us. the eWidget-destructor will to this for us.
}


void eDemoDialog::selectedItem(eListBoxEntryText *item)
{
		// if the user selected something, show that:
	if (item)
	{
		eString message;
		if (counter++ > 2)
			message="Was soll das Hin und Her? ;)";
		else
			switch ((int)item->getKey())
			{
			case 0:
				if (fileExists("/var/etc/.enigma"))
				{	system("rm /var/etc/.enigma");
					message="Enigma-Direktstart ist deaktiviert";}
				else
				{	system("rm /var/etc/.neutrino");
					system("touch /var/etc/.enigma");
					message="Enigma-Direktstart ist aktiviert";}
				break;
			case 1:
				if (fileExists("/var/etc/.boot_info"))
				{	system("rm /var/etc/.boot_info");
					message="Boot-Info ist deaktiviert";}
				else
				{	system("touch /var/etc/.boot_info");
					message="Boot-Info ist aktiviert";}
				break;
			case 2:
				if (fileExists("/var/etc/.hw_sections"))
				{	system("rm /var/etc/.hw_sections");
					message="Hardware-Sections ist deaktiviert";}
				else
				{	system("touch /var/etc/.hw_sections");
					message="Hardware-Sections ist aktiviert";}
				break;
			case 3:
			        if (fileExists("/var/etc/.spts_mode"))
				{	system("rm /var/etc/.spts_mode");
					message="SPTS-Mode ist deaktiviert";}
				else
				{	system("touch /var/etc/.spts_mode");
					message="SPTS-Mode ist aktiviert";}
				break;
			case 4:
			        if (fileExists("/var/etc/.cable"))
				{	system("rm /var/etc/.cable");
					message="Nokia Kabeltreiber ist deaktiviert";}
				else
				{	system("touch /var/etc/.cable");
					message="Nokia Kabeltreiber ist aktiviert";}
				break;
			case 5:
			        if (fileExists("/var/etc/.tda80xx.o"))
				{	system("rm /var/etc/.tda80xx.o");
					message="spezieller Philipstreiber ist deaktiviert";}
				else
				{	system("touch /var/etc/.tda80xx.o");
					message="spezieller Philipstreiber ist aktiviert";}
				break;
			}
		eMessageBox msg(message, "Bootoption", ((counter&1)?eMessageBox::iconQuestion:eMessageBox::iconInfo)|eMessageBox::btOK);
		msg.show(); msg.exec(); msg.hide();
	} else
			// otherwise, just close the dialog.
		reject();
}

void eDemoDialog::selectionChanged(eListBoxEntryText *item)
{
	counter=0;
	eString submessage;
	switch ((int)item->getKey())
			{
			case 0:
				if (fileExists("/var/etc/.enigma"))
					submessage="ausschalten";
				else
					submessage="einschalten";
				break;
			case 1:
				if (fileExists("/var/etc/.boot_info"))
					submessage="ausschalten";
				else
					submessage="einschalten";
				break;
			case 2:
				if (fileExists("/var/etc/.hw_sections"))
					submessage="ausschalten";
				else
					submessage="einschalten";
				break;
			case 3:
			        if (fileExists("/var/etc/.spts_mode"))
					submessage="ausschalten";
				else
					submessage="einschalten";
				break;
			case 4:
			        if (fileExists("/var/etc/.cable"))
					submessage="ausschalten";
				else
					submessage="einschalten";
				break;
			case 5:
			        if (fileExists("/var/etc/.tda80xx.o"))
					submessage="ausschalten";
				else
					submessage="einschalten";
				break;
			}
	lb_selected->setText(submessage);
}
