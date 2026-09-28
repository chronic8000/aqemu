/****************************************************************************
**
** Copyright (C) 2026 Chronic Engineering
**
** This file is part of AQEMU.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA  02110-1301, USA.
**
****************************************************************************/

#include "SAN_Storage_Dialog.h"
#include "ui_SAN_Storage_Dialog.h"
#include "AQ_UI_Style.h"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTimer>

SAN_Storage_Dialog::SAN_Storage_Dialog( QWidget *parent )
    : QDialog( parent ),
      ui( new Ui::SAN_Storage_Dialog )
{
    ui->setupUi( this );
    AQ_Intelligently_Size_Dialog( this, 660, 580 );

    // Configure inventory tree headers
    ui->tree_Host_SAN->header()->resizeSection( 0, 160 );
    ui->tree_Host_SAN->header()->resizeSection( 1, 100 );
    ui->tree_Host_SAN->header()->resizeSection( 2, 80 );
    ui->tree_Host_SAN->header()->setStretchLastSection( true );

    // Default sample IQNs in dropdown
    ui->combo_iSCSI_IQN->addItem( QStringLiteral( "iqn.2005-10.org.truenas.ctl:iscsi-disk0" ) );
    ui->combo_iSCSI_IQN->addItem( QStringLiteral( "iqn.2001-04.com.synology:storage0" ) );
    ui->combo_iSCSI_IQN->addItem( QStringLiteral( "iqn.2003-01.org.linux-iscsi.target:sn.1" ) );

    // Connect iSCSI field changes
    connect( ui->edit_iSCSI_Host, &QLineEdit::textChanged, this, &SAN_Storage_Dialog::on_iSCSI_Changed );
    connect( ui->spin_iSCSI_Port, QOverload<int>::of(&QSpinBox::valueChanged), this, &SAN_Storage_Dialog::on_iSCSI_Changed );
    connect( ui->combo_iSCSI_IQN, &QComboBox::editTextChanged, this, &SAN_Storage_Dialog::on_iSCSI_Changed );
    connect( ui->spin_iSCSI_LUN, QOverload<int>::of(&QSpinBox::valueChanged), this, &SAN_Storage_Dialog::on_iSCSI_Changed );
    connect( ui->edit_CHAP_User, &QLineEdit::textChanged, this, &SAN_Storage_Dialog::on_iSCSI_Changed );
    connect( ui->edit_CHAP_Pass, &QLineEdit::textChanged, this, &SAN_Storage_Dialog::on_iSCSI_Changed );

    // Connect NVMe field changes
    connect( ui->combo_NVMe_Transport, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SAN_Storage_Dialog::on_NVMe_Changed );
    connect( ui->edit_NVMe_Address, &QLineEdit::textChanged, this, &SAN_Storage_Dialog::on_NVMe_Changed );
    connect( ui->spin_NVMe_Port, QOverload<int>::of(&QSpinBox::valueChanged), this, &SAN_Storage_Dialog::on_NVMe_Changed );
    connect( ui->combo_NVMe_NQN, &QComboBox::editTextChanged, this, &SAN_Storage_Dialog::on_NVMe_Changed );

    connect( ui->btn_Attach, &QPushButton::clicked, this, &SAN_Storage_Dialog::on_btn_Attach_clicked );

    Update_iSCSI_URI_Preview();
    Refresh_Host_NVMe_List();
    Scan_Host_SAN_Devices();
}

SAN_Storage_Dialog::~SAN_Storage_Dialog()
{
    delete ui;
}

VM_Native_Storage_Device SAN_Storage_Dialog::Get_Configured_Device() const
{
    return Configured_Device;
}

QString SAN_Storage_Dialog::Get_Device_Path() const
{
    return Configured_Device.Get_File_Path();
}

VM::Device_Interface SAN_Storage_Dialog::Get_Device_Interface() const
{
    return Configured_Device.Get_Interface();
}

int SAN_Storage_Dialog::Get_Logical_Sector_Size() const
{
    return Configured_Device.Get_Logical_Block_Size();
}

