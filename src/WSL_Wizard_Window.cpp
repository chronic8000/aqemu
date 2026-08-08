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

	Edit_Password = new QLineEdit();
	Edit_Password->setEchoMode( QLineEdit::Password );
	Edit_Password->setPlaceholderText( tr( "Optional — prefer wsl -u root for admin tasks" ) );
	form->addRow( tr("WSL Password (optional):"), Edit_Password );

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

QString WSL_Wizard_Window::Get_Password() const
{
	return Edit_Password->text();
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
	Settings.setValue( "WSL_Launch/Distro", Get_Distro() );
	Settings.setValue( "WSL_Launch/Username", Get_Username() );
	// Do not persist the WSL password — optional sudo prompts can use root via wsl -u root.
	Settings.remove( "WSL_Launch/Password" );
	accept();
}

void WSL_Wizard_Window::on_Btn_Cancel_clicked()
{
	reject();
}
