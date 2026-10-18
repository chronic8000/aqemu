/****************************************************************************
** Local crash reports. Nothing is uploaded.
****************************************************************************/
#ifndef CRASH_REPORT_H
#define CRASH_REPORT_H

#include <QString>

class QWidget;

/** Install the crash handler. Safe to call again after Qt starts. */
void AQ_Install_Crash_Handler();

/** Deliberate crash for --test-crash. Does not return. */
void AQ_Test_Crash();

/** Folder that holds the reports. */
QString AQ_Crash_Directory();

/** Open that folder in the host file manager. */
void AQ_Crash_Open_Directory();

/**
 * If the last run wrote a crash report, return its path and clear the
 * pending marker. The report files stay on disk.
 */
bool AQ_Crash_Consume_Pending( QString *report_path );

/** Same table as Help → Copy setup, plus the crash folder. */
QString AQ_Crash_Setup_Markdown();

/** Ask the user what to do with a report left by the previous run. */
void AQ_Crash_Prompt_If_Pending( QWidget *parent );

#endif
