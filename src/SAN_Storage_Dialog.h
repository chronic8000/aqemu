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

#ifndef SAN_STORAGE_DIALOG_H
#define SAN_STORAGE_DIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include "VM_Devices.h"

namespace Ui {
    class SAN_Storage_Dialog;
}

class SAN_Storage_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit SAN_Storage_Dialog( QWidget *parent = nullptr );
    ~SAN_Storage_Dialog();

    VM_Native_Storage_Device Get_Configured_Device() const;
    QString Get_Device_Path() const;
    VM::Device_Interface Get_Device_Interface() const;
    int Get_Logical_Sector_Size() const;

private slots:
    void on_tabWidget_currentChanged( int index );
    void on_iSCSI_Changed();
    void on_NVMe_Changed();

    // iSCSI Actions
    void on_btn_Discover_iSCSI_clicked();
    void on_btn_Test_iSCSI_clicked();
    void on_btn_Copy_iSCSI_URI_clicked();
    void on_check_iSCSI_CHAP_toggled( bool checked );

    // NVMe-oF Actions
    void on_btn_Discover_NVMe_clicked();
    void on_btn_Connect_NVMe_clicked();
    void on_btn_Disconnect_NVMe_clicked();
    void on_btn_Test_NVMe_clicked();
    void on_btn_Refresh_Host_NVMe_clicked();

    // Host SAN Inventory Scanner
    void on_btn_Scan_Host_SAN_clicked();
    void on_tree_Host_SAN_itemDoubleClicked( QTreeWidgetItem *item, int column );
    void on_btn_Select_Host_SAN_clicked();

    void on_btn_Attach_clicked();

private:
    void Update_iSCSI_URI_Preview();
    void Refresh_Host_NVMe_List();
    void Scan_Host_SAN_Devices();
    bool Test_TCP_Port( const QString &host, quint16 port, QString &out_msg );

    Ui::SAN_Storage_Dialog *ui;
    VM_Native_Storage_Device Configured_Device;
};

#endif // SAN_STORAGE_DIALOG_H
