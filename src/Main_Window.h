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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QList>
#include <QCloseEvent>

#include "VM.h"
#include "Utils.h"
#include "ui_Main_Window.h"
#include "ui_Advanced_Options.h"
#include "ui_KVM_Options.h"
#include "ui_Architecture_Options.h"
#include "System_Info.h"
#include "HDD_Image_Info.h"
#include "Network_Widget.h"
#include "Old_Network_Widget.h"
#include "SMP_Settings_Window.h"
#include "Add_New_Device_Window.h"
#include "SPICE_Settings_Widget.h"

#include <QSystemTrayIcon>

class Ports_Tab_Widget;
class Device_Manager_Widget;
class Folder_Sharing_Widget;
class Network_Card_Widget;
class Block_VM_Changed_Signals;
class VM_Session_Widget;
class QStackedWidget;
class QTimer;

class Main_Window: public QMainWindow
{
    friend class No_Boot_Device;
    friend class Block_VM_Changed_Signals;

    class Block_VM_Changed_Signals
    {
        public:
            // sync_on_exit=false: used by Update_VM_Ui — do not re-run dirty detection
            // (that falsely marks Apply and triggers auto-save thrash when switching VMs).
            Block_VM_Changed_Signals( Main_Window * _mw, bool sync_on_exit = true )
            {
                mw = _mw;
                call_VM_Changed = sync_on_exit;
                mw->block_VM_changed_signals = true;
            }

            ~Block_VM_Changed_Signals()
            {
                mw->block_VM_changed_signals = false;
                if( call_VM_Changed )
                    mw->VM_Changed();
            }

        private:
            Main_Window *mw;
            bool call_VM_Changed;
    };

	Q_OBJECT
	
	public:
		Main_Window( QWidget *parent = 0 );
        ~Main_Window();

    public slots:
        void VM_State_Changed(const QString& vm, int state);
		
	private slots:
		void on_Machines_List_currentItemChanged( QListWidgetItem *current,
							  QListWidgetItem *previous );
		void on_Machines_List_customContextMenuRequested( const QPoint &pos );
		void on_Machines_List_itemDoubleClicked( QListWidgetItem *item );
		void VM_State_Changed( Virtual_Machine *vm, VM::VM_State s );
		void Show_State_VM( Virtual_Machine *vm );
		void Show_State_Current( Virtual_Machine *vmvm );
		void Set_Widgets_State( bool enabled );
		void VM_Changed();
		void Update_Mouse_Options_Enabled();
		void Sync_Mouse_Pointer_Mode_From_Type();
		void On_Mouse_Pointer_Mode_Changed();
		void Update_Emulator_Control( Virtual_Machine *cur_vm );
		
        void SB_VNC_Display_changed(int);
        void SB_VNC_Display_Port_changed(int);

		// Actions
		void on_actionChange_Icon_triggered();
		void on_actionAbout_AQEMU_triggered();
		void on_actionAbout_Qt_triggered();
		void on_actionDelete_VM_triggered();
		void on_actionDelete_VM_And_Files_triggered();
		void on_actionExit_triggered();
		void on_actionShow_New_VM_Wizard_triggered();
		void on_actionAdd_New_VM_triggered();
		void on_actionCreate_HDD_Image_triggered();
		void on_actionConvert_HDD_Image_triggered();
		void on_actionShow_Advanced_Settings_Window_triggered();
		void on_actionShow_First_Run_Wizard_triggered();
		void on_actionPower_On_triggered();
		void on_actionSave_triggered();
        void on_actionShutdown_triggered();
        void on_actionPower_Off_triggered();
		void on_actionPause_triggered();
		void on_actionReset_triggered();
		void on_actionLoad_VM_From_File_triggered();
		void on_actionSave_As_Template_triggered();
		void on_actionCopy_triggered();
		void on_actionManage_Snapshots_triggered();
		void on_actionShow_Emulator_Control_triggered();
		void on_actionShow_QEMU_Arguments_triggered();
		void on_actionCreate_Shell_Script_triggered();
		void on_actionShow_QEMU_Error_Log_Window_triggered();
		void slot_iOS_Firmware_Tool_triggered();
		void slot_Apple_SoC_Restore_triggered();
		void Maybe_Prompt_WSL_Config_On_Boot();
		void Build_Apple_SoC_Inferno_Ui();
		void Apply_Apple_SoC_Fields_To_VM( Virtual_Machine *vm );
		void Load_Apple_SoC_Fields_From_VM( const Virtual_Machine *vm );
		
