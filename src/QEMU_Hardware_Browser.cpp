/****************************************************************************
** Browse every device group from the QEMU probe for one qemu-system binary.
****************************************************************************/

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "QEMU_Hardware_Browser.h"
#include "QEMU_Probe_Catalog.h"
#include "Utils.h"
#include "VM.h"

namespace {

const QString kBegin = QStringLiteral( "# aqemu-probe-devices" );
const QString kEnd = QStringLiteral( "# aqemu-probe-devices-end" );

struct Probe_Row
{
	QString group;
	QString name;
	QString caption;
	QString kind;
};

QStringList Lines_Of( const QString &text )
{
	return text.split( QRegExp( QStringLiteral( "[\\r\\n]+" ) ), QString::SkipEmptyParts );
}

QString Argument_For( const Probe_Row &row )
{
	const QString id = row.name;
	if( row.kind == QLatin1String( "object" ) )
		return QStringLiteral( "-object " ) + id;
	if( row.kind == QLatin1String( "chardev" ) )
		return QStringLiteral( "-chardev %1,id=char-%1" ).arg( id );
	if( row.kind == QLatin1String( "netdev" ) )
		return QStringLiteral( "-netdev %1,id=nd-%1" ).arg( id );
	if( row.kind == QLatin1String( "machine" ) )
		return QString();
	return QStringLiteral( "-device " ) + id;
}

void Add_Group( QList<Probe_Row> &rows, const QString &group, const QString &kind,
                const QList<Device_Map> &list )
{
	for( int i = 0; i < list.count(); ++i )
	{
		if( list[i].QEMU_Name.trimmed().isEmpty() )
			continue;
		Probe_Row row;
		row.group = group;
		row.name = list[i].QEMU_Name.trimmed();
		row.caption = list[i].Caption.isEmpty() ? row.name : list[i].Caption;
		row.kind = kind;
		rows << row;
	}
}

void Add_Names( QList<Probe_Row> &rows, const QString &group, const QString &kind,
                const QStringList &names )
{
	for( int i = 0; i < names.count(); ++i )
	{
		const QString name = names.at( i ).trimmed();
		if( name.isEmpty() )
			continue;
		Probe_Row row;
		row.group = group;
		row.name = name;
		row.caption = name;
		row.kind = kind;
		rows << row;
	}
}

QStringList Checked_Arguments( const QString &additional )
{
	QStringList out;
	bool in = false;
	const QStringList lines = Lines_Of( additional );
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString t = lines.at( i ).trimmed();
		if( t == kBegin )
		{
			in = true;
			continue;
		}
		if( t == kEnd )
			break;
		if( in && ! t.isEmpty() && ! t.startsWith( QLatin1Char( '#' ) ) )
			out << t;
	}
	return out;
}

QString Without_Block( const QString &additional )
{
	const QStringList lines = additional.split( QRegExp( QStringLiteral( "[\\r\\n]+" ) ) );
	QStringList kept;
	bool in = false;
	for( int i = 0; i < lines.count(); ++i )
	{
		const QString t = lines.at( i ).trimmed();
		if( t == kBegin )
		{
			in = true;
			continue;
		}
		if( t == kEnd )
		{
			in = false;
			continue;
		}
		if( ! in )
			kept << lines.at( i );
	}
	return kept.join( QStringLiteral( "\n" ) ).trimmed();
}

} // namespace

