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

#include <QDir>
#include <QRegExp>
#include <QFileDialog>
#include <QLabel>
#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QToolButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QTreeWidget>
#include <QListWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QScrollArea>
#include <QSizePolicy>
#include <QFrame>

#include "Utils.h"
#include "WSL_Launch.h"
#include "VM_Wizard_Window.h"
#include "System_Info.h"
#include "VM_Devices.h"

VM_Wizard_Window::VM_Wizard_Window( QWidget *parent )
	: QDialog(parent)
{
	ui.setupUi( this );
	
	New_VM = new Virtual_Machine();
	Win11_ARM_Page = nullptr;
	Creation_Method_Page = nullptr;
	OS_Tree_Page = nullptr;
	Platform_Tree_Page = nullptr;
	Arch_List_Page = nullptr;
	Arch_Machines_Page = nullptr;
	RB_Method_Guest_OS = nullptr;
	RB_Method_Platform = nullptr;
	RB_Method_Architecture = nullptr;
	RB_Method_Custom = nullptr;
	Tree_OS = nullptr;
	Tree_Platform = nullptr;
	List_Arch = nullptr;
	Tree_Arch_Machines = nullptr;
	Current_Method = Method_None;
	Three_Path_Active = false;
	Current_Devices = nullptr;
	Use_Accelerator_Page = true;
	Guest_RAM_MB = 2048;
	Guest_HDD_GB = 20.0;
	Guest_NIC_Model = "e1000";
	Guest_Sound = VM::Sound_Cards();
	Guest_Sound.Audio_HDA = true;
	Guest_Suggest_Win2K_Hack = false;
	Label_Guest_Compat_Tip = nullptr;
	Label_Arch_Summary = nullptr;
	
	// Hide release date widgets
	ui.Label_Relese_Date->hide();
	ui.CB_Relese_Date->hide();
	
	Build_Three_Path_Pages();
	Build_Windows11_ARM_Page();
	Build_Intel_MacOS_Page();

	// Summary + tip labels on Template / Architecture page
	Label_Arch_Summary = new QLabel( ui.Template_Page );
	Label_Arch_Summary->setWordWrap( true );
	Label_Arch_Summary->setStyleSheet( "QLabel { color: #223; font-weight: bold; padding: 6px; }" );
	Label_Guest_Compat_Tip = new QLabel( ui.Template_Page );
	Label_Guest_Compat_Tip->setWordWrap( true );
	Label_Guest_Compat_Tip->setStyleSheet( "QLabel { color: #335; padding: 6px; }" );
	if( QGridLayout *gl = qobject_cast<QGridLayout*>( ui.Template_Page->layout() ) )
	{
		gl->addWidget( Label_Arch_Summary, 19, 0, 1, 2 );
		gl->addWidget( Label_Guest_Compat_Tip, 20, 0, 1, 2 );
	}
	Label_Arch_Summary->hide();
	Label_Guest_Compat_Tip->hide();

	// Fixed wizard size — do not grow/shrink when changing pages
	setMinimumSize( 640, 620 );
	resize( 640, 620 );

	// Loading All Templates
	if( Load_OS_Templates() )
	{
		// Find default template
		for( int ix = 0; ix < ui.CB_OS_Type->count(); ++ix )
		{
			if( ui.CB_OS_Type->itemText(ix) == Settings.value("Default_VM_Template", "Linux 2.6").toString() )
				ui.CB_OS_Type->setCurrentIndex( ix );
		}
	}
	else
	{
		AQWarning( "void VM_Wizard_Window::on_Button_Next_clicked()",
				   "No VM Templates Found!" );
	}

	ui.Wizard_Pages->setCurrentWidget( Creation_Method_Page );
	ui.Label_Page->setText( tr("Creation Method") );
	ui.Button_Back->setEnabled( false );

    connect(ui.RB_Emulator_KVM, SIGNAL(toggled(bool)),this, SLOT(KVM_toggled(bool)));
}

void VM_Wizard_Window::Build_Three_Path_Pages()
{
	Load_Wizard_Trees();

	// --- Creation method ---
	Creation_Method_Page = new QWidget();
	QVBoxLayout *methodLay = new QVBoxLayout( Creation_Method_Page );
	methodLay->addWidget( new QLabel( tr(
		"<b>How do you want to create this virtual machine?</b>") ) );
	RB_Method_Guest_OS = new QRadioButton( tr("Guest Operating System") );
	RB_Method_Platform = new QRadioButton( tr("System / Machine Platform") );
	RB_Method_Architecture = new QRadioButton( tr("CPU Architecture") );
	RB_Method_Custom = new QRadioButton( tr("Custom / Advanced") );
	RB_Method_Guest_OS->setChecked( true );
	methodLay->addWidget( RB_Method_Guest_OS );
	methodLay->addWidget( RB_Method_Platform );
	methodLay->addWidget( RB_Method_Architecture );
	methodLay->addWidget( RB_Method_Custom );
	methodLay->addStretch( 1 );
	ui.Wizard_Pages->insertWidget( 0, Creation_Method_Page );

	// --- Guest OS / Select OS tree ---
	OS_Tree_Page = new QWidget();
	QVBoxLayout *osLay = new QVBoxLayout( OS_Tree_Page );
	osLay->addWidget( new QLabel( tr("Select a guest operating system:") ) );
	Tree_OS = new QTreeWidget();
	Tree_OS->setHeaderHidden( true );
	Tree_OS->setRootIsDecorated( true );
	osLay->addWidget( Tree_OS );
	Populate_OS_Tree();
	ui.Wizard_Pages->addWidget( OS_Tree_Page );

	// --- Platform tree ---
	Platform_Tree_Page = new QWidget();
	QVBoxLayout *platLay = new QVBoxLayout( Platform_Tree_Page );
	platLay->addWidget( new QLabel( tr("Select a system / machine platform:") ) );
	Tree_Platform = new QTreeWidget();
	Tree_Platform->setHeaderHidden( true );
	platLay->addWidget( Tree_Platform );
	Populate_Platform_Tree();
	ui.Wizard_Pages->addWidget( Platform_Tree_Page );

	// --- Architecture list ---
	Arch_List_Page = new QWidget();
	QVBoxLayout *archLay = new QVBoxLayout( Arch_List_Page );
	archLay->addWidget( new QLabel( tr("Select a CPU architecture:") ) );
	List_Arch = new QListWidget();
	archLay->addWidget( List_Arch );
	Populate_Arch_List();
	ui.Wizard_Pages->addWidget( Arch_List_Page );

	// --- Machines filtered by arch ---
	Arch_Machines_Page = new QWidget();
	QVBoxLayout *machLay = new QVBoxLayout( Arch_Machines_Page );
	machLay->addWidget( new QLabel( tr("Select a machine (filtered by architecture):") ) );
	Tree_Arch_Machines = new QTreeWidget();
	Tree_Arch_Machines->setHeaderHidden( true );
	machLay->addWidget( Tree_Arch_Machines );
	ui.Wizard_Pages->addWidget( Arch_Machines_Page );
}

bool VM_Wizard_Window::Load_Wizard_Trees()
{
	QFile f( ":/wizard_trees.json" );
	if( ! f.open( QIODevice::ReadOnly ) )
	{
		AQWarning( "VM_Wizard_Window::Load_Wizard_Trees()",
				   "Cannot open :/wizard_trees.json" );
		return false;
	}
	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson( f.readAll(), &err );
	f.close();
	if( err.error != QJsonParseError::NoError || ! doc.isObject() )
	{
		AQWarning( "VM_Wizard_Window::Load_Wizard_Trees()",
				   "Invalid wizard_trees.json: " + err.errorString() );
		return false;
	}
	Wizard_Trees = doc.object();
	return true;
}

void VM_Wizard_Window::Populate_OS_Tree()
{
	if( ! Tree_OS ) return;
	Tree_OS->clear();
	QJsonObject os = Wizard_Trees.value( "operating_systems" ).toObject();
	QStringList families = os.keys();
	families.sort( Qt::CaseInsensitive );
	for( const QString &fam : families )
	{
		QTreeWidgetItem *family = new QTreeWidgetItem( Tree_OS );
		family->setText( 0, fam );
		family->setFlags( family->flags() & ~Qt::ItemIsSelectable );
		QJsonArray children = os.value( fam ).toArray();
		// Keep JSON order (Microsoft is chronological; others are A–Z in the file)
		for( int i = 0; i < children.size(); ++i )
		{
			QTreeWidgetItem *leaf = new QTreeWidgetItem( family );
			leaf->setText( 0, children.at(i).toString() );
		}
	}
	Tree_OS->expandToDepth( 0 );
}

void VM_Wizard_Window::Populate_Platform_Tree()
{
	if( ! Tree_Platform ) return;
	Tree_Platform->clear();
	QJsonObject plats = Wizard_Trees.value( "platforms" ).toObject();
	for( QJsonObject::const_iterator it = plats.constBegin(); it != plats.constEnd(); ++it )
	{
		QTreeWidgetItem *group = new QTreeWidgetItem( Tree_Platform );
		group->setText( 0, it.key() );
		group->setFlags( group->flags() & ~Qt::ItemIsSelectable );
		QJsonArray children = it.value().toArray();
		for( int i = 0; i < children.size(); ++i )
		{
			QTreeWidgetItem *leaf = new QTreeWidgetItem( group );
			leaf->setText( 0, children.at(i).toString() );
		}
	}
	Tree_Platform->expandToDepth( 0 );
}

void VM_Wizard_Window::Populate_Arch_List()
{
	if( ! List_Arch ) return;
	List_Arch->clear();
	QJsonArray arches = Wizard_Trees.value( "architectures" ).toArray();
	for( int i = 0; i < arches.size(); ++i )
		List_Arch->addItem( arches.at(i).toString() );
	if( List_Arch->count() > 0 )
		List_Arch->setCurrentRow( 1 ); // x86-64 by default if present
}

void VM_Wizard_Window::Populate_Arch_Machines( const QString &arch_display )
{
	if( ! Tree_Arch_Machines ) return;
	Tree_Arch_Machines->clear();

	QJsonObject targets = Wizard_Trees.value( "architecture_targets" ).toObject();
	QString target = targets.value( arch_display ).toString();
	if( target.isEmpty() )
		return;
	Selected_Target = target;
	Selected_Arch_Name = arch_display;

	QTreeWidgetItem *root = new QTreeWidgetItem( Tree_Arch_Machines );
	root->setText( 0, arch_display );
	root->setFlags( root->flags() & ~Qt::ItemIsSelectable );

	QJsonObject bindings = Wizard_Trees.value( "platform_bindings" ).toObject();
	for( QJsonObject::const_iterator it = bindings.constBegin(); it != bindings.constEnd(); ++it )
	{
		QJsonObject b = it.value().toObject();
		if( b.value( "target" ).toString() != target )
			continue;
		QTreeWidgetItem *leaf = new QTreeWidgetItem( root );
		leaf->setText( 0, it.key() );
		leaf->setData( 0, Qt::UserRole, b.value( "machine" ).toString() );
		leaf->setData( 0, Qt::UserRole + 1, target );
	}

	// Optional: expand from probed catalog for this binary
	QStringList catalogPaths;
	catalogPaths << QCoreApplication::applicationDirPath() + "/qemu_machine_catalog.json"
	             << QCoreApplication::applicationDirPath() + "/../docs/qemu_machine_catalog.json"
	             << QDir::currentPath() + "/docs/qemu_machine_catalog.json";
	for( int p = 0; p < catalogPaths.size(); ++p )
	{
		QFile cf( catalogPaths[p] );
		if( ! cf.open( QIODevice::ReadOnly ) )
			continue;
		QJsonDocument cdoc = QJsonDocument::fromJson( cf.readAll() );
		cf.close();
		if( ! cdoc.isObject() )
			continue;
		QJsonArray binaries = cdoc.object().value( "binaries" ).toArray();
		for( int bi = 0; bi < binaries.size(); ++bi )
		{
			QJsonObject bo = binaries.at(bi).toObject();
			if( bo.value( "target" ).toString() != target )
				continue;
			QJsonArray machines = bo.value( "machines" ).toArray();
			QTreeWidgetItem *more = new QTreeWidgetItem( root );
			more->setText( 0, tr("All available machines…") );
			more->setFlags( more->flags() & ~Qt::ItemIsSelectable );
			for( int mi = 0; mi < machines.size(); ++mi )
			{
				QJsonObject m = machines.at(mi).toObject();
				if( m.value( "hidden" ).toBool() )
					continue;
				QString display = m.value( "display_name" ).toString();
				if( display.isEmpty() )
					display = m.value( "name" ).toString();
				QTreeWidgetItem *leaf = new QTreeWidgetItem( more );
				leaf->setText( 0, display );
				leaf->setData( 0, Qt::UserRole, m.value( "name" ).toString() );
				leaf->setData( 0, Qt::UserRole + 1, target );
			}
			break;
		}
		break;
	}

	Tree_Arch_Machines->expandToDepth( 0 );
}

QString VM_Wizard_Window::Selected_Tree_Leaf( QTreeWidget *tree ) const
{
	if( ! tree || ! tree->currentItem() )
		return QString();
	QTreeWidgetItem *item = tree->currentItem();
	if( item->childCount() > 0 )
		return QString(); // group, not leaf
	return item->text( 0 );
}

bool VM_Wizard_Window::Ensure_Emulator_Ready()
{
	Current_Emulator = Get_Default_Emulator();
	All_Systems = Current_Emulator.Get_Devices();
	if( All_Systems.isEmpty() )
	{
		AQGraphic_Warning( tr("Emulator"), tr("Cannot get emulator devices. Check QEMU installation.") );
		return false;
	}

	ui.CB_Computer_Type->clear();
	for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); ++it )
		ui.CB_Computer_Type->addItem( it.value().System.Caption );

	return true;
}

bool VM_Wizard_Window::Apply_Selected_Computer_Type( const QString &target )
{
	QString qemu_name = "qemu-system-" + target;
	if( ! All_Systems.contains( qemu_name ) )
	{
		// Prefer exact qemu-system-<target>; only then fall back to suffix match
		bool found = false;
		const QString suffix = QStringLiteral( "qemu-system-" ) + target;
		for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); ++it )
		{
			if( it.key().compare( suffix, Qt::CaseInsensitive ) == 0 ||
			    it.value().System.QEMU_Name.compare( suffix, Qt::CaseInsensitive ) == 0 )
			{
				qemu_name = it.key();
				found = true;
				break;
			}
		}
		if( ! found )
		{
			for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); ++it )
			{
				if( it.key().endsWith( target, Qt::CaseInsensitive ) ||
				    it.value().System.QEMU_Name.endsWith( target, Qt::CaseInsensitive ) )
				{
					qemu_name = it.key();
					found = true;
					break;
				}
			}
		}
		if( ! found )
		{
			AQGraphic_Warning( tr("Architecture"),
				tr("No QEMU binary for target \"%1\" is configured in AQEMU.\nExpected: %2")
					.arg( target ).arg( "qemu-system-" + target ) );
			return false;
		}
	}

	Current_Devices = &All_Systems[ qemu_name ];
	New_VM->Set_Computer_Type( Current_Devices->System.QEMU_Name );
	// Keep Selected_Target aligned with the resolved binary (not a fuzzy partial)
	Selected_Target = Current_Devices->System.QEMU_Name;
	Selected_Target.remove( QStringLiteral( "qemu-system-" ) );

	// Sync Architecture list label when known
	{
		QJsonObject targets = Wizard_Trees.value( "architecture_targets" ).toObject();
		for( QJsonObject::const_iterator it = targets.constBegin(); it != targets.constEnd(); ++it )
		{
			if( it.value().toString() == Selected_Target )
			{
				Selected_Arch_Name = it.key();
				break;
			}
		}
	}

	// Sync combo for Generate path — set Computer Type BEFORE checking Generate,
	// so on_RB_Generate_VM_toggled cannot stomp a 32-bit OS default with 64-bit.
	bool matched = false;
	for( int ix = 0; ix < ui.CB_Computer_Type->count(); ++ix )
	{
		if( ui.CB_Computer_Type->itemText(ix) == Current_Devices->System.Caption )
		{
			ui.CB_Computer_Type->setCurrentIndex( ix );
			matched = true;
			break;
		}
	}
	ui.RB_Generate_VM->setChecked( true );
	if( ! matched )
	{
		// Caption may differ after emulator probe; try QEMU name substring
		for( int ix = 0; ix < ui.CB_Computer_Type->count(); ++ix )
		{
			const QString text = ui.CB_Computer_Type->itemText( ix );
			if( text.contains( Selected_Target, Qt::CaseInsensitive ) ||
			    ( Selected_Target == QLatin1String( "i386" ) &&
			      ( text.contains( QLatin1String( "32Bit" ), Qt::CaseInsensitive ) ||
			        text.contains( QLatin1String( "i386" ), Qt::CaseInsensitive ) ) ) ||
			    ( Selected_Target == QLatin1String( "x86_64" ) &&
			      ( text.contains( QLatin1String( "64Bit" ), Qt::CaseInsensitive ) ||
			        text.contains( QLatin1String( "x86_64" ), Qt::CaseInsensitive ) ) ) )
			{
				// Avoid matching x86_64 when looking for i386 via "x86"
				if( Selected_Target == QLatin1String( "i386" ) &&
				    ( text.contains( QLatin1String( "64Bit" ), Qt::CaseInsensitive ) ||
				      text.contains( QLatin1String( "x86_64" ), Qt::CaseInsensitive ) ) )
					continue;
				ui.CB_Computer_Type->setCurrentIndex( ix );
				matched = true;
				break;
			}
		}
	}
	return true;
}