void SAN_Storage_Dialog::on_tabWidget_currentChanged( int index )
{
    if( index == 2 )
    {
        Scan_Host_SAN_Devices();
    }
}

void SAN_Storage_Dialog::on_iSCSI_Changed()
{
    Update_iSCSI_URI_Preview();
}

void SAN_Storage_Dialog::on_NVMe_Changed()
{
    // Auto-update log hint
    const QString addr = ui->edit_NVMe_Address->text().trimmed();
    const QString nqn = ui->combo_NVMe_NQN->currentText().trimmed();
    if( ! addr.isEmpty() && ! nqn.isEmpty() )
    {
        ui->text_NVMe_Log->setText( tr( "Target configured: %1:%2 (%3)\nClick 'Connect Fabric' to attach to Linux host kernel." )
                                        .arg( addr ).arg( ui->spin_NVMe_Port->value() ).arg( nqn ) );
    }
}

void SAN_Storage_Dialog::on_check_iSCSI_CHAP_toggled( bool checked )
{
    ui->edit_CHAP_User->setEnabled( checked );
    ui->edit_CHAP_Pass->setEnabled( checked );
    Update_iSCSI_URI_Preview();
}

void SAN_Storage_Dialog::Update_iSCSI_URI_Preview()
{
    QString host = ui->edit_iSCSI_Host->text().trimmed();
    if( host.isEmpty() )
        host = QStringLiteral( "<portal-host>" );

    const int port = ui->spin_iSCSI_Port->value();
    QString iqn = ui->combo_iSCSI_IQN->currentText().trimmed();
    if( iqn.isEmpty() )
        iqn = QStringLiteral( "<target-iqn>" );

    const int lun = ui->spin_iSCSI_LUN->value();

    QString uri = QStringLiteral( "iscsi://" );
    if( ui->check_iSCSI_CHAP->isChecked() && ! ui->edit_CHAP_User->text().isEmpty() )
    {
        uri += ui->edit_CHAP_User->text();
        if( ! ui->edit_CHAP_Pass->text().isEmpty() )
            uri += QStringLiteral( "%" ) + ui->edit_CHAP_Pass->text();
        uri += QStringLiteral( "@" );
    }

    uri += host;
    if( port != 3260 )
        uri += QStringLiteral( ":%1" ).arg( port );

    uri += QStringLiteral( "/%1/%2" ).arg( iqn ).arg( lun );
    ui->edit_iSCSI_Preview->setText( uri );
}

bool SAN_Storage_Dialog::Test_TCP_Port( const QString &host, quint16 port, QString &out_msg )
{
    if( host.isEmpty() )
    {
        out_msg = tr( "Please specify a valid hostname or IP address." );
        return false;
    }

    QTcpSocket socket;
    socket.connectToHost( host, port );
    if( socket.waitForConnected( 2500 ) )
    {
        socket.disconnectFromHost();
        out_msg = tr( "Success: Connection to %1:%2 succeeded!" ).arg( host ).arg( port );
        return true;
    }
    else
    {
        out_msg = tr( "Connection failed: %1" ).arg( socket.errorString() );
        return false;
    }
}

void SAN_Storage_Dialog::on_btn_Test_iSCSI_clicked()
{
    QString msg;
    const QString host = ui->edit_iSCSI_Host->text().trimmed();
    const quint16 port = static_cast<quint16>( ui->spin_iSCSI_Port->value() );

    const bool ok = Test_TCP_Port( host, port, msg );
    if( ok )
    {
        ui->label_iSCSI_Status->setText( QStringLiteral( "<font color='#10b981'><b>✓ %1</b></font>" ).arg( msg ) );
    }
    else
    {
        ui->label_iSCSI_Status->setText( QStringLiteral( "<font color='#ef4444'><b>✗ %1</b></font>" ).arg( msg ) );
    }
}

