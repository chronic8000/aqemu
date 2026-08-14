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

/**
 * Run a privileged shell script inside WSL.
 * Prefers `wsl -u root`. If that cannot fix KVM and a secure password is stored,
 * falls back to `sudo -S` with the password on stdin (never on argv).
 * @param timeout_ms  Wait budget (apt installs need several minutes).
 */
bool WSL_Run_Privileged_Script( const QString &distro, const QString &script,
                                int timeout_ms = 15000 );

/**
 * Reims on Windows/WSL: ensure a non-llvmpipe Vulkan ICD (typically Mesa Dozen/dzn
 * over WSLg D3D12). Called when provisioning or starting Reims VMs for every user.
 * May install packages via wsl -u root (or remembered sudo). Returns true when an
 * accelerated Vulkan device is available afterward.
 */
bool WSL_Ensure_Reims_Vulkan_Stack( const QString &distro = QString(),
                                    QString *status_message = nullptr,
                                    bool force_reinstall = false );

/** True if Mesa Dozen (libvulkan_dzn) is present in the WSL distro. */
bool WSL_Has_Dozen_ICD( const QString &distro = QString() );

/** Letters, digits, underscore, hyphen only — empty if invalid. */
QString WSL_Sanitize_Username( const QString &raw );
bool WSL_Is_Valid_Username( const QString &raw );

/**
 * Probe WSL Vulkan for a real (non-CPU/llvmpipe) GPU name.
 * Empty string = only software Vulkan or probe failed.
 * Vendor-agnostic: NVIDIA, AMD, Intel Arc, etc. — Reims only needs Vulkan.
 */
QString WSL_Probe_Accelerated_Vulkan_GPU( const QString &distro = QString() );

/** All accelerated WSL Vulkan device names (empty if only llvmpipe). */
QStringList WSL_List_Accelerated_Vulkan_GPUs( const QString &distro = QString() );

/**
 * Prefer a specific WSL Vulkan device for Reims (substring match on deviceName).
 * Empty = auto (first accelerated device). Applied via MESA_VK_DEVICE_SELECT when IDs known.
 */
void WSL_Set_Preferred_Vulkan_Device( const QString &device_name_or_empty );
QString WSL_Preferred_Vulkan_Device();
QStringList WSL_Vulkan_Launch_Env( const QString &distro = QString() );

/** Retrieve a list of all installed WSL distributions. */
QStringList WSL_Get_Installed_Distros();

/** Get the default non-root username for a given WSL distro. */
QString WSL_Get_Distro_Default_User( const QString &distro );

QString Windows_Path_To_WSL( const QString &windows_path );
/** Rewrite file=/firmware paths in a QEMU arg list for Linux inside WSL. */
QStringList Rewrite_Args_For_WSL( const QStringList &win_args );
/** Build wsl.exe argv: optional -d distro, optional -u user, -e, linux_qemu, rewritten qemu args. */
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
