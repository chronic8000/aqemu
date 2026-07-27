# Wire wave2 Advanced Options into Main_Window
from pathlib import Path
cpp = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\Main_Window.cpp")
t = cpp.read_text(encoding="utf-8")

old = '''\ttmp_vm->Use_IOThread( ui_ao.CH_IOThread->isChecked() );
	tmp_vm->Use_Modern_Netdev( ui_ao.CH_Modern_Netdev->isChecked() );
	tmp_vm->Set_UUID( ui_ao.Edit_UUID->text() );
	tmp_vm->Set_BIOS_File( ui_ao.Edit_BIOS_File->text().trimmed() );
	tmp_vm->Set_Mem_Path( ui_ao.Edit_Mem_Path->text().trimmed() );
	tmp_vm->Use_Mem_Prealloc( ui_ao.CH_Mem_Prealloc->isChecked() );
	tmp_vm->Set_Machine_Extra_Props( ui_ao.Edit_Machine_Extra_Props->text() );
	tmp_vm->Use_Snapshot_Mode( ui.CH_Snapshot->isChecked() );'''

new = '''\ttmp_vm->Use_IOThread( ui_ao.CH_IOThread->isChecked() );
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
	tmp_vm->Use_Snapshot_Mode( ui.CH_Snapshot->isChecked() );'''

if old not in t:
    raise SystemExit('apply block missing')
t = t.replace(old, new, 1)

old = '''\tui_ao.CH_IOThread->setChecked( tmp_vm->Use_IOThread() );
	ui_ao.CH_Modern_Netdev->setChecked( tmp_vm->Use_Modern_Netdev() );
	ui_ao.Edit_UUID->setText( tmp_vm->Get_UUID() );
	ui_ao.Edit_BIOS_File->setText( tmp_vm->Get_BIOS_File() );
	ui_ao.Edit_Mem_Path->setText( tmp_vm->Get_Mem_Path() );
	ui_ao.CH_Mem_Prealloc->setChecked( tmp_vm->Use_Mem_Prealloc() );
	ui_ao.Edit_Machine_Extra_Props->setText( tmp_vm->Get_Machine_Extra_Props() );
	Refresh_Gamepad_List( tmp_vm->Get_Gamepad_Filter_IDs() );'''

new = '''\tui_ao.CH_IOThread->setChecked( tmp_vm->Use_IOThread() );
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
	Refresh_Gamepad_List( tmp_vm->Get_Gamepad_Filter_IDs() );'''

if old not in t:
    raise SystemExit('load block missing')
t = t.replace(old, new, 1)
cpp.write_text(t, encoding='utf-8')
print('Main_Window wired')