		void on_Tabs_currentChanged( int index );
		
		// Apply and Cancel Buttons
		void on_Button_Apply_clicked();
		void on_Button_Cancel_clicked();
		
		// General Tab
		void on_CB_Computer_Type_currentIndexChanged( int index );
		void on_CB_Machine_Type_Main_currentIndexChanged( int index );
		void on_CB_CPU_Type_Main_currentIndexChanged( int index );
		void sync_arch_Machine_Type_changed( int index );
		void sync_arch_CPU_Type_changed( int index );
		void on_CB_Machine_Accelerator_currentIndexChanged( int index );
		void CB_Boot_Priority_currentIndexChanged( int index );
		void on_TB_Show_Boot_Settings_Window_clicked();
		void Set_Boot_Order( const QList<VM::Boot_Order> &list );
		void on_TB_Show_Architecture_Options_Window_clicked();
		void on_TB_Show_Accelerator_Options_Window_clicked();
		void on_TB_Show_Advanced_Options_Window_clicked();
		void on_actionQEMU_Help_Browser_triggered();
		void on_TB_Show_SMP_Settings_Window_clicked();
		bool Validate_CPU_Count( const QString &text );
		void Apply_Emulator( int mode );
		
		void on_CH_Local_Time_toggled( bool on );
		void on_Button_VirtIO_Defaults_clicked();
		void on_Button_Win11_Install_clicked();
		void on_Button_Win11_First_Boot_clicked();
		void on_Button_Win11_Normal_clicked();
		void on_Button_Win11_Repair_clicked();
		void Apply_Win11_Lifecycle_Mode( VM::Win11_Lifecycle_Mode mode );
		void Update_Win11_Lifecycle_Ui();
		void Update_Intel_MacOS_Settings_Ui();
		void Update_DeviceTree_Visibility();
		bool Uses_Apple_SoC_Boot_UI( const Virtual_Machine *vm ) const;
		void Update_Intel_Mac_GPU_Passthrough_Ui();
		void Apply_Intel_Mac_GPU_Passthrough_Ui_From_Cache();
		void Start_Host_GPU_Scan();
		void on_TB_Intel_Mac_OpenCore_Browse_Main_clicked();
		void on_TB_Intel_Mac_Recovery_Browse_Main_clicked();
		void on_TB_Intel_Mac_GPU_Refresh_clicked();
		void on_TB_Intel_Mac_GPU_ROM_Browse_clicked();
		void on_CB_Intel_Mac_GPU_currentIndexChanged( int index );
		
		// Memory
		void on_Memory_Size_valueChanged( int value );
		void on_CB_RAM_Size_editTextChanged( const QString &text );
		void on_CH_Remove_RAM_Size_Limitation_stateChanged( int state );
		void on_TB_Update_Available_RAM_Size_clicked();
		void Update_RAM_Size_ComboBox( int freeRAM );
		
		QStringList Create_Info_HDD_String( const QString &disk_format,
						   const VM::Device_Size &virtual_size,
						   const VM::Device_Size &disk_size,
						    int cluster_size );

		// Network Tab
		void on_CH_Use_Network_toggled( bool on );
		void on_RB_Network_Mode_New_toggled( bool on );
		
		void on_Redirections_List_cellClicked( int row, int column );
		void on_Button_Add_Redirections_clicked();
		void on_Button_Delete_Redirections_clicked();
		void Update_Current_Redirection_Item();
		void on_Button_Clear_Redirections_clicked();
		