void VM_Wizard_Window::Apply_Platform_Binding( const QString &platform_display )
{
	Selected_Platform_Name = platform_display;
	QJsonObject bindings = Wizard_Trees.value( "platform_bindings" ).toObject();
	QJsonObject b = bindings.value( platform_display ).toObject();
	if( b.isEmpty() )
	{
		// Catalog-picked leaf may already store machine/target on tree item
		QTreeWidgetItem *item = Tree_Platform->currentItem();
		if( ! item && Tree_Arch_Machines )
			item = Tree_Arch_Machines->currentItem();
		if( item )
		{
			Selected_Machine_Id = item->data( 0, Qt::UserRole ).toString();
			QString t = item->data( 0, Qt::UserRole + 1 ).toString();
			if( ! t.isEmpty() )
				Selected_Target = t;
		}
		if( Selected_Machine_Id.isEmpty() )
			Selected_Machine_Id = "virt";
		if( Selected_Target.isEmpty() )
			Selected_Target = "x86_64";
	}
	else
	{
		Selected_Target = b.value( "target" ).toString( "x86_64" );
		Selected_Machine_Id = b.value( "machine" ).toString( "virt" );
	}
	New_VM->Set_Machine_Type( Selected_Machine_Id );
}

void VM_Wizard_Window::Apply_Sound_Preset( const QString &preset )
{
	Guest_Sound = VM::Sound_Cards();
	if( preset == "none" )
		return;
	if( preset == "sb16" )
		Guest_Sound.Audio_sb16 = true;
	else if( preset == "sb16_adlib_pcspk" )
	{
		Guest_Sound.Audio_sb16 = true;
		Guest_Sound.Audio_Adlib = true;
		Guest_Sound.Audio_PC_Speaker = true;
	}
	else if( preset == "sb16_es1370_pcspk" )
	{
		Guest_Sound.Audio_sb16 = true;
		Guest_Sound.Audio_es1370 = true;
		Guest_Sound.Audio_PC_Speaker = true;
	}
	else if( preset == "es1370" )
		Guest_Sound.Audio_es1370 = true;
	else if( preset == "es1370_ac97" )
	{
		Guest_Sound.Audio_es1370 = true;
		Guest_Sound.Audio_AC97 = true;
	}
	else if( preset == "ac97" )
		Guest_Sound.Audio_AC97 = true;
	else if( preset == "pcspk" )
		Guest_Sound.Audio_PC_Speaker = true;
	else if( preset == "virtio" )
		Guest_Sound.Audio_VirtIO = true;
	else if( preset == "hda_virtio" )
	{
		Guest_Sound.Audio_HDA = true;
		Guest_Sound.Audio_VirtIO = true;
	}
	else // hda (default modern)
		Guest_Sound.Audio_HDA = true;
}

void VM_Wizard_Window::Apply_OS_Defaults( const QString &os_name )
{
	Selected_OS_Name = os_name;
	if( ui.Edit_VM_Name->text().isEmpty() || ui.Edit_VM_Name->text().startsWith( "Virtual Machine" ) )
		ui.Edit_VM_Name->setText( os_name );

	Guest_Suggest_Win2K_Hack = false;
	Guest_Sound = VM::Sound_Cards();
	Guest_RAM_MB = 2048;
	Guest_HDD_GB = 20.0;
	Guest_NIC_Model = "e1000";
	Guest_Compat_Tip.clear();

	const QString host = AQ_Get_Host_CPU_Architecture();
	const bool host_arm = ( host == "aarch64" );


	// Only invent target/machine for Guest-OS path (Platform/Arch already chose hardware)
	if( Current_Method != Method_Guest_OS )
	{
		Guest_Compat_Tip = tr( "Select or confirm a guest OS. Architecture stays fully editable." );
		Update_Guest_Compat_Tip();
		return;
	}

	QJsonObject profiles = Wizard_Trees.value( "os_profiles" ).toObject();
	QJsonObject profile = profiles.value( os_name ).toObject();

	if( profile.isEmpty() )
	{
		Selected_Target = host_arm ? "aarch64" : "x86_64";
		Selected_Machine_Id = host_arm ? "virt" : "q35";
		Guest_RAM_MB = 2048;
		Guest_Compat_Tip = tr( "No profile for \"%1\" — using host architecture default. "
		                       "Computer Type remains fully editable." ).arg( os_name );
	}
	else
	{
		const QJsonArray flags = profile.value( "flags" ).toArray();
		auto has_flag = [ &flags ]( const char *name ) -> bool {
			for( const QJsonValue &v : flags )
			{
				if( v.toString() == QLatin1String( name ) )
					return true;
			}
			return false;
		};

		Selected_Target = profile.value( "target" ).toString( "x86_64" );
		Selected_Machine_Id = profile.value( "machine" ).toString( "q35" );
		Guest_RAM_MB = profile.value( "ram_mb" ).toInt( 2048 );
		Guest_HDD_GB = profile.value( "hdd_gb" ).toDouble( 20.0 );
		Guest_NIC_Model = profile.value( "nic" ).toString( "e1000" );
		Apply_Sound_Preset( profile.value( "sound" ).toString( "hda" ) );
		Guest_Compat_Tip = profile.value( "tip" ).toString();

		if( has_flag( "win2k_hack" ) )
			Guest_Suggest_Win2K_Hack = true;

		if( has_flag( "host_default" ) || Selected_Target == "host" )
		{
			Selected_Target = host_arm ? "aarch64" : "x86_64";
			Selected_Machine_Id = host_arm ? "virt" : "q35";
		}

		// Windows 11 on ARM host → AArch64 + virt guided path
		if( has_flag( "win11_host_arm" ) && host_arm )
		{
			Selected_Target = "aarch64";
			Selected_Machine_Id = "virt";
			Guest_RAM_MB = 8192;
			Guest_HDD_GB = 64.0;
			Guest_NIC_Model = "virtio-net-pci";
			Guest_Sound = VM::Sound_Cards();
			Guest_Sound.Audio_USB = true;
			Guest_Sound.Audio_VirtIO = true;
			Guest_Compat_Tip = tr( "Windows 11 on ARM host → AArch64 + virt (VirtIO). Architecture remains editable." );
		}
	}

	New_VM->Set_Machine_Type( Selected_Machine_Id );
	Update_Guest_Compat_Tip();
}

void VM_Wizard_Window::Update_Architecture_Page_Chrome()
{
	const bool three_path_generate = Three_Path_Active && ui.RB_Generate_VM->isChecked();

	// Hide unused legacy template row when Guest OS / Platform / Arch already chose the guest
	ui.RB_VM_Template->setVisible( ! three_path_generate );
	ui.Label_OS_Type->setVisible( ! three_path_generate );
	ui.CB_OS_Type->setVisible( ! three_path_generate );
	if( QWidget *line = ui.Template_Page->findChild<QWidget*>( "line" ) )
		line->setVisible( ! three_path_generate );

	if( three_path_generate )
	{
		ui.RB_Generate_VM->setText( tr( "&QEMU system (editable)" ) );
		ui.Label_Computer_Type->setText( tr( "Comp&uter Type:" ) );
	}
	else
	{
		ui.RB_Generate_VM->setText( tr( "&Generate VM (any QEMU arch)" ) );
	}

	if( ! Label_Arch_Summary )
		return;

	if( ! three_path_generate || Selected_Target.isEmpty() )
	{
		Label_Arch_Summary->hide();
		return;
	}

	QString guest = Selected_OS_Name;
	if( guest.isEmpty() )
		guest = Selected_Platform_Name;
	if( guest.isEmpty() )
		guest = Selected_Arch_Name;
	if( guest.isEmpty() )
		guest = tr( "Custom" );

	const QString machine = Selected_Machine_Id.isEmpty()
		? tr( "(default)" )
		: Selected_Machine_Id;

	Label_Arch_Summary->setText( tr( "Guest: %1 → qemu-system-%2 + machine %3" )
		.arg( guest )
		.arg( Selected_Target )
		.arg( machine ) );
	Label_Arch_Summary->show();
}

void VM_Wizard_Window::Update_Guest_Compat_Tip()
{
	if( ! Label_Guest_Compat_Tip )
		return;

	QString tip = Guest_Compat_Tip;
	const QString archCaption = ui.CB_Computer_Type->currentText();
	const QString os = Selected_OS_Name;

	const bool legacy_x86_32 =
		os == "Windows 95" || os == "Windows 98" || os == "Windows ME" ||
		os == "Windows 1.x" || os == "Windows 2.x" || os == "Windows 3.x" ||
		os == "Windows NT 3.x" || os == "Windows NT 4.0" ||
		os == "MS-DOS" || os == "PC DOS" || os == "DR-DOS" ||
		os == "Windows 2000" ||
		os == "Windows Server 2000" || os == "Windows Server 2003" ||
		os == "Solaris x86 (32-bit)" || os == "OS/2" || os == "eComStation" ||
		os == "ArcaOS" || os == "BeOS" ||
		os == "KolibriOS" || os == "TempleOS" ||
		os.contains( QLatin1String( "(32-bit)" ) ) ||
		( os.startsWith( QLatin1String( "Windows XP" ) ) && ! os.contains( QLatin1String( "64-bit" ) ) ) ||
		( os.startsWith( QLatin1String( "ReactOS" ) ) && ! os.contains( QLatin1String( "64-bit" ) ) );

	if( legacy_x86_32 &&
	    ( archCaption.contains( "x86_64", Qt::CaseInsensitive ) ||
	      archCaption.contains( "64Bit", Qt::CaseInsensitive ) ) )
	{
		tip += tr( "\n\nWarning: %1 is a 32-bit guest. "
		           "Prefer IBM PC 32Bit / x86 (i386 PC), not 64-bit." )
			.arg( os.isEmpty() ? tr("This OS") : os );
	}

	if( tip.isEmpty() )
	{
		Label_Guest_Compat_Tip->hide();
	}
	else
	{
		Label_Guest_Compat_Tip->setText( tip );
		Label_Guest_Compat_Tip->show();
	}

	Update_Architecture_Page_Chrome();
}

