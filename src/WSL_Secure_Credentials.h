#ifndef WSL_SECURE_CREDENTIALS_H
#define WSL_SECURE_CREDENTIALS_H

#include <QString>

/**
 * Secure WSL password helpers (Windows Credential Manager).
 * Username/distro stay in QSettings; the password is never written to AQEMU.ini.
 */
bool WSL_Secure_Password_Available();
bool WSL_Has_Secure_Password();
bool WSL_Save_Secure_Password( const QString &wsl_username, const QString &password );
QString WSL_Load_Secure_Password();
bool WSL_Clear_Secure_Password();

#endif