		void on_TB_Browse_SMB_clicked();
		void on_TB_Browse_TFTP_clicked();

		// Advanced
		void adv_on_CH_Start_Date_toggled( bool on );
		void Refresh_Gamepad_List( const QStringList &selected_ids );
		void AO_Refresh_Gamepads_clicked();
		void AO_Edit_Blockdev_Graph_clicked();
		
		// Other Tab
		void on_TB_VNC_Unix_Socket_Browse_clicked();
		void on_TB_x509_Browse_clicked();
		void on_TB_x509verify_Browse_clicked();
		
		void on_TB_Linux_bzImage_SetPath_clicked();
		void on_TB_Linux_Initrd_SetPath_clicked();
		void on_TB_DeviceTree_SetPath_clicked();
		void on_TB_App_Kernel_SetPath_clicked();
		
		void on_TB_ROM_File_Browse_clicked();
		void on_TB_MTDBlock_File_Browse_clicked();
		void on_TB_SD_Image_File_Browse_clicked();
		void on_TB_PFlash_File_Browse_clicked();

		void Enter_Session_Mode( Virtual_Machine *vm );
		void Enter_Session_Mode_Preparing( Virtual_Machine *vm );
		void Exit_Session_Mode();
		void On_Session_Exit_View();
		void On_Session_Request_Stop();
		void On_Session_Request_Shutdown();
		void On_Session_Request_Reset();
		void On_Session_Request_Pause();
		void On_Session_Request_Save();
		void on_actionConnect_Session_triggered();
		void Update_Connect_Action();
		void Init_System_Tray();
		void Update_System_Tray();
		void On_Tray_Show();
		void On_Tray_Activated( QSystemTrayIcon::ActivationReason reason );
		void Hide_To_Tray();
		void Update_Display_Window_Mode_Hint();
		void On_Display_Window_Mode_Toggled( bool on );
		
	protected:
		void closeEvent( QCloseEvent *event );
		void changeEvent( QEvent *event );
		
	private:
		Virtual_Machine *Get_VM_By_UID( const QString &uid );
		Virtual_Machine *Get_Current_VM();
        void init_dbus();
		
		void Connect_Signals();
		void Polish_Settings_Tabs_Layout();
		
		const QMap<QString, Available_Devices> Get_Devices_Info( bool *ok ) const;
		Available_Devices Get_Current_Machine_Devices( bool *ok ) const;
		
		bool Create_VM_From_Ui( Virtual_Machine *tmp_vm, Virtual_Machine *old_vm, bool show_user_errors = true );
		
		bool Load_Settings();
		bool Save_Settings();
        bool Save_Or_Discard(bool forced = false);

        void Discard_Changes(QDialog*);

		void setStateActionsEnabled(bool enabled);
        void Change_The_Icon(Virtual_Machine*,QString);
		void Update_VM_Ui( bool update_info_tab = true );
		void Schedule_Update_VM_Ui();
		void Update_VM_Port_Number();
		void Update_Info_Text( int info_mode = 0 );
		void Update_Disabled_Controls();
		void Update_Recent_CD_ROM_Images_List();
		void Update_Recent_Floppy_Images_List();
        void Computer_Type_Changed();
        void Enforce_Accel_Honesty();
	void Enforce_Disk_Bus_Honesty();
	int Disk_Interface_To_Combo_Index( VM::Device_Interface iface ) const;
	VM::Device_Interface Combo_Index_To_Disk_Interface( int index ) const;
        void Update_Accelerator_Options();

	private slots:
        void Update_Machine_Accelerators();
        void Update_Computer_Types();
		void Fill_Display_Resolution_Combo();
		void Update_Display_Resolution_Enabled();
		void Apply_Display_Resolution_To_Ui( const QString &res );
		void Fill_Mouse_Combos();
		void Apply_Mouse_Settings_To_Ui( const Virtual_Machine *vm );
		static bool Mouse_Type_Is_Seamless( const QString &mouse_type );
		void Schedule_Auto_Save();
		
