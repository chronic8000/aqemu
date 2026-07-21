/****************************************************************************
**
** Copyright (C) 2008-2010 Andrey Rijov <ANDron142@yandex.ru>
** Copyright (C) 2016 Tobias Gläßer
**
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
****************************************************************************/

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextFrame>
#include <QTextTableCell>
#include <QUrl>
#include <QHeaderView>
#include <QValidator>
#include <QPainter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QSysInfo>
#include <QAbstractItemView>
#include <QComboBox>
#include <QTimer>
#include <QApplication>
#include <QEventLoop>
#ifndef Q_OS_WIN32
#include <QtDBus>
#endif

#include <memory>

#include "Main_Window.h"
#include "Delete_VM_Files_Window.h"
#include "Device_Manager_Widget.h"
#include "Folder_Sharing_Widget.h"
#include "Select_Icon_Window.h"
#include "About_Window.h"
#include "Create_HDD_Image_Window.h"
#include "Convert_HDD_Image_Window.h"
#include "VM_Wizard_Window.h"
#include "VM_Session_Widget.h"
#include <QStackedWidget>
#include "Ports_Tab_Widget.h"
#include "Create_Template_Window.h"
#include "Snapshots_Window.h"
#include "VNC_Password_Window.h"
#include "Copy_VM_Window.h"
#include "Advanced_Settings_Window.h"
#include "First_Start_Wizard.h"
#include "Emulator_Control_Window.h"
#include "Boot_Device_Window.h"
#include "SMP_Settings_Window.h"
#include "Settings_Widget.h"
#include "Utils.h"
#include "Service.h"
#include "No_Boot_Device.h"

// This is static emulator devices data
QMap<QString, Available_Devices> System_Info::Emulator_QEMU_2_0;


QList<VM_USB> System_Info::All_Host_USB;
QList<VM_USB> System_Info::Used_Host_USB;

Main_Window::Main_Window( QWidget *parent )
	: QMainWindow( parent )
	, block_VM_changed_signals( true )
	, Main_Stack( nullptr )
	, Session_Widget( nullptr )
	, Session_VM( nullptr )
	, Session_Mode_Active( false )
	, Auto_Save_Timer( nullptr )
{
    Advanced_Options = new QDialog(this);
    Accelerator_Options = new QDialog(this);
    Architecture_Options = new QDialog(this);
    SMP_Settings = new SMP_Settings_Window(this);

	Auto_Save_Timer = new QTimer( this );
	Auto_Save_Timer->setSingleShot( true );
	Auto_Save_Timer->setInterval( 400 );
	connect( Auto_Save_Timer, SIGNAL(timeout()), this, SLOT(on_Button_Apply_clicked()) );

    ui.setupUi( this );
	ui_ao.setupUi( Advanced_Options );

	// Embedded session shell (guest view replaces idle UI)
	Idle_Window_Title = windowTitle();
	QWidget *idle_root = takeCentralWidget();
	Main_Stack = new QStackedWidget( this );
	Session_Widget = new VM_Session_Widget( this );
	Main_Stack->addWidget( idle_root );
	Main_Stack->addWidget( Session_Widget );
	setCentralWidget( Main_Stack );
	Main_Stack->setCurrentIndex( 0 );

	connect( Session_Widget, SIGNAL(Exit_Session_View()), this, SLOT(On_Session_Exit_View()) );
	connect( Session_Widget, SIGNAL(Request_Stop()), this, SLOT(On_Session_Request_Stop()) );
	connect( Session_Widget, SIGNAL(Request_Shutdown()), this, SLOT(On_Session_Request_Shutdown()) );
	connect( Session_Widget, SIGNAL(Request_Reset()), this, SLOT(On_Session_Request_Reset()) );
	connect( Session_Widget, SIGNAL(Request_Pause()), this, SLOT(On_Session_Request_Pause()) );
	connect( Session_Widget, SIGNAL(Request_Save()), this, SLOT(On_Session_Request_Save()) );

	// Defaults for embedded session (new installs)
	if( ! Settings.contains( "Embedded_Session" ) )
		Settings.setValue( "Embedded_Session", "yes" );
	if( ! Settings.contains( "Embedded_Display_Backend" ) )
	{
#ifdef VNC_DISPLAY
		Settings.setValue( "Embedded_Display_Backend", "vnc" );
#else
		Settings.setValue( "Embedded_Display_Backend", "spice" );
#endif
	}

    connect(ui_ao.CH_Start_Date,SIGNAL(toggled(bool)),this,SLOT(adv_on_CH_Start_Date_toggled(bool)));

	ui_kvm.setupUi( Accelerator_Options );
	ui_arch.setupUi( Architecture_Options );

	// Combos were truncating long QEMU machine/video names
	auto fixComboElide = []( QComboBox *cb ) {
		if( ! cb ) return;
		cb->setSizeAdjustPolicy( QComboBox::AdjustToContents );
		cb->setMinimumContentsLength( 28 );
		if( cb->view() )
		{
			cb->view()->setTextElideMode( Qt::ElideNone );
			cb->view()->setMinimumWidth( 360 );
		}
	};
	fixComboElide( ui.CB_Computer_Type );
	fixComboElide( ui.CB_Machine_Type_Main );
	fixComboElide( ui.CB_CPU_Type_Main );
	fixComboElide( ui.CB_Video_Card );
	fixComboElide( ui.CB_Display_Resolution );
	fixComboElide( ui.CB_Mouse_Pointer_Mode );
	fixComboElide( ui.CB_Mouse_Type );
	fixComboElide( ui.CB_Mouse_USB_Controller );
	fixComboElide( ui.CB_Mouse_USB_Version );
	fixComboElide( ui.CB_SPICE_Agent_Mouse );
	fixComboElide( ui.CB_Boot_Priority );
	fixComboElide( ui.CB_Disk_Interface );
	fixComboElide( ui_arch.CB_Machine_Type );
	fixComboElide( ui_arch.CB_CPU_Type );

	Fill_Display_Resolution_Combo();
	Update_Display_Resolution_Enabled();
	Fill_Mouse_Combos();
	Update_Mouse_Options_Enabled();

	// Settings auto-save on change — Apply/Cancel are unused.
	ui.Button_Apply->hide();
	ui.Button_Cancel->hide();

    ui.Tabs->setCurrentIndex(0);
    ui.Use_Linux_Boot_Widget->setEnabled(false);

	QRegExp rx( "^[\\d]{1,2}|1[\\d]{,2}|2[0-4]{,2}|25[0-5]$" );
	QValidator *validator = new QRegExpValidator( rx, this );
	ui.CB_CPU_Count->setValidator( validator );

	// This for Tab Info Backgroud Color
	Update_Info_Text( 1 );

	Native_Device_Window = new Add_New_Device_Window();

	// Network Settigns
	New_Network_Settings_Widget = new Network_Widget();
	Old_Network_Settings_Widget = new Old_Network_Widget();

	// SPICE
	SPICE_Widget = new SPICE_Settings_Widget(this);
	ui.TabWidget_Display->insertTab( 1, SPICE_Widget, QIcon(":/pepper.png"), tr("SPICE Remote") );

    Display_Settings_Widget = new Settings_Widget( ui.TabWidget_Display, QBoxLayout::LeftToRight, true );
    Display_Settings_Widget->setIconSize(QSize(32,32));
    Display_Settings_Widget->addToGroup("Main");

	// Update Emulators Information
	System_Info::Update_VM_Computers_List();

	All_Emulators_List = Get_Emulators_List();

	GUI_User_Mode = true;
	Apply_Emulator( 0 );

	// Create Icon_Menu
	Icon_Menu = new QMenu( ui.Machines_List );

    Icon_Menu->addAction( ui.actionPower_On );
	Icon_Menu->addAction( ui.actionPause );
    Icon_Menu->addAction( ui.actionShutdown );
	Icon_Menu->addAction( ui.actionPower_Off );
	Icon_Menu->addAction( ui.actionReset );
    Icon_Menu->addAction( ui.actionSave );
	Icon_Menu->addSeparator();
    //Icon_Menu->addAction( ui.actionDelete_VM );
	Icon_Menu->addAction( ui.actionDelete_VM_And_Files );
	Icon_Menu->addAction( ui.actionSave_As_Template );
	Icon_Menu->addAction( ui.actionCopy );
	Icon_Menu->addSeparator();
	Icon_Menu->addAction( ui.actionManage_Snapshots );
	Icon_Menu->addAction( ui.actionShow_Emulator_Control );
	Icon_Menu->addAction( ui.actionShow_QEMU_Arguments );
	Icon_Menu->addAction( ui.actionCreate_Shell_Script );
	Icon_Menu->addAction( ui.actionShow_QEMU_Error_Log_Window );
	Icon_Menu->addAction( ui.actionChange_Icon );

	// Create VM List Menu
	VM_List_Menu = new QMenu( ui.Machines_List );

	VM_List_Menu->addAction( ui.actionAdd_New_VM );
	VM_List_Menu->addAction( ui.actionLoad_VM_From_File );
	VM_List_Menu->addAction( ui.actionCreate_HDD_Image );

	Ports_Tab = new Ports_Tab_Widget();
	ui.TabWidget_Media->insertTab( 0, Ports_Tab, QIcon(":/usb.png"), tr("Computer Ports") );

	Dev_Manager = new Device_Manager_Widget();
	Folder_Sharing = new Folder_Sharing_Widget();
	ui.TabWidget_Media->insertTab( 0, Folder_Sharing, QIcon(":/open-folder.png"), tr("Folder Sharing") );

	ui.TabWidget_Media->insertTab( 0, Dev_Manager, QIcon(":/hdd.png"), tr("Device Manager") );
    ui.TabWidget_Media->setCurrentWidget(Dev_Manager);

    Media_Settings_Widget = new Settings_Widget( ui.TabWidget_Media, QBoxLayout::LeftToRight, true );
    Media_Settings_Widget->setIconSize(QSize(32,32));
    Media_Settings_Widget->addToGroup("Main");
	

    //// code to sync sizes of widgets in Device Manager, Folder Sharing and Ports Tab Widget
    Folder_Sharing->syncLayout(Dev_Manager);
    Ports_Tab->syncLayout(Dev_Manager);
    ////

    Network_Settings_Widget = new Settings_Widget( ui.Network_Cards_Tabs, QBoxLayout::LeftToRight, true );
    Network_Settings_Widget->setIconSize(QSize(32,32));
    Network_Settings_Widget->addToGroup("Main");

	// This For Network Redirections Table
	QHeaderView *hv = new QHeaderView( Qt::Vertical, ui.Redirections_List );
	hv->setSectionResizeMode( QHeaderView::Fixed );
	ui.Redirections_List->setVerticalHeader( hv );

	hv = new QHeaderView( Qt::Horizontal, ui.Redirections_List );
	hv->setSectionResizeMode( QHeaderView::Stretch );
	ui.Redirections_List->setHorizontalHeader( hv );

	hv = new QHeaderView( Qt::Vertical, ui.Redirections_List );
	hv->setSectionResizeMode( QHeaderView::Fixed );
	ui.Redirections_List->setVerticalHeader( hv );

	hv = new QHeaderView( Qt::Horizontal, ui.Redirections_List );
	hv->setSectionResizeMode( QHeaderView::Stretch );
	ui.Redirections_List->setHorizontalHeader( hv );

	// Get max RAM size
	on_TB_Update_Available_RAM_Size_clicked();

    init_dbus();

	// Loading AQEMU Settings
	if( ! Load_Settings() )
	{
	// no Settings
		AQWarning( "Main_Window::Main_Window( QWidget *parent )", "Cannot Load Settings!" );
	}
	else
	{
		if( ! Load_Virtual_Machines() ) // Loading XML VM files
		{
			// no vm's
			AQDebug( "Main_Window::Main_Window", "No VM Loaded!" );

			// FIXME
			if( VM_List.count() <= 0 )
			{
				ui.actionPower_On->setEnabled( false );
				ui.actionSave->setEnabled( false );
				ui.actionPause->setEnabled( false );
				ui.actionPower_Off->setEnabled( false );
				ui.actionReset->setEnabled( false );
                ui.actionShutdown->setEnabled( false );

				Set_Widgets_State( false );

				Update_Info_Text( 1 );
			}
		}
		else
		{
			// ok, vm's loaded. show it...
			AQDebug( "Main_Window::Main_Window( QWidget *parent )", "All OK Loading Complete!" );
		}
	}

    Settings_Widget::syncGroupIconSizes("Main");

    // Signals for watching VM changes
    Connect_Signals();
    block_VM_changed_signals = false;
}

void Main_Window::init_dbus()
{
#ifndef Q_OS_WIN32
    //dbus listening stuff

    if (!QDBusConnection::sessionBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus session bus.\n"
                "To start it, run:\n"
                "\teval `dbus-launch --auto-syntax`\n");
    }

    if (!QDBusConnection::sessionBus().registerService("org.aqemu.main_window")) {
        fprintf(stderr, "%s\n",
                qPrintable(QDBusConnection::sessionBus().lastError().message()));
    }

    AQDebug("void Main_Window::init_dbus()", "registered");

    QDBusConnection::sessionBus().unregisterObject("/main_window", QDBusConnection::UnregisterTree);
    QDBusConnection::sessionBus().registerObject("/main_window", this, QDBusConnection::ExportAllSlots);
#endif
}

Main_Window::~Main_Window()
{
    delete Advanced_Options;
    delete Accelerator_Options;
    delete Architecture_Options;
    delete Native_Device_Window;
    delete New_Network_Settings_Widget;
    delete Old_Network_Settings_Widget;
    delete SPICE_Widget;
    delete Display_Settings_Widget;
    delete Icon_Menu;
    delete VM_List_Menu;
    delete Ports_Tab;
    delete Dev_Manager;
    delete Folder_Sharing;
    delete Media_Settings_Widget;
    delete SMP_Settings;

#ifndef Q_OS_WIN32
    QDBusConnection::sessionBus().unregisterService("org.aqemu.main_window");
#endif
}

void Main_Window::VM_State_Changed(const QString &vm, int state)
{
    AQDebug("void Main_Window::VM_State_Changed(const QString &vm, int state)","state changed");

    for ( int i = 0; i < VM_List.count(); i++ )
    {
        if ( QFileInfo(vm) == QFileInfo(VM_List.at(i)->Get_VM_XML_File_Path()) )
        {
            VM_List.at(i)->Set_State( static_cast<VM::VM_State>(state) ); //FIXME
            AQDebug("void Main_Window::VM_State_Changed(const QString &vm, int state)",VM_List.at(i)->Get_State_Text());
            break;
        }
    }
}

void Main_Window::closeEvent( QCloseEvent *event )
{
	// Tear down embedded VNC/SPICE before killing QEMU — otherwise the RFB
	// thread blocks in BlockingQueuedConnection and the UI hangs (AppHang).
	if( Session_Mode_Active )
		Exit_Session_Mode();

	if( ! Save_Settings() )
		AQGraphic_Error( "void Main_Window::closeEvent( QCloseEvent *event )",
						 tr("AQEMU"), tr("Could not save main window settings!"), false );

	// Stop any QEMU processes owned by the service before leaving
	AQEMU_Service::get().stop_all();

	// forced=true: allow quit even if UI→VM sync fails (do not trap the user)
    if ( ! Save_Or_Discard(true) )
        event->ignore();
    else
        event->accept();
}

Virtual_Machine *Main_Window::Get_VM_By_UID( const QString &uid )
{
	for( int ix = 0; ix < VM_List.count(); ix++ )
	{
		if( VM_List[ix]->Get_UID() == uid )
            return VM_List[ ix ];
	}

	// VM Not Found!
	AQWarning( "Virtual_Machine *Main_Window::Get_VM_By_UID( const QString &uid, bool &ok )",
			   "UID Not Found!" );
	return NULL;
}

Virtual_Machine *Main_Window::Get_Current_VM()
{
	if( ui.Machines_List->currentRow() < 0 )
        return NULL;

	return Get_VM_By_UID( ui.Machines_List->currentItem()->data(256).toString() );
}

