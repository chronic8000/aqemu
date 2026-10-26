/****************************************************************************
** Browse every device group from the QEMU probe for one qemu-system binary.
****************************************************************************/

#ifndef QEMU_HARDWARE_BROWSER_H
#define QEMU_HARDWARE_BROWSER_H

#include <QDialog>

class Virtual_Machine;

class QEMU_Hardware_Browser
{
	public:
		/** Edit vm additional args. Returns true when the VM changed. */
		static bool Edit( QWidget *parent, Virtual_Machine *vm, const QString &arch_hint );
};

#endif