void SAN_Storage_Dialog::on_btn_Copy_iSCSI_URI_clicked()
{
    QClipboard *cb = QApplication::clipboard();
    if( cb )
    {
        cb->setText( ui->edit_iSCSI_Preview->text() );
        ui->btn_Copy_iSCSI_URI->setText( tr( "Copied!" ) );
        QTimer::singleShot( 2000, this, [this]() {
            if( ui && ui->btn_Copy_iSCSI_URI )
                ui->btn_Copy_iSCSI_URI->setText( tr( "Copy URI" ) );
        } );
    }
}

void SAN_Storage_Dialog::on_btn_Discover_iSCSI_clicked()
{
    const QString host = ui->edit_iSCSI_Host->text().trimmed();
    if( host.isEmpty() )
    {
        ui->label_iSCSI_Status->setText( tr( "<font color='orange'>Enter Target Host / IP before discovering.</font>" ) );
        return;
    }

    const int port = ui->spin_iSCSI_Port->value();
    ui->label_iSCSI_Status->setText( tr( "Querying iSCSI portal %1:%2 using SendTargets..." ).arg( host ).arg( port ) );
    qApp->processEvents();

    QProcess proc;
    QStringList args;
    args << QStringLiteral( "-m" ) << QStringLiteral( "discovery" )
         << QStringLiteral( "-t" ) << QStringLiteral( "sendtargets" )
         << QStringLiteral( "-p" ) << QStringLiteral( "%1:%2" ).arg( host ).arg( port );

    proc.start( QStringLiteral( "iscsiadm" ), args );
    if( ! proc.waitForStarted( 1500 ) )
    {
        // iscsiadm binary not found on host
        QString msg;
        const bool tcp_ok = Test_TCP_Port( host, static_cast<quint16>( port ), msg );
        if( tcp_ok )
        {
            ui->label_iSCSI_Status->setText( tr( "<font color='#10b981'><b>✓ Portal reachable.</b></font> "
                                                "('iscsiadm' not installed on host for discovery; enter target IQN from TrueNAS directly)." ) );
        }
        else
        {
            ui->label_iSCSI_Status->setText( tr( "<font color='orange'>'iscsiadm' not installed on host, and TCP connection failed: %1</font>" ).arg( msg ) );
        }
        return;
    }

    proc.waitForFinished( 4000 );
    const QString out = QString::fromUtf8( proc.readAllStandardOutput() );
    const QString err = QString::fromUtf8( proc.readAllStandardError() );

    QStringList discovered_iqns;
    const QStringList lines = out.split( QLatin1Char( '\n' ), Qt::SkipEmptyParts );
    for( const QString &line : lines )
    {
        // Typical format: 192.168.1.100:3260,1 iqn.2005-10.org.truenas.ctl:disk0
        const int iqn_idx = line.indexOf( QStringLiteral( "iqn." ) );
        if( iqn_idx >= 0 )
        {
            QString iqn = line.mid( iqn_idx ).trimmed();
            if( ! iqn.isEmpty() && ! discovered_iqns.contains( iqn ) )
                discovered_iqns << iqn;
        }
    }

    if( ! discovered_iqns.isEmpty() )
    {
        ui->combo_iSCSI_IQN->clear();
        for( const QString &iqn : discovered_iqns )
            ui->combo_iSCSI_IQN->addItem( iqn );
        ui->label_iSCSI_Status->setText( tr( "<font color='#10b981'><b>✓ Discovered %1 target(s) successfully!</b></font>" )
                                             .arg( discovered_iqns.count() ) );
    }
    else
    {
        ui->label_iSCSI_Status->setText( tr( "No targets returned. %1" ).arg( err.isEmpty() ? out : err ) );
    }
}

void SAN_Storage_Dialog::on_btn_Test_NVMe_clicked()
{
    QString msg;
    const QString host = ui->edit_NVMe_Address->text().trimmed();
    const quint16 port = static_cast<quint16>( ui->spin_NVMe_Port->value() );

    const bool ok = Test_TCP_Port( host, port, msg );
    ui->text_NVMe_Log->append( ok ? tr( "[TCP Ping] SUCCESS: Port %1 is open on %2." ).arg( port ).arg( host )
                                  : tr( "[TCP Ping] ERROR: %1" ).arg( msg ) );
}