void VM_Wizard_Window::Apply_Guest_Hardware_To_New_VM()
{
	if( ! Three_Path_Active )
		return;

	if( ! Selected_Machine_Id.isEmpty() )
		New_VM->Set_Machine_Type( Selected_Machine_Id );

	if( Guest_Sound.isEnabled() )
		New_VM->Set_Audio_Cards( Guest_Sound );

	if( Guest_Suggest_Win2K_Hack )
		New_VM->Use_Win2K_Hack( true );

	// Pointer device defaults by guest class
	{
		const QString os = Selected_OS_Name;
		const bool legacy_win =
			os == "Windows 95" || os == "Windows 98" || os == "Windows ME" ||
			os == "Windows 1.x" || os == "Windows 2.x" || os == "Windows 3.x" ||
			os == "Windows NT 3.x" || os == "Windows NT 4.0" ||
			os == "MS-DOS" || os == "PC DOS" || os == "DR-DOS" ||
			os == "Windows 2000" || os.startsWith( QLatin1String( "Windows XP" ) ) ||
			os == "Windows Server 2000" || os == "Windows Server 2003";
		// Prior to ME: force pure TCG (WHPX hangs Win98 at splash).
		const bool force_tcg_legacy =
			os == "Windows 95" || os == "Windows 98" ||
			os == "Windows 1.x" || os == "Windows 2.x" || os == "Windows 3.x" ||
			os == "MS-DOS" || os == "PC DOS" || os == "DR-DOS";
		const bool classic_mac =
			os == "Mac OS X PPC" || os == "Mac OS 7" || os == "Mac OS 8" || os == "Mac OS 9";
		const bool modern_need_tablet =
			os.contains( "Windows" ) || os.contains( "Linux" ) || os.contains( "BSD" ) ||
			os.contains( "Haiku" ) || os.contains( "Ubuntu" ) || os.contains( "ChromeOS" ) ||
			os.contains( "Chrome OS Flex" ) || os.contains( "SteamOS" ) ||
			os.contains( "SerenityOS" ) || Selected_Target.contains( "aarch64" );

		const bool xp_family =
			os == "Windows 2000" || os.startsWith( QLatin1String( "Windows XP" ) ) ||
			os == "Windows Server 2000" || os == "Windows Server 2003";
		const bool xp_x64 = os.startsWith( QLatin1String( "Windows XP" ) ) &&
		                    os.contains( QLatin1String( "64-bit" ) );

		if( legacy_win )
		{
			// PS/2 is reliable for Win9x splash/GUI. USB tablet often hangs PnP
			// until the guest USB stack is happy — switch to tablet later if needed.
			New_VM->Set_Mouse_Type( QStringLiteral( "ps2" ) );
			New_VM->Set_Mouse_USB_Controller( QStringLiteral( "uhci" ) );
			New_VM->Set_Mouse_USB_Version( 1 );
			// XP text-mode setup: cirrus + SPICE = black after "inspecting hardware".
			// std VGA + QEMU SDL window (auto display mode) works; Win9x keeps cirrus.
			New_VM->Set_Video_Card( xp_family ? QStringLiteral( "std" )
			                                 : QStringLiteral( "cirrus" ) );
			New_VM->Use_ACPI( false );
			// Win9x: keep FDD boot check (matches proven Win98 .aqemu); XP/2k: off.
			New_VM->Use_Check_FDD_Boot_Sector( ! xp_family );
			New_VM->Set_Display_Window_Mode( xp_family
				? QStringLiteral( "embedded" )
				: QStringLiteral( "auto" ) );

			// No inbox VirtIO drivers — IDE or setup reports "no hard disks".
			VM_HDD hda = New_VM->Get_HDA();
			if( hda.Get_Enabled() )
			{
				VM_Native_Storage_Device native = hda.Get_Native_Device();
				native.Use_Interface( true );
				native.Set_Interface( VM::DI_IDE );
				if( ! native.Use_File_Path() || native.Get_File_Path().trimmed().isEmpty() )
				{
					native.Use_File_Path( true );
					native.Set_File_Path( hda.Get_File_Name() );
				}
				hda.Set_Native_Device( native );
				New_VM->Set_HDA( hda );
			}
		}
		if( xp_family )
		{
			// Dual CPU during XP CD setup commonly hangs after hardware inspection.
			New_VM->Set_SMP_CPU_Count( 1 );
			New_VM->Set_CPU_Type( xp_x64 ? QStringLiteral( "qemu64" )
			                            : QStringLiteral( "pentium3" ) );
			New_VM->Set_Machine_Type( QStringLiteral( "pc" ) );
			// WHPX disables SMM → QEMU hides VGA text RAM → black XP setup screen.
			New_VM->Use_Force_TCG( true );
			New_VM->Set_Machine_Accelerator( VM::TCG );
			New_VM->Set_Display_Window_Mode( QStringLiteral( "embedded" ) );
			// QEMU 11+ vapic can BSOD XP; harmless on older QEMU.
			if( ! New_VM->Get_Additional_Args().contains(
				QLatin1String( "apic.vapic=off" ), Qt::CaseInsensitive ) )
			{
				const QString cur = New_VM->Get_Additional_Args().trimmed();
				New_VM->Set_Additional_Args( cur.isEmpty()
					? QStringLiteral( "-global apic.vapic=off" )
					: ( cur + QLatin1String( " -global apic.vapic=off" ) ) );
			}
		}

		const bool os2_family =
			os == "OS/2" || os == "eComStation" || os == "ArcaOS";
		if( os2_family )
		{
			// OS/2 LVM has no VirtIO; VirtIO + ACPI → "Error 9 returned from LVM.DLL".
			New_VM->Set_Mouse_Type( QStringLiteral( "ps2" ) );
			New_VM->Set_Video_Card( QStringLiteral( "cirrus" ) );
			New_VM->Use_ACPI( false );
			New_VM->Use_Check_FDD_Boot_Sector( false );
			New_VM->Set_SMP_CPU_Count( 1 );
			New_VM->Set_CPU_Type( QStringLiteral( "pentium3" ) );
			New_VM->Set_Machine_Type( QStringLiteral( "pc" ) );
			New_VM->Use_Force_TCG( true );
			New_VM->Set_Machine_Accelerator( VM::TCG );
			// Match proven OS/2 .aqemu (embedded session works with cirrus).
			New_VM->Set_Display_Window_Mode( QStringLiteral( "embedded" ) );
			Guest_NIC_Model = QStringLiteral( "rtl8139" );
			{
				VM::Sound_Cards audio;
				audio.Audio_es1370 = true;
				New_VM->Set_Audio_Cards( audio );
			}
			if( New_VM->Get_Memory_Size() < 512 )
				New_VM->Set_Memory_Size( 512 );
			VM_HDD hda = New_VM->Get_HDA();
			if( hda.Get_Enabled() )
			{
				VM_Native_Storage_Device native = hda.Get_Native_Device();
				native.Use_Interface( true );
				native.Set_Interface( VM::DI_IDE );
				if( ! native.Use_File_Path() || native.Get_File_Path().trimmed().isEmpty() )
				{
					native.Use_File_Path( true );
					native.Set_File_Path( hda.Get_File_Name() );
				}
				hda.Set_Native_Device( native );
				New_VM->Set_HDA( hda );
			}
		}

		// reactos.org/wiki/QEMU — known-working QEMU profile for current releases.
		if( os.startsWith( QLatin1String( "ReactOS" ) ) )
		{
			New_VM->Set_Machine_Type( QStringLiteral( "pc" ) );
			New_VM->Set_SMP_CPU_Count( 1 );
			New_VM->Set_CPU_Type( os.contains( QLatin1String( "64-bit" ) )
				? QStringLiteral( "qemu64" )
				: QStringLiteral( "pentium3" ) );
			New_VM->Use_Force_TCG( true );
			New_VM->Set_Machine_Accelerator( VM::TCG );
			New_VM->Use_Local_Time( true );
			New_VM->Use_Check_FDD_Boot_Sector( false );
			// Bochs VESA (std) works OOB; Cirrus needs a special NT driver.
			New_VM->Set_Video_Card( QStringLiteral( "std" ) );
			New_VM->Set_Display_Window_Mode( QStringLiteral( "native" ) );
			// Forum/wiki: -usbdevice tablet
			New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
			New_VM->Set_Mouse_USB_Controller( QStringLiteral( "uhci" ) );
			New_VM->Set_Mouse_USB_Version( 1 );
			New_VM->Use_USB_Hub( true );
			// AC97 works OOB on 0.4.15+; HDA / multi-audio can break boot.
			VM::Sound_Cards audio;
			audio.Audio_AC97 = true;
			New_VM->Set_Audio_Cards( audio );
			Guest_NIC_Model = QStringLiteral( "e1000" );
			if( New_VM->Get_Memory_Size() < 1024 )
				New_VM->Set_Memory_Size( 1024 );
			VM_HDD hda = New_VM->Get_HDA();
			if( hda.Get_Enabled() )
			{
				// Classic -hda (no native -drive). Format must persist for setup.
				hda.Set_Native_Device( VM_Native_Storage_Device() );
				New_VM->Set_HDA( hda );
			}
		}
		if( force_tcg_legacy )
		{
			New_VM->Use_Force_TCG( true );
			New_VM->Set_Machine_Accelerator( VM::TCG );
			New_VM->Set_CPU_Type( QStringLiteral( "pentium2" ) );
			New_VM->Set_SMP_CPU_Count( 1 );
			New_VM->Use_ACPI( false );
			if( New_VM->Get_Video_Card().isEmpty() )
				New_VM->Set_Video_Card( QStringLiteral( "cirrus" ) );
		}
		else if( classic_mac )
		{
			// mac99 uses onboard video/USB; do not force x86 cirrus / virtio-gpu.
			New_VM->Set_Video_Card( QString() );
			New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
			New_VM->Set_Mouse_USB_Controller( QStringLiteral( "auto" ) );
			New_VM->Set_SMP_CPU_Count( 1 );
			New_VM->Use_ACPI( false );
			New_VM->Use_Force_TCG( false );
			// Prefer sungem / macio-nic when the probed PPC device list has them
			if( Current_Devices )
			{
				QString nic;
				for( int i = 0; i < Current_Devices->Network_Card_List.count(); ++i )
				{
					const QString n = Current_Devices->Network_Card_List[i].QEMU_Name;
					if( n.contains( QLatin1String( "sungem" ), Qt::CaseInsensitive ) )
					{
						nic = n;
						break;
					}
					if( nic.isEmpty() && n.contains( QLatin1String( "macio" ), Qt::CaseInsensitive ) )
						nic = n;
				}
				Guest_NIC_Model = nic; // empty = leave unset rather than e1000
			}
			// Screamer is not a modern -device checkbox; hint via additional args if empty.
			if( New_VM->Get_Additional_Args().trimmed().isEmpty() )
				New_VM->Set_Additional_Args( QStringLiteral( "-device screamer" ) );
		}
		else if( modern_need_tablet && ! legacy_win )
		{
			New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
			New_VM->Set_Mouse_USB_Controller( QStringLiteral( "auto" ) );
			New_VM->Set_Mouse_USB_Version( 0 );
		}

		const bool chromeos_flex = ( os == QLatin1String( "Chrome OS Flex" ) );
		const bool steamos = ( os == QLatin1String( "SteamOS" ) );
		const bool nixos = ( os == QLatin1String( "NixOS" ) );
		const bool pop_os = ( os == QLatin1String( "Pop!_OS" ) );
		const bool qnx = ( os == QLatin1String( "QNX" ) );
		const bool hpux = ( os == QLatin1String( "HP-UX" ) );
		const bool plan9_family =
			os == QLatin1String( "Plan 9" ) || os == QLatin1String( "9front" );
		const bool sco_unix =
			os == QLatin1String( "OpenServer" ) || os == QLatin1String( "UnixWare" );

		if( chromeos_flex )
		{
			// Flex expects a capable GPU; VirtIO VGA works broadly.
			// virtio-vga-gl needs QEMU GTK/SDL OpenGL (best on Linux/KVM).
#ifdef Q_OS_WIN32
			New_VM->Set_Video_Card( QStringLiteral( "virtio" ) );
#else
			New_VM->Set_Video_Card( QStringLiteral( "virtio-vga-gl" ) );
#endif
			New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
			New_VM->Use_USB_Hub( true );
			if( New_VM->Get_Memory_Size() < 4096 )
				New_VM->Set_Memory_Size( 8192 );
		}

		if( pop_os )
		{
#ifdef Q_OS_WIN32
			New_VM->Set_Video_Card( QStringLiteral( "virtio" ) );
#else
			New_VM->Set_Video_Card( QStringLiteral( "virtio-vga-gl" ) );
#endif
			New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
		}

		if( plan9_family )
		{
			New_VM->Set_Video_Card( QStringLiteral( "std" ) );
			New_VM->Set_Mouse_Type( QStringLiteral( "ps2" ) );
		}

		if( sco_unix )
		{
			New_VM->Set_Video_Card( QStringLiteral( "cirrus" ) );
			New_VM->Set_Mouse_Type( QStringLiteral( "ps2" ) );
		}

		const bool aix_ppc = ( os == QLatin1String( "AIX" ) );
		if( aix_ppc )
		{
			// AIX is POWER-only: qemu-system-ppc64 -M pseries, TCG on x86 hosts
			// (WHPX cannot accelerate ppc). Common recipes: -cpu POWER8, virtio-scsi,
			// serial console / SLOF prom-env. Extremely slow under TCG.
			New_VM->Set_Machine_Type( QStringLiteral( "pseries" ) );
			New_VM->Set_CPU_Type( QStringLiteral( "POWER8" ) );
			New_VM->Set_SMP_CPU_Count( 1 );
			New_VM->Use_Force_TCG( false );
			New_VM->Set_Machine_Accelerator( VM::TCG );
			New_VM->Set_Video_Card( QStringLiteral( "std" ) );
			New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
			New_VM->Set_Display_Window_Mode( QStringLiteral( "native" ) );
			New_VM->Use_Local_Time( true );
			New_VM->Use_USB_Hub( true );
			{
				VM::Sound_Cards audio; // keep all off — AIX guides omit audio
				New_VM->Set_Audio_Cards( audio );
			}
			Guest_NIC_Model = QStringLiteral( "virtio-net-pci" );
			if( New_VM->Get_Memory_Size() < 2048 )
				New_VM->Set_Memory_Size( 2048 );
			// Do NOT force input/output to /vdevice/vty — that is for -nographic
			// -serial mon:stdio. With a VGA window, VTY steals the console and the
			// graphical keyboard appears dead.
			const QString prom = QStringLiteral( "-prom-env \"boot-command=boot disk:\"" );
			const QString cur = New_VM->Get_Additional_Args().trimmed();
			if( ! cur.contains( QLatin1String( "boot-command=" ), Qt::CaseInsensitive ) )
				New_VM->Set_Additional_Args( cur.isEmpty() ? prom : ( cur + QLatin1Char( ' ' ) + prom ) );
			New_VM->Set_Machine_Extra_Props( QStringLiteral(
				"cap-cfpc=broken,cap-ibs=broken,cap-ccf-assist=off" ) );
			VM_HDD hda = New_VM->Get_HDA();
			if( hda.Get_Enabled() )
			{
				VM_Native_Storage_Device native = hda.Get_Native_Device();
				native.Use_Interface( true );
				native.Set_Interface( VM::DI_Virtio_SCSI );
				native.Use_Cache( true );
				native.Set_Cache( QStringLiteral( "writeback" ) );
				if( ! native.Use_File_Path() || native.Get_File_Path().trimmed().isEmpty() )
				{
					native.Use_File_Path( true );
					native.Set_File_Path( hda.Get_File_Name() );
				}
				hda.Set_Native_Device( native );
				New_VM->Set_HDA( hda );
			}
		}

		const bool solaris_x86 =
			os == QLatin1String( "Solaris x86" ) ||
			os == QLatin1String( "Solaris x86 (32-bit)" ) ||
			os == QLatin1String( "OpenSolaris" );
		if( solaris_x86 )
		{
			// Match known-working QEMU recipe (r/qemu_kvm / itayemi):
			// -machine pc,usb=off -device VGA -e1000 -global PIIX4_PM.disable_s3/s4
			// Install can sit at 99% for 1–2 hours while the disk image keeps growing.
			New_VM->Set_Video_Card( QStringLiteral( "std" ) );
			New_VM->Set_Mouse_Type( QStringLiteral( "ps2" ) );
			New_VM->Set_SMP_CPU_Count( 2 );
			New_VM->Set_CPU_Type( os.contains( QLatin1String( "32-bit" ) )
				? QStringLiteral( "qemu32" )
				: QStringLiteral( "qemu64" ) );
			New_VM->Set_Machine_Type( QStringLiteral( "pc" ) );
			New_VM->Set_Machine_Extra_Props( QStringLiteral( "usb=off" ) );
			New_VM->Use_Force_TCG( false );
			New_VM->Set_Machine_Accelerator( VM::KVM );
			New_VM->Set_Display_Window_Mode( QStringLiteral( "native" ) );
			New_VM->Use_Local_Time( true );
			New_VM->Use_ACPI( true );
			New_VM->Use_USB_Hub( false );
			Guest_NIC_Model = QStringLiteral( "e1000" );
			{
				VM::Sound_Cards audio;
				audio.Audio_sb16 = true; // install recipe; switch to AC97/HDA after pkg update if desired
				New_VM->Set_Audio_Cards( audio );
			}
			const QString piix = QStringLiteral(
				"-global PIIX4_PM.disable_s3=1 -global PIIX4_PM.disable_s4=1" );
			const QString cur = New_VM->Get_Additional_Args().trimmed();
			if( ! cur.contains( QLatin1String( "PIIX4_PM.disable_s3" ), Qt::CaseInsensitive ) )
				New_VM->Set_Additional_Args( cur.isEmpty() ? piix : ( cur + QLatin1Char( ' ' ) + piix ) );
			if( os == QLatin1String( "Solaris x86" ) && New_VM->Get_Memory_Size() < 8192 )
				New_VM->Set_Memory_Size( 8192 );
			else if( New_VM->Get_Memory_Size() < 4096 )
				New_VM->Set_Memory_Size( 4096 );
			VM_HDD hda = New_VM->Get_HDA();
			if( hda.Get_Enabled() )
			{
				VM_Native_Storage_Device native = hda.Get_Native_Device();
				native.Use_Interface( true );
				native.Set_Interface( VM::DI_IDE );
				native.Use_Cache( true );
				native.Set_Cache( QStringLiteral( "writeback" ) );
				if( ! native.Use_File_Path() || native.Get_File_Path().trimmed().isEmpty() )
				{
					native.Use_File_Path( true );
					native.Set_File_Path( hda.Get_File_Name() );
				}
				hda.Set_Native_Device( native );
				New_VM->Set_HDA( hda );
			}
		}

		if( qnx )
		{
			// QNX timers run wild without a VM-locked RTC.
			const QString rtc = QStringLiteral( "-rtc base=localtime,clock=vm" );
			const QString cur = New_VM->Get_Additional_Args().trimmed();
			if( ! cur.contains( QLatin1String( "clock=vm" ), Qt::CaseInsensitive ) )
				New_VM->Set_Additional_Args( cur.isEmpty() ? rtc : ( cur + QLatin1Char( ' ' ) + rtc ) );
		}

		if( hpux )
		{
			New_VM->Set_Mouse_Type( QStringLiteral( "ps2" ) );
			const QString scsi = QStringLiteral( "-device lsi53c895a" );
			const QString cur = New_VM->Get_Additional_Args().trimmed();
			if( ! cur.contains( QLatin1String( "lsi53c895a" ), Qt::CaseInsensitive ) )
				New_VM->Set_Additional_Args( cur.isEmpty() ? scsi : ( cur + QLatin1Char( ' ' ) + scsi ) );
		}

		if( steamos || nixos )
		{
			if( steamos )
			{
				New_VM->Set_Machine_Type( QStringLiteral( "q35" ) );
				New_VM->Set_Video_Card( QStringLiteral( "virtio" ) );
				New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
				New_VM->Use_USB_Hub( true );
				New_VM->Use_Pass_Through_Gamepads( true );
				if( New_VM->Get_Memory_Size() < 4096 )
					New_VM->Set_Memory_Size( 8192 );
			}
			New_VM->Use_UEFI( true );

			// Wire OVMF paths now; VARS file is prepared at Create_VM time.
			Emulator emul = Get_Default_Emulator();
			QMap<QString, QString> bins = emul.Get_Binary_Files();
			QString qemu_bin;
			if( bins.contains( QStringLiteral( "qemu-system-x86_64" ) ) )
				qemu_bin = bins[ QStringLiteral( "qemu-system-x86_64" ) ];
			QString code = Find_UEFI_Firmware_CODE( qemu_bin, QStringLiteral( "x86_64" ) );
			QString vm_dir = Settings.value( QStringLiteral( "VM_Directory" ), "~" ).toString();
			QString vm_base = Get_FS_Compatible_VM_Name( ui.Edit_VM_Name->text() );
			QString vars_dest = vm_dir + vm_base + QStringLiteral( "_VARS.fd" );
			if( ! code.isEmpty() )
				New_VM->Set_UEFI_CODE_File( code );
			New_VM->Set_UEFI_VARS_File( vars_dest );
		}

		// Always set an explicit HDA bus. If left unset, Main_Window's post-wizard
		// Apply used to default the combo to VirtIO and rewrite the .aqemu file —
		// XP/OS/2/ReactOS setup then report "no hard disk".
		const bool want_nvme_disk = steamos;
		const bool want_scsi_disk = hpux;
		const bool want_virtio_scsi_disk = aix_ppc;
		const bool want_virtio_disk =
			! want_nvme_disk && ! want_scsi_disk && ! want_virtio_scsi_disk && (
			Selected_Target.contains( QLatin1String( "aarch64" ) ) ||
			Selected_Target == QLatin1String( "arm" ) ||
			Selected_Target == QLatin1String( "s390x" ) ||
			os.contains( QLatin1String( "Linux" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "BSD" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "Haiku" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "SerenityOS" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "ChromeOS" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "Chrome OS Flex" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "Android" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "Fuchsia" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "Redox" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "NixOS" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "Pop!_OS" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "CentOS" ), Qt::CaseInsensitive ) ||
			os.contains( QLatin1String( "elementary" ), Qt::CaseInsensitive ) ||
			os.startsWith( QLatin1String( "Windows 8" ) ) ||
			os.startsWith( QLatin1String( "Windows 10" ) ) ||
			os == QLatin1String( "Windows 11" ) ||
			os.startsWith( QLatin1String( "Windows Server 201" ) ) ||
			os.startsWith( QLatin1String( "Windows Server 202" ) ) );
		VM_HDD hda_bus = New_VM->Get_HDA();
		if( hda_bus.Get_Enabled() )
		{
			VM_Native_Storage_Device native = hda_bus.Get_Native_Device();
			// Preserve IDE if a legacy/OS2 block already forced it.
			// AIX already set VirtIO-SCSI above — keep that too.
			const bool already_ide =
				native.Use_Interface() && native.Get_Interface() == VM::DI_IDE;
			const bool already_vscsi =
				native.Use_Interface() && native.Get_Interface() == VM::DI_Virtio_SCSI;
			if( ! already_ide && ! already_vscsi )
			{
				native.Use_Interface( true );
				if( want_nvme_disk )
					native.Set_Interface( VM::DI_NVMe );
				else if( want_scsi_disk )
					native.Set_Interface( VM::DI_SCSI );
				else if( want_virtio_scsi_disk )
					native.Set_Interface( VM::DI_Virtio_SCSI );
				else
					native.Set_Interface( want_virtio_disk ? VM::DI_Virtio : VM::DI_IDE );
			}
			if( ! native.Use_File_Path() || native.Get_File_Path().trimmed().isEmpty() )
			{
				native.Use_File_Path( true );
				native.Set_File_Path( hda_bus.Get_File_Name() );
			}
			hda_bus.Set_Native_Device( native );
			New_VM->Set_HDA( hda_bus );
		}
	}

	if( ! Guest_NIC_Model.isEmpty() && New_VM->Get_Network_Cards_Count() > 0 )
	{
		QList<VM_Net_Card> cards = New_VM->Get_Network_Cards();
		cards[0].Set_Card_Model( Guest_NIC_Model );
		New_VM->Set_Network_Cards( cards );
	}
	else if( ! Guest_NIC_Model.isEmpty() && ui.RB_User_Mode_Network->isChecked() )
	{
		VM_Net_Card card;
		card.Set_Net_Mode( VM::Net_Mode_Usermode );
		card.Set_Card_Model( Guest_NIC_Model );
		New_VM->Set_Use_Network( true );
		if( New_VM->Get_Network_Cards_Count() == 0 )
			New_VM->Add_Network_Card( card );
		else
		{
			QList<VM_Net_Card> cards = New_VM->Get_Network_Cards();
			cards[0].Set_Card_Model( Guest_NIC_Model );
			New_VM->Set_Network_Cards( cards );
		}
	}
}

