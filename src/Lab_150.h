/****************************************************************************
** AQEMU 1.5.1 lab tools (issues 69 and 70).
****************************************************************************/
#ifndef LAB_150_H
#define LAB_150_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

class QWidget;
class QProcess;
class Virtual_Machine;

QMap<QString, QString> AQ_Lab_Map( const QString &blob );
QString AQ_Lab_Blob( const QMap<QString, QString> &opt );
QString AQ_Lab_Get( const Virtual_Machine *vm, const QString &key );
void AQ_Lab_Set( Virtual_Machine *vm, const QString &key, const QString &value );

void AQ_Lab_Append_Args( QStringList &args, Virtual_Machine *vm );
void AQ_Lab_Adjust_Launch( Virtual_Machine *vm, QString &bin_path, QStringList *args );
bool AQ_Lab_Prepare_Start( Virtual_Machine *vm );
void AQ_Lab_After_Start( Virtual_Machine *vm, QProcess *proc );
void AQ_Lab_Stop_Companions( Virtual_Machine *vm );
void AQ_Lab_Apply_Guest_Profile( Virtual_Machine *vm, const QString &os_name );
QString AQ_Lab_Effective_Accel( const Virtual_Machine *vm );

void AQ_Lab_Open_VM_Window( Virtual_Machine *vm, const QList<Virtual_Machine*> &others, QWidget *parent );
void AQ_Lab_Support_Bundle( QWidget *parent, Virtual_Machine *vm );
void AQ_Lab_Convert( QWidget *parent, Virtual_Machine *vm );
void AQ_Lab_Chain_Studio( QWidget *parent, Virtual_Machine *vm );
void AQ_Lab_Catalog( QWidget *parent, Virtual_Machine *vm );
void AQ_Lab_Health( QWidget *parent, Virtual_Machine *vm );
void AQ_Lab_Auditor( QWidget *parent );
void AQ_Lab_Snippets( Virtual_Machine *vm, QWidget *parent );
void AQ_Lab_NBD( QWidget *parent, Virtual_Machine *vm );
void AQ_Lab_Pack( QWidget *parent, Virtual_Machine *vm, bool do_import );
void AQ_Lab_Quickemu( QWidget *parent, Virtual_Machine *vm );
void AQ_Lab_Snapshot_Timeline( Virtual_Machine *vm, QWidget *parent );
void AQ_Lab_Command_Diff( Virtual_Machine *vm, QWidget *parent );
void AQ_Lab_Block_Jobs( Virtual_Machine *vm, QWidget *parent );
void AQ_Lab_Migrate( Virtual_Machine *vm, QWidget *parent );
void AQ_Lab_WSL( QWidget *parent );
void AQ_Lab_Firmware( Virtual_Machine *vm, QWidget *parent );
void AQ_Lab_First_Run( QWidget *parent );
QList<Virtual_Machine*> AQ_Lab_Linked_Clones( Virtual_Machine *base, QWidget *parent );
void AQ_Lab_Start_Group( const QList<Virtual_Machine*> &vms, QWidget *parent, bool stop );

#endif