void SAN_Storage_Dialog::on_btn_Discover_NVMe_clicked()
{
    const QString transport = ui->combo_NVMe_Transport->currentText().section( QLatin1Char( ' ' ), 0, 0 ).trimmed();
    const QString addr = ui->edit_NVMe_Address->text().trimmed();
    const QString port = QString::number( ui->spin_NVMe_Port->value() );

    if( addr.isEmpty() )
    {
        ui->text_NVMe_Log->append( tr( "Please enter target IP or address." ) );
        return;
    }

    ui->text_NVMe_Log->append( tr( "Running nvme discover -t %1 -a %2 -s %3..." ).arg( transport, addr, port ) );

    QProcess proc;
    QStringList args;
    args << QStringLiteral( "discover" )
         << QStringLiteral( "-t" ) << transport
         << QStringLiteral( "-a" ) << addr
         << QStringLiteral( "-s" ) << port;

    proc.start( QStringLiteral( "nvme" ), args );
    if( ! proc.waitForStarted( 1500 ) )
    {
        ui->text_NVMe_Log->append( tr( "Note: 'nvme' (nvme-cli) command not found in host PATH. Install nvme-cli to enable discovery, or enter Subsystem NQN manually." ) );
        return;
    }

    proc.waitForFinished( 4000 );
    const QString out = QString::fromUtf8( proc.readAllStandardOutput() );
    const QString err = QString::fromUtf8( proc.readAllStandardError() );
    ui->text_NVMe_Log->append( out.isEmpty() ? err : out );

    // Parse subnqn: entries
    QRegularExpression rx( QStringLiteral( "subnqn:\\s*(\\S+)" ) );
    QRegularExpressionMatchIterator iter = rx.globalMatch( out );
    bool found = false;
    while( iter.hasNext() )
    {
        QRegularExpressionMatch m = iter.next();
        QString nqn = m.captured( 1 );
        if( ! nqn.isEmpty() )
        {
            if( ! found )
            {
                ui->combo_NVMe_NQN->clear();
                found = true;
            }
            ui->combo_NVMe_NQN->addItem( nqn );
        }
    }
}

void SAN_Storage_Dialog::on_btn_Connect_NVMe_clicked()
{
    const QString transport = ui->combo_NVMe_Transport->currentText().section( QLatin1Char( ' ' ), 0, 0 ).trimmed();
    const QString addr = ui->edit_NVMe_Address->text().trimmed();
    const QString port = QString::number( ui->spin_NVMe_Port->value() );
    const QString nqn = ui->combo_NVMe_NQN->currentText().trimmed();

    if( addr.isEmpty() || nqn.isEmpty() )
    {
        ui->text_NVMe_Log->append( tr( "Error: Target address and Subsystem NQN are required to connect." ) );
        return;
    }

    ui->text_NVMe_Log->append( tr( "Executing: nvme connect -t %1 -a %2 -s %3 -n %4" ).arg( transport, addr, port, nqn ) );

    QProcess proc;
    QStringList args;
    args << QStringLiteral( "connect" )
         << QStringLiteral( "-t" ) << transport
         << QStringLiteral( "-a" ) << addr
         << QStringLiteral( "-s" ) << port
         << QStringLiteral( "-n" ) << nqn;

    proc.start( QStringLiteral( "nvme" ), args );
    if( ! proc.waitForStarted( 1500 ) )
    {
        ui->text_NVMe_Log->append( tr( "'nvme' CLI not available. Run on host with root: sudo nvme connect -t %1 -a %2 -s %3 -n %4" )
                                       .arg( transport, addr, port, nqn ) );
        return;
    }

    proc.waitForFinished( 4000 );
    const QString out = QString::fromUtf8( proc.readAllStandardOutput() );
    const QString err = QString::fromUtf8( proc.readAllStandardError() );
    ui->text_NVMe_Log->append( out.isEmpty() ? err : out );

    // Refresh devices after connecting
    Refresh_Host_NVMe_List();
}