void Main_Window::Connect_Signals()
{
	// General Tab
	connect( ui.Edit_Machine_Name, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CB_Computer_Type, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

    connect( ui_arch.CB_CPU_Type, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(sync_arch_CPU_Type_changed(int)) );

	connect( ui.CB_CPU_Count, SIGNAL(editTextChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

    connect( ui_arch.CB_Machine_Type, SIGNAL(currentIndexChanged(int)),
             this, SLOT(sync_arch_Machine_Type_changed(int)) );

	connect( ui.CB_Machine_Type_Main, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(on_CB_Machine_Type_Main_currentIndexChanged(int)) );

	connect( ui.CB_CPU_Type_Main, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(on_CB_CPU_Type_Main_currentIndexChanged(int)) );

	connect( ui.CB_Boot_Priority, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CB_Boot_Priority, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(CB_Boot_Priority_currentIndexChanged(int)) );

	connect( ui.CB_Video_Card, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CB_Display_Resolution, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CB_Keyboard_Layout, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CB_Mouse_Pointer_Mode, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(On_Mouse_Pointer_Mode_Changed()) );
	connect( ui.CB_Mouse_Type, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );
	connect( ui.CB_Mouse_Type, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(Update_Mouse_Options_Enabled()) );
	connect( ui.CB_Mouse_Type, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(Sync_Mouse_Pointer_Mode_From_Type()) );
	connect( ui.CB_Mouse_USB_Controller, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );
	connect( ui.CB_Mouse_USB_Version, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );
	connect( ui.CB_SPICE_Agent_Mouse, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Memory_Size, SIGNAL(valueChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CB_RAM_Size, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Remove_RAM_Size_Limitation, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_sb16, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_es1370, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Adlib, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_AC97, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_GUS, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_PCSPK, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_HDA, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_cs4231a, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_VirtIO_Sound, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_USB_Audio, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CB_Disk_Interface, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Button_VirtIO_Defaults, SIGNAL(clicked()),
			 this, SLOT(on_Button_VirtIO_Defaults_clicked()) );
	connect( ui.Button_Win11_Install, SIGNAL(clicked()),
			 this, SLOT(on_Button_Win11_Install_clicked()) );
	connect( ui.Button_Win11_First_Boot, SIGNAL(clicked()),
			 this, SLOT(on_Button_Win11_First_Boot_clicked()) );
	connect( ui.Button_Win11_Normal, SIGNAL(clicked()),
			 this, SLOT(on_Button_Win11_Normal_clicked()) );

	connect( ui.CH_Fullscreen, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Local_Time, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Snapshot, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_ACPI, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_No_Reboot, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_No_Shutdown, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	// Network Tab
	connect( ui.CH_Use_Network, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.RB_Network_Mode_Old, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );

	connect( ui.RB_Network_Mode_New, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );

	connect( New_Network_Settings_Widget, SIGNAL(Changed()),
			 this, SLOT(VM_Changed()) );

	connect( Old_Network_Settings_Widget, SIGNAL(Changed()),
			 this, SLOT(VM_Changed()) );

	// Ports
	connect( Ports_Tab, SIGNAL(Settings_Changed()),
			 this, SLOT(VM_Changed()) );

	// Additional Network Settings
	connect( ui.CH_Redirections, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Redirections_List, SIGNAL(itemChanged(QTableWidgetItem*)),
			 this, SLOT(VM_Changed()) );

	connect( ui.RB_TCP, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );

	connect( ui.RB_UDP, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );

	connect( ui.SB_Redir_Port, SIGNAL(valueChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_Guest_IP, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.SB_Guest_Port, SIGNAL(valueChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.RB_TCP, SIGNAL(toggled(bool)),
			 this, SLOT(Update_Current_Redirection_Item()) );

	connect( ui.RB_UDP, SIGNAL(toggled(bool)),
			 this, SLOT(Update_Current_Redirection_Item()) );

	connect( ui.SB_Redir_Port, SIGNAL(valueChanged(int)),
			 this, SLOT(Update_Current_Redirection_Item()) );

	connect( ui.Edit_Guest_IP, SIGNAL(textChanged(const QString &)),
			 this, SLOT(Update_Current_Redirection_Item()) );

	connect( ui.SB_Guest_Port, SIGNAL(valueChanged(int)),
			 this, SLOT(Update_Current_Redirection_Item()) );

	connect( ui.Edit_TFTP_Prefix, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_SMB_Folder, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	// Advanced Tab
	connect( ui.CH_No_Frame, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Alt_Grab, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_No_Quit, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Portrait, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Curses, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Show_Cursor, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_Start_CPU, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_FDD_Boot, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_Win2K_Hack, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_RTC_TD_Hack, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_Start_Date, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.DTE_Start_Date, SIGNAL(dateTimeChanged(const QDateTime &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Init_Graphic_Mode, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.SB_InitGM_Width, SIGNAL(valueChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.SB_InitGM_Height, SIGNAL(valueChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CB_InitGM_Depth, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(VM_Changed()) );

	// Advanced Options
	connect( ui_ao.Edit_Additional_Args, SIGNAL(textChanged()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_Only_User_Args, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_ao.CH_Use_User_Binary, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	// Hardware Virtualization Tab

	/*connect( ui_kvm.CH_No_KVM_IRQChip, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );*/ //FIXME: use new non-kvm option

	/*connect( ui_kvm.CH_No_KVM_Pit, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );*/ //possibly remove

	connect( ui_kvm.CH_KVM_Shadow_Memory, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui_kvm.SB_KVM_Shadow_Memory_Size, SIGNAL(valueChanged(int)),
			 this, SLOT(VM_Changed()) );

	// SPICE
	connect( SPICE_Widget, SIGNAL(State_Changed()),
			 this, SLOT(VM_Changed()) );

	// VNC Tab
	connect( ui.CH_Activate_VNC, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.RB_VNC_Display_Number, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );

	connect( ui.SB_VNC_Display, SIGNAL(valueChanged(int)),
			 this, SLOT(VM_Changed()) );

	connect( ui.RB_VNC_Unix_Socket, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_VNC_Password, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_Use_VNC_TLS, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_x509_Folder, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_x509_Folder, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_x509verify_Folder, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_x509verify_Folder, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	// Optional Images
	connect( ui.CH_ROM_File, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_ROM_File, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_MTDBlock, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_MTDBlock_File, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_SD_Image, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_SD_Image_File, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.CH_PFlash, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_PFlash_File, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	// Boot Linux Kernel
	connect( ui.CH_Use_Linux_Boot, SIGNAL(clicked()),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_Linux_bzImage_Path, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_Linux_Initrd_Path, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_Linux_Command_Line, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );


	connect( Folder_Sharing, SIGNAL(Folder_Changed()),
	         this, SLOT(VM_Changed()) );

	connect( Dev_Manager, SIGNAL(Device_Changed()),
			 this, SLOT(VM_Changed()) );

    connect( ui.SB_VNC_Display, SIGNAL(valueChanged(int)), this, SLOT(SB_VNC_Display_changed(int)));
    connect( ui.SB_VNC_Display_Port, SIGNAL(valueChanged(int)), this, SLOT(SB_VNC_Display_Port_changed(int)));

}

void Main_Window::SB_VNC_Display_changed(int num)
{
    ui.SB_VNC_Display_Port->setValue(5900+num);
}

void Main_Window::SB_VNC_Display_Port_changed(int port)
{
    ui.SB_VNC_Display->setValue(port-5900);
}

const QMap<QString, Available_Devices> Main_Window::Get_Devices_Info( bool *ok ) const
{
	// Get current emulator
	Emulator curEmul;
	QMap<QString, Available_Devices> retList;

	curEmul = Get_Default_Emulator();

	if( curEmul.Get_Name().isEmpty() )
	{
		AQError( "QList<Available_Devices> &Main_Window::Get_Devices_Info( bool *ok )",
				 "Emulator empty!" );
		*ok = false;
		return retList;
	}

	*ok = true;
	return curEmul.Get_Devices();
}

Available_Devices Main_Window::Get_Current_Machine_Devices( bool *ok ) const
{
	// Get all devices
	bool devOk = false;
	QMap<QString, Available_Devices> allDevList = Get_Devices_Info( &devOk );

	if( ! devOk )
	{
		AQError( "Available_Devices Main_Window::Get_Current_Machine_Devices( bool *ok ) const",
				 "Cannot get devices!" );
		*ok = false;
		return Available_Devices();
	}

	// UI not ready yet (startup / empty combo) — not an error.
	if( ui.CB_Computer_Type->count() <= 0 ||
		ui.CB_Computer_Type->currentIndex() < 0 ||
		ui.CB_Computer_Type->currentText().isEmpty() )
	{
		*ok = false;
		return Available_Devices();
	}

	// Find current device
	for( QMap<QString, Available_Devices>::const_iterator ix = allDevList.constBegin(); ix != allDevList.constEnd(); ++ix )
	{
		if( ui.CB_Computer_Type->currentText() == ix.value().System.Caption )
        {
			*ok = true;
			Available_Devices d = ix.value();
			if( System_Info::Emulator_QEMU_2_0.contains( ix.key() ) )
			{
				const Available_Devices &fb = System_Info::Emulator_QEMU_2_0[ ix.key() ];
				d.PSO_SMP_Count = qMax( d.PSO_SMP_Count, fb.PSO_SMP_Count );
				d.PSO_SMP_Cores = d.PSO_SMP_Cores || fb.PSO_SMP_Cores;
				d.PSO_SMP_Threads = d.PSO_SMP_Threads || fb.PSO_SMP_Threads;
				d.PSO_SMP_Sockets = d.PSO_SMP_Sockets || fb.PSO_SMP_Sockets;
				d.PSO_SMP_MaxCPUs = d.PSO_SMP_MaxCPUs || fb.PSO_SMP_MaxCPUs;
			}
			System_Info::Normalize_Virt_Arch_Devices( d );
			return d;
		}
    }

	// Not found
	AQError( "Available_Devices Main_Window::Get_Current_Machine_Devices( bool *ok ) const",
			 "Cannot get current machine device!" );
	*ok = false;
	return Available_Devices();
}

bool Main_Window::Create_VM_From_Ui( Virtual_Machine *tmp_vm, Virtual_Machine *old_vm, bool show_user_errors )
{
    std::unique_ptr<Disable_User_Graphic_Warning> dugw;
    if ( show_user_errors == false )
        dugw.reset(new Disable_User_Graphic_Warning());

	if( old_vm == NULL )
	{
        if ( show_user_errors )
    		AQError( "bool Main_Window::Create_VM_From_Ui( Virtual_Machine *tmp_vm, Virtual_Machine *old_vm )",
				 "old_vm == NULL" );

        return false;
	}

	// Save file name
	tmp_vm->Set_VM_XML_File_Path( old_vm->Get_VM_XML_File_Path() );

	// UID
	tmp_vm->Set_UID( old_vm->Get_UID() );

	// Preserve runtime/state fields the UI does not edit
	tmp_vm->Set_State( old_vm->Get_State() );
	tmp_vm->Set_Snapshots( old_vm->Get_Snapshots() );

	// Machine Name
	if( ui.Edit_Machine_Name->text().isEmpty() )
	{
        if ( show_user_errors )
    		AQGraphic_Warning( tr("Error!"), tr("VM Name is Empty!") );

        return false;
	}
	else
	{
		tmp_vm->Set_Machine_Name( ui.Edit_Machine_Name->text() );
	}

	// Icon Path
	for( int ix = 0; ix < ui.Machines_List->count(); ix++ )
	{
		if( ui.Machines_List->item(ix)->data(256).toString() == old_vm->Get_UID() )
		{
			if( ui.Machines_List->item(ix)->data(128).toString() ==
				QDir::toNativeSeparators(Settings.value("VM_Directory", "~").toString() + Get_FS_Compatible_VM_Name(ui.Edit_Machine_Name->text())) )
			{
				tmp_vm->Set_Icon_Path( old_vm->Get_Icon_Path() );
				tmp_vm->Set_Screenshot_Path( ui.Machines_List->item(ix)->data(128).toString() );
			}
			else
			{
				tmp_vm->Set_Icon_Path( ui.Machines_List->item(ix)->data(128).toString() );
			}
		}
	}

	// Get devices
	bool curMachineOk = false;
	Available_Devices curComp = Get_Current_Machine_Devices( &curMachineOk );
	if( ! curMachineOk )
    {
        return false;
    }

    // Machine Accelerator (prefer UserRole id over translated text)
	{
		const QVariant accel_data = ui.CB_Machine_Accelerator->currentData( Qt::UserRole );
		if( accel_data.isValid() && ! accel_data.toString().isEmpty() )
			tmp_vm->Set_Machine_Accelerator( VM::String_To_Accel( accel_data.toString() ) );
		else
			tmp_vm->Set_Machine_Accelerator( VM::String_To_Accel( ui.CB_Machine_Accelerator->currentText() ) );
	}

	// Computer Type
	tmp_vm->Set_Computer_Type( curComp.System.QEMU_Name );

	// Machine Type
	if( ui_arch.CB_Machine_Type->currentIndex() != -1 && ui_arch.CB_Machine_Type->currentIndex() < curComp.Machine_List.count() )
	{
		tmp_vm->Set_Machine_Type( curComp.Machine_List[ui_arch.CB_Machine_Type->currentIndex()].QEMU_Name );
	}
	else if( ! ui_arch.CB_Machine_Type->currentText().isEmpty() )
	{
		tmp_vm->Set_Machine_Type( ui_arch.CB_Machine_Type->currentText() );
	}
	else
	{
		tmp_vm->Set_Machine_Type( old_vm->Get_Machine_Type() );
	}

	// CPU Type
	if( ui_arch.CB_CPU_Type->currentIndex() != -1 && ui_arch.CB_CPU_Type->currentIndex() < curComp.CPU_List.count() )
	{
		tmp_vm->Set_CPU_Type( curComp.CPU_List[ui_arch.CB_CPU_Type->currentIndex()].QEMU_Name );
	}
	else if( ! ui_arch.CB_CPU_Type->currentText().isEmpty() )
	{
		tmp_vm->Set_CPU_Type( ui_arch.CB_CPU_Type->currentText() );
	}
	else
	{
		tmp_vm->Set_CPU_Type( old_vm->Get_CPU_Type() );
	}

	// Create Emulator Info
	Emulator tmp_emul = Get_Default_Emulator();
	tmp_emul.Set_Name( "" );
	tmp_vm->Set_Emulator( tmp_emul );

	// Video
    {
		QString video_name = System_Info::Sanitize_Video_Card(
			tmp_vm->Get_Computer_Type(),
			old_vm->Get_Video_Card(),
			tmp_vm->Get_Machine_Type() );

		const QVariant data = ui.CB_Video_Card->currentData( Qt::UserRole );
		if( data.isValid() && ! data.toString().isEmpty() )
			video_name = System_Info::Sanitize_Video_Card(
				tmp_vm->Get_Computer_Type(), data.toString(), tmp_vm->Get_Machine_Type() );

		tmp_vm->Set_Video_Card( video_name );
    }

	// Display resolution (VirtIO-GPU EDID)
	{
		const QVariant data = ui.CB_Display_Resolution->currentData( Qt::UserRole );
		if( data.isValid() && ! data.toString().isEmpty() )
			tmp_vm->Set_Display_Resolution( data.toString() );
		else
			tmp_vm->Set_Display_Resolution( QStringLiteral( "native" ) );
	}

	// Mouse / pointer
	{
		const QVariant mt = ui.CB_Mouse_Type->currentData( Qt::UserRole );
		tmp_vm->Set_Mouse_Type( mt.isValid() ? mt.toString() : QStringLiteral( "ps2" ) );

		const QVariant mc = ui.CB_Mouse_USB_Controller->currentData( Qt::UserRole );
		tmp_vm->Set_Mouse_USB_Controller( mc.isValid() ? mc.toString() : QStringLiteral( "auto" ) );

		const QVariant mv = ui.CB_Mouse_USB_Version->currentData( Qt::UserRole );
		tmp_vm->Set_Mouse_USB_Version( mv.isValid() ? mv.toInt() : 0 );

		const QVariant am = ui.CB_SPICE_Agent_Mouse->currentData( Qt::UserRole );
		tmp_vm->Set_SPICE_Agent_Mouse( am.isValid() ? am.toString() : QStringLiteral( "default" ) );
	}

	// CPU Count — CB_CPU_Count is authoritative; Set_SMP() must not wipe it with a stale dialog value.
    if( ! Validate_CPU_Count(ui.CB_CPU_Count->currentText()) )
    {
        return false;
    }
	{
		VM::SMP_Options smp = SMP_Settings->Get_Values();
		smp.SMP_Count = ui.CB_CPU_Count->currentText().toInt();
		if( smp.SMP_Count < 1 )
			smp.SMP_Count = 1;
		tmp_vm->Set_SMP( smp );
		tmp_vm->Set_SMP_CPU_Count( smp.SMP_Count );
	}

	// Keyboard Layout
	if( ui.CB_Keyboard_Layout->currentIndex() == 0 ) // Default
		tmp_vm->Set_Keyboard_Layout( "Default" );
	else
		tmp_vm->Set_Keyboard_Layout( ui.CB_Keyboard_Layout->currentText() );

	// Boot Priority
	tmp_vm->Set_Boot_Order_List( Boot_Order_List );
	tmp_vm->Set_Show_Boot_Menu( Show_Boot_Menu );

	// Audio
	VM::Sound_Cards snd_card;

	snd_card.Audio_sb16 = ui.CH_sb16->isChecked();
	snd_card.Audio_es1370 = ui.CH_es1370->isChecked();
	snd_card.Audio_Adlib = ui.CH_Adlib->isChecked();
	snd_card.Audio_PC_Speaker = ui.CH_PCSPK->isChecked();
	snd_card.Audio_GUS = ui.CH_GUS->isChecked();
	snd_card.Audio_AC97 = ui.CH_AC97->isChecked();
	snd_card.Audio_HDA = ui.CH_HDA->isChecked();
	snd_card.Audio_cs4231a = ui.CH_cs4231a->isChecked();
	snd_card.Audio_VirtIO = ui.CH_VirtIO_Sound->isChecked();
	snd_card.Audio_USB = ui.CH_USB_Audio->isChecked();

	tmp_vm->Set_Audio_Cards( snd_card );

	// Memory
	tmp_vm->Set_Memory_Size( ui.Memory_Size->value() );

	// Check free ram
	tmp_vm->Set_Remove_RAM_Size_Limitation( ui.CH_Remove_RAM_Size_Limitation->isChecked() );

	// Options
	tmp_vm->Use_Fullscreen_Mode( ui.CH_Fullscreen->isChecked() );
	tmp_vm->Use_Win2K_Hack( ui_ao.CH_Win2K_Hack->isChecked() );
	tmp_vm->Use_Local_Time( ui.CH_Local_Time->isChecked() );

	tmp_vm->Use_Check_FDD_Boot_Sector( ui_ao.CH_FDD_Boot->isChecked() );
	tmp_vm->Use_ACPI( ui_ao.CH_ACPI->isChecked() );
	tmp_vm->Use_Snapshot_Mode( ui.CH_Snapshot->isChecked() );
	tmp_vm->Use_Start_CPU( ui_ao.CH_Start_CPU->isChecked() );
	tmp_vm->Use_No_Reboot( ui_ao.CH_No_Reboot->isChecked() );
	tmp_vm->Use_No_Shutdown( ui_ao.CH_No_Shutdown->isChecked() );

	tmp_vm->Set_FD0( Dev_Manager->Floppy1 );
	tmp_vm->Set_FD1( Dev_Manager->Floppy2 );
	tmp_vm->Set_CD_ROM( Dev_Manager->CD_ROM );

	tmp_vm->Set_HDA( Dev_Manager->HDA );
	tmp_vm->Set_HDB( Dev_Manager->HDB );
	tmp_vm->Set_HDC( Dev_Manager->HDC );
	tmp_vm->Set_HDD( Dev_Manager->HDD );

	// Disk bus from VM page (primary HDA)
	{
		VM_HDD hda = tmp_vm->Get_HDA();
		if( hda.Get_Enabled() )
		{
			VM_Native_Storage_Device native = hda.Get_Native_Device();
			native.Use_Interface( true );
			switch( ui.CB_Disk_Interface->currentIndex() )
			{
				case 0: native.Set_Interface( VM::DI_Virtio ); break;
				case 1: native.Set_Interface( VM::DI_Virtio_SCSI ); break;
				case 2: native.Set_Interface( VM::DI_SCSI ); break;
				case 3: native.Set_Interface( VM::DI_IDE ); break;
				case 4: native.Set_Interface( VM::DI_SD ); break;
				default: native.Set_Interface( VM::DI_Virtio ); break;
			}
			if( ! native.Use_File_Path() )
			{
				native.Use_File_Path( true );
				native.Set_File_Path( hda.Get_File_Name() );
			}
			hda.Set_Native_Device( native );
			tmp_vm->Set_HDA( hda );
		}
	}

	tmp_vm->Set_Storage_Devices_List( Dev_Manager->Storage_Devices );

    // Shared Folders
	tmp_vm->Set_Shared_Folders_List( Folder_Sharing->Shared_Folders );

	// Network Tab
	tmp_vm->Set_Use_Network( ui.CH_Use_Network->isChecked() );

	// Use Nativ Network
	tmp_vm->Use_Native_Network( ui.RB_Network_Mode_New->isChecked() );

	// Redirections List
	if( ui.CH_Redirections->isChecked() && ui.Redirections_List->rowCount() < 1 )
	{
        if ( show_user_errors )
    		AQGraphic_Warning( tr("Error!"), tr("Redirection List is Empty! Please Disable Redirections!") );
		return false;
	}

	// Redirections
	tmp_vm->Set_Use_Redirections( ui.CH_Redirections->isChecked() );

	// Redirections List
	for( int rx = 0; rx < ui.Redirections_List->rowCount(); rx++ )
	{
		VM_Redirection tmp_redir;

        auto item = ui.Redirections_List->item(rx, 0);

        if ( item == nullptr )
            continue;

        if( ui.Redirections_List->item(rx, 0)->text() == "TCP" )
            tmp_redir.Set_Protocol( "TCP" );
        else
            tmp_redir.Set_Protocol( "UDP" );

        if ( ui.Redirections_List->item(rx, 1) == nullptr ||
             ui.Redirections_List->item(rx, 2) == nullptr ||
             ui.Redirections_List->item(rx, 3) == nullptr )
            continue;

		tmp_redir.Set_Host_Port( ui.Redirections_List->item(rx, 1)->text().toInt() );
		tmp_redir.Set_Guest_IP( ui.Redirections_List->item(rx, 2)->text() );
		tmp_redir.Set_Guest_Port( ui.Redirections_List->item(rx, 3)->text().toInt() );

		tmp_vm->Add_Network_Redirection( tmp_redir );
	}

	// TFTP
	tmp_vm->Set_TFTP_Prefix( ui.Edit_TFTP_Prefix->text() );

	// SMB Dir
	tmp_vm->Set_SMB_Directory( ui.Edit_SMB_Folder->text() );

	// Network Cards
	QList<VM_Net_Card> tmp_net_cards;
    if( Old_Network_Settings_Widget->Get_Network_Cards(tmp_net_cards) )
	{
		tmp_vm->Set_Network_Cards( tmp_net_cards );
	}
    else
    {
        return false;
    }

	// Nativ
	QList<VM_Net_Card_Native> tmp_net_cards_nativ;
	if( New_Network_Settings_Widget->Get_Network_Cards(tmp_net_cards_nativ) )
	{
		tmp_vm->Set_Network_Cards_Nativ( tmp_net_cards_nativ );
	}
    else
    {
        return false;
    }

	// Port page
	tmp_vm->Set_Serial_Ports( Ports_Tab->Get_Serial_Ports() );
	tmp_vm->Set_Parallel_Ports( Ports_Tab->Get_Parallel_Ports() );
	tmp_vm->Set_USB_Ports( Ports_Tab->Get_USB_Ports() );

	// Other Page
	tmp_vm->Set_Use_Linux_Boot( ui.CH_Use_Linux_Boot->isChecked() );
	tmp_vm->Set_bzImage_Path( ui.Edit_Linux_bzImage_Path->text() );
	tmp_vm->Set_Initrd_Path( ui.Edit_Linux_Initrd_Path->text() );
	tmp_vm->Set_Kernel_ComLine( ui.Edit_Linux_Command_Line->text() );

	// Optional Images
	// ROM File
	tmp_vm->Set_Use_ROM_File( ui.CH_ROM_File->isChecked() );
	tmp_vm->Set_ROM_File( ui.Edit_ROM_File->text() );

	// On-Board Flash Image
	tmp_vm->Use_MTDBlock_File( ui.CH_MTDBlock->isChecked() );
	tmp_vm->Set_MTDBlock_File( ui.Edit_MTDBlock_File->text() );

	// SecureDigital Card Image
	tmp_vm->Use_SecureDigital_File( ui.CH_SD_Image->isChecked() );
	tmp_vm->Set_SecureDigital_File( ui.Edit_SD_Image_File->text() );

	// Parallel Flash Image
	tmp_vm->Use_PFlash_File( ui.CH_PFlash->isChecked() );
	tmp_vm->Set_PFlash_File( ui.Edit_PFlash_File->text() );

	// UEFI / VirtIO extras (set by Windows 11 ARM wizard; preserve until full UI exists)
	tmp_vm->Use_UEFI( old_vm->Use_UEFI() );
	tmp_vm->Set_UEFI_CODE_File( old_vm->Get_UEFI_CODE_File() );
	tmp_vm->Set_UEFI_VARS_File( old_vm->Get_UEFI_VARS_File() );
	tmp_vm->Use_VirtIO_RNG( old_vm->Use_VirtIO_RNG() );
	tmp_vm->Use_VirtIO_Balloon( old_vm->Use_VirtIO_Balloon() );
	tmp_vm->Use_VirtIO_Keyboard( old_vm->Use_VirtIO_Keyboard() );
	tmp_vm->Use_USB_Hub( old_vm->Use_USB_Hub() );
	tmp_vm->Set_Win11_Lifecycle_Mode( old_vm->Get_Win11_Lifecycle_Mode() );

	// Additional QEMU Arguments
	tmp_vm->Set_Additional_Args( ui_ao.Edit_Additional_Args->toPlainText() );

	// Only_User_Args
	tmp_vm->Set_Only_User_Args( ui_ao.CH_Only_User_Args->isChecked() );

	// Use_User_Emulator_Binary
	tmp_vm->Set_Use_User_Emulator_Binary( ui_ao.CH_Use_User_Binary->isChecked() );

	/*// Disable KVM kernel mode PIC/IOAPIC/LAPIC
	tmp_vm->Use_KVM_IRQChip( ui_kvm.CH_No_KVM_IRQChip->isChecked() );

	// Disable KVM kernel mode PIT
	tmp_vm->Use_No_KVM_Pit( ui_kvm.CH_No_KVM_Pit->isChecked() );

	// KVM_No_Pit_Reinjection
	tmp_vm->Use_KVM_No_Pit_Reinjection( ui_kvm.CH_KVM_No_Pit_Reinjection->isChecked() );

	// KVM_Nesting
	tmp_vm->Use_KVM_Nesting( ui_kvm.CH_KVM_Nesting->isChecked() );*/ //FIXME: deprecated stuff //are there replacements?

	// KVM Shadow Memory
	tmp_vm->Use_KVM_Shadow_Memory( ui_kvm.CH_KVM_Shadow_Memory->isChecked() );
	tmp_vm->Set_KVM_Shadow_Memory_Size( ui_kvm.SB_KVM_Shadow_Memory_Size->value() );

	// Initial Graphical Mode
	VM_Init_Graphic_Mode tmp_mode;

	tmp_mode.Set_Enabled( ui.CH_Init_Graphic_Mode->isChecked() );
	tmp_mode.Set_Width( ui.SB_InitGM_Width->value() );
	tmp_mode.Set_Height( ui.SB_InitGM_Height->value() );

	switch( ui.CB_InitGM_Depth->currentIndex() )
	{
		case 0:
			tmp_mode.Set_Depth( 8 );
			break;

		case 1:
			tmp_mode.Set_Depth( 16 );
			break;

		case 2:
			tmp_mode.Set_Depth( 24 );
			break;

		case 3:
			tmp_mode.Set_Depth( 32 );
			break;

		default:
            if ( show_user_errors && ui.CB_InitGM_Depth->currentIndex() != -1 )
    			AQError( "bool Main_Window::Create_VM_From_Ui( Virtual_Machine *tmp_vm, QListWidgetItem *item )",
					 "Initial Graphical Mode: Default Section!" );
			tmp_mode.Set_Depth( 24 );
			break;
	}

	tmp_vm->Set_Init_Graphic_Mode( tmp_mode );

	// Show QEMU Window Without a Frame and Window Decorations
	tmp_vm->Use_No_Frame( ui.CH_No_Frame->isChecked() );

	// Use Ctrl-Alt-Shift to Grab Mouse (Instead of Ctrl-Alt)
	tmp_vm->Use_Alt_Grab( ui.CH_Alt_Grab->isChecked() );

	// Disable SDL Window Close Capability
	tmp_vm->Use_No_Quit( ui.CH_No_Quit->isChecked() );

	// Rotate Graphical Output 90 Deg Left (Only PXA LCD)
	tmp_vm->Use_Portrait( ui.CH_Portrait->isChecked() );

	// Show_Cursor
	tmp_vm->Use_Show_Cursor( ui.CH_Show_Cursor->isChecked() );

	// Curses
	tmp_vm->Use_Curses( ui.CH_Curses->isChecked() );

	// RTC_TD_Hack
	tmp_vm->Use_RTC_TD_Hack( ui_ao.CH_RTC_TD_Hack->isChecked() );

	// Start Date
	tmp_vm->Use_Start_Date( ui_ao.CH_Start_Date->isChecked() );
	tmp_vm->Set_Start_Date( ui_ao.DTE_Start_Date->dateTime() );

	// SPICE
	bool spiceSettingsOK = false;
	tmp_vm->Set_SPICE( SPICE_Widget->Get_Settings( spiceSettingsOK ) );
    if( ! spiceSettingsOK )
    {
        return false;
    }

	// VNC
	tmp_vm->Use_VNC( ui.CH_Activate_VNC->isChecked() );

	// Use Unix Socket Mode for VNC
	tmp_vm->Set_VNC_Socket_Mode( ui.RB_VNC_Unix_Socket->isChecked() );

	// UNIX Domain Socket Path
	tmp_vm->Set_VNC_Unix_Socket_Path( ui.Edit_VNC_Unix_Socket->text() );

	// VNC Display Number
	tmp_vm->Set_VNC_Display_Number( ui.SB_VNC_Display->value() );

	// Use Password for VNC
	tmp_vm->Use_VNC_Password( ui.CH_VNC_Password->isChecked() );

	// Use TLS
	tmp_vm->Use_VNC_TLS( ui.CH_Use_VNC_TLS->isChecked() );

	// Use x509
	tmp_vm->Use_VNC_x509( ui.CH_x509_Folder->isChecked() );

	// x509 Folder
	tmp_vm->Set_VNC_x509_Folder_Path( ui.Edit_x509_Folder->text() );

	// Use x509verify
	tmp_vm->Use_VNC_x509verify( ui.CH_x509verify_Folder->isChecked() );

	// x509 Folder
	tmp_vm->Set_VNC_x509verify_Folder_Path( ui.Edit_x509verify_Folder->text() );

	return true;
}

bool Main_Window::Load_Settings()
{
	// Main Window Size
	resize( Settings.value("General_Window_Width", "885").toInt(),
			Settings.value("General_Window_Height", "544").toInt() );

	// Main Window Position
	move( Settings.value("General_Window_Position", QPoint(300, 300)).toPoint() );

	// Toolbar State
	restoreState( Settings.value("General_Window_State").toByteArray());

	// Session mode hides these; that visibility must never stick after restart.
	ui.Tool_Bar_VM_Manage->setVisible( true );
	ui.Tool_Bar_VM_Control->setVisible( true );

	// Splitter
	ui.splitter->restoreState( Settings.value("General_Splitter",
							   QByteArray("\0\0\0\xff\0\0\0\0\0\0\0\x2\0\0\0\xbc\0\0\x2$\0\0\0\0\x4\x1\0\0\0\x1")).toByteArray() );

	// VM Icons Size
	ui.Machines_List->setIconSize( QSize( Settings.value("VM_Icons_Size", "48").toInt(),
								   Settings.value("VM_Icons_Size", "48").toInt()) );

	// Load CD Exists Images List
	VM_Folder = QDir::toNativeSeparators( Settings.value("VM_Directory", "~").toString() );
	Load_Recent_Images_List();

	if( Settings.status() == QSettings::NoError )
	{

		//	if( ui.Machines_List->count() > 0 ) Update_VM_Ui();

		return true;
	}
	else
	{
		AQError( "bool Main_Window::Load_Settings()",
				 "Settings.status() != QSettings::NoError" );
		return false;
	}
}

bool Main_Window::Save_Settings()
{
	// Current VM Index
	Settings.setValue( "Current_VM_Index", ui.Machines_List->currentRow() );

	// Save Windows Size
	Settings.setValue( "General_Window_Width", QString::number(this->width()) );
	Settings.setValue( "General_Window_Height", QString::number(this->height()) );

	// Save Toolbar State — never persist session-hidden left bars
	const bool manage_vis = ui.Tool_Bar_VM_Manage->isVisible();
	const bool control_vis = ui.Tool_Bar_VM_Control->isVisible();
	ui.Tool_Bar_VM_Manage->setVisible( true );
	ui.Tool_Bar_VM_Control->setVisible( true );
	Settings.setValue( "General_Window_State", saveState() );
	ui.Tool_Bar_VM_Manage->setVisible( manage_vis );
	ui.Tool_Bar_VM_Control->setVisible( control_vis );

	// Save Main Window Position
	Settings.setValue( "General_Window_Position", pos() );

	// Splitter
	Settings.setValue( "General_Splitter", ui.splitter->saveState() );

	// Save
	Settings.sync();

	if( Settings.status() == QSettings::NoError ) return true;
	else return false;
}

bool Main_Window::Load_Virtual_Machines()
{
	QDir vm_dir( QDir::toNativeSeparators(Settings.value("VM_Directory", "~").toString()) );
	QFileInfoList fil = vm_dir.entryInfoList( QStringList("*.aqemu"), QDir::Files, QDir::Name );

	if( fil.count() <= 0 ) return false;

	int real_index = 0;
	for( int ix = 0; ix < fil.count(); ix++ )
	{
		// Check Permissions
		if( ! fil[ix].isWritable() )
		{
			AQGraphic_Error( "bool Main_Window::Load_Virtual_Machines()", tr("Error!"),
							 tr("VM File \"") + fil[ix].filePath() + tr("\" is Read Only!\nCheck Permissions!"), true );
		}

		Virtual_Machine *new_vm = new Virtual_Machine();

		if( ! new_vm->Load_VM(fil[ix].filePath()) )
		{
			--real_index;
		}
		else
		{
			new_vm->Set_UID( QUuid::createUuid().toString() ); // Create UID

			QObject::connect( new_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
							  this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

			QListWidgetItem *item = new QListWidgetItem( new_vm->Get_Machine_Name(), ui.Machines_List );
			item->setData( 256, new_vm->Get_UID() );

			// Load OS Logo or OS Screenshot Icon
			if( new_vm->Get_State() == VM::VMS_Saved &&
				Settings.value("Use_Screenshot_for_OS_Logo", "yes").toString() == "yes" )
			{
				// Screenshot File Not Found? Use OS Icon.
				if( QFile::exists(new_vm->Get_Screenshot_Path()) )
					item->setIcon( QIcon(new_vm->Get_Screenshot_Path()) );
				else
					item->setIcon( QIcon(new_vm->Get_Icon_Path()) );
			}
			else
			{
				item->setIcon( QIcon(new_vm->Get_Icon_Path()) );
			}

			// Append new VM
			VM_List << new_vm;

		}

		++real_index;
	}

    AQEMU_Service::get().call("status");

	// Set last used vm
	int cur_row = Settings.value( "Current_VM_Index", 0 ).toInt();

	if( cur_row >= 0 )
	{
		if( cur_row < ui.Machines_List->count() )
		{
			ui.Machines_List->setCurrentRow( cur_row );
		}
		else
		{
			AQWarning( "bool Main_Window::Load_Virtual_Machines()", "cur_row > ui.Machines_List->count()" );
			ui.Machines_List->setCurrentRow( 0 );
		}
	}
	else
	{
		AQWarning( "bool Main_Window::Load_Virtual_Machines()", "cur_row < 0" );
		ui.Machines_List->setCurrentRow( 0 );
	}

	Update_VM_Ui();

	return true;
}

bool Main_Window::Save_Virtual_Machines()
{
	return true;
}

void Main_Window::Update_VM_Ui(bool update_info_tab)
{
    Block_VM_Changed_Signals bvmcs(this);

	Update_VM_Port_Number();

	if( ui.Machines_List->currentRow() < 0 )
	{
		AQWarning( "void Main_Window::Update_VM_Ui()",
				   "VM Index Out of Range" );
		/*
		AQGraphic_Error( "void Main_Window::Update_VM_Ui()", tr("Critical Error!"),
						 tr("VM Index Out of Range! Close AQEMU?"), true );*/
		return;
	}

	Virtual_Machine *tmp_vm = Get_Current_VM();

	if( tmp_vm == NULL )
	{
		AQError( "void Main_Window::Update_VM_Ui()",
				 "Cannot Find VM!" );
		return;
	}

	// Machine Name
	ui.Edit_Machine_Name->setText( tmp_vm->Get_Machine_Name() );

	Show_State_Current( tmp_vm );
    Show_State_VM( tmp_vm);

	if( tmp_vm->Get_State() == VM::VMS_In_Error )
	{
		AQError( "void Main_Window::Update_VM_Ui()",
				 "VM in VM::VMS_In_Error state!" );
		return;
	}

    int found = false;
	const QString want_accel = VM::Accel_To_String( tmp_vm->Get_Machine_Accelerator() ).toLower();
	for( int ix = 0; ix < ui.CB_Machine_Accelerator->count(); ix++ )
	{
		const QString id = ui.CB_Machine_Accelerator->itemData( ix, Qt::UserRole ).toString().toLower();
		const QString caption = ui.CB_Machine_Accelerator->itemText( ix ).toLower();
		if( id == want_accel || caption == want_accel )
		{
			ui.CB_Machine_Accelerator->setCurrentIndex( ix );
            found = true;
			break;
		}
	}

    if ( ! found )
    {
    	ui.CB_Machine_Accelerator->setCurrentIndex( 0 );
    }

	Enforce_Accel_Honesty();

	/*if( ui.CB_Machine_Accelerator->count() <= 0 )
	{
		AQError( "void Main_Window::Update_VM_Ui()",
				 "ui.CB_Machine_Accelerator->count() <= 0" );
		return;
	}*/

	// Get current VM devices
	Available_Devices curComp = tmp_vm->Get_Emulator().Get_Devices()[ tmp_vm->Get_Computer_Type() ];
	if( System_Info::Emulator_QEMU_2_0.contains( tmp_vm->Get_Computer_Type() ) )
	{
		const Available_Devices &fb = System_Info::Emulator_QEMU_2_0[ tmp_vm->Get_Computer_Type() ];
		curComp.PSO_SMP_Count = qMax( curComp.PSO_SMP_Count, fb.PSO_SMP_Count );
		curComp.PSO_SMP_Cores = curComp.PSO_SMP_Cores || fb.PSO_SMP_Cores;
		curComp.PSO_SMP_Threads = curComp.PSO_SMP_Threads || fb.PSO_SMP_Threads;
		curComp.PSO_SMP_Sockets = curComp.PSO_SMP_Sockets || fb.PSO_SMP_Sockets;
		curComp.PSO_SMP_MaxCPUs = curComp.PSO_SMP_MaxCPUs || fb.PSO_SMP_MaxCPUs;
	}
	System_Info::Normalize_Virt_Arch_Devices( curComp );

	if( curComp.System.QEMU_Name.isEmpty() )
	{
		AQError( "void Main_Window::Update_VM_Ui()",
				 "cur_comp not valid!" );
		return;
	}

	// Computer Type
	int compTypeIndex = ui.CB_Computer_Type->findText( curComp.System.Caption );

	if( compTypeIndex != -1 )
		ui.CB_Computer_Type->setCurrentIndex( compTypeIndex );
	else
	{
		AQError( "void Main_Window::Update_VM_Ui()",
				 "Cannot find computer type index!" );
		return;
	}

	// Machine Type
	QString tmp_str = tmp_vm->Get_Machine_Type();
	bool machine_found = false;
	for( int mx = 0; mx < curComp.Machine_List.count(); ++mx )
	{
		if( tmp_str == curComp.Machine_List[mx].QEMU_Name )
		{
			ui_arch.CB_Machine_Type->setCurrentIndex( mx );
			ui.CB_Machine_Type_Main->setCurrentIndex( mx );
			machine_found = true;
			break;
		}
	}
	if( ! machine_found && ! tmp_str.isEmpty() )
	{
		ui_arch.CB_Machine_Type->addItem( tmp_str );
		ui.CB_Machine_Type_Main->addItem( tmp_str );
		ui_arch.CB_Machine_Type->setCurrentIndex( ui_arch.CB_Machine_Type->count() - 1 );
		ui.CB_Machine_Type_Main->setCurrentIndex( ui.CB_Machine_Type_Main->count() - 1 );
	}

	// CPU Type
	tmp_str = tmp_vm->Get_CPU_Type();
	bool cpu_found = false;
	for( int cx = 0; cx < curComp.CPU_List.count(); ++cx )
	{
		if( tmp_str == curComp.CPU_List[cx].QEMU_Name )
		{
			ui_arch.CB_CPU_Type->setCurrentIndex( cx );
			ui.CB_CPU_Type_Main->setCurrentIndex( cx );
			cpu_found = true;
			break;
		}
	}
	if( ! cpu_found && ! tmp_str.isEmpty() )
	{
		ui_arch.CB_CPU_Type->addItem( tmp_str );
		ui.CB_CPU_Type_Main->addItem( tmp_str );
		ui_arch.CB_CPU_Type->setCurrentIndex( ui_arch.CB_CPU_Type->count() - 1 );
		ui.CB_CPU_Type_Main->setCurrentIndex( ui.CB_CPU_Type_Main->count() - 1 );
	}

	// Video Card
	tmp_str = System_Info::Sanitize_Video_Card(
		tmp_vm->Get_Computer_Type(), tmp_vm->Get_Video_Card(), tmp_vm->Get_Machine_Type() );
	if( tmp_str != tmp_vm->Get_Video_Card() )
		tmp_vm->Set_Video_Card( tmp_str );

	ui.CB_Video_Card->clear();
	for( int vx = 0; vx < curComp.Video_Card_List.count(); ++vx )
	{
		const Device_Map &vc = curComp.Video_Card_List[vx];
		ui.CB_Video_Card->addItem( vc.Caption, vc.QEMU_Name );
	}

	const int video_index = ui.CB_Video_Card->findData( tmp_str );
	if( video_index >= 0 )
		ui.CB_Video_Card->setCurrentIndex( video_index );
	else if( ui.CB_Video_Card->count() > 0 )
		ui.CB_Video_Card->setCurrentIndex( 0 );

	Apply_Display_Resolution_To_Ui( tmp_vm->Get_Display_Resolution() );
	Update_Display_Resolution_Enabled();

	// Keyboard Layout
	int lang_index = ui.CB_Keyboard_Layout->findText( tmp_vm->Get_Keyboard_Layout() );

	if( lang_index >= 0 && lang_index < ui.CB_Keyboard_Layout->count() )
	{
		ui.CB_Keyboard_Layout->setCurrentIndex( lang_index );
	}
	else
	{
		ui.CB_Keyboard_Layout->setCurrentIndex( 0 ); // default lang
	}

	Apply_Mouse_Settings_To_Ui( tmp_vm );
	Update_Mouse_Options_Enabled();

	// Boot
	Set_Boot_Order( tmp_vm->Get_Boot_Order_List() );
	Boot_Order_List = tmp_vm->Get_Boot_Order_List();
	Show_Boot_Menu = tmp_vm->Get_Show_Boot_Menu();

	// Audio Cards
	if( tmp_vm->Get_Audio_Cards().Audio_sb16 ) ui.CH_sb16->setChecked( true );
	else ui.CH_sb16->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_es1370 ) ui.CH_es1370->setChecked( true );
	else ui.CH_es1370->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_Adlib ) ui.CH_Adlib->setChecked( true );
	else ui.CH_Adlib->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_PC_Speaker ) ui.CH_PCSPK->setChecked( true );
	else ui.CH_PCSPK->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_GUS ) ui.CH_GUS->setChecked( true );
	else ui.CH_GUS->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_AC97 ) ui.CH_AC97->setChecked( true );
	else ui.CH_AC97->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_HDA ) ui.CH_HDA->setChecked( true );
	else ui.CH_HDA->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_cs4231a ) ui.CH_cs4231a->setChecked( true );
	else ui.CH_cs4231a->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_VirtIO ) ui.CH_VirtIO_Sound->setChecked( true );
	else ui.CH_VirtIO_Sound->setChecked( false );

	if( tmp_vm->Get_Audio_Cards().Audio_USB ) ui.CH_USB_Audio->setChecked( true );
	else ui.CH_USB_Audio->setChecked( false );

	// Disk bus (HDA)
	{
		int disk_idx = 0; // VirtIO default
		if( tmp_vm->Get_HDA().Get_Enabled() && tmp_vm->Get_HDA().Get_Native_Mode() )
		{
			switch( tmp_vm->Get_HDA().Get_Native_Device().Get_Interface() )
			{
				case VM::DI_Virtio: disk_idx = 0; break;
				case VM::DI_Virtio_SCSI: disk_idx = 1; break;
				case VM::DI_SCSI: disk_idx = 2; break;
				case VM::DI_IDE: disk_idx = 3; break;
				case VM::DI_SD: disk_idx = 4; break;
				default: disk_idx = 0; break;
			}
		}
		ui.CB_Disk_Interface->setCurrentIndex( disk_idx );
	}

	// RAM
	if( tmp_vm->Get_Memory_Size() < 1 )
	{
		AQGraphic_Warning( tr("Error!"),
						   tr("Memory size < 1! Using default value: 256 MB") );
		ui.Memory_Size->setValue( 256 );
	}
	else if( tmp_vm->Get_Memory_Size() >= ui.Memory_Size->maximum() )
	{
		AQGraphic_Warning( tr("Error!"),
						   tr("Memory size > all free memory on this system!") );
		ui.Memory_Size->setValue( ui.Memory_Size->maximum() );
	}
	else ui.Memory_Size->setValue( tmp_vm->Get_Memory_Size() );

	ui.CH_Remove_RAM_Size_Limitation->setChecked( tmp_vm->Get_Remove_RAM_Size_Limitation() );
	on_CH_Remove_RAM_Size_Limitation_stateChanged( ui.CH_Remove_RAM_Size_Limitation->checkState() );

	// General Tab. Options
	ui.CH_Fullscreen->setChecked( tmp_vm->Use_Fullscreen_Mode() );
	ui_ao.CH_ACPI->setChecked( tmp_vm->Use_ACPI() );
	ui.CH_Snapshot->setChecked( tmp_vm->Use_Snapshot_Mode() );
	ui_ao.CH_FDD_Boot->setChecked( tmp_vm->Use_Check_FDD_Boot_Sector() );
	ui.CH_Local_Time->setChecked( tmp_vm->Use_Local_Time() );
	ui_ao.CH_Win2K_Hack->setChecked( tmp_vm->Use_Win2K_Hack() );


    Dev_Manager->Set_VM( *tmp_vm ); // FIXME Use pointer

    // Shared Folders

	Folder_Sharing->Set_VM( *tmp_vm ); // FIXME Use pointer

	// Network tab. Redirections

	// Remove all rows...
	while( ui.Redirections_List->rowCount() > 0 ) ui.Redirections_List->removeRow( 0 );

	// Add values
	for( int rx = 0; rx < tmp_vm->Get_Network_Redirections_Count(); rx++ )
	{
		ui.Redirections_List->insertRow( ui.Redirections_List->rowCount() );

		// protocol
		QTableWidgetItem *newItem = new QTableWidgetItem( tmp_vm->Get_Network_Redirection(rx).Get_Protocol() );
		ui.Redirections_List->setItem( ui.Redirections_List->rowCount()-1, 0, newItem );

		// host port
		newItem = new QTableWidgetItem( QString::number(tmp_vm->Get_Network_Redirection(rx).Get_Host_Port()) );
		ui.Redirections_List->setItem( ui.Redirections_List->rowCount()-1, 1, newItem );

		// ip
		newItem = new QTableWidgetItem( tmp_vm->Get_Network_Redirection(rx).Get_Guest_IP() );
		ui.Redirections_List->setItem( ui.Redirections_List->rowCount()-1, 2, newItem );

		// guest port
		newItem = new QTableWidgetItem( QString::number(tmp_vm->Get_Network_Redirection(rx).Get_Guest_Port()) );
		ui.Redirections_List->setItem( ui.Redirections_List->rowCount()-1, 3, newItem );

		// set focus to new row
		ui.Redirections_List->setCurrentCell( ui.Redirections_List->rowCount()-1 , 0 );
	}

	Old_Network_Settings_Widget->Set_Network_Card_Models( curComp.Network_Card_List );
	Old_Network_Settings_Widget->Set_Network_Cards( tmp_vm->Get_Network_Cards() );

	New_Network_Settings_Widget->Set_Network_Card_Models( curComp.Network_Card_List );
	New_Network_Settings_Widget->Set_Network_Cards( tmp_vm->Get_Network_Cards_Nativ() );

	// Use Nativ Network Cards
	ui.RB_Network_Mode_New->setChecked( tmp_vm->Use_Native_Network() );
	ui.RB_Network_Mode_Old->setChecked( ! tmp_vm->Use_Native_Network() );
	on_RB_Network_Mode_New_toggled( ui.RB_Network_Mode_New->isChecked() );

	ui.Edit_TFTP_Prefix->setText( tmp_vm->Get_TFTP_Prefix() );
	ui.Edit_SMB_Folder->setText( tmp_vm->Get_SMB_Directory() );

	ui.CH_Redirections->setChecked( ! tmp_vm->Get_Use_Redirections() );
	ui.CH_Redirections->setChecked( tmp_vm->Get_Use_Redirections() );
	ui.CH_Use_Network->setChecked( ! tmp_vm->Get_Use_Network() );
	ui.CH_Use_Network->setChecked( tmp_vm->Get_Use_Network() );

	// Ports Tab
	Ports_Tab->Clear_Old_Ports();
	Ports_Tab->Set_Serial_Ports( tmp_vm->Get_Serial_Ports() );
	Ports_Tab->Set_Parallel_Ports( tmp_vm->Get_Parallel_Ports() );
	Ports_Tab->Set_USB_Ports( tmp_vm->Get_USB_Ports() );

	// Additional Options
	ui_ao.CH_RTC_TD_Hack->setChecked( tmp_vm->Use_RTC_TD_Hack() );
	ui_ao.CH_No_Shutdown->setChecked( tmp_vm->Use_No_Shutdown() );
	ui_ao.CH_No_Reboot->setChecked( tmp_vm->Use_No_Reboot() );
	ui_ao.CH_Start_CPU->setChecked( tmp_vm->Use_Start_CPU() );

	// Start Date
	ui_ao.CH_Start_Date->setChecked( tmp_vm->Use_Start_Date() );
	ui_ao.DTE_Start_Date->setDateTime( tmp_vm->Get_Start_Date() );

	// Additional Arguments
	ui_ao.Edit_Additional_Args->setPlainText( tmp_vm->Get_Additional_Args() );

	// Only_User_Args
	ui_ao.CH_Only_User_Args->setChecked( tmp_vm->Get_Only_User_Args() );

	// Use_User_Emulator_Binary
	ui_ao.CH_Use_User_Binary->setChecked( tmp_vm->Get_Use_User_Emulator_Binary() );

	// QEMU Window Option

	// Show QEMU Window Without a Frame and Window Decorations
	ui.CH_No_Frame->setChecked( tmp_vm->Use_No_Frame() );

	// Use Ctrl-Alt-Shift to Grab Mouse (Instead of Ctrl-Alt)
	ui.CH_Alt_Grab->setChecked( tmp_vm->Use_Alt_Grab() );

	// Disable SDL Window Close Capability
	ui.CH_No_Quit->setChecked( tmp_vm->Use_No_Quit() );

	// Rotate Graphical Output 90 Deg Left (Only PXA LCD)
	ui.CH_Portrait->setChecked( tmp_vm->Use_Portrait() );

	// Curses
	ui.CH_Curses->setChecked( tmp_vm->Use_Curses() );

	// Show_Cursor
	ui.CH_Show_Cursor->setChecked( tmp_vm->Use_Show_Cursor() );

	// Initial Graphical Mode
	ui.CH_Init_Graphic_Mode->setChecked( tmp_vm->Get_Init_Graphic_Mode().Get_Enabled() );
	ui.SB_InitGM_Width->setValue( tmp_vm->Get_Init_Graphic_Mode().Get_Width() );
	ui.SB_InitGM_Height->setValue( tmp_vm->Get_Init_Graphic_Mode().Get_Height() );

	switch( tmp_vm->Get_Init_Graphic_Mode().Get_Depth() )
	{
		case 8:
			ui.CB_InitGM_Depth->setCurrentIndex( 0 );
			break;

		case 16:
			ui.CB_InitGM_Depth->setCurrentIndex( 1 );
			break;

		case 24:
			ui.CB_InitGM_Depth->setCurrentIndex( 2 );
			break;

		case 32:
			ui.CB_InitGM_Depth->setCurrentIndex( 3 );
			break;

		default:
			if( tmp_vm->Get_Init_Graphic_Mode().Get_Depth() != 0 )
			{
				AQError( "void Main_Window::Update_VM_Ui()",
						 "Initial Graphical Mode: Default Section!" );
			}
			ui.CB_InitGM_Depth->setCurrentIndex( 2 );
			break;
	}

	// Other tab
	ui.CH_Use_Linux_Boot->setChecked( tmp_vm->Get_Use_Linux_Boot() );
	ui.Edit_Linux_bzImage_Path->setText( tmp_vm->Get_bzImage_Path() );
	ui.Edit_Linux_Initrd_Path->setText( tmp_vm->Get_Initrd_Path() );
	ui.Edit_Linux_Command_Line->setText( tmp_vm->Get_Kernel_ComLine() );

	// ROM File
	ui.CH_ROM_File->setChecked( tmp_vm->Get_Use_ROM_File() );
	ui.Edit_ROM_File->setText( tmp_vm->Get_ROM_File() );

	// On-Board Flash Image
	ui.CH_MTDBlock->setChecked( tmp_vm->Use_MTDBlock_File() );
	ui.Edit_MTDBlock_File->setText( tmp_vm->Get_MTDBlock_File() );

	// SecureDigital Card Image
	ui.CH_SD_Image->setChecked( tmp_vm->Use_SecureDigital_File() );
	ui.Edit_SD_Image_File->setText( tmp_vm->Get_SecureDigital_File() );

	// Parallel Flash Image
	ui.CH_PFlash->setChecked( tmp_vm->Use_PFlash_File() );
	ui.Edit_PFlash_File->setText( tmp_vm->Get_PFlash_File() );

	/*// Disable KVM kernel mode PIC/IOAPIC/LAPIC
	ui_kvm.CH_No_KVM_IRQChip->setChecked( tmp_vm->Use_KVM_IRQChip() );

	// Disable KVM kernel mode PIT
	ui_kvm.CH_No_KVM_Pit->setChecked( tmp_vm->Use_No_KVM_Pit() );

	// KVM_No_Pit_Reinjection
	ui_kvm.CH_KVM_No_Pit_Reinjection->setChecked( tmp_vm->Use_KVM_No_Pit_Reinjection() );

	// KVM_Nesting
	ui_kvm.CH_KVM_Nesting->setChecked( tmp_vm->Use_KVM_Nesting() );*/ //FIXME: deprecated //alternatives?

	// KVM Shadow Memory
	ui_kvm.CH_KVM_Shadow_Memory->setChecked( tmp_vm->Use_KVM_Shadow_Memory() );
	ui_kvm.SB_KVM_Shadow_Memory_Size->setValue( tmp_vm->Get_KVM_Shadow_Memory_Size() );

	// SPICE
	SPICE_Widget->Set_Settings( tmp_vm->Get_SPICE() );

	// VNC
	ui.CH_Activate_VNC->setChecked( tmp_vm->Use_VNC() );

	// Use Unix Socket Mode for VNC
	ui.RB_VNC_Unix_Socket->setChecked( tmp_vm->Get_VNC_Socket_Mode() );

	// UNIX Domain Socket Path
	ui.Edit_VNC_Unix_Socket->setText( tmp_vm->Get_VNC_Unix_Socket_Path() );

	// VNC Display Number
	ui.SB_VNC_Display->setValue( tmp_vm->Get_VNC_Display_Number() );

	// Use Password for VNC
	ui.CH_VNC_Password->setChecked( tmp_vm->Use_VNC_Password() );

	// Use TLS
	ui.CH_Use_VNC_TLS->setChecked( tmp_vm->Use_VNC_TLS() );

	// Use x509
	ui.CH_x509_Folder->setChecked( tmp_vm->Use_VNC_x509() );

	// x509 Folder
	ui.Edit_x509_Folder->setText( tmp_vm->Get_VNC_x509_Folder_Path() );

	// Use x509verify
	ui.CH_x509verify_Folder->setChecked( tmp_vm->Use_VNC_x509verify() );

	// x509 Folder
	ui.Edit_x509verify_Folder->setText( tmp_vm->Get_VNC_x509verify_Folder_Path() );

    if ( update_info_tab )
    {
    	Update_Info_Text();
    }
	Update_Win11_Lifecycle_Ui();
	Update_Disabled_Controls(); // FIXME

	// CPU count AFTER Update_Disabled_Controls — that rebuilds the combo and used to wipe this to 1.
	ui.CB_CPU_Count->setEditText( QString::number( tmp_vm->Get_SMP_CPU_Count() ) );
	SMP_Settings->Set_Values( tmp_vm->Get_SMP(), curComp.PSO_SMP_Count, curComp.PSO_SMP_Cores,
							  curComp.PSO_SMP_Threads, curComp.PSO_SMP_Sockets, curComp.PSO_SMP_MaxCPUs );

    /* TODO: POST 0.9.1
    QString info_text = tr("Machine:") + " " + tmp_vm->Get_Machine_Name();
    info_text += " " + tr("State:") + " " + tmp_vm->Get_State_Text();

    ui.Label_Machine_Info->setText(info_text);
    */

	// For VM Changes Signals
	ui.Button_Apply->setEnabled( false );
    ui.Button_Cancel->setEnabled( false );
}

void Main_Window::Update_VM_Port_Number()
{
	for( int ix = 0; ix < VM_List.count(); ++ix )
	{
		VM_List[ ix ]->Set_Embedded_Display_Port( ix );
	}
}

void Main_Window::Update_Info_Text( int info_mode )
{
	Virtual_Machine *tmp_vm = Get_Current_VM();

	if( tmp_vm == NULL && info_mode == 0 )
	{
		AQError( "void Main_Window::Update_Info_Text( int info_mode )",
				 "Cannot Find VM!" );
		return;
	}

    ui.VM_Information_Text->setHtml(tmp_vm->GenerateHTMLInfoText(info_mode));
}

void Main_Window::Update_Disabled_Controls()
{
	// Get devices
	bool curMachineOk = false;
	Available_Devices curComp = Get_Current_Machine_Devices( &curMachineOk );
	if( ! curMachineOk ) return;

	// Apply emulator

	// CPU — preserve the user's/current value across rebuild (clear() resets to "1").
	disconnect( ui.CB_CPU_Count, SIGNAL(editTextChanged(const QString &)),
				this, SLOT(Validate_CPU_Count(const QString&)) );
	disconnect( ui.CB_CPU_Count, SIGNAL(editTextChanged(const QString &)),
				this, SLOT(VM_Changed()) );

	const QString keep_cpu = ui.CB_CPU_Count->currentText().trimmed();

	ui.CB_CPU_Count->clear();

	if( curComp.PSO_SMP_Count == 1 )
	{
		ui.CB_CPU_Count->addItem( QString::number(1) );
		ui.CB_CPU_Count->setEnabled( false );
		ui.TB_Show_SMP_Settings_Window->setEnabled( false );
	}
	else
	{
		for( int cx = 1; (cx -1) <= curComp.PSO_SMP_Count; cx *= 2 )
		{
			if( cx == 256 ) ui.CB_CPU_Count->addItem( QString::number(255) );
			else ui.CB_CPU_Count->addItem( QString::number(cx) );
		}

		ui.CB_CPU_Count->setEnabled( true );
		ui.TB_Show_SMP_Settings_Window->setEnabled( true );
	}

	if( ! keep_cpu.isEmpty() )
		ui.CB_CPU_Count->setEditText( keep_cpu );
	else if( ui.CB_CPU_Count->count() > 0 )
		ui.CB_CPU_Count->setCurrentIndex( 0 );

	connect( ui.CB_CPU_Count, SIGNAL(editTextChanged(const QString &)),
			 this, SLOT(Validate_CPU_Count(const QString&)) );
	connect( ui.CB_CPU_Count, SIGNAL(editTextChanged(const QString &)),
			 this, SLOT(VM_Changed()) );
	/*
	SMP_Settings

	if( PSO_SMP_Count ) // FIXME
	else
	if( curComp.PSO_SMP_Cores )
	else
	if( curComp.PSO_SMP_Threads )
	else
	if( curComp.PSO_SMP_Sockets )
	else
	if( curComp.PSO_SMP_MaxCPUs )
	else */

	// Drive
	/*if( curComp.PSO_Drive )
	else
	if( curComp.PSO_Drive_File )
	else
	if( curComp.PSO_Drive_If )
	else
	if( curComp.PSO_Drive_Bus_Unit )
	else
	if( curComp.PSO_Drive_Index )
	else
	if( curComp.PSO_Drive_Media )
	else
	if( curComp.PSO_Drive_Cyls_Heads_Secs_Trans )
	else
	if( curComp.PSO_Drive_Snapshot )
	else
	if( curComp.PSO_Drive_Cache )
	else
	if( curComp.PSO_Drive_AIO )
	else
	if( curComp.PSO_Drive_Format )
	else
	if( curComp.PSO_Drive_Serial )
	else
	if( curComp.PSO_Drive_ADDR )
	else
	if( curComp.PSO_Drive_Boot )
	else
	*/

	// Options
	if( curComp.PSO_Boot_Order ) ui.TB_Show_Boot_Settings_Window->setEnabled( true );
	else ui.TB_Show_Boot_Settings_Window->setEnabled( false );

	if( curComp.PSO_Initial_Graphic_Mode ) ui.CH_Init_Graphic_Mode->setEnabled( true );
	else ui.CH_Init_Graphic_Mode->setEnabled( false );

	if( curComp.PSO_No_FB_Boot_Check ) ui_ao.CH_FDD_Boot->setEnabled( true );
	else ui_ao.CH_FDD_Boot->setEnabled( false );

	if( curComp.PSO_Win2K_Hack ) ui_ao.CH_Win2K_Hack->setEnabled( true );
	else ui_ao.CH_Win2K_Hack->setEnabled( false );

	if( curComp.PSO_No_ACPI ) ui_ao.CH_ACPI->setEnabled( true );
	else ui_ao.CH_ACPI->setEnabled( false );

	if( curComp.PSO_RTC_TD_Hack ) ui_ao.CH_RTC_TD_Hack->setEnabled( true );
	else ui_ao.CH_RTC_TD_Hack->setEnabled( false );

	if( curComp.PSO_MTDBlock ) ui.CH_MTDBlock->setEnabled( true );
	else ui.CH_MTDBlock->setEnabled( false );

	if( curComp.PSO_SD ) ui.CH_SD_Image->setEnabled( true );
	else ui.CH_SD_Image->setEnabled( false );

	if( curComp.PSO_PFlash ) ui.CH_PFlash->setEnabled( true );
	else ui.CH_PFlash->setEnabled( false );

	//if( curComp.PSO_Name )
	//else

	if( curComp.PSO_Curses ) ui.CH_Curses->setEnabled( true );
	else ui.CH_Curses->setEnabled( false );

	if( curComp.PSO_No_Frame ) ui.CH_No_Frame->setEnabled( true );
	else ui.CH_No_Frame->setEnabled( false );

	if( curComp.PSO_Alt_Grab ) ui.CH_Alt_Grab->setEnabled( true );
	else ui.CH_Alt_Grab->setEnabled( false );

	if( curComp.PSO_No_Quit ) ui.CH_No_Quit->setEnabled( true );
	else ui.CH_No_Quit->setEnabled( false );

	//if( curComp.PSO_SDL )
	//else

	if( curComp.PSO_Portrait ) ui.CH_Portrait->setEnabled( true );
	else ui.CH_Portrait->setEnabled( false );

	if( curComp.PSO_No_Shutdown ) ui_ao.CH_No_Shutdown->setEnabled( true );
	else ui_ao.CH_No_Shutdown->setEnabled( false );

	if( curComp.PSO_Startdate )
	{
		ui_ao.CH_Start_Date->setEnabled( true );
		ui_ao.DTE_Start_Date->setEnabled( true );
	}
	else
	{
		ui_ao.CH_Start_Date->setEnabled( false );
		ui_ao.DTE_Start_Date->setEnabled( false );
	}

	if( curComp.PSO_Show_Cursor ) ui.CH_Show_Cursor->setEnabled( true );
	else ui.CH_Show_Cursor->setEnabled( false );

	//if( curComp.PSO_Bootp )
	//else

	New_Network_Settings_Widget->Set_Devices( curComp );
	// Nativ mode network
	if( ui.RB_Network_Mode_New->isChecked() )
	{
		// FIXME
	}

	//if( curComp.PSO_No_KVM ) ui.CH_No_KVM->setEnabled( true );
	//else ui.CH_No_KVM->setEnabled( false );

	/*if( curComp.PSO_No_KVM_IRQChip ) ui_kvm.CH_No_KVM_IRQChip->setEnabled( true ); //FIXME: deprecated //alternatives?
	else ui_kvm.CH_No_KVM_IRQChip->setEnabled( false );

	if( curComp.PSO_No_KVM_Pit ) ui_kvm.CH_No_KVM_Pit->setEnabled( true );
	else ui_kvm.CH_No_KVM_Pit->setEnabled( false );

	if( curComp.PSO_No_KVM_Pit_Reinjection ) ui_kvm.CH_KVM_No_Pit_Reinjection->setEnabled( true );
	else ui_kvm.CH_KVM_No_Pit_Reinjection->setEnabled( false );

	if( curComp.PSO_Enable_Nesting ) ui_kvm.CH_KVM_Nesting->setEnabled( true );
	else ui_kvm.CH_KVM_Nesting->setEnabled( false );*/

	if( curComp.PSO_KVM_Shadow_Memory )
	{
		ui_kvm.CH_KVM_Shadow_Memory->setEnabled( true );
		ui_kvm.SB_KVM_Shadow_Memory_Size->setEnabled( true );
		ui_kvm.Label_KVM_Shadow_Memory_Mb->setEnabled( true );
	}
	else
	{
		ui_kvm.CH_KVM_Shadow_Memory->setEnabled( false );
		ui_kvm.SB_KVM_Shadow_Memory_Size->setEnabled( false );
		ui_kvm.Label_KVM_Shadow_Memory_Mb->setEnabled( false );
	}

	// SPICE
	SPICE_Widget->setEnabled( curComp.PSO_SPICE );
	SPICE_Widget->Set_PSO_GXL( curComp.PSO_QXL );

	// Obsolete QEMU options
	if( curComp.PSO_TFTP )
	{
		ui.Label_TFTP->setEnabled( true );
		ui.Edit_TFTP_Prefix->setEnabled( true );
		ui.TB_Browse_TFTP->setEnabled( true );
	}
	else
	{
		ui.Label_TFTP->setEnabled( false );
		ui.Edit_TFTP_Prefix->setEnabled( false );
		ui.TB_Browse_TFTP->setEnabled( false );
	}

	if( curComp.PSO_SMB )
	{
		ui.Label_SMB_Folder->setEnabled( true );
		ui.TB_Browse_SMB->setEnabled( true );
		ui.Edit_SMB_Folder->setEnabled( true );
	}
	else
	{
		ui.Label_SMB_Folder->setEnabled( false );
		ui.TB_Browse_SMB->setEnabled( false );
		ui.Edit_SMB_Folder->setEnabled( false );
	}

	//if( curComp.PSO_Std_VGA )
	//else
}

void Main_Window::Update_Recent_CD_ROM_Images_List()
{
	// CD-ROM
	QStringList cd_list = System_Info::Get_Host_CDROM_List();
	cd_list += Get_CD_Recent_Images_List();
}

void Main_Window::Update_Recent_Floppy_Images_List()
{
	// Floppy
	QStringList fd_list = System_Info::Get_Host_FDD_List();
	fd_list += Get_FDD_Recent_Images_List();
}

QString Main_Window::Get_Storage_Device_Info_String( const QString &path )
{
	if( path.isEmpty() ) return tr( "Type: none     Size: 0" );

	if( ! QFile::exists(path) )
	{
		AQWarning( "QString Main_Window::Get_Storage_Device_Info_String( const QString &path )",
				   "File \"" + path + "\" not exists!" );
		return tr( "Type: none     Size: 0" );
	}

	QFileInfo file = QFileInfo( path );
	qint64 size_in_bytes = file.size();

	if( It_Host_Device(path) ) return tr( "Type: Host Device" );
	if( file.isFile() == false && file.isSymLink() == false ) return tr( "Type: none     Size: 0" );
	if( size_in_bytes <= 0 ) return tr( "Type: Image     Size: 0" );

	QString suf = "";
	float size = 0;

	if( (size_in_bytes / 1024.0) < 1 )
	{
		suf = tr( "Byte" );
		size = (float) size_in_bytes;
	}
	else if( (size_in_bytes / 1024.0 / 1024.0) < 1 )
	{
		suf = tr( "Kb" );
		size = (float) size_in_bytes / 1024.0;
	}
	else if( (size_in_bytes / 1024.0 / 1024.0 / 1024.0) < 1 )
	{
		suf = tr( "Mb" );
		size = (float) size_in_bytes / 1024.0 / 1024.0;
	}
	else
	{
		suf = tr( "Gb" );
		size = (float) size_in_bytes / 1024.0/ 1024.0 / 1024.0;
	}

	return tr("Type: Image     Size: ") + QString::number(size, 'f', 2) + suf;
}

void Main_Window::VM_State_Changed( Virtual_Machine *vm, VM::VM_State s )
{
	if( vm == NULL )
	{
		AQError( "void Main_Window::VM_State_Changed( Virtual_Machine *vm, VM::VM_State s )",
				 "Error: vm == NULL" );
		return;
	}

	Virtual_Machine *cur_vm = Get_Current_VM();
	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::VM_State_Changed( Virtual_Machine *vm, VM::VM_State s )",
				 "Cannot Get Current VM" );
		return;
	}

	// This is current VM?
	if( *vm == *cur_vm )
	{
		Update_Info_Text();
		Show_State_Current( cur_vm );
	}
    Show_State_VM( vm );

	vm->Save_VM(); // Save New State

	// Embedded session: enter guest view when VM starts
	if( Settings.value( "Embedded_Session", "yes" ).toString() == "yes" )
	{
		if( s == VM::VMS_Running || s == VM::VMS_Pause )
		{
			if( ! Session_Mode_Active || Session_VM == vm )
				Enter_Session_Mode( vm );
		}
		else if( Session_Mode_Active && Session_VM == vm &&
		         ( s == VM::VMS_Power_Off || s == VM::VMS_Saved ) )
		{
			Exit_Session_Mode();
		}
	}
}

void Main_Window::Enter_Session_Mode( Virtual_Machine *vm )
{
	if( ! vm || ! Session_Widget || ! Main_Stack )
		return;

	Session_VM = vm;
	Session_Mode_Active = true;
	setWindowTitle( tr( "AQEMU – %1" ).arg( vm->Get_Machine_Name() ) );

	// Session view owns all runtime controls on its top toolbar — hide left chrome
	ui.Tool_Bar_VM_Manage->setVisible( false );
	ui.Tool_Bar_VM_Control->setVisible( false );

	const int vnc_tcp = vm->Get_Embedded_VNC_Port() > 0
		? vm->Get_Embedded_VNC_Port()
		: ( vm->Get_Embedded_Display_Port() + Settings.value( "First_VNC_Port", "5910" ).toString().toInt() );
	const QString backend = Settings.value( "Embedded_Display_Backend", "spice" ).toString();

	Session_Widget->Attach_VM( vm, vm->Get_QMP(),
	                           QStringLiteral( "127.0.0.1" ),
	                           vm->Get_Embedded_Spice_Port(),
	                           vnc_tcp,
	                           backend );
	Main_Stack->setCurrentWidget( Session_Widget );
}

void Main_Window::Enter_Session_Mode_Preparing( Virtual_Machine *vm )
{
	if( ! vm || ! Session_Widget || ! Main_Stack )
		return;

	Session_VM = vm;
	Session_Mode_Active = true;
	setWindowTitle( tr( "AQEMU – %1" ).arg( vm->Get_Machine_Name() ) );

	ui.Tool_Bar_VM_Manage->setVisible( false );
	ui.Tool_Bar_VM_Control->setVisible( false );

	const QString backend = Settings.value( "Embedded_Display_Backend", "spice" ).toString();
	// Ports are allocated during Start — show the session shell first so QEMU
	// does not race ahead of the UI.
	Session_Widget->Attach_VM( vm, nullptr,
	                           QStringLiteral( "127.0.0.1" ),
	                           0, 0, backend );
	Main_Stack->setCurrentWidget( Session_Widget );
	QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
}

void Main_Window::Exit_Session_Mode()
{
	if( ! Session_Mode_Active )
		return;

	if( Session_Widget )
		Session_Widget->Detach();

	Session_Mode_Active = false;
	Session_VM = nullptr;
	if( Main_Stack )
		Main_Stack->setCurrentIndex( 0 );

	ui.Tool_Bar_VM_Manage->setVisible( true );
	ui.Tool_Bar_VM_Control->setVisible( true );

	setWindowTitle( Idle_Window_Title.isEmpty() ? tr( "AQEMU" ) : Idle_Window_Title );

	// Refresh device tabs so floppy/CD paths saved during the session show up
	Update_VM_Ui();
}

void Main_Window::On_Session_Exit_View()
{
	// Leave session UI without forcing power-off
	Exit_Session_Mode();
}

void Main_Window::On_Session_Request_Stop()
{
	Virtual_Machine *vm = Session_VM;
	// Leave the session UI immediately so Stop/kill cannot thrash the display.
	if( Session_Mode_Active )
		Exit_Session_Mode();
	if( vm )
		AQEMU_Service::get().call( "stop", vm );
}

void Main_Window::On_Session_Request_Shutdown()
{
	if( Session_VM )
		AQEMU_Service::get().call( "shutdown", Session_VM );
}

void Main_Window::On_Session_Request_Reset()
{
	if( Session_VM )
		AQEMU_Service::get().call( "reset", Session_VM );
}

void Main_Window::On_Session_Request_Pause()
{
	if( Session_VM )
		AQEMU_Service::get().call( "pause", Session_VM );
}

void Main_Window::On_Session_Request_Save()
{
	if( Session_VM )
		AQEMU_Service::get().call( "save", Session_VM );
}

void Main_Window::Change_The_Icon(Virtual_Machine* vm, QString _icon)
{
    //find QListWidgetItem in ui.Machines_List matching the vm
    QString name = vm->Get_Machine_Name();
    int i = 0;
    QListWidgetItem* vm_item = nullptr;
    for ( QListWidgetItem* item = ui.Machines_List->item(0); item != 0;  item = ui.Machines_List->item(0+i))
    {
        if ( item->text() == name )
            vm_item = item;

        i++;
    }

    if ( vm_item == nullptr )
    {
        AQDebug("void Main_Window::Change_The_Icon(Virtual_Machine* vm, QString _icon)", "Matching icon not found");
        return;
    }

    if ( _icon.contains("stop") )
    {
        vm_item->setIcon(QIcon(vm->Get_Icon_Path()));
        return;
    }

    int s = Settings.value("VM_Icons_Size", "48").toInt();

    QIcon icon = QIcon(vm->Get_Icon_Path());
    auto pix = new QPixmap(icon.pixmap(QSize(s,s)));

    auto painter = new QPainter(pix);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPixmap pix2(QIcon(_icon).pixmap(QSize(s,s)));

    QRect rect(s/2,s/2,s/2,s/2);

    painter->drawPixmap(rect,pix2);

    QIcon icon2(*pix);

    vm_item->setIcon(icon2);

    delete painter;
    delete pix;
}

void Main_Window::setStateActionsEnabled(bool enabled)
{
	ui.actionPower_On->setEnabled( enabled );
	ui.actionSave->setEnabled( enabled );
	ui.actionPause->setEnabled( enabled );
	ui.actionPower_Off->setEnabled( enabled );
    ui.actionShutdown->setEnabled( enabled );
	ui.actionReset->setEnabled( enabled );
}

void Main_Window::Show_State_Current( Virtual_Machine *vm)
{
	if( vm == NULL )
	{
		AQError( "void Main_Window::Show_State( Virtual_Machine *vm, VM::VM_State s )",
				 "vm == NULL" );
		return;
	}

	if( vm->Get_State() == VM::VMS_Saved && Settings.value("Use_Screenshot_for_OS_Logo", "yes").toString() == "yes" )
	{
		ui.Machines_List->currentItem()->setIcon( QIcon(vm->Get_Screenshot_Path()) );
		ui.Machines_List->currentItem()->setData( 128, vm->Get_Screenshot_Path() );
	}
	else
	{
		ui.Machines_List->currentItem()->setIcon( QIcon(vm->Get_Icon_Path()) );
		ui.Machines_List->currentItem()->setData( 128, vm->Get_Icon_Path() );
	}

	switch( vm->Get_State() )
	{
		case VM::VMS_Running:
		    setStateActionsEnabled( true );
			ui.actionPower_On->setEnabled( false );
			ui.actionPause->setChecked( false );

			Set_Widgets_State( false );
			break;

		case VM::VMS_Power_Off:
		    setStateActionsEnabled( false );
			ui.actionPower_On->setEnabled( true );
			ui.actionPause->setChecked( false );

			Set_Widgets_State( true );
			break;

		case VM::VMS_Pause:
		    setStateActionsEnabled( true );
			ui.actionPower_On->setEnabled( false );
			ui.actionPause->setChecked( true );

			Set_Widgets_State( false );
			break;

		case VM::VMS_Saved:
			ui.actionPower_On->setEnabled( true );
			ui.actionSave->setEnabled( false );
			ui.actionPause->setEnabled( false );
			ui.actionPause->setChecked( false );
			ui.actionPower_Off->setEnabled( true );
            ui.actionShutdown->setEnabled( true );
			ui.actionReset->setEnabled( true );

			Set_Widgets_State( false );
			break;

		case VM::VMS_In_Error:
		    setStateActionsEnabled( false );
			ui.actionPause->setChecked( false );
			Set_Widgets_State( false );

			Update_Info_Text( 2 );
			break;

		default:
			break;
	}

	ui.Button_Apply->setEnabled( false );
	ui.Button_Cancel->setEnabled( false );

	Update_Emulator_Control( vm );
}

void Main_Window::Show_State_VM( Virtual_Machine *vm )
{
	if( vm == NULL )
	{
		AQError( "void Main_Window::Show_State_VM( Virtual_Machine *vm )",
				 "vm == NULL" );
		return;
	}

	switch( vm->Get_State() )
	{
		case VM::VMS_Running:
            Change_The_Icon(vm, ":/play.png");
			break;

		case VM::VMS_Power_Off:
            Change_The_Icon(vm, ":/stop.png");
			break;

		case VM::VMS_Pause:
            Change_The_Icon(vm, ":/pause.png");
			break;

		case VM::VMS_Saved:
            Change_The_Icon(vm, ":/save.png");
			break;

		case VM::VMS_In_Error:
            Change_The_Icon(vm, ":/error.png");
			break;

		default:
			break;
	}

	Update_Emulator_Control( vm );
}

void Main_Window::Set_Widgets_State( bool enabled )
{
    QList<QWidget*> list;

	// Tabs
	ui.Tab_General->setEnabled( enabled );
	//ui.Tab_HDD->setEnabled( enabled );
	//ui.Tab_Removable_Disks->setEnabled( enabled );

    // Media
	Dev_Manager->Set_Enabled( enabled );
	Folder_Sharing->Set_Enabled( enabled );
	Ports_Tab->setEnabled( enabled );
	ui.Tab_Optional_Images->setEnabled( enabled );
	ui.Tab_Boot_Linux->setEnabled( enabled );

	// Tab network
	ui.Widget_Use_Network->setEnabled( enabled );
	Old_Network_Settings_Widget->Set_Enabled( enabled );
	New_Network_Settings_Widget->Set_Enabled( enabled );

    // Network redirections
    list.clear(); list << ui.Redirection_Widget << ui.Widget_Redirection_Buttons;
	Checkbox_Dependend_Set_Enabled( list, ui.CH_Redirections, enabled );

    // Tab Display
    list.clear(); list << ui.VNC_General << ui.VNC_Security;
	Checkbox_Dependend_Set_Enabled( list, ui.CH_Activate_VNC, enabled );

	SPICE_Widget->My_Set_Enabled( enabled );
	ui.Tab_Emulator_Window_Options->setEnabled( enabled );

	// Children previously setEnabled(false) stay disabled when the parent is
	// re-enabled — refresh resolution enablement explicitly.
	Update_Display_Resolution_Enabled();
}

void Main_Window::Fill_Display_Resolution_Combo()
{
	ui.CB_Display_Resolution->blockSignals( true );
	ui.CB_Display_Resolution->clear();
	ui.CB_Display_Resolution->addItem( tr( "Native (host screen)" ), QStringLiteral( "native" ) );
	ui.CB_Display_Resolution->addItem( tr( "Auto (guest / firmware)" ), QStringLiteral( "auto" ) );
	ui.CB_Display_Resolution->addItem( QStringLiteral( "1024 × 768" ), QStringLiteral( "1024x768" ) );
	ui.CB_Display_Resolution->addItem( QStringLiteral( "1280 × 720" ), QStringLiteral( "1280x720" ) );
	ui.CB_Display_Resolution->addItem( QStringLiteral( "1280 × 800" ), QStringLiteral( "1280x800" ) );
	ui.CB_Display_Resolution->addItem( QStringLiteral( "1366 × 768" ), QStringLiteral( "1366x768" ) );
	ui.CB_Display_Resolution->addItem( QStringLiteral( "1600 × 900" ), QStringLiteral( "1600x900" ) );
	ui.CB_Display_Resolution->addItem( QStringLiteral( "1920 × 1080" ), QStringLiteral( "1920x1080" ) );
	ui.CB_Display_Resolution->addItem( QStringLiteral( "2560 × 1440" ), QStringLiteral( "2560x1440" ) );
	ui.CB_Display_Resolution->setCurrentIndex( 0 );
	ui.CB_Display_Resolution->blockSignals( false );
}

void Main_Window::Apply_Display_Resolution_To_Ui( const QString &res )
{
	const QString want = res.trimmed().isEmpty() ? QStringLiteral( "native" ) : res.trimmed();
	ui.CB_Display_Resolution->blockSignals( true );
	int ix = ui.CB_Display_Resolution->findData( want );
	if( ix < 0 )
	{
		// Custom WxH from an older/hand-edited VM file
		ui.CB_Display_Resolution->addItem( want, want );
		ix = ui.CB_Display_Resolution->findData( want );
	}
	if( ix >= 0 )
		ui.CB_Display_Resolution->setCurrentIndex( ix );
	else
		ui.CB_Display_Resolution->setCurrentIndex( 0 );
	ui.CB_Display_Resolution->blockSignals( false );
}

void Main_Window::Fill_Mouse_Combos()
{
	ui.CB_Mouse_Pointer_Mode->blockSignals( true );
	ui.CB_Mouse_Pointer_Mode->clear();
	ui.CB_Mouse_Pointer_Mode->addItem(
		tr( "Seamless (absolute — no grab)" ), QStringLiteral( "seamless" ) );
	ui.CB_Mouse_Pointer_Mode->addItem(
		tr( "Relative (click to capture)" ), QStringLiteral( "relative" ) );
	ui.CB_Mouse_Pointer_Mode->setCurrentIndex( 0 );
	ui.CB_Mouse_Pointer_Mode->blockSignals( false );

	ui.CB_Mouse_Type->blockSignals( true );
	ui.CB_Mouse_Type->clear();
	ui.CB_Mouse_Type->addItem( tr( "PS/2 mouse (relative)" ), QStringLiteral( "ps2" ) );
	ui.CB_Mouse_Type->addItem( tr( "USB Tablet (seamless)" ), QStringLiteral( "usb-tablet" ) );
	ui.CB_Mouse_Type->addItem( tr( "USB Mouse (relative)" ), QStringLiteral( "usb-mouse" ) );
	ui.CB_Mouse_Type->addItem( tr( "USB Wacom Tablet (seamless)" ), QStringLiteral( "usb-wacom-tablet" ) );
	ui.CB_Mouse_Type->addItem( tr( "VirtIO Tablet (seamless)" ), QStringLiteral( "virtio-tablet-pci" ) );
	ui.CB_Mouse_Type->addItem( tr( "VirtIO Mouse (relative)" ), QStringLiteral( "virtio-mouse-pci" ) );
	ui.CB_Mouse_Type->addItem( tr( "VMware mouse (seamless)" ), QStringLiteral( "vmmouse" ) );
	ui.CB_Mouse_Type->setCurrentIndex( 0 );
	ui.CB_Mouse_Type->blockSignals( false );

	ui.CB_Mouse_USB_Controller->blockSignals( true );
	ui.CB_Mouse_USB_Controller->clear();
	ui.CB_Mouse_USB_Controller->addItem( tr( "Auto (UHCI on PC, xHCI on virt)" ), QStringLiteral( "auto" ) );
	ui.CB_Mouse_USB_Controller->addItem( tr( "UHCI (-usb) — typical for Windows 9x" ), QStringLiteral( "uhci" ) );
	ui.CB_Mouse_USB_Controller->addItem( tr( "xHCI (qemu-xhci) — modern / ARM virt" ), QStringLiteral( "xhci" ) );
	ui.CB_Mouse_USB_Controller->addItem( tr( "None (use existing USB bus only)" ), QStringLiteral( "none" ) );
	ui.CB_Mouse_USB_Controller->setCurrentIndex( 0 );
	ui.CB_Mouse_USB_Controller->blockSignals( false );

	ui.CB_Mouse_USB_Version->blockSignals( true );
	ui.CB_Mouse_USB_Version->clear();
	ui.CB_Mouse_USB_Version->addItem( tr( "Default" ), 0 );
	ui.CB_Mouse_USB_Version->addItem( tr( "USB 1.1" ), 1 );
	ui.CB_Mouse_USB_Version->addItem( tr( "USB 2.0" ), 2 );
	ui.CB_Mouse_USB_Version->setCurrentIndex( 0 );
	ui.CB_Mouse_USB_Version->blockSignals( false );

	ui.CB_SPICE_Agent_Mouse->blockSignals( true );
	ui.CB_SPICE_Agent_Mouse->clear();
	ui.CB_SPICE_Agent_Mouse->addItem( tr( "Default (QEMU)" ), QStringLiteral( "default" ) );
	ui.CB_SPICE_Agent_Mouse->addItem( tr( "On (agent-mouse=on)" ), QStringLiteral( "on" ) );
	ui.CB_SPICE_Agent_Mouse->addItem( tr( "Off (agent-mouse=off)" ), QStringLiteral( "off" ) );
	ui.CB_SPICE_Agent_Mouse->setCurrentIndex( 0 );
	ui.CB_SPICE_Agent_Mouse->blockSignals( false );
}

bool Main_Window::Mouse_Type_Is_Seamless( const QString &mouse_type )
{
	const QString mt = mouse_type.trimmed().toLower();
	return mt == QStringLiteral( "usb-tablet" )
	    || mt == QStringLiteral( "usb-wacom-tablet" )
	    || mt == QStringLiteral( "virtio-tablet-pci" )
	    || mt == QStringLiteral( "virtio-tablet" )
	    || mt == QStringLiteral( "vmmouse" )
	    || mt.contains( QStringLiteral( "tablet" ) );
}

void Main_Window::Sync_Mouse_Pointer_Mode_From_Type()
{
	const QString mt = ui.CB_Mouse_Type->currentData( Qt::UserRole ).toString();
	const QString mode = Mouse_Type_Is_Seamless( mt )
		? QStringLiteral( "seamless" )
		: QStringLiteral( "relative" );

	ui.CB_Mouse_Pointer_Mode->blockSignals( true );
	const int ix = ui.CB_Mouse_Pointer_Mode->findData( mode );
	if( ix >= 0 )
		ui.CB_Mouse_Pointer_Mode->setCurrentIndex( ix );
	ui.CB_Mouse_Pointer_Mode->blockSignals( false );
}

void Main_Window::On_Mouse_Pointer_Mode_Changed()
{
	const QString mode = ui.CB_Mouse_Pointer_Mode->currentData( Qt::UserRole ).toString();
	const QString mt = ui.CB_Mouse_Type->currentData( Qt::UserRole ).toString();
	const bool want_seamless = ( mode == QStringLiteral( "seamless" ) );
	const bool is_seamless = Mouse_Type_Is_Seamless( mt );

	if( want_seamless == is_seamless )
	{
		VM_Changed();
		return;
	}

	// Switch to a sensible default device for the chosen mode.
	const QString next = want_seamless
		? QStringLiteral( "usb-tablet" )
		: QStringLiteral( "ps2" );

	ui.CB_Mouse_Type->blockSignals( true );
	const int ix = ui.CB_Mouse_Type->findData( next );
	if( ix >= 0 )
		ui.CB_Mouse_Type->setCurrentIndex( ix );
	ui.CB_Mouse_Type->blockSignals( false );

	Update_Mouse_Options_Enabled();
	VM_Changed();
}

void Main_Window::Update_Mouse_Options_Enabled()
{
	const QString mt = ui.CB_Mouse_Type->currentData( Qt::UserRole ).toString();
	const bool usb = mt.startsWith( QStringLiteral( "usb-" ) );
	ui.CB_Mouse_USB_Controller->setEnabled( usb );
	ui.Label_Mouse_USB_Controller->setEnabled( usb );
	ui.CB_Mouse_USB_Version->setEnabled( mt == "usb-tablet" || mt == "usb-mouse" );
	ui.Label_Mouse_USB_Version->setEnabled( mt == "usb-tablet" || mt == "usb-mouse" );
}

void Main_Window::Apply_Mouse_Settings_To_Ui( const Virtual_Machine *vm )
{
	if( ! vm ) return;

	auto set_combo = []( QComboBox *cb, const QVariant &data )
	{
		if( ! cb ) return;
		cb->blockSignals( true );
		int ix = cb->findData( data );
		if( ix < 0 ) ix = 0;
		cb->setCurrentIndex( ix );
		cb->blockSignals( false );
	};

	set_combo( ui.CB_Mouse_Type, vm->Get_Mouse_Type() );
	set_combo( ui.CB_Mouse_USB_Controller, vm->Get_Mouse_USB_Controller() );
	set_combo( ui.CB_Mouse_USB_Version, vm->Get_Mouse_USB_Version() );
	set_combo( ui.CB_SPICE_Agent_Mouse, vm->Get_SPICE_Agent_Mouse() );
	Sync_Mouse_Pointer_Mode_From_Type();
}

void Main_Window::Update_Display_Resolution_Enabled()
{
	QString video = ui.CB_Video_Card->currentData( Qt::UserRole ).toString();
	if( video.isEmpty() )
		video = ui.CB_Video_Card->currentText();
	if( video.isEmpty() )
	{
		Virtual_Machine *vm = Get_Current_VM();
		if( vm )
			video = vm->Get_Video_Card();
	}

	const bool virtio = video.contains( QStringLiteral( "virtio-gpu" ), Qt::CaseInsensitive );
	// Respect parent tab enablement (disabled while VM is running).
	const bool parent_ok = ui.Tab_General->isEnabled();
	ui.CB_Display_Resolution->setEnabled( virtio && parent_ok );
	ui.Label_Display_Resolution->setEnabled( virtio && parent_ok );
}

void Main_Window::VM_Changed()
{
    if ( block_VM_changed_signals )
        return;

	Update_Display_Resolution_Enabled();
	Update_Mouse_Options_Enabled();

    // check if there's really a change compared
    // to the current VM /(and saved VM file)
    auto old_vm = Get_Current_VM();

    if ( old_vm != nullptr )
    {
        auto tmp_vm = new Virtual_Machine();

        Create_VM_From_Ui(tmp_vm,old_vm,false);

        bool test = ( *old_vm != *tmp_vm );

	    ui.Button_Apply->setEnabled( test );
	    ui.Button_Cancel->setEnabled( test );

        delete tmp_vm;

		// Persist immediately (debounced) — no "save changes?" prompts.
		if( test )
			Schedule_Auto_Save();
		else if( Auto_Save_Timer )
			Auto_Save_Timer->stop();
    }
}

void Main_Window::Schedule_Auto_Save()
{
	if( Auto_Save_Timer )
		Auto_Save_Timer->start();
}

// FIXME This will be rewritten in the future. Deleting and creating new tabs/layouts is not done optimally
void Main_Window::Update_Emulator_Control( Virtual_Machine *cur_vm )
{
	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::Update_Emulator_Control()",
				 "cur_vm == NULL" );
		return;
	}

	// VM Running?
	bool emulRun = (cur_vm->Get_State() == VM::VMS_Running || cur_vm->Get_State() == VM::VMS_Pause);

	if( Settings.value("Show_Emulator_Control_Window", "no").toString() == "yes" )
	{
		if( Settings.value("Include_Emulator_Control", "no").toString() == "yes" )
		{
			if( Settings.value("Use_VNC_Display", "no").toString() == "yes" )
			{
				// Add Display tab
				if( emulRun )
				{
					cur_vm->Emu_Ctl->Use_Minimal_Size( false );
					ui.Tabs->insertTab( 0, cur_vm->Emu_Ctl, tr("Display") );
					ui.Tabs->setCurrentIndex( 0 );
				}
				else
				{
					// Check and delete Emulator Control tab
					if( ui.Tabs->tabText(0) == tr("Display") )
					{
						ui.Tabs->removeTab( 0 );
						ui.Tabs->setCurrentIndex( 0 );
					}
				}
			}
			else // No VNC
			{
				// Hide all
				for( int vx = 0; vx < VM_List.count(); ++vx )
					VM_List[vx]->Hide_Emu_Ctl_Win();

				// Create new layout for tab Info
				delete ui.Tab_Info->layout();
				QVBoxLayout *layout = new QVBoxLayout;
				cur_vm->Emu_Ctl->setMaximumSize( 4096, 30 );

				if( emulRun )
				{
					layout->addWidget( cur_vm->Emu_Ctl );
					cur_vm->Show_Emu_Ctl_Win();
				}

				layout->addWidget( ui.VM_Information_Text );
                layout->setContentsMargins(0,0,0,0);
				ui.Tab_Info->setLayout( layout );
			}
		}
		else // Don't include
		{
			if( emulRun )
				cur_vm->Show_Emu_Ctl_Win();
			else
				cur_vm->Hide_Emu_Ctl_Win();
		}
	}
	else // No show emulator control
	{
		// Hide all
		for( int vx = 0; vx < VM_List.count(); ++vx )
			VM_List[vx]->Hide_Emu_Ctl_Win();
	}
}

void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous )
{
	if( VM_List.count() < 1 )
	{
		AQDebug( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
				 "VM_List.count() < 1" );
		return;
	}

	if( ui.Machines_List->row(previous) < 0 ) return;

    Virtual_Machine tmp_vm;
	Virtual_Machine *old_vm = Get_VM_By_UID( previous->data(256).toString() );

	if( old_vm == NULL )
	{
		AQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous )",
				 "old_vm == NULL" );
		return;
	}

    if( Create_VM_From_Ui(&tmp_vm, old_vm) == false &&
		old_vm->Get_State() != VM::VMS_In_Error )
	{
		AQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
				 "Cannot Create VM! Discarding UI changes for previous VM." );

		// Don't block switching — leave previous VM as-is on disk.
		if( ui.Machines_List->row(current) >= 0 &&
			ui.Machines_List->row(current) < ui.Machines_List->count() )
		{
			Update_VM_Ui();
		}
		return;
	}

	// if previous machine settings were changed — auto-save, never ask
    if( *old_vm != tmp_vm &&
		old_vm->Get_State() != VM::VMS_In_Error && ui.Button_Apply->isEnabled() )
	{
		if( Auto_Save_Timer )
			Auto_Save_Timer->stop();

		disconnect( old_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
					this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

		*old_vm = tmp_vm;

		connect( old_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
				 this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

		old_vm->Save_VM();
		Update_VM_Ui();
		return;
	}
	else
	{
		if( ui.Machines_List->row(current) >= 0 &&
			ui.Machines_List->row(current) < ui.Machines_List->count() )
		{
			Update_VM_Ui();
		}
		else
		{
			AQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
					 "Index Invalid!" );
		}
	}
}

void Main_Window::on_Machines_List_customContextMenuRequested( const QPoint &pos )
{
	QListWidgetItem *it = ui.Machines_List->itemAt( pos );

	if( it != NULL )
		Icon_Menu->exec( ui.Machines_List->mapToGlobal(pos) );
	else
		VM_List_Menu->exec( ui.Machines_List->mapToGlobal(pos) );
}

void Main_Window::on_Machines_List_itemDoubleClicked( QListWidgetItem *item )
{
	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_Machines_List_itemDoubleClicked( QListWidgetItem *item )",
				 "cur_vm == NULL" );
		return;
	}

	if( cur_vm->Get_State() == VM::VMS_Saved )
	{
		AQGraphic_Warning( tr("Warning"), tr("Cannot Change Icon When VM in Save State.") );
		return;
	}

	on_actionChange_Icon_triggered();
}

