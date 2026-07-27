# Wire wave3 UI into Main_Window.cpp
from pathlib import Path
cpp = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\Main_Window.cpp")
t = cpp.read_text(encoding="utf-8")

old = '''\ttmp_vm->Use_Modern_Chardev( ui_ao.CH_Modern_Chardev->isChecked() );
\ttmp_vm->Use_Blockdev( ui_ao.CH_Use_Blockdev->isChecked() );
\ttmp_vm->Use_Snapshot_Mode( ui.CH_Snapshot->isChecked() );'''

new = '''\ttmp_vm->Use_Modern_Chardev( ui_ao.CH_Modern_Chardev->isChecked() );
\ttmp_vm->Use_Blockdev( ui_ao.CH_Use_Blockdev->isChecked() );
\ttmp_vm->Use_NUMA_Memdev( ui_ao.CH_NUMA_Memdev->isChecked() );
\ttmp_vm->Use_SMBIOS_Type0( ui_ao.CH_SMBIOS_Type0->isChecked() );
\ttmp_vm->Set_SMBIOS_Vendor( ui_ao.Edit_SMBIOS_Vendor->text() );
\ttmp_vm->Set_SMBIOS_Version( ui_ao.Edit_SMBIOS_Version->text() );
\ttmp_vm->Set_SMBIOS_Date( ui_ao.Edit_SMBIOS_Date->text() );
\ttmp_vm->Use_SMBIOS_Type1( ui_ao.CH_SMBIOS_Type1->isChecked() );
\ttmp_vm->Set_SMBIOS_Manufacturer( ui_ao.Edit_SMBIOS_Manufacturer->text() );
\ttmp_vm->Set_SMBIOS_Product( ui_ao.Edit_SMBIOS_Product->text() );
\ttmp_vm->Set_SMBIOS_Type1_Version( ui_ao.Edit_SMBIOS_Type1_Version->text() );
\ttmp_vm->Set_SMBIOS_Serial( ui_ao.Edit_SMBIOS_Serial->text() );
\ttmp_vm->Set_SMBIOS_File( ui_ao.Edit_SMBIOS_File->text().trimmed() );
\ttmp_vm->Set_FW_CFG_Lines( ui_ao.Edit_FW_CFG_Lines->toPlainText() );
\ttmp_vm->Set_ICount( ui_ao.Edit_ICount->text() );
\ttmp_vm->Set_Sandbox( ui_ao.Edit_Sandbox->text() );
\ttmp_vm->Use_Snapshot_Mode( ui.CH_Snapshot->isChecked() );'''

if old not in t:
    raise SystemExit('ao apply missing')
t = t.replace(old, new, 1)

# Audio apply after Set_Audio_Cards
old = '''\ttmp_vm->Set_Audio_Cards( snd_card );'''
# find unique context
idx = t.find(old)
if idx < 0:
    raise SystemExit('audio apply missing')
# only first occurrence after snd_card setup is fine
insert = '''\ttmp_vm->Set_Audio_Cards( snd_card );
\t{
\t\tconst int ai = ui.CB_Audiodev_Backend->currentIndex();
\t\ttmp_vm->Set_Audiodev_Backend( ai <= 0 ? QString() : ui.CB_Audiodev_Backend->currentText() );
\t\ttmp_vm->Set_Audiodev_Timer_Period( ui.SB_Audiodev_Timer_Period->value() );
\t}'''
# Avoid double-insert
if 'Set_Audiodev_Backend' not in t:
    t = t.replace(old, insert, 1)

old = '''\tui_ao.CH_Modern_Chardev->setChecked( tmp_vm->Use_Modern_Chardev() );
\tui_ao.CH_Use_Blockdev->setChecked( tmp_vm->Use_Blockdev() );
\tRefresh_Gamepad_List( tmp_vm->Get_Gamepad_Filter_IDs() );'''

new = '''\tui_ao.CH_Modern_Chardev->setChecked( tmp_vm->Use_Modern_Chardev() );
\tui_ao.CH_Use_Blockdev->setChecked( tmp_vm->Use_Blockdev() );
\tui_ao.CH_NUMA_Memdev->setChecked( tmp_vm->Use_NUMA_Memdev() );
\tui_ao.CH_SMBIOS_Type0->setChecked( tmp_vm->Use_SMBIOS_Type0() );
\tui_ao.Edit_SMBIOS_Vendor->setText( tmp_vm->Get_SMBIOS_Vendor() );
\tui_ao.Edit_SMBIOS_Version->setText( tmp_vm->Get_SMBIOS_Version() );
\tui_ao.Edit_SMBIOS_Date->setText( tmp_vm->Get_SMBIOS_Date() );
\tui_ao.CH_SMBIOS_Type1->setChecked( tmp_vm->Use_SMBIOS_Type1() );
\tui_ao.Edit_SMBIOS_Manufacturer->setText( tmp_vm->Get_SMBIOS_Manufacturer() );
\tui_ao.Edit_SMBIOS_Product->setText( tmp_vm->Get_SMBIOS_Product() );
\tui_ao.Edit_SMBIOS_Type1_Version->setText( tmp_vm->Get_SMBIOS_Type1_Version() );
\tui_ao.Edit_SMBIOS_Serial->setText( tmp_vm->Get_SMBIOS_Serial() );
\tui_ao.Edit_SMBIOS_File->setText( tmp_vm->Get_SMBIOS_File() );
\tui_ao.Edit_FW_CFG_Lines->setPlainText( tmp_vm->Get_FW_CFG_Lines() );
\tui_ao.Edit_ICount->setText( tmp_vm->Get_ICount() );
\tui_ao.Edit_Sandbox->setText( tmp_vm->Get_Sandbox() );
\tRefresh_Gamepad_List( tmp_vm->Get_Gamepad_Filter_IDs() );'''

if old not in t:
    raise SystemExit('ao load missing')
t = t.replace(old, new, 1)

# Audio load — after USB audio checkbox
old = '''\tif( tmp_vm->Get_Audio_Cards().Audio_USB ) ui.CH_USB_Audio->setChecked( true );'''
# Need more context - check what follows
if 'CB_Audiodev_Backend' not in t.split(old, 1)[-1][:800] if old in t else True:
    pass

marker = '''\tif( tmp_vm->Get_Audio_Cards().Audio_USB ) ui.CH_USB_Audio->setChecked( true );
\telse ui.CH_USB_Audio->setChecked( false );'''
audio_load = marker + '''
\t{
\t\tconst QString ab = tmp_vm->Get_Audiodev_Backend();
\t\tint ai = 0;
\t\tif( ! ab.isEmpty() )
\t\t{
\t\t\tai = ui.CB_Audiodev_Backend->findText( ab );
\t\t\tif( ai < 0 ) ai = 0;
\t\t}
\t\tui.CB_Audiodev_Backend->setCurrentIndex( ai );
\t\tui.SB_Audiodev_Timer_Period->setValue( tmp_vm->Get_Audiodev_Timer_Period() );
\t}'''
if 'Get_Audiodev_Backend' not in t:
    if marker not in t:
        raise SystemExit('audio load marker missing')
    t = t.replace(marker, audio_load, 1)

cpp.write_text(t, encoding='utf-8')
print('Main_Window wired')