void SAN_Storage_Dialog::on_btn_Disconnect_NVMe_clicked()
{
    const QString nqn = ui->combo_NVMe_NQN->currentText().trimmed();
    if( nqn.isEmpty() )
    {
        ui->text_NVMe_Log->append( tr( "Error: Subsystem NQN is required to disconnect." ) );
        return;
    }

    QProcess proc;
    QStringList args;
    args << QStringLiteral( "disconnect" ) << QStringLiteral( "-n" ) << nqn;
    proc.start( QStringLiteral( "nvme" ), args );
    if( proc.waitForStarted( 1500 ) )
    {
        proc.waitForFinished( 4000 );
        ui->text_NVMe_Log->append( tr( "Disconnected: %1" ).arg( QString::fromUtf8( proc.readAllStandardOutput() ) ) );
    }
    Refresh_Host_NVMe_List();
}

void SAN_Storage_Dialog::on_btn_Refresh_Host_NVMe_clicked()
{
    Refresh_Host_NVMe_List();
}

void SAN_Storage_Dialog::Refresh_Host_NVMe_List()
{
    ui->combo_NVMe_Device->clear();

    QDir devDir( QStringLiteral( "/dev" ) );
    const QStringList entries = devDir.entryList( QStringList() << QStringLiteral( "nvme*n*" ), QDir::System );
    for( const QString &e : entries )
    {
        // Ignore partitions (e.g. nvme0n1p1)
        if( ! e.contains( QLatin1Char( 'p' ) ) )
        {
            ui->combo_NVMe_Device->addItem( QStringLiteral( "/dev/" ) + e );
        }
    }

    if( ui->combo_NVMe_Device->count() == 0 )
    {
        ui->combo_NVMe_Device->addItem( QStringLiteral( "/dev/nvme1n1" ) );
    }
}

void SAN_Storage_Dialog::on_btn_Scan_Host_SAN_clicked()
{
    Scan_Host_SAN_Devices();
}