void VM_Wizard_Window::Prefer_Accelerator_For_Target( const QString &target )
{
	const QString host = AQ_Get_Host_CPU_Architecture();
	const QString guest = AQ_Normalize_CPU_Architecture( target );
	const bool prefer_native = AQ_Should_Prefer_Native_Accelerator( target );

	if( prefer_native )
		ui.RB_Emulator_KVM->setChecked( true );
	else
		ui.RB_Emulator_QEMU->setChecked( true );

#ifdef Q_OS_WIN32
	ui.RB_Emulator_KVM->setText( tr( "&KVM / WHPX (native acceleration)" ) );
#else
	ui.RB_Emulator_KVM->setText( tr( "&KVM (native acceleration)" ) );
#endif
	ui.RB_Emulator_QEMU->setText( tr( "&TCG (software emulation)" ) );

	QString tip;
	if( prefer_native )
	{
		tip = tr( "Host CPU is %1; guest target is %2 — using native acceleration "
		          "(KVM on Linux/WSL, WHPX on Windows)." )
			.arg( host ).arg( guest.isEmpty() ? target : guest );
	}
	else if( AQ_Guest_Matches_Host_Architecture( target ) && guest == QLatin1String( "i386" ) )
	{
		tip = tr( "Host is %1 but 32-bit x86 guests on Windows are more reliable with TCG "
		          "(WHPX often hangs). You can still try KVM/WHPX if you prefer." )
			.arg( host );
	}
	else
	{
		tip = tr( "Host CPU is %1; guest target is %2 — architectures do not match, "
		          "so TCG (software translation) is required." )
			.arg( host ).arg( guest.isEmpty() ? target : guest );
	}
	ui.RB_Emulator_KVM->setToolTip( tip );
	ui.RB_Emulator_QEMU->setToolTip( tip );
}

void VM_Wizard_Window::Goto_Hardware_Flow()
{
	Three_Path_Active = true;
	// Re-apply OS/platform target so Generate-VM defaults cannot force 64-bit over i386
	if( ! Selected_Target.isEmpty() )
		Apply_Selected_Computer_Type( Selected_Target );
	else
	{
		ui.RB_Generate_VM->setChecked( true );
		on_RB_Generate_VM_toggled( true );
	}
	ui.RB_Typical->setChecked( true );
	Use_Accelerator_Page = true;
	Prefer_Accelerator_For_Target( Selected_Target );
	Update_Guest_Compat_Tip();
	Update_Architecture_Page_Chrome();
	// Always land on Architecture page so user can override smart defaults
	ui.Wizard_Pages->setCurrentWidget( ui.Template_Page );
	ui.Label_Page->setText( tr("Architecture") );
	ui.Label_Template->setText( tr(
		"Confirm QEMU system and machine from the guest OS. Computer Type is fully editable:" ) );
	ui.Button_Back->setEnabled( true );
	ui.Button_Next->setEnabled( true );
}

void VM_Wizard_Window::Build_Windows11_ARM_Page()
{
	Win11_ARM_Page = new QWidget();
	QVBoxLayout *mainLay = new QVBoxLayout( Win11_ARM_Page );
	
	QLabel *intro = new QLabel( tr(
		"<b>Windows 11 ARM setup</b><br>"
		"Uses VirtIO for disk, network, GPU, keyboard, RNG, balloon, and sound by default. "
		"Provide a slipstreamed Windows 11 ARM ISO (VirtIO drivers already included)." ) );
	intro->setWordWrap( true );
	mainLay->addWidget( intro );
	
	QGroupBox *diskBox = new QGroupBox( tr("Virtual disk") );
	QVBoxLayout *diskLay = new QVBoxLayout( diskBox );
	RB_Win11_New_Disk = new QRadioButton( tr("Create a new disk image") );
	RB_Win11_Existing_Disk = new QRadioButton( tr("Use an existing disk image") );
	RB_Win11_New_Disk->setChecked( true );
	diskLay->addWidget( RB_Win11_New_Disk );
	diskLay->addWidget( RB_Win11_Existing_Disk );
	
	QHBoxLayout *existLay = new QHBoxLayout();
	Edit_Win11_Existing_Disk = new QLineEdit();
	Edit_Win11_Existing_Disk->setEnabled( false );
	TB_Win11_Existing_Disk_Browse = new QToolButton();
	TB_Win11_Existing_Disk_Browse->setText( "..." );
	TB_Win11_Existing_Disk_Browse->setEnabled( false );
	existLay->addWidget( Edit_Win11_Existing_Disk );
	existLay->addWidget( TB_Win11_Existing_Disk_Browse );
	diskLay->addLayout( existLay );
	mainLay->addWidget( diskBox );
	
	CH_Win11_Already_Installed = new QCheckBox( tr("Windows is already installed on this disk") );
	mainLay->addWidget( CH_Win11_Already_Installed );
	
	QGroupBox *isoBox = new QGroupBox( tr("Install media") );
	QVBoxLayout *isoLay = new QVBoxLayout( isoBox );
	QHBoxLayout *isoPathLay = new QHBoxLayout();
	Edit_Win11_ISO = new QLineEdit();
	TB_Win11_ISO_Browse = new QToolButton();
	TB_Win11_ISO_Browse->setText( "..." );
	isoPathLay->addWidget( new QLabel( tr("Windows 11 ARM ISO:") ) );
	isoPathLay->addWidget( Edit_Win11_ISO );
	isoPathLay->addWidget( TB_Win11_ISO_Browse );
	isoLay->addLayout( isoPathLay );
	
	CH_Win11_VirtIO_ISO = new QCheckBox( tr("Also attach virtio-win.iso (only if drivers were not slipstreamed)") );
	isoLay->addWidget( CH_Win11_VirtIO_ISO );
	QHBoxLayout *virtioLay = new QHBoxLayout();
	Edit_Win11_VirtIO_ISO = new QLineEdit();
	Edit_Win11_VirtIO_ISO->setEnabled( false );
	TB_Win11_VirtIO_ISO_Browse = new QToolButton();
	TB_Win11_VirtIO_ISO_Browse->setText( "..." );
	TB_Win11_VirtIO_ISO_Browse->setEnabled( false );
	virtioLay->addWidget( Edit_Win11_VirtIO_ISO );
	virtioLay->addWidget( TB_Win11_VirtIO_ISO_Browse );
	isoLay->addLayout( virtioLay );
	mainLay->addWidget( isoBox );
	
	Label_Win11_UEFI_Status = new QLabel();
	Label_Win11_UEFI_Status->setWordWrap( true );
	mainLay->addWidget( Label_Win11_UEFI_Status );
	
	Label_Win11_Finish_Help = new QLabel( tr(
		"<b>After Finish:</b> Start the VM, install Windows from the ISO, then set boot order to the hard disk "
		"(or uncheck the CD-ROM) and start again for daily use." ) );
	Label_Win11_Finish_Help->setWordWrap( true );
	mainLay->addWidget( Label_Win11_Finish_Help );
	mainLay->addStretch();
	
	ui.Wizard_Pages->addWidget( Win11_ARM_Page );
	
	connect( RB_Win11_New_Disk, SIGNAL(toggled(bool)), this, SLOT(Win11_New_Disk_Toggled(bool)) );
	connect( CH_Win11_Already_Installed, SIGNAL(toggled(bool)), this, SLOT(Win11_Already_Installed_Toggled(bool)) );
	connect( TB_Win11_ISO_Browse, SIGNAL(clicked()), this, SLOT(Win11_ISO_Browse_Clicked()) );
	connect( TB_Win11_Existing_Disk_Browse, SIGNAL(clicked()), this, SLOT(Win11_Existing_Disk_Browse_Clicked()) );
	connect( TB_Win11_VirtIO_ISO_Browse, SIGNAL(clicked()), this, SLOT(Win11_VirtIO_ISO_Browse_Clicked()) );
	connect( CH_Win11_VirtIO_ISO, SIGNAL(toggled(bool)), this, SLOT(Win11_VirtIO_ISO_Toggled(bool)) );
}

void VM_Wizard_Window::Build_Intel_MacOS_Page()
{
	Intel_MacOS_Page = new QWidget();
	QVBoxLayout *pageLay = new QVBoxLayout( Intel_MacOS_Page );
	pageLay->setContentsMargins( 0, 0, 0, 0 );

	QScrollArea *scroll = new QScrollArea( Intel_MacOS_Page );
	scroll->setWidgetResizable( true );
	scroll->setFrameShape( QFrame::NoFrame );
	scroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	pageLay->addWidget( scroll );

	QWidget *content = new QWidget();
	scroll->setWidget( content );
	QVBoxLayout *mainLay = new QVBoxLayout( content );
	mainLay->setContentsMargins( 8, 8, 8, 8 );
	mainLay->setSpacing( 8 );

	auto lock_height = []( QWidget *w ) {
		w->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		w->setMinimumHeight( w->sizeHint().height() );
	};

	QLabel *intro = new QLabel( tr(
		"<b>Intel macOS (experimental)</b> — AQEMU does not ship OpenCore, OVMF, OSK, or Apple OS images. "
		"Uses <code>qemu-system-x86_64</code> + <code>q35</code> + OVMF. "
		"OpenCore <b>.iso</b> files attach as CD/DVD.<br/>"
		"Default display is VMware SVGA (software). "
		"<b>AMD Metal</b> passthrough appears only when an AMD GPU is detected, and only works on "
		"bare-metal Linux with VFIO — not Windows/WSL. Install macOS first, then enable passthrough." ) );
	intro->setWordWrap( true );
	intro->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
	mainLay->addWidget( intro );

	CH_Intel_Mac_Supply_Files = new QCheckBox( tr(
		"I will supply all Apple / OpenCore / OVMF files myself" ) );
	CH_Intel_Mac_Supply_Files->setChecked( true );
	lock_height( CH_Intel_Mac_Supply_Files );
	mainLay->addWidget( CH_Intel_Mac_Supply_Files );

	QGroupBox *ocBox = new QGroupBox( tr("OpenCore (ISO or disk image)") );
	QVBoxLayout *ocOuter = new QVBoxLayout( ocBox );
	ocOuter->setSpacing( 6 );
	QLabel *ocHint = new QLabel( tr(
		"e.g. LongQT-OpenCore-v0.x.iso (CD/DVD) or OpenCore .qcow2/.img" ) );
	ocHint->setWordWrap( true );
	ocOuter->addWidget( ocHint );
	QHBoxLayout *ocLay = new QHBoxLayout();
	Edit_Intel_Mac_OpenCore = new QLineEdit();
	lock_height( Edit_Intel_Mac_OpenCore );
	TB_Intel_Mac_OpenCore_Browse = new QToolButton();
	TB_Intel_Mac_OpenCore_Browse->setText( "..." );
	TB_Intel_Mac_OpenCore_Browse->setFixedWidth( 32 );
	lock_height( TB_Intel_Mac_OpenCore_Browse );
	ocLay->addWidget( Edit_Intel_Mac_OpenCore );
	ocLay->addWidget( TB_Intel_Mac_OpenCore_Browse );
	ocOuter->addLayout( ocLay );
	ocBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
	mainLay->addWidget( ocBox );

	QGroupBox *diskBox = new QGroupBox( tr("System disk") );
	QVBoxLayout *diskLay = new QVBoxLayout( diskBox );
	diskLay->setSpacing( 6 );
	RB_Intel_Mac_New_Disk = new QRadioButton( tr("Create a new disk image") );
	RB_Intel_Mac_Existing_Disk = new QRadioButton( tr("Use an existing disk image") );
	RB_Intel_Mac_New_Disk->setChecked( true );
	lock_height( RB_Intel_Mac_New_Disk );
	lock_height( RB_Intel_Mac_Existing_Disk );
	diskLay->addWidget( RB_Intel_Mac_New_Disk );
	diskLay->addWidget( RB_Intel_Mac_Existing_Disk );
	QHBoxLayout *existLay = new QHBoxLayout();
	Edit_Intel_Mac_Existing_Disk = new QLineEdit();
	Edit_Intel_Mac_Existing_Disk->setEnabled( false );
	lock_height( Edit_Intel_Mac_Existing_Disk );
	TB_Intel_Mac_Disk_Browse = new QToolButton();
	TB_Intel_Mac_Disk_Browse->setText( "..." );
	TB_Intel_Mac_Disk_Browse->setFixedWidth( 32 );
	TB_Intel_Mac_Disk_Browse->setEnabled( false );
	lock_height( TB_Intel_Mac_Disk_Browse );
	existLay->addWidget( Edit_Intel_Mac_Existing_Disk );
	existLay->addWidget( TB_Intel_Mac_Disk_Browse );
	diskLay->addLayout( existLay );
	diskBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
	mainLay->addWidget( diskBox );

	QGroupBox *recBox = new QGroupBox( tr("Installer disk image (MIST APM/HFS .iso)") );
	QHBoxLayout *recLay = new QHBoxLayout( recBox );
	Edit_Intel_Mac_Recovery = new QLineEdit();
	lock_height( Edit_Intel_Mac_Recovery );
	TB_Intel_Mac_Recovery_Browse = new QToolButton();
	TB_Intel_Mac_Recovery_Browse->setText( "..." );
	TB_Intel_Mac_Recovery_Browse->setFixedWidth( 32 );
	lock_height( TB_Intel_Mac_Recovery_Browse );
	recLay->addWidget( Edit_Intel_Mac_Recovery );
	recLay->addWidget( TB_Intel_Mac_Recovery_Browse );
	recBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
	mainLay->addWidget( recBox );

	QGroupBox *oskBox = new QGroupBox( tr("Apple SMC OSK (required to start)") );
	QVBoxLayout *oskLay = new QVBoxLayout( oskBox );
	Edit_Intel_Mac_OSK = new QLineEdit();
	Edit_Intel_Mac_OSK->setEchoMode( QLineEdit::Password );
	Edit_Intel_Mac_OSK->setPlaceholderText( tr( "Paste your own OSK — AQEMU never pre-fills this" ) );
	lock_height( Edit_Intel_Mac_OSK );
	oskLay->addWidget( Edit_Intel_Mac_OSK );
	oskBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
	mainLay->addWidget( oskBox );

	Label_Intel_Mac_UEFI_Status = new QLabel();
	Label_Intel_Mac_UEFI_Status->setWordWrap( true );
	Label_Intel_Mac_UEFI_Status->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
	mainLay->addWidget( Label_Intel_Mac_UEFI_Status );

	CH_Intel_Mac_Prefer_WSL = new QCheckBox( tr(
		"Prefer WSL/KVM when /dev/kvm is available (Windows)" ) );
#ifdef Q_OS_WIN32
	CH_Intel_Mac_Prefer_WSL->setChecked(
		Settings.value( QStringLiteral( "WSL_Launch/Enabled" ), false ).toBool() );
	CH_Intel_Mac_Prefer_WSL->setEnabled( true );
	CH_Intel_Mac_Prefer_WSL->setToolTip( tr(
		"Uses Linux QEMU inside WSL with KVM. Probing /dev/kvm is deferred so the New VM wizard opens instantly." ) );
#else
	CH_Intel_Mac_Prefer_WSL->setChecked( false );
	CH_Intel_Mac_Prefer_WSL->setEnabled( false );
	CH_Intel_Mac_Prefer_WSL->setVisible( false );
#endif
	lock_height( CH_Intel_Mac_Prefer_WSL );
	mainLay->addWidget( CH_Intel_Mac_Prefer_WSL );

	QLabel *finishHelp = new QLabel( tr(
		"<b>After Finish:</b> Start should reach the OpenCore picker if your OpenCore image is valid. "
		"Install success depends on your OpenCore config — not AQEMU." ) );
	finishHelp->setWordWrap( true );
	finishHelp->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
	mainLay->addWidget( finishHelp );
	mainLay->addStretch( 1 );

	ui.Wizard_Pages->addWidget( Intel_MacOS_Page );

	connect( RB_Intel_Mac_New_Disk, SIGNAL(toggled(bool)), this, SLOT(Intel_Mac_New_Disk_Toggled(bool)) );
	connect( TB_Intel_Mac_OpenCore_Browse, SIGNAL(clicked()), this, SLOT(Intel_Mac_OpenCore_Browse_Clicked()) );
	connect( TB_Intel_Mac_Disk_Browse, SIGNAL(clicked()), this, SLOT(Intel_Mac_Disk_Browse_Clicked()) );
	connect( TB_Intel_Mac_Recovery_Browse, SIGNAL(clicked()), this, SLOT(Intel_Mac_Recovery_Browse_Clicked()) );
}

void VM_Wizard_Window::Show_Intel_MacOS_Page()
{
	ui.Wizard_Pages->setCurrentWidget( Intel_MacOS_Page );
	ui.Label_Page->setText( tr( "Intel macOS Setup" ) );
	Probe_WSL_For_Intel_Mac_Page();
}

