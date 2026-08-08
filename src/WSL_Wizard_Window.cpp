#include "WSL_Wizard_Window.h"
#include "WSL_Launch.h"
#include "Utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

WSL_Wizard_Window::WSL_Wizard_Window( QWidget *parent )
	: QDialog( parent )
{
	setWindowTitle( tr("WSL First-Run Setup") );
	setMinimumWidth( 400 );

	QVBoxLayout *mainLay = new QVBoxLayout( this );

	QLabel *intro = new QLabel( tr("AQEMU needs your WSL configuration to start VMs. Please configure your distro details below:") );
	intro->setWordWrap( true );
	mainLay->addWidget( intro );

	QFormLayout *form = new QFormLayout();

	CB_Distro = new QComboBox();
	CB_Distro->setEditable( true );
	CB_Distro->addItem( "" );
	CB_Distro->addItems( WSL_Get_Installed_Distros() );
	const QString savedDistro = Settings.value( "WSL_Launch/Distro", "" ).toString();
	CB_Distro->setCurrentText( savedDistro );
	form->addRow( tr("WSL Distro:"), CB_Distro );

	Edit_User = new QLineEdit();
	Edit_User->setText( Settings.value( "WSL_Launch/Username", "" ).toString() );
	form->addRow( tr("WSL Username:"), Edit_User );

	QLabel *hint = new QLabel( tr( "Admin tasks (KVM group) use wsl -u root — no password needed." ) );
	hint->setWordWrap( true );
	hint->setStyleSheet( QStringLiteral( "color: gray;" ) );
	form->addRow( QString(), hint );

	mainLay->addLayout( form );

	QHBoxLayout *btnLay = new QHBoxLayout();
	QPushButton *btnOk = new QPushButton( tr("OK") );
	QPushButton *btnCancel = new QPushButton( tr("Cancel") );
	btnLay->addStretch();
	btnLay->addWidget( btnOk );
	btnLay->addWidget( btnCancel );
	mainLay->addLayout( btnLay );

	connect( CB_Distro, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CB_Distro_currentIndexChanged(int)) );
	connect( btnOk, SIGNAL(clicked()), this, SLOT(on_Btn_Ok_clicked()) );
	connect( btnCancel, SIGNAL(clicked()), this, SLOT(on_Btn_Cancel_clicked()) );
}

QString WSL_Wizard_Window::Get_Distro() const
{
	return CB_Distro->currentText().trimmed();
}

QString WSL_Wizard_Window::Get_Username() const
{
	return Edit_User->text().trimmed();
}

void WSL_Wizard_Window::on_CB_Distro_currentIndexChanged( int index )
{
	Q_UNUSED( index );
	const QString distro = CB_Distro->currentText().trimmed();
	if( ! distro.isEmpty() && Edit_User->text().isEmpty() )
	{
		Edit_User->setText( WSL_Get_Distro_Default_User( distro ) );
	}
}

void WSL_Wizard_Window::on_Btn_Ok_clicked()
{
	if( Edit_User->text().trimmed().isEmpty() )
	{
		AQGraphic_Warning( tr("Warning"), tr("WSL Username is required!") );
		return;
	}
	if( ! WSL_Is_Valid_Username( Edit_User->text() ) )
	{
		AQGraphic_Warning( tr( "Warning" ),
			tr( "WSL username may only contain letters, digits, underscore, and hyphen." ) );
		return;
	}
	Settings.setValue( "WSL_Launch/Distro", Get_Distro() );
	Settings.setValue( "WSL_Launch/Username", WSL_Sanitize_Username( Get_Username() ) );
	// Never persist WSL passwords; privileged ops use wsl -u root.
	Settings.remove( "WSL_Launch/Password" );
	WSL_Clear_Probe_Cache();
	accept();
}

void WSL_Wizard_Window::on_Btn_Cancel_clicked()
{
	reject();
}
