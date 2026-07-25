/****************************************************************************
** Install-media OS guess: filename + ISO9660 volume ID (+ optional libosinfo).
****************************************************************************/
#ifndef ISO_GUESS_H
#define ISO_GUESS_H

#include <QString>

struct ISO_Guess_Result
{
	QString os_name;       // wizard_trees leaf, if known
	QString confidence;    // high / medium / low / none
	QString tip;
	QString volume_id;     // ISO9660 PVD volume identifier when readable
};

/** Guess guest OS from ISO/image path (filename + ISO9660 + optional libosinfo). */
ISO_Guess_Result AQ_Guess_OS_From_Media( const QString &path );

/** Read ISO 9660 Primary Volume Descriptor volume identifier (empty if not ISO). */
QString AQ_Read_ISO9660_Volume_Id( const QString &path );

#endif
