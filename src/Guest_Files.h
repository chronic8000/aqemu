/****************************************************************************
** Copy files into a running guest as a FAT disk. Works on Linux and Windows,
** x86_64 and ARM: the guest only needs a floppy or USB controller.
****************************************************************************/
#ifndef GUEST_FILES_H
#define GUEST_FILES_H

#include <QString>
#include <QStringList>

class QMimeData;

/** File paths from a drop or from the host clipboard (URLs and file:// text). */
QStringList AQ_File_Paths_From_Mime( const QMimeData *mime );

/**
 * Write a FAT disk the guest can open.
 * Small sets become a 1.44 MB floppy. Larger sets become a FAT16 hard-disk image.
 * Returns false and sets error when the files are missing or too large (512 MB).
 */
bool AQ_Write_Guest_Fat_Image( const QString &image_path, const QStringList &source_files,
                               bool *floppy_out, QString *error );

/** Read one file back out of an image written by AQ_Write_Guest_Fat_Image. For tests. */
QByteArray AQ_Read_Guest_Fat_File( const QString &image_path, const QString &file_name );

#endif
