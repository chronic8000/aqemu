# -*- coding: utf-8 -*-
from pathlib import Path

p = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\Main_Window.cpp")
t = p.read_text(encoding="utf-8")

old = """\t, Auto_Save_Timer( nullptr )
{
    Advanced_Options = new QDialog(this);
    Accelerator_Options = new QDialog(this);
    Architecture_Options = new QDialog(this);
    SMP_Settings = new SMP_Settings_Window(this);

\tAuto_Save_Timer = new QTimer( this );
\tAuto_Save_Timer->setSingleShot( true );
\tAuto_Save_Timer->setInterval( 400 );
\tconnect( Auto_Save_Timer, SIGNAL(timeout()), this, SLOT(on_Button_Apply_clicked()) );

    ui.setupUi( this );"""

new = """\t, Auto_Save_Timer( nullptr )
\t, VM_Ui_Refresh_Timer( nullptr )
{
    Advanced_Options = new QDialog(this);
    Accelerator_Options = new QDialog(this);
    Architecture_Options = new QDialog(this);
    SMP_Settings = new SMP_Settings_Window(this);

\tAuto_Save_Timer = new QTimer( this );
\tAuto_Save_Timer->setSingleShot( true );
\tAuto_Save_Timer->setInterval( 400 );
\tconnect( Auto_Save_Timer, SIGNAL(timeout()), this, SLOT(on_Button_Apply_clicked()) );

\t// Coalesce rapid VM-list clicks so we don't rebuild the whole form per click.
\tVM_Ui_Refresh_Timer = new QTimer( this );
\tVM_Ui_Refresh_Timer->setSingleShot( true );
\tVM_Ui_Refresh_Timer->setInterval( 60 );
\tconnect( VM_Ui_Refresh_Timer, &QTimer::timeout, this, [this]() { Update_VM_Ui(); } );

    ui.setupUi( this );"""

if old not in t:
    raise SystemExit("ctor block not found")
t = t.replace(old, new, 1)

old2 = """\tif( ui.Tabs )
\t{
\t\tui.Tabs->setDocumentMode( false );
\t\tui.Tabs->setStyleSheet( QString() ); // never inherit pane rules
\t}

\tif( ui.Tab_General && ui.Tab_General->layout() )
\t{
\t\tui.Tab_General->layout()->setContentsMargins( 8, 8, 10, 8 );"""

new2 = """\tif( ui.Tabs )
\t{
\t\tui.Tabs->setDocumentMode( false );
\t\tui.Tabs->setStyleSheet( QString() ); // never inherit pane rules
\t}

\t// Redundant with the left VM list + Name field — drop the blue "Machine" header.
\tif( ui.label )
\t\tui.label->hide();
\tif( ui.widget && ui.widget->layout() )
\t\tui.widget->layout()->setContentsMargins( 12, 4, 6, 6 );

\tif( ui.Tab_General && ui.Tab_General->layout() )
\t{
\t\tui.Tab_General->layout()->setContentsMargins( 8, 4, 10, 8 );"""

if old2 not in t:
    raise SystemExit("polish block not found")
t = t.replace(old2, new2, 1)

old3 = """\t// RAM
\tif( tmp_vm->Get_Memory_Size() < 1 )
\t{
\t\tAQGraphic_Warning( tr("Error!"),
\t\t\t\t\t\t   tr("Memory size < 1! Using default value: 256 MB") );
\t\tui.Memory_Size->setValue( 256 );
\t}
\telse if( tmp_vm->Get_Memory_Size() >= ui.Memory_Size->maximum() )
\t{
\t\tAQGraphic_Warning( tr("Error!"),
\t\t\t\t\t\t   tr("Memory size > all free memory on this system!") );
\t\tui.Memory_Size->setValue( ui.Memory_Size->maximum() );
\t}
\telse ui.Memory_Size->setValue( tmp_vm->Get_Memory_Size() );"""

new3 = """\t// RAM — clamp silently while switching VMs (no popup thrash).
\tif( tmp_vm->Get_Memory_Size() < 1 )
\t\tui.Memory_Size->setValue( 256 );
\telse if( tmp_vm->Get_Memory_Size() >= ui.Memory_Size->maximum() )
\t\tui.Memory_Size->setValue( ui.Memory_Size->maximum() );
\telse ui.Memory_Size->setValue( tmp_vm->Get_Memory_Size() );"""

