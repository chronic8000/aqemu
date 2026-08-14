#ifndef INFERNO_COMPANION_SETUP_H
#define INFERNO_COMPANION_SETUP_H

#include <QString>

class QWidget;
class Virtual_Machine;

/** Wizard / restore dialog name for the companion Ubuntu VM. */
QString AQ_Inferno_Companion_OS_Name();

bool AQ_Is_Inferno_Companion_OS( const QString &os_name );

/** Canonical Ubuntu Server live ISO URL (amd64). */
QString AQ_Inferno_Companion_Ubuntu_ISO_URL();

QString AQ_Inferno_Companion_Ubuntu_ISO_FileName();

/**
 * Download Ubuntu Server ISO (if missing) and create companion.qcow2 under vm_folder.
 * If preferred_local_iso is set and exists, that file is used instead of downloading.
 * Returns false on cancel/failure; fills disk_out / iso_out on success.
 */
bool AQ_Inferno_Companion_Prepare_Assets( QWidget *parent,
                                         const QString &vm_folder,
                                         QString *disk_out,
                                         QString *iso_out,
                                         QString *error_out,
                                         const QString &preferred_local_iso = QString() );

/** Human-readable post-install checklist for the notepad dialog. */
QString AQ_Inferno_Companion_Post_Install_Notes( const QString &disk_path,
                                                const QString &iso_path,
                                                const QString &ssh_user_hint );

void AQ_Inferno_Companion_Show_Notes( QWidget *parent, const QString &notes );

/** Apply network / WSL / USB-remote bookkeeping on a new companion VM. */
void AQ_Apply_Inferno_Companion_VM_Defaults( Virtual_Machine *vm,
                                             const QString &disk_path,
                                             const QString &iso_path );

/** Remember disk path for Apple SoC Restore dialog. */
void AQ_Inferno_Companion_Remember_Disk( const QString &disk_path );

/**
 * Create a full AQEMU VM (Ubuntu Server ISO mounted on CD, companion.qcow2 as HDA),
 * write the .aqemu file, and return ownership to the caller (add to Main_Window list).
 * On failure returns nullptr and sets error_out.
 */
Virtual_Machine *AQ_Inferno_Companion_Create_VM( QWidget *parent,
                                                 const QString &preferred_name,
                                                 QString *error_out );

#endif
