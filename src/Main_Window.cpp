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
#include <QListWidget>
#include <QListWidgetItem>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QApplication>
#include <QEventLoop>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QEvent>
#include <QWindowStateChangeEvent>
#include <QGuiApplication>
#include <QScreen>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QAbstractButton>
#include "iOS_Firmware_Tool_Window.h"
#include "Apple_SoC_Restore_Window.h"
#include "Apple_SoC_Support.h"
#include "WSL_Launch.h"
#include <QStyle>
#include <QFontMetrics>
#include <QLineEdit>
#include <QSpinBox>
#include <QClipboard>
#include <QScrollArea>
#include <QFrame>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QGroupBox>
#include <QTabBar>
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
#include "QEMU_Help_Browser.h"
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
#include "WSL_Wizard_Window.h"
#include "First_Start_Wizard.h"
#include "Emulator_Control_Window.h"
#include "Boot_Device_Window.h"
#include "SMP_Settings_Window.h"
#include "Settings_Widget.h"
#include "Storage_Browser_Window.h"
#include "Remote_Host_Window.h"
#include "AQ_UI_Style.h"
#include "Utils.h"
#include "QEMU_Probe_Catalog.h"
#include "Blockdev_Graph_Window.h"
#include "Service.h"
#include "No_Boot_Device.h"

// This is static emulator devices data
QMap<QString, Available_Devices> System_Info::Emulator_QEMU_2_0;


QList<VM_USB> System_Info::All_Host_USB;
QList<VM_USB> System_Info::Used_Host_USB;
QList<Host_GPU> System_Info::All_Host_GPU;
bool System_Info::Host_GPU_Scanned = false;