void VM_Wizard_Window::Probe_WSL_For_Intel_Mac_Page()
{
#ifdef Q_OS_WIN32
	if( ! CH_Intel_Mac_Prefer_WSL )
		return;

	AQ_Run_With_Busy_Dialog( this, tr( "Checking WSL/KVM…" ), [ this ]() {
		const QString distro =
			Settings.value( QStringLiteral( "WSL_Launch/Distro" ), QString() ).toString();
		const bool kvm = WSL_Is_Available( false ) && WSL_Ensure_KVM_Access( distro );
		if( kvm )
			CH_Intel_Mac_Prefer_WSL->setChecked( true );
		else if( ! Settings.value( QStringLiteral( "WSL_Launch/Enabled" ), false ).toBool() )
			CH_Intel_Mac_Prefer_WSL->setChecked( false );

		CH_Intel_Mac_Prefer_WSL->setToolTip( kvm
			? tr( "WSL/KVM available. Prefer launching Linux QEMU inside WSL." )
			: tr( "WSL/KVM not detected. Native Windows QEMU (WHPX/TCG) will be used unless you enable WSL in Settings after fixing KVM." ) );
	} );
#else
	Q_UNUSED( 0 );
#endif
}

bool VM_Wizard_Window::Is_Intel_MacOS_Template() const
{
	if( Three_Path_Active )
	{
		return Selected_OS_Name.contains( "macOS", Qt::CaseInsensitive ) ||
		       Selected_OS_Name.contains( "Mac OS X Intel" ) ||
		       Selected_OS_Name.contains( "Darwin" );
	}
	if( ! ui.RB_VM_Template->isChecked() )
		return false;
	if( ui.CB_OS_Type->currentIndex() <= 0 )
		return false;
	const QString t = ui.CB_OS_Type->currentText();
	return t.contains( "MacOS X x86", Qt::CaseInsensitive ) ||
	       t.contains( "macOS", Qt::CaseInsensitive );
}

void VM_Wizard_Window::Intel_Mac_New_Disk_Toggled( bool on )
{
	Edit_Intel_Mac_Existing_Disk->setEnabled( ! on );
	TB_Intel_Mac_Disk_Browse->setEnabled( ! on );
}

void VM_Wizard_Window::Intel_Mac_OpenCore_Browse_Clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select OpenCore ISO or disk image"),
		Get_Last_Dir_Path( Edit_Intel_Mac_OpenCore->text() ),
		tr("OpenCore (*.iso *.qcow2 *.qcow *.img *.raw);;ISO CD/DVD (*.iso);;Disk Images (*.qcow2 *.qcow *.img *.raw);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Intel_Mac_OpenCore->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::Intel_Mac_Disk_Browse_Clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select system disk image"),
		Get_Last_Dir_Path( Edit_Intel_Mac_Existing_Disk->text() ),
		tr("Disk Images (*.qcow2 *.qcow *.img *.vhd *.vhdx *.raw);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Intel_Mac_Existing_Disk->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::Intel_Mac_Recovery_Browse_Clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select Recovery / BaseSystem image"),
		Get_Last_Dir_Path( Edit_Intel_Mac_Recovery->text() ),
		tr("Disk / ISO (*.qcow2 *.qcow *.img *.raw *.iso *.dmg);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Intel_Mac_Recovery->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::Win11_VirtIO_ISO_Toggled( bool on )
{
	Edit_Win11_VirtIO_ISO->setEnabled( on );
	TB_Win11_VirtIO_ISO_Browse->setEnabled( on );
}

bool VM_Wizard_Window::Is_Windows11_ARM_Template() const
{
	if( Three_Path_Active )
	{
		return Selected_OS_Name.contains( "Windows 11", Qt::CaseInsensitive ) &&
		       Selected_Target == "aarch64";
	}
	if( ! ui.RB_VM_Template->isChecked() )
		return false;
	if( ui.CB_OS_Type->currentIndex() <= 0 )
		return false;
	return ui.CB_OS_Type->currentText().contains( "Windows 11 ARM", Qt::CaseInsensitive );
}

void VM_Wizard_Window::Win11_New_Disk_Toggled( bool on )
{
	Edit_Win11_Existing_Disk->setEnabled( ! on );
	TB_Win11_Existing_Disk_Browse->setEnabled( ! on );
}

void VM_Wizard_Window::Win11_Already_Installed_Toggled( bool on )
{
	Edit_Win11_ISO->setEnabled( ! on );
	TB_Win11_ISO_Browse->setEnabled( ! on );
	CH_Win11_VirtIO_ISO->setEnabled( ! on );
	if( on )
	{
		CH_Win11_VirtIO_ISO->setChecked( false );
		Edit_Win11_VirtIO_ISO->setEnabled( false );
		TB_Win11_VirtIO_ISO_Browse->setEnabled( false );
	}
}

void VM_Wizard_Window::Win11_ISO_Browse_Clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select Windows 11 ARM ISO"),
		Get_Last_Dir_Path( Edit_Win11_ISO->text() ),
		tr("ISO Images (*.iso);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Win11_ISO->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::Win11_Existing_Disk_Browse_Clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select existing disk image"),
		Get_Last_Dir_Path( Edit_Win11_Existing_Disk->text() ),
		tr("Disk Images (*.qcow2 *.qcow *.img *.vhd *.vhdx *.raw);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Win11_Existing_Disk->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::Win11_VirtIO_ISO_Browse_Clicked()
{
	QString file = QFileDialog::getOpenFileName( this, tr("Select virtio-win ISO"),
		Get_Last_Dir_Path( Edit_Win11_VirtIO_ISO->text() ),
		tr("ISO Images (*.iso);;All Files (*)") );
	if( ! file.isEmpty() )
		Edit_Win11_VirtIO_ISO->setText( QDir::toNativeSeparators( file ) );
}

void VM_Wizard_Window::KVM_toggled(bool toggled)
{
    if ( toggled )
        ui.toolBox_accelInfo->setCurrentIndex(1);
    else
        ui.toolBox_accelInfo->setCurrentIndex(0);
}

void VM_Wizard_Window::Set_VM_List( QList<Virtual_Machine*> *list )
{
	VM_List = list;
}

void VM_Wizard_Window::on_Button_Back_clicked()
{
	ui.Button_Next->setEnabled( true );
	ui.Button_Next->setText( tr("&Next") );
	
	if( Creation_Method_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Button_Back->setEnabled( false );
	}
	else if( OS_Tree_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Current_Method == Method_Platform || Current_Method == Method_Architecture )
		{
			if( Current_Method == Method_Architecture )
			{
				ui.Wizard_Pages->setCurrentWidget( Arch_Machines_Page );
				ui.Label_Page->setText( tr("Select Machine") );
			}
			else
			{
				ui.Wizard_Pages->setCurrentWidget( Platform_Tree_Page );
				ui.Label_Page->setText( tr("System / Machine Platform") );
			}
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( Creation_Method_Page );
			ui.Label_Page->setText( tr("Creation Method") );
			ui.Button_Back->setEnabled( false );
			Three_Path_Active = false;
			Current_Method = Method_None;
		}
	}
	else if( Platform_Tree_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( Creation_Method_Page );
		ui.Label_Page->setText( tr("Creation Method") );
		ui.Button_Back->setEnabled( false );
		Three_Path_Active = false;
		Current_Method = Method_None;
	}
	else if( Arch_List_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( Creation_Method_Page );
		ui.Label_Page->setText( tr("Creation Method") );
		ui.Button_Back->setEnabled( false );
		Three_Path_Active = false;
		Current_Method = Method_None;
	}
	else if( Arch_Machines_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( Arch_List_Page );
		ui.Label_Page->setText( tr("CPU Architecture") );
	}
	else if( ui.Wizard_Mode_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( Creation_Method_Page );
		ui.Label_Page->setText( tr("Creation Method") );
		ui.Button_Back->setEnabled( false );
		Current_Method = Method_None;
		Three_Path_Active = false;
	}
	else if( ui.Template_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Three_Path_Active )
		{
			if( Current_Method == Method_Architecture )
			{
				ui.Wizard_Pages->setCurrentWidget( OS_Tree_Page );
				ui.Label_Page->setText( tr("Select Operating System") );
			}
			else if( Current_Method == Method_Platform )
			{
				ui.Wizard_Pages->setCurrentWidget( OS_Tree_Page );
				ui.Label_Page->setText( tr("Select Operating System") );
			}
			else
			{
				ui.Wizard_Pages->setCurrentWidget( OS_Tree_Page );
				ui.Label_Page->setText( tr("Guest Operating System") );
			}
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Wizard_Mode_Page );
			ui.Label_Page->setText( tr("Wizard Mode") );
		}
	}
	else if( ui.Accelerator_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Three_Path_Active )
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Template_Page );
			ui.Label_Page->setText( tr("Architecture") );
			Update_Guest_Compat_Tip();
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Template_Page );
			ui.Label_Page->setText( tr("VM Hardware Template") );
		}
	}
	else if( ui.General_Settings_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Accelerator_Page );
		ui.Label_Page->setText( tr("Accelerator") );
	}
	else if( ui.Memory_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.General_Settings_Page );
		ui.Label_Page->setText( tr("VM Name and CPU Type") );
		ui.Label_Caption_CPU_Type->setVisible( true );
		ui.Line_CPU_Type->setVisible( true );
		ui.Label_CPU_Type->setVisible( true );
		ui.CB_CPU_Type->setVisible( true );
	}
	else if( ui.Typical_HDD_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.General_Settings_Page );
		ui.Label_Page->setText( tr("Virtual Machine Name") );
		ui.Label_Caption_CPU_Type->setVisible( false );
		ui.Line_CPU_Type->setVisible( false );
		ui.Label_CPU_Type->setVisible( false );
		ui.CB_CPU_Type->setVisible( false );
	}
	else if( Win11_ARM_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( ui.RB_Typical->isChecked() )
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Typical_HDD_Page );
			ui.Label_Page->setText( tr("Hard Disk Size") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Custom_HDD_Page );
			ui.Label_Page->setText( tr("Virtual Hard Disk") );
		}
	}
	else if( Intel_MacOS_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( ui.RB_Typical->isChecked() )
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Typical_HDD_Page );
			ui.Label_Page->setText( tr("Hard Disk Size") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Custom_HDD_Page );
			ui.Label_Page->setText( tr("Virtual Hard Disk") );
		}
	}
	else if( ui.Custom_HDD_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Memory_Page );
		ui.Label_Page->setText( tr("Memory") );
	}
	else if( ui.Network_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Is_Intel_MacOS_Template() )
		{
			Show_Intel_MacOS_Page();
		}
		else if( Is_Windows11_ARM_Template() )
		{
			ui.Wizard_Pages->setCurrentWidget( Win11_ARM_Page );
			ui.Label_Page->setText( tr("Windows 11 ARM Install") );
		}
		else if( ui.RB_Typical->isChecked() ) // typical or custom mode
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Typical_HDD_Page );
			ui.Label_Page->setText( tr("Hard Disk Size") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Custom_HDD_Page );
			ui.Label_Page->setText( tr("Virtual Hard Disk") );
		}
	}
	else if( ui.Finish_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
		ui.Label_Page->setText( tr("Network") );
		ui.Button_Next->setText( tr("&Next") );
	}
	else
	{
		AQError( "void VM_Wizard_Window::on_Button_Back_clicked()",
				 "Default Section!" );
	}
}