if old3 not in t:
    raise SystemExit("ram block not found")
t = t.replace(old3, new3, 1)

old4 = """    if ( update_info_tab )
    {
    \tUpdate_Info_Text();
    }
\tUpdate_Win11_Lifecycle_Ui();
\tUpdate_Intel_MacOS_Settings_Ui();
\tUpdate_Disabled_Controls(); // FIXME

\t// CPU count AFTER Update_Disabled_Controls — that rebuilds the combo and used to wipe this to 1.
\tui.CB_CPU_Count->setEditText( QString::number( tmp_vm->Get_SMP_CPU_Count() ) );
\tSMP_Settings->Set_Values( tmp_vm->Get_SMP(), curComp.PSO_SMP_Count, curComp.PSO_SMP_Cores,
\t\t\t\t\t\t\t  curComp.PSO_SMP_Threads, curComp.PSO_SMP_Sockets, curComp.PSO_SMP_MaxCPUs );

    /* TODO: POST 0.9.1
    QString info_text = tr("Machine:") + " " + tmp_vm->Get_Machine_Name();
    info_text += " " + tr("State:") + " " + tmp_vm->Get_State_Text();

    ui.Label_Machine_Info->setText(info_text);
    */

\t// For VM Changes Signals
\tui.Button_Apply->setEnabled( false );
    ui.Button_Cancel->setEnabled( false );
}"""

new4 = """\t// Skip heavy HTML Info rebuild unless that tab is visible.
\tif( update_info_tab && ui.Tabs && ui.Tabs->currentWidget() == ui.Tab_Info )
\t\tUpdate_Info_Text();
\tUpdate_Win11_Lifecycle_Ui();
\tUpdate_Intel_MacOS_Settings_Ui();
\tUpdate_Disabled_Controls(); // FIXME

\t// CPU count AFTER Update_Disabled_Controls — that rebuilds the combo and used to wipe this to 1.
\tui.CB_CPU_Count->setEditText( QString::number( tmp_vm->Get_SMP_CPU_Count() ) );
\tSMP_Settings->Set_Values( tmp_vm->Get_SMP(), curComp.PSO_SMP_Count, curComp.PSO_SMP_Cores,
\t\t\t\t\t\t\t  curComp.PSO_SMP_Threads, curComp.PSO_SMP_Sockets, curComp.PSO_SMP_MaxCPUs );

\t// For VM Changes Signals
\tui.Button_Apply->setEnabled( false );
\tui.Button_Cancel->setEnabled( false );

\tsetUpdatesEnabled( true );
}"""

if old4 not in t:
    raise SystemExit("end Update_VM_Ui not found")
t = t.replace(old4, new4, 1)

t = t.replace(
    """\tif( tmp_vm->Get_State() == VM::VMS_In_Error )
\t{
\t\tAQError( "void Main_Window::Update_VM_Ui()",
\t\t\t\t "VM in VM::VMS_In_Error state!" );
\t\treturn;
\t}""",
    """\tif( tmp_vm->Get_State() == VM::VMS_In_Error )
\t{
\t\tAQError( "void Main_Window::Update_VM_Ui()",
\t\t\t\t "VM in VM::VMS_In_Error state!" );
\t\tsetUpdatesEnabled( true );
\t\treturn;
\t}""",
    1,
)

t = t.replace(
    """\tif( curComp.System.QEMU_Name.isEmpty() )
\t{
\t\tAQError( "void Main_Window::Update_VM_Ui()",
\t\t\t\t "cur_comp not valid!" );
\t\treturn;
\t}""",
    """\tif( curComp.System.QEMU_Name.isEmpty() )
\t{
\t\tAQError( "void Main_Window::Update_VM_Ui()",
\t\t\t\t "cur_comp not valid!" );
\t\tsetUpdatesEnabled( true );
\t\treturn;
\t}""",
    1,
)

t = t.replace(
    """\telse
\t{
\t\tAQError( "void Main_Window::Update_VM_Ui()",
\t\t\t\t "Cannot find computer type index!" );
\t\treturn;
\t}""",
    """\telse
\t{
\t\tAQError( "void Main_Window::Update_VM_Ui()",
\t\t\t\t "Cannot find computer type index!" );
\t\tsetUpdatesEnabled( true );
\t\treturn;
\t}""",
    1,
)

