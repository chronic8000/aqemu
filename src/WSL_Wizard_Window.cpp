#include "WSL_Wizard_Window.h"
#include "WSL_Launch.h"
#include "WSL_Secure_Credentials.h"
#include "Utils.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>

WSL_Wizard_Window::WSL_Wizard_Window( QWidget *parent )
	: QDialog( parent )
{
	setWindowTitle( tr("WSL First-Run Setup") );
	setMinimumWidth( 420 );

	QVBoxLayout *mainLay = new QVBoxLayout( this );

	QLabel *intro = new QLabel( tr(
		"AQEMU needs your WSL configuration to start VMs. "
		"Username is saved in settings; password is optional and can be stored "
		"securely in Windows Credential Manager." ) );
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
	if( WSL_Has_Secure_Password() )
		Edit_Password->setPlaceholderText( tr( "Saved securely — enter a new password to replace" ) );
	else
		Edit_Password->setPlaceholderText( tr( "Optional — only if sudo is needed" ) );
	form->addRow( tr("WSL Password:"), Edit_Password );

	CH_Remember_Password = new QCheckBox( tr( "Remember password securely (Windows Credential Manager)" ) );
	CH_Remember_Password->setChecked(
		Settings.value( "WSL_Launch/Remember_Password", false ).toBool() && WSL_Has_Secure_Password() );
	CH_Remember_Password->setEnabled( WSL_Secure_Password_Available() );
	form->addRow( QString(), CH_Remember_Password );

	QLabel *hint = new QLabel( tr(
		"Preferred: KVM fixes use wsl -u root (no password). "
		"Saved password is only used as a sudo fallback and is never written to AQEMU.ini." ) );
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

	const QString user = WSL_Sanitize_Username( Get_Username() );
	Settings.setValue( "WSL_Launch/Distro", Get_Distro() );
	Settings.setValue( "WSL_Launch/Username", user );
	Settings.remove( "WSL_Launch/Password" );

	if( CH_Remember_Password->isChecked() && WSL_Secure_Password_Available() )
	{
		const QString pass = Edit_Password->text();
		if( ! pass.isEmpty() )
		{
			if( ! WSL_Save_Secure_Password( user, pass ) )
			{
				AQGraphic_Warning( tr( "WSL" ),
					tr( "Could not save the password to Windows Credential Manager." ) );
				return;
			}
		}
		else if( ! WSL_Has_Secure_Password() )
		{
			AQGraphic_Warning( tr( "WSL" ),
				tr( "Enter a password to remember, or uncheck Remember password." ) );
			return;
		}
		Settings.setValue( "WSL_Launch/Remember_Password", true );
	}
	else
	{
		WSL_Clear_Secure_Password();
		Settings.setValue( "WSL_Launch/Remember_Password", false );
	}

	WSL_Clear_Probe_Cache();
	accept();
}

void WSL_Wizard_Window::on_Btn_Cancel_clicked()
{
	reject();
}
