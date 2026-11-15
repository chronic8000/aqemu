/****************************************************************************
** Pre-start display and port checks (issue #107). Behaviour inspired by
** TrueNAS pylibvirt; clean-room, no pylibvirt code.
****************************************************************************/
#ifndef AQEMU_START_VALIDATION_H
#define AQEMU_START_VALIDATION_H

#include <QList>
#include <QString>
#include <QStringList>

class Virtual_Machine;

struct AQ_Start_Issue
{
	enum Kind { Error, Warning };
	Kind kind;
	QString text;
};

QList<AQ_Start_Issue> AQ_Collect_Start_Issues( const Virtual_Machine *vm );

/** Show every collected problem in one dialog. Returns false if start must abort. */
bool AQ_Run_Start_Validation( Virtual_Machine *vm );

#endif