QString Main_Window::Get_QEMU_Args()
{
	if( ui.Machines_List->currentRow() < 0 || ui.CB_Computer_Type->currentIndex() < 0 )
	{
		AQWarning( "QString Main_Window::Get_QEMU_Args()", "Index < 0" );
		return "";
	}

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "QString Main_Window::Get_QEMU_Args()",
				 "cur_vm == NULL" );
		return "";
	}

	QString line = "";

	if( cur_vm->Get_Use_User_Emulator_Binary() &&
		cur_vm->Get_Only_User_Args() )
	{
		QStringList all_args = cur_vm->Build_QEMU_Args_For_Tab_Info();
		line = all_args.takeAt( 0 );

		for( int i = 0; i < all_args.count(); ++i ) line += " " + all_args[i];
	}
	else
	{
		line = Get_Current_Binary_Name();

		QStringList all_args = cur_vm->Build_QEMU_Args_For_Tab_Info();

		for( int i = 0; i < all_args.count(); ++i ) line += " " + all_args[i];
	}

	return line;
}

QString Main_Window::Get_Current_Binary_Name()
{
	QString line = "";
	Emulator cur_emul = Get_Default_Emulator();

	QMap<QString, QString> bin_list = cur_emul.Get_Binary_Files();

	// Get devices
	bool curMachineOk = false;
	QString find_name = Get_Current_Machine_Devices( &curMachineOk ).System.QEMU_Name;
	if( ! curMachineOk ) return "";

	for( QMap<QString, QString>::const_iterator iter = bin_list.constBegin(); iter != bin_list.constEnd(); iter++ )
	{
		if( iter.key() == find_name )
		{
			line = iter.value();
			break;
		}
	}

	return line;
}

