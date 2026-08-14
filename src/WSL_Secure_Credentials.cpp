#include "WSL_Secure_Credentials.h"

#include <QString>
#include <string>

#ifdef Q_OS_WIN32

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <wincred.h>

namespace {

const wchar_t kCredTarget[] = L"AQEMU/WSL";

void Secure_Wipe( std::wstring &s )
{
	if( s.empty() )
		return;
	SecureZeroMemory( &s[0], s.size() * sizeof( wchar_t ) );
	s.clear();
}

} // namespace

bool WSL_Secure_Password_Available()
{
	return true;
}

bool WSL_Has_Secure_Password()
{
	PCREDENTIALW cred = nullptr;
	const BOOL ok = CredReadW( kCredTarget, CRED_TYPE_GENERIC, 0, &cred );
	if( ! ok || ! cred )
		return false;
	CredFree( cred );
	return true;
}

bool WSL_Clear_Secure_Password()
{
	CredDeleteW( kCredTarget, CRED_TYPE_GENERIC, 0 );
	return true;
}

bool WSL_Save_Secure_Password( const QString &wsl_username, const QString &password )
{
	if( password.isEmpty() )
		return WSL_Clear_Secure_Password();

	std::wstring user = wsl_username.trimmed().toStdWString();
	if( user.empty() )
		user = L"wsl";
	std::wstring pass = password.toStdWString();

	CREDENTIALW cred;
	ZeroMemory( &cred, sizeof( cred ) );
	cred.Type = CRED_TYPE_GENERIC;
	cred.TargetName = const_cast<LPWSTR>( kCredTarget );
	cred.UserName = &user[0];
	cred.CredentialBlobSize = static_cast<DWORD>( pass.size() * sizeof( wchar_t ) );
	cred.CredentialBlob = reinterpret_cast<LPBYTE>( &pass[0] );
	cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
	cred.Comment = const_cast<LPWSTR>( L"AQEMU WSL sudo password (optional)" );

	const BOOL ok = CredWriteW( &cred, 0 );
	Secure_Wipe( pass );
	Secure_Wipe( user );
	return ok == TRUE;
}

QString WSL_Load_Secure_Password()
{
	PCREDENTIALW cred = nullptr;
	if( ! CredReadW( kCredTarget, CRED_TYPE_GENERIC, 0, &cred ) || ! cred )
		return QString();

	QString out;
	if( cred->CredentialBlob && cred->CredentialBlobSize > 0 )
	{
		const int nchars = static_cast<int>( cred->CredentialBlobSize / sizeof( wchar_t ) );
		out = QString::fromWCharArray( reinterpret_cast<const wchar_t *>( cred->CredentialBlob ), nchars );
	}
	CredFree( cred );
	return out;
}

#else

bool WSL_Secure_Password_Available() { return false; }
bool WSL_Has_Secure_Password() { return false; }
bool WSL_Save_Secure_Password( const QString &, const QString & ) { return false; }
QString WSL_Load_Secure_Password() { return QString(); }
bool WSL_Clear_Secure_Password() { return true; }

#endif