void VM_Wizard_Window::on_Button_Next_clicked()
{
	if( Creation_Method_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( RB_Method_Guest_OS->isChecked() )
		{
			Current_Method = Method_Guest_OS;
			Three_Path_Active = true;
			ui.Wizard_Pages->setCurrentWidget( OS_Tree_Page );
			ui.Label_Page->setText( tr("Guest Operating System") );
		}
		else if( RB_Method_Platform->isChecked() )
		{
			Current_Method = Method_Platform;
			Three_Path_Active = true;
			ui.Wizard_Pages->setCurrentWidget( Platform_Tree_Page );
			ui.Label_Page->setText( tr("System / Machine Platform") );
		}
		else if( RB_Method_Architecture->isChecked() )
		{
			Current_Method = Method_Architecture;
			Three_Path_Active = true;
			ui.Wizard_Pages->setCurrentWidget( Arch_List_Page );
			ui.Label_Page->setText( tr("CPU Architecture") );
		}
		else
		{
			Current_Method = Method_Custom;
			Three_Path_Active = false;
			ui.Wizard_Pages->setCurrentWidget( ui.Wizard_Mode_Page );
			ui.Label_Page->setText( tr("Wizard Mode") );
		}
		ui.Button_Back->setEnabled( true );
		return;
	}
	else if( OS_Tree_Page == ui.Wizard_Pages->currentWidget() )
	{
		QString os = Selected_Tree_Leaf( Tree_OS );
		if( os.isEmpty() )
		{
			AQGraphic_Warning( tr("Select OS"), tr("Please select a guest operating system (leaf item).") );
			return;
		}
		if( ! Ensure_Emulator_Ready() )
			return;
		Apply_OS_Defaults( os );
		if( ! Apply_Selected_Computer_Type( Selected_Target ) )
			return;
		if( ! Selected_Machine_Id.isEmpty() )
			New_VM->Set_Machine_Type( Selected_Machine_Id );
		Goto_Hardware_Flow();
		return;
	}
	else if( Platform_Tree_Page == ui.Wizard_Pages->currentWidget() )
	{
		QString plat = Selected_Tree_Leaf( Tree_Platform );
		if( plat.isEmpty() )
		{
			AQGraphic_Warning( tr("Select Platform"), tr("Please select a machine platform (leaf item).") );
			return;
		}
		if( plat == "Hundreds More..." )
		{
			AQGraphic_Warning( tr("More platforms"),
				tr("Use \"CPU Architecture\" then expand \"All available machines…\", "
				   "or place qemu_machine_catalog.json next to aqemu.exe.") );
			return;
		}
		if( plat == "Import Existing QEMU Command Line" )
		{
			AQGraphic_Warning( tr("Not implemented"),
				tr("Importing an existing QEMU command line is not available yet. "
				   "Choose Custom / Advanced for manual setup.") );
			return;
		}
		if( ! Ensure_Emulator_Ready() )
			return;
		Apply_Platform_Binding( plat );
		if( ! Apply_Selected_Computer_Type( Selected_Target ) )
			return;
		ui.Wizard_Pages->setCurrentWidget( OS_Tree_Page );
		ui.Label_Page->setText( tr("Select Operating System") );
		return;
	}
	else if( Arch_List_Page == ui.Wizard_Pages->currentWidget() )
	{
		QListWidgetItem *item = List_Arch->currentItem();
		if( ! item )
		{
			AQGraphic_Warning( tr("Select Architecture"), tr("Please select a CPU architecture.") );
			return;
		}
		Populate_Arch_Machines( item->text() );
		ui.Wizard_Pages->setCurrentWidget( Arch_Machines_Page );
		ui.Label_Page->setText( tr("Select Machine") );
		return;
	}
	else if( Arch_Machines_Page == ui.Wizard_Pages->currentWidget() )
	{
		QTreeWidgetItem *item = Tree_Arch_Machines->currentItem();
		QString name = Selected_Tree_Leaf( Tree_Arch_Machines );
		if( name.isEmpty() || ! item )
		{
			AQGraphic_Warning( tr("Select Machine"), tr("Please select a machine (leaf item).") );
			return;
		}
		if( ! Ensure_Emulator_Ready() )
			return;
		Selected_Platform_Name = name;
		QString mid = item->data( 0, Qt::UserRole ).toString();
		QString tgt = item->data( 0, Qt::UserRole + 1 ).toString();
		if( ! tgt.isEmpty() )
			Selected_Target = tgt;
		if( ! mid.isEmpty() )
			Selected_Machine_Id = mid;
		else
			Apply_Platform_Binding( name );
		if( ! Apply_Selected_Computer_Type( Selected_Target ) )
			return;
		if( ! Selected_Machine_Id.isEmpty() )
			New_VM->Set_Machine_Type( Selected_Machine_Id );
		ui.Wizard_Pages->setCurrentWidget( OS_Tree_Page );
		ui.Label_Page->setText( tr("Select Operating System") );
		return;
	}
	else if( ui.Wizard_Mode_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Template_Page );
		ui.Label_Page->setText( tr("Template For VM") );
		on_RB_VM_Template_toggled( ui.RB_VM_Template->isChecked() );

        if( ! Ensure_Emulator_Ready() )
            return;

        // Prefer host CPU architecture when available in the emulator list
        int defaultIndex = 0;
        const QString host_arch = AQ_Get_Host_CPU_Architecture();
        for( int ix = 0; ix < ui.CB_Computer_Type->count(); ++ix )
        {
            QString text = ui.CB_Computer_Type->itemText( ix );
            QString lower = text.toLower();
            if( host_arch == QLatin1String( "aarch64" ) )
            {
                if( lower.contains( "aarch64" ) || lower.contains( "arm 64" ) )
                {
                    defaultIndex = ix;
                    break;
                }
            }
            else if( host_arch == QLatin1String( "x86_64" ) )
            {
                if( lower.contains( "x86_64" ) || text.contains( "64Bit" ) )
                {
                    defaultIndex = ix;
                    break;
                }
            }
            else if( lower.contains( host_arch ) )
            {
                defaultIndex = ix;
                break;
            }
        }
        if( ui.CB_Computer_Type->count() > 0 )
        {
            ui.CB_Computer_Type->setCurrentIndex( defaultIndex );
        }

        // Auto-select first template if none is selected
        if( ui.CB_OS_Type->currentIndex() == 0 && ui.CB_OS_Type->count() > 1 )
        {
            ui.CB_OS_Type->setCurrentIndex( 1 );
        }

        ui.Button_Next->setEnabled( true );
		ui.Button_Back->setEnabled( true );
	}
	else if( ui.Accelerator_Page == ui.Wizard_Pages->currentWidget() )
	{
        applyTemplate();
        ui.Wizard_Pages->setCurrentWidget( ui.General_Settings_Page );

	}
	else if( ui.Template_Page == ui.Wizard_Pages->currentWidget() )
	{
        Use_Accelerator_Page = true;

		// Sync Selected_Target from Architecture combo (user may have overridden OS default)
		if( ui.RB_Generate_VM->isChecked() )
		{
			for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin();
			     it != All_Systems.constEnd(); ++it )
			{
				if( it.value().System.Caption == ui.CB_Computer_Type->currentText() )
				{
					QString qn = it.value().System.QEMU_Name;
					Selected_Target = qn;
					Selected_Target.remove( "qemu-system-" );
					New_VM->Set_Computer_Type( qn );
					Current_Devices = &it.value();
					break;
				}
			}
		}

        // Prefer KVM/WHPX when guest matches host arch; otherwise TCG
        if( ! Selected_Target.isEmpty() )
			Prefer_Accelerator_For_Target( Selected_Target );
		else
		{
			// Fall back: derive target from Computer Type caption / binary
			QString tgt;
			for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin();
			     it != All_Systems.constEnd(); ++it )
			{
				if( it.value().System.Caption == ui.CB_Computer_Type->currentText() )
				{
					tgt = it.value().System.QEMU_Name;
					tgt.remove( "qemu-system-" );
					break;
				}
			}
			Prefer_Accelerator_For_Target( tgt.isEmpty() ? QStringLiteral( "x86_64" ) : tgt );
		}

		Update_Guest_Compat_Tip();
		ui.Wizard_Pages->setCurrentWidget( ui.Accelerator_Page );
		ui.Label_Page->setText( tr("Accelerator") );
	}
	else if( ui.General_Settings_Page == ui.Wizard_Pages->currentWidget() )
	{
		for( int vx = 0; vx < VM_List->count(); ++vx )
		{
			if( VM_List->at(vx)->Get_Machine_Name() == ui.Edit_VM_Name->text() )
			{
				AQGraphic_Warning( tr("Warning"), tr("This VM Name Does Already Exist!") );
				return;
			}
		}

		if( ui.RB_Typical->isChecked() )
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Typical_HDD_Page );
			ui.Label_Page->setText( tr("Hard Disk Size") );
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Memory_Page );
			ui.Label_Page->setText( tr("Memory") );
		}
	}
	else if( ui.Memory_Page == ui.Wizard_Pages->currentWidget() )
	{
		on_CH_Remove_RAM_Size_Limitation_stateChanged( Qt::Unchecked ); // update max available RAM size
		ui.Wizard_Pages->setCurrentWidget( ui.Custom_HDD_Page );
		ui.Label_Page->setText( tr("Virtual Hard Disk") );
	}
	else if( ui.Typical_HDD_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Is_Windows11_ARM_Template() )
		{
			// Refresh UEFI status label
			QString qemu_bin;
			Emulator emul = Get_Default_Emulator();
			QMap<QString, QString> bins = emul.Get_Binary_Files();
			if( bins.contains( "qemu-system-aarch64" ) )
				qemu_bin = bins[ "qemu-system-aarch64" ];
			QString code = Find_UEFI_Firmware_CODE( qemu_bin );
			if( code.isEmpty() )
				Label_Win11_UEFI_Status->setText( tr("<font color='orange'>UEFI firmware (AAVMF/EDK2 aarch64) not found automatically. "
					"Install QEMU EDK2/AAVMF or set UEFI paths after creating the VM.</font>") );
			else
				Label_Win11_UEFI_Status->setText( tr("UEFI CODE found: %1").arg( code ) );
			
			ui.Wizard_Pages->setCurrentWidget( Win11_ARM_Page );
			ui.Label_Page->setText( tr("Windows 11 ARM Install") );
		}
		else if( Is_Intel_MacOS_Template() )
		{
			QString qemu_bin;
			Emulator emul = Get_Default_Emulator();
			QMap<QString, QString> bins = emul.Get_Binary_Files();
			if( bins.contains( "qemu-system-x86_64" ) )
				qemu_bin = bins[ "qemu-system-x86_64" ];
			else if( bins.contains( "qemu-system-x86" ) )
				qemu_bin = bins[ "qemu-system-x86" ];
			QString code = Find_UEFI_Firmware_CODE( qemu_bin, QStringLiteral( "x86_64" ) );
			if( code.isEmpty() )
				Label_Intel_Mac_UEFI_Status->setText( tr(
					"<font color='orange'>OVMF CODE not found automatically. "
					"Install OVMF / edk2-x86_64 firmware or set UEFI paths after creating the VM.</font>" ) );
			else
				Label_Intel_Mac_UEFI_Status->setText( tr( "OVMF CODE found: %1" ).arg( code ) );
			Show_Intel_MacOS_Page();
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
			ui.Label_Page->setText( tr("Network") );
		}
	}
	else if( Win11_ARM_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( ! CH_Win11_Already_Installed->isChecked() && Edit_Win11_ISO->text().isEmpty() )
		{
			AQGraphic_Warning( tr("Windows 11 ARM"),
				tr("Please select a Windows 11 ARM ISO, or check \"Windows is already installed on this disk\".") );
			return;
		}
		if( RB_Win11_Existing_Disk->isChecked() && Edit_Win11_Existing_Disk->text().isEmpty() )
		{
			AQGraphic_Warning( tr("Windows 11 ARM"),
				tr("Please select an existing disk image, or choose \"Create a new disk image\".") );
			return;
		}
		ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
		ui.Label_Page->setText( tr("Network") );
	}
	else if( Intel_MacOS_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( ! CH_Intel_Mac_Supply_Files->isChecked() )
		{
			AQGraphic_Warning( tr("Intel macOS"),
				tr("Please confirm that you will supply all Apple/OpenCore/OVMF files yourself.") );
			return;
		}
		const QString oc = QDir::toNativeSeparators( Edit_Intel_Mac_OpenCore->text().trimmed() );
		Edit_Intel_Mac_OpenCore->setText( oc );
		if( oc.isEmpty() )
		{
			AQGraphic_Warning( tr("Intel macOS"),
				tr("Please select your OpenCore ISO or disk image.") );
			return;
		}
		if( ! QFile::exists( oc ) )
		{
			AQGraphic_Warning( tr("Intel macOS"),
				tr( "OpenCore file does not exist:\n%1" ).arg( oc ) );
			return;
		}
		if( Edit_Intel_Mac_OSK->text().trimmed().isEmpty() )
		{
			AQGraphic_Warning( tr("Intel macOS"),
				tr("Please paste your Apple SMC OSK. AQEMU does not provide one.") );
			return;
		}
		if( RB_Intel_Mac_Existing_Disk->isChecked() )
		{
			const QString disk = QDir::toNativeSeparators( Edit_Intel_Mac_Existing_Disk->text().trimmed() );
			Edit_Intel_Mac_Existing_Disk->setText( disk );
			if( disk.isEmpty() )
			{
				AQGraphic_Warning( tr("Intel macOS"),
					tr("Please select an existing system disk, or choose \"Create a new disk image\".") );
				return;
			}
			if( ! QFile::exists( disk ) )
			{
				AQGraphic_Warning( tr("Intel macOS"),
					tr( "System disk file does not exist:\n%1" ).arg( disk ) );
				return;
			}
		}
		const QString recovery = QDir::toNativeSeparators( Edit_Intel_Mac_Recovery->text().trimmed() );
		Edit_Intel_Mac_Recovery->setText( recovery );
		if( ! recovery.isEmpty() && ! QFile::exists( recovery ) )
		{
			AQGraphic_Warning( tr("Intel macOS"),
				tr( "Recovery / installer file does not exist:\n%1" ).arg( recovery ) );
			return;
		}
		ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
		ui.Label_Page->setText( tr("Network") );
	}
	else if( ui.Custom_HDD_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Is_Windows11_ARM_Template() )
		{
			ui.Wizard_Pages->setCurrentWidget( Win11_ARM_Page );
			ui.Label_Page->setText( tr("Windows 11 ARM Install") );
		}
		else if( Is_Intel_MacOS_Template() )
		{
			Show_Intel_MacOS_Page();
		}
		else
		{
			ui.Wizard_Pages->setCurrentWidget( ui.Network_Page );
			ui.Label_Page->setText( tr("Network") );
		}
	}
	else if( ui.Network_Page == ui.Wizard_Pages->currentWidget() )
	{
		ui.Wizard_Pages->setCurrentWidget( ui.Finish_Page );
		ui.Button_Next->setText( tr("&Finish") );
		ui.Label_Page->setText( tr("Finish!") );
        Create_New_VM(true);
        ui.VM_Information_Text->setHtml(New_VM->GenerateHTMLInfoText(3));
		Update_Finish_Page_Guidance();
	}
	else if( ui.Finish_Page == ui.Wizard_Pages->currentWidget() )
	{
		if( Create_New_VM() ) accept();
	}
	else
	{
		AQError( "void VM_Wizard_Window::on_Button_Next_clicked()",
				 "Default Section!" );
	}
}

void VM_Wizard_Window::applyTemplate()
{
	// Use Selected Template
	if( ui.RB_VM_Template->isChecked() )
	{
		if( ! New_VM->Load_VM(OS_Templates_List[ui.CB_OS_Type->currentIndex()-1].filePath()) )
		{
			AQGraphic_Error( "void VM_Wizard_Window::Create_New_VM()", tr("Error!"),
							 tr("Cannot Create New VM from Template!") );
			return;
		}
	}
	
	// Emulator
	New_VM->Set_Emulator( Current_Emulator );
	
	// Find CPU List For This Template
	bool devices_found = false;
	
	if( ui.RB_VM_Template->isChecked() )
	{
		if( ui.RB_Emulator_KVM->isChecked() )
			New_VM->Set_Machine_Accelerator(VM::KVM);
		else
			New_VM->Set_Machine_Accelerator(VM::TCG);
			
		Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
		devices_found = true;
	}
	else
	{
		if( ui.RB_Emulator_KVM->isChecked() )
			New_VM->Set_Machine_Accelerator(VM::KVM);
		else
			New_VM->Set_Machine_Accelerator(VM::TCG);

		if( Three_Path_Active && ! Selected_Target.isEmpty() )
		{
			QString qemu_name = "qemu-system-" + Selected_Target;
			if( All_Systems.contains( qemu_name ) )
			{
				Current_Devices = &All_Systems[ qemu_name ];
				New_VM->Set_Computer_Type( Current_Devices->System.QEMU_Name );
				devices_found = true;
			}
		}

		if( ! devices_found )
		{
			// Find from combo caption (Generate VM)
			QString compCaption = ui.CB_Computer_Type->currentText();
			for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); it++ )
			{
				if( it.value().System.Caption == compCaption )
				{
					Current_Devices = &it.value();
					devices_found = true;
					New_VM->Set_Computer_Type( Current_Devices->System.QEMU_Name );
					break;
				}
			}
		}

		if( ! devices_found )
		{
			New_VM->Set_Computer_Type( "qemu-system-x86_64" );
			Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
			devices_found = ! Current_Devices->System.QEMU_Name.isEmpty();
		}

		if( Three_Path_Active && ! Selected_Machine_Id.isEmpty() )
			New_VM->Set_Machine_Type( Selected_Machine_Id );
	}
	
	// Use Selected Template
	if( ui.RB_VM_Template->isChecked() )
	{
		// Name
		ui.Edit_VM_Name->setText( New_VM->Get_Machine_Name() );
		
		// Memory
		ui.Memory_Size->setValue( New_VM->Get_Memory_Size() );

		// CPU Cores
		ui.SB_CPU_Cores->setValue( New_VM->Get_SMP_CPU_Count() );
		
		// HDA
		double hda_size = New_VM->Get_HDA().Get_Virtual_Size_in_GB();
		
		if( hda_size != 0.0 )
			ui.SB_HDD_Size->setValue( hda_size );
		else
			ui.SB_HDD_Size->setValue( 10.0 );
		
		// Network
		ui.RB_User_Mode_Network->setChecked( New_VM->Get_Use_Network() );

		// Find CPU List For This Template
		Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
		if( ! Current_Devices->System.QEMU_Name.isEmpty() ) devices_found = true;
	}
	else // Create New VM in Date Mode
	{
		if( Three_Path_Active && Guest_RAM_MB > 0 )
			ui.Memory_Size->setValue( Guest_RAM_MB );
		else
			ui.Memory_Size->setValue( 2048 );
		if( Three_Path_Active && Guest_HDD_GB > 0 )
			ui.SB_HDD_Size->setValue( Guest_HDD_GB );
		else
			ui.SB_HDD_Size->setValue( 20.0 );
		ui.SB_CPU_Cores->setValue( 1 );

		// Find CPU List For This Template
		QString compCaption = ui.CB_Computer_Type->currentText();
		for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); it++ )
		{
			if( it.value().System.Caption == compCaption )
			{
				Current_Devices = &it.value();
				/*if( ! Current_Devices->System.QEMU_Name.isEmpty() )*/ devices_found = true;
				New_VM->Set_Computer_Type( Current_Devices->System.QEMU_Name );
			}
		}
	}

	if( ! devices_found )
	{
		AQGraphic_Error( "void VM_Wizard_Window::applyTemplate()", tr("Error!"),
						tr("Cannot Find Emulator System ID!") );
	}
	else
	{
		// Add CPU's
		ui.CB_CPU_Type->clear();
		for( int cx = 0; cx < Current_Devices->CPU_List.count(); ++cx )
			ui.CB_CPU_Type->addItem( Current_Devices->CPU_List[cx].Caption );
	}

	// Typical or custom mode
    Typical_Or_Custom();
}


void VM_Wizard_Window::Typical_Or_Custom()
{
	if( ui.RB_Typical->isChecked() )
	{
		ui.Label_Page->setText( tr("Virtual Machine Name") );
		on_Edit_VM_Name_textEdited( ui.Edit_VM_Name->text() );

		ui.Label_Caption_CPU_Type->setVisible( false );
		ui.Line_CPU_Type->setVisible( false );
		ui.Label_CPU_Type->setVisible( false );
		ui.CB_CPU_Type->setVisible( false );

		ui.Label_Caption_CPU_Cores->setVisible( false );
		ui.Line_CPU_Cores->setVisible( false );
		ui.Label_CPU_Cores->setVisible( false );
		ui.SB_CPU_Cores->setVisible( false );
	}
	else
	{
		ui.Label_Page->setText( tr("VM Name and CPU Type") );
		on_Edit_VM_Name_textEdited( ui.Edit_VM_Name->text() );

		ui.Label_Caption_CPU_Type->setVisible( true );
		ui.Line_CPU_Type->setVisible( true );
		ui.Label_CPU_Type->setVisible( true );
		ui.CB_CPU_Type->setVisible( true );

		ui.Label_Caption_CPU_Cores->setVisible( true );
		ui.Line_CPU_Cores->setVisible( true );
		ui.Label_CPU_Cores->setVisible( true );
		ui.SB_CPU_Cores->setVisible( true );
	}
}

bool VM_Wizard_Window::Load_OS_Templates()
{
	QList<QString> tmp_list = Get_Templates_List();

	for( int ax = 0; ax < tmp_list.count(); ++ax )
	{
		OS_Templates_List.append( QFileInfo(tmp_list[ax]) );
	}

	for( int ix = 0; ix < OS_Templates_List.count(); ++ix )
	{
		ui.CB_OS_Type->addItem( OS_Templates_List[ix].completeBaseName() );
	}

	 // no items found
	if( ui.CB_OS_Type->count() < 2 ) return false;
	else return true;
}

