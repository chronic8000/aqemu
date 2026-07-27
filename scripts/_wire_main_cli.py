# Wire Main_Window for CLI parity fields + help browser
from pathlib import Path

cpp = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\Main_Window.cpp")
t = cpp.read_text(encoding="utf-8")

if 'QEMU_Help_Browser.h' not in t:
    t = t.replace('#include "About_Window.h"', '#include "About_Window.h"\n#include "QEMU_Help_Browser.h"')

# apply save - after RTC clock block
old = '''\t{
		const int idx = ui_ao.CB_RTC_Clock->currentIndex();
		if( idx == 1 ) tmp_vm->Set_RTC_Clock( QStringLiteral( "vm" ) );
		else if( idx == 2 ) tmp_vm->Set_RTC_Clock( QStringLiteral( "rt" ) );
		else tmp_vm->Set_RTC_Clock( QStringLiteral( "host" ) );
	}
	tmp_vm->Use_Snapshot_Mode( ui.CH_Snapshot->isChecked() );'''
new = '''\t{
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
	tmp_vm->Use_Snapshot_Mode( ui.CH_Snapshot->isChecked() );'''
if old not in t:
    raise SystemExit('apply RTC block missing')
t = t.replace(old, new, 1)

# display mode apply
old = '''\tif( ui.RB_Display_Embedded->isChecked() )
		tmp_vm->Set_Display_Window_Mode( QStringLiteral( "embedded" ) );
	else if( ui.RB_Display_Native->isChecked() )
		tmp_vm->Set_Display_Window_Mode( QStringLiteral( "native" ) );
	else
		tmp_vm->Set_Display_Window_Mode( QStringLiteral( "auto" ) );'''
# find actual
import re
m = re.search(r'if\( ui\.RB_Display_Embedded.*?Set_Display_Window_Mode.*?auto.*?\);', t, re.S)
if not m:
    raise SystemExit('display apply missing')
print('FOUND DISPLAY APPLY', m.group(0)[:200])
old_disp = m.group(0)
new_disp = '''if( ui.RB_Display_Nographic->isChecked() )
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
	}'''
t = t.replace(old_disp, new_disp, 1)

# load UI
old = '''\t{
		const QString clk = tmp_vm->Get_RTC_Clock().trimmed().toLower();
		if( clk == QLatin1String( "vm" ) ) ui_ao.CB_RTC_Clock->setCurrentIndex( 1 );
		else if( clk == QLatin1String( "rt" ) ) ui_ao.CB_RTC_Clock->setCurrentIndex( 2 );
		else ui_ao.CB_RTC_Clock->setCurrentIndex( 0 );
	}
	Refresh_Gamepad_List( tmp_vm->Get_Gamepad_Filter_IDs() );'''
new = '''\t{
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
	Refresh_Gamepad_List( tmp_vm->Get_Gamepad_Filter_IDs() );'''
if old not in t:
    raise SystemExit('load RTC missing')
t = t.replace(old, new, 1)

# load display radios - find block
m = re.search(r'const QString m = tmp_vm->Get_Display_Window_Mode\(\).*?Update_Display_Window_Mode_Hint\(\);', t, re.S)
if not m:
    raise SystemExit('load display missing')
print('LOAD DISP', m.group(0)[:300])
old_ld = m.group(0)
new_ld = '''const QString m = tmp_vm->Get_Display_Window_Mode().trimmed().toLower();
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
		Update_Display_Window_Mode_Hint();'''
t = t.replace(old_ld, new_ld, 1)

# connect nographic radio
old = '''\tconnect( ui.RB_Display_Auto, SIGNAL(toggled(bool)),
			 this, SLOT(On_Display_Window_Mode_Toggled(bool)) );'''
# find all three connects
if 'RB_Display_Nographic' not in t.split('On_Display_Window_Mode_Toggled')[0][-500:]:
    t = t.replace(
        'connect( ui.RB_Display_Native, SIGNAL(toggled(bool)),\n'
        '\t\t\t this, SLOT(On_Display_Window_Mode_Toggled(bool)) );',
        'connect( ui.RB_Display_Native, SIGNAL(toggled(bool)),\n'
        '\t\t\t this, SLOT(On_Display_Window_Mode_Toggled(bool)) );\n'
        '\tconnect( ui.RB_Display_Nographic, SIGNAL(toggled(bool)),\n'
        '\t\t\t this, SLOT(On_Display_Window_Mode_Toggled(bool)) );',
        1,
    )

# About slot - add help browser nearby
if 'on_actionQEMU_Help_Browser_triggered' not in t:
    about = 'void Main_Window::on_actionAbout_AQEMU_triggered()'
    # find and add after about function
    idx = t.find('void Main_Window::on_actionAbout_AQEMU_triggered()')
    if idx < 0:
        raise SystemExit('about missing')
    # find end of function - next void Main_Window
    end = t.find('\nvoid Main_Window::', idx + 10)
    insert = '''

void Main_Window::on_actionQEMU_Help_Browser_triggered()
{
	QEMU_Help_Browser dlg( this );
	dlg.exec();
}
'''
    t = t[:end] + insert + t[end:]

cpp.write_text(t, encoding='utf-8')
print('Main_Window.cpp wired')

# header slot
h = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\Main_Window.h")
ht = h.read_text(encoding='utf-8')
if 'on_actionQEMU_Help_Browser_triggered' not in ht:
    ht = ht.replace(
        'void on_TB_Show_Advanced_Options_Window_clicked();',
        'void on_TB_Show_Advanced_Options_Window_clicked();\n'
        '\t\tvoid on_actionQEMU_Help_Browser_triggered();',
    )
    h.write_text(ht, encoding='utf-8')
print('header OK')
