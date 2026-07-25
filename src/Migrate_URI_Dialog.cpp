/****************************************************************************
** Build a QEMU migrate URI with host/port helpers.
****************************************************************************/

#include "Migrate_URI_Dialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>

Migrate_URI_Dialog::Migrate_URI_Dialog( QWidget *parent )
	: QDialog( parent )
{
	setWindowTitle( tr( "Live migrate to host" ) );
	resize( 480, 220 );

	QVBoxLayout *lay = new QVBoxLayout( this );
	lay->addWidget( new QLabel( tr(
		"Destination QEMU must already be waiting with -incoming tcp:0:PORT "
		"(matching machine, firmware, and disk layout)." ) ) );

	QFormLayout *form = new QFormLayout();
	Edit_Host = new QLineEdit( QStringLiteral( "127.0.0.1" ) );
	Edit_Port = new QLineEdit( QStringLiteral( "4444" ) );
	Edit_Raw = new QLineEdit();
	Edit_Raw->setPlaceholderText( tr( "Or paste full URI: tcp:host:4444" ) );
	CH_Blk = new QCheckBox( tr( "Also migrate non-shared storage (blk=true — slower)" ) );
	form->addRow( tr( "Destination host" ), Edit_Host );
	form->addRow( tr( "Port" ), Edit_Port );
	form->addRow( tr( "Raw URI (optional)" ), Edit_Raw );
	lay->addLayout( form );
	lay->addWidget( CH_Blk );

	QDialogButtonBox *box = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
	lay->addWidget( box );
	connect( box, &QDialogButtonBox::accepted, this, &QDialog::accept );
	connect( box, &QDialogButtonBox::rejected, this, &QDialog::reject );
}

QString Migrate_URI_Dialog::URI() const
{
	const QString raw = Edit_Raw->text().trimmed();
	if( ! raw.isEmpty() )
		return raw;
	return QStringLiteral( "tcp:%1:%2" )
		.arg( Edit_Host->text().trimmed().isEmpty()
			? QStringLiteral( "127.0.0.1" ) : Edit_Host->text().trimmed() )
		.arg( Edit_Port->text().trimmed().isEmpty()
			? QStringLiteral( "4444" ) : Edit_Port->text().trimmed() );
}

bool Migrate_URI_Dialog::Copy_Storage() const
{
	return CH_Blk && CH_Blk->isChecked();
}