bool VM_Wizard_Window::Create_New_VM(bool simulate)
{
	// Icon
	QString icon_path = Find_OS_Icon( ui.Edit_VM_Name->text() );
	if( icon_path.isEmpty() && Is_Intel_MacOS_Template() )
		icon_path = QStringLiteral( ":/default_macos.png" );
	if( icon_path.isEmpty() && Three_Path_Active && ! Selected_OS_Name.isEmpty() )
		icon_path = Find_OS_Icon( Selected_OS_Name );
	if( icon_path.isEmpty() )
	{
		AQWarning( "void VM_Wizard_Window::Create_New_VM()", "Icon for new VM not Found!" );
		New_VM->Set_Icon_Path( ":/other.png" );
	}
	else
	{
		New_VM->Set_Icon_Path( icon_path );
	}

	// Name
	New_VM->Set_Machine_Name( ui.Edit_VM_Name->text() );

	// Create path valid string
	QString VM_File_Name = Get_FS_Compatible_VM_Name( ui.Edit_VM_Name->text() );

	// Set Computer Type?
	if( ui.RB_Generate_VM->isChecked() )
	{
		New_VM->Set_Computer_Type( Current_Devices->System.QEMU_Name );
	}

	if( Three_Path_Active && ! Selected_Machine_Id.isEmpty() )
		New_VM->Set_Machine_Type( Selected_Machine_Id );

	// RAM
	New_VM->Set_Memory_Size( ui.Memory_Size->value() );

	// CPU Cores
	New_VM->Set_SMP_CPU_Count( ui.SB_CPU_Cores->value() );

	// Wizard Mode
	if( ui.RB_Typical->isChecked() )
	{
		QString hd_path = Settings.value( "VM_Directory", "~" ).toString() + VM_File_Name;
		
		if( Is_Windows11_ARM_Template() && RB_Win11_Existing_Disk->isChecked() )
		{
			New_VM->Set_HDA( VM_HDD(true, Edit_Win11_Existing_Disk->text()) );
		}
		else if( Is_Intel_MacOS_Template() && RB_Intel_Mac_Existing_Disk->isChecked() )
		{
			New_VM->Set_HDA( VM_HDD(true, Edit_Intel_Mac_Existing_Disk->text()) );
		}
		else
		{
			// Hard Disk
			VM::Device_Size hd_size;
			hd_size.Size = ui.SB_HDD_Size->value();
			hd_size.Suffix = VM::Size_Suf_Gb;

			if ( ! simulate )
				Create_New_HDD_Image( hd_path + "_HDA.img", hd_size );

			New_VM->Set_HDA( VM_HDD(true, hd_path + "_HDA.img") );
		}

		// Other HDD's
		if( New_VM->Get_HDB().Get_Enabled() )
		{
            if ( ! simulate )
                Create_New_HDD_Image( hd_path + "_HDB.img", New_VM->Get_HDB().Get_Virtual_Size() );
			New_VM->Set_HDB( VM_HDD(true, hd_path + "_HDB.img") );
		}
		
		if( New_VM->Get_HDC().Get_Enabled() )
		{
            if ( ! simulate )
                Create_New_HDD_Image( hd_path + "_HDC.img", New_VM->Get_HDC().Get_Virtual_Size() );
			New_VM->Set_HDC( VM_HDD(true, hd_path + "_HDC.img") );
		}
		
		if( New_VM->Get_HDD().Get_Enabled() )
		{
            if ( ! simulate )
                Create_New_HDD_Image( hd_path + "_HDD.img", New_VM->Get_HDD().Get_Virtual_Size() );
			New_VM->Set_HDD( VM_HDD(true, hd_path + "_HDD.img") );
		}
	}
	else
	{
		bool devices_found = false;
		
		// CPU Type
		if( ui.RB_VM_Template->isChecked() )
		{
			Current_Devices = &All_Systems[ New_VM->Get_Computer_Type() ];
			if( ! Current_Devices->System.QEMU_Name.isEmpty() ) devices_found = true;
		}
		else
		{
			// Find QEMU System Name in CB_Computer_Type (do not force x86_64 just because KVM is on)
			for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin(); it != All_Systems.constEnd(); it++ )
			{
				if( it.value().System.Caption == ui.CB_Computer_Type->currentText() )
				{
					Current_Devices = &it.value();
					devices_found = true;
					break;
				}
			}
			if( ! devices_found && Three_Path_Active && ! Selected_Target.isEmpty() )
			{
				QString qemu_name = "qemu-system-" + Selected_Target;
				if( All_Systems.contains( qemu_name ) )
				{
					Current_Devices = &All_Systems[ qemu_name ];
					devices_found = true;
				}
			}
		}
		
		if( ! devices_found )
		{
			AQGraphic_Error( "bool VM_Wizard_Window::Create_New_VM()", tr("Error!"),
							 tr("Cannot Find QEMU System ID!") );
			return false;
		}
		
		New_VM->Set_CPU_Type( Current_Devices->CPU_List[ui.CB_CPU_Type->currentIndex()].QEMU_Name );
		
		// Hard Disk
		if( ! ui.Edit_HDA_File_Name->text().isEmpty() )
			New_VM->Set_HDA( VM_HDD(true, ui.Edit_HDA_File_Name->text()) );
		else
			New_VM->Set_HDA( VM_HDD(false, "") );
	}
	
	// Network
	if( ui.RB_User_Mode_Network->isChecked() )
	{
		if( New_VM->Get_Network_Cards_Count() == 0 )
		{
			New_VM->Set_Use_Network( true );
			VM_Net_Card net_card;
			net_card.Set_Net_Mode( VM::Net_Mode_Usermode );
			
			New_VM->Add_Network_Card( net_card );
		}
	}
	else if( ui.RB_No_Network->isChecked() )
	{
		New_VM->Set_Use_Network( false );
		
		for( int rx = 0; rx < New_VM->Get_Network_Cards_Count(); ++rx )
		{
			New_VM->Delete_Network_Card( 0 );
		}
	}
	
	// Set Emulator Name (version) to Default ("")
	Emulator tmp_emul = New_VM->Get_Emulator();
	tmp_emul.Set_Name( "" );
	New_VM->Set_Emulator( tmp_emul );
	
	if( Is_Windows11_ARM_Template() )
		Apply_Windows11_ARM_Profile( simulate );
	else if( Is_Intel_MacOS_Template() )
		Apply_Intel_MacOS_Profile( simulate );
	else if( New_VM->Get_Computer_Type().contains( "aarch64", Qt::CaseInsensitive ) ||
			 New_VM->Get_Computer_Type().contains( "qemu-system-arm", Qt::CaseInsensitive ) )
		Apply_AArch64_Generic_Profile( simulate );
	else if( Three_Path_Active )
		Apply_Guest_Hardware_To_New_VM();

	// Guests that opted into UEFI need a writable OVMF VARS file
	if( ! simulate && New_VM->Use_UEFI() )
	{
		Emulator emul = Get_Default_Emulator();
		QMap<QString, QString> bins = emul.Get_Binary_Files();
		QString qemu_bin = New_VM->Get_Computer_Type();
		if( bins.contains( qemu_bin ) )
			qemu_bin = bins[ qemu_bin ];
		else if( bins.contains( QStringLiteral( "qemu-system-x86_64" ) ) )
			qemu_bin = bins[ QStringLiteral( "qemu-system-x86_64" ) ];
		else
			qemu_bin.clear();
		const QString vars = New_VM->Get_UEFI_VARS_File();
		QString arch = Selected_Target;
		if( arch.isEmpty() && qemu_bin.contains( QLatin1String( "x86_64" ) ) )
			arch = QStringLiteral( "x86_64" );
		if( ! vars.isEmpty() && ! qemu_bin.isEmpty() )
			Prepare_UEFI_VARS_File( vars, qemu_bin, arch );
	}
	
    if ( ! simulate )
    {
        // Create New VM XML File
        New_VM->Create_VM_File( Settings.value("VM_Directory", "~").toString() + VM_File_Name + ".aqemu", false );
    }

	return true;
}

void VM_Wizard_Window::Update_Finish_Page_Guidance()
{
	QString help;
	
	if( Is_Windows11_ARM_Template() )
	{
		if( CH_Win11_Already_Installed->isChecked() )
		{
			help = tr( "<p><b>Windows 11 ARM - ready to boot</b><br>"
				"Defaults match the BVM/kiosk QEMU profile (virt + VirtIO disk/net/GPU, UEFI, USB audio). "
				"Lifecycle mode is set to <b>Normal</b>. "
				"Use the Windows 11 ARM buttons on the VM page if you need Install or First boot profiles later.</p>" );
		}
		else
		{
			help = tr( "<p><b>Windows 11 ARM - install checklist</b></p><ol>"
				"<li>Lifecycle is set to <b>Install Windows</b> (ramfb + USB installer ISO).</li>"
				"<li>Start the VM and complete Windows Setup.</li>"
				"<li>After Setup finishes, click <b>First boot</b> (virtio-gpu, no ISO) to finish OOBE, then <b>Normal</b> for everyday use.</li>"
				"<li>Use <b>Normal</b> for everyday use.</li>"
				"</ol>"
				"<p>All machine/CPU/device options remain editable in the main GUI after creation.</p>" );
		}
	}
	else if( Selected_OS_Name.contains( "Mac OS X PPC" ) ||
	         Selected_OS_Name.startsWith( "Mac OS 7" ) ||
	         Selected_OS_Name.startsWith( "Mac OS 8" ) ||
	         Selected_OS_Name.startsWith( "Mac OS 9" ) )
	{
		help = tr( "<p><b>Classic Mac OS (PowerPC)</b></p><ul>"
			"<li>Uses <code>qemu-system-ppc</code> and machine <code>mac99</code>.</li>"
			"<li>Attach your own Mac OS CD/ISO or a prepared HDD image in Device Manager.</li>"
			"<li>AQEMU does not ship Apple install media.</li>"
			"<li>If Start fails finding a binary, install a QEMU build that includes "
			"<code>qemu-system-ppc</code> and re-run the First Start Wizard.</li>"
			"</ul>" );
	}
	else if( Selected_OS_Name.contains( "macOS", Qt::CaseInsensitive ) ||
	         Selected_OS_Name.contains( "Mac OS X Intel" ) ||
	         Selected_OS_Name.contains( "Darwin" ) )
	{
		help = tr( "<p><b>Intel macOS (experimental)</b></p><ul>"
			"<li>You must supply OpenCore boot disk, OVMF firmware, OSK, and install/system disks.</li>"
			"<li>AQEMU does not ship Apple OS files or a default OSK.</li>"
			"<li>On Windows, WHPX is used unless you enable Launch via WSL/KVM.</li>"
			"</ul>" );
	}
	else if( Selected_OS_Name == QLatin1String( "SteamOS" ) )
	{
		help = tr( "<p><b>SteamOS (Deck recovery)</b></p><ol>"
			"<li>Download Valve’s recovery <code>.bz2</code>, extract the raw <code>.img</code>.</li>"
			"<li>Attach that <code>.img</code> as a second raw disk (Device Manager / HDB) — it is not an ISO.</li>"
			"<li>Primary disk is <b>NVMe</b> (installer looks for <code>/dev/nvme0n1</code>). UEFI/OVMF is enabled.</li>"
			"<li>Xbox / USB gamepads are auto-passed through when connected (Advanced Options).</li>"
			"<li>After install finishes: <b>do not reboot into Gaming Mode</b> — open a terminal "
			"(Ctrl+Alt+T) and set Plasma on A/B partitions, then reboot.</li>"
			"<li>Gaming Mode needs the Deck’s AMD GPU; optional AMD GPU passthrough is advanced "
			"and not required for Plasma desktop.</li>"
			"</ol>" );
	}
	else if( Selected_OS_Name.contains( QLatin1String( "Raspberry Pi" ) ) ||
	         Selected_OS_Name.contains( QLatin1String( "Arduino" ) ) ||
	         Selected_OS_Name.contains( QLatin1String( "micro:bit" ) ) ||
	         Selected_OS_Name == QLatin1String( "NeXT Cube" ) ||
	         Selected_OS_Name == QLatin1String( "AmigaOne" ) ||
	         Selected_OS_Name == QLatin1String( "Pegasos II" ) ||
	         Selected_OS_Name.contains( QLatin1String( "Quadra" ) ) ||
	         Selected_OS_Name.contains( QLatin1String( "Cubieboard" ) ) ||
	         Selected_OS_Name.contains( QLatin1String( "Orange Pi" ) ) ||
	         Selected_OS_Name.startsWith( QLatin1String( "ARM " ) ) )
	{
		help = tr( "<p><b>Consoles &amp; Retro (QEMU boards)</b></p><ul>"
			"<li>These are real QEMU machine models (Pi, micro:bit, Arduino AVR, NeXT, AmigaOne…), "
			"not MAME arcade/NES/PlayStation emulation.</li>"
			"<li>Many need firmware, SD images, or ELF/hex binaries instead of a desktop ISO.</li>"
			"<li>Check that your QEMU build includes the matching <code>qemu-system-*</code> binary "
			"and <code>-M</code> machine.</li>"
			"</ul>" );
	}
	else if( Selected_OS_Name == QLatin1String( "Chrome OS Flex" ) )
	{
		help = tr( "<p><b>Chrome OS Flex</b></p><ol>"
			"<li>Extract Google’s recovery archive to a raw <code>.bin</code> (not an ISO).</li>"
			"<li>Attach the <code>.bin</code> as a raw disk (HDB / Device Manager); install onto the VirtIO qcow2.</li>"
			"<li>Video defaults to VirtIO VGA. On Linux/KVM you can switch to "
			"<b>VirtIO VGA (GL / OpenGL)</b> for better graphics.</li>"
			"<li>Google does not officially support Flex in VMs — expect rough edges.</li>"
			"</ol>" );
	}
	else if( ui.RB_Generate_VM->isChecked() &&
			 ( ui.CB_Computer_Type->currentText().contains( "AArch64", Qt::CaseInsensitive ) ||
			   ui.CB_Computer_Type->currentText().contains( "ARM", Qt::CaseInsensitive ) ) )
	{
		help = tr( "<p><b>AArch64 / ARM custom VM</b><br>"
			"Starts with machine <code>virt</code> and VirtIO-friendly defaults so Linux and other ARM guests work. "
			"Pick any machine/CPU/video/network option in the main window for your OS.</p>" );
	}
	else
	{
		return;
	}
	
	ui.VM_Information_Text->setHtml( ui.VM_Information_Text->toHtml() + help );
}

void VM_Wizard_Window::Apply_Windows11_ARM_Profile( bool simulate )
{
	QString vm_dir = Settings.value( "VM_Directory", "~" ).toString();
	QString vm_base = Get_FS_Compatible_VM_Name( ui.Edit_VM_Name->text() );
	
	// CPU: host with Linux KVM; max on Windows / TCG (Windows aarch64 QEMU has no 'host')
	if( New_VM->Get_Machine_Accelerator() == VM::KVM )
	{
		#ifdef Q_OS_WIN32
		New_VM->Set_CPU_Type( "max" );
		#else
		New_VM->Set_CPU_Type( "host" );
		#endif
	}
	else if( New_VM->Get_CPU_Type().isEmpty() )
		New_VM->Set_CPU_Type( "max" );
	
	New_VM->Set_Machine_Type( "virt" );
	New_VM->Set_Video_Card( "virtio-gpu-pci" );
	// Fixed EDID size — Win11 ARM often cannot change resolution in-guest; 720p is a good TCG default.
	// Install mode overrides video to ramfb below.
	New_VM->Set_Display_Resolution( QStringLiteral( "1280x720" ) );
	New_VM->Set_SMP_CPU_Count( 4 );
	New_VM->Set_Memory_Size( 8192 );
	New_VM->Use_USB_Hub( true );
	New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
	New_VM->Set_Mouse_USB_Controller( QStringLiteral( "xhci" ) );
	New_VM->Set_Mouse_USB_Version( 0 );
	New_VM->Use_VirtIO_RNG( true );
	New_VM->Use_VirtIO_Balloon( true );
	New_VM->Use_VirtIO_Keyboard( true );
	
	VM::Sound_Cards audio;
	// win11-pi5-kiosk uses usb-audio (virtio-win has no Windows ARM sound driver)
	audio.Audio_USB = true;
	audio.Audio_VirtIO = false;
	New_VM->Set_Audio_Cards( audio );
	
	// Network virtio-net-pci
	if( New_VM->Get_Network_Cards_Count() > 0 )
	{
		VM_Net_Card card = New_VM->Get_Network_Card( 0 );
		card.Set_Card_Model( "virtio-net-pci" );
		New_VM->Set_VM_Network_Card( 0, card );
	}
	
	// VirtIO disk
	VM_HDD hda = New_VM->Get_HDA();
	if( hda.Get_Enabled() )
	{
		VM_Native_Storage_Device native;
		native.Use_File_Path( true );
		native.Set_File_Path( hda.Get_File_Name() );
		native.Use_Interface( true );
		native.Set_Interface( VM::DI_Virtio );
		native.Use_Cache( true );
		#ifdef Q_OS_WIN32
		// cache=none (O_DIRECT) breaks qcow2 on Windows with a misleading format error
		native.Set_Cache( "writeback" );
		#else
		native.Set_Cache( "none" );
		#endif
		native.Use_AIO( true );
		native.Set_AIO( "threads" );
		native.Use_Media( true );
		native.Set_Media( VM::DM_Disk );
		hda.Set_Native_Device( native );
		New_VM->Set_HDA( hda );
	}
	
	// Install ISO / boot order
	if( CH_Win11_Already_Installed->isChecked() )
	{
		New_VM->Set_CD_ROM( VM_Storage_Device( false, "" ) );
		QList<VM::Boot_Order> boot;
		VM::Boot_Order b;
		b.Type = VM::Boot_From_HDD;
		b.Enabled = true;
		boot << b;
		New_VM->Set_Boot_Order_List( boot );
		New_VM->Set_Win11_Lifecycle_Mode( VM::Win11_Normal );
		New_VM->Set_Video_Card( "virtio-gpu-pci" );
	}
	else if( ! Edit_Win11_ISO->text().isEmpty() )
	{
		VM_Storage_Device cd( true, Edit_Win11_ISO->text() );
		New_VM->Set_CD_ROM( cd );
		
		QList<VM::Boot_Order> boot;
		VM::Boot_Order bcd;
		bcd.Type = VM::Boot_From_CDROM;
		bcd.Enabled = true;
		boot << bcd;
		VM::Boot_Order bhdd;
		bhdd.Type = VM::Boot_From_HDD;
		bhdd.Enabled = true;
		boot << bhdd;
		New_VM->Set_Boot_Order_List( boot );
		New_VM->Set_Win11_Lifecycle_Mode( VM::Win11_Install );
		// BVM firstboot: ramfb only during install
		New_VM->Set_Video_Card( "ramfb" );
		
		if( CH_Win11_VirtIO_ISO->isChecked() && ! Edit_Win11_VirtIO_ISO->text().isEmpty() )
		{
			VM_Native_Storage_Device virtio_cd;
			virtio_cd.Use_File_Path( true );
			virtio_cd.Set_File_Path( Edit_Win11_VirtIO_ISO->text() );
			virtio_cd.Use_Media( true );
			virtio_cd.Set_Media( VM::DM_CD_ROM );
			QList<VM_Native_Storage_Device> extras = New_VM->Get_Storage_Devices_List();
			extras.append( virtio_cd );
			New_VM->Set_Storage_Devices_List( extras );
		}
	}
	
	// UEFI dual pflash
	Emulator emul = Get_Default_Emulator();
	QString qemu_bin;
	QMap<QString, QString> bins = emul.Get_Binary_Files();
	if( bins.contains( "qemu-system-aarch64" ) )
		qemu_bin = bins[ "qemu-system-aarch64" ];
	
	QString code = Find_UEFI_Firmware_CODE( qemu_bin );
	QString vars_dest = vm_dir + vm_base + "_VARS.fd";
	
	New_VM->Use_UEFI( true );
	if( ! code.isEmpty() )
		New_VM->Set_UEFI_CODE_File( code );
	
	New_VM->Set_UEFI_VARS_File( vars_dest );
	if( ! simulate )
		Prepare_UEFI_VARS_File( vars_dest, qemu_bin );
}