Main_Window::Main_Window( QWidget *parent )
	: QMainWindow( parent )
	, block_VM_changed_signals( true )
	, Main_Stack( nullptr )
	, Session_Widget( nullptr )
	, Session_VM( nullptr )
	, Session_Mode_Active( false )
	, Session_User_Detached( false )
	, Session_Block_During_Start( false )
	, GPU_Scan_Busy( false )
	, Edit_Apple_Trustcache( nullptr )
	, Edit_Apple_Ticket( nullptr )
	, Edit_Apple_SEP_FW( nullptr )
	, Edit_Apple_SEP_ROM( nullptr )
	, Edit_Apple_IPSW( nullptr )
	, Edit_Apple_USB_Conn_Addr( nullptr )
	, CB_Apple_USB_Conn_Type( nullptr )
	, SB_Apple_USB_Conn_Port( nullptr )
	, Tray_Icon( nullptr )
	, Act_Tray_Show( nullptr )
	, Act_Tray_Quit( nullptr )
	, Tray_Restore_Maximized( false )
	, Auto_Save_Timer( nullptr )
	, VM_Ui_Refresh_Timer( nullptr )
{
    Advanced_Options = new QDialog(this);
    Accelerator_Options = new QDialog(this);
    Architecture_Options = new QDialog(this);
    SMP_Settings = new SMP_Settings_Window(this);

	Auto_Save_Timer = new QTimer( this );
	Auto_Save_Timer->setSingleShot( true );
	Auto_Save_Timer->setInterval( 150 );
	connect( Auto_Save_Timer, SIGNAL(timeout()), this, SLOT(on_Button_Apply_clicked()) );

	// Coalesce rapid VM-list clicks so we don't rebuild the whole form per click.
	VM_Ui_Refresh_Timer = new QTimer( this );
	VM_Ui_Refresh_Timer->setSingleShot( true );
	VM_Ui_Refresh_Timer->setInterval( 60 );
	connect( VM_Ui_Refresh_Timer, &QTimer::timeout, this, [this]() { Update_VM_Ui(); } );

    ui.setupUi( this );
	ui_ao.setupUi( Advanced_Options );
	Build_Apple_SoC_Inferno_Ui();

	// Make the options body inside Advanced Options scrollable while keeping OK/Cancel fixed at the bottom
	if( ui_ao.groupBox_options && ui_ao.groupBox_options->parentWidget() )
	{
		QWidget *container = ui_ao.groupBox_options->parentWidget();
		AQ_Make_Tab_Scrollable( container, QStringLiteral( "AQ_Adv_Options_Inner" ) );
	}

	// File → Storage browser (VM_Directory pool)
	{
		QAction *actPool = new QAction( QIcon( ":/open-folder.png" ),
			tr( "Storage &Browser…" ), this );
		actPool->setStatusTip( tr( "Browse disk images and ISOs in your VM folder" ) );
		connect( actPool, &QAction::triggered, this, [this]() {
			Storage_Browser_Window dlg( this );
			dlg.exec();
		} );
		ui.menuFile->insertAction( ui.actionCreate_HDD_Image, actPool );
		ui.menuFile->insertSeparator( ui.actionCreate_HDD_Image );
	}
	// File → Remote hosts (SSH tunnels / libvirt helper for Xen·LXC)
	{
		QAction *actRemote = new QAction( QIcon( ":/preferences-system-network.png" ),
			tr( "&Remote Hosts…" ), this );
		actRemote->setStatusTip( tr(
			"SSH tunnels to remote QEMU, or open libvirt/Xen/LXC in virt-manager" ) );
		connect( actRemote, &QAction::triggered, this, [this]() {
			Remote_Host_Window dlg( this );
			dlg.exec();
		} );
		ui.menuFile->insertAction( ui.actionCreate_HDD_Image, actRemote );
	}
	// File → Configure WSL (distro + username)
	{
		QAction *actWsl = new QAction( QIcon( ":/configure.png" ),
			tr( "Configure &WSL…" ), this );
		actWsl->setStatusTip( tr( "Set WSL default distribution and username for KVM launches" ) );
		connect( actWsl, &QAction::triggered, this, [this]() {
			WSL_Wizard_Window wizard( this );
			wizard.exec();
		} );
		ui.menuFile->insertAction( ui.actionCreate_HDD_Image, actWsl );
	}
	// File → iOS Firmware Tool
	{
		QAction *actIosFw = new QAction( QIcon( QStringLiteral( ":/default_mac.png" ) ),
			tr( "iOS &Firmware Tool…" ), this );
		actIosFw->setStatusTip( tr( "Unpack IPSW archives and process IM4P payloads with pyimg4" ) );
		connect( actIosFw, &QAction::triggered, this, &Main_Window::slot_iOS_Firmware_Tool_triggered );
		ui.menuFile->insertAction( ui.actionCreate_HDD_Image, actIosFw );
	}
	// File → Apple SoC Restore (companion + idevicerestore)
	{
		QAction *actRestore = new QAction( QIcon( QStringLiteral( ":/default_mac.png" ) ),
			tr( "Apple SoC &Restore…" ), this );
		actRestore->setStatusTip( tr( "Companion VM helper and idevicerestore for Inferno iOS guests" ) );
		connect( actRestore, &QAction::triggered, this, &Main_Window::slot_Apple_SoC_Restore_triggered );
		ui.menuFile->insertAction( ui.actionCreate_HDD_Image, actRestore );
	}
	if( ui.actionCopy )
		ui.actionCopy->setText( tr( "Clone &VM…" ) );

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
#ifdef Q_OS_WIN32
	// Migrate existing installs off spice-client-glib (process crashes on channel errors).
	if( Settings.value( "Embedded_Display_Backend" ).toString().toLower() == QLatin1String( "spice" ) )
		Settings.setValue( "Embedded_Display_Backend", "vnc" );
#endif

    connect(ui_ao.CH_Start_Date,SIGNAL(toggled(bool)),this,SLOT(adv_on_CH_Start_Date_toggled(bool)));
	connect(ui_ao.TB_Refresh_Gamepads, SIGNAL(clicked()), this, SLOT(AO_Refresh_Gamepads_clicked()));
	connect(ui_ao.TB_Edit_Blockdev_Graph, SIGNAL(clicked()), this, SLOT(AO_Edit_Blockdev_Graph_clicked()));
	connect(ui_ao.TB_BIOS_File_Browse, &QToolButton::clicked, this, [this]() {
		const QString file = QFileDialog::getOpenFileName( this, tr( "Select BIOS / Board Firmware File" ),
			QString(), tr( "ROM / Firmware Files (*.bin *.rom *.fd *.elf);;All Files (*)" ) );
		if( ! file.isEmpty() )
			ui_ao.Edit_BIOS_File->setText( QDir::toNativeSeparators( file ) );
	} );

	ui_kvm.setupUi( Accelerator_Options );
	ui_arch.setupUi( Architecture_Options );

	// Combos: keep the closed field compact; show full names in the popup.
	// AdjustToContents + MinimumContentsLength(28) stole width from labels
	// ("Mouse device:" → "Mouse devic").
	auto fixComboElide = []( QComboBox *cb ) {
		if( ! cb ) return;
		cb->setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLengthWithIcon );
		cb->setMinimumContentsLength( 8 );
		cb->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		if( cb->view() )
		{
			cb->view()->setTextElideMode( Qt::ElideNone );
			cb->view()->setMinimumWidth( 280 );
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

	Polish_Settings_Tabs_Layout();

	Fill_Display_Resolution_Combo();
	Update_Display_Resolution_Enabled();
	Update_Display_Window_Mode_Hint();
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
    Display_Settings_Widget->setIconSize( AQ_Nav_Icon_Size( this ) );
    Display_Settings_Widget->addToGroup("Main");

	// Update Emulators Information
	System_Info::Update_VM_Computers_List();

	All_Emulators_List = Get_Emulators_List();

	GUI_User_Mode = true;
	Apply_Emulator( 0 );

	// Create Icon_Menu
	Icon_Menu = new QMenu( ui.Machines_List );

    Icon_Menu->addAction( ui.actionPower_On );
	Icon_Menu->addAction( ui.actionConnect_Session );
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
    Media_Settings_Widget->setIconSize( AQ_Nav_Icon_Size( this ) );
    Media_Settings_Widget->addToGroup("Main");
	

    //// code to sync sizes of widgets in Device Manager, Folder Sharing and Ports Tab Widget
    Folder_Sharing->syncLayout(Dev_Manager);
    Ports_Tab->syncLayout(Dev_Manager);
    ////

    Network_Settings_Widget = new Settings_Widget( ui.Network_Cards_Tabs, QBoxLayout::LeftToRight, true );
    Network_Settings_Widget->setIconSize( AQ_Nav_Icon_Size( this ) );
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

	Init_System_Tray();
#ifdef Q_OS_WIN32
	QTimer::singleShot( 0, this, [this]() { Maybe_Prompt_WSL_Config_On_Boot(); } );
#endif
}

void Main_Window::Init_System_Tray()
{
	if( Tray_Icon )
		return;

	if( ! QSystemTrayIcon::isSystemTrayAvailable() )
		return;

	Tray_Icon = new QSystemTrayIcon( this );
	QIcon icon = windowIcon();
	if( icon.isNull() )
		icon = QIcon( QStringLiteral( ":/aqemu.png" ) );
	Tray_Icon->setIcon( icon );
	Tray_Icon->setToolTip( QStringLiteral( "AQEMU" ) );

	QMenu *menu = new QMenu( this );
	Act_Tray_Show = menu->addAction( tr( "Show AQEMU" ), this, SLOT(On_Tray_Show()) );
	menu->addSeparator();
	Act_Tray_Quit = menu->addAction( tr( "Quit" ), this, SLOT(close()) );
	Tray_Icon->setContextMenu( menu );

	connect( Tray_Icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
	         this, SLOT(On_Tray_Activated(QSystemTrayIcon::ActivationReason)) );

	Update_System_Tray();
}

void Main_Window::Update_System_Tray()
{
	if( ! Tray_Icon )
		return;

	const bool enable = Settings.value( "Minimize_To_Tray", "yes" ).toString() == "yes";
	if( ! enable )
	{
		Tray_Icon->hide();
		if( ! isVisible() )
			showNormal();
		return;
	}

	// Icon stays ready; shown when we actually minimize.
	if( isMinimized() || ! isVisible() )
		Tray_Icon->show();
	else
		Tray_Icon->hide();
}

void Main_Window::On_Tray_Show()
{
	if( Tray_Restore_Maximized )
		showMaximized();
	else
		showNormal();
	raise();
	activateWindow();
	if( Tray_Icon )
		Tray_Icon->hide();
}

void Main_Window::On_Tray_Activated( QSystemTrayIcon::ActivationReason reason )
{
	if( reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick )
		On_Tray_Show();
}

void Main_Window::changeEvent( QEvent *event )
{
	if( event->type() == QEvent::WindowStateChange )
	{
		auto *se = static_cast<QWindowStateChangeEvent *>( event );
		if( Settings.value( "Minimize_To_Tray", "yes" ).toString() == "yes" &&
		    Tray_Icon && isMinimized() )
		{
			Tray_Restore_Maximized = se->oldState().testFlag( Qt::WindowMaximized );
			QTimer::singleShot( 0, this, SLOT(Hide_To_Tray()) );
		}
	}
	QMainWindow::changeEvent( event );
}

void Main_Window::Hide_To_Tray()
{
	if( Settings.value( "Minimize_To_Tray", "yes" ).toString() != "yes" )
		return;
	if( ! Tray_Icon )
		return;
	hide();
	Tray_Icon->show();
	Tray_Icon->showMessage( tr( "AQEMU" ),
	                        tr( "Running in the system tray." ),
	                        QSystemTrayIcon::Information, 2000 );
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

void Main_Window::Polish_Settings_Tabs_Layout()
{
	if( ui.Tabs )
	{
		ui.Tabs->setDocumentMode( false );
		ui.Tabs->setStyleSheet( QString() );
		ui.Tabs->setTabPosition( QTabWidget::West );
		// Rename VM → Machine so the West rail matches the section header language.
		const int gen_ix = ui.Tabs->indexOf( ui.Tab_General );
		if( gen_ix >= 0 )
			ui.Tabs->setTabText( gen_ix, tr( "Machine" ) );

		// Stock West tab bar — custom AQ_West_TabBar + QSS collapsed the rail to
		// zero width (stylesheet overrides tabSizeHint). Keep it native + visible.
		if( QTabBar *bar = ui.Tabs->tabBar() )
		{
			bar->setExpanding( false );
			bar->setDrawBase( true );
			bar->setStyleSheet( QString() );
			QStyle *st = style();
			const int icon = st ? st->pixelMetric( QStyle::PM_SmallIconSize, nullptr, this ) : 16;
			bar->setIconSize( QSize( icon, icon ) );
			// Ensure the rail has a readable thickness without QSS padding tricks.
			const int thick = qMax( icon + 16, QFontMetrics( bar->font() ).height() + 14 );
			bar->setMinimumWidth( thick );
			bar->show();
		}
		ui.Tabs->show();
	}

	// Soft floor so the layout can shrink; Media chip strip no longer locks width.
	setMinimumSize( 640, 480 );
	if( ui.splitter )
	{
		ui.splitter->setChildrenCollapsible( false );
		ui.splitter->setHandleWidth( qMax( 4, AQ_Px( 4, this ) ) );
	}
	if( ui.Machines_List )
		ui.Machines_List->setMinimumWidth( 160 );

	// Keep the Machine / Memory / Audio… section headers; tighten the page.
	if( ui.label )
		ui.label->show();

	if( ui.Tab_General && ui.Tab_General->layout() )
	{
		const int m = AQ_Px( 8, this );
		ui.Tab_General->layout()->setContentsMargins( m, AQ_Px( 6, this ), AQ_Px( 10, this ), m );
		ui.Tab_General->layout()->setSpacing( AQ_Px( 2, this ) );
		AQ_Tighten_Layout_Spacers( ui.Tab_General->layout() );
	}
	if( ui.Tab_Display && ui.Tab_Display->layout() )
	{
		const int m = AQ_Px( 8, this );
		ui.Tab_Display->layout()->setContentsMargins( m, m, AQ_Px( 10, this ), m );
		ui.Tab_Display->layout()->setSpacing( AQ_Px( 6, this ) );
	}
	if( ui.Tab_Media && ui.Tab_Media->layout() )
	{
		const int m = AQ_Px( 6, this );
		ui.Tab_Media->layout()->setContentsMargins( m, m, m, AQ_Px( 8, this ) );
		ui.Tab_Media->layout()->setSpacing( AQ_Px( 6, this ) );
	}
	if( ui.Tab_Network && ui.Tab_Network->layout() )
	{
		const int m = AQ_Px( 6, this );
		ui.Tab_Network->layout()->setContentsMargins( m, m, m, AQ_Px( 8, this ) );
		ui.Tab_Network->layout()->setSpacing( AQ_Px( 6, this ) );
	}
	if( ui.Tab_Info && ui.Tab_Info->layout() )
	{
		const int m = AQ_Px( 8, this );
		ui.Tab_Info->layout()->setContentsMargins( m, m, m, m );
		if( ui.VM_Information_Text )
		{
			ui.VM_Information_Text->setFrameShape( QFrame::StyledPanel );
			ui.VM_Information_Text->setStyleSheet(
				QStringLiteral(
					"QTextEdit {"
					"  border: 1px solid palette(mid);"
					"  border-radius: %1px;"
					"  background: palette(base);"
					"  padding: %2px;"
					"}" ).arg( AQ_Px( 6, this ) ).arg( AQ_Px( 8, this ) ) );
		}
	}

	// Cap readable column from font metrics so ultrawide doesn't leave desert gaps.
	// Machine (ui.widget) and every section below must share the full content width.
	if( ui.Memory_Size )
	{
		ui.Memory_Size->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		ui.Memory_Size->setMaximumHeight( QWIDGETSIZE_MAX );
		ui.Memory_Size->setMaximumWidth( QWIDGETSIZE_MAX );
	}
	if( ui.CB_RAM_Size )
	{
		ui.CB_RAM_Size->setMinimumWidth( AQ_Px( 110, this ) );
		ui.CB_RAM_Size->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	}

	// Expand horizontally; never shrink vertically below content (scroll instead).
	auto expand_section = []( QWidget *w ) {
		if( ! w ) return;
		w->setMaximumWidth( QWIDGETSIZE_MAX );
		w->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
		if( QLayout *lay = w->layout() )
			lay->setSizeConstraint( QLayout::SetMinimumSize );
	};
	expand_section( ui.widget );
	expand_section( ui.GB_Memory );
	expand_section( ui.GB_Audio );
	expand_section( ui.GB_Disk_Bus );
	expand_section( ui.GB_Win11_Lifecycle );
	expand_section( ui.GB_Intel_MacOS_Settings );
	expand_section( ui.GB_Options );
	expand_section( ui.Widget_Use_Network );
	expand_section( ui.GB_Guest_Display_Mode );

	if( ui.widget && ui.widget->layout() )
	{
		ui.widget->layout()->setContentsMargins( AQ_Px( 8, this ), AQ_Px( 4, this ),
			AQ_Px( 8, this ), AQ_Px( 4, this ) );
		ui.widget->layout()->setSpacing( AQ_Px( 6, this ) );
		if( QGridLayout *gl = qobject_cast<QGridLayout*>( ui.widget->layout() ) )
		{
			// gridLayout_12: 0=left fields, 1=5px gap, 2=right fields, 3=trailing spacer.
			// Stretch field columns — stretching col 1/3 left Machine bunched left.
			gl->setColumnStretch( 0, 1 );
			gl->setColumnStretch( 1, 0 );
			gl->setColumnStretch( 2, 1 );
			gl->setColumnStretch( 3, 0 );
			gl->setHorizontalSpacing( AQ_Px( 16, this ) );
		}
		if( ui.Widget_for_General_Tab )
		{
			ui.Widget_for_General_Tab->setMaximumWidth( AQ_Px( 12, this ) );
			ui.Widget_for_General_Tab->setMinimumWidth( AQ_Px( 8, this ) );
			ui.Widget_for_General_Tab->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
		}
		// Nested left/right grids: labels fixed, value columns expand.
		const auto nested = ui.widget->findChildren<QGridLayout *>();
		for( QGridLayout *sub : nested )
		{
			if( ! sub || sub == ui.widget->layout() ) continue;
			sub->setColumnStretch( 0, 0 );
			sub->setColumnStretch( 1, 1 );
			if( sub->columnCount() > 2 )
				sub->setColumnStretch( 2, 0 );
			sub->setHorizontalSpacing( AQ_Px( 8, this ) );
		}
		const auto labels = ui.widget->findChildren<QLabel *>();
		for( QLabel *lab : labels )
		{
			if( ! lab ) continue;
			lab->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred );
			lab->setMinimumWidth( lab->sizeHint().width() );
		}
		const auto combos = ui.widget->findChildren<QComboBox *>();
		for( QComboBox *cb : combos )
		{
			if( ! cb ) continue;
			cb->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
			cb->setSizeAdjustPolicy( QComboBox::AdjustToMinimumContentsLengthWithIcon );
			cb->setMinimumContentsLength( 6 );
		}
		const auto edits = ui.widget->findChildren<QLineEdit *>();
		for( QLineEdit *le : edits )
		{
			if( ! le ) continue;
			le->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		}
	}

	if( ui.TB_Show_Advanced_Options_Window )
	{
		ui.TB_Show_Advanced_Options_Window->setSizePolicy(
			QSizePolicy::Maximum, QSizePolicy::Fixed );
		ui.TB_Show_Advanced_Options_Window->setMinimumWidth( 0 );
		const int btn_h = style()->pixelMetric( QStyle::PM_ButtonDefaultIndicator, nullptr, this );
		Q_UNUSED( btn_h );
		ui.TB_Show_Advanced_Options_Window->setMinimumHeight(
			qMax( AQ_Px( 24, this ), fontMetrics().height() + AQ_Px( 8, this ) ) );
		ui.TB_Show_Advanced_Options_Window->setMaximumHeight( QWIDGETSIZE_MAX );
	}

	// Disk / Win11 action rows — expand controls so they aren't glued left.
	const int ctrl_h = qMax( AQ_Px( 28, this ), fontMetrics().height() + AQ_Px( 10, this ) );
	auto polish_btn = [ctrl_h]( QPushButton *b ) {
		if( ! b ) return;
		b->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		b->setMinimumHeight( ctrl_h );
		b->setMaximumHeight( QWIDGETSIZE_MAX );
	};
	polish_btn( ui.Button_Win11_Install );
	polish_btn( ui.Button_Win11_First_Boot );
	polish_btn( ui.Button_Win11_Normal );
	polish_btn( ui.Button_Win11_Repair );
	polish_btn( ui.Button_VirtIO_Defaults );
	if( ui.CB_Disk_Interface )
		ui.CB_Disk_Interface->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

	// Never allow checkboxes / radios on the Machine page to squash into unreadability.
	if( ui.Tab_General )
	{
		const auto boxes = ui.Tab_General->findChildren<QAbstractButton *>();
		for( QAbstractButton *b : boxes )
		{
			if( ! b ) continue;
			if( qobject_cast<QPushButton *>( b ) && ! qobject_cast<QToolButton *>( b ) )
				continue; // already polished above / size policy set
			b->setSizePolicy( b->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed );
			b->setMinimumHeight( qMax( b->minimumHeight(), fontMetrics().height() + AQ_Px( 4, this ) ) );
		}
	}

	// Trailing spacers in those rows should not steal all remaining width.
	auto soft_trailing_spacer = []( QLayout *lay ) {
		if( ! lay ) return;
		for( int i = 0; i < lay->count(); ++i )
		{
			QLayoutItem *it = lay->itemAt( i );
			if( ! it || ! it->spacerItem() ) continue;
			if( i == lay->count() - 1 )
				it->spacerItem()->changeSize( 8, 0, QSizePolicy::Minimum, QSizePolicy::Minimum );
		}
	};
	if( ui.GB_Disk_Bus )
		soft_trailing_spacer( ui.GB_Disk_Bus->layout() );
	if( ui.GB_Win11_Lifecycle )
	{
		if( QVBoxLayout *vl = qobject_cast<QVBoxLayout *>( ui.GB_Win11_Lifecycle->layout() ) )
		{
			for( int i = 0; i < vl->count(); ++i )
			{
				if( QLayout *sub = vl->itemAt( i )->layout() )
					soft_trailing_spacer( sub );
			}
		}
	}

	if( ui.GB_Options )
	{
		if( QGridLayout *gl = qobject_cast<QGridLayout*>( ui.GB_Options->layout() ) )
		{
			gl->setColumnStretch( 0, 1 );
			gl->setColumnStretch( 1, 1 );
			gl->setColumnStretch( 2, 1 );
			gl->setColumnStretch( 3, 1 );
		}
	}

	// White page behind Machine / Memory / … (avoid grey Window chrome bleed).
	auto paint_white = []( QWidget *w ) {
		if( ! w ) return;
		w->setAutoFillBackground( true );
		QPalette p = w->palette();
		p.setColor( QPalette::Window, Qt::white );
		p.setColor( QPalette::Base, Qt::white );
		w->setPalette( p );
	};
	paint_white( ui.centralwidget );
	paint_white( ui.Widget_for_Tabs );
	paint_white( ui.Tabs );
	paint_white( ui.Tab_General );
	paint_white( ui.Tab_Info );
	paint_white( ui.Tab_Media );
	paint_white( ui.Tab_Display );
	paint_white( ui.Tab_Network );
	paint_white( ui.Machines_List );

	auto polish_nested_tabs = []( QTabWidget *tw ) {
		if( ! tw ) return;
		tw->setDocumentMode( false );
		tw->setElideMode( Qt::ElideNone );
	};
	polish_nested_tabs( ui.TabWidget_Media );
	polish_nested_tabs( ui.TabWidget_Display );
	polish_nested_tabs( ui.Network_Cards_Tabs );

	if( ui.Machines_List )
	{
		ui.Machines_List->setSpacing( 2 );
		ui.Machines_List->setUniformItemSizes( true );
	}

	// Scroll when the Machine page is taller than the window — never crush controls.
	if( ui.Tab_General )
		AQ_Make_Tab_Scrollable( ui.Tab_General, QStringLiteral( "AQ_Machine_Tab_Inner" ) );
}

void Main_Window::Connect_Signals()
{
	// Refresh Info HTML when that tab becomes visible (deferred while on other tabs).
	connect( ui.Tabs, &QTabWidget::currentChanged, this, [this]( int ) {
		if( ui.Tabs->currentWidget() == ui.Tab_Info )
			Update_Info_Text();
	} );

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

	// Boot priority: only CB_Boot_Priority_currentIndexChanged — it updates
	// Boot_Order_List then calls VM_Changed(). A prior VM_Changed() here ran
	// with a stale list and could cancel a pending auto-save.
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
	connect( ui.Button_Win11_Repair, SIGNAL(clicked()),
			 this, SLOT(on_Button_Win11_Repair_clicked()) );
	connect( ui.TB_Intel_Mac_OpenCore_Browse_Main, SIGNAL(clicked()),
			 this, SLOT(on_TB_Intel_Mac_OpenCore_Browse_Main_clicked()) );
	connect( ui.TB_Intel_Mac_Recovery_Browse_Main, SIGNAL(clicked()),
			 this, SLOT(on_TB_Intel_Mac_Recovery_Browse_Main_clicked()) );
	connect( ui.Edit_Intel_Mac_OpenCore_Main, SIGNAL(textEdited(const QString &)),
			 this, SLOT(VM_Changed()) );
	connect( ui.Edit_Intel_Mac_Recovery_Main, SIGNAL(textEdited(const QString &)),
			 this, SLOT(VM_Changed()) );
	connect( ui.Edit_Intel_Mac_OSK_Main, SIGNAL(textEdited(const QString &)),
			 this, SLOT(VM_Changed()) );
	connect( ui.CH_Intel_Mac_WSL_Main, SIGNAL(toggled(bool)),
			 this, SLOT(Update_Intel_Mac_GPU_Passthrough_Ui()) );
	connect( ui.CH_Intel_Mac_WSL_Main, SIGNAL(toggled(bool)),
			 this, SLOT(Update_Machine_Accelerators()) );
	connect( ui.CH_Intel_Mac_WSL_Main, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );
	connect( ui.CH_Intel_Mac_GPU_Passthrough, SIGNAL(toggled(bool)),
			 this, SLOT(VM_Changed()) );
	connect( ui.Edit_Intel_Mac_GPU_Audio, SIGNAL(textEdited(const QString &)),
			 this, SLOT(VM_Changed()) );
	connect( ui.Edit_Intel_Mac_GPU_ROM, SIGNAL(textEdited(const QString &)),
			 this, SLOT(VM_Changed()) );
	connect( ui.TB_Intel_Mac_GPU_Refresh, SIGNAL(clicked()),
			 this, SLOT(on_TB_Intel_Mac_GPU_Refresh_clicked()) );
	connect( ui.TB_Intel_Mac_GPU_ROM_Browse, SIGNAL(clicked()),
			 this, SLOT(on_TB_Intel_Mac_GPU_ROM_Browse_clicked()) );
	connect( ui.CB_Intel_Mac_GPU, SIGNAL(currentIndexChanged(int)),
			 this, SLOT(on_CB_Intel_Mac_GPU_currentIndexChanged(int)) );

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
	connect( ui.RB_Display_Auto, SIGNAL(toggled(bool)),
			 this, SLOT(On_Display_Window_Mode_Toggled(bool)) );
	connect( ui.RB_Display_Embedded, SIGNAL(toggled(bool)),
			 this, SLOT(On_Display_Window_Mode_Toggled(bool)) );
	connect( ui.RB_Display_Native, SIGNAL(toggled(bool)),
			 this, SLOT(On_Display_Window_Mode_Toggled(bool)) );
	connect( ui.RB_Display_Nographic, SIGNAL(toggled(bool)),
			 this, SLOT(On_Display_Window_Mode_Toggled(bool)) );

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
	connect( ui_ao.CH_Launch_Via_WSL, SIGNAL(clicked()),
			 this, SLOT(Update_Machine_Accelerators()) );

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

	connect( ui.Edit_DeviceTree_Path, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_App_Kernel_Path, SIGNAL(textChanged(const QString &)),
			 this, SLOT(VM_Changed()) );

	connect( ui.Edit_App_Kernel_Args, SIGNAL(textChanged(const QString &)),
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

	// Find current device by UserRole target key first, fallback to System.Caption match
	const QString target_key = ui.CB_Computer_Type->currentData( Qt::UserRole ).toString();
	const QString target_text = ui.CB_Computer_Type->currentText();

	if( ! target_key.isEmpty() && allDevList.contains( target_key ) )
	{
		*ok = true;
		Available_Devices d = allDevList[ target_key ];
		if( System_Info::Emulator_QEMU_2_0.contains( target_key ) )
		{
			const Available_Devices &fb = System_Info::Emulator_QEMU_2_0[ target_key ];
			d.PSO_SMP_Count = qMax( d.PSO_SMP_Count, fb.PSO_SMP_Count );
			d.PSO_SMP_Cores = d.PSO_SMP_Cores || fb.PSO_SMP_Cores;
			d.PSO_SMP_Threads = d.PSO_SMP_Threads || fb.PSO_SMP_Threads;
			d.PSO_SMP_Sockets = d.PSO_SMP_Sockets || fb.PSO_SMP_Sockets;
			d.PSO_SMP_MaxCPUs = d.PSO_SMP_MaxCPUs || fb.PSO_SMP_MaxCPUs;
		}
		System_Info::Normalize_Virt_Arch_Devices( d );
		QEMU_Probe_Catalog::Merge_Into( d );
		System_Info::Filter_Video_Card_List( d );
		return d;
	}

	for( QMap<QString, Available_Devices>::const_iterator ix = allDevList.constBegin(); ix != allDevList.constEnd(); ++ix )
	{
		if( target_text == ix.value().System.Caption || (! target_key.isEmpty() && target_key == ix.key()) )
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
			QEMU_Probe_Catalog::Merge_Into( d );
			System_Info::Filter_Video_Card_List( d );
			return d;
		}
    }

	if( ! target_key.isEmpty() && System_Info::Emulator_QEMU_2_0.contains( target_key ) )
	{
		*ok = true;
		Available_Devices d = System_Info::Emulator_QEMU_2_0[ target_key ];
		System_Info::Normalize_Virt_Arch_Devices( d );
		QEMU_Probe_Catalog::Merge_Into( d );
		System_Info::Filter_Video_Card_List( d );
		return d;
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

	// Icon Path — never use display role 128 (may be a screenshot when Saved) (PR #1 / Qodo)
	{
		QString list_icon;
		for( int ix = 0; ix < ui.Machines_List->count(); ix++ )
		{
			if( ui.Machines_List->item(ix)->data(256).toString() == old_vm->Get_UID() )
			{
				list_icon = ui.Machines_List->item(ix)->data(257).toString();
				break;
			}
		}
		if( list_icon.isEmpty() )
			list_icon = old_vm->Get_Icon_Path();
		if( ! list_icon.isEmpty() &&
		    ( list_icon.startsWith( QLatin1String( ":/" ) ) || QFile::exists( list_icon ) ) )
		{
			tmp_vm->Set_Icon_Path( list_icon );
		}
		else if( ! old_vm->Get_Icon_Path().isEmpty() )
		{
			tmp_vm->Set_Icon_Path( old_vm->Get_Icon_Path() );
		}
		else
		{
			tmp_vm->Set_Icon_Path( QStringLiteral( ":/other.png" ) );
		}
	}

	// Get devices
	bool curMachineOk = false;
	Available_Devices curComp = Get_Current_Machine_Devices( &curMachineOk );
	if( ! curMachineOk )
    {
        if( show_user_errors )
        {
            AQGraphic_Error( "bool Main_Window::Create_VM_From_Ui",
                             tr("VM Save Error!"),
                             tr("Cannot retrieve device settings for selected target '%1'. "
                                "Check emulator installation under Advanced Settings.")
                                .arg( ui.CB_Computer_Type->currentText() ),
                             false );
        }
        return false;
    }

    // Machine Accelerator (prefer UserRole id over translated text)
	{
		const QVariant accel_data = ui.CB_Machine_Accelerator->currentData( Qt::UserRole );
		QString accel_id = accel_data.isValid() ? accel_data.toString().trimmed().toLower() : QString();
		if( accel_id.isEmpty() )
		{
			const QString caption = ui.CB_Machine_Accelerator->currentText().trimmed().toLower();
			if( caption.startsWith( QLatin1String( "tcg" ) ) || caption.contains( QLatin1String( "software" ) ) )
				accel_id = QStringLiteral( "tcg" );
			else if( caption.startsWith( QLatin1String( "kvm" ) ) || caption.contains( QLatin1String( "whpx" ) ) )
				accel_id = QStringLiteral( "kvm" );
			else if( caption.startsWith( QLatin1String( "xen" ) ) )
				accel_id = QStringLiteral( "xen" );
			else
				accel_id = caption;
		}
		tmp_vm->Set_Machine_Accelerator( VM::String_To_Accel( accel_id ) );
	}

	// Computer Type
	tmp_vm->Set_Computer_Type( curComp.System.QEMU_Name );

	// Machine Type — Match selected UI text / index against QEMU_Name / Caption
	{
		QString machine_name = ui.CB_Machine_Type_Main->currentText().trimmed();
		if( machine_name.isEmpty() )
			machine_name = ui_arch.CB_Machine_Type->currentText().trimmed();

		bool found = false;
		for( int mx = 0; mx < curComp.Machine_List.count(); ++mx )
		{
			if( curComp.Machine_List[mx].QEMU_Name == machine_name ||
			    curComp.Machine_List[mx].Caption == machine_name )
			{
				tmp_vm->Set_Machine_Type( curComp.Machine_List[mx].QEMU_Name );
				found = true;
				break;
			}
		}
		if( ! found && ! machine_name.isEmpty() )
			tmp_vm->Set_Machine_Type( machine_name );
		else if( ! found )
			tmp_vm->Set_Machine_Type( old_vm->Get_Machine_Type() );
	}

	// CPU Type — Match selected UI text / index against QEMU_Name / Caption
	{
		QString cpu_name = ui.CB_CPU_Type_Main->currentText().trimmed();
		if( cpu_name.isEmpty() )
			cpu_name = ui_arch.CB_CPU_Type->currentText().trimmed();

		bool found = false;
		for( int cx = 0; cx < curComp.CPU_List.count(); ++cx )
		{
			if( curComp.CPU_List[cx].QEMU_Name == cpu_name ||
			    curComp.CPU_List[cx].Caption == cpu_name )
			{
				tmp_vm->Set_CPU_Type( curComp.CPU_List[cx].QEMU_Name );
				found = true;
				break;
			}
		}
		if( ! found && ! cpu_name.isEmpty() )
			tmp_vm->Set_CPU_Type( cpu_name );
		else if( ! found )
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
	Boot_Order_List = VM::Expand_Boot_Order_List( Boot_Order_List );
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
	{
		const int ai = ui.CB_Audiodev_Backend->currentIndex();
		tmp_vm->Set_Audiodev_Backend( ai <= 0 ? QString() : ui.CB_Audiodev_Backend->currentText() );
		tmp_vm->Set_Audiodev_Timer_Period( ui.SB_Audiodev_Timer_Period->value() );
	}

	// Memory
	tmp_vm->Set_Memory_Size( ui.Memory_Size->value() );

	// Check free ram
	tmp_vm->Set_Remove_RAM_Size_Limitation( ui.CH_Remove_RAM_Size_Limitation->isChecked() );

	// Options
	tmp_vm->Use_Fullscreen_Mode( ui.CH_Fullscreen->isChecked() );
	tmp_vm->Use_Win2K_Hack( ui_ao.CH_Win2K_Hack->isChecked() );
	tmp_vm->Use_Local_Time( ui.CH_Local_Time->isChecked() );

	tmp_vm->Use_Check_FDD_Boot_Sector( ui_ao.CH_FDD_Boot->isChecked() );
	tmp_vm->Use_ACPI( ui.CH_Machine_ACPI->isChecked() );
	ui_ao.CH_ACPI->setChecked( ui.CH_Machine_ACPI->isChecked() );
	tmp_vm->Use_Force_TCG( ui_ao.CH_Force_TCG->isChecked() );
	tmp_vm->Use_Pass_Through_Gamepads( ui_ao.CH_Pass_Through_Gamepads->isChecked() );
	tmp_vm->Use_Emulate_USB_Gamepad( ui_ao.CH_Emulate_USB_Gamepad->isChecked() );
	tmp_vm->Use_No_Defaults( ui_ao.CH_No_Defaults->isChecked() );
	tmp_vm->Use_VirtIO_Balloon( ui_ao.CH_VirtIO_Balloon->isChecked() );
	tmp_vm->Use_VirtIO_RNG( ui_ao.CH_VirtIO_RNG->isChecked() );
	tmp_vm->Use_VirtIO_Keyboard( ui_ao.CH_VirtIO_Keyboard->isChecked() );
	{
		QStringList ids;
		for( int i = 0; i < ui_ao.LW_Gamepads->count(); ++i )
		{
			QListWidgetItem *it = ui_ao.LW_Gamepads->item( i );
			if( it && it->checkState() == Qt::Checked )
				ids << it->data( Qt::UserRole ).toString();
		}
		// If none checked but passthrough on → empty filter = all pads
		tmp_vm->Set_Gamepad_Filter_IDs( ids );
	}
	{
		const int idx = ui_ao.CB_RTC_Clock->currentIndex();
		if( idx == 1 ) tmp_vm->Set_RTC_Clock( QStringLiteral( "vm" ) );
		else if( idx == 2 ) tmp_vm->Set_RTC_Clock( QStringLiteral( "rt" ) );
		else tmp_vm->Set_RTC_Clock( QStringLiteral( "host" ) );
	}
	tmp_vm->Use_IOThread( ui_ao.CH_IOThread->isChecked() );
	tmp_vm->Use_Modern_Netdev( ui_ao.CH_Modern_Netdev->isChecked() );
	tmp_vm->Set_UUID( ui_ao.Edit_UUID->text() );
	tmp_vm->Set_BIOS_File( ui_ao.Edit_BIOS_File->text().trimmed() );
	tmp_vm->Set_Mem_Path( ui_ao.Edit_Mem_Path->text().trimmed() );
	tmp_vm->Use_Mem_Prealloc( ui_ao.CH_Mem_Prealloc->isChecked() );
	tmp_vm->Set_Machine_Extra_Props( ui_ao.Edit_Machine_Extra_Props->text() );
	tmp_vm->Use_NUMA( ui_ao.CH_NUMA->isChecked() );
	tmp_vm->Set_NUMA_Nodes( ui_ao.SB_NUMA_Nodes->value() );
	{
		const int wi = ui_ao.CB_Watchdog_Model->currentIndex();
		tmp_vm->Set_Watchdog_Model( wi <= 0 ? QString() : ui_ao.CB_Watchdog_Model->currentText() );
		tmp_vm->Set_Watchdog_Action( ui_ao.CB_Watchdog_Action->currentText() );
	}
	{
		const int ti = ui_ao.CB_TPM_Type->currentIndex();
		if( ti == 1 ) tmp_vm->Set_TPM_Type( QStringLiteral( "emulator" ) );
		else if( ti == 2 ) tmp_vm->Set_TPM_Type( QStringLiteral( "passthrough" ) );
		else tmp_vm->Set_TPM_Type( QStringLiteral( "none" ) );
		tmp_vm->Set_TPM_Path( ui_ao.Edit_TPM_Path->text().trimmed() );
	}
	tmp_vm->Use_Secret_Object( ui_ao.CH_Secret_Object->isChecked() );
	tmp_vm->Set_Secret_ID( ui_ao.Edit_Secret_ID->text() );
	tmp_vm->Set_Secret_File( ui_ao.Edit_Secret_File->text().trimmed() );
	tmp_vm->Set_Secret_Data( ui_ao.Edit_Secret_Data->text() );
	tmp_vm->Set_Incoming_URI( ui_ao.Edit_Incoming_URI->text() );
	tmp_vm->Use_Modern_Chardev( ui_ao.CH_Modern_Chardev->isChecked() );
	tmp_vm->Use_Blockdev( ui_ao.CH_Use_Blockdev->isChecked() );
	tmp_vm->Set_Blockdev_Extra_Lines( AO_Blockdev_Extra_Lines );
	tmp_vm->Use_NUMA_Memdev( ui_ao.CH_NUMA_Memdev->isChecked() );
	tmp_vm->Use_SMBIOS_Type0( ui_ao.CH_SMBIOS_Type0->isChecked() );
	tmp_vm->Set_SMBIOS_Vendor( ui_ao.Edit_SMBIOS_Vendor->text() );
	tmp_vm->Set_SMBIOS_Version( ui_ao.Edit_SMBIOS_Version->text() );
	tmp_vm->Set_SMBIOS_Date( ui_ao.Edit_SMBIOS_Date->text() );
	tmp_vm->Use_SMBIOS_Type1( ui_ao.CH_SMBIOS_Type1->isChecked() );
	tmp_vm->Set_SMBIOS_Manufacturer( ui_ao.Edit_SMBIOS_Manufacturer->text() );
	tmp_vm->Set_SMBIOS_Product( ui_ao.Edit_SMBIOS_Product->text() );
	tmp_vm->Set_SMBIOS_Type1_Version( ui_ao.Edit_SMBIOS_Type1_Version->text() );
	tmp_vm->Set_SMBIOS_Serial( ui_ao.Edit_SMBIOS_Serial->text() );
	tmp_vm->Set_SMBIOS_File( ui_ao.Edit_SMBIOS_File->text().trimmed() );
	tmp_vm->Set_FW_CFG_Lines( ui_ao.Edit_FW_CFG_Lines->toPlainText() );
	tmp_vm->Set_ICount( ui_ao.Edit_ICount->text() );
	tmp_vm->Set_Sandbox( ui_ao.Edit_Sandbox->text() );
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
				case 4: native.Set_Interface( VM::DI_AHCI ); break;
				case 5: native.Set_Interface( VM::DI_SD ); break;
				case 6: native.Set_Interface( VM::DI_NVMe ); break;
				default: native.Set_Interface( VM::DI_Virtio ); break;
			}
			// Clamp to what this arch/machine supports
			{
				bool dok = false;
				const Available_Devices ddev = Get_Current_Machine_Devices( &dok );
				const QString computer = dok ? ddev.System.QEMU_Name : QString();
				const QString machine = ui.CB_Machine_Type_Main->currentText();
				if( ! computer.isEmpty() )
					native.Set_Interface( System_Info::Sanitize_Disk_Bus(
						computer, machine, native.Get_Interface(), false ) );
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
	tmp_vm->Set_DeviceTree_Path( ui.Edit_DeviceTree_Path->text() );
	tmp_vm->Set_App_Kernel_Path( ui.Edit_App_Kernel_Path->text() );
	// Prefer authoritative Apple SoC markers on tmp_vm (computer/machine type), not widget
	// visibility which may lag until Update_DeviceTree_Visibility runs.
	if( Uses_Apple_SoC_Boot_UI( tmp_vm ) )
		tmp_vm->Set_Kernel_ComLine( ui.Edit_App_Kernel_Args->text() );
	else
		tmp_vm->Set_Kernel_ComLine( ui.Edit_Linux_Command_Line->text() );
	Apply_Apple_SoC_Fields_To_VM( tmp_vm );

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

	// UEFI extras (wizard/advanced); VirtIO balloon/RNG/keyboard come from Advanced Options UI above
	tmp_vm->Use_UEFI( old_vm->Use_UEFI() );
	tmp_vm->Set_UEFI_CODE_File( old_vm->Get_UEFI_CODE_File() );
	tmp_vm->Set_UEFI_VARS_File( old_vm->Get_UEFI_VARS_File() );
	tmp_vm->Use_USB_Hub( old_vm->Use_USB_Hub() );
	tmp_vm->Set_Win11_Lifecycle_Mode( old_vm->Get_Win11_Lifecycle_Mode() );
	// Honor the checkbox — do not OR with old_vm (that made Intel Mac / WSL sticky forever).
	// Reims computer type still implies Intel macOS OpenCore profile (not display-name heuristics).
	const bool is_reims_vm = old_vm &&
		old_vm->Get_Computer_Type().contains( QLatin1String( "reimsvgpu" ), Qt::CaseInsensitive );
	tmp_vm->Use_Intel_MacOS_Profile( ui_ao.CH_Intel_MacOS_Profile->isChecked() || is_reims_vm );
	{
		// Prefer main VM-page Intel macOS fields when that section is shown; else Advanced Options;
		// finally preserve wizard-set values from old_vm.
		QString osk = ui.Edit_Intel_Mac_OSK_Main->text();
		if( osk.trimmed().isEmpty() )
			osk = ui_ao.Edit_Apple_SMC_OSK->text();
		if( osk.trimmed().isEmpty() )
			osk = old_vm->Get_Apple_SMC_OSK();

		QString oc = AQ_Normalize_File_Path( ui.Edit_Intel_Mac_OpenCore_Main->text() );
		if( oc.isEmpty() )
			oc = AQ_Normalize_File_Path( ui_ao.Edit_OpenCore_Boot_Path->text() );
		if( oc.isEmpty() )
			oc = old_vm->Get_OpenCore_Boot_Path();

		QString recovery = AQ_Normalize_File_Path( ui.Edit_Intel_Mac_Recovery_Main->text() );
		if( recovery.isEmpty() )
			recovery = old_vm->Get_Mac_Recovery_Image_Path();

		// WSL launch is opt-in from visible UI only — never sticky-OR old_vm (that forced
		// accel=kvm on Mac OS X PPC and other TCG guests after one accidental enable).
		const bool wsl = ui.CH_Intel_Mac_WSL_Main->isChecked() ||
		                 ui_ao.CH_Launch_Via_WSL->isChecked();

		tmp_vm->Set_Apple_SMC_OSK( osk );
		tmp_vm->Set_OpenCore_Boot_Path( oc );
		tmp_vm->Set_Mac_Recovery_Image_Path( recovery );
		tmp_vm->Use_Apple_SMC( tmp_vm->Use_Intel_MacOS_Profile() && ! osk.trimmed().isEmpty() );
		tmp_vm->Use_Launch_Via_WSL( wsl );

		if( ui.GB_Intel_Mac_GPU_Passthrough->isVisible() )
		{
			const bool can_pass = ui.CH_Intel_Mac_GPU_Passthrough->isEnabled();
			if( can_pass )
			{
				tmp_vm->Use_GPU_Passthrough( ui.CH_Intel_Mac_GPU_Passthrough->isChecked() );
				tmp_vm->Set_GPU_PCI_Address( ui.CB_Intel_Mac_GPU->currentData().toString() );
				tmp_vm->Set_GPU_Audio_PCI_Address( ui.Edit_Intel_Mac_GPU_Audio->text() );
				tmp_vm->Set_GPU_ROM_File( ui.Edit_Intel_Mac_GPU_ROM->text() );
				tmp_vm->Use_GPU_Passthrough_Multifunction( true );
			}
			else
			{
				// Windows/WSL: controls disabled — preserve saved passthrough config (PR #2 / Qodo)
				tmp_vm->Use_GPU_Passthrough( old_vm->Use_GPU_Passthrough() );
				tmp_vm->Set_GPU_PCI_Address( old_vm->Get_GPU_PCI_Address() );
				tmp_vm->Set_GPU_Audio_PCI_Address( old_vm->Get_GPU_Audio_PCI_Address() );
				tmp_vm->Set_GPU_ROM_File( old_vm->Get_GPU_ROM_File() );
				tmp_vm->Use_GPU_Passthrough_Multifunction( old_vm->Use_GPU_Passthrough_Multifunction() );
			}
		}
		else
		{
			// No AMD on this host — preserve settings from another machine / prior save
			tmp_vm->Use_GPU_Passthrough( old_vm->Use_GPU_Passthrough() );
			tmp_vm->Set_GPU_PCI_Address( old_vm->Get_GPU_PCI_Address() );
			tmp_vm->Set_GPU_Audio_PCI_Address( old_vm->Get_GPU_Audio_PCI_Address() );
			tmp_vm->Set_GPU_ROM_File( old_vm->Get_GPU_ROM_File() );
			tmp_vm->Use_GPU_Passthrough_Multifunction( old_vm->Use_GPU_Passthrough_Multifunction() );
		}

		// Keep Advanced Options widgets in sync
		ui_ao.Edit_Apple_SMC_OSK->setText( osk );
		ui_ao.Edit_OpenCore_Boot_Path->setText( oc );
		ui_ao.CH_Launch_Via_WSL->setChecked( wsl );
		ui_ao.CH_Intel_MacOS_Profile->setChecked( tmp_vm->Use_Intel_MacOS_Profile() );
	}
	if( tmp_vm->Use_Intel_MacOS_Profile() )
	{
		tmp_vm->Use_UEFI( true );
		if( ! ui_ao.Edit_UEFI_CODE_File->text().trimmed().isEmpty() )
			tmp_vm->Set_UEFI_CODE_File( ui_ao.Edit_UEFI_CODE_File->text().trimmed() );
		else
			tmp_vm->Set_UEFI_CODE_File( old_vm->Get_UEFI_CODE_File() );
		if( ! ui_ao.Edit_UEFI_VARS_File->text().trimmed().isEmpty() )
			tmp_vm->Set_UEFI_VARS_File( ui_ao.Edit_UEFI_VARS_File->text().trimmed() );
		else
			tmp_vm->Set_UEFI_VARS_File( old_vm->Get_UEFI_VARS_File() );
	}

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

	// Guest display: embedded SPICE/VNC vs separate QEMU SDL/GTK window
	if( ui.RB_Display_Nographic->isChecked() )
	{
		tmp_vm->Set_Display_Window_Mode( QStringLiteral( "native" ) );
		tmp_vm->Set_Display_Backend( QStringLiteral( "nographic" ) );
	}
	else if( ui.RB_Display_Embedded->isChecked() )
	{
		tmp_vm->Set_Display_Window_Mode( QStringLiteral( "embedded" ) );
		tmp_vm->Set_Display_Backend( QString() );
	}
	else if( ui.RB_Display_Native->isChecked() )
	{
		tmp_vm->Set_Display_Window_Mode( QStringLiteral( "native" ) );
		const int bi = ui.CB_Display_Backend->currentIndex();
		static const char *backends[] = { "", "sdl", "gtk", "none", "curses", "egl-headless" };
		tmp_vm->Set_Display_Backend( bi >= 0 && bi < 6 ? QString::fromLatin1( backends[bi] ) : QString() );
	}
	else
	{
		tmp_vm->Set_Display_Window_Mode( QStringLiteral( "auto" ) );
		tmp_vm->Set_Display_Backend( QString() );
	}

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
	// Main Window Size — clamp to the available screen so a prior maximized
	// save cannot leave the window stuck at "whole desktop" normal size.
	int w = Settings.value( "General_Window_Width", 885 ).toInt();
	int h = Settings.value( "General_Window_Height", 544 ).toInt();
	QPoint pos = Settings.value( "General_Window_Position", QPoint( 300, 300 ) ).toPoint();

	QScreen *screen = QGuiApplication::screenAt( pos );
	if( ! screen )
		screen = QGuiApplication::primaryScreen();
	if( screen )
	{
		const QRect avail = screen->availableGeometry();
		w = qBound( 640, w, avail.width() );
		h = qBound( 480, h, avail.height() );
		if( ! avail.contains( pos ) )
		{
			pos.setX( qBound( avail.left(), pos.x(), avail.right() - w ) );
			pos.setY( qBound( avail.top(), pos.y(), avail.bottom() - h ) );
		}
	}
	else
	{
		w = qMax( 640, w );
		h = qMax( 480, h );
	}

	resize( w, h );
	move( pos );

	// Toolbar State
	restoreState( Settings.value("General_Window_State").toByteArray());

	// Session mode hides these; that visibility must never stick after restart.
	ui.Tool_Bar_VM_Manage->setVisible( true );
	ui.Tool_Bar_VM_Control->setVisible( true );

	// Splitter
	ui.splitter->restoreState( Settings.value("General_Splitter",
							   QByteArray("\0\0\0\xff\0\0\0\0\0\0\0\x2\0\0\0\xbc\0\0\x2$\0\0\0\0\x4\x1\0\0\0\x1")).toByteArray() );

	if( Settings.value( "General_Window_Maximized", false ).toBool() )
		showMaximized();

	// VM Icons Size — user override if set, else host DPI / style metric.
	{
		const QSize dpi_icon = AQ_Vm_List_Icon_Size( this );
		const int sz = Settings.contains( QStringLiteral( "VM_Icons_Size" ) )
			? Settings.value( QStringLiteral( "VM_Icons_Size" ) ).toInt()
			: dpi_icon.width();
		ui.Machines_List->setIconSize( QSize( sz, sz ) );
	}

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

	// Persist the restored (non-maximized) geometry so Maximize does not
	// permanently inflate the next normal session size.
	const QRect geo = normalGeometry();
	Settings.setValue( "General_Window_Width", QString::number( geo.width() ) );
	Settings.setValue( "General_Window_Height", QString::number( geo.height() ) );
	Settings.setValue( "General_Window_Position", geo.topLeft() );
	Settings.setValue( "General_Window_Maximized",
		isMaximized() || ( ! isVisible() && Tray_Restore_Maximized ) );

	// Save Toolbar State — never persist session-hidden left bars
	const bool manage_vis = ui.Tool_Bar_VM_Manage->isVisible();
	const bool control_vis = ui.Tool_Bar_VM_Control->isVisible();
	ui.Tool_Bar_VM_Manage->setVisible( true );
	ui.Tool_Bar_VM_Control->setVisible( true );
	Settings.setValue( "General_Window_State", saveState() );
	ui.Tool_Bar_VM_Manage->setVisible( manage_vis );
	ui.Tool_Bar_VM_Control->setVisible( control_vis );

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
			item->setData( 257, new_vm->Get_Icon_Path() );

			// Load OS Logo or OS Screenshot Icon
			if( new_vm->Get_State() == VM::VMS_Saved &&
				Settings.value("Use_Screenshot_for_OS_Logo", "yes").toString() == "yes" )
			{
				// Screenshot File Not Found? Use OS Icon.
				if( QFile::exists(new_vm->Get_Screenshot_Path()) )
				{
					item->setIcon( QIcon(new_vm->Get_Screenshot_Path()) );
					item->setData( 128, new_vm->Get_Screenshot_Path() );
				}
				else
				{
					item->setIcon( QIcon(new_vm->Get_Icon_Path()) );
					item->setData( 128, new_vm->Get_Icon_Path() );
				}
			}
			else
			{
				item->setIcon( QIcon(new_vm->Get_Icon_Path()) );
				item->setData( 128, new_vm->Get_Icon_Path() );
			}

			// Append new VM
			VM_List << new_vm;

		}

		++real_index;
	}

	AQEMU_Startup_Log(
		QStringLiteral( "VMs loaded: %1" ).arg( VM_List.count() ) );

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

void Main_Window::Schedule_Update_VM_Ui()
{
	if( VM_Ui_Refresh_Timer )
		VM_Ui_Refresh_Timer->start();
	else
		Update_VM_Ui();
}

void Main_Window::Update_VM_Ui(bool update_info_tab)
{
	// Do not sync dirty-state on exit — that re-ran Create_VM_From_Ui and often
	// falsely enabled Apply + auto-save after every list selection.
	Block_VM_Changed_Signals bvmcs( this, false );

	setUpdatesEnabled( false );

	Update_VM_Port_Number();

	if( ui.Machines_List->currentRow() < 0 )
	{
		AQWarning( "void Main_Window::Update_VM_Ui()",
				   "VM Index Out of Range" );
		setUpdatesEnabled( true );
		return;
	}

	Virtual_Machine *tmp_vm = Get_Current_VM();

	if( tmp_vm == NULL )
	{
		AQError( "void Main_Window::Update_VM_Ui()",
				 "Cannot Find VM!" );
		setUpdatesEnabled( true );
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
		setUpdatesEnabled( true );
		return;
	}

    int found = false;
	const QString want_accel = VM::Accel_To_String( tmp_vm->Get_Machine_Accelerator() ).toLower();
	ui.CB_Machine_Accelerator->blockSignals( true );
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
	ui.CB_Machine_Accelerator->blockSignals( false );

	Enforce_Accel_Honesty();
	Update_Computer_Types();

	// Get current VM devices
	Available_Devices curComp = tmp_vm->Get_Emulator().Get_Devices()[ tmp_vm->Get_Computer_Type() ];
	if( curComp.System.QEMU_Name.isEmpty() && System_Info::Emulator_QEMU_2_0.contains( tmp_vm->Get_Computer_Type() ) )
	{
		curComp = System_Info::Emulator_QEMU_2_0[ tmp_vm->Get_Computer_Type() ];
	}
	else if( System_Info::Emulator_QEMU_2_0.contains( tmp_vm->Get_Computer_Type() ) )
	{
		const Available_Devices &fb = System_Info::Emulator_QEMU_2_0[ tmp_vm->Get_Computer_Type() ];
		curComp.PSO_SMP_Count = qMax( curComp.PSO_SMP_Count, fb.PSO_SMP_Count );
		curComp.PSO_SMP_Cores = curComp.PSO_SMP_Cores || fb.PSO_SMP_Cores;
		curComp.PSO_SMP_Threads = curComp.PSO_SMP_Threads || fb.PSO_SMP_Threads;
		curComp.PSO_SMP_Sockets = curComp.PSO_SMP_Sockets || fb.PSO_SMP_Sockets;
		curComp.PSO_SMP_MaxCPUs = curComp.PSO_SMP_MaxCPUs || fb.PSO_SMP_MaxCPUs;
	}
	System_Info::Normalize_Virt_Arch_Devices( curComp );
	QEMU_Probe_Catalog::Merge_Into( curComp );
	System_Info::Filter_Video_Card_List( curComp );

	if( curComp.System.QEMU_Name.isEmpty() )
	{
		AQError( "void Main_Window::Update_VM_Ui()",
				 "cur_comp not valid!" );
		setUpdatesEnabled( true );
		return;
	}

	// Computer Type
	ui.CB_Computer_Type->blockSignals( true );
	ui.CB_Machine_Type_Main->blockSignals( true );
	ui_arch.CB_Machine_Type->blockSignals( true );
	ui.CB_CPU_Type_Main->blockSignals( true );
	ui_arch.CB_CPU_Type->blockSignals( true );

	int compTypeIndex = ui.CB_Computer_Type->findData( tmp_vm->Get_Computer_Type(), Qt::UserRole );
	if( compTypeIndex == -1 )
		compTypeIndex = ui.CB_Computer_Type->findText( curComp.System.Caption );
	if( compTypeIndex == -1 && ! curComp.System.QEMU_Name.isEmpty() )
		compTypeIndex = ui.CB_Computer_Type->findText( curComp.System.QEMU_Name );

	if( compTypeIndex != -1 )
		ui.CB_Computer_Type->setCurrentIndex( compTypeIndex );
	else
	{
		ui.CB_Computer_Type->blockSignals( false );
		ui.CB_Machine_Type_Main->blockSignals( false );
		ui_arch.CB_Machine_Type->blockSignals( false );
		ui.CB_CPU_Type_Main->blockSignals( false );
		ui_arch.CB_CPU_Type->blockSignals( false );
		AQError( "void Main_Window::Update_VM_Ui()",
				 "Cannot find computer type index!" );
		setUpdatesEnabled( true );
		return;
	}

	// Populate Machine & CPU comboboxes for current architecture
	ui_arch.CB_CPU_Type->clear();
	ui.CB_CPU_Type_Main->clear();
	ui_arch.CB_Machine_Type->clear();
	ui.CB_Machine_Type_Main->clear();

	QStringList cpu_items, machine_items;
	for( int i = 0; i < curComp.CPU_List.count(); ++i )
		cpu_items << curComp.CPU_List[i].Caption;
	for( int i = 0; i < curComp.Machine_List.count(); ++i )
		machine_items << curComp.Machine_List[i].Caption;

	ui_arch.CB_CPU_Type->addItems( cpu_items );
	ui.CB_CPU_Type_Main->addItems( cpu_items );
	ui_arch.CB_Machine_Type->addItems( machine_items );
	ui.CB_Machine_Type_Main->addItems( machine_items );

	// Machine Type
	QString tmp_str = tmp_vm->Get_Machine_Type();
	bool machine_found = false;
	for( int mx = 0; mx < curComp.Machine_List.count(); ++mx )
	{
		if( tmp_str == curComp.Machine_List[mx].QEMU_Name || tmp_str == curComp.Machine_List[mx].Caption )
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
		if( tmp_str == curComp.CPU_List[cx].QEMU_Name || tmp_str == curComp.CPU_List[cx].Caption )
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

	ui.CB_Computer_Type->blockSignals( false );
	ui.CB_Machine_Type_Main->blockSignals( false );
	ui_arch.CB_Machine_Type->blockSignals( false );
	ui.CB_CPU_Type_Main->blockSignals( false );
	ui_arch.CB_CPU_Type->blockSignals( false );

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

	// Boot — expand truncated lists so the combo can select any device type
	Boot_Order_List = VM::Expand_Boot_Order_List( tmp_vm->Get_Boot_Order_List() );
	Set_Boot_Order( Boot_Order_List );
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
	{
		const QString ab = tmp_vm->Get_Audiodev_Backend();
		int ai = 0;
		if( ! ab.isEmpty() )
		{
			ai = ui.CB_Audiodev_Backend->findText( ab );
			if( ai < 0 ) ai = 0;
		}
		ui.CB_Audiodev_Backend->setCurrentIndex( ai );
		ui.SB_Audiodev_Timer_Period->setValue( tmp_vm->Get_Audiodev_Timer_Period() );
	}

	// Disk bus (HDA)
	{
		// Default IDE — VirtIO is invisible to XP/OS/2/ReactOS/DOS installers.
		// (Old default was VirtIO; wizard Apply then rewrote every new VM to VirtIO.)
		int disk_idx = 3;
		if( tmp_vm->Get_HDA().Get_Enabled() && tmp_vm->Get_HDA().Get_Native_Mode() )
		{
			switch( tmp_vm->Get_HDA().Get_Native_Device().Get_Interface() )
			{
				case VM::DI_Virtio: disk_idx = 0; break;
				case VM::DI_Virtio_SCSI: disk_idx = 1; break;
				case VM::DI_SCSI: disk_idx = 2; break;
				case VM::DI_IDE: disk_idx = 3; break;
				case VM::DI_AHCI: disk_idx = 4; break;
				case VM::DI_SD: disk_idx = 5; break;
				case VM::DI_NVMe: disk_idx = 6; break;
				default: disk_idx = 3; break;
			}
		}
		else if( tmp_vm->Get_Computer_Type().contains( QLatin1String( "aarch64" ), Qt::CaseInsensitive ) ||
		         tmp_vm->Get_Computer_Type().contains( QLatin1String( "qemu-system-arm" ), Qt::CaseInsensitive ) ||
		         tmp_vm->Get_Machine_Type().compare( QLatin1String( "virt" ), Qt::CaseInsensitive ) == 0 )
		{
			disk_idx = 0; // virt machines have no IDE
		}
		ui.CB_Disk_Interface->setCurrentIndex( disk_idx );
		Enforce_Disk_Bus_Honesty();
	}

	// RAM — clamp silently while switching VMs (no popup thrash).
	if( tmp_vm->Get_Memory_Size() < 1 )
		ui.Memory_Size->setValue( 256 );
	else if( tmp_vm->Get_Memory_Size() >= ui.Memory_Size->maximum() )
		ui.Memory_Size->setValue( ui.Memory_Size->maximum() );
	else ui.Memory_Size->setValue( tmp_vm->Get_Memory_Size() );

	ui.CH_Remove_RAM_Size_Limitation->setChecked( tmp_vm->Get_Remove_RAM_Size_Limitation() );
	on_CH_Remove_RAM_Size_Limitation_stateChanged( ui.CH_Remove_RAM_Size_Limitation->checkState() );

	// General Tab. Options
	ui.CH_Fullscreen->setChecked( tmp_vm->Use_Fullscreen_Mode() );
	ui.CH_Machine_ACPI->setChecked( tmp_vm->Use_ACPI() );
	ui_ao.CH_ACPI->setChecked( tmp_vm->Use_ACPI() );
	ui_ao.CH_Force_TCG->setChecked( tmp_vm->Use_Force_TCG() );
	ui_ao.CH_Pass_Through_Gamepads->setChecked( tmp_vm->Use_Pass_Through_Gamepads() );
	ui_ao.CH_Emulate_USB_Gamepad->setChecked( tmp_vm->Use_Emulate_USB_Gamepad() );
	ui_ao.CH_No_Defaults->setChecked( tmp_vm->Use_No_Defaults() );
	ui_ao.CH_VirtIO_Balloon->setChecked( tmp_vm->Use_VirtIO_Balloon() );
	ui_ao.CH_VirtIO_RNG->setChecked( tmp_vm->Use_VirtIO_RNG() );
	ui_ao.CH_VirtIO_Keyboard->setChecked( tmp_vm->Use_VirtIO_Keyboard() );
	{
		const QString clk = tmp_vm->Get_RTC_Clock().trimmed().toLower();
		if( clk == QLatin1String( "vm" ) ) ui_ao.CB_RTC_Clock->setCurrentIndex( 1 );
		else if( clk == QLatin1String( "rt" ) ) ui_ao.CB_RTC_Clock->setCurrentIndex( 2 );
		else ui_ao.CB_RTC_Clock->setCurrentIndex( 0 );
	}
	ui_ao.CH_IOThread->setChecked( tmp_vm->Use_IOThread() );
	ui_ao.CH_Modern_Netdev->setChecked( tmp_vm->Use_Modern_Netdev() );
	ui_ao.Edit_UUID->setText( tmp_vm->Get_UUID() );
	ui_ao.Edit_BIOS_File->setText( tmp_vm->Get_BIOS_File() );
	ui_ao.Edit_Mem_Path->setText( tmp_vm->Get_Mem_Path() );
	ui_ao.CH_Mem_Prealloc->setChecked( tmp_vm->Use_Mem_Prealloc() );
	ui_ao.Edit_Machine_Extra_Props->setText( tmp_vm->Get_Machine_Extra_Props() );
	ui_ao.CH_NUMA->setChecked( tmp_vm->Use_NUMA() );
	ui_ao.SB_NUMA_Nodes->setValue( tmp_vm->Get_NUMA_Nodes() );
	{
		const QString wm = tmp_vm->Get_Watchdog_Model();
		int wi = 0;
		if( wm == QLatin1String( "i6300esb" ) ) wi = 1;
		else if( wm == QLatin1String( "ib700" ) ) wi = 2;
		ui_ao.CB_Watchdog_Model->setCurrentIndex( wi );
		const QString wa = tmp_vm->Get_Watchdog_Action();
		int ai = ui_ao.CB_Watchdog_Action->findText( wa );
		ui_ao.CB_Watchdog_Action->setCurrentIndex( ai >= 0 ? ai : 0 );
	}
	{
		const QString tt = tmp_vm->Get_TPM_Type();
		if( tt == QLatin1String( "emulator" ) ) ui_ao.CB_TPM_Type->setCurrentIndex( 1 );
		else if( tt == QLatin1String( "passthrough" ) ) ui_ao.CB_TPM_Type->setCurrentIndex( 2 );
		else ui_ao.CB_TPM_Type->setCurrentIndex( 0 );
		ui_ao.Edit_TPM_Path->setText( tmp_vm->Get_TPM_Path() );
	}
	ui_ao.CH_Secret_Object->setChecked( tmp_vm->Use_Secret_Object() );
	ui_ao.Edit_Secret_ID->setText( tmp_vm->Get_Secret_ID() );
	ui_ao.Edit_Secret_File->setText( tmp_vm->Get_Secret_File() );
	ui_ao.Edit_Secret_Data->setText( tmp_vm->Get_Secret_Data() );
	ui_ao.Edit_Incoming_URI->setText( tmp_vm->Get_Incoming_URI() );
	ui_ao.CH_Modern_Chardev->setChecked( tmp_vm->Use_Modern_Chardev() );
	ui_ao.CH_Use_Blockdev->setChecked( tmp_vm->Use_Blockdev() );
	AO_Blockdev_Extra_Lines = tmp_vm->Get_Blockdev_Extra_Lines();
	ui_ao.CH_NUMA_Memdev->setChecked( tmp_vm->Use_NUMA_Memdev() );
	ui_ao.CH_SMBIOS_Type0->setChecked( tmp_vm->Use_SMBIOS_Type0() );
	ui_ao.Edit_SMBIOS_Vendor->setText( tmp_vm->Get_SMBIOS_Vendor() );
	ui_ao.Edit_SMBIOS_Version->setText( tmp_vm->Get_SMBIOS_Version() );
	ui_ao.Edit_SMBIOS_Date->setText( tmp_vm->Get_SMBIOS_Date() );
	ui_ao.CH_SMBIOS_Type1->setChecked( tmp_vm->Use_SMBIOS_Type1() );
	ui_ao.Edit_SMBIOS_Manufacturer->setText( tmp_vm->Get_SMBIOS_Manufacturer() );
	ui_ao.Edit_SMBIOS_Product->setText( tmp_vm->Get_SMBIOS_Product() );
	ui_ao.Edit_SMBIOS_Type1_Version->setText( tmp_vm->Get_SMBIOS_Type1_Version() );
	ui_ao.Edit_SMBIOS_Serial->setText( tmp_vm->Get_SMBIOS_Serial() );
	ui_ao.Edit_SMBIOS_File->setText( tmp_vm->Get_SMBIOS_File() );
	ui_ao.Edit_FW_CFG_Lines->setPlainText( tmp_vm->Get_FW_CFG_Lines() );
	ui_ao.Edit_ICount->setText( tmp_vm->Get_ICount() );
	ui_ao.Edit_Sandbox->setText( tmp_vm->Get_Sandbox() );
	Refresh_Gamepad_List( tmp_vm->Get_Gamepad_Filter_IDs() );

	ui_ao.CH_Intel_MacOS_Profile->setChecked( tmp_vm->Use_Intel_MacOS_Profile() );
	ui_ao.Edit_OpenCore_Boot_Path->setText( tmp_vm->Get_OpenCore_Boot_Path() );
	ui_ao.Edit_Apple_SMC_OSK->setText( tmp_vm->Get_Apple_SMC_OSK() );
	ui_ao.Edit_UEFI_CODE_File->setText( tmp_vm->Get_UEFI_CODE_File() );
	ui_ao.Edit_UEFI_VARS_File->setText( tmp_vm->Get_UEFI_VARS_File() );
	ui_ao.CH_Launch_Via_WSL->setChecked( tmp_vm->Use_Launch_Via_WSL() );
	{
		const bool is_apple_soc =
			tmp_vm->Get_Computer_Type().contains( QLatin1String( "applesoc" ), Qt::CaseInsensitive ) ||
			tmp_vm->Get_Machine_Name().contains( QLatin1String( "Apple Silicon" ), Qt::CaseInsensitive ) ||
			tmp_vm->Get_Machine_Name().contains( QLatin1String( "iOS" ), Qt::CaseInsensitive );
		ui_ao.GB_Intel_MacOS->setVisible( ! is_apple_soc && (
			tmp_vm->Use_Intel_MacOS_Profile() ||
			tmp_vm->Get_Machine_Name().contains( "macOS", Qt::CaseInsensitive ) ||
			tmp_vm->Get_Machine_Name().contains( "Mac OS X", Qt::CaseInsensitive ) ||
			tmp_vm->Get_Machine_Name().contains( "Darwin", Qt::CaseInsensitive ) ) );
	}

	ui.Edit_Intel_Mac_OpenCore_Main->setText( tmp_vm->Get_OpenCore_Boot_Path() );
	ui.Edit_Intel_Mac_Recovery_Main->setText( tmp_vm->Get_Mac_Recovery_Image_Path() );
	ui.Edit_Intel_Mac_OSK_Main->setText( tmp_vm->Get_Apple_SMC_OSK() );
	ui.CH_Intel_Mac_WSL_Main->setChecked( tmp_vm->Use_Launch_Via_WSL() );
	ui.CH_Intel_Mac_GPU_Passthrough->setChecked( tmp_vm->Use_GPU_Passthrough() );
	ui.Edit_Intel_Mac_GPU_Audio->setText( tmp_vm->Get_GPU_Audio_PCI_Address() );
	ui.Edit_Intel_Mac_GPU_ROM->setText( tmp_vm->Get_GPU_ROM_File() );
	Update_Intel_MacOS_Settings_Ui();
	// Select saved GPU BDF after combo is populated
	{
		const int ix = ui.CB_Intel_Mac_GPU->findData( tmp_vm->Get_GPU_PCI_Address() );
		if( ix >= 0 )
			ui.CB_Intel_Mac_GPU->setCurrentIndex( ix );
	}

	// Repair blank/missing icons for Intel macOS VMs (old absolute paths were
	// mangled on load by prepending AQEMU_Data_Folder).
	if( tmp_vm->Use_Intel_MacOS_Profile() ||
	    tmp_vm->Get_Machine_Name().contains( QLatin1String( "macOS" ), Qt::CaseInsensitive ) ||
	    tmp_vm->Get_Machine_Name().contains( QLatin1String( "Mac OS" ), Qt::CaseInsensitive ) )
	{
		const QString ic = tmp_vm->Get_Icon_Path();
		const bool missing =
			ic.isEmpty() ||
			ic.endsWith( QLatin1String( "other.png" ) ) ||
			( ! ic.startsWith( QLatin1String( ":/" ) ) && ! QFile::exists( ic ) ) ||
			QIcon( ic ).isNull();
		if( missing )
		{
			const QString mac_icon = QStringLiteral( ":/default_macos.png" );
			tmp_vm->Set_Icon_Path( mac_icon );
			if( ui.Machines_List->currentItem() )
			{
				ui.Machines_List->currentItem()->setIcon( QIcon( mac_icon ) );
				ui.Machines_List->currentItem()->setData( 128, mac_icon );
				ui.Machines_List->currentItem()->setData( 257, mac_icon );
			}
			tmp_vm->Save_VM();
		}
	}
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

	// Guest display chrome
	{
		const QString m = tmp_vm->Get_Display_Window_Mode().trimmed().toLower();
		const QString db = tmp_vm->Get_Display_Backend().trimmed().toLower();
		ui.RB_Display_Auto->blockSignals( true );
		ui.RB_Display_Embedded->blockSignals( true );
		ui.RB_Display_Native->blockSignals( true );
		ui.RB_Display_Nographic->blockSignals( true );
		if( db == QLatin1String( "nographic" ) )
			ui.RB_Display_Nographic->setChecked( true );
		else if( m == QLatin1String( "embedded" ) )
			ui.RB_Display_Embedded->setChecked( true );
		else if( m == QLatin1String( "native" ) )
			ui.RB_Display_Native->setChecked( true );
		else
			ui.RB_Display_Auto->setChecked( true );
		ui.RB_Display_Auto->blockSignals( false );
		ui.RB_Display_Embedded->blockSignals( false );
		ui.RB_Display_Native->blockSignals( false );
		ui.RB_Display_Nographic->blockSignals( false );
		int bi = 0;
		if( db == QLatin1String( "sdl" ) ) bi = 1;
		else if( db == QLatin1String( "gtk" ) ) bi = 2;
		else if( db == QLatin1String( "none" ) ) bi = 3;
		else if( db == QLatin1String( "curses" ) ) bi = 4;
		else if( db == QLatin1String( "egl-headless" ) ) bi = 5;
		ui.CB_Display_Backend->setCurrentIndex( bi );
		Update_Display_Window_Mode_Hint();
	}

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
	ui.Edit_DeviceTree_Path->setText( tmp_vm->Get_DeviceTree_Path() );
	ui.Edit_App_Kernel_Path->setText( tmp_vm->Get_App_Kernel_Path() );
	ui.Edit_App_Kernel_Args->setText( tmp_vm->Get_Kernel_ComLine() );
	ui.Edit_Linux_Command_Line->setText( tmp_vm->Get_Kernel_ComLine() );
	Load_Apple_SoC_Fields_From_VM( tmp_vm );

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

	// Skip heavy HTML Info rebuild unless that tab is visible.
	if( update_info_tab && ui.Tabs && ui.Tabs->currentWidget() == ui.Tab_Info )
		Update_Info_Text();
	Update_Win11_Lifecycle_Ui();
	Update_Intel_MacOS_Settings_Ui();
	Update_DeviceTree_Visibility();
	Update_Disabled_Controls(); // FIXME

	// CPU count AFTER Update_Disabled_Controls — that rebuilds the combo and used to wipe this to 1.
	ui.CB_CPU_Count->setEditText( QString::number( tmp_vm->Get_SMP_CPU_Count() ) );
	SMP_Settings->Set_Values( tmp_vm->Get_SMP(), curComp.PSO_SMP_Count, curComp.PSO_SMP_Cores,
							  curComp.PSO_SMP_Threads, curComp.PSO_SMP_Sockets, curComp.PSO_SMP_MaxCPUs );

	// For VM Changes Signals
	ui.Button_Apply->setEnabled( false );
	ui.Button_Cancel->setEnabled( false );

	setUpdatesEnabled( true );
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
		QSet<int> added;
		for( int cx = 1; cx <= 16 && cx <= curComp.PSO_SMP_Count; ++cx )
		{
			ui.CB_CPU_Count->addItem( QString::number(cx) );
			added.insert( cx );
		}
		for( int cx = 32; (cx - 1) <= curComp.PSO_SMP_Count; cx *= 2 )
		{
			if( ! added.contains( cx ) )
			{
				if( cx == 256 ) ui.CB_CPU_Count->addItem( QString::number(255) );
				else ui.CB_CPU_Count->addItem( QString::number(cx) );
				added.insert( cx );
			}
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

	// Always leave the guest view when this VM stops — even if the list has no
	// selection (Get_Current_VM() can be null while session mode is active).
	if( Settings.value( "Embedded_Session", "yes" ).toString() == "yes" )
	{
		if( s == VM::VMS_Power_Off || s == VM::VMS_Saved || s == VM::VMS_In_Error )
		{
			if( Session_Mode_Active && Session_VM == vm )
				Exit_Session_Mode();
			// Allow the next Start to auto-open the session again.
			if( s == VM::VMS_Power_Off || s == VM::VMS_In_Error )
				Session_User_Detached = false;
		}
		else if( ( s == VM::VMS_Running || s == VM::VMS_Pause ) &&
		         ! vm->Prefer_Native_VGA_Window() )
		{
			// Never attach SPICE/VNC while Start() is still inside the modal
			// "Please wait" dialog — waitForStarted() pumps events and nested
			// spice-glib teardown was crashing the whole app.
			if( ! Session_Block_During_Start )
			{
				// Refresh attach while viewing this VM (e.g. Preparing → Running).
				// Do not auto-reopen after the user explicitly left with Exit view.
				if( Session_Mode_Active && Session_VM == vm )
					Enter_Session_Mode( vm );
				else if( ! Session_Mode_Active && ! Session_User_Detached )
					Enter_Session_Mode( vm );
			}
		}
	}

	Virtual_Machine *cur_vm = Get_Current_VM();
	if( cur_vm == NULL )
	{
		Show_State_VM( vm );
		vm->Save_VM();
		Update_Connect_Action();
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
	Update_Connect_Action();
}

void Main_Window::Enter_Session_Mode( Virtual_Machine *vm )
{
	if( ! vm || ! Session_Widget || ! Main_Stack )
		return;

	Session_User_Detached = false;
	Session_VM = vm;
	Session_Mode_Active = true;
	setWindowTitle( tr( "AQEMU – %1" ).arg( vm->Get_Machine_Name() ) );

	// Session view owns all runtime controls on its top toolbar — hide left chrome
	ui.Tool_Bar_VM_Manage->setVisible( false );
	ui.Tool_Bar_VM_Control->setVisible( false );

	const int vnc_tcp = vm->Get_Embedded_VNC_Port() > 0
		? vm->Get_Embedded_VNC_Port()
		: ( vm->Get_Embedded_Display_Port() + Settings.value( "First_VNC_Port", "5910" ).toString().toInt() );
	QString backend = Settings.value( "Embedded_Display_Backend", "vnc" ).toString();
#ifdef Q_OS_WIN32
	if( backend.toLower() == QLatin1String( "spice" ) )
		backend = QStringLiteral( "vnc" );
#endif

	Session_Widget->Attach_VM( vm, vm->Get_QMP(),
	                           QStringLiteral( "127.0.0.1" ),
	                           vm->Get_Embedded_Spice_Port(),
	                           vnc_tcp,
	                           backend );
	Main_Stack->setCurrentWidget( Session_Widget );
	Update_Connect_Action();
}

void Main_Window::Enter_Session_Mode_Preparing( Virtual_Machine *vm )
{
	if( ! vm || ! Session_Widget || ! Main_Stack )
		return;

	Session_User_Detached = false;
	Session_VM = vm;
	Session_Mode_Active = true;
	setWindowTitle( tr( "AQEMU – %1" ).arg( vm->Get_Machine_Name() ) );

	ui.Tool_Bar_VM_Manage->setVisible( false );
	ui.Tool_Bar_VM_Control->setVisible( false );

	QString backend = Settings.value( "Embedded_Display_Backend", "vnc" ).toString();
#ifdef Q_OS_WIN32
	if( backend.toLower() == QLatin1String( "spice" ) )
		backend = QStringLiteral( "vnc" );
#endif
	// Ports are allocated during Start — show the session shell first so QEMU
	// does not race ahead of the UI.
	Session_Widget->Attach_VM( vm, nullptr,
	                           QStringLiteral( "127.0.0.1" ),
	                           0, 0, backend );
	Main_Stack->setCurrentWidget( Session_Widget );
	QApplication::processEvents( QEventLoop::ExcludeUserInputEvents );
	Update_Connect_Action();
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
	Update_Connect_Action();
}

void Main_Window::On_Session_Exit_View()
{
	// Leave session UI without forcing power-off.
	// Only remember "user left the view" when the guest is still running so
	// Connect stays available and Start does not immediately steal focus again.
	if( Session_VM &&
		( Session_VM->Get_State() == VM::VMS_Running ||
		  Session_VM->Get_State() == VM::VMS_Pause ) )
		Session_User_Detached = true;
	else
		Session_User_Detached = false;

	Exit_Session_Mode();
}

void Main_Window::Update_Connect_Action()
{
	Virtual_Machine *vm = Get_Current_VM();
	const bool can_connect =
		vm &&
		Settings.value( "Embedded_Session", "yes" ).toString() == "yes" &&
		! vm->Prefer_Native_VGA_Window() &&
		( vm->Get_State() == VM::VMS_Running || vm->Get_State() == VM::VMS_Pause ) &&
		!( Session_Mode_Active && Session_VM == vm );

	ui.actionConnect_Session->setEnabled( can_connect );
}

void Main_Window::on_actionConnect_Session_triggered()
{
	Virtual_Machine *vm = Get_Current_VM();
	if( ! vm )
		return;

	if( vm->Get_State() != VM::VMS_Running && vm->Get_State() != VM::VMS_Pause )
	{
		AQGraphic_Warning( tr( "Connect" ),
						   tr( "This virtual machine is not running. Use Start first." ) );
		return;
	}

	if( Settings.value( "Embedded_Session", "yes" ).toString() != "yes" )
	{
		AQGraphic_Warning( tr( "Connect" ),
						   tr( "Embedded session display is disabled in AQEMU settings." ) );
		return;
	}

	// Switching from another guest view, or reconnecting after Exit view.
	if( Session_Mode_Active && Session_VM != vm )
		Exit_Session_Mode();

	Session_User_Detached = false;
	Enter_Session_Mode( vm );
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
	// Role 257 = persisted icon path (never the screenshot) (PR #1 / Qodo)
	ui.Machines_List->currentItem()->setData( 257, vm->Get_Icon_Path() );

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
	Update_Connect_Action();
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
	ui.CB_Display_Resolution->addItem( QStringLiteral( "3840 × 2160 (4K UHD)" ), QStringLiteral( "3840x2160" ) );
	ui.CB_Display_Resolution->addItem( QStringLiteral( "4096 × 2160 (4K DCI)" ), QStringLiteral( "4096x2160" ) );
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
	Virtual_Machine *vm = Get_Current_VM();
	const bool intel_mac = vm && vm->Use_Intel_MacOS_Profile();
	const bool mac_vga = intel_mac && (
		video.contains( QStringLiteral( "vmware" ), Qt::CaseInsensitive ) ||
		video.contains( QStringLiteral( "std" ), Qt::CaseInsensitive ) ||
		video.isEmpty() );
	// Respect parent tab enablement (disabled while VM is running).
	const bool parent_ok = ui.Tab_General->isEnabled();
	const bool enable = ( virtio || mac_vga ) && parent_ok;
	ui.CB_Display_Resolution->setEnabled( enable );
	ui.Label_Display_Resolution->setEnabled( enable );
	if( intel_mac )
	{
		ui.CB_Display_Resolution->setToolTip( tr(
			"Intel macOS: AQEMU patches OpenCore UEFI Resolution to this size on start "
			"(System Settings rarely lists modes). Native uses your host physical pixels "
			"(DPI-aware), up to 4096×2160. Reboot the guest after changing." ) );
	}
}

void Main_Window::On_Display_Window_Mode_Toggled( bool on )
{
	if( ! on )
		return;
	Update_Display_Window_Mode_Hint();
	VM_Changed();
}

void Main_Window::Update_Display_Window_Mode_Hint()
{
	if( ! ui.Label_Display_Window_Mode_Hint )
		return;

	if( ui.RB_Display_Embedded->isChecked() )
	{
		ui.Label_Display_Window_Mode_Hint->setText( tr(
			"Next Start opens the guest inside AQEMU. Use this to leave a separate SDL window." ) );
	}
	else if( ui.RB_Display_Nographic->isChecked() )
	{
		ui.Label_Display_Window_Mode_Hint->setText( tr(
			"Next Start uses -nographic (serial console only). Connect via the monitor/serial tab or an external terminal." ) );
	}
	else if( ui.RB_Display_Native->isChecked() )
	{
		ui.Label_Display_Window_Mode_Hint->setText( tr(
			"Next Start opens a separate QEMU window. Pick sdl/gtk/none/curses under “QEMU -display backend”." ) );
	}
	else
	{
		ui.Label_Display_Window_Mode_Hint->setText( tr(
			"Auto: DOS / Win9x / XP-family get a separate QEMU window; other VMs use the embedded viewer. "
			"Network disks: Device Manager file path can be iscsi://, rbd:, nbd:, gluster, or ssh://." ) );
	}
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

        // Incomplete Create_VM_From_Ui must not mark dirty / schedule save —
        // that would race Apply with a half-built VM and drop edits.
        if( ! Create_VM_From_Ui( tmp_vm, old_vm, false ) )
        {
            delete tmp_vm;
            return;
        }

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

	Virtual_Machine *old_vm = Get_VM_By_UID( previous->data(256).toString() );

	if( old_vm == NULL )
	{
		AQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous )",
				 "old_vm == NULL" );
		return;
	}

	// Skip expensive Create_VM_From_Ui when there are no pending edits.
	if( ui.Button_Apply->isEnabled() && old_vm->Get_State() != VM::VMS_In_Error )
	{
		Virtual_Machine tmp_vm;
		if( Create_VM_From_Ui( &tmp_vm, old_vm ) == false )
		{
			AQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
					 "Cannot Create VM! Discarding UI changes for previous VM." );
		}
		else if( *old_vm != tmp_vm )
		{
			if( Auto_Save_Timer )
				Auto_Save_Timer->stop();

			disconnect( old_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
						this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

			*old_vm = tmp_vm;

			connect( old_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
					 this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

			old_vm->Save_VM();
		}
	}

	if( ui.Machines_List->row(current) >= 0 &&
		ui.Machines_List->row(current) < ui.Machines_List->count() )
	{
		Schedule_Update_VM_Ui();
	}
	else
	{
		AQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
				 "Index Invalid!" );
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
	Q_UNUSED( item );
	Virtual_Machine *cur_vm = Get_Current_VM();

	if( cur_vm == NULL )
	{
		AQError( "void Main_Window::on_Machines_List_itemDoubleClicked( QListWidgetItem *item )",
				 "cur_vm == NULL" );
		return;
	}

	// Running guests: double-click reconnects to the embedded display.
	if( cur_vm->Get_State() == VM::VMS_Running || cur_vm->Get_State() == VM::VMS_Pause )
	{
		on_actionConnect_Session_triggered();
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

	// Apple SoC / iOS: require a kernel and DeviceTree (do not skip all boot checks).
	if( tmp_vm->Get_Computer_Type().contains( QLatin1String( "applesoc" ), Qt::CaseInsensitive ) ||
	    tmp_vm->Get_Machine_Type().contains( QLatin1String( "t8030" ), Qt::CaseInsensitive ) ||
	    tmp_vm->Get_Machine_Type().contains( QLatin1String( "s8000" ), Qt::CaseInsensitive ) )
	{
		const QString kernel = tmp_vm->Get_App_Kernel_Path().trimmed();
		const QString dtb = tmp_vm->Get_DeviceTree_Path().trimmed();

		if( kernel.isEmpty() || ! QFile::exists( kernel ) )
		{
			AQGraphic_Warning( tr( "Error!" ),
				tr( "Apple SoC / iOS VMs need a valid kernelcache path "
				    "(DeviceTree / Kernel section)." ) );
			return false;
		}
		if( dtb.isEmpty() || ! QFile::exists( dtb ) )
		{
			AQGraphic_Warning( tr( "Error!" ),
				tr( "Apple SoC / iOS VMs need a valid DeviceTree path "
				    "(.dtb or extracted payload)." ) );
			return false;
		}
		if( dtb.endsWith( QLatin1String( ".im4p" ), Qt::CaseInsensitive ) &&
		    ! AQ_Is_Apple_SoC_VM( tmp_vm ) )
		{
			AQGraphic_Warning( tr( "Error!" ),
				tr( "DeviceTree is still an .im4p payload. Extract/decrypt it to a "
				    "raw .dtb (or .dec) with the iOS Firmware Tool before starting.\n\n"
				    "Inferno Apple SoC guests may use .im4p DeviceTrees with the full "
				    "trustcache / SEP recipe." ) );
			return false;
		}
		// Disk image is optional at first boot (kernel+dtb only), but warn if missing.
		return true;
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
			ui.Machines_List->currentItem()->setData( 257, icon_win.Get_New_Icon_Path() );
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


void Main_Window::on_actionQEMU_Help_Browser_triggered()
{
	QEMU_Help_Browser dlg( this );
	dlg.exec();
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
	VM_Wizard_Window *Wizard_Win = new VM_Wizard_Window( this );
	Wizard_Win->Set_VM_List( &VM_List );

	if( ! Wizard_Win )
		return;

	if( Wizard_Win->exec() == QDialog::Accepted )
	{
		Virtual_Machine *vm = Wizard_Win->New_VM;
		vm->Set_UID( QUuid::createUuid().toString() ); // Create UID
		VM_List << vm;

		QObject::connect( vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
						  this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

		QListWidgetItem *item = new QListWidgetItem( vm->Get_Machine_Name(), ui.Machines_List );
		item->setIcon( QIcon(vm->Get_Icon_Path()) );
		item->setData( 256, vm->Get_UID() );
		item->setData( 128, vm->Get_Icon_Path() );
		item->setData( 257, vm->Get_Icon_Path() );

		ui.Machines_List->setCurrentItem( item );

		Update_VM_Ui();

		// Sync Advanced Options widgets from the new VM before Apply, so Apply
		// cannot wipe wizard-only fields (OpenCore / OSK) that were not on-screen yet.
		ui_ao.CH_Intel_MacOS_Profile->setChecked( vm->Use_Intel_MacOS_Profile() );
		ui_ao.Edit_OpenCore_Boot_Path->setText( vm->Get_OpenCore_Boot_Path() );
		ui_ao.Edit_Apple_SMC_OSK->setText( vm->Get_Apple_SMC_OSK() );
		ui_ao.Edit_UEFI_CODE_File->setText( vm->Get_UEFI_CODE_File() );
		ui_ao.Edit_UEFI_VARS_File->setText( vm->Get_UEFI_VARS_File() );
		ui_ao.CH_Launch_Via_WSL->setChecked( vm->Use_Launch_Via_WSL() );

		on_Button_Apply_clicked();
	}

	Wizard_Win->deleteLater();
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
		Update_System_Tray();

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
			ui.Machines_List->item(ix)->setData( 257, tmp_vm->Get_Icon_Path() );
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

	// Already running: treat Start as Connect to the guest view.
	if( cur_vm->Get_State() == VM::VMS_Running || cur_vm->Get_State() == VM::VMS_Pause )
	{
		on_actionConnect_Session_triggered();
		return;
	}

	if( ! Boot_Is_Correct(cur_vm) ) return;

	// Show the session window first, then start QEMU (display connects when Running).
	// Legacy Win9x/XP use QEMU's own SDL/GTK window — skip embed (VNC can't do text mode).
	const bool embed = Settings.value( "Embedded_Session", "yes" ).toString() == "yes" &&
	                   ! cur_vm->Prefer_Native_VGA_Window();
	if( embed )
		Enter_Session_Mode_Preparing( cur_vm );

	bool started = false;
	Session_Block_During_Start = true;
	AQ_Run_With_Busy_Dialog( this, tr( "Starting virtual machine…" ), [ & ]() {
		started = AQEMU_Service::get().call( "start", cur_vm );
	} );
	Session_Block_During_Start = false;

	if( ! started )
	{
		AQGraphic_Error( "void Main_Window::on_action_Power_On_triggered()",
		                 tr( "Cannot start VM" ),
		                 tr( "QEMU did not start. Check the emulator binary path, "
		                     "machine settings, and the QEMU error log." ),
		                 false );
		if( Session_Mode_Active && Session_VM == cur_vm )
			Exit_Session_Mode();
		return;
	}

	// Attach display only after the busy dialog closes (ports are allocated).
	if( embed &&
	    ( cur_vm->Get_State() == VM::VMS_Running || cur_vm->Get_State() == VM::VMS_Pause ) )
		Enter_Session_Mode( cur_vm );
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
	Q_UNUSED( index );
	// Do NOT call Apply_Emulator(1): its old fall-through rebuilt computer/machine/CPU
	// lists and auto-saved index 0, so picking TCG/KVM silently rewrote other settings.
	Update_Accelerator_Options();
	VM_Changed();
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

	// Keep the user's current picks across list rebuilds (accel refresh, same-arch re-entry).
	const QString keep_machine_caption = ui.CB_Machine_Type_Main->currentText();
	const QString keep_cpu_caption = ui.CB_CPU_Type_Main->currentText();

	curComp = Get_Current_Machine_Devices( &devOk );
	if( ! devOk )
	{
		ui_arch.CB_CPU_Type->blockSignals(false);
		ui_arch.CB_Machine_Type->blockSignals(false);
		ui.CB_CPU_Type_Main->blockSignals(false);
		ui.CB_Machine_Type_Main->blockSignals(false);
		ui.CB_Video_Card->blockSignals(false);
		return;
	}

	Virtual_Machine *live_vm = Get_Current_VM();
	if( live_vm && ! curComp.System.QEMU_Name.isEmpty() )
	{
		live_vm->Set_Computer_Type( curComp.System.QEMU_Name );
	}

	// CPU
	ui_arch.CB_CPU_Type->clear();
	ui.CB_CPU_Type_Main->clear();

	cl = QStringList();

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

	const QString arch_bin = curComp.System.QEMU_Name;
	const bool is_virt_arch =
		arch_bin.contains( "aarch64", Qt::CaseInsensitive ) ||
		arch_bin.contains( "qemu-system-arm", Qt::CaseInsensitive ) ||
		arch_bin.contains( "applesoc", Qt::CaseInsensitive ) ||
		arch_bin.contains( "riscv", Qt::CaseInsensitive );

	Virtual_Machine *cur_vm = Get_Current_VM();
	QString want_machine;
	QString want_cpu;
	const bool same_arch = cur_vm && ( cur_vm->Get_Computer_Type() == arch_bin );
	if( same_arch )
	{
		want_machine = cur_vm->Get_Machine_Type();
		want_cpu = cur_vm->Get_CPU_Type();
	}

	auto select_machine = [&]( const QString &qemu_name ) -> bool
	{
		if( qemu_name.isEmpty() ) return false;
		for( int mx = 0; mx < curComp.Machine_List.count(); ++mx )
		{
			if( curComp.Machine_List[mx].QEMU_Name == qemu_name ||
			    curComp.Machine_List[mx].Caption == qemu_name )
			{
				ui_arch.CB_Machine_Type->setCurrentIndex( mx );
				ui.CB_Machine_Type_Main->setCurrentIndex( mx );
				return true;
			}
		}
		return false;
	};
	auto select_cpu = [&]( const QString &qemu_name ) -> bool
	{
		if( qemu_name.isEmpty() ) return false;
		for( int cx = 0; cx < curComp.CPU_List.count(); ++cx )
		{
			if( curComp.CPU_List[cx].QEMU_Name == qemu_name ||
			    curComp.CPU_List[cx].Caption == qemu_name )
			{
				ui_arch.CB_CPU_Type->setCurrentIndex( cx );
				ui.CB_CPU_Type_Main->setCurrentIndex( cx );
				return true;
			}
		}
		return false;
	};

	// 1) Restore VM values when arch is unchanged
	// 2) Else keep current UI captions if they still exist in the new list
	// 3) Else virt/max defaults for virt arches (new arch only)
	if( ! select_machine( want_machine ) )
		if( ! select_machine( keep_machine_caption ) )
			if( is_virt_arch )
				select_machine( QStringLiteral( "virt" ) );

	if( ! select_cpu( want_cpu ) )
	{
		if( ! select_cpu( keep_cpu_caption ) && is_virt_arch )
		{
			QString prefer_cpu =
			#ifdef Q_OS_WIN32
				QStringLiteral( "max" );
			#else
				QStringLiteral( "host" );
			#endif
			if( ! select_cpu( prefer_cpu ) )
				select_cpu( QStringLiteral( "max" ) );
		}
	}

	// Video
	ui.CB_Video_Card->clear();

	QString want_video = System_Info::Default_Video_Card( arch_bin );
	if( cur_vm && same_arch )
	{
		// UI-only sanitize — never mutate the live VM until Apply/save.
		want_video = System_Info::Sanitize_Video_Card(
			arch_bin, cur_vm->Get_Video_Card(),
			want_machine.isEmpty() ? cur_vm->Get_Machine_Type() : want_machine );
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
	Update_Intel_MacOS_Settings_Ui();
	Update_DeviceTree_Visibility();
	Enforce_Accel_Honesty();
	Enforce_Disk_Bus_Honesty();
	Update_Disabled_Controls();
	VM_Changed();
}

void Main_Window::on_CB_Machine_Type_Main_currentIndexChanged( int index )
{
	if( index < 0 ) return;
	ui_arch.CB_Machine_Type->blockSignals(true);
	if( index < ui_arch.CB_Machine_Type->count() )
		ui_arch.CB_Machine_Type->setCurrentIndex( index );
	ui_arch.CB_Machine_Type->blockSignals(false);

	bool devOk = false;
	Available_Devices curComp = Get_Current_Machine_Devices( &devOk );
	Virtual_Machine *live_vm = Get_Current_VM();
	if( devOk && live_vm && index >= 0 && index < curComp.Machine_List.count() )
	{
		live_vm->Set_Machine_Type( curComp.Machine_List[index].QEMU_Name );
	}
	else if( live_vm && ! ui.CB_Machine_Type_Main->currentText().isEmpty() )
	{
		live_vm->Set_Machine_Type( ui.CB_Machine_Type_Main->currentText() );
	}

	Enforce_Disk_Bus_Honesty();
	Update_DeviceTree_Visibility();
	VM_Changed();
}

void Main_Window::on_CB_CPU_Type_Main_currentIndexChanged( int index )
{
	if( index < 0 ) return;
	ui_arch.CB_CPU_Type->blockSignals(true);
	if( index < ui_arch.CB_CPU_Type->count() )
		ui_arch.CB_CPU_Type->setCurrentIndex( index );
	ui_arch.CB_CPU_Type->blockSignals(false);

	bool devOk = false;
	Available_Devices curComp = Get_Current_Machine_Devices( &devOk );
	Virtual_Machine *live_vm = Get_Current_VM();
	if( devOk && live_vm && index >= 0 && index < curComp.CPU_List.count() )
	{
		live_vm->Set_CPU_Type( curComp.CPU_List[index].QEMU_Name );
	}
	else if( live_vm && ! ui.CB_CPU_Type_Main->currentText().isEmpty() )
	{
		live_vm->Set_CPU_Type( ui.CB_CPU_Type_Main->currentText() );
	}

	VM_Changed();
}

void Main_Window::sync_arch_Machine_Type_changed( int index )
{
	if( index < 0 ) return;
	ui.CB_Machine_Type_Main->blockSignals(true);
	if( index < ui.CB_Machine_Type_Main->count() )
		ui.CB_Machine_Type_Main->setCurrentIndex( index );
	ui.CB_Machine_Type_Main->blockSignals(false);

	bool devOk = false;
	Available_Devices curComp = Get_Current_Machine_Devices( &devOk );
	Virtual_Machine *live_vm = Get_Current_VM();
	if( devOk && live_vm && index >= 0 && index < curComp.Machine_List.count() )
	{
		live_vm->Set_Machine_Type( curComp.Machine_List[index].QEMU_Name );
	}
	else if( live_vm && ! ui.CB_Machine_Type_Main->currentText().isEmpty() )
	{
		live_vm->Set_Machine_Type( ui.CB_Machine_Type_Main->currentText() );
	}

	Update_DeviceTree_Visibility();
	VM_Changed();
}

void Main_Window::sync_arch_CPU_Type_changed( int index )
{
	if( index < 0 ) return;
	ui.CB_CPU_Type_Main->blockSignals(true);
	if( index < ui.CB_CPU_Type_Main->count() )
		ui.CB_CPU_Type_Main->setCurrentIndex( index );
	ui.CB_CPU_Type_Main->blockSignals(false);

	bool devOk = false;
	Available_Devices curComp = Get_Current_Machine_Devices( &devOk );
	Virtual_Machine *live_vm = Get_Current_VM();
	if( devOk && live_vm && index >= 0 && index < curComp.CPU_List.count() )
	{
		live_vm->Set_CPU_Type( curComp.CPU_List[index].QEMU_Name );
	}
	else if( live_vm && ! ui.CB_CPU_Type_Main->currentText().isEmpty() )
	{
		live_vm->Set_CPU_Type( ui.CB_CPU_Type_Main->currentText() );
	}

	VM_Changed();
}

void Main_Window::slot_iOS_Firmware_Tool_triggered()
{
	iOS_Firmware_Tool_Window dlg( this );
	connect( &dlg, &iOS_Firmware_Tool_Window::DeviceTree_Path_Suggested,
	         this, [this]( const QString &path ) {
		if( path.isEmpty() )
			return;
		if( ui.Edit_DeviceTree_Path->text().trimmed().isEmpty() )
		{
			ui.Edit_DeviceTree_Path->setText( path );
			VM_Changed();
		}
	} );
	dlg.exec();
}

void Main_Window::slot_Apple_SoC_Restore_triggered()
{
	Virtual_Machine *vm = Get_Current_VM();
	if( vm )
		Apply_Apple_SoC_Fields_To_VM( vm );
	Apple_SoC_Restore_Window dlg( vm, this );
	dlg.exec();
	if( vm )
		Load_Apple_SoC_Fields_From_VM( vm );
}

void Main_Window::Maybe_Prompt_WSL_Config_On_Boot()
{
#ifdef Q_OS_WIN32
	const QString distro = Settings.value( QStringLiteral( "WSL_Launch/Distro" ), QString() ).toString();
	const QString user = Settings.value( QStringLiteral( "WSL_Launch/Username" ), QString() ).toString();
	if( ! distro.trimmed().isEmpty() && WSL_Is_Valid_Username( user ) )
		return;

	bool needs = Settings.value( QStringLiteral( "WSL_Launch/Enabled" ), false ).toBool();
	for( int i = 0; ! needs && i < VM_List.count(); ++i )
	{
		Virtual_Machine *vm = VM_List[i];
		if( ! vm )
			continue;
		if( AQ_Is_Apple_SoC_VM( vm ) ||
		    vm->Get_Computer_Type().contains( QLatin1String( "reimsvgpu" ), Qt::CaseInsensitive ) ||
		    vm->Use_Launch_Via_WSL() )
			needs = true;
	}
	if( ! needs )
		return;

	const auto ans = QMessageBox::question( this, tr( "WSL configuration" ),
		tr( "AQEMU 1.3.0 uses WSL for Apple SoC (Inferno) and hardware-accelerated "
		    "macOS (Reims) on Windows.\n\n"
		    "WSL distro / username are not configured yet. Set them now?\n\n"
		    "(Passwords are not stored — KVM fixes use wsl -u root.)" ),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
	if( ans == QMessageBox::Yes )
	{
		WSL_Wizard_Window wizard( this );
		wizard.exec();
	}
#endif
}

void Main_Window::Build_Apple_SoC_Inferno_Ui()
{
	QVBoxLayout *lay = ui.Widget_DeviceTree_Main
		? qobject_cast<QVBoxLayout *>( ui.Widget_DeviceTree_Main->layout() )
		: nullptr;
	if( ! lay )
		return;

	auto add_path_row = [&]( const QString &label, QLineEdit **editOut, const QString &filter ) {
		QHBoxLayout *row = new QHBoxLayout();
		row->addWidget( new QLabel( label ) );
		QLineEdit *edit = new QLineEdit();
		*editOut = edit;
		row->addWidget( edit, 1 );
		QToolButton *tb = new QToolButton();
		tb->setText( QStringLiteral( "..." ) );
		tb->setIcon( QIcon( QStringLiteral( ":/open-file.png" ) ) );
		connect( tb, &QToolButton::clicked, this, [this, edit, filter, label]() {
			const QString f = QFileDialog::getOpenFileName( this, label,
				Get_Last_Dir_Path( edit->text() ), filter );
			if( ! f.isEmpty() )
			{
				edit->setText( QDir::toNativeSeparators( f ) );
				VM_Changed();
			}
		} );
		row->addWidget( tb );
		lay->addLayout( row );
		connect( edit, &QLineEdit::textChanged, this, [this]( const QString & ) { VM_Changed(); } );
	};

	add_path_row( tr( "Trustcache:" ), &Edit_Apple_Trustcache,
		tr( "Trustcache (*);;All (*)" ) );
	add_path_row( tr( "Restore ticket:" ), &Edit_Apple_Ticket,
		tr( "Ticket (*);;All (*)" ) );
	add_path_row( tr( "SEP firmware:" ), &Edit_Apple_SEP_FW,
		tr( "SEP FW (*);;All (*)" ) );
	add_path_row( tr( "SEP ROM:" ), &Edit_Apple_SEP_ROM,
		tr( "SEP ROM (*);;All (*)" ) );
	add_path_row( tr( "IPSW (restore):" ), &Edit_Apple_IPSW,
		tr( "IPSW (*.ipsw *.zip);;All (*)" ) );

	QHBoxLayout *usbRow = new QHBoxLayout();
	usbRow->addWidget( new QLabel( tr( "USB remote:" ) ) );
	CB_Apple_USB_Conn_Type = new QComboBox();
	CB_Apple_USB_Conn_Type->addItem( tr( "UNIX (WSL/Linux)" ), QStringLiteral( "unix" ) );
	CB_Apple_USB_Conn_Type->addItem( tr( "IPv4 TCP" ), QStringLiteral( "ipv4" ) );
	usbRow->addWidget( CB_Apple_USB_Conn_Type );
	Edit_Apple_USB_Conn_Addr = new QLineEdit();
	Edit_Apple_USB_Conn_Addr->setPlaceholderText( QStringLiteral( "/tmp/InfernoUSBRemote or 127.0.0.1" ) );
	usbRow->addWidget( Edit_Apple_USB_Conn_Addr, 1 );
	SB_Apple_USB_Conn_Port = new QSpinBox();
	SB_Apple_USB_Conn_Port->setRange( 1, 65535 );
	SB_Apple_USB_Conn_Port->setValue( 8030 );
	SB_Apple_USB_Conn_Port->setToolTip( tr( "TCP port when USB remote type is IPv4" ) );
	usbRow->addWidget( SB_Apple_USB_Conn_Port );
	lay->addLayout( usbRow );

	connect( CB_Apple_USB_Conn_Type, QOverload<int>::of( &QComboBox::currentIndexChanged ),
	         this, [this]( int ) { VM_Changed(); } );
	connect( Edit_Apple_USB_Conn_Addr, &QLineEdit::textChanged,
	         this, [this]( const QString & ) { VM_Changed(); } );
	connect( SB_Apple_USB_Conn_Port, QOverload<int>::of( &QSpinBox::valueChanged ),
	         this, [this]( int ) { VM_Changed(); } );

	QHBoxLayout *btnRow = new QHBoxLayout();
	QPushButton *btnRestore = new QPushButton( tr( "Apple SoC Restore…" ) );
	connect( btnRestore, &QPushButton::clicked, this, &Main_Window::slot_Apple_SoC_Restore_triggered );
	btnRow->addWidget( btnRestore );
	btnRow->addStretch();
	lay->addLayout( btnRow );
}

void Main_Window::Apply_Apple_SoC_Fields_To_VM( Virtual_Machine *vm )
{
	if( ! vm )
		return;
	// Persist profile only from authoritative target markers — never from VM name heuristics.
	const QString c_type = vm->Get_Computer_Type();
	const QString m_type = vm->Get_Machine_Type();
	if( c_type.contains( QLatin1String( "applesoc" ), Qt::CaseInsensitive ) ||
	    m_type.contains( QLatin1String( "t8030" ), Qt::CaseInsensitive ) ||
	    m_type.contains( QLatin1String( "s8000" ), Qt::CaseInsensitive ) )
		vm->Use_Apple_SoC_Profile( true );
	if( Edit_Apple_Trustcache )
		vm->Set_Apple_Trustcache_Path( Edit_Apple_Trustcache->text() );
	if( Edit_Apple_Ticket )
		vm->Set_Apple_Ticket_Path( Edit_Apple_Ticket->text() );
	if( Edit_Apple_SEP_FW )
		vm->Set_Apple_SEP_FW_Path( Edit_Apple_SEP_FW->text() );
	if( Edit_Apple_SEP_ROM )
		vm->Set_Apple_SEP_ROM_Path( Edit_Apple_SEP_ROM->text() );
	if( Edit_Apple_IPSW )
		vm->Set_Apple_IPSW_Path( Edit_Apple_IPSW->text() );
	if( CB_Apple_USB_Conn_Type )
		vm->Set_Apple_USB_Conn_Type( CB_Apple_USB_Conn_Type->currentData().toString() );
	if( Edit_Apple_USB_Conn_Addr )
		vm->Set_Apple_USB_Conn_Addr( Edit_Apple_USB_Conn_Addr->text() );
	if( SB_Apple_USB_Conn_Port )
		vm->Set_Apple_USB_Conn_Port( SB_Apple_USB_Conn_Port->value() );
}

void Main_Window::Load_Apple_SoC_Fields_From_VM( const Virtual_Machine *vm )
{
	if( ! vm )
		return;
	if( Edit_Apple_Trustcache )
		Edit_Apple_Trustcache->setText( vm->Get_Apple_Trustcache_Path() );
	if( Edit_Apple_Ticket )
		Edit_Apple_Ticket->setText( vm->Get_Apple_Ticket_Path() );
	if( Edit_Apple_SEP_FW )
		Edit_Apple_SEP_FW->setText( vm->Get_Apple_SEP_FW_Path() );
	if( Edit_Apple_SEP_ROM )
		Edit_Apple_SEP_ROM->setText( vm->Get_Apple_SEP_ROM_Path() );
	if( Edit_Apple_IPSW )
		Edit_Apple_IPSW->setText( vm->Get_Apple_IPSW_Path() );
	if( CB_Apple_USB_Conn_Type )
	{
		const QString t = vm->Get_Apple_USB_Conn_Type();
		const int idx = CB_Apple_USB_Conn_Type->findData(
			t.compare( QLatin1String( "ipv4" ), Qt::CaseInsensitive ) == 0
				? QStringLiteral( "ipv4" ) : QStringLiteral( "unix" ) );
		if( idx >= 0 )
			CB_Apple_USB_Conn_Type->setCurrentIndex( idx );
	}
	if( Edit_Apple_USB_Conn_Addr )
		Edit_Apple_USB_Conn_Addr->setText( vm->Get_Apple_USB_Conn_Addr() );
	if( SB_Apple_USB_Conn_Port )
		SB_Apple_USB_Conn_Port->setValue( vm->Get_Apple_USB_Conn_Port() > 0 ? vm->Get_Apple_USB_Conn_Port() : 8030 );
}

void Main_Window::Update_Machine_Accelerators()
{
	const QString keep = ui.CB_Machine_Accelerator->currentData( Qt::UserRole ).toString().toLower();

	ui.CB_Machine_Accelerator->blockSignals( true );
	ui.CB_Machine_Accelerator->clear();
	ui.CB_Machine_Accelerator->addItem( tr( "TCG" ), QStringLiteral( "tcg" ) );
	ui.CB_Machine_Accelerator->addItem( tr( "KVM" ), QStringLiteral( "kvm" ) );
	ui.CB_Machine_Accelerator->addItem( tr( "XEN" ), QStringLiteral( "xen" ) );

	int restore = 0;
	for( int i = 0; i < ui.CB_Machine_Accelerator->count(); ++i )
	{
		if( ui.CB_Machine_Accelerator->itemData( i, Qt::UserRole ).toString().toLower() == keep )
		{
			restore = i;
			break;
		}
	}
	ui.CB_Machine_Accelerator->setCurrentIndex( restore );
	ui.CB_Machine_Accelerator->blockSignals( false );
	Enforce_Accel_Honesty();
}

void Main_Window::Enforce_Accel_Honesty()
{
	const QString host = AQ_Get_Host_CPU_Architecture();
	bool ok = false;
	const Available_Devices dev = Get_Current_Machine_Devices( &ok );
	const QString guest_bin = ok ? dev.System.QEMU_Name : QString();
	const QString guest = AQ_Normalize_CPU_Architecture( guest_bin );

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

	const bool is_wsl = ui.CH_Intel_Mac_WSL_Main->isChecked() || ui_ao.CH_Launch_Via_WSL->isChecked();
	const bool is_native = AQ_Guest_Matches_Host_Architecture( guest ) || is_wsl;

	int tcg_index = -1;
	int kvm_index = -1;
	for( int i = 0; i < ui.CB_Machine_Accelerator->count(); ++i )
	{
		const QString id = ui.CB_Machine_Accelerator->itemData( i, Qt::UserRole ).toString().toLower();
		if( id == QLatin1String( "tcg" ) )
			tcg_index = i;
		if( id == QLatin1String( "kvm" ) )
			kvm_index = i;

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

	bool forced_tcg = false;
	bool accel_index_changed = false;
	if( is_wsl )
	{
		ui.CB_Machine_Accelerator->setEnabled( false );
		if( kvm_index >= 0 && ui.CB_Machine_Accelerator->currentIndex() != kvm_index )
		{
			ui.CB_Machine_Accelerator->setCurrentIndex( kvm_index );
			accel_index_changed = true;
		}
		ui.CB_Machine_Accelerator->setToolTip(
			tr( "WSL execution selected: native KVM acceleration is enforced inside the WSL VM." ) );
	}
	else if( ! is_native )
	{
		ui.CB_Machine_Accelerator->setEnabled( true );
		if( tcg_index >= 0 && ui.CB_Machine_Accelerator->currentIndex() != tcg_index )
		{
			ui.CB_Machine_Accelerator->setCurrentIndex( tcg_index );
			forced_tcg = true;
		}
		ui.CB_Machine_Accelerator->setToolTip(
			tr( "Cross-architecture emulation: host is %1, guest is %2. "
			    "Native acceleration (KVM / WHPX / HVF) cannot run this guest. Using TCG." )
				.arg( host ).arg( guest ) );
	}
	else
	{
		ui.CB_Machine_Accelerator->setEnabled( true );
		ui.CB_Machine_Accelerator->setToolTip(
			tr( "Host %1 matches guest %2. On Windows, KVM maps to WHPX/HAX with TCG fallback." )
				.arg( host ).arg( guest ) );
	}

	ui.CB_Machine_Accelerator->blockSignals( false );
	Update_Accelerator_Options();

	// Persist forced accel (WSL→KVM or cross-arch→TCG) so Apply/Cancel don't lose it.
	if( forced_tcg || accel_index_changed )
		VM_Changed();
}

int Main_Window::Disk_Interface_To_Combo_Index( VM::Device_Interface iface ) const
{
	switch( iface )
	{
		case VM::DI_Virtio: return 0;
		case VM::DI_Virtio_SCSI: return 1;
		case VM::DI_SCSI: return 2;
		case VM::DI_IDE: return 3;
		case VM::DI_AHCI: return 4;
		case VM::DI_SD: return 5;
		case VM::DI_NVMe: return 6;
		default: return 3;
	}
}

VM::Device_Interface Main_Window::Combo_Index_To_Disk_Interface( int index ) const
{
	switch( index )
	{
		case 0: return VM::DI_Virtio;
		case 1: return VM::DI_Virtio_SCSI;
		case 2: return VM::DI_SCSI;
		case 3: return VM::DI_IDE;
		case 4: return VM::DI_AHCI;
		case 5: return VM::DI_SD;
		case 6: return VM::DI_NVMe;
		default: return VM::DI_IDE;
	}
}

void Main_Window::Enforce_Disk_Bus_Honesty()
{
	bool ok = false;
	const Available_Devices dev = Get_Current_Machine_Devices( &ok );
	const QString computer = ok ? dev.System.QEMU_Name : QString();
	const QString machine = ui.CB_Machine_Type_Main->currentText();

	auto *model = qobject_cast<QStandardItemModel *>( ui.CB_Disk_Interface->model() );
	ui.CB_Disk_Interface->blockSignals( true );

	// Main Window power-user path: expose every drive interface. QEMU may still
	// reject a bad combo at launch — that is intentional (unlike the wizard).
	for( int i = 0; i < ui.CB_Disk_Interface->count(); ++i )
	{
		if( model )
		{
			QStandardItem *item = model->item( i );
			if( item )
				item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
		}
	}

	if( ! computer.isEmpty() )
	{
		ui.CB_Disk_Interface->setToolTip( tr(
			"Drive interface for the primary hard disk. All QEMU interfaces are "
			"selectable here; pick one that matches the guest machine.\n"
			"Computer: %1  Machine: %2" )
			.arg( computer, machine.isEmpty() ? tr( "(default)" ) : machine ) );
	}

	ui.CB_Disk_Interface->blockSignals( false );
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
	for( QMap<QString, Available_Devices>::const_iterator it = System_Info::Emulator_QEMU_2_0.constBegin();
	     it != System_Info::Emulator_QEMU_2_0.constEnd(); ++it )
	{
		if( ! current_devices.contains( it.key() ) )
			current_devices.insert( it.key(), it.value() );
	}

    QString text = ui.CB_Computer_Type->currentText();

    ui.CB_Computer_Type->blockSignals(true);

	ui.CB_Computer_Type->clear();

	for( QMap<QString, Available_Devices>::const_iterator i = current_devices.constBegin(); i != current_devices.constEnd(); i++ )
    {
		ui.CB_Computer_Type->addItem( i->System.Caption, i.key() );
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

	// Modes are independent — no fall-through. The old cascade made mode 1
	// (accelerator options) rebuild arch/machine/CPU and wipe the user's picks.
	switch( mode )
	{
		case 0:
			Update_Machine_Accelerators();
			Update_Accelerator_Options();
			Update_Computer_Types();
			Computer_Type_Changed();
			break;
		case 1:
			Update_Accelerator_Options();
			break;
		case 2:
			Update_Computer_Types();
			Computer_Type_Changed();
			break;
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
	// Custom multi-boot row (index 5+) already matches Boot_Order_List — do not
	// strip it or map through the single-device enable logic (that wiped the list
	// to all-disabled → UI "None" and UEFI shell).
	if( index >= 5 )
	{
		VM_Changed();
		return;
	}

	// Drop the dynamic multi-boot label only (base items are indices 0–4).
	while( ui.CB_Boot_Priority->count() > 5 )
		ui.CB_Boot_Priority->removeItem( 5 );

	VM::Boot_Device bootDev;

	// Use the signal's index — not currentIndex() after removeItem, which can
	// jump to "None" when the removed row was selected.
	switch( index )
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
			AQWarning( "void Main_Window::CB_Boot_Priority_currentIndexChanged( int index )",
					   "Use Default Boot Device: CD-ROM" );
			bootDev = VM::Boot_From_CDROM;
			break;
	}

	// Expand truncated lists (Win11 lifecycle / wizard) so the chosen type exists.
	VM::Set_Boot_Order_Single( Boot_Order_List, bootDev );

	VM_Changed();
}

void Main_Window::Set_Boot_Order( const QList<VM::Boot_Order> &list )
{
	disconnect( ui.CB_Boot_Priority, SIGNAL(currentIndexChanged(int)),
				this, SLOT(CB_Boot_Priority_currentIndexChanged(int)) );

	const QList<VM::Boot_Order> expanded = VM::Expand_Boot_Order_List( list );
	QStringList bootStr = VM::Boot_Order_To_String_List( expanded );

	// Clear dynamic multi-boot label (indices 0–4 are fixed)
	while( ui.CB_Boot_Priority->count() > 5 )
		ui.CB_Boot_Priority->removeItem( 5 );

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
		Boot_Order_List = VM::Expand_Boot_Order_List( boot_win.data() );
		Show_Boot_Menu = boot_win.useBootMenu();

		// Apply data to UI and persist (Set_Boot_Order alone may not emit
		// currentIndexChanged when the combo index stays the same).
		Set_Boot_Order( Boot_Order_List );
		VM_Changed();
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
    if( ! old_vm )
        return;

    Virtual_Machine old_vm_copy( *old_vm );
    Virtual_Machine tmp_vm;
    bool ok = Create_VM_From_Ui( &tmp_vm, old_vm, false );

    if( dialog->exec() == QDialog::Accepted )
    {
        // Dialog widgets may already have fired VM_Changed; force a flush so
        // Accept never leaves unsaved edits (index-unchanged combos, etc.).
        VM_Changed();
        if( ui.Button_Apply->isEnabled() )
            on_Button_Apply_clicked();
        return;
    }

    // Cancel: restore UI to the pre-dialog snapshot
    if( ok )
    {
        *old_vm = tmp_vm;
        Update_VM_Ui( false );
        *old_vm = old_vm_copy;
    }
}

void Main_Window::on_TB_Show_Advanced_Options_Window_clicked()
{
	Refresh_Gamepad_List( Get_Current_VM() ? Get_Current_VM()->Get_Gamepad_Filter_IDs() : QStringList() );
	if( Advanced_Options )
	{
		const QRect scr = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->availableGeometry() : QRect( 0, 0, 1024, 768 );
		const int target_w = qMax( 900, static_cast<int>( scr.width() * 0.65 ) );
		const int target_h = qMin( scr.height() - 100, static_cast<int>( scr.height() * 0.75 ) );
		Advanced_Options->resize( target_w, target_h );
		Advanced_Options->setMaximumHeight( target_h );
	}
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

void Main_Window::on_Button_Win11_Repair_clicked()
{
	Virtual_Machine *vm = Get_Current_VM();
	const QString disk = ( vm && vm->Get_HDA().Get_Enabled() )
		? vm->Get_HDA().Get_File_Name()
		: ( Dev_Manager ? Dev_Manager->HDA.Get_File_Name() : QString() );

	QDialog dlg( this );
	dlg.setWindowTitle( tr( "Windows 11 ARM — OOBE / UCPD helpers" ) );
	dlg.resize( 720, 520 );

	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	QLabel *intro = new QLabel( tr(
		"<p>Use <b>Send Shift+F10</b> on the session toolbar (or Shift+F10 with the guest focused) "
		"to open Setup CMD, then paste one of the recipes below.</p>"
		"<p>Account screens that show <i>An error occurred</i> on ARM/TCG are usually Microsoft "
		"account / network services failing — skip to a local account with the OOBE recipe.</p>" ), &dlg );
	intro->setWordWrap( true );
	lay->addWidget( intro );

	QTextEdit *edit = new QTextEdit( &dlg );
	edit->setReadOnly( true );
	edit->setPlainText(
		tr( "=== OOBE bypass (try this first) ===\n\n" ) +
		Win11_OOBE_Bypass_Guest_Commands() +
		QStringLiteral( "\n\n" ) +
		tr( "=== Remove UCPD.sys (if it reboots after OOBE / BSOD) ===\n\n" ) +
		Win11_UCPD_Guest_Commands() +
		QStringLiteral( "\n\n" ) +
		tr( "Disk image: %1" ).arg( disk.isEmpty() ? tr( "(none)" ) : disk ) );
	lay->addWidget( edit );

	QHBoxLayout *btns = new QHBoxLayout();
	QPushButton *copy_oobe = new QPushButton( tr( "Copy OOBE commands" ), &dlg );
	QPushButton *copy_ucpd = new QPushButton( tr( "Copy UCPD commands" ), &dlg );
	QPushButton *auto_ucpd = new QPushButton( tr( "Remove UCPD from disk image…" ), &dlg );
	btns->addWidget( copy_oobe );
	btns->addWidget( copy_ucpd );
	btns->addWidget( auto_ucpd );
	btns->addStretch();
	lay->addLayout( btns );

	QDialogButtonBox *box = new QDialogButtonBox( QDialogButtonBox::Close, &dlg );
	lay->addWidget( box );
	connect( box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject );

	connect( copy_oobe, &QPushButton::clicked, &dlg, []() {
		QApplication::clipboard()->setText( Win11_OOBE_Bypass_Guest_Commands() );
	} );
	connect( copy_ucpd, &QPushButton::clicked, &dlg, []() {
		QApplication::clipboard()->setText( Win11_UCPD_Guest_Commands() );
	} );
	connect( auto_ucpd, &QPushButton::clicked, &dlg, [ this, vm, disk ]() {
		if( disk.isEmpty() || ! QFile::exists( disk ) )
		{
			QMessageBox::warning( this, tr( "UCPD removal" ),
								  tr( "No HDA disk image is configured for this VM." ) );
			return;
		}
		if( vm && ( vm->Get_State() == VM::VMS_Running || vm->Get_State() == VM::VMS_Pause ) )
		{
			QMessageBox::warning( this, tr( "UCPD removal" ),
								  tr( "Shut down the VM first (or use the guest Shift+F10 UCPD commands while OOBE is open)." ) );
			return;
		}
		QString msg;
		const bool ok = Remove_UCPD_From_Disk_Image( disk, &msg );
		if( ok )
			QMessageBox::information( this, tr( "UCPD removal" ), msg );
		else
			QMessageBox::warning( this, tr( "UCPD removal" ), msg );
	} );

	dlg.exec();
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

void Main_Window::Update_Intel_MacOS_Settings_Ui()
{
	Virtual_Machine *vm = Get_Current_VM();
	const bool is_apple_soc = vm && (
		vm->Get_Computer_Type().contains( QLatin1String( "applesoc" ), Qt::CaseInsensitive ) ||
		vm->Get_Machine_Name().contains( QLatin1String( "Apple Silicon" ), Qt::CaseInsensitive ) ||
		vm->Get_Machine_Name().contains( QLatin1String( "iOS" ), Qt::CaseInsensitive ) );
	const bool is_reims = vm &&
		vm->Get_Computer_Type().contains( QLatin1String( "reimsvgpu" ), Qt::CaseInsensitive );
	const bool show = vm && ! is_apple_soc && ( vm->Use_Intel_MacOS_Profile() || is_reims );

	ui.label_intel_macos->setVisible( show );
	ui.GB_Intel_MacOS_Settings->setVisible( show );
	ui.verticalSpacer_intel_macos->changeSize(
		20, show ? 16 : 0, QSizePolicy::Minimum, show ? QSizePolicy::Fixed : QSizePolicy::Ignored );
	ui.verticalSpacer_intel_macos->invalidate();

	if( show )
		Update_Intel_Mac_GPU_Passthrough_Ui();
	else
		ui.GB_Intel_Mac_GPU_Passthrough->setVisible( false );
}

void Main_Window::Update_DeviceTree_Visibility()
{
	Virtual_Machine *vm = Get_Current_VM();
	if( ! vm )
	{
		ui.Widget_DeviceTree_Main->setVisible( false );
		return;
	}

	ui.Widget_DeviceTree_Main->setVisible( Uses_Apple_SoC_Boot_UI( vm ) );
}

bool Main_Window::Uses_Apple_SoC_Boot_UI( const Virtual_Machine *vm ) const
{
	if( ! vm )
		return false;

	const QString m_name = vm->Get_Machine_Name();
	const QString c_type = vm->Get_Computer_Type();
	const QString m_type = vm->Get_Machine_Type();

	if( c_type.contains( QLatin1String( "reimsvgpu" ), Qt::CaseInsensitive ) )
		return false;

	return vm->Use_Apple_SoC_Profile() ||
	       m_name.contains( QLatin1String( "iOS" ), Qt::CaseInsensitive ) ||
	       m_name.contains( QLatin1String( "iPhone" ), Qt::CaseInsensitive ) ||
	       m_name.contains( QLatin1String( "iPad" ), Qt::CaseInsensitive ) ||
	       m_name.contains( QLatin1String( "Apple Silicon" ), Qt::CaseInsensitive ) ||
	       c_type.contains( QLatin1String( "applesoc" ), Qt::CaseInsensitive ) ||
	       m_type.contains( QLatin1String( "t8030" ), Qt::CaseInsensitive ) ||
	       m_type.contains( QLatin1String( "s8000" ), Qt::CaseInsensitive );
}

void Main_Window::Update_Intel_Mac_GPU_Passthrough_Ui()
{
	if( ! System_Info::Host_GPU_Was_Scanned() && ! GPU_Scan_Busy )
	{
		ui.Label_Intel_Mac_GPU_Status->setText( tr( "Scanning host GPUs…" ) );
		Start_Host_GPU_Scan();
	}
	Apply_Intel_Mac_GPU_Passthrough_Ui_From_Cache();
}

void Main_Window::Start_Host_GPU_Scan()
{
	if( GPU_Scan_Busy )
		return;
	GPU_Scan_Busy = true;
	QThread *th = QThread::create( []() {
		System_Info::Update_Host_GPU();
	} );
	connect( th, &QThread::finished, this, [this, th]() {
		GPU_Scan_Busy = false;
		th->deleteLater();
		Apply_Intel_Mac_GPU_Passthrough_Ui_From_Cache();
	} );
	th->start();
}

void Main_Window::Apply_Intel_Mac_GPU_Passthrough_Ui_From_Cache()
{
	const bool has_amd = System_Info::Has_AMD_Display_GPU();
	const bool has_nvidia = System_Info::Has_NVIDIA_Display_GPU();
	const bool has_wsl_gpu = System_Info::Has_WSL_Vulkan_GPU();

	Virtual_Machine *vm = Get_Current_VM();
	const bool is_reims = vm &&
		vm->Get_Computer_Type().contains( QLatin1String( "reimsvgpu" ), Qt::CaseInsensitive );

	// Reims: show GPU status for AMD and NVIDIA (WSL Vulkan), even without VFIO.
	if( is_reims )
	{
		ui.GB_Intel_Mac_GPU_Passthrough->setVisible( true );
		ui.CH_Intel_Mac_GPU_Passthrough->setEnabled( false );
		ui.CB_Intel_Mac_GPU->setEnabled( false );
		ui.Edit_Intel_Mac_GPU_Audio->setEnabled( false );
		ui.Edit_Intel_Mac_GPU_ROM->setEnabled( false );
		ui.TB_Intel_Mac_GPU_ROM_Browse->setEnabled( false );
		ui.TB_Intel_Mac_GPU_Refresh->setEnabled( true );
		ui.CH_Intel_Mac_GPU_Passthrough->setChecked( false );

		QString vendors;
		if( has_amd && has_nvidia )
			vendors = tr( "AMD and NVIDIA" );
		else if( has_amd )
			vendors = tr( "AMD" );
		else if( has_nvidia )
			vendors = tr( "NVIDIA" );
		else if( System_Info::Host_GPU_Was_Scanned() )
			vendors = tr( "no AMD/NVIDIA display GPU detected" );
		else
			vendors = tr( "scanning…" );

#ifdef Q_OS_WIN32
		ui.Label_Intel_Mac_GPU_Status->setText( tr(
			"Reims hardware acceleration uses Linux qemu-system-reims3d under WSL with "
			"host GPU Vulkan (AMD and NVIDIA via WSLg). Detected: %1.\n"
			"PCIe Metal passthrough is not available on Windows — leave VFIO off." )
			.arg( vendors ) );
#else
		ui.Label_Intel_Mac_GPU_Status->setText( tr(
			"Reims acceleration needs qemu-system-reims3d with Vulkan. Detected: %1.\n"
			"AMD Metal VFIO passthrough is separate and only on bare-metal Linux." )
			.arg( vendors ) );
#endif
		Q_UNUSED( has_wsl_gpu );
		return;
	}

	ui.GB_Intel_Mac_GPU_Passthrough->setVisible( has_amd || ! System_Info::Host_GPU_Was_Scanned() );
	if( ! has_amd )
	{
		if( System_Info::Host_GPU_Was_Scanned() )
			ui.GB_Intel_Mac_GPU_Passthrough->setVisible( false );
		return;
	}

	const bool can_pass = System_Info::Host_Supports_PCI_Passthrough() &&
	                      ! ui.CH_Intel_Mac_WSL_Main->isChecked();

	ui.CH_Intel_Mac_GPU_Passthrough->setEnabled( can_pass );
	ui.CB_Intel_Mac_GPU->setEnabled( can_pass );
	ui.Edit_Intel_Mac_GPU_Audio->setEnabled( can_pass );
	ui.Edit_Intel_Mac_GPU_ROM->setEnabled( can_pass );
	ui.TB_Intel_Mac_GPU_ROM_Browse->setEnabled( can_pass );
	ui.TB_Intel_Mac_GPU_Refresh->setEnabled( true );

	const QString saved_bdf = ui.CB_Intel_Mac_GPU->currentData().toString();
	ui.CB_Intel_Mac_GPU->blockSignals( true );
	ui.CB_Intel_Mac_GPU->clear();
	const QList<Host_GPU> &gpus = System_Info::Get_Cached_Host_GPU_List();
	for( int i = 0; i < gpus.count(); ++i )
	{
		if( ! gpus[i].Is_Display || gpus[i].Vendor != QLatin1String( "AMD" ) )
			continue;
		const QString label = gpus[i].PCI_Address.isEmpty()
			? gpus[i].Name
			: QString( "%1 (%2)" ).arg( gpus[i].Name, gpus[i].PCI_Address );
		ui.CB_Intel_Mac_GPU->addItem( label, gpus[i].PCI_Address );
	}
	const int restore = ui.CB_Intel_Mac_GPU->findData( saved_bdf );
	if( restore >= 0 )
		ui.CB_Intel_Mac_GPU->setCurrentIndex( restore );
	else if( ui.CB_Intel_Mac_GPU->count() > 0 )
		ui.CB_Intel_Mac_GPU->setCurrentIndex( 0 );
	ui.CB_Intel_Mac_GPU->blockSignals( false );

	if( can_pass )
	{
		ui.Label_Intel_Mac_GPU_Status->setText( tr(
			"Native Linux VFIO available. Keep passthrough off until macOS is installed; "
			"then bind the AMD GPU to vfio-pci and enable this option." ) );
	}
	else
	{
#ifdef Q_OS_WIN32
		ui.Label_Intel_Mac_GPU_Status->setText( tr(
			"AMD GPU detected. Metal PCIe passthrough needs bare-metal Linux VFIO.\n"
			"For hardware-accelerated macOS on Windows, use the Reims (reims3d) target "
			"under WSL — AMD and NVIDIA GPUs both work via WSLg Vulkan.\n"
			"Saved VFIO settings are kept but cannot be changed here." ) );
#else
		ui.Label_Intel_Mac_GPU_Status->setText( tr(
			"AMD GPU detected, but PCIe passthrough is not available in this environment "
			"(WSL or no host PCI). Use bare-metal Linux for Metal VFIO, or Reims + Vulkan." ) );
#endif
	}
}

void Main_Window::on_TB_Intel_Mac_GPU_Refresh_clicked()
{
	ui.Label_Intel_Mac_GPU_Status->setText( tr( "Scanning host GPUs…" ) );
	Start_Host_GPU_Scan();
	VM_Changed();
}

void Main_Window::on_TB_Intel_Mac_GPU_ROM_Browse_clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr( "Select GPU ROM / VBIOS" ),
		Get_Last_Dir_Path( ui.Edit_Intel_Mac_GPU_ROM->text() ),
		tr( "ROM (*.rom *.bin);;All Files (*)" ) );
	if( file.isEmpty() )
		return;
	ui.Edit_Intel_Mac_GPU_ROM->setText( QDir::toNativeSeparators( file ) );
	VM_Changed();
}

void Main_Window::on_CB_Intel_Mac_GPU_currentIndexChanged( int index )
{
	if( index < 0 )
		return;
	const QString bdf = ui.CB_Intel_Mac_GPU->itemData( index ).toString();
	if( ui.Edit_Intel_Mac_GPU_Audio->text().trimmed().isEmpty() && ! bdf.isEmpty() )
	{
		const QString audio = System_Info::Suggest_AMD_Audio_For( bdf );
		if( ! audio.isEmpty() )
			ui.Edit_Intel_Mac_GPU_Audio->setText( audio );
	}
	VM_Changed();
}

void Main_Window::on_TB_Intel_Mac_OpenCore_Browse_Main_clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr( "Select OpenCore ISO or disk image" ),
		Get_Last_Dir_Path( ui.Edit_Intel_Mac_OpenCore_Main->text() ),
		tr( "OpenCore (*.iso *.qcow2 *.qcow *.img *.raw);;ISO (*.iso);;All Files (*)" ) );
	if( file.isEmpty() )
		return;
	ui.Edit_Intel_Mac_OpenCore_Main->setText( QDir::toNativeSeparators( file ) );
	ui_ao.Edit_OpenCore_Boot_Path->setText( ui.Edit_Intel_Mac_OpenCore_Main->text() );
	VM_Changed();
}

void Main_Window::on_TB_Intel_Mac_Recovery_Browse_Main_clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr( "Select Recovery / installer image" ),
		Get_Last_Dir_Path( ui.Edit_Intel_Mac_Recovery_Main->text() ),
		tr( "Images (*.iso *.dmg *.img *.raw *.qcow2);;All Files (*)" ) );
	if( file.isEmpty() )
		return;
	ui.Edit_Intel_Mac_Recovery_Main->setText( QDir::toNativeSeparators( file ) );
	VM_Changed();
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
	// BVM uses usb-kbd only (no virtio-keyboard) for install and everyday boot.
	vm->Use_VirtIO_Keyboard( false );
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

		// Full boot list (CD then HDD) — truncated lists break the boot combo
		VM::Set_Boot_Order_Enabled( Boot_Order_List, VM::Boot_From_CDROM, VM::Boot_From_HDD );
		Set_Boot_Order( Boot_Order_List );
	}
	else if( mode == VM::Win11_First_Boot )
	{
		// BVM post-install "boot" uses virtio-gpu only (ramfb is install-only).
		const int vix = ui.CB_Video_Card->findData( "virtio-gpu-pci" );
		if( vix >= 0 )
			ui.CB_Video_Card->setCurrentIndex( vix );

		if( ui.Memory_Size->value() > 4096 )
			ui.Memory_Size->setValue( 4096 );

		// Keep path but detach ISO for this phase
		Dev_Manager->CD_ROM.Set_Enabled( false );

		VM::Set_Boot_Order_Enabled( Boot_Order_List, VM::Boot_From_HDD );
		Set_Boot_Order( Boot_Order_List );
	}
	else // Normal
	{
		const int vix = ui.CB_Video_Card->findData( "virtio-gpu-pci" );
		if( vix >= 0 )
			ui.CB_Video_Card->setCurrentIndex( vix );

		Dev_Manager->CD_ROM.Set_Enabled( false );

		VM::Set_Boot_Order_Enabled( Boot_Order_List, VM::Boot_From_HDD );
		Set_Boot_Order( Boot_Order_List );
	}

	Update_Win11_Lifecycle_Ui();
	Update_Intel_MacOS_Settings_Ui();
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

void Main_Window::AO_Refresh_Gamepads_clicked()
{
	QStringList selected;
	for( int i = 0; i < ui_ao.LW_Gamepads->count(); ++i )
	{
		QListWidgetItem *it = ui_ao.LW_Gamepads->item( i );
		if( it && it->checkState() == Qt::Checked )
			selected << it->data( Qt::UserRole ).toString();
	}
	Refresh_Gamepad_List( selected );
}

void Main_Window::AO_Edit_Blockdev_Graph_clicked()
{
	Blockdev_Graph_Window dlg( this );
	dlg.Set_Lines( AO_Blockdev_Extra_Lines );
	if( dlg.exec() == QDialog::Accepted )
		AO_Blockdev_Extra_Lines = dlg.Get_Lines();
}

void Main_Window::Refresh_Gamepad_List( const QStringList &selected_ids )
{
	QSet<QString> selected;
	for( int i = 0; i < selected_ids.count(); ++i )
		selected.insert( selected_ids[i].trimmed().toLower() );

	ui_ao.LW_Gamepads->clear();
	const QList<VM_USB> pads = System_Info::Get_Host_Gamepads();
	if( pads.isEmpty() )
	{
		QListWidgetItem *empty = new QListWidgetItem( tr( "(No USB gamepads detected — plug in and Refresh)" ) );
		empty->setFlags( Qt::NoItemFlags );
		ui_ao.LW_Gamepads->addItem( empty );
		return;
	}

	for( int i = 0; i < pads.count(); ++i )
	{
		const QString vid = pads[i].Get_Vendor_ID().toLower();
		const QString pid = pads[i].Get_Product_ID().toLower();
		const QString key = vid + QLatin1Char( ':' ) + pid;
		const QString label = QStringLiteral( "%1 (%2)" )
			.arg( pads[i].Get_Product_Name().isEmpty() ? tr( "USB Gamepad" ) : pads[i].Get_Product_Name() )
			.arg( key );
		QListWidgetItem *it = new QListWidgetItem( label );
		it->setFlags( it->flags() | Qt::ItemIsUserCheckable );
		it->setData( Qt::UserRole, key );
		it->setCheckState( selected.contains( key ) ? Qt::Checked : Qt::Unchecked );
		ui_ao.LW_Gamepads->addItem( it );
	}
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

void Main_Window::on_TB_DeviceTree_SetPath_clicked()
{
	QString dtb = QFileDialog::getOpenFileName( this, tr("Select DeviceTree File (.dtb / .im4p)"),
												Get_Last_Dir_Path(ui.Edit_DeviceTree_Path->text()),
												tr("DeviceTree Files (*.dtb *.im4p);;All Files (*)") );

	if( ! dtb.isEmpty() )
		ui.Edit_DeviceTree_Path->setText( QDir::toNativeSeparators(dtb) );
}

void Main_Window::on_TB_App_Kernel_SetPath_clicked()
{
	QString kernel = QFileDialog::getOpenFileName( this, tr("Select Kernel File (.research / .elf / kernelcache)"),
												   Get_Last_Dir_Path(ui.Edit_App_Kernel_Path->text()),
												   tr("Kernel Files (*.research *.elf kernelcache*);;All Files (*)") );

	if( ! kernel.isEmpty() )
		ui.Edit_App_Kernel_Path->setText( QDir::toNativeSeparators(kernel) );
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