bool QEMU_Hardware_Browser::Edit( QWidget *parent, Virtual_Machine *vm, const QString &arch_hint )
{
	if( ! vm )
		return false;

	QString arch = arch_hint.trimmed();
	if( arch.isEmpty() )
		arch = vm->Get_Computer_Type();
	arch = AQ_Canonical_QEMU_Binary_Name( arch );
	const QString bin = vm->Get_Current_Emulator_Binary_Path( arch );
	if( ! bin.isEmpty() )
		QEMU_Probe_Catalog::Use_Binary_Path( bin );

	Available_Devices ad;
	if( ! QEMU_Probe_Catalog::Load_Architecture( arch, ad ) )
	{
		QMessageBox::warning( parent, QObject::tr( "QEMU hardware" ),
			QObject::tr( "No probe catalog for %1." ).arg( arch ) );
		return false;
	}

	QList<Probe_Row> rows;
	Add_Group( rows, QObject::tr( "Storage" ), QStringLiteral( "device" ), ad.Storage_Device_List );
	Add_Group( rows, QObject::tr( "USB" ), QStringLiteral( "device" ), ad.USB_Device_List );
	Add_Group( rows, QObject::tr( "Input" ), QStringLiteral( "device" ), ad.Input_Device_List );
	Add_Group( rows, QObject::tr( "Controllers" ), QStringLiteral( "device" ), ad.Controller_Device_List );
	Add_Group( rows, QObject::tr( "CPU devices" ), QStringLiteral( "device" ), ad.CPU_Device_List );
	Add_Group( rows, QObject::tr( "Sound" ), QStringLiteral( "device" ), ad.Sound_Device_List );
	Add_Group( rows, QObject::tr( "Watchdog" ), QStringLiteral( "device" ), ad.Watchdog_Device_List );
	Add_Group( rows, QObject::tr( "Miscellaneous" ), QStringLiteral( "device" ), ad.Misc_Device_List );
	Add_Group( rows, QObject::tr( "Other devices" ), QStringLiteral( "device" ), ad.Other_Device_List );
	Add_Group( rows, QObject::tr( "User objects" ), QStringLiteral( "object" ), ad.Object_Type_List );
	Add_Names( rows, QObject::tr( "Character backends" ), QStringLiteral( "chardev" ), ad.Chardev_Backends );
	Add_Names( rows, QObject::tr( "Network backends" ), QStringLiteral( "netdev" ), ad.Netdev_Backends );
	Add_Group( rows, QObject::tr( "Machines" ), QStringLiteral( "machine" ), ad.Machine_List );

	const QStringList already = Checked_Arguments( vm->Get_Additional_Args() );

	QDialog dlg( parent );
	dlg.setWindowTitle( QObject::tr( "QEMU hardware — %1" ).arg( arch ) );
	dlg.resize( 860, 560 );
	QVBoxLayout *lay = new QVBoxLayout( &dlg );
	lay->addWidget( new QLabel( QObject::tr(
		"Everything this qemu-system binary reported. Checked rows are added when the VM starts. "
		"Reims still defaults to reims-vgpu-pci and Apple SoC still defaults to its onboard display." ) ) );

	QSplitter *split = new QSplitter( Qt::Horizontal, &dlg );
	QTreeWidget *tree = new QTreeWidget( split );
	tree->setHeaderLabels( QStringList() << QObject::tr( "Device" ) << QObject::tr( "Id" ) );
	tree->setColumnWidth( 0, 320 );
	QPlainTextEdit *help = new QPlainTextEdit( split );
	help->setReadOnly( true );
	split->addWidget( tree );
	split->addWidget( help );
	lay->addWidget( split, 1 );

	QMap<QString, QTreeWidgetItem *> groups;
	for( int i = 0; i < rows.count(); ++i )
	{
		const Probe_Row &row = rows.at( i );
		if( ! groups.contains( row.group ) )
		{
			QTreeWidgetItem *group = new QTreeWidgetItem( tree, QStringList() << row.group );
			group->setFlags( group->flags() & ~Qt::ItemIsSelectable );
			groups.insert( row.group, group );
		}
		QTreeWidgetItem *item = new QTreeWidgetItem( groups.value( row.group ) );
		item->setText( 0, row.caption );
		item->setText( 1, row.name );
		item->setData( 0, Qt::UserRole, row.name );
		item->setData( 0, Qt::UserRole + 1, row.kind );
		const QString arg = Argument_For( row );
		if( ! arg.isEmpty() )
		{
			item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
			item->setCheckState( 0, already.contains( arg ) ? Qt::Checked : Qt::Unchecked );
		}
	}
	tree->expandToDepth( 0 );

	QObject::connect( tree, &QTreeWidget::currentItemChanged, &dlg,
		[help, arch]( QTreeWidgetItem *item, QTreeWidgetItem * )
		{
			if( ! item || item->data( 0, Qt::UserRole ).toString().isEmpty() )
			{
				help->clear();
				return;
			}
			const QString name = item->data( 0, Qt::UserRole ).toString();
			const QString kind = item->data( 0, Qt::UserRole + 1 ).toString();
			const QString bucket = ( kind == QLatin1String( "machine" ) )
				? QStringLiteral( "machines" )
				: ( kind == QLatin1String( "object" ) ? QStringLiteral( "objects" )
				                                      : QStringLiteral( "devices" ) );
			QString text = QEMU_Probe_Catalog::Property_Help( arch, bucket, name );
			if( text.trimmed().isEmpty() )
				text = QObject::tr( "No property help has been probed for %1 yet." ).arg( name );
			help->setPlainText( text );
		} );

	QDialogButtonBox *box = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg );
	QObject::connect( box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept );
	QObject::connect( box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject );
	lay->addWidget( box );
	if( dlg.exec() != QDialog::Accepted )
		return false;

	QStringList chosen;
	for( int g = 0; g < tree->topLevelItemCount(); ++g )
	{
		QTreeWidgetItem *group = tree->topLevelItem( g );
		for( int c = 0; c < group->childCount(); ++c )
		{
			QTreeWidgetItem *item = group->child( c );
			if( item->checkState( 0 ) != Qt::Checked )
				continue;
			Probe_Row row;
			row.name = item->data( 0, Qt::UserRole ).toString();
			row.kind = item->data( 0, Qt::UserRole + 1 ).toString();
			const QString arg = Argument_For( row );
			if( ! arg.isEmpty() )
				chosen << arg;
		}
	}

	QString rest = Without_Block( vm->Get_Additional_Args() );
	if( ! chosen.isEmpty() )
	{
		if( ! rest.isEmpty() )
			rest += QLatin1Char( '\n' );
		rest += kBegin + QLatin1Char( '\n' );
		rest += chosen.join( QStringLiteral( "\n" ) );
		rest += QLatin1Char( '\n' ) + kEnd;
	}
	vm->Set_Additional_Args( rest );
	return true;
}