void VM_Wizard_Window::Apply_Intel_MacOS_Profile( bool simulate )
{
	QString vm_dir = Settings.value( "VM_Directory", "~" ).toString();
	QString vm_base = Get_FS_Compatible_VM_Name( ui.Edit_VM_Name->text() );

	New_VM->Use_Intel_MacOS_Profile( true );
	New_VM->Set_Machine_Type( QStringLiteral( "q35" ) );
	New_VM->Set_CPU_Type( QStringLiteral( "Skylake-Client-v4" ) );
	New_VM->Set_SMP_CPU_Count( 2 );
	if( New_VM->Get_Memory_Size() < 4096 )
		New_VM->Set_Memory_Size( 4096 );
	New_VM->Set_Video_Card( QStringLiteral( "vmware" ) ); // vmware-svga — HiDPI/4K friendly vs std VGA
	New_VM->Set_Display_Resolution( QStringLiteral( "native" ) ); // match host; OpenCore patched on start
	New_VM->Use_USB_Hub( true );
	New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
	New_VM->Set_Mouse_USB_Controller( QStringLiteral( "xhci" ) );
	New_VM->Set_Mouse_USB_Version( 0 );
	New_VM->Use_ACPI( true );

	VM::Sound_Cards audio;
	audio.Audio_HDA = true;
	New_VM->Set_Audio_Cards( audio );

	if( New_VM->Get_Network_Cards_Count() > 0 )
	{
		VM_Net_Card card = New_VM->Get_Network_Card( 0 );
		card.Set_Card_Model( QStringLiteral( "virtio-net-pci" ) );
		New_VM->Set_VM_Network_Card( 0, card );
	}

	New_VM->Set_OpenCore_Boot_Path( QDir::toNativeSeparators( AQ_Normalize_File_Path( Edit_Intel_Mac_OpenCore->text() ) ) );
	New_VM->Set_Mac_Recovery_Image_Path( QDir::toNativeSeparators( AQ_Normalize_File_Path( Edit_Intel_Mac_Recovery->text() ) ) );
	New_VM->Set_Apple_SMC_OSK( Edit_Intel_Mac_OSK->text() );
	New_VM->Use_Apple_SMC( ! Edit_Intel_Mac_OSK->text().trimmed().isEmpty() );
	New_VM->Use_Launch_Via_WSL( CH_Intel_Mac_Prefer_WSL->isChecked() );

#ifdef Q_OS_WIN32
	if( CH_Intel_Mac_Prefer_WSL->isChecked() )
	{
		Settings.setValue( QStringLiteral( "WSL_Launch/Enabled" ), true );
	}
#endif

	QString qemu_bin;
	Emulator emul = Get_Default_Emulator();
	QMap<QString, QString> bins = emul.Get_Binary_Files();
	if( bins.contains( "qemu-system-x86_64" ) )
		qemu_bin = bins[ "qemu-system-x86_64" ];
	else if( bins.contains( "qemu-system-x86" ) )
		qemu_bin = bins[ "qemu-system-x86" ];

	QString code = Find_UEFI_Firmware_CODE( qemu_bin, QStringLiteral( "x86_64" ) );
	QString vars_dest = vm_dir + vm_base + "_OVMF_VARS.fd";
	New_VM->Use_UEFI( true );
	if( ! code.isEmpty() )
		New_VM->Set_UEFI_CODE_File( code );
	New_VM->Set_UEFI_VARS_File( vars_dest );
	if( ! simulate )
		Prepare_UEFI_VARS_File( vars_dest, qemu_bin, QStringLiteral( "x86_64" ) );

	QList<VM::Boot_Order> boot;
	const QString oc_path = Edit_Intel_Mac_OpenCore->text().trimmed().toLower();
	if( oc_path.endsWith( QLatin1String( ".iso" ) ) )
	{
		VM::Boot_Order bcd;
		bcd.Type = VM::Boot_From_CDROM;
		bcd.Enabled = true;
		boot << bcd;
	}
	VM::Boot_Order bhdd;
	bhdd.Type = VM::Boot_From_HDD;
	bhdd.Enabled = true;
	boot << bhdd;
	New_VM->Set_Boot_Order_List( boot );
}

void VM_Wizard_Window::Apply_AArch64_Generic_Profile( bool simulate )
{
	// Flexible defaults for any aarch64/ARM guest (Linux, BSD, etc.) - not Win11-specific
	if( New_VM->Get_Machine_Type().isEmpty() )
		New_VM->Set_Machine_Type( "virt" );
	
	if( New_VM->Get_CPU_Type().isEmpty() )
	{
		#ifdef Q_OS_WIN32
		New_VM->Set_CPU_Type( "max" );
		#else
		if( New_VM->Get_Machine_Accelerator() == VM::KVM )
			New_VM->Set_CPU_Type( "host" );
		else
			New_VM->Set_CPU_Type( "max" );
		#endif
	}
	
	if( New_VM->Get_Video_Card().isEmpty() || New_VM->Get_Video_Card() == "std" )
		New_VM->Set_Video_Card( System_Info::Default_Video_Card( New_VM->Get_Computer_Type() ) );
	
	// VirtIO-friendly defaults; user can change everything in the main window
	New_VM->Use_USB_Hub( true );
	New_VM->Set_Mouse_Type( QStringLiteral( "usb-tablet" ) );
	New_VM->Set_Mouse_USB_Controller( QStringLiteral( "xhci" ) );
	New_VM->Use_VirtIO_RNG( true );
	New_VM->Use_VirtIO_Keyboard( true );
	
	if( New_VM->Get_Network_Cards_Count() > 0 )
	{
		VM_Net_Card card = New_VM->Get_Network_Card( 0 );
		QString model = card.Get_Card_Model();
		if( model.isEmpty() || model == "virtio" || model == "ne2k_pci" )
		{
			card.Set_Card_Model( "virtio-net-pci" );
			New_VM->Set_VM_Network_Card( 0, card );
		}
	}
	
	VM_HDD hda = New_VM->Get_HDA();
	if( hda.Get_Enabled() && ! hda.Get_Native_Mode() )
	{
		VM_Native_Storage_Device native;
		native.Use_File_Path( true );
		native.Set_File_Path( hda.Get_File_Name() );
		native.Use_Interface( true );
		native.Set_Interface( VM::DI_Virtio );
		native.Use_Cache( true );
		native.Set_Cache( "writeback" );
		native.Use_AIO( true );
		native.Set_AIO( "threads" );
		native.Use_Media( true );
		native.Set_Media( VM::DM_Disk );
		hda.Set_Native_Device( native );
		New_VM->Set_HDA( hda );
	}
	
	// UEFI when firmware is available (modern ARM guests)
	Emulator emul = Get_Default_Emulator();
	QMap<QString, QString> bins = emul.Get_Binary_Files();
	QString qemu_bin;
	if( bins.contains( New_VM->Get_Computer_Type() ) )
		qemu_bin = bins[ New_VM->Get_Computer_Type() ];
	else if( bins.contains( "qemu-system-aarch64" ) )
		qemu_bin = bins[ "qemu-system-aarch64" ];
	
	QString code = Find_UEFI_Firmware_CODE( qemu_bin );
	if( ! code.isEmpty() )
	{
		QString vm_dir = Settings.value( "VM_Directory", "~" ).toString();
		QString vm_base = Get_FS_Compatible_VM_Name( ui.Edit_VM_Name->text() );
		QString vars_dest = vm_dir + vm_base + "_VARS.fd";
		New_VM->Use_UEFI( true );
		New_VM->Set_UEFI_CODE_File( code );
		New_VM->Set_UEFI_VARS_File( vars_dest );
		if( ! simulate )
			Prepare_UEFI_VARS_File( vars_dest, qemu_bin );
	}
}

QString VM_Wizard_Window::Find_OS_Icon( const QString os_name )
{
	if( os_name.isEmpty() )
	{
		AQError( "QString VM_Wizard_Window::Find_OS_Icon( const QString os_name )",
				 "os_name is Empty!" );
		return "";
	}

	// Find all os icons
	QDir icons_dir( QDir::toNativeSeparators(Settings.value("AQEMU_Data_Folder","").toString() + "/os_icons/") );
	QFileInfoList all_os_icons = icons_dir.entryInfoList( QStringList("*.png"), QDir::Files, QDir::Unsorted );

	QRegExp rex;
	rex.setPatternSyntax( QRegExp::Wildcard );
	rex.setCaseSensitivity( Qt::CaseInsensitive );

	for( int i = 0; i < all_os_icons.count(); i++ )
	{
		rex.setPattern( "*" + all_os_icons[i].baseName() + "*" );

		if( rex.exactMatch(os_name) )
		{
			return all_os_icons[ i ].absoluteFilePath();
		}
	}

	// select os family...

	// Linux
	rex.setPattern( "*linux*" );
	if( rex.exactMatch(os_name) )
		return ":/default_linux.png";

	// Windows
	rex.setPattern( "*windows*" );
	if( rex.exactMatch(os_name) )
		return ":/default_windows.png";

	// Apple / macOS / Darwin
	rex.setPattern( "*mac*os*" );
	if( rex.exactMatch(os_name) )
		return ":/default_macos.png";
	rex.setPattern( "*macos*" );
	if( rex.exactMatch(os_name) )
		return ":/default_macos.png";
	rex.setPattern( "*darwin*" );
	if( rex.exactMatch(os_name) )
		return ":/default_macos.png";
	rex.setPattern( "*osx*" );
	if( rex.exactMatch(os_name) )
		return ":/default_macos.png";
	rex.setPattern( "*apple*" );
	if( rex.exactMatch(os_name) )
		return ":/default_macos.png";

	return ":/other.png";
}

void VM_Wizard_Window::on_RB_VM_Template_toggled( bool on )
{
	if( on )
	{
		if( ui.CB_OS_Type->currentIndex() == 0 )
			ui.Button_Next->setEnabled( false );
		else
			ui.Button_Next->setEnabled( true );
	}
}

void VM_Wizard_Window::on_RB_Generate_VM_toggled( bool on )
{
	if( on )
	{
		if( ui.CB_Computer_Type->count() > 0 )
		{
			int defaultIndex = ui.CB_Computer_Type->currentIndex();

			// Guest OS / Platform / Arch already chose a target — honour it (never force 64-bit)
			if( ! Selected_Target.isEmpty() )
			{
				const QString qemu_name = QStringLiteral( "qemu-system-" ) + Selected_Target;
				if( All_Systems.contains( qemu_name ) )
				{
					const QString want = All_Systems[ qemu_name ].System.Caption;
					for( int ix = 0; ix < ui.CB_Computer_Type->count(); ++ix )
					{
						if( ui.CB_Computer_Type->itemText( ix ) == want )
						{
							defaultIndex = ix;
							break;
						}
					}
				}
			}
			else if( defaultIndex < 0 )
			{
				// No prior selection: prefer host arch, else first item
				defaultIndex = 0;
				const QString host_arch = AQ_Get_Host_CPU_Architecture();
				for( int ix = 0; ix < ui.CB_Computer_Type->count(); ++ix )
				{
					const QString text = ui.CB_Computer_Type->itemText( ix );
					const QString lower = text.toLower();
					if( host_arch == QLatin1String( "aarch64" ) &&
					    ( lower.contains( "aarch64" ) || lower.contains( "arm 64" ) ) )
					{
						defaultIndex = ix;
						break;
					}
					if( host_arch == QLatin1String( "x86_64" ) &&
					    ( lower.contains( "x86_64" ) || text.contains( "64Bit" ) ) )
					{
						defaultIndex = ix;
						break;
					}
				}
			}

			if( defaultIndex >= 0 )
				ui.CB_Computer_Type->setCurrentIndex( defaultIndex );
		}

		if( ui.CB_Computer_Type->currentIndex() == -1 )
			ui.Button_Next->setEnabled( false );
		else
			ui.Button_Next->setEnabled( true );
	}
}

void VM_Wizard_Window::on_CB_OS_Type_currentIndexChanged( int index )
{
	if( index == 0 )
		ui.Button_Next->setEnabled( false );
	else
		ui.Button_Next->setEnabled( true );
}

void VM_Wizard_Window::on_CB_Computer_Type_currentIndexChanged( int index )
{
	if( index == -1 )
	{
		ui.Button_Next->setEnabled( false );
	}
	else
	{
		ui.Button_Next->setEnabled( true );
		// Keep Selected_Target in sync when user overrides Computer Type
		if( Three_Path_Active && ui.RB_Generate_VM->isChecked() )
		{
			for( QMap<QString, Available_Devices>::const_iterator it = All_Systems.constBegin();
			     it != All_Systems.constEnd(); ++it )
			{
				if( it.value().System.Caption == ui.CB_Computer_Type->currentText() )
				{
					QString qn = it.value().System.QEMU_Name;
					Selected_Target = qn;
					Selected_Target.remove( "qemu-system-" );
					break;
				}
			}
		}
		Update_Guest_Compat_Tip();
	}
}

void VM_Wizard_Window::on_Memory_Size_valueChanged( int value )
{
	int cursorPos = ui.CB_RAM_Size->lineEdit()->cursorPosition();
	
	if( value % 1024 == 0 )
	    ui.CB_RAM_Size->setEditText( QString("%1 GB").arg(value / 1024) );
	else
	    ui.CB_RAM_Size->setEditText( QString("%1 MB").arg(value) );
	
	ui.CB_RAM_Size->lineEdit()->setCursorPosition( cursorPos );
}

void VM_Wizard_Window::on_CB_RAM_Size_editTextChanged( const QString &text )
{
	if( text.isEmpty() )
	    return;
	
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
	
	if( ramStrings[2] == "MB" || ramStrings[2] == "M" ); // Size in megabytes
	else if( ramStrings[2] == "GB" || ramStrings[2] == "G" ) value *= 1024;
	else
	{
		AQGraphic_Warning( tr("Error"),
						   tr("Cannot convert \"%1\" to size suffix! Valid suffixes: MB, GB").arg(ramStrings[2]) );
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
							  "To setup this value, check \"Remove limitation on maximum amount of memory\".")
						   .arg(value).arg(ui.Memory_Size->maximum()) );
		
		on_Memory_Size_valueChanged( ui.Memory_Size->value() ); // Set valid size
		return;
	}
	
	// All OK. Set memory size
	ui.Memory_Size->setValue( value );
}

void VM_Wizard_Window::on_CH_Remove_RAM_Size_Limitation_stateChanged( int state )
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
			AQGraphic_Warning( tr("Error"), tr("Current memory size bigger than all existing host memory!\nUsing maximum available size.") );
		
		ui.Memory_Size->setMaximum( allRAM );
		ui.Label_Available_Free_Memory->setText( QString("%1 MB").arg(allRAM) );
		Update_RAM_Size_ComboBox( allRAM );
	}
}

void VM_Wizard_Window::on_TB_Update_Available_RAM_Size_clicked()
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

void VM_Wizard_Window::Update_RAM_Size_ComboBox( int freeRAM )
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
		AQGraphic_Warning( tr("Error"), tr("Free memory on this system is lower than 32 MB!") );
		return;
	}
	
	if( maxRamIndex > ramSizes.count() )
	{
		AQError( "void VM_Wizard_Window::Update_RAM_Size_ComboBox( int freeRAM )",
				 "maxRamIndex > ramSizes.count()" );
		return;
	}
	
	QString oldText = ui.CB_RAM_Size->currentText();
	
	ui.CB_RAM_Size->clear();
	for( int ix = 0; ix < maxRamIndex; ix++ ) ui.CB_RAM_Size->addItem( ramSizes[ix] );
	
	ui.CB_RAM_Size->setEditText( oldText );
}

void VM_Wizard_Window::on_Edit_VM_Name_textEdited( const QString &text )
{
	if( ui.Edit_VM_Name->text().isEmpty() ) ui.Button_Next->setEnabled( false );
	else ui.Button_Next->setEnabled( true );
}

void VM_Wizard_Window::on_Button_New_HDD_clicked()
{
	Create_HDD_Image_Window Create_HDD_Win( this );
	
	Create_HDD_Win.Set_Image_Size( ui.SB_HDD_Size->value() ); // Set Initial HDA Size
	
	if( Create_HDD_Win.exec() == QDialog::Accepted )
		ui.Edit_HDA_File_Name->setText( Create_HDD_Win.Get_Image_File_Name() );
}

void VM_Wizard_Window::on_Button_Existing_clicked()
{
	QString hddPath = QFileDialog::getOpenFileName( this, tr("Select HDD Image"),
													Get_Last_Dir_Path(ui.Edit_HDA_File_Name->text()),
													tr("All Files (*)") );

	if( ! hddPath.isEmpty() )
		ui.Edit_HDA_File_Name->setText( QDir::toNativeSeparators(hddPath) );
}
