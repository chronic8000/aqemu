# -*- coding: utf-8 -*-
from pathlib import Path

h = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\HDD_Image_Info.h")
ht = h.read_text(encoding="utf-8")
if "QHash" not in ht:
    ht = ht.replace("#include <QProcess>", "#include <QProcess>\n#include <QHash>\n#include <QString>")
    ht = ht.replace(
        """\tprivate:
\t\tVM::Disk_Info Info;
\t\tQProcess* QEMU_IMG_Proc;
};""",
        """\tprivate:
\t\tVM::Disk_Info Info;
\t\tQProcess* QEMU_IMG_Proc;
\t\tstatic QHash<QString, VM::Disk_Info> Info_Cache;
};""",
    )
    h.write_text(ht, encoding="utf-8")
    print("header OK")
else:
    print("header already patched")

c = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\HDD_Image_Info.cpp")
ct = c.read_text(encoding="utf-8")

if "Info_Cache" not in ct:
    ct = ct.replace(
        '#include "HDD_Image_Info.h"\n',
        '#include "HDD_Image_Info.h"\n\nQHash<QString, VM::Disk_Info> HDD_Image_Info::Info_Cache;\n',
    )

old = """void HDD_Image_Info::Update_Disk_Info( const QString &path )
{
\tInfo.Image_File_Name = path;
\t
\tif( Info.Image_File_Name.isEmpty() )
\t{
\t\tClear_Info();
\t\treturn;
\t}
\t
\tif( QFile::exists(Info.Image_File_Name) == false )
\t{
\t\tAQWarning( "void HDD_Image_Info::Update_Disk_Info( const QString &path )",
                   "Image \\"" + Info.Image_File_Name + "\\" does not exist!" );
\t\tClear_Info();
\t\treturn;
\t}
\telse
\t{
\t\tQStringList args;
\t\targs << "info" << Info.Image_File_Name;
\t\t
\t\tQEMU_IMG_Proc = new QProcess( this );
\t\tQEMU_IMG_Proc->start( Get_QEMU_IMG_Path(), args );
\t\t
\t\tconnect( QEMU_IMG_Proc, SIGNAL(finished(int, QProcess::ExitStatus)),
\t\t\t\t this, SLOT(Parse_Info(int, QProcess::ExitStatus)), Qt::DirectConnection );
\t\t
\t\tconnect( QEMU_IMG_Proc, SIGNAL(error(QProcess::ProcessError)),
\t\t\t\t this, SLOT(Clear_Info()), Qt::DirectConnection );
\t}
}"""

# Use exact file content
old = '''void HDD_Image_Info::Update_Disk_Info( const QString &path )
{
\tInfo.Image_File_Name = path;
\t
\tif( Info.Image_File_Name.isEmpty() )
\t{
\t\tClear_Info();
\t\treturn;
\t}
\t
\tif( QFile::exists(Info.Image_File_Name) == false )
\t{
\t\tAQWarning( "void HDD_Image_Info::Update_Disk_Info( const QString &path )",
                   "Image \\"" + Info.Image_File_Name + "\\" does not exist!" );
\t\tClear_Info();
\t\treturn;
\t}
\telse
\t{
\t\tQStringList args;
\t\targs << "info" << Info.Image_File_Name;
\t\t
\t\tQEMU_IMG_Proc = new QProcess( this );
\t\tQEMU_IMG_Proc->start( Get_QEMU_IMG_Path(), args );
\t\t
\t\tconnect( QEMU_IMG_Proc, SIGNAL(finished(int, QProcess::ExitStatus)),
\t\t\t\t this, SLOT(Parse_Info(int, QProcess::ExitStatus)), Qt::DirectConnection );
\t\t
\t\tconnect( QEMU_IMG_Proc, SIGNAL(error(QProcess::ProcessError)),
\t\t\t\t this, SLOT(Clear_Info()), Qt::DirectConnection );
\t}
}'''

# Read exact bytes from file for the function
start = ct.find("void HDD_Image_Info::Update_Disk_Info")
end = ct.find("void HDD_Image_Info::Clear_Info")
if start < 0 or end < 0:
    raise SystemExit("Update_Disk_Info not found")
print("FOUND function, len", end-start)
print(repr(ct[start:start+200]))

new_fn = '''void HDD_Image_Info::Update_Disk_Info( const QString &path )
{
\tInfo.Image_File_Name = path;

\tif( Info.Image_File_Name.isEmpty() )
\t{
\t\tClear_Info();
\t\treturn;
\t}

\t// Reuse cached qemu-img results when switching between VMs that share disks.
\tif( Info_Cache.contains( Info.Image_File_Name ) )
\t{
\t\tInfo = Info_Cache.value( Info.Image_File_Name );
\t\temit Completed( true );
\t\treturn;
\t}

\tif( QFile::exists(Info.Image_File_Name) == false )
\t{
\t\tAQWarning( "void HDD_Image_Info::Update_Disk_Info( const QString &path )",
\t\t\t\t   "Image \\"" + Info.Image_File_Name + "\\" does not exist!" );
\t\tClear_Info();
\t\treturn;
\t}

\tif( QEMU_IMG_Proc->state() != QProcess::NotRunning )
\t{
\t\tQEMU_IMG_Proc->disconnect( this );
\t\tQEMU_IMG_Proc->kill();
\t\tQEMU_IMG_Proc->waitForFinished( 200 );
\t}

\tQStringList args;
\targs << "info" << Info.Image_File_Name;

\tconnect( QEMU_IMG_Proc, SIGNAL(finished(int, QProcess::ExitStatus)),
\t\t\t this, SLOT(Parse_Info(int, QProcess::ExitStatus)), Qt::UniqueConnection );

\tconnect( QEMU_IMG_Proc, SIGNAL(error(QProcess::ProcessError)),
\t\t\t this, SLOT(Clear_Info()), Qt::UniqueConnection );

\tQEMU_IMG_Proc->start( Get_QEMU_IMG_Path(), args );
}

'''

ct = ct[:start] + new_fn + ct[end:]

# Cache on successful parse
old_emit = """\tInfo.Cluster_Size = cluster_size.isEmpty() ? 0 : cluster_size.toInt();

\temit Completed( true );
}"""
new_emit = """\tInfo.Cluster_Size = cluster_size.isEmpty() ? 0 : cluster_size.toInt();

\tInfo_Cache.insert( Info.Image_File_Name, Info );
\temit Completed( true );
}"""
if old_emit not in ct:
    raise SystemExit("parse emit not found")
ct = ct.replace(old_emit, new_emit, 1)

c.write_text(ct, encoding="utf-8")
print("HDD_Image_Info.cpp OK")