bool Main_Window::Boot_Is_Correct( Virtual_Machine *tmp_vm )
{
	if( tmp_vm == NULL )
	{
		AQError( "bool Main_Window::Boot_Is_Correct( Virtual_Machine *tmp_vm )",
				 "tmp_vm == NULL" );
		return false;
	}

	// Floppy A
	if( tmp_vm->Get_FD0().Get_Enabled() )
	{
		if( ! QFile::exists(tmp_vm->Get_FD0().Get_File_Name()) )
		{
			if( ! No_Device_Found("Floppy A", tmp_vm->Get_FD0().Get_File_Name(), VM::Boot_From_FDA) )
			{
				return false;
			}
			else
			{
				VM_Storage_Device tmp_fd = tmp_vm->Get_FD0();
				tmp_fd.Set_Enabled( false );
				tmp_vm->Set_FD0( tmp_fd );
			}
		}
	}

	// Floppy B
	if( tmp_vm->Get_FD1().Get_Enabled() )
	{
		if( ! QFile::exists(tmp_vm->Get_FD1().Get_File_Name()) )
		{
			if( ! No_Device_Found("Floppy B", tmp_vm->Get_FD1().Get_File_Name(), VM::Boot_From_FDA) )
			{
				return false;
			}
			else
			{
				VM_Storage_Device tmp_fd = tmp_vm->Get_FD1();
				tmp_fd.Set_Enabled( false );
				tmp_vm->Set_FD1( tmp_fd );
			}
		}
	}

	// CD-ROM
	if( tmp_vm->Get_CD_ROM().Get_Enabled() )
	{
		if( ! QFile::exists(tmp_vm->Get_CD_ROM().Get_File_Name()) )
		{
			if( ! No_Device_Found("CD-ROM", tmp_vm->Get_CD_ROM().Get_File_Name(), VM::Boot_From_CDROM) )
			{
				return false;
			}
			else
			{
				VM_Storage_Device tmp_cd = tmp_vm->Get_CD_ROM();
				tmp_cd.Set_Enabled( false );
				tmp_vm->Set_CD_ROM( tmp_cd );
			}
		}
	}

	// HDA
	if( tmp_vm->Get_HDA().Get_Enabled() )
	{
		if( ! QFile::exists(tmp_vm->Get_HDA().Get_File_Name()) )
		{
			if( ! No_Device_Found("HDA", tmp_vm->Get_HDA().Get_File_Name(), VM::Boot_From_HDD) )
			{
				return false;
			}
			else
			{
				VM_HDD tmp_hd = tmp_vm->Get_HDA();
				tmp_hd.Set_Enabled( false );
				tmp_vm->Set_HDA( tmp_hd );
			}
		}
	}

	// HDB
	if( tmp_vm->Get_HDB().Get_Enabled() )
	{
		if( ! QFile::exists(tmp_vm->Get_HDB().Get_File_Name()) )
		{
			if( ! No_Device_Found("HDB", tmp_vm->Get_HDB().Get_File_Name(), VM::Boot_From_HDD) )
			{
				return false;
			}
			else
			{
				VM_HDD tmp_hd = tmp_vm->Get_HDB();
				tmp_hd.Set_Enabled( false );
				tmp_vm->Set_HDB( tmp_hd );
			}
		}
	}

	// HDC
	if( tmp_vm->Get_HDC().Get_Enabled() )
	{
		if( ! QFile::exists(tmp_vm->Get_HDC().Get_File_Name()) )
		{
			if( ! No_Device_Found("HDC", tmp_vm->Get_HDC().Get_File_Name(), VM::Boot_From_HDD) )
			{
				return false;
			}
			else
			{
				VM_HDD tmp_hd = tmp_vm->Get_HDC();
				tmp_hd.Set_Enabled( false );
				tmp_vm->Set_HDC( tmp_hd );
			}
		}
	}

	// HDD
	if( tmp_vm->Get_HDD().Get_Enabled() )
	{
		if( ! QFile::exists(tmp_vm->Get_HDD().Get_File_Name()) )
		{
			if( ! No_Device_Found("HDD", tmp_vm->Get_HDD().Get_File_Name(), VM::Boot_From_HDD) )
			{
				return false;
			}
			else
			{
				VM_HDD tmp_hd = tmp_vm->Get_HDD();
				tmp_hd.Set_Enabled( false );
				tmp_vm->Set_HDD( tmp_hd );
			}
		}
	}

	if( ui.Machines_List->currentRow() >= 0 &&
		ui.Machines_List->currentRow() < VM_List.count() )
	{
		Virtual_Machine *cur_vm = Get_Current_VM();

		if( cur_vm == NULL )
		{
			AQError( "bool Main_Window::Boot_Is_Correct( Virtual_Machine *tmp_vm )",
					 "cur_vm == NULL" );
			return false;
		}

		Dev_Manager->Set_VM( *cur_vm ); // FIXME Use pointer
	}

	// Linux Kernel Files
	if( tmp_vm->Get_Use_Linux_Boot() )
	{
		if( ! QFile::exists(tmp_vm->Get_bzImage_Path()) )
		{
			if( ! No_Device_Found(tr("bzImage"), tmp_vm->Get_bzImage_Path(), VM::Boot_None) )
			{
				return false;
			}
			else
			{
				ui.CH_Use_Linux_Boot->setChecked( false );
				tmp_vm->Set_Use_Linux_Boot( false );
			}
		}

		if( ! QFile::exists(tmp_vm->Get_Initrd_Path()) )
		{
			if( ! No_Device_Found(tr("Initrd"), tmp_vm->Get_Initrd_Path(), VM::Boot_None) )
			{
				return false;
			}
			else
			{
				ui.CH_Use_Linux_Boot->setChecked( false );
				tmp_vm->Set_Use_Linux_Boot( false );
			}
		}
	}

	// ROM File
	if( tmp_vm->Get_Use_ROM_File() )
	{
		if( ! QFile::exists(tmp_vm->Get_ROM_File()) )
		{
			if( ! No_Device_Found(tr("ROM File"), tmp_vm->Get_ROM_File(), VM::Boot_None) )
			{
				return false;
			}
			else
			{
				ui.CH_ROM_File->setChecked( false );
				tmp_vm->Set_Use_ROM_File( false );
			}
		}
	}

	// On-Board Flash Image
	if( tmp_vm->Use_MTDBlock_File() )
	{
		if( ! QFile::exists(tmp_vm->Get_MTDBlock_File()) )
		{
			if( ! No_Device_Found(tr("On-Board Flash"), tmp_vm->Get_MTDBlock_File(), VM::Boot_None) )
			{
				return false;
			}
			else
			{
				ui.CH_MTDBlock->setChecked( false );
				tmp_vm->Use_MTDBlock_File( false );
			}
		}
	}

	// SecureDigital Card Image
	if( tmp_vm->Use_SecureDigital_File() )
	{
		if( ! QFile::exists(tmp_vm->Get_SecureDigital_File()) )
		{
			if( ! No_Device_Found(tr("SecureDigital Card"), tmp_vm->Get_SecureDigital_File(), VM::Boot_None) )
			{
				return false;
			}
			else
			{
				ui.CH_SD_Image->setChecked( false );
				tmp_vm->Use_SecureDigital_File( false );
			}
		}
	}

	// Parallel Flash Image
	if( tmp_vm->Use_PFlash_File() )
	{
		if( ! QFile::exists(tmp_vm->Get_PFlash_File()) )
		{
			if( ! No_Device_Found(tr("Parallel Flash"), tmp_vm->Get_PFlash_File(), VM::Boot_None) )
			{
				return false;
			}
			else
			{
				ui.CH_PFlash->setChecked( false );
				tmp_vm->Use_PFlash_File( false );
			}
		}
	}

    // VNC Certificates
	if( tmp_vm->Use_VNC() && tmp_vm->Use_VNC_TLS() )
	{
		if( tmp_vm->Use_VNC_x509() )
		{
			if( ! QFile::exists(tmp_vm->Get_VNC_x509_Folder_Path()) )
			{
				if( ! No_Device_Found(tr("VNC x509 Folder"), tmp_vm->Get_VNC_x509_Folder_Path(), VM::Boot_None) )
				{
					return false;
				}
				else
				{
					ui.CH_Use_VNC_TLS->setChecked( false );
					tmp_vm->Use_VNC_x509( false );
				}
			}
		}

		if( tmp_vm->Use_VNC_x509verify() )
		{
			if( ! QFile::exists(tmp_vm->Get_VNC_x509verify_Folder_Path()) )
			{
				if( ! No_Device_Found(tr("VNC x509verify Folder"), tmp_vm->Get_VNC_x509verify_Folder_Path(), VM::Boot_None) )
				{
					return false;
				}
				else
				{
					ui.CH_Use_VNC_TLS->setChecked( false );
					tmp_vm->Use_VNC_x509verify( false );
				}
			}
		}
	}

	// Boot is correct?
	QList<VM::Boot_Order> bootOrderList = tmp_vm->Get_Boot_Order_List();
	bool foundEnabledDevice = false;

	for( int bx = 0; bx < bootOrderList.count(); bx++ )
	{
		if( bootOrderList[bx].Enabled )
		{
			foundEnabledDevice = true;

			switch( bootOrderList[bx].Type )
			{
				case VM::Boot_From_FDA:
					if( tmp_vm->Get_FD0().Get_Enabled() &&
					    QFile::exists( tmp_vm->Get_FD0().Get_File_Name() ) )
						return true;
					break;

				case VM::Boot_From_FDB:
					if( tmp_vm->Get_FD1().Get_Enabled() &&
					    QFile::exists( tmp_vm->Get_FD1().Get_File_Name() ) )
						return true;
					break;

				case VM::Boot_From_CDROM:
					if( tmp_vm->Get_CD_ROM().Get_Enabled() &&
					    QFile::exists( tmp_vm->Get_CD_ROM().Get_File_Name() ) )
						return true;
					break;

				case VM::Boot_From_HDD:
					if( ( tmp_vm->Get_HDA().Get_Enabled() && QFile::exists( tmp_vm->Get_HDA().Get_File_Name() ) ) ||
					    ( tmp_vm->Get_HDB().Get_Enabled() && QFile::exists( tmp_vm->Get_HDB().Get_File_Name() ) ) ||
					    ( tmp_vm->Get_HDC().Get_Enabled() && QFile::exists( tmp_vm->Get_HDC().Get_File_Name() ) ) ||
					    ( tmp_vm->Get_HDD().Get_Enabled() && QFile::exists( tmp_vm->Get_HDD().Get_File_Name() ) ) )
						return true;
					break;

				case VM::Boot_From_Network1:
				case VM::Boot_From_Network2:
				case VM::Boot_From_Network3:
				case VM::Boot_From_Network4:
					if( tmp_vm->Get_Use_Network() ) return true;
					break;

				default:
					break;
			}
		}
	}

	if( foundEnabledDevice )
	{
		//AQGraphic_Warning( tr("Error!"), tr("No boot device found!") );
        No_Boot_Device(this).exec();
		return false;
	}
	else return true; // boot device type: None
}