void SAN_Storage_Dialog::Scan_Host_SAN_Devices()
{
    ui->tree_Host_SAN->clear();

    // 1. Scan /dev/disk/by-path/ for iSCSI and NVMe-oF
    QDir byPath( QStringLiteral( "/dev/disk/by-path" ) );
    if( byPath.exists() )
    {
        const QFileInfoList list = byPath.entryInfoList( QDir::Files | QDir::System );
        for( const QFileInfo &fi : list )
        {
            const QString name = fi.fileName();
            if( name.endsWith( QStringLiteral( "-part" ) ) || name.contains( QStringLiteral( "-part" ) ) )
                continue;

            if( name.contains( QStringLiteral( "iscsi" ), Qt::CaseInsensitive ) )
            {
                const QString target = fi.symLinkTarget();
                QTreeWidgetItem *it = new QTreeWidgetItem( ui->tree_Host_SAN );
                it->setIcon( 0, QIcon( QStringLiteral( ":/preferences-system-network-sharing.png" ) ) );
                it->setText( 0, target.isEmpty() ? fi.absoluteFilePath() : target );
                it->setText( 1, QStringLiteral( "iSCSI Target" ) );
                it->setText( 2, QStringLiteral( "SAN LUN" ) );
                it->setText( 3, name );
                it->setData( 0, Qt::UserRole, target.isEmpty() ? fi.absoluteFilePath() : target );
                it->setData( 1, Qt::UserRole, QStringLiteral( "iscsi" ) );
            }
            else if( name.contains( QStringLiteral( "nvme" ), Qt::CaseInsensitive ) )
            {
                const QString target = fi.symLinkTarget();
                QTreeWidgetItem *it = new QTreeWidgetItem( ui->tree_Host_SAN );
                it->setIcon( 0, QIcon( QStringLiteral( ":/blockdevice.png" ) ) );
                it->setText( 0, target.isEmpty() ? fi.absoluteFilePath() : target );
                it->setText( 1, QStringLiteral( "NVMe Storage" ) );
                it->setText( 2, QStringLiteral( "Block Dev" ) );
                it->setText( 3, name );
                it->setData( 0, Qt::UserRole, target.isEmpty() ? fi.absoluteFilePath() : target );
                it->setData( 1, Qt::UserRole, QStringLiteral( "nvme" ) );
            }
        }
    }

    // 2. Scan /sys/class/nvme for active fabric controllers
    QDir sysNvme( QStringLiteral( "/sys/class/nvme" ) );
    if( sysNvme.exists() )
    {
        const QStringList ctrls = sysNvme.entryList( QStringList() << QStringLiteral( "nvme*" ), QDir::Dirs | QDir::NoDotAndDotDot );
        for( const QString &ctrl : ctrls )
        {
            QFile transportFile( sysNvme.filePath( ctrl + QStringLiteral( "/transport" ) ) );
            QString transport = QStringLiteral( "pcie" );
            if( transportFile.open( QIODevice::ReadOnly ) )
            {
                transport = QString::fromUtf8( transportFile.readAll() ).trimmed();
                transportFile.close();
            }

            if( transport == QLatin1String( "tcp" ) || transport == QLatin1String( "rdma" ) || transport == QLatin1String( "fc" ) )
            {
                QFile nqnFile( sysNvme.filePath( ctrl + QStringLiteral( "/subsysnqn" ) ) );
                QString nqn;
                if( nqnFile.open( QIODevice::ReadOnly ) )
                {
                    nqn = QString::fromUtf8( nqnFile.readAll() ).trimmed();
                    nqnFile.close();
                }

                // Check for block devices linked to this controller
                QDir devDir( QStringLiteral( "/dev" ) );
                const QStringList blkList = devDir.entryList( QStringList() << ( ctrl + QStringLiteral( "n*" ) ), QDir::System );
                for( const QString &blk : blkList )
                {
                    if( ! blk.contains( QLatin1Char( 'p' ) ) )
                    {
                        QTreeWidgetItem *it = new QTreeWidgetItem( ui->tree_Host_SAN );
                        it->setIcon( 0, QIcon( QStringLiteral( ":/preferences-system-network-sharing.png" ) ) );
                        it->setText( 0, QStringLiteral( "/dev/" ) + blk );
                        it->setText( 1, QStringLiteral( "NVMe-oF (%1)" ).arg( transport.toUpper() ) );
                        it->setText( 2, QStringLiteral( "Fabric Disk" ) );
                        it->setText( 3, nqn );
                        it->setData( 0, Qt::UserRole, QStringLiteral( "/dev/" ) + blk );
                        it->setData( 1, Qt::UserRole, QStringLiteral( "nvme" ) );
                    }
                }
            }
        }
    }

    if( ui->tree_Host_SAN->topLevelItemCount() == 0 )
    {
        QTreeWidgetItem *it = new QTreeWidgetItem( ui->tree_Host_SAN );
        it->setText( 0, tr( "No SAN devices detected" ) );
        it->setText( 1, QStringLiteral( "-" ) );
        it->setText( 2, QStringLiteral( "-" ) );
        it->setText( 3, tr( "Connect using iSCSI or NVMe-oF tabs above, or log in via host initiator." ) );
    }
}

void SAN_Storage_Dialog::on_tree_Host_SAN_itemDoubleClicked( QTreeWidgetItem *item, int column )
{
    Q_UNUSED( column );
    if( item && ! item->data( 0, Qt::UserRole ).toString().isEmpty() )
    {
        on_btn_Select_Host_SAN_clicked();
    }
}

