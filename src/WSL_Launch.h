/****************************************************************************
** WSL/KVM launch helpers (Windows host → Linux QEMU inside WSL)
****************************************************************************/

#ifndef WSL_LAUNCH_H
#define WSL_LAUNCH_H

#include <QString>
#include <QStringList>

/** Fast path: returns last cached probe if still fresh (default ~60s). */
bool WSL_Is_Available( bool force_refresh = false );
/** True when /dev/kvm is readable and writable by the default WSL user. */
bool WSL_Has_KVM( const QString &distro = QString(), bool force_refresh = false );
/**
 * If /dev/kvm is not writable, try to fix it automatically via `wsl -u root`
 * (usermod -aG kvm + chmod 666 for this boot). Returns true when KVM is usable after.
 */
bool WSL_Ensure_KVM_Access( const QString &distro = QString() );
/** Clear cached WSL/KVM probe results (e.g. after Settings Probe). */
void WSL_Clear_Probe_Cache();

QString Windows_Path_To_WSL( const QString &windows_path );
/** Rewrite file=/firmware paths in a QEMU arg list for Linux inside WSL. */
QStringList Rewrite_Args_For_WSL( const QStringList &win_args );
/** Build wsl.exe argv: optional -d distro, --, linux_qemu, rewritten qemu args. */
QStringList Build_WSL_Launch_Args( const QString &distro,
                                   const QString &linux_qemu_binary,
                                   const QStringList &qemu_args );

/**
 * Pick a host -audiodev backend that the Linux QEMU inside WSL actually supports.
 * Probes `qemu-system-… -audiodev help` (cached). Prefer: preferred, then alsa, spice, oss, wav, none.
 */
QString WSL_Pick_Audio_Backend( const QString &distro = QString(),
                                const QString &linux_qemu_binary = QString(),
                                const QString &preferred = QString() );

#endif