		QString Get_Storage_Device_Info_String( const QString &path );
		
		bool Load_Virtual_Machines();
		bool Save_Virtual_Machines();
		
		QString Get_QEMU_Args();
		QString Get_Current_Binary_Name();
		bool Boot_Is_Correct( Virtual_Machine *tmp_vm );
		bool No_Device_Found( const QString &name, const QString &path, VM::Boot_Device type );
		
		QString Copy_VM_Hard_Drive( const QString &vm_name, const QString &hd_name, const VM_HDD &hd );
		QString Copy_VM_Floppy( const QString &vm_name, const QString &fd_name, const VM_Storage_Device &fd );
		
	private:
		Ui::Main_Window ui;
		Ui::Advanced_Options ui_ao;
		Ui::KVM_Options ui_kvm;
        Ui::Architecture_Options ui_arch;

        QDialog* Advanced_Options;
        QDialog* Accelerator_Options;
        QDialog* Architecture_Options;
		QString AO_Blockdev_Extra_Lines;
        Settings_Widget* Display_Settings_Widget;
        Settings_Widget* Media_Settings_Widget;
        Settings_Widget* Network_Settings_Widget;

		QMenu *Icon_Menu; // Context menu for vm icons
		QMenu *VM_List_Menu; // Context menu for vm list
		QSettings Settings;
		
		bool GUI_User_Mode;
		QString VM_Folder;
		
		QList<Virtual_Machine*> VM_List;
		QList<Emulator> All_Emulators_List; // FIXME use call
		
        SMP_Settings_Window* SMP_Settings;
		
		QList<VM::Boot_Order> Boot_Order_List;
		bool Show_Boot_Menu;
		
		HDD_Image_Info* HDA_Info;
		HDD_Image_Info* HDB_Info;
		HDD_Image_Info* HDC_Info;
		HDD_Image_Info* HDD_Info;
		
		Add_New_Device_Window *Native_Device_Window;
		
		VM_Native_Storage_Device Native_FD0;
		VM_Native_Storage_Device Native_FD1;
		VM_Native_Storage_Device Native_CD_ROM;
		VM_Native_Storage_Device Native_HDA;
		VM_Native_Storage_Device Native_HDB;
		VM_Native_Storage_Device Native_HDC;
		VM_Native_Storage_Device Native_HDD;
		
		Ports_Tab_Widget *Ports_Tab;
		Device_Manager_Widget *Dev_Manager;
        Folder_Sharing_Widget* Folder_Sharing;
		
		Network_Widget *New_Network_Settings_Widget;
		Old_Network_Widget *Old_Network_Settings_Widget;
		
        SPICE_Settings_Widget* SPICE_Widget;

		QStackedWidget *Main_Stack;
		VM_Session_Widget *Session_Widget;
		Virtual_Machine *Session_VM;
		QString Idle_Window_Title;
		bool Session_Mode_Active;
		bool Session_User_Detached;
		/** True while Start() runs under the busy dialog — do not attach SPICE yet. */
		bool Session_Block_During_Start;
		bool GPU_Scan_Busy;

		QLineEdit *Edit_Apple_Trustcache;
		QLineEdit *Edit_Apple_Ticket;
		QLineEdit *Edit_Apple_SEP_FW;
		QLineEdit *Edit_Apple_SEP_ROM;
		QLineEdit *Edit_Apple_IPSW;
		QLineEdit *Edit_Apple_USB_Conn_Addr;
		QComboBox *CB_Apple_USB_Conn_Type;
		QSpinBox *SB_Apple_USB_Conn_Port;

		QSystemTrayIcon *Tray_Icon;
		QAction *Act_Tray_Show;
		QAction *Act_Tray_Quit;
		bool Tray_Restore_Maximized;

        bool block_VM_changed_signals;
		QTimer *Auto_Save_Timer;
		QTimer *VM_Ui_Refresh_Timer;
};

#endif