old5 = """void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous )
{
\tif( VM_List.count() < 1 )
\t{
\t\tAQDebug( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
\t\t\t\t "VM_List.count() < 1" );
\t\treturn;
\t}

\tif( ui.Machines_List->row(previous) < 0 ) return;

    Virtual_Machine tmp_vm;
\tVirtual_Machine *old_vm = Get_VM_By_UID( previous->data(256).toString() );

\tif( old_vm == NULL )
\t{
\t\tAQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous )",
\t\t\t\t "old_vm == NULL" );
\t\treturn;
\t}

    if( Create_VM_From_Ui(&tmp_vm, old_vm) == false &&
\t\told_vm->Get_State() != VM::VMS_In_Error )
\t{
\t\tAQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
\t\t\t\t "Cannot Create VM! Discarding UI changes for previous VM." );

\t\t// Don't block switching — leave previous VM as-is on disk.
\t\tif( ui.Machines_List->row(current) >= 0 &&
\t\t\tui.Machines_List->row(current) < ui.Machines_List->count() )
\t\t{
\t\t\tUpdate_VM_Ui();
\t\t}
\t\treturn;
\t}

\t// if previous machine settings were changed — auto-save, never ask
    if( *old_vm != tmp_vm &&
\t\told_vm->Get_State() != VM::VMS_In_Error && ui.Button_Apply->isEnabled() )
\t{
\t\tif( Auto_Save_Timer )
\t\t\tAuto_Save_Timer->stop();

\t\tdisconnect( old_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
\t\t\t\t\tthis, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

\t\t*old_vm = tmp_vm;

\t\tconnect( old_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
\t\t\t\t this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

\t\told_vm->Save_VM();
\t\tUpdate_VM_Ui();
\t\treturn;
\t}
\telse
\t{
\t\tif( ui.Machines_List->row(current) >= 0 &&
\t\t\tui.Machines_List->row(current) < ui.Machines_List->count() )
\t\t{
\t\t\tUpdate_VM_Ui();
\t\t}
\t\telse
\t\t{
\t\t\tAQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
\t\t\t\t\t "Index Invalid!" );
\t\t}
\t}
}"""

new5 = """void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous )
{
\tif( VM_List.count() < 1 )
\t{
\t\tAQDebug( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
\t\t\t\t "VM_List.count() < 1" );
\t\treturn;
\t}

\tif( ui.Machines_List->row(previous) < 0 ) return;

\tVirtual_Machine *old_vm = Get_VM_By_UID( previous->data(256).toString() );

\tif( old_vm == NULL )
\t{
\t\tAQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous )",
\t\t\t\t "old_vm == NULL" );
\t\treturn;
\t}

\t// Skip expensive Create_VM_From_Ui when there are no pending edits.
\tif( ui.Button_Apply->isEnabled() && old_vm->Get_State() != VM::VMS_In_Error )
\t{
\t\tVirtual_Machine tmp_vm;
\t\tif( Create_VM_From_Ui( &tmp_vm, old_vm ) == false )
\t\t{
\t\t\tAQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
\t\t\t\t\t "Cannot Create VM! Discarding UI changes for previous VM." );
\t\t}
\t\telse if( *old_vm != tmp_vm )
\t\t{
\t\t\tif( Auto_Save_Timer )
\t\t\t\tAuto_Save_Timer->stop();

\t\t\tdisconnect( old_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
\t\t\t\t\t\tthis, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

\t\t\t*old_vm = tmp_vm;

\t\t\tconnect( old_vm, SIGNAL(State_Changed(Virtual_Machine*, VM::VM_State)),
\t\t\t\t\t this, SLOT(VM_State_Changed(Virtual_Machine*, VM::VM_State)) );

\t\t\told_vm->Save_VM();
\t\t}
\t}

\tif( ui.Machines_List->row(current) >= 0 &&
\t\tui.Machines_List->row(current) < ui.Machines_List->count() )
\t{
\t\tSchedule_Update_VM_Ui();
\t}
\telse
\t{
\t\tAQError( "void Main_Window::on_Machines_List_currentItemChanged( QListWidgetItem* current, QListWidgetItem* previous )",
\t\t\t\t "Index Invalid!" );
\t}
}"""

if old5 not in t:
    raise SystemExit("switch handler not found")
t = t.replace(old5, new5, 1)

p.write_text(t, encoding="utf-8")
print("Main_Window.cpp OK")