void SAN_Storage_Dialog::on_btn_Select_Host_SAN_clicked()
{
    QTreeWidgetItem *cur = ui->tree_Host_SAN->currentItem();
    if( ! cur )
    {
        QMessageBox::information( this, tr( "Select Device" ), tr( "Please click a SAN block device in the list first." ) );
        return;
    }

    const QString path = cur->data( 0, Qt::UserRole ).toString();
    const QString type = cur->data( 1, Qt::UserRole ).toString();
    if( path.isEmpty() )
    {
        QMessageBox::information( this, tr( "Select Device" ), tr( "The selected row is not a valid block device." ) );
        return;
    }

    Configured_Device = VM_Native_Storage_Device();
    Configured_Device.Use_File_Path( true );
    Configured_Device.Set_File_Path( path );
    Configured_Device.Use_Media( true );
    Configured_Device.Set_Media( VM::DM_Disk );
    Configured_Device.Use_Cache( true );
    Configured_Device.Set_Cache( QStringLiteral( "none" ) );
    Configured_Device.Use_AIO( true );
    Configured_Device.Set_AIO( QStringLiteral( "threads" ) );
    Configured_Device.Use_Block_Size( true );
    Configured_Device.Set_Logical_Block_Size( 4096 );
    Configured_Device.Set_Physical_Block_Size( 4096 );

    if( type == QLatin1String( "nvme" ) )
    {
        Configured_Device.Use_Interface( true );
        Configured_Device.Set_Interface( VM::DI_NVMe );
    }
    else
    {
        Configured_Device.Use_Interface( true );
        Configured_Device.Set_Interface( VM::DI_Virtio_SCSI );
    }

    accept();
}

void SAN_Storage_Dialog::on_btn_Attach_clicked()
{
    const int tab = ui->tabWidget->currentIndex();
    Configured_Device = VM_Native_Storage_Device();
    Configured_Device.Use_Media( true );
    Configured_Device.Set_Media( VM::DM_Disk );
    Configured_Device.Use_Cache( true );
    Configured_Device.Set_Cache( QStringLiteral( "none" ) );
    Configured_Device.Use_AIO( true );
    Configured_Device.Set_AIO( QStringLiteral( "threads" ) );

    if( tab == 0 ) // iSCSI Target
    {
        Update_iSCSI_URI_Preview();
        const QString uri = ui->edit_iSCSI_Preview->text().trimmed();
        if( uri.isEmpty() || uri.contains( QStringLiteral( "<" ) ) )
        {
            QMessageBox::warning( this, tr( "Incomplete iSCSI Target" ),
                                  tr( "Please specify a Target Host / IP and Target IQN before attaching." ) );
            return;
        }

        Configured_Device.Use_File_Path( true );
        Configured_Device.Set_File_Path( uri );

        // Select interface
        const int iface_idx = ui->combo_iSCSI_Iface->currentIndex();
        VM::Device_Interface iface = VM::DI_Virtio;
        if( iface_idx == 1 ) iface = VM::DI_Virtio_SCSI;
        else if( iface_idx == 2 ) iface = VM::DI_NVMe;
        else if( iface_idx == 3 ) iface = VM::DI_AHCI;

        Configured_Device.Use_Interface( true );
        Configured_Device.Set_Interface( iface );

        // Sector size
        const int sector_size = ( ui->combo_iSCSI_Sector->currentIndex() == 0 ) ? 4096 : 512;
        Configured_Device.Use_Block_Size( true );
        Configured_Device.Set_Logical_Block_Size( sector_size );
        Configured_Device.Set_Physical_Block_Size( 4096 );

        accept();
    }
    else if( tab == 1 ) // NVMe-oF
    {
        QString dev = ui->combo_NVMe_Device->currentText().trimmed();
        if( dev.isEmpty() )
        {
            QMessageBox::warning( this, tr( "Host Device Required" ),
                                  tr( "Please select or enter the mapped host block device (e.g. /dev/nvme1n1)." ) );
            return;
        }

        Configured_Device.Use_File_Path( true );
        Configured_Device.Set_File_Path( dev );

        const int iface_idx = ui->combo_NVMe_Iface->currentIndex();
        Configured_Device.Use_Interface( true );
        Configured_Device.Set_Interface( iface_idx == 0 ? VM::DI_NVMe : VM::DI_Virtio );

        const int sector_size = ( ui->combo_NVMe_Sector->currentIndex() == 0 ) ? 4096 : 512;
        Configured_Device.Use_Block_Size( true );
        Configured_Device.Set_Logical_Block_Size( sector_size );
        Configured_Device.Set_Physical_Block_Size( 4096 );

        accept();
    }
    else // Host inventory
    {
        on_btn_Select_Host_SAN_clicked();
    }
}