bool Main_Window::No_Device_Found( const QString &name, const QString &path, VM::Boot_Device type )
{
	int retVal = QMessageBox::critical( this, tr("Error!"),
										tr("%1 Image \"%2\" doesn't Exist! Continue Without this Image?").arg(name).arg(path),
										QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

	if( retVal == QMessageBox::No ) return false;
	else
	{
		if( ui.Machines_List->currentRow() >= 0 &&
			ui.Machines_List->currentRow() < VM_List.count() )
		{
			Virtual_Machine *cur_vm = Get_Current_VM();

			if( cur_vm == NULL )
			{
				AQError( "bool Main_Window::No_Device_Found( const QString &name, const QString &path, VM::Boot_Device type )",
						 "cur_vm == NULL" );
				return false;
			}

			cur_vm->Save_VM();
		}

		return true;
	}
}

void Main_Window::on_actionChange_Icon_triggered()
{
	if( VM_List.count() <= 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionChange_Icon_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	Select_Icon_Window icon_win( this );
	icon_win.Set_Previous_Icon_Path( cur_vm->Get_Icon_Path() );

	if( QDialog::Accepted == icon_win.exec() )
	{
		if( ! icon_win.Get_New_Icon_Path().isEmpty() )
		{
			ui.Machines_List->currentItem()->setIcon( QIcon(icon_win.Get_New_Icon_Path()) );
			ui.Machines_List->currentItem()->setData( 128, icon_win.Get_New_Icon_Path() );
		}

		Virtual_Machine *cur_vm = Get_Current_VM();

		if( cur_vm == NULL )
		{
			AQError( "void Main_Window::on_actionChange_Icon_triggered()",
					 "cur_vm == NULL" );
			return;
		}

		cur_vm->Set_Icon_Path( icon_win.Get_New_Icon_Path() );
		cur_vm->Save_VM();
	}
}

void Main_Window::on_actionAbout_AQEMU_triggered()
{
	About_Window( this ).exec();
}

void Main_Window::on_actionAbout_Qt_triggered()
{
	QApplication::aboutQt();
}

void Main_Window::on_actionDelete_VM_triggered()
{
	if( VM_List.count() <= 0 ) return;
	if( ui.Machines_List->currentRow() < 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionDelete_VM_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	int mes_ret = QMessageBox::question( this, tr("Delete?"),
										 tr("Delete \"") + cur_vm->Get_Machine_Name() + tr("\" VM?"),
										 QMessageBox::Yes | QMessageBox::No, QMessageBox::No );

	if( mes_ret == QMessageBox::Yes )
	{
		if( QFile::remove(cur_vm->Get_VM_XML_File_Path()) )
		{
			QString uid = ui.Machines_List->currentItem()->data( 256 ).toString();
			ui.Machines_List->takeItem( ui.Machines_List->currentRow() );

			for( int ix = 0; ix < VM_List.count(); ix++ )
			{
				if( uid == VM_List[ix]->Get_UID() )
                {
                    delete VM_List[ix];
                    VM_List.removeAt(ix);
                }
			}
		}
		else
		{
			AQGraphic_Error( "void Main_Window::on_actionDelete_Virtual_Machine_triggered()",
							 tr("Error!"), tr("Cannot Delete VM XML File!"), false );
		}
	}

	if( VM_List.count() <= 0 )
	{
		ui.actionPower_On->setEnabled( false );
		ui.actionSave->setEnabled( false );
		ui.actionPause->setEnabled( false );
		ui.actionPower_Off->setEnabled( false );
		ui.actionReset->setEnabled( false );
        ui.actionShutdown->setEnabled( false );

		ui.Button_Apply->setEnabled( false );
		ui.Button_Cancel->setEnabled( false );

		Set_Widgets_State( false );

		Update_Info_Text( 1 );
	}
}

void Main_Window::on_actionDelete_VM_And_Files_triggered()
{
	if( VM_List.count() <= 0 ) return;
	if( ui.Machines_List->currentRow() < 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionDelete_VM_And_Files_triggered()",
				 "cur_vm == NULL" );
		return;
	}

    Delete_VM_Files_Window del_win( cur_vm, this );

	if( del_win.exec() == QDialog::Accepted )
	{
		// Delete VM
		QString uid = ui.Machines_List->currentItem()->data( 256 ).toString();
		ui.Machines_List->takeItem( ui.Machines_List->currentRow() );

		for( int ix = 0; ix < VM_List.count(); ix++ )
		{
            if( uid == VM_List[ix]->Get_UID() )
                VM_List.removeAt( ix );
		}

		// No VMs More?
		if( VM_List.count() <= 0 )
		{
			ui.actionPower_On->setEnabled( false );
			ui.actionSave->setEnabled( false );
			ui.actionPause->setEnabled( false );
			ui.actionPower_Off->setEnabled( false );
			ui.actionReset->setEnabled( false );
            ui.actionShutdown->setEnabled( false );

			ui.Button_Apply->setEnabled( false );
			ui.Button_Cancel->setEnabled( false );

			Set_Widgets_State( false );

			Update_Info_Text( 1 );
		}
	}
}

void Main_Window::on_actionExit_triggered()
{
	close();
}

void Main_Window::on_actionShow_New_VM_Wizard_triggered()
{
	VM_Wizard_Window Wizard_Win;

	Wizard_Win.Set_VM_List( &VM_List );

	if( Wizard_Win.exec() == QDialog::Accepted )
	{
		Virtual_Machine *vm = Wizard_Win.New_VM;
		vm->Set_UID( QUuid::createUuid().toString() ); // Create UID
		VM_List << vm;

		QObject::connect( vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
						  this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

		QListWidgetItem *item = new QListWidgetItem( vm->Get_Machine_Name(), ui.Machines_List );
		item->setIcon( QIcon(vm->Get_Icon_Path()) );
		item->setData( 256, vm->Get_UID() );

		ui.Machines_List->setCurrentItem( item );

		Update_VM_Ui();

		on_Button_Apply_clicked();
	}
}

void Main_Window::on_actionAdd_New_VM_triggered()
{
    on_actionShow_New_VM_Wizard_triggered();
}

void Main_Window::on_actionCreate_HDD_Image_triggered()
{
	Create_HDD_Image_Window Create_HDD_Win( this );

	Create_HDD_Win.exec();
}

void Main_Window::on_actionConvert_HDD_Image_triggered()
{
	Convert_HDD_Image_Window Convert_HDD_Win( this );

	Convert_HDD_Win.exec();
}

void Main_Window::on_actionShow_Advanced_Settings_Window_triggered()
{
	Advanced_Settings_Window ad_set;

	if( ad_set.exec() == QDialog::Accepted )
	{
		Save_Settings();

		// Use Log
		if( Settings.value( "Log/Save_In_File", "yes" ).toString() == "yes" )
			AQUse_Log( true );
		else
			AQUse_Log( false );

		// Log File Name
		AQLog_Path( Settings.value("Log/Log_Path", "").toString() );

		// Log Filter
		AQUse_Debug_Output( Settings.value("Log/Print_In_STDOUT", "yes").toString() == "yes",
							Settings.value("Log/Save_Debug","no").toString() == "yes",
							Settings.value("Log/Save_Warning","yes").toString() == "yes",
							Settings.value("Log/Save_Error","yes").toString() == "yes" );

		// Emulators Information Changed?
		QList<Emulator> tmpEmulatorsList = Get_Emulators_List();
		if( tmpEmulatorsList != All_Emulators_List )
		{
            Save_Or_Discard(true);

			// Update Emulators Information
			All_Emulators_List = Get_Emulators_List();

			GUI_User_Mode = true;
			Apply_Emulator( 0 );

			bool q = false, k = false;

			for( int ix = 0; ix < ui.CB_Machine_Accelerator->count(); ix++ )
			{
				if( ui.CB_Machine_Accelerator->itemText(ix) == "TCG" ) q = true;
				else if( ui.CB_Machine_Accelerator->itemText(ix) == "KVM" ) k = true;
			}

			for( int ix = 0; ix < VM_List.count(); ix++ )
			{
				QString type = (VM_List[ ix ]->Get_Machine_Accelerator() == VM::TCG ? "TCG" : "KVM");

				if( type == "TCG" && q == false ) VM_List[ix]->Set_State( VM::VMS_In_Error );
				else if( type == "KVM" && k == false ) VM_List[ix]->Set_State( VM::VMS_In_Error );
				else
				{
					if( VM_List[ix]->Get_State() == VM::VMS_In_Error )
					{
						if( (type == "TCG" && q == true) || (type == "KVM" && k == true) )
						{
							VM_List[ ix ]->Update_Current_Emulator_Devices();
							VM_List[ ix ]->Set_State( VM::VMS_Power_Off );
							VM_List[ ix ]->Save_VM();
							VM_List[ ix ]->Load_VM( VM_List[ix]->Get_VM_XML_File_Path() );
						}
					}
				}
			}

			Load_Settings();
			Update_VM_Ui();
		}
		else
		{
			// Update text in tab Info
			Update_Info_Text();
		}

        // Old/Merged Settings Window
		bool apply_enabled = ui.Button_Apply->isEnabled();
		bool cancel_enabled = ui.Button_Cancel->isEnabled();

		if( QDir::toNativeSeparators(Settings.value("VM_Directory", "~").toString()) != VM_Folder )
		{
            Save_Or_Discard(true);

			// Apply Settings
			Load_Settings();

			// Clear old vm's
			VM_List.clear();
			ui.Machines_List->clear();

			// Load new vm's
			Load_Virtual_Machines();

			return;
		}
		else
		{
			// Apply Settings
			Load_Settings();
		}

		// Update Icons
		for( int ix = 0; ix < VM_List.count(); ++ix )
		{
			Virtual_Machine *tmp_vm = Get_VM_By_UID( ui.Machines_List->item(ix)->data(256).toString() );

			if( tmp_vm == NULL )
			{
				AQError( "void Main_Window::on_actionShow_Settings_Window_triggered()",
						 "tmp_vm == NULL" );
				continue;
			}

			if( tmp_vm->Get_State() == VM::VMS_Saved &&
				Settings.value("Use_Screenshot_for_OS_Logo", "yes").toString() == "yes" )
			{
				ui.Machines_List->item(ix)->setIcon( QIcon(tmp_vm->Get_Screenshot_Path()) );
				ui.Machines_List->item(ix)->setData( 128, tmp_vm->Get_Screenshot_Path() );
			}
			else
			{
				ui.Machines_List->item(ix)->setIcon( QIcon(tmp_vm->Get_Icon_Path()) );
				ui.Machines_List->item(ix)->setData( 128, tmp_vm->Get_Icon_Path() );
			}
		}

        // Adapted from old/merged Settings Window code, but this is/was a hack,
        // so the code above should at some time be rewritten to make the next
        // two lines obsolete
		ui.Button_Apply->setEnabled( apply_enabled );
		ui.Button_Cancel->setEnabled( cancel_enabled );
	}
}

void Main_Window::on_actionShow_First_Run_Wizard_triggered()
{
    if ( ! Save_Or_Discard() )
        return;

	First_Start_Wizard first_start_win( this );

	if( first_start_win.exec() == QDialog::Accepted )
	{
		// Update Emulator List
		if( Update_Emulators_List() )
		{
			All_Emulators_List = Get_Emulators_List();

			GUI_User_Mode = true;
			Apply_Emulator( 0 );

			// Apply Settings
			Load_Settings();

			// Clear old vm's
			VM_List.clear();
			ui.Machines_List->clear();

			// Load new vm's
			Load_Virtual_Machines();
		}
	}
}

// return false on error or when the user cancels
bool Main_Window::Save_Or_Discard(bool forced)
{
    if( ui.Machines_List->count() == 0 )
        return true;

    Virtual_Machine tmp_vm;
	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::Save_Or_Discard()",
				 "cur_vm == NULL" );
		return false;
	}

    if( Create_VM_From_Ui(&tmp_vm, cur_vm) == false )
	{
		AQError( "void Main_Window::Save_Or_Discard()",
				 "Cannot Create VM From Ui!" );
		// On forced quit, do not block the user behind a sync failure
		return forced;
	}

	// Always persist UI changes — never prompt to save/discard.
	if( tmp_vm != *cur_vm )
	{
		if( Auto_Save_Timer )
			Auto_Save_Timer->stop();

		disconnect( cur_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
					this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

		*cur_vm = tmp_vm;

		cur_vm->Save_VM();
		// Refresh UI from the saved VM (CPU count must survive Update_Disabled_Controls).
		Update_VM_Ui();

		connect( cur_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
				 this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );
	}
	else if( Auto_Save_Timer )
	{
		Auto_Save_Timer->stop();
	}

    return true;
}

void Main_Window::on_actionPower_On_triggered()
{
	if ( ! Save_Or_Discard() )
        return;

    Virtual_Machine *cur_vm = Get_Current_VM();
	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionPower_On_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	if( ! Boot_Is_Correct(cur_vm) ) return;

	// Show the session window first, then start QEMU (display connects when Running).
	if( Settings.value( "Embedded_Session", "yes" ).toString() == "yes" )
		Enter_Session_Mode_Preparing( cur_vm );

    if( ! AQEMU_Service::get().call( "start" , cur_vm ) )
	{
        AQError( "void Main_Window::on_action_Power_On_triggered()", "Cannot Start VM!" );
		if( Session_Mode_Active && Session_VM == cur_vm )
			Exit_Session_Mode();
	}
}

void Main_Window::on_actionSave_triggered()
{
	if( VM_List.count() <= 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionSave_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	if( cur_vm->Use_Snapshot_Mode() )
	{
		AQGraphic_Warning( tr("Warning!"), tr("QEMU running in snapshot mode. VM can not be saved in this mode.") );
		return;
	}

	if( Settings.value("Info/Show_Screenshot_in_Save_Mode", "no").toString() == "yes" )
	{
		Virtual_Machine *cur_vm = Get_Current_VM();

		if( cur_vm == NULL )
		{
			AQError( "void Main_Window::on_actionSave_triggered()",
					 "cur_vm == NULL" );
			return;
		}

		QString img_path = QDir::toNativeSeparators( Settings.value("VM_Directory", "~" ).toString() +
													 Get_FS_Compatible_VM_Name(cur_vm->Get_Machine_Name()) + "_screenshot" );

		cur_vm->Take_Screenshot( img_path );
	}

    AQEMU_Service::get().call( "stop" , cur_vm );
}

void Main_Window::on_actionPower_Off_triggered()
{
	if( VM_List.count() <= 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();
	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionPower_Off_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	if( QMessageBox::question(this, tr("Are you sure?"),
		tr("Power off VM \"%1\"?").arg(cur_vm->Get_Machine_Name()),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No )
	{
		return;
	}

	if( Session_Mode_Active && Session_VM == cur_vm )
		Exit_Session_Mode();

    AQEMU_Service::get().call( "stop" , cur_vm );
}

void Main_Window::on_actionShutdown_triggered()
{
    if( VM_List.count() <= 0 ) return;

    Virtual_Machine *cur_vm = Get_Current_VM();
    if( cur_vm == NULL )
    {
        AQError( "void Main_Window::on_actionShutdown_triggered()",
                 "cur_vm == NULL" );
        return;
    }

    if( QMessageBox::question(this, tr("Are you sure?"),
        tr("Shutdown VM \"%1\"?").arg(cur_vm->Get_Machine_Name()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No )
    {
        return;
    }

    AQEMU_Service::get().call( "shutdown" , cur_vm );
}


void Main_Window::on_actionPause_triggered()
{
	if( VM_List.count() <= 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionPause_triggered()",
				 "cur_vm == NULL" );
		return;
	}

    AQEMU_Service::get().call( "pause" , cur_vm );
}

void Main_Window::on_actionReset_triggered()
{
	if( VM_List.count() <= 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionReset_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	if( QMessageBox::question(this, tr("Are you sure?"),
		tr("Reboot VM \"%1\"?").arg(cur_vm->Get_Machine_Name()),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No )
	{
		return;
	}

    AQEMU_Service::get().call( "reset" , cur_vm );
}

void Main_Window::on_actionLoad_VM_From_File_triggered()
{
	QString load_path = QFileDialog::getOpenFileName( this, tr("Open AQEMU VM File"),
													  QDir::homePath(),
													  tr("AQEMU VM (*.aqemu)") );

	if( ! QFile::exists(load_path) ) return;
	load_path = QDir::toNativeSeparators( load_path );

	// ok file name valid
	QFileInfo vm_file( load_path );

	QString new_file_path = QDir::toNativeSeparators( Settings.value("VM_Directory", "~").toString() + vm_file.fileName() );

	if( QFile::exists(new_file_path) )
	{
		AQGraphic_Warning( tr("Warning"), tr("VM With This Name Already Exists!") );
		return;
	}

	QFile::copy( load_path, new_file_path );

	Virtual_Machine *new_vm = new Virtual_Machine();

	new_vm->Load_VM( QDir::toNativeSeparators(Settings.value("VM_Directory", "~").toString() + vm_file.fileName()) );
	new_vm->Set_UID( QUuid::createUuid().toString() ); // Create UID

	VM_List << new_vm;

	connect( new_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
			 this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

	QListWidgetItem *item = new QListWidgetItem( new_vm->Get_Machine_Name(), ui.Machines_List );
	item->setIcon( QIcon(new_vm->Get_Icon_Path()) );
	item->setData( 256, new_vm->Get_UID() );

	ui.Machines_List->setCurrentItem( item );
	//ui.Machines_List->setCurrentRow( ui.Machines_List->count()-1 );

	Update_VM_Ui();
}

void Main_Window::on_actionCopy_triggered()
{
    if ( Get_Current_VM() == nullptr )
    {
        AQError( "void Main_Window::on_actionCopy_triggered()",
                 "cur_vm == NULL" );
        return;
    }

	Copy_VM_Window copy_win;

	// Create Machine Name List
	for( int ix = 0; ix < VM_List.count(); ++ix )
	{
		copy_win.Add_VM_Machine_Name( VM_List[ix]->Get_Machine_Name() );
	}

	if( copy_win.exec() == QDialog::Accepted )
	{
		// Copy VM Object
        auto new_vm = new Virtual_Machine(*Get_Current_VM());

		new_vm->Set_Machine_Name( copy_win.Get_New_VM_Name() );
		new_vm->Set_VM_XML_File_Path( Get_Complete_VM_File_Path(copy_win.Get_New_VM_Name()) );

		// Copy Disk Images
		if( copy_win.Get_Copy_Disk_Images() )
		{
			// Copy Floppy Images
			if( copy_win.Get_Copy_Floppy() )
			{
				if( new_vm->Get_FD0().Get_Enabled() )
				{
					new_vm->Set_FD0( VM_Storage_Device(true, Copy_VM_Floppy(new_vm->Get_Machine_Name(), "FD0", new_vm->Get_FD0())) );
				}

				if( new_vm->Get_FD1().Get_Enabled() )
				{
					new_vm->Set_FD1( VM_Storage_Device(true, Copy_VM_Floppy(new_vm->Get_Machine_Name(), "FD1", new_vm->Get_FD1())) );
				}
			}

			// Copy Hard Drive Images
			if( copy_win.Get_Copy_Hard_Drive() )
			{
				if( new_vm->Get_HDA().Get_Enabled() )
				{
					new_vm->Set_HDA( VM_HDD( true, Copy_VM_Hard_Drive(new_vm->Get_Machine_Name(), "HDA", new_vm->Get_HDA()) ) );
				}

				if( new_vm->Get_HDB().Get_Enabled() )
				{
					new_vm->Set_HDB( VM_HDD( true, Copy_VM_Hard_Drive(new_vm->Get_Machine_Name(), "HDB", new_vm->Get_HDB()) ) );
				}

				if( new_vm->Get_HDC().Get_Enabled() )
				{
					new_vm->Set_HDC( VM_HDD( true, Copy_VM_Hard_Drive(new_vm->Get_Machine_Name(), "HDC", new_vm->Get_HDC()) ) );
				}

				if( new_vm->Get_HDD().Get_Enabled() )
				{
					new_vm->Set_HDD( VM_HDD( true, Copy_VM_Hard_Drive(new_vm->Get_Machine_Name(), "HDD", new_vm->Get_HDD()) ) );
				}
			}
		}

		// Add New VM
		new_vm->Set_UID( QUuid::createUuid().toString() ); // Create UID

		VM_List << new_vm;

		connect( new_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
				 this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

		QListWidgetItem *item = new QListWidgetItem( new_vm->Get_Machine_Name(), ui.Machines_List );
		item->setIcon( QIcon(new_vm->Get_Icon_Path()) );
		item->setData( 256, new_vm->Get_UID() );

		ui.Machines_List->setCurrentItem( item );
		//ui.Machines_List->setCurrentRow( ui.Machines_List->count()-1 );

		Update_VM_Ui();

		on_Button_Apply_clicked();
	}
}

void Main_Window::on_actionSave_As_Template_triggered()
{
	Create_Template_Window templ_win;
	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionSave_As_Template_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	if( VM_List.count() > 0 )
		templ_win.Set_VM_Path( cur_vm->Get_VM_XML_File_Path() );

	if( templ_win.exec() == QDialog::Accepted )
		QMessageBox::information( this, tr("Information"), tr("New Template Created!") );
}

void Main_Window::on_actionShow_Emulator_Control_triggered()
{
	if( VM_List.count() < 0 || ui.Machines_List->currentRow() < 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionShow_Emulator_Control_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	if( cur_vm->Get_State() == VM::VMS_Running ||
		cur_vm->Get_State() == VM::VMS_Pause )
	{
        /*// Emulator Control is Visible?
		if( (Settings.value("Use_VNC_Display", "no").toString() == "yes" && ui.Tabs->tabText(0) == tr("Display")) )
		{
            AQGraphic_Warning( tr("Warning"), tr("Emulator Control Already Shown") );
		}
		else
        {*/
            AQEMU_Service::get().call( "control" , cur_vm );
            //cur_vm->Show_Emu_Ctl_Win();
        /*}*/
	}
	else
	{
		AQGraphic_Warning( tr("Warning"), tr("This Feature Works Only With A Running VM!") );
	}
}

void Main_Window::on_actionManage_Snapshots_triggered()
{
	if( VM_List.count() < 0 ||
		ui.Machines_List->currentRow() < 0 )
	{
		return;
	}

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionManage_Snapshots_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	Snapshots_Window snapshot_win( this );
	snapshot_win.Set_VM( cur_vm );
	snapshot_win.exec();
}

void Main_Window::on_actionShow_QEMU_Arguments_triggered()
{
	if( VM_List.count() > 0 ) QMessageBox::information( this, tr("QEMU Arguments:"), Get_QEMU_Args().replace(" -"," \\\n    -") );
	else QMessageBox::information( this, tr("QEMU Arguments:"), tr("No VM Found!") );
}

void Main_Window::on_actionCreate_Shell_Script_triggered()
{
	if( VM_List.count() <= 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionCreate_Shell_Script_triggered()",
				 "cur_vm == NULL" );
		return;
	}

	QString script_code = "#!/bin/sh\n# This script was created by AQEMU\n" + Get_Current_Binary_Name();
	QStringList all_args = cur_vm->Build_QEMU_Args_For_Script();

	for( int ix = 0; ix < all_args.count(); ix++ ) script_code += " " + all_args[ ix ];

	script_code = script_code.remove( "-monitor stdio" );

	// Save Script
	QString selectedFilter = "";
	QString fileName = QFileDialog::getSaveFileName( this, tr("Save VM to Script"),
											 "VM_" + Get_FS_Compatible_VM_Name(cur_vm->Get_Machine_Name()),
											 tr("Shell Script Files (*.sh);;All Files (*)") );

	if( ! fileName.isEmpty() )
	{
		fileName = QDir::toNativeSeparators( fileName );

		// Save to File
		if( selectedFilter.indexOf("(*.sh)") >= 0 &&
			fileName.endsWith(".sh") == false )
		{
			fileName += ".sh";
		}

		QFile scriptFile( fileName );

		if( ! scriptFile.open(QIODevice::WriteOnly | QIODevice::Text) )
		{
			AQGraphic_Error( "void Main_Window::on_actionCreate_Shell_Script_triggered()",
							 tr("Error!"), tr("Cannot Open File!") );
			return;
		}

		QTextStream out( &scriptFile );
		out << script_code << " \"$@\"\n";
		
		// Set File Permissions
		scriptFile.setPermissions( scriptFile.permissions() | QFile::ExeOwner | QFile::ExeUser );
	}
}

void Main_Window::on_actionShow_QEMU_Error_Log_Window_triggered()
{
	if( VM_List.count() < 0 || ui.Machines_List->currentRow() < 0 ) return;

	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_actionShow_QEMU_Error_Log_Window_triggered()",
				 "cur_vm == NULL" );
		return;
	}

    AQEMU_Service::get().call( "error" , cur_vm );
}

void Main_Window::on_Memory_Size_valueChanged( int value )
{
	int cursorPos = ui.CB_RAM_Size->lineEdit()->cursorPosition();

	if( value % 1024 == 0 ) ui.CB_RAM_Size->setEditText( QString("%1 GB").arg(value / 1024) );
	else ui.CB_RAM_Size->setEditText( QString("%1 MB").arg(value) );

	ui.CB_RAM_Size->lineEdit()->setCursorPosition( cursorPos );
}

void Main_Window::on_CB_RAM_Size_editTextChanged( const QString &text )
{
	if( text.isEmpty() ) return;

	QRegExp rx( "\\s*([\\d]+)\\s*(MB|GB|M|G|)\\s*" ); // like: 512MB or 512
	if( ! rx.exactMatch(text.toUpper()) )
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to memory size!").arg(text) );
		return;
	}

	QStringList ramStrings = rx.capturedTexts();
	if( ramStrings.count() != 3 )
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to memory size!").arg(text) );
		return;
	}

	bool ok = false;
	int value = ramStrings[1].toInt( &ok, 10 );
	if( ! ok )
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to integer!").arg(ramStrings[1]) );
		return;
	}

	if( ramStrings[2] == "MB" || ramStrings[2] == "M" || ramStrings[2] == "" ); // Size in megabytes
	else if( ramStrings[2] == "GB" || ramStrings[2] == "G" ) value *= 1024;
	else
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to size suffix! Valid suffixes are: MB, GB").arg(ramStrings[2]) );
		return;
	}

	if( value <= 0 )
	{
		AQGraphic_Warning( tr("Error"), tr("Memory size < 0! Valid size is 1 or more") );
		return;
	}

	on_TB_Update_Available_RAM_Size_clicked();
	if( (value > ui.Memory_Size->maximum()) &&
		(ui.CH_Remove_RAM_Size_Limitation->isChecked() == false) )
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Your memory size %1 MB > %2 MB - all free RAM on this system!\n"
							  "To set this value, check \"Remove limitation on maximum amount of memory\".")
						   .arg(value).arg(ui.Memory_Size->maximum()) );

		on_Memory_Size_valueChanged( ui.Memory_Size->value() ); // Set valid size
		return;
	}

	// All OK. Set memory size
	ui.Memory_Size->setValue( value );
}

void Main_Window::on_CH_Remove_RAM_Size_Limitation_stateChanged( int state )
{
	if( state == Qt::Checked )
	{
		ui.Memory_Size->setMaximum( 32768 );
		ui.Label_Available_Free_Memory->setText( "32 GB" );
		Update_RAM_Size_ComboBox( 32768 );
	}
	else
	{
		int allRAM = 0, freeRAM = 0;
		System_Info::Get_Free_Memory_Size( allRAM, freeRAM );

		if( allRAM < ui.Memory_Size->value() )
			AQGraphic_Warning( tr("Error"), tr("Current memory size more of all host memory!\nUse the maximum available size.") );

		ui.Memory_Size->setMaximum( allRAM );
		ui.Label_Available_Free_Memory->setText( QString("%1 MB").arg(allRAM) );
		Update_RAM_Size_ComboBox( allRAM );
	}
}

void Main_Window::on_TB_Update_Available_RAM_Size_clicked()
{
	int allRAM = 0, freeRAM = 0;
	System_Info::Get_Free_Memory_Size( allRAM, freeRAM );
	ui.TB_Update_Available_RAM_Size->setText( tr("Free memory: %1 MB").arg(freeRAM) );

	if( ! ui.CH_Remove_RAM_Size_Limitation->isChecked() )
	{
		ui.Memory_Size->setMaximum( allRAM );
		Update_RAM_Size_ComboBox( allRAM );
	}
}

void Main_Window::Update_RAM_Size_ComboBox( int freeRAM )
{
	static int oldRamSize = 0;
	if( freeRAM == oldRamSize ) return;
	else oldRamSize = freeRAM;

	QStringList ramSizes;
	ramSizes << "32 MB" << "64 MB" << "128 MB" << "256 MB" << "512 MB"
			 << "1 GB" << "2 GB" << "3 GB" << "4 GB" << "8 GB" << "16 GB" << "32 GB";
	int maxRamIndex = 0;
	if( freeRAM >= 32768 ) maxRamIndex = 12;
	else if( freeRAM >= 16384 ) maxRamIndex = 11;
	else if( freeRAM >= 8192 ) maxRamIndex = 10;
	else if( freeRAM >= 4096 ) maxRamIndex = 9;
	else if( freeRAM >= 3072 ) maxRamIndex = 8;
	else if( freeRAM >= 2048 ) maxRamIndex = 7;
	else if( freeRAM >= 1024 ) maxRamIndex = 6;
	else if( freeRAM >= 512 ) maxRamIndex = 5;
	else if( freeRAM >= 256 ) maxRamIndex = 4;
	else if( freeRAM >= 128 ) maxRamIndex = 3;
	else if( freeRAM >= 64 ) maxRamIndex = 2;
	else if( freeRAM >= 32 ) maxRamIndex = 1;
	else
	{
		AQGraphic_Warning( tr("Error"), tr("Free memory on this system is less than 32 MB!") );
		return;
	}

	if( maxRamIndex > ramSizes.count() )
	{
		AQError( "void Main_Window::Update_RAM_Size_ComboBox( int freeRAM )",
				 "maxRamIndex > ramSizes.count()" );
		return;
	}

	QString oldText = ui.CB_RAM_Size->currentText();

	ui.CB_RAM_Size->clear();
	for( int ix = 0; ix < maxRamIndex; ix++ ) ui.CB_RAM_Size->addItem( ramSizes[ix] );

	ui.CB_RAM_Size->setEditText( oldText );
}

QStringList Main_Window::Create_Info_HDD_String( const QString &disk_format, const VM::Device_Size &virtual_size,
												 const VM::Device_Size &disk_size, int cluster_size )
{
	QString suf_v = Get_TR_Size_Suffix( virtual_size );
	QString suf_d = Get_TR_Size_Suffix( disk_size );

	QStringList ret;
	ret << tr("Image Format: ") + disk_format + "\n" + tr("Allocated Disk Space: ") + QString::number(disk_size.Size) + " " + suf_d;
	ret << tr("Virtual Size: ") + QString::number(virtual_size.Size) + " " + suf_v + "\n" + tr("Cluster Size: ") + QString::number(cluster_size);

	return ret;
}

void Main_Window::on_CB_Computer_Type_currentIndexChanged( int index )
{
	Computer_Type_Changed();
}

void Main_Window::on_CB_Machine_Accelerator_currentIndexChanged( int index )
{
	Apply_Emulator( 1 );
}


void Main_Window::Computer_Type_Changed()
{
	bool devOk = false;
	Available_Devices curComp;
	int comp_index = 0;

	comp_index = ui.CB_Computer_Type->currentIndex();

	if( comp_index < 0 )
	{
        AQDebug("index below 00000","asdf");
	    return;
	}

	QStringList cl;

    ui_arch.CB_CPU_Type->blockSignals(true);
    ui_arch.CB_Machine_Type->blockSignals(true);
    ui.CB_CPU_Type_Main->blockSignals(true);
    ui.CB_Machine_Type_Main->blockSignals(true);
    ui.CB_Video_Card->blockSignals(true);

	// CPU
	ui_arch.CB_CPU_Type->clear();
	ui.CB_CPU_Type_Main->clear();

	cl = QStringList();

	curComp = Get_Current_Machine_Devices( &devOk );
	if( ! devOk )
	    return;

	for( int mx = 0; mx < curComp.CPU_List.count(); ++mx )
		cl << curComp.CPU_List[mx].Caption;

	ui_arch.CB_CPU_Type->addItems( cl );
	ui.CB_CPU_Type_Main->addItems( cl );

	// Machine — full probed list (was truncated at 64 before)
	ui_arch.CB_Machine_Type->clear();
	ui.CB_Machine_Type_Main->clear();

	cl = QStringList();

	for( int mx = 0; mx < curComp.Machine_List.count(); ++mx )
		cl << curComp.Machine_List[mx].Caption;

	ui_arch.CB_Machine_Type->addItems( cl );
	ui.CB_Machine_Type_Main->addItems( cl );

	// Prefer virt / max for aarch64 (and friends) as the working default
	const QString arch_bin = curComp.System.QEMU_Name;
	const bool is_virt_arch =
		arch_bin.contains( "aarch64", Qt::CaseInsensitive ) ||
		arch_bin.contains( "qemu-system-arm", Qt::CaseInsensitive ) ||
		arch_bin.contains( "riscv", Qt::CaseInsensitive );

	if( is_virt_arch )
	{
		for( int mx = 0; mx < curComp.Machine_List.count(); ++mx )
		{
			if( curComp.Machine_List[mx].QEMU_Name == "virt" )
			{
				ui_arch.CB_Machine_Type->setCurrentIndex( mx );
				ui.CB_Machine_Type_Main->setCurrentIndex( mx );
				break;
			}
		}
		QString prefer_cpu =
		#ifdef Q_OS_WIN32
			"max";
		#else
			"host";
		#endif
		int cpu_idx = -1;
		for( int cx = 0; cx < curComp.CPU_List.count(); ++cx )
		{
			if( curComp.CPU_List[cx].QEMU_Name == prefer_cpu )
			{
				cpu_idx = cx;
				break;
			}
			if( cpu_idx < 0 && curComp.CPU_List[cx].QEMU_Name == "max" )
				cpu_idx = cx;
		}
		if( cpu_idx >= 0 )
		{
			ui_arch.CB_CPU_Type->setCurrentIndex( cpu_idx );
			ui.CB_CPU_Type_Main->setCurrentIndex( cpu_idx );
		}
		for( int vx = 0; vx < curComp.Video_Card_List.count(); ++vx )
		{
			if( curComp.Video_Card_List[vx].QEMU_Name == "virtio-gpu-pci" )
			{
				// Will set after video list is filled below
				break;
			}
		}
	}

	// Video
	ui.CB_Video_Card->clear();

	QString want_video = System_Info::Default_Video_Card( arch_bin );
	Virtual_Machine *cur_vm = Get_Current_VM();
	if( cur_vm )
	{
		want_video = System_Info::Sanitize_Video_Card(
			arch_bin, cur_vm->Get_Video_Card(), cur_vm->Get_Machine_Type() );
		if( want_video != cur_vm->Get_Video_Card() )
			cur_vm->Set_Video_Card( want_video );
	}

	for( int vx = 0; vx < curComp.Video_Card_List.count(); ++vx )
	{
		const Device_Map &vc = curComp.Video_Card_List[vx];
		ui.CB_Video_Card->addItem( vc.Caption, vc.QEMU_Name );
	}

	const int video_sel = ui.CB_Video_Card->findData( want_video );
	if( video_sel >= 0 )
		ui.CB_Video_Card->setCurrentIndex( video_sel );
	else if( ui.CB_Video_Card->count() > 0 )
		ui.CB_Video_Card->setCurrentIndex( 0 );

	// Use Nativ Network Cards FIXME set emulator PSO to net card widget
	if( ui.RB_Network_Mode_New->isChecked() )
		New_Network_Settings_Widget->Set_Network_Card_Models( curComp.Network_Card_List );
	else
		Old_Network_Settings_Widget->Set_Network_Card_Models( curComp.Network_Card_List );

	// Audio — enable every device this arch supports (user can pick)
	ui.CH_sb16->setEnabled( curComp.Audio_Card_List.Audio_sb16 );
	ui.CH_es1370->setEnabled( curComp.Audio_Card_List.Audio_es1370 );
	ui.CH_Adlib->setEnabled( curComp.Audio_Card_List.Audio_Adlib );
	ui.CH_AC97->setEnabled( curComp.Audio_Card_List.Audio_AC97 );
	ui.CH_GUS->setEnabled( curComp.Audio_Card_List.Audio_GUS );
	ui.CH_PCSPK->setEnabled( curComp.Audio_Card_List.Audio_PC_Speaker );
	ui.CH_HDA->setEnabled( curComp.Audio_Card_List.Audio_HDA );
	ui.CH_cs4231a->setEnabled( curComp.Audio_Card_List.Audio_cs4231a );
	ui.CH_VirtIO_Sound->setEnabled( curComp.Audio_Card_List.Audio_VirtIO );
	ui.CH_USB_Audio->setEnabled( curComp.Audio_Card_List.Audio_USB );

	// Default to Intel HDA when nothing is selected yet
	const bool any_sound =
		ui.CH_sb16->isChecked() || ui.CH_es1370->isChecked() || ui.CH_Adlib->isChecked() ||
		ui.CH_AC97->isChecked() || ui.CH_GUS->isChecked() || ui.CH_PCSPK->isChecked() ||
		ui.CH_HDA->isChecked() || ui.CH_cs4231a->isChecked() ||
		ui.CH_VirtIO_Sound->isChecked() || ui.CH_USB_Audio->isChecked();
	if( ! any_sound && curComp.Audio_Card_List.Audio_HDA && ui.CH_HDA->isEnabled() )
		ui.CH_HDA->setChecked( true );

    ui_arch.CB_CPU_Type->blockSignals(false);
    ui_arch.CB_Machine_Type->blockSignals(false);
    ui.CB_CPU_Type_Main->blockSignals(false);
    ui.CB_Machine_Type_Main->blockSignals(false);
    ui.CB_Video_Card->blockSignals(false);

	Update_Display_Resolution_Enabled();

	// Other Options
	Update_Win11_Lifecycle_Ui();
	Enforce_Accel_Honesty();
	Update_Disabled_Controls();
}

void Main_Window::on_CB_Machine_Type_Main_currentIndexChanged( int index )
{
	if( index < 0 ) return;
	ui_arch.CB_Machine_Type->blockSignals(true);
	if( index < ui_arch.CB_Machine_Type->count() )
		ui_arch.CB_Machine_Type->setCurrentIndex( index );
	ui_arch.CB_Machine_Type->blockSignals(false);
	VM_Changed();
}

void Main_Window::on_CB_CPU_Type_Main_currentIndexChanged( int index )
{
	if( index < 0 ) return;
	ui_arch.CB_CPU_Type->blockSignals(true);
	if( index < ui_arch.CB_CPU_Type->count() )
		ui_arch.CB_CPU_Type->setCurrentIndex( index );
	ui_arch.CB_CPU_Type->blockSignals(false);
	VM_Changed();
}

void Main_Window::sync_arch_Machine_Type_changed( int index )
{
	if( index < 0 ) return;
	ui.CB_Machine_Type_Main->blockSignals(true);
	if( index < ui.CB_Machine_Type_Main->count() )
		ui.CB_Machine_Type_Main->setCurrentIndex( index );
	ui.CB_Machine_Type_Main->blockSignals(false);
	VM_Changed();
}

void Main_Window::sync_arch_CPU_Type_changed( int index )
{
	if( index < 0 ) return;
	ui.CB_CPU_Type_Main->blockSignals(true);
	if( index < ui.CB_CPU_Type_Main->count() )
		ui.CB_CPU_Type_Main->setCurrentIndex( index );
	ui.CB_CPU_Type_Main->blockSignals(false);
	VM_Changed();
}

void Main_Window::Update_Machine_Accelerators()
{
    ui.CB_Machine_Accelerator->blockSignals(true);
	ui.CB_Machine_Accelerator->clear();
	ui.CB_Machine_Accelerator->addItem( tr("TCG"), QStringLiteral( "tcg" ) );
	ui.CB_Machine_Accelerator->addItem( tr("KVM"), QStringLiteral( "kvm" ) );
	ui.CB_Machine_Accelerator->addItem( tr("XEN"), QStringLiteral( "xen" ) );
    ui.CB_Machine_Accelerator->blockSignals(false);
	Enforce_Accel_Honesty();
}

void Main_Window::Enforce_Accel_Honesty()
{
	const QString host = QSysInfo::currentCpuArchitecture().toLower();
	bool ok = false;
	const Available_Devices dev = Get_Current_Machine_Devices( &ok );
	const QString guest = ok ? dev.System.QEMU_Name.toLower() : QString();

	auto *model = qobject_cast<QStandardItemModel *>( ui.CB_Machine_Accelerator->model() );
	ui.CB_Machine_Accelerator->blockSignals( true );

	// Guest binary unknown (startup / no VM selected): keep all accelerators selectable.
	if( guest.isEmpty() )
	{
		for( int i = 0; i < ui.CB_Machine_Accelerator->count(); ++i )
		{
			if( model )
			{
				QStandardItem *item = model->item( i );
				if( item )
					item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
			}
		}
		ui.CB_Machine_Accelerator->setToolTip( QString() );
		ui.CB_Machine_Accelerator->blockSignals( false );
		Update_Accelerator_Options();
		return;
	}

	const bool host_x86 =
		host.contains( QLatin1String( "x86" ) ) ||
		host == QLatin1String( "amd64" ) ||
		host == QLatin1String( "i386" );
	const bool host_arm =
		host.contains( QLatin1String( "arm" ) ) ||
		host.contains( QLatin1String( "aarch64" ) );

	const bool guest_x86 =
		guest.contains( QLatin1String( "x86_64" ) ) ||
		guest.contains( QLatin1String( "i386" ) );
	const bool guest_arm =
		guest.contains( QLatin1String( "aarch64" ) ) ||
		guest.contains( QLatin1String( "qemu-system-arm" ) ) ||
		( guest.contains( QLatin1String( "arm" ) ) &&
		  ! guest.contains( QLatin1String( "x86" ) ) );

	const bool is_native =
		( host_x86 && guest_x86 ) ||
		( host_arm && guest_arm );

	int tcg_index = -1;
	for( int i = 0; i < ui.CB_Machine_Accelerator->count(); ++i )
	{
		const QString id = ui.CB_Machine_Accelerator->itemData( i, Qt::UserRole ).toString().toLower();
		if( id == QLatin1String( "tcg" ) )
			tcg_index = i;

		const bool native_only =
			( id == QLatin1String( "kvm" ) || id == QLatin1String( "xen" ) );

		if( model )
		{
			QStandardItem *item = model->item( i );
			if( ! item )
				continue;
			if( ! is_native && native_only )
				item->setFlags( item->flags() & ~( Qt::ItemIsEnabled | Qt::ItemIsSelectable ) );
			else
				item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
		}
	}

	if( ! is_native )
	{
		if( tcg_index >= 0 )
			ui.CB_Machine_Accelerator->setCurrentIndex( tcg_index );
		ui.CB_Machine_Accelerator->setToolTip(
			tr( "Cross-architecture emulation: native acceleration (KVM / WHPX / HVF) "
				"cannot run this guest on this host. Using software translation (TCG)." ) );
	}
	else
	{
		ui.CB_Machine_Accelerator->setToolTip(
			tr( "On Windows x86 hosts, KVM maps to WHPX/HAX with TCG fallback when available." ) );
	}

	ui.CB_Machine_Accelerator->blockSignals( false );
	Update_Accelerator_Options();
}

void Main_Window::Update_Accelerator_Options()
{
	const QString id = ui.CB_Machine_Accelerator->currentData( Qt::UserRole ).toString().toLower();
	const bool is_kvm =
		( id == QLatin1String( "kvm" ) ) ||
		( ui.CB_Machine_Accelerator->currentText().compare( QLatin1String( "KVM" ), Qt::CaseInsensitive ) == 0 );
    if ( is_kvm )
        ui.TB_Show_Accelerator_Options_Window->setEnabled(true);
    else
        ui.TB_Show_Accelerator_Options_Window->setEnabled(false);
}

void Main_Window::Update_Computer_Types()
{
	QMap<QString, Available_Devices> current_devices;
	bool devOk = false;

	current_devices = Get_Devices_Info( &devOk );
	if( ! devOk )
	    return;

    QString text = ui.CB_Computer_Type->currentText();

    ui.CB_Computer_Type->blockSignals(true);

	ui.CB_Computer_Type->clear();

	for( QMap<QString, Available_Devices>::const_iterator i = current_devices.constBegin(); i != current_devices.constEnd(); i++ )
    {
		ui.CB_Computer_Type->addItem( i->System.Caption );
    }
    ui.CB_Computer_Type->setCurrentText(text);

	// Never grey out architectures — users may pick any guest arch.
	// Accelerators that cannot run a guest will fall back to TCG at start / via accel tip.
    auto model = qobject_cast<QStandardItemModel*>(ui.CB_Computer_Type->model());
	if( model )
	{
		for( int i = 0; i < model->rowCount(); i++ )
		{
			auto item = model->item( i );
			if( ! item ) continue;
			item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
			item->setData( QVariant(), Qt::TextColorRole );
		}
	}

    ui.CB_Computer_Type->blockSignals(false);
}

void Main_Window::Apply_Emulator( int mode )
{
    // FIXME
	//static bool firstRun = true;
	static bool running = false;

	if( GUI_User_Mode == false )
	    return;
	//if( running == true && firstRun == false ) return;

	//firstRun = false;
	if( running == true )
	    return;
	running = true;

	switch( mode )
	{
		case 0:
			// Machine Accelerators
			Update_Machine_Accelerators();
		case 1:
            Update_Accelerator_Options();
		case 2:
			// Computer Type
		    Update_Computer_Types();
		case 3:
            Computer_Type_Changed();
            break;

		default:
			AQWarning( "void Main_Window::Apply_Emulator( int mode )", "Default Section!" );
			break;
	}

	VM_Changed();
    running = false;
}

void Main_Window::CB_Boot_Priority_currentIndexChanged( int index )
{
	// Clear old string
	if( ui.CB_Boot_Priority->count() >= 5 ) ui.CB_Boot_Priority->removeItem( 5 );

	VM::Boot_Device bootDev;

	switch( ui.CB_Boot_Priority->currentIndex() )
	{
		case 0:
			bootDev = VM::Boot_From_FDA;
			break;

		case 1:
			bootDev = VM::Boot_From_HDD;
			break;

		case 2:
			bootDev = VM::Boot_From_CDROM;
			break;

		case 3:
			bootDev = VM::Boot_From_Network1;
			break;

		case 4:
			bootDev = VM::Boot_None;
			break;

		default:
			AQWarning( "void Main_Window::on_CB_Boot_Priority_currentIndexChanged( int index )",
					   "Use Default Boot Device: CD-ROM" );
			bootDev = VM::Boot_From_CDROM;
			break;
	}

	for( int bx = 0; bx < Boot_Order_List.count(); bx++ )
	{
		if( Boot_Order_List[bx].Type == bootDev ) Boot_Order_List[ bx ].Enabled = true;
		else Boot_Order_List[ bx ].Enabled = false;
	}

    VM_Changed();
}

void Main_Window::Set_Boot_Order( const QList<VM::Boot_Order> &list )
{
	disconnect( ui.CB_Boot_Priority, SIGNAL(currentIndexChanged(int)),
				this, SLOT(CB_Boot_Priority_currentIndexChanged(int)) );

    QStringList bootStr = VM::Boot_Order_To_String_List(list);

	// Clear old string
	if( ui.CB_Boot_Priority->count() >= 5 ) ui.CB_Boot_Priority->removeItem( 5 );

	// Select boot device
	if( bootStr.count() < 1 ) // None
	{
		ui.CB_Boot_Priority->setCurrentIndex( 4 );
	}
	else if( bootStr.count() == 1 ) // One
	{
		if( bootStr[0] == "FDA" || bootStr[0] == "FDB" ) ui.CB_Boot_Priority->setCurrentIndex( 0 );
		else if( bootStr[0] == "CDROM" ) ui.CB_Boot_Priority->setCurrentIndex( 2 );
		else if( bootStr[0] == "HDD" ) ui.CB_Boot_Priority->setCurrentIndex( 1 );
		else if( bootStr[0] == "Net1" || bootStr[0] == "Net2" ||
				 bootStr[0] == "Net3" || bootStr[0] == "Net4" ) ui.CB_Boot_Priority->setCurrentIndex( 3 );
		else
		{
			AQError( "void Main_Window::Set_Boot_Order( QList<VM::Boot_Order> &list )",
					 "Incorrent boot device type \"" + bootStr[0] +"\"!" );
		}
	}
	else // More (Boot order list)
	{
		QString itemText = "";

		for( int ix = 0; ix < bootStr.count(); ix++ )
		{
			itemText += bootStr[ ix ];
			if( (ix + 1) < bootStr.count() ) itemText += "/";
		}

		ui.CB_Boot_Priority->addItem( itemText );
		ui.CB_Boot_Priority->setCurrentIndex( ui.CB_Boot_Priority->count() - 1 );
	}

	connect( ui.CB_Boot_Priority, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(CB_Boot_Priority_currentIndexChanged(int)) );
}

void Main_Window::on_TB_Show_Boot_Settings_Window_clicked()
{
    Boot_Device_Window boot_win(this);
	boot_win.setData( Boot_Order_List );
	boot_win.setUseBootMenu( Show_Boot_Menu );

	if( boot_win.exec() == QDialog::Accepted )
	{
		Boot_Order_List = boot_win.data();
		Show_Boot_Menu = boot_win.useBootMenu();

		// Apply data to UI
		Set_Boot_Order( Boot_Order_List );
	}
}

void Main_Window::on_TB_Show_Accelerator_Options_Window_clicked()
{
    Discard_Changes ( Accelerator_Options );
}

void Main_Window::on_TB_Show_Architecture_Options_Window_clicked()
{
    Discard_Changes ( Architecture_Options );
}

void Main_Window::Discard_Changes(QDialog* dialog)
{
    auto old_vm = Get_Current_VM();
    Virtual_Machine old_vm_copy(*old_vm);
    Virtual_Machine tmp_vm;
    bool ok = Create_VM_From_Ui(&tmp_vm, old_vm, false);
    bool a = ui.Button_Apply->isEnabled();
    bool c = ui.Button_Cancel->isEnabled();

    if ( dialog->exec() == QDialog::Accepted )
        return;

    if ( ok )
    {
        *old_vm = tmp_vm;
        Update_VM_Ui(false);

        *old_vm = old_vm_copy;

	    ui.Button_Apply->setEnabled( a );
	    ui.Button_Cancel->setEnabled( c );
    }
}

void Main_Window::on_TB_Show_Advanced_Options_Window_clicked()
{
    Discard_Changes ( Advanced_Options );
}

void Main_Window::on_TB_Show_SMP_Settings_Window_clicked()
{
	if( ! Validate_CPU_Count(ui.CB_CPU_Count->currentText()) ) return;

	// New SMP count?
    if( SMP_Settings->Get_Values().SMP_Count != ui.CB_CPU_Count->currentText().toInt() )
        SMP_Settings->Set_SMP_Count( ui.CB_CPU_Count->currentText().toInt() );

    if( SMP_Settings->exec() == QDialog::Accepted )
	{
        if( SMP_Settings->Get_Values().SMP_Count != ui.CB_CPU_Count->currentText().toInt() )
		{
			// Set new CPU count value
			disconnect( ui.CB_CPU_Count, SIGNAL(editTextChanged(const QString &)),
						this, SLOT(Validate_CPU_Count(const QString&)) );

            ui.CB_CPU_Count->setEditText( QString::number(SMP_Settings->Get_Values().SMP_Count) );

			connect( ui.CB_CPU_Count, SIGNAL(editTextChanged(const QString &)),
					 this, SLOT(Validate_CPU_Count(const QString&)) );
		}
		else
		{
			// Settings changed?
            if( SMP_Settings->Get_Values() != Get_Current_VM()->Get_SMP() )
				VM_Changed();
		}
	}
}

bool Main_Window::Validate_CPU_Count( const QString &text )
{
	if( text.isEmpty() ) return false;

	bool cpuOk = false;
	int cpuCountTmp = text.toInt( &cpuOk );
	if( ! cpuOk )
	{
		AQGraphic_Warning( tr("Error!"), tr("CPU count value is not valid digit!") );
		return false;
	}

	cpuOk = false;
	Available_Devices tmpDev = Get_Current_Machine_Devices( &cpuOk );
	if( ! cpuOk )
	{
		AQError( "bool Main_Window::Validate_CPU_Count( const QString &text )",
				 "Cannot get devices!" );
		return false;
	}

	if( cpuCountTmp <= tmpDev.PSO_SMP_Count )
	{
		// Reset old SMP options
        if( SMP_Settings->Get_Values().SMP_Count != ui.CB_CPU_Count->currentText().toInt() )
            SMP_Settings->Set_SMP_Count( cpuCountTmp );

		return true;
	}
	else
	{
		AQGraphic_Warning( tr("Warning"), tr("CPU count > max CPU count for this emulator!") );
		return false;
	}
}

void Main_Window::on_CH_Local_Time_toggled( bool on )
{
	if( on ) ui_ao.CH_Start_Date->setChecked( false );
}

void Main_Window::on_Button_VirtIO_Defaults_clicked()
{
	bool ok = false;
	Available_Devices cur = Get_Current_Machine_Devices( &ok );
	if( ! ok ) return;

	// Machine: virt
	for( int i = 0; i < cur.Machine_List.count(); ++i )
	{
		if( cur.Machine_List[i].QEMU_Name == "virt" )
		{
			ui.CB_Machine_Type_Main->setCurrentIndex( i );
			break;
		}
	}

	// CPU: max (or host on Linux)
	QString prefer =
	#ifdef Q_OS_WIN32
		"max";
	#else
		"host";
	#endif
	int cpu_idx = -1;
	for( int i = 0; i < cur.CPU_List.count(); ++i )
	{
		if( cur.CPU_List[i].QEMU_Name == prefer ) { cpu_idx = i; break; }
		if( cpu_idx < 0 && cur.CPU_List[i].QEMU_Name == "max" ) cpu_idx = i;
	}
	if( cpu_idx >= 0 )
		ui.CB_CPU_Type_Main->setCurrentIndex( cpu_idx );

	// Video: virtio-gpu-pci
	const int vix = ui.CB_Video_Card->findData( "virtio-gpu-pci" );
	if( vix >= 0 )
		ui.CB_Video_Card->setCurrentIndex( vix );

	ui.CB_Disk_Interface->setCurrentIndex( 0 ); // VirtIO
	ui.CB_CPU_Count->setEditText( "4" );
	ui.Memory_Size->setValue( 8192 );

	// Audio: USB (Win11 ARM / kiosk); leave VirtIO sound available but unchecked
	ui.CH_sb16->setChecked( false );
	ui.CH_es1370->setChecked( false );
	ui.CH_Adlib->setChecked( false );
	ui.CH_AC97->setChecked( false );
	ui.CH_GUS->setChecked( false );
	ui.CH_PCSPK->setChecked( false );
	ui.CH_HDA->setChecked( false );
	ui.CH_cs4231a->setChecked( false );
	ui.CH_VirtIO_Sound->setChecked( false );
	ui.CH_USB_Audio->setChecked( true );

	// Network: virtio-net-pci
	QList<VM_Net_Card> nets;
	if( Old_Network_Settings_Widget->Get_Network_Cards( nets ) && nets.count() > 0 )
	{
		nets[0].Set_Card_Model( "virtio-net-pci" );
		Old_Network_Settings_Widget->Set_Network_Cards( nets );
	}

	Virtual_Machine *vm = Get_Current_VM();
	if( vm )
	{
		vm->Use_USB_Hub( true );
		vm->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
		vm->Set_Mouse_USB_Controller( QStringLiteral( "xhci" ) );
		vm->Use_VirtIO_RNG( true );
		vm->Use_VirtIO_Balloon( true );
		vm->Use_VirtIO_Keyboard( true );
	}

	VM_Changed();
}

void Main_Window::on_Button_Win11_Install_clicked()
{
	Apply_Win11_Lifecycle_Mode( VM::Win11_Install );
}

void Main_Window::on_Button_Win11_First_Boot_clicked()
{
	Apply_Win11_Lifecycle_Mode( VM::Win11_First_Boot );
}

void Main_Window::on_Button_Win11_Normal_clicked()
{
	Apply_Win11_Lifecycle_Mode( VM::Win11_Normal );
}

void Main_Window::Update_Win11_Lifecycle_Ui()
{
	const bool aarch64 =
		ui.CB_Computer_Type->currentText().contains( "AArch64", Qt::CaseInsensitive ) ||
		ui.CB_Computer_Type->currentText().contains( "aarch64", Qt::CaseInsensitive ) ||
		ui.CB_Computer_Type->currentData().toString().contains( "aarch64", Qt::CaseInsensitive );

	ui.label_win11_lifecycle->setVisible( aarch64 );
	ui.GB_Win11_Lifecycle->setVisible( aarch64 );
	ui.verticalSpacer_win11_lifecycle->changeSize(
		20, aarch64 ? 16 : 0, QSizePolicy::Minimum, aarch64 ? QSizePolicy::Fixed : QSizePolicy::Ignored );
	ui.verticalSpacer_win11_lifecycle->invalidate();

	if( ! aarch64 )
		return;

	Virtual_Machine *vm = Get_Current_VM();
	const VM::Win11_Lifecycle_Mode mode =
		vm ? vm->Get_Win11_Lifecycle_Mode() : VM::Win11_Normal;

	QString status;
	switch( mode )
	{
		case VM::Win11_Install:
			status = tr( "Mode: Install Windows" );
			break;
		case VM::Win11_First_Boot:
			status = tr( "Mode: First boot" );
			break;
		case VM::Win11_Normal:
		default:
			status = tr( "Mode: Normal" );
			break;
	}
	ui.Label_Win11_Lifecycle_Status->setText( status );
}

void Main_Window::Apply_Win11_Lifecycle_Mode( VM::Win11_Lifecycle_Mode mode )
{
	Virtual_Machine *vm = Get_Current_VM();
	if( ! vm )
		return;

	bool ok = false;
	Available_Devices cur = Get_Current_Machine_Devices( &ok );
	if( ! ok ) return;

	vm->Set_Win11_Lifecycle_Mode( mode );

	// Shared VirtIO / Win11 ARM baseline (same spirit as Apply VirtIO defaults)
	for( int i = 0; i < cur.Machine_List.count(); ++i )
	{
		if( cur.Machine_List[i].QEMU_Name == "virt" )
		{
			ui.CB_Machine_Type_Main->setCurrentIndex( i );
			break;
		}
	}

	ui.CB_Disk_Interface->setCurrentIndex( 0 ); // VirtIO
	ui.CH_sb16->setChecked( false );
	ui.CH_es1370->setChecked( false );
	ui.CH_Adlib->setChecked( false );
	ui.CH_AC97->setChecked( false );
	ui.CH_GUS->setChecked( false );
	ui.CH_PCSPK->setChecked( false );
	ui.CH_HDA->setChecked( false );
	ui.CH_cs4231a->setChecked( false );
	ui.CH_VirtIO_Sound->setChecked( false );
	ui.CH_USB_Audio->setChecked( true );

	QList<VM_Net_Card> nets;
	if( Old_Network_Settings_Widget->Get_Network_Cards( nets ) && nets.count() > 0 )
	{
		nets[0].Set_Card_Model( "virtio-net-pci" );
		Old_Network_Settings_Widget->Set_Network_Cards( nets );
	}

	vm->Use_USB_Hub( true );
	vm->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
	vm->Set_Mouse_USB_Controller( QStringLiteral( "xhci" ) );
	vm->Use_VirtIO_RNG( true );
	vm->Use_VirtIO_Balloon( true );
	vm->Use_VirtIO_Keyboard( true );
	vm->Use_UEFI( true );

	const int mouse_ix = ui.CB_Mouse_Type->findData( "usb-tablet" );
	if( mouse_ix >= 0 )
		ui.CB_Mouse_Type->setCurrentIndex( mouse_ix );
	else
	{
		const int by_text = ui.CB_Mouse_Type->findText( "usb-tablet", Qt::MatchContains );
		if( by_text >= 0 )
			ui.CB_Mouse_Type->setCurrentIndex( by_text );
	}

	QList<VM::Boot_Order> boot;
	VM::Boot_Order b;

	if( mode == VM::Win11_Install )
	{
		const int vix = ui.CB_Video_Card->findData( "ramfb" );
		if( vix >= 0 )
			ui.CB_Video_Card->setCurrentIndex( vix );

		if( ui.Memory_Size->value() > 8192 )
			ui.Memory_Size->setValue( 8192 );

		// Keep ISO path; ensure CD is enabled for install
		if( ! Dev_Manager->CD_ROM.Get_File_Name().isEmpty() )
			Dev_Manager->CD_ROM.Set_Enabled( true );

		b.Type = VM::Boot_From_CDROM;
		b.Enabled = true;
		boot << b;
		b.Type = VM::Boot_From_HDD;
		b.Enabled = true;
		boot << b;
		Boot_Order_List = boot;
		Set_Boot_Order( Boot_Order_List );
	}
	else if( mode == VM::Win11_First_Boot )
	{
		const int vix = ui.CB_Video_Card->findData( "ramfb" );
		if( vix >= 0 )
			ui.CB_Video_Card->setCurrentIndex( vix );

		// Keep path but detach ISO for this phase
		Dev_Manager->CD_ROM.Set_Enabled( false );

		b.Type = VM::Boot_From_HDD;
		b.Enabled = true;
		boot << b;
		Boot_Order_List = boot;
		Set_Boot_Order( Boot_Order_List );
	}
	else // Normal
	{
		const int vix = ui.CB_Video_Card->findData( "virtio-gpu-pci" );
		if( vix >= 0 )
			ui.CB_Video_Card->setCurrentIndex( vix );

		Dev_Manager->CD_ROM.Set_Enabled( false );

		b.Type = VM::Boot_From_HDD;
		b.Enabled = true;
		boot << b;
		Boot_Order_List = boot;
		Set_Boot_Order( Boot_Order_List );
	}

	Update_Win11_Lifecycle_Ui();
	Update_Display_Resolution_Enabled();
	on_Button_Apply_clicked();
}

void Main_Window::on_Tabs_currentChanged( int index )
{
	if( index == 2 ) Dev_Manager->Update_List_Mode();
}

void Main_Window::on_Button_Apply_clicked()
{
    Virtual_Machine tmp_vm;
	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_Button_Apply_clicked()",
				 "cur_vm == NULL" );
		return;
	}

    if( Create_VM_From_Ui(&tmp_vm, cur_vm) == false )
        return;

	QString old_path = "";

    if( cur_vm->Get_Machine_Name() != tmp_vm.Get_Machine_Name() )
	{
		old_path = cur_vm->Get_VM_XML_File_Path();
	}

	// save all Settings
	disconnect( cur_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
				this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

    *cur_vm = tmp_vm;

	connect( cur_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
			 this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

	// save to file
	if( ! old_path.isEmpty() )
	{
		// Create new file name
		cur_vm->Set_VM_XML_File_Path( Get_Complete_VM_File_Path(cur_vm->Get_Machine_Name()) );
	}

	if( cur_vm->Save_VM() == false )
	{
		AQGraphic_Error( "void Main_Window::on_Button_Apply_clicked()",
						 tr("Error!"), tr("Cannot Save This VM to File!") );
		return;
	}
	else if( ! old_path.isEmpty() ) // OK New File Saved
	{
		if( ! QFile::remove( old_path ) )
		{
			AQWarning( "void Main_Window::on_Button_Apply_clicked()",
					   "Cannot Remove File: \"" + old_path + "\"" );
		}
	}

	// Set VM Name
	ui.Machines_List->currentItem()->setText( cur_vm->Get_Machine_Name() );

	Update_Info_Text();

	// Keep CPU / other fields in sync with what we just saved.
	{
		Block_VM_Changed_Signals bvmcs( this );
		ui.CB_CPU_Count->setEditText( QString::number( cur_vm->Get_SMP_CPU_Count() ) );
	}

	// For VM Changes Signals
	ui.Button_Apply->setEnabled( false );
	ui.Button_Cancel->setEnabled( false );
}

void Main_Window::on_Button_Cancel_clicked()
{
	// load Settings
	Update_VM_Ui();
}

void Main_Window::on_CH_Use_Network_toggled( bool on )
{
	Old_Network_Settings_Widget->Set_Enabled( on );
	New_Network_Settings_Widget->Set_Enabled( on );

	ui.Redirection_Widget->setEnabled( on );
	ui.Redirections_List->setEnabled( on );
	ui.Widget_Redirection_Buttons->setEnabled( on );
	ui.CH_Redirections->setEnabled( on );
}

void Main_Window::on_RB_Network_Mode_New_toggled( bool on )
{
	while( ui.Stack_Network_Basic_And_Native->count() > 0 )
		ui.Stack_Network_Basic_And_Native->removeWidget( ui.Stack_Network_Basic_And_Native->widget(0) );

	if( on )
		ui.Stack_Network_Basic_And_Native->insertWidget( 0, New_Network_Settings_Widget );
	else
		ui.Stack_Network_Basic_And_Native->insertWidget( 0, Old_Network_Settings_Widget );

	ui.Stack_Network_Basic_And_Native->setCurrentIndex( 0 );
}

void Main_Window::on_Redirections_List_cellClicked ( int row, int column )
{
	if( ui.Redirections_List->item( row, 0 )->text() == "TCP" ) ui.RB_TCP->setChecked( true );
	else ui.RB_UDP->setChecked( true );

	ui.SB_Redir_Port->setValue( ui.Redirections_List->item( row, 1 )->text().toInt() );
	ui.Edit_Guest_IP->setText( ui.Redirections_List->item( row, 2 )->text() );
	ui.SB_Guest_Port->setValue( ui.Redirections_List->item( row, 3 )->text().toInt() );
}

void Main_Window::on_Button_Add_Redirections_clicked()
{
	ui.Redirections_List->insertRow( ui.Redirections_List->rowCount() );
	ui.Redirections_List->setCurrentCell( ui.Redirections_List->rowCount()-1 , 0 );
	Update_Current_Redirection_Item();
}

void Main_Window::on_Button_Delete_Redirections_clicked()
{
	if( ui.Redirections_List->currentRow() > -1 )
		ui.Redirections_List->removeRow( ui.Redirections_List->currentRow() );
}

void Main_Window::Update_Current_Redirection_Item()
{
	// Port < 1024
	#ifndef Q_OS_WIN32
	if( ui.SB_Redir_Port->value() < 1024 &&
		Settings.value("Ignore_Redirection_Port_Varning", "no").toString() == "no" )
	{
		int ret = QMessageBox::question( this, tr("Warning!"),
										 tr("To Create Socket With Port Number < 1024, in Unix You Need to Run AQEMU in root Mode!\n"
											"Press \"Ignore\" button for hide this message in future.\nAdd This Record?"),
										 QMessageBox::Yes | QMessageBox::No | QMessageBox::Ignore, QMessageBox::Yes );

		if( ret == QMessageBox::No )
			return;
		else if( ret == QMessageBox::Ignore )
			Settings.setValue( "Ignore_Redirection_Port_Varning", "yes" );
	}
	#endif

	QTableWidgetItem *newItem;

	// protocol
	if( ui.RB_TCP->isChecked() ) newItem = new QTableWidgetItem( "TCP" );
	else newItem = new QTableWidgetItem( "UDP" );
	ui.Redirections_List->setItem( ui.Redirections_List->currentRow(), 0, newItem );

	// port
	newItem = new QTableWidgetItem( QString::number(ui.SB_Redir_Port->value()) );
	ui.Redirections_List->setItem( ui.Redirections_List->currentRow(), 1, newItem );

	// ip
	newItem = new QTableWidgetItem( ui.Edit_Guest_IP->text() );
	ui.Redirections_List->setItem( ui.Redirections_List->currentRow(), 2, newItem );

	// guest port
	newItem = new QTableWidgetItem( QString::number(ui.SB_Guest_Port->value()) );
	ui.Redirections_List->setItem( ui.Redirections_List->currentRow(), 3, newItem );
}

void Main_Window::on_Button_Clear_Redirections_clicked()
{
	while( ui.Redirections_List->currentRow() > -1 )
		ui.Redirections_List->removeRow( ui.Redirections_List->currentRow() );
}

void Main_Window::on_TB_Browse_SMB_clicked()
{
	QString SMB_Dir = QFileDialog::getExistingDirectory( this, tr("Select SMB Directory"),
														 Get_Last_Dir_Path(ui.Edit_SMB_Folder->text()),
														 QFileDialog::ShowDirsOnly );

	if( ! SMB_Dir.isEmpty() )
		ui.Edit_SMB_Folder->setText( QDir::toNativeSeparators(SMB_Dir) );
}

void Main_Window::on_TB_Browse_TFTP_clicked()
{
	QString TFTP_Dir = QFileDialog::getExistingDirectory( this, tr("Select TFTP Directory"),
														 Get_Last_Dir_Path(ui.Edit_TFTP_Prefix->text()),
														 QFileDialog::ShowDirsOnly );

	if( ! TFTP_Dir.isEmpty() )
		ui.Edit_TFTP_Prefix->setText( QDir::toNativeSeparators(TFTP_Dir) );
}


void Main_Window::adv_on_CH_Start_Date_toggled( bool on )
{
	if( on ) ui.CH_Local_Time->setChecked( false );
}

void Main_Window::on_TB_VNC_Unix_Socket_Browse_clicked()
{
	QString socketPath = QFileDialog::getOpenFileName( this, tr("UNIX Domain Socket Path"),
														Get_Last_Dir_Path(ui.Edit_Linux_bzImage_Path->text()),
														tr("All Files (*)") );

	if( ! socketPath.isEmpty() )
		ui.Edit_VNC_Unix_Socket->setText( QDir::toNativeSeparators(socketPath) );
}

void Main_Window::on_TB_x509_Browse_clicked()
{
	QString x509Dir = QFileDialog::getExistingDirectory( this, tr("Select x509 Certificate Folder"),
														 Get_Last_Dir_Path(ui.Edit_x509verify_Folder->text()),
														 QFileDialog::ShowDirsOnly );

	if( ! x509Dir.isEmpty() )
		ui.Edit_x509_Folder->setText( QDir::toNativeSeparators(x509Dir) );
}

void Main_Window::on_TB_x509verify_Browse_clicked()
{
	QString x509verifyDir = QFileDialog::getExistingDirectory( this, tr("Select x509 Verify Certificate Folder"),
															   Get_Last_Dir_Path(ui.Edit_x509verify_Folder->text()),
															   QFileDialog::ShowDirsOnly );

	if( ! x509verifyDir.isEmpty() )
		ui.Edit_x509verify_Folder->setText( QDir::toNativeSeparators(x509verifyDir) );
}

void Main_Window::on_TB_Linux_bzImage_SetPath_clicked()
{
	QString kernel = QFileDialog::getOpenFileName( this, tr("Select Kernel Image File"),
												   Get_Last_Dir_Path(ui.Edit_Linux_bzImage_Path->text()),
												   tr("All Files (*)") );

	if( ! kernel.isEmpty() )
		ui.Edit_Linux_bzImage_Path->setText( QDir::toNativeSeparators(kernel) );
}

void Main_Window::on_TB_Linux_Initrd_SetPath_clicked()
{
	QString initrd = QFileDialog::getOpenFileName( this, tr("Select InitRD File"),
												   Get_Last_Dir_Path(ui.Edit_Linux_Initrd_Path->text()),
												   tr("All Files (*)") );

	if( ! initrd.isEmpty() )
		ui.Edit_Linux_Initrd_Path->setText( QDir::toNativeSeparators(initrd) );
}

void Main_Window::on_TB_ROM_File_Browse_clicked()
{
	QString romFile = QFileDialog::getOpenFileName( this, tr("Select ROM File"),
													Get_Last_Dir_Path(ui.Edit_ROM_File->text()),
													tr("All Files (*)") );

	if( ! romFile.isEmpty() )
		ui.Edit_ROM_File->setText( QDir::toNativeSeparators(romFile) );
}

void Main_Window::on_TB_MTDBlock_File_Browse_clicked()
{
	QString mtd_file = QFileDialog::getOpenFileName( this, tr("Select On-Board Flash Image"),
													 Get_Last_Dir_Path(ui.Edit_MTDBlock_File->text()),
													 tr("All Files (*)") );

	if( ! mtd_file.isEmpty() )
		ui.Edit_MTDBlock_File->setText( QDir::toNativeSeparators(mtd_file) );
}

void Main_Window::on_TB_SD_Image_File_Browse_clicked()
{
	QString sd_file = QFileDialog::getOpenFileName( this, tr("Select SecureDigital Card Image"),
													Get_Last_Dir_Path(ui.Edit_SD_Image_File->text()),
													tr("All Files (*)") );

	if( ! sd_file.isEmpty() )
		ui.Edit_SD_Image_File->setText( QDir::toNativeSeparators(sd_file) );
}

void Main_Window::on_TB_PFlash_File_Browse_clicked()
{
	QString flash_file = QFileDialog::getOpenFileName( this, tr("Select Parallel Flash Image"),
													   Get_Last_Dir_Path(ui.Edit_PFlash_File->text()),
													   tr("All Files (*)") );

	if( ! flash_file.isEmpty() )
		ui.Edit_PFlash_File->setText( QDir::toNativeSeparators(flash_file) );
}

QString Main_Window::Copy_VM_Hard_Drive( const QString &vm_name, const QString &hd_name, const VM_HDD &hd )
{
	if( vm_name.isEmpty() )
	{
		AQError( "QString Main_Window::Copy_VM_Hard_Drive( const QString &vm_name, const QString &hd_name, const VM_HDD &hd )",
				 "vm_name is Empty!" );
		return "";
	}
	else
	{
		QString new_name = QDir::toNativeSeparators( Settings.value("VM_Directory", "~").toString() +
													 Get_FS_Compatible_VM_Name( vm_name ) + "_" + hd_name );

		if( QFile::exists(new_name) )
		{
			for( int ix = 0; ix < 1024; ++ix )
			{
				if( ! QFile::exists(new_name + "_" + QString::number(ix)) ) new_name += "_" + QString::number( ix );
			}
		}

		if( ! QFile::copy(hd.Get_File_Name(), new_name) )
		{
			AQError( "QString Main_Window::Copy_VM_Hard_Drive( const QString &vm_name, const QString &hd_name, const VM_HDD &hd )",
					 "Copy Error!" );
		}

		return new_name;
	}
}

QString Main_Window::Copy_VM_Floppy( const QString &vm_name, const QString &fd_name, const VM_Storage_Device &fd )
{
	if( vm_name.isEmpty() )
	{
		AQError( "QString Main_Window::Copy_VM_Floppy( const QString &vm_name, const QString &fd_name, const VM_Storage_Device &fd )",
				 "vm_name is Empty!" );
		return "";
	}
	else
	{
		QString new_name = QDir::toNativeSeparators( Settings.value("VM_Directory", "~").toString() +
													 Get_FS_Compatible_VM_Name(vm_name) + "_" + fd_name );

		if( QFile::exists(new_name) )
		{
			for( int ix = 0; ix < 1024; ++ix )
			{
				if( ! QFile::exists(new_name + "_" + QString::number(ix)) ) new_name += "_" + QString::number( ix );
			}
		}

		if( ! QFile::copy(fd.Get_File_Name(), new_name) )
		{
			AQError( "QString Main_Window::Copy_VM_Floppy( const QString &vm_name, const QString &fd_name, const VM_Storage_Device &fd )",
					 "Copy Error!" );
		}

		return new_name;
	}
}
