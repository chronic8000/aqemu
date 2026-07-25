# Wave 2 CLI parity: NUMA, watchdog, TPM, secret, incoming, chardev, blockdev
from pathlib import Path

# --- VM.h ---
vm_h = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\VM.h")
h = vm_h.read_text(encoding="utf-8")

api = '''
		/** -numa node topology (simple equal-RAM split across N nodes). */
		bool Use_NUMA() const;
		void Use_NUMA( bool use );
		int Get_NUMA_Nodes() const;
		void Set_NUMA_Nodes( int nodes );

		/** Watchdog device model (empty = off). e.g. i6300esb, ib700 */
		const QString &Get_Watchdog_Model() const;
		void Set_Watchdog_Model( const QString &model );
		const QString &Get_Watchdog_Action() const;
		void Set_Watchdog_Action( const QString &action );

		/** TPM: none | emulator | passthrough */
		const QString &Get_TPM_Type() const;
		void Set_TPM_Type( const QString &type );
		const QString &Get_TPM_Path() const;
		void Set_TPM_Path( const QString &path );

		/** -object secret,id=… for encrypted disks / TLS material */
		bool Use_Secret_Object() const;
		void Use_Secret_Object( bool use );
		const QString &Get_Secret_ID() const;
		void Set_Secret_ID( const QString &id );
		const QString &Get_Secret_Data() const;
		void Set_Secret_Data( const QString &data );
		const QString &Get_Secret_File() const;
		void Set_Secret_File( const QString &path );

		/** -incoming URI for migration listen (e.g. tcp:0:4444) */
		const QString &Get_Incoming_URI() const;
		void Set_Incoming_URI( const QString &uri );

		/** Prefer -chardev + -serial chardev:id over legacy -serial backends */
		bool Use_Modern_Chardev() const;
		void Use_Modern_Chardev( bool use );

		/** Prefer -blockdev + -device for native virtio/nvme disks */
		bool Use_Blockdev() const;
		void Use_Blockdev( bool use );

'''

needle = '\t\tbool Use_Modern_Netdev() const;\n\t\tvoid Use_Modern_Netdev( bool use );\n'
if 'Use_NUMA()' not in h:
    if needle not in h:
        raise SystemExit('API needle missing')
    h = h.replace(needle, needle + '\n' + api)

members = '''\t\tbool Modern_Netdev;
\t\tbool Use_NUMA_Flag;
\t\tint NUMA_Nodes;
\t\tQString Watchdog_Model;
\t\tQString Watchdog_Action;
\t\tQString TPM_Type;
\t\tQString TPM_Path;
\t\tbool Use_Secret_Object_Flag;
\t\tQString Secret_ID;
\t\tQString Secret_Data;
\t\tQString Secret_File;
\t\tQString Incoming_URI;
\t\tbool Modern_Chardev;
\t\tbool Use_Blockdev_Flag;
\t\tQString Display_Window_Mode; // auto | embedded | native'''

old_m = '\t\tbool Modern_Netdev;\n\t\tQString Display_Window_Mode; // auto | embedded | native'
if old_m not in h:
    raise SystemExit('members needle missing: ' + repr(old_m[:40]))
h = h.replace(old_m, members)
vm_h.write_text(h, encoding='utf-8')
print('VM.h OK')

# --- VM.cpp ---
p = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\VM.cpp")
t = p.read_text(encoding='utf-8')

def must_replace(old, new, label, count=1):
    global t
    if old not in t:
        raise SystemExit('missing ' + label)
    t = t.replace(old, new, count)

# copy this->
must_replace(
    '\tthis->Modern_Netdev = vm.Use_Modern_Netdev();',
    '\tthis->Modern_Netdev = vm.Use_Modern_Netdev();\n'
    '\tthis->Use_NUMA_Flag = vm.Use_NUMA();\n'
    '\tthis->NUMA_Nodes = vm.Get_NUMA_Nodes();\n'
    '\tthis->Watchdog_Model = vm.Get_Watchdog_Model();\n'
    '\tthis->Watchdog_Action = vm.Get_Watchdog_Action();\n'
    '\tthis->TPM_Type = vm.Get_TPM_Type();\n'
    '\tthis->TPM_Path = vm.Get_TPM_Path();\n'
    '\tthis->Use_Secret_Object_Flag = vm.Use_Secret_Object();\n'
    '\tthis->Secret_ID = vm.Get_Secret_ID();\n'
    '\tthis->Secret_Data = vm.Get_Secret_Data();\n'
    '\tthis->Secret_File = vm.Get_Secret_File();\n'
    '\tthis->Incoming_URI = vm.Get_Incoming_URI();\n'
    '\tthis->Modern_Chardev = vm.Use_Modern_Chardev();\n'
    '\tthis->Use_Blockdev_Flag = vm.Use_Blockdev();',
    'copy',
)

must_replace(
    '\tModern_Netdev = true;',
    '\tModern_Netdev = true;\n'
    '\tUse_NUMA_Flag = false;\n'
    '\tNUMA_Nodes = 2;\n'
    '\tWatchdog_Model.clear();\n'
    '\tWatchdog_Action = QStringLiteral( "reset" );\n'
    '\tTPM_Type = QStringLiteral( "none" );\n'
    '\tTPM_Path.clear();\n'
    '\tUse_Secret_Object_Flag = false;\n'
    '\tSecret_ID = QStringLiteral( "sec0" );\n'
    '\tSecret_Data.clear();\n'
    '\tSecret_File.clear();\n'
    '\tIncoming_URI.clear();\n'
    '\tModern_Chardev = false;\n'
    '\tUse_Blockdev_Flag = false;',
    'init',
)

must_replace(
    '\t\tthis->Modern_Netdev == vm.Use_Modern_Netdev() &&',
    '\t\tthis->Modern_Netdev == vm.Use_Modern_Netdev() &&\n'
    '\t\tthis->Use_NUMA_Flag == vm.Use_NUMA() &&\n'
    '\t\tthis->NUMA_Nodes == vm.Get_NUMA_Nodes() &&\n'
    '\t\tthis->Watchdog_Model == vm.Get_Watchdog_Model() &&\n'
    '\t\tthis->Watchdog_Action == vm.Get_Watchdog_Action() &&\n'
    '\t\tthis->TPM_Type == vm.Get_TPM_Type() &&\n'
    '\t\tthis->TPM_Path == vm.Get_TPM_Path() &&\n'
    '\t\tthis->Use_Secret_Object_Flag == vm.Use_Secret_Object() &&\n'
    '\t\tthis->Secret_ID == vm.Get_Secret_ID() &&\n'
    '\t\tthis->Secret_Data == vm.Get_Secret_Data() &&\n'
    '\t\tthis->Secret_File == vm.Get_Secret_File() &&\n'
    '\t\tthis->Incoming_URI == vm.Get_Incoming_URI() &&\n'
    '\t\tthis->Modern_Chardev == vm.Use_Modern_Chardev() &&\n'
    '\t\tthis->Use_Blockdev_Flag == vm.Use_Blockdev() &&',
    'cmp',
)

must_replace(
    '\tModern_Netdev = vm.Use_Modern_Netdev();',
    '\tModern_Netdev = vm.Use_Modern_Netdev();\n'
    '\tUse_NUMA_Flag = vm.Use_NUMA();\n'
    '\tNUMA_Nodes = vm.Get_NUMA_Nodes();\n'
    '\tWatchdog_Model = vm.Get_Watchdog_Model();\n'
    '\tWatchdog_Action = vm.Get_Watchdog_Action();\n'
    '\tTPM_Type = vm.Get_TPM_Type();\n'
    '\tTPM_Path = vm.Get_TPM_Path();\n'
    '\tUse_Secret_Object_Flag = vm.Use_Secret_Object();\n'
    '\tSecret_ID = vm.Get_Secret_ID();\n'
    '\tSecret_Data = vm.Get_Secret_Data();\n'
    '\tSecret_File = vm.Get_Secret_File();\n'
    '\tIncoming_URI = vm.Get_Incoming_URI();\n'
    '\tModern_Chardev = vm.Use_Modern_Chardev();\n'
    '\tUse_Blockdev_Flag = vm.Use_Blockdev();',
    'op=',
)

# XML save after Modern_Netdev
must_replace(
    '\tDom_Element = New_Dom_Document.createElement( "Modern_Netdev" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Modern_Netdev ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );',
    '\tDom_Element = New_Dom_Document.createElement( "Modern_Netdev" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Modern_Netdev ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Use_NUMA" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Use_NUMA_Flag ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "NUMA_Nodes" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( QString::number( NUMA_Nodes ) );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Watchdog_Model" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Watchdog_Model );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Watchdog_Action" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Watchdog_Action );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "TPM_Type" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( TPM_Type );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "TPM_Path" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( TPM_Path );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Use_Secret_Object" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Use_Secret_Object_Flag ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Secret_ID" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Secret_ID );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Secret_Data" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Secret_Data );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Secret_File" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Secret_File );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Incoming_URI" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Incoming_URI );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Modern_Chardev" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Modern_Chardev ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Use_Blockdev" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Use_Blockdev_Flag ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );',
    'xml save',
)

# XML load after Modern_Netdev block
must_replace(
    '\t\t\t{\n'
    '\t\t\t\tconst QString mn = Child_Element.firstChildElement( "Modern_Netdev" ).text();\n'
    '\t\t\t\tModern_Netdev = ( mn.isEmpty() || mn == "true" );\n'
    '\t\t\t}',
    '\t\t\t{\n'
    '\t\t\t\tconst QString mn = Child_Element.firstChildElement( "Modern_Netdev" ).text();\n'
    '\t\t\t\tModern_Netdev = ( mn.isEmpty() || mn == "true" );\n'
    '\t\t\t}\n'
    '\t\t\tUse_NUMA_Flag = ( Child_Element.firstChildElement( "Use_NUMA" ).text() == "true" );\n'
    '\t\t\t{\n'
    '\t\t\t\tbool ok = false;\n'
    '\t\t\t\tint n = Child_Element.firstChildElement( "NUMA_Nodes" ).text().toInt( &ok );\n'
    '\t\t\t\tNUMA_Nodes = ( ok && n >= 2 ) ? n : 2;\n'
    '\t\t\t}\n'
    '\t\t\tWatchdog_Model = Child_Element.firstChildElement( "Watchdog_Model" ).text().trimmed();\n'
    '\t\t\t{\n'
    '\t\t\t\tconst QString wa = Child_Element.firstChildElement( "Watchdog_Action" ).text().trimmed();\n'
    '\t\t\t\tWatchdog_Action = wa.isEmpty() ? QStringLiteral( "reset" ) : wa;\n'
    '\t\t\t}\n'
    '\t\t\t{\n'
    '\t\t\t\tconst QString tt = Child_Element.firstChildElement( "TPM_Type" ).text().trimmed().toLower();\n'
    '\t\t\t\tTPM_Type = tt.isEmpty() ? QStringLiteral( "none" ) : tt;\n'
    '\t\t\t}\n'
    '\t\t\tTPM_Path = Child_Element.firstChildElement( "TPM_Path" ).text();\n'
    '\t\t\tUse_Secret_Object_Flag = ( Child_Element.firstChildElement( "Use_Secret_Object" ).text() == "true" );\n'
    '\t\t\t{\n'
    '\t\t\t\tconst QString sid = Child_Element.firstChildElement( "Secret_ID" ).text().trimmed();\n'
    '\t\t\t\tSecret_ID = sid.isEmpty() ? QStringLiteral( "sec0" ) : sid;\n'
    '\t\t\t}\n'
    '\t\t\tSecret_Data = Child_Element.firstChildElement( "Secret_Data" ).text();\n'
    '\t\t\tSecret_File = Child_Element.firstChildElement( "Secret_File" ).text();\n'
    '\t\t\tIncoming_URI = Child_Element.firstChildElement( "Incoming_URI" ).text().trimmed();\n'
    '\t\t\tModern_Chardev = ( Child_Element.firstChildElement( "Modern_Chardev" ).text() == "true" );\n'
    '\t\t\tUse_Blockdev_Flag = ( Child_Element.firstChildElement( "Use_Blockdev" ).text() == "true" );',
    'xml load',
)

# getters after Use_Modern_Netdev
must_replace(
    'void Virtual_Machine::Use_Modern_Netdev( bool use )\n'
    '{\n'
    '\tModern_Netdev = use;\n'
    '}\n',
    'void Virtual_Machine::Use_Modern_Netdev( bool use )\n'
    '{\n'
    '\tModern_Netdev = use;\n'
    '}\n\n'
    'bool Virtual_Machine::Use_NUMA() const { return Use_NUMA_Flag; }\n'
    'void Virtual_Machine::Use_NUMA( bool use ) { Use_NUMA_Flag = use; }\n'
    'int Virtual_Machine::Get_NUMA_Nodes() const { return NUMA_Nodes; }\n'
    'void Virtual_Machine::Set_NUMA_Nodes( int nodes ) { NUMA_Nodes = qMax( 2, nodes ); }\n\n'
    'const QString &Virtual_Machine::Get_Watchdog_Model() const { return Watchdog_Model; }\n'
    'void Virtual_Machine::Set_Watchdog_Model( const QString &model ) { Watchdog_Model = model.trimmed(); }\n'
    'const QString &Virtual_Machine::Get_Watchdog_Action() const { return Watchdog_Action; }\n'
    'void Virtual_Machine::Set_Watchdog_Action( const QString &action )\n'
    '{\n'
    '\tWatchdog_Action = action.trimmed().isEmpty() ? QStringLiteral( "reset" ) : action.trimmed();\n'
    '}\n\n'
    'const QString &Virtual_Machine::Get_TPM_Type() const { return TPM_Type; }\n'
    'void Virtual_Machine::Set_TPM_Type( const QString &type )\n'
    '{\n'
    '\tTPM_Type = type.trimmed().toLower();\n'
    '\tif( TPM_Type.isEmpty() ) TPM_Type = QStringLiteral( "none" );\n'
    '}\n'
    'const QString &Virtual_Machine::Get_TPM_Path() const { return TPM_Path; }\n'
    'void Virtual_Machine::Set_TPM_Path( const QString &path ) { TPM_Path = path; }\n\n'
    'bool Virtual_Machine::Use_Secret_Object() const { return Use_Secret_Object_Flag; }\n'
    'void Virtual_Machine::Use_Secret_Object( bool use ) { Use_Secret_Object_Flag = use; }\n'
    'const QString &Virtual_Machine::Get_Secret_ID() const { return Secret_ID; }\n'
    'void Virtual_Machine::Set_Secret_ID( const QString &id )\n'
    '{\n'
    '\tSecret_ID = id.trimmed().isEmpty() ? QStringLiteral( "sec0" ) : id.trimmed();\n'
    '}\n'
    'const QString &Virtual_Machine::Get_Secret_Data() const { return Secret_Data; }\n'
    'void Virtual_Machine::Set_Secret_Data( const QString &data ) { Secret_Data = data; }\n'
    'const QString &Virtual_Machine::Get_Secret_File() const { return Secret_File; }\n'
    'void Virtual_Machine::Set_Secret_File( const QString &path ) { Secret_File = path; }\n\n'
    'const QString &Virtual_Machine::Get_Incoming_URI() const { return Incoming_URI; }\n'
    'void Virtual_Machine::Set_Incoming_URI( const QString &uri ) { Incoming_URI = uri.trimmed(); }\n\n'
    'bool Virtual_Machine::Use_Modern_Chardev() const { return Modern_Chardev; }\n'
    'void Virtual_Machine::Use_Modern_Chardev( bool use ) { Modern_Chardev = use; }\n'
    'bool Virtual_Machine::Use_Blockdev() const { return Use_Blockdev_Flag; }\n'
    'void Virtual_Machine::Use_Blockdev( bool use ) { Use_Blockdev_Flag = use; }\n',
    'getters',
)

# Emit after iothread object
must_replace(
    '\tif( Use_IOThread_Flag )\n'
    '\t\tArgs << "-object" << "iothread,id=aq-iothread0";\n',
    '\tif( Use_IOThread_Flag )\n'
    '\t\tArgs << "-object" << "iothread,id=aq-iothread0";\n\n'
    '\tif( Use_Secret_Object_Flag )\n'
    '\t{\n'
    '\t\tQString sid = Secret_ID.trimmed().isEmpty() ? QStringLiteral( "sec0" ) : Secret_ID.trimmed();\n'
    '\t\tQString sobj = QStringLiteral( "secret,id=" ) + sid;\n'
    '\t\tif( ! Secret_File.trimmed().isEmpty() )\n'
    '\t\t{\n'
    '\t\t\tif( Build_QEMU_Args_for_Script_Mode )\n'
    '\t\t\t\tsobj += QStringLiteral( ",file=\\"" ) + Secret_File + QStringLiteral( "\\"" );\n'
    '\t\t\telse\n'
    '\t\t\t\tsobj += QStringLiteral( ",file=" ) + Secret_File;\n'
    '\t\t}\n'
    '\t\telse if( ! Secret_Data.isEmpty() )\n'
    '\t\t{\n'
    '\t\t\t// data= is plain text; prefer file= for production secrets\n'
    '\t\t\tQString d = Secret_Data;\n'
    '\t\t\td.replace( QLatin1Char( \',\' ), QLatin1String( ",," ) );\n'
    '\t\t\tsobj += QStringLiteral( ",data=" ) + d;\n'
    '\t\t}\n'
    '\t\tArgs << "-object" << sobj;\n'
    '\t}\n\n'
    '\tif( ! Incoming_URI.trimmed().isEmpty() )\n'
    '\t\tArgs << "-incoming" << Incoming_URI.trimmed();\n',
    'emit secret/incoming',
)

# NUMA after -m / mem-path block — find uuid emit and append numa after memory section
must_replace(
    '\tif( ! UUID.trimmed().isEmpty() )\n'
    '\t\tArgs << "-uuid" << UUID.trimmed();\n'
    '\t\n'
    '\t// full screen',
    '\tif( ! UUID.trimmed().isEmpty() )\n'
    '\t\tArgs << "-uuid" << UUID.trimmed();\n\n'
    '\t// NUMA (equal split of -m across nodes)\n'
    '\tif( Use_NUMA_Flag && NUMA_Nodes >= 2 && Memory_Size > 0 )\n'
    '\t{\n'
    '\t\tconst int nodes = qMin( NUMA_Nodes, 8 );\n'
    '\t\tconst int per = Memory_Size / nodes;\n'
    '\t\tint rem = Memory_Size - ( per * nodes );\n'
    '\t\tfor( int ni = 0; ni < nodes; ++ni )\n'
    '\t\t{\n'
    '\t\t\tint mb = per + ( ni == 0 ? rem : 0 );\n'
    '\t\t\tArgs << "-numa" << QStringLiteral( "node,nodeid=%1,mem=%2" ).arg( ni ).arg( mb );\n'
    '\t\t}\n'
    '\t}\n'
    '\t\n'
    '\t// full screen',
    'numa',
)

# Watchdog + TPM before SPICE / after BIOS
must_replace(
    '\t// BIOS (SeaBIOS alternative / board firmware)\n',
    '\t// Watchdog (qemu-doc)\n'
    '\tif( ! Watchdog_Model.trimmed().isEmpty() )\n'
    '\t{\n'
    '\t\tArgs << "-watchdog" << Watchdog_Model.trimmed();\n'
    '\t\tconst QString act = Watchdog_Action.trimmed().isEmpty()\n'
    '\t\t\t? QStringLiteral( "reset" ) : Watchdog_Action.trimmed();\n'
    '\t\tArgs << "-watchdog-action" << act;\n'
    '\t}\n\n'
    '\t// TPM\n'
    '\t{\n'
    '\t\tconst QString tt = TPM_Type.trimmed().toLower();\n'
    '\t\tif( tt == QLatin1String( "emulator" ) )\n'
    '\t\t{\n'
    '\t\t\tQString path = TPM_Path.trimmed().isEmpty()\n'
    '\t\t\t\t? QStringLiteral( "/tmp/aqemu-swtpm.sock" ) : TPM_Path.trimmed();\n'
    '\t\t\tArgs << "-tpmdev" << QStringLiteral( "emulator,id=tpm0,chardev=chrtpm" );\n'
    '\t\t\tArgs << "-chardev" << QStringLiteral( "socket,id=chrtpm,path=" ) + path;\n'
    '\t\t\tArgs << "-device" << "tpm-tis,tpmdev=tpm0";\n'
    '\t\t}\n'
    '\t\telse if( tt == QLatin1String( "passthrough" ) && ! TPM_Path.trimmed().isEmpty() )\n'
    '\t\t{\n'
    '\t\t\tArgs << "-tpmdev" << QStringLiteral( "passthrough,id=tpm0,path=" ) + TPM_Path.trimmed();\n'
    '\t\t\tArgs << "-device" << "tpm-tis,tpmdev=tpm0";\n'
    '\t\t}\n'
    '\t}\n\n'
    '\t// BIOS (SeaBIOS alternative / board firmware)\n',
    'watchdog/tpm',
)

# Modern chardev for serial ports
old_serial = '''\t// Ports Tabs
\tfor( int ix = 0; ix < Serial_Ports.count(); ix++ )
\t{
\t\tif( Serial_Ports[ix].Get_Port_Redirection() == VM::PR_Default ) continue;
\t\t
\t\tArgs << "-serial";
\t\t
\t\tswitch( Serial_Ports[ix].Get_Port_Redirection() )
\t\t{
\t\t\tcase VM::PR_vc:
\t\t\t\tArgs << "vc:" + Serial_Ports[ix].Get_Parametrs_Line();
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_pty:
\t\t\t\tArgs << "pty";
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_none:
\t\t\t\tArgs << "none";
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_null:
\t\t\t\tArgs << "null";
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_dev:
\t\t\t\tArgs << Serial_Ports[ix].Get_Parametrs_Line();
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_file:
\t\t\t\tArgs << "file:" + Serial_Ports[ix].Get_Parametrs_Line();
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_stdio:
\t\t\t\tArgs << "stdio";
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_pipe:
\t\t\t\tArgs << "pipe:" + Serial_Ports[ix].Get_Parametrs_Line();
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_udp:
\t\t\t\tArgs << "udp:" + Serial_Ports[ix].Get_Parametrs_Line();
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_tcp:
\t\t\t\tArgs << "tcp:" + Serial_Ports[ix].Get_Parametrs_Line();
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_telnet:
\t\t\t\tArgs << "telnet:" + Serial_Ports[ix].Get_Parametrs_Line();
\t\t\t\tbreak;
\t\t\t
\t\t\tcase VM::PR_unix:'''

# Read rest of unix and default from file to replace whole loop start with modern path
# Simpler: insert modern branch at start of loop body
must_replace(
    '\t// Ports Tabs\n'
    '\tfor( int ix = 0; ix < Serial_Ports.count(); ix++ )\n'
    '\t{\n'
    '\t\tif( Serial_Ports[ix].Get_Port_Redirection() == VM::PR_Default ) continue;\n'
    '\t\t\n'
    '\t\tArgs << "-serial";\n',
    '\t// Ports Tabs\n'
    '\tfor( int ix = 0; ix < Serial_Ports.count(); ix++ )\n'
    '\t{\n'
    '\t\tif( Serial_Ports[ix].Get_Port_Redirection() == VM::PR_Default ) continue;\n\n'
    '\t\t// Modern -chardev path (qemu-doc Character devices)\n'
    '\t\tif( Modern_Chardev )\n'
    '\t\t{\n'
    '\t\t\tconst QString cid = QStringLiteral( "aqchr%1" ).arg( ix );\n'
    '\t\t\tQString cdev;\n'
    '\t\t\tconst VM::Port_Redirection pr = Serial_Ports[ix].Get_Port_Redirection();\n'
    '\t\t\tconst QString params = Serial_Ports[ix].Get_Parametrs_Line();\n'
    '\t\t\tswitch( pr )\n'
    '\t\t\t{\n'
    '\t\t\t\tcase VM::PR_null: cdev = QStringLiteral( "null,id=" ) + cid; break;\n'
    '\t\t\t\tcase VM::PR_stdio: cdev = QStringLiteral( "stdio,id=" ) + cid; break;\n'
    '\t\t\t\tcase VM::PR_pty: cdev = QStringLiteral( "pty,id=" ) + cid; break;\n'
    '\t\t\t\tcase VM::PR_file:\n'
    '\t\t\t\t\tcdev = QStringLiteral( "file,id=" ) + cid + QStringLiteral( ",path=" ) + params;\n'
    '\t\t\t\t\tbreak;\n'
    '\t\t\t\tcase VM::PR_pipe:\n'
    '\t\t\t\t\tcdev = QStringLiteral( "pipe,id=" ) + cid + QStringLiteral( ",path=" ) + params;\n'
    '\t\t\t\t\tbreak;\n'
    '\t\t\t\tcase VM::PR_tcp:\n'
    '\t\t\t\t\tcdev = QStringLiteral( "socket,id=" ) + cid + QStringLiteral( ",host=," )\n'
    '\t\t\t\t\t       + QStringLiteral( "port=" ) + params + QStringLiteral( ",server=on,wait=off" );\n'
    '\t\t\t\t\t// params often "host:port" — if contains \':\', split\n'
    '\t\t\t\t\tif( params.contains( QLatin1Char( \':\' ) ) )\n'
    '\t\t\t\t\t{\n'
    '\t\t\t\t\t\tconst QString host = params.section( QLatin1Char( \':\' ), 0, 0 );\n'
    '\t\t\t\t\t\tconst QString port = params.section( QLatin1Char( \':\' ), 1 );\n'
    '\t\t\t\t\t\tcdev = QStringLiteral( "socket,id=" ) + cid + QStringLiteral( ",host=" )\n'
    '\t\t\t\t\t\t       + host + QStringLiteral( ",port=" ) + port + QStringLiteral( ",server=on,wait=off" );\n'
    '\t\t\t\t\t}\n'
    '\t\t\t\t\tbreak;\n'
    '\t\t\t\tcase VM::PR_unix:\n'
    '\t\t\t\t\tcdev = QStringLiteral( "socket,id=" ) + cid + QStringLiteral( ",path=" )\n'
    '\t\t\t\t\t       + params + QStringLiteral( ",server=on,wait=off" );\n'
    '\t\t\t\t\tbreak;\n'
    '\t\t\t\tcase VM::PR_vc:\n'
    '\t\t\t\t\tcdev = QStringLiteral( "vc,id=" ) + cid;\n'
    '\t\t\t\t\tif( ! params.isEmpty() ) cdev += QLatin1Char( \',\' ) + params;\n'
    '\t\t\t\t\tbreak;\n'
    '\t\t\t\tdefault:\n'
    '\t\t\t\t\tcdev.clear();\n'
    '\t\t\t\t\tbreak;\n'
    '\t\t\t}\n'
    '\t\t\tif( ! cdev.isEmpty() )\n'
    '\t\t\t{\n'
    '\t\t\t\tArgs << "-chardev" << cdev;\n'
    '\t\t\t\tArgs << "-serial" << QStringLiteral( "chardev:" ) + cid;\n'
    '\t\t\t\tcontinue;\n'
    '\t\t\t}\n'
    '\t\t}\n'
    '\t\t\n'
    '\t\tArgs << "-serial";\n',
    'chardev',
)

# Blockdev emit — at end of Build_Native_Device_Args before return, if Use_Blockdev_Flag and virtio/nvme
# Find return args; at the virtio-blk section we already have device. Add alternative path.

# After building driveStr, if Use_Blockdev_Flag && (virtio or nvme disk):
old_ret = '''\t// return
\tQStringList args;
\tconst bool virt_arch_blk ='''
# Insert before that a note - actually replace the final return assembly for virtio

# Simpler approach: after `args << "-drive" << driveStr; return args;` for virtio path,
# when Use_Blockdev_Flag, emit blockdev instead.

# Find:
# args << "-drive" << driveStr;
# return args;
# There may be multiple. Look at the function end.

idx = t.find('QStringList Virtual_Machine::Build_Native_Device_Args')
end = t.find('\nQStringList Virtual_Machine::Build_Shared_Folder_Args', idx)
fn = t[idx:end]
if 'Use_Blockdev_Flag' in fn:
    print('blockdev already in native')
else:
    # Replace the final `args << "-drive" << driveStr;\n\treturn args;`
    old = '\targs << "-drive" << driveStr;\n\treturn args;\n}'
    # only in this function - replace last occurrence in fn
    if fn.count('args << "-drive" << driveStr;') < 1:
        raise SystemExit('drive return missing')
    new_fn = fn.replace(
        '\targs << "-drive" << driveStr;\n\treturn args;\n',
        '\tif( Use_Blockdev_Flag && device.Use_File_Path() &&\n'
        '\t    ( device.Get_Interface() == VM::DI_Virtio ||\n'
        '\t      device.Get_Interface() == VM::DI_NVMe ||\n'
        '\t      device.Get_Interface() == VM::DI_Virtio_SCSI ) )\n'
        '\t{\n'
        '\t\t// Modern -blockdev graph (qemu-doc): file node + format node\n'
        '\t\tconst QString node = QStringLiteral( "aqbd-" ) + vsname;\n'
        '\t\tconst QString file_node = node + QStringLiteral( "-file" );\n'
        '\t\tQString file_bd = QStringLiteral( "driver=file,node-name=" ) + file_node +\n'
        '\t\t                  QStringLiteral( ",filename=" ) + device.Get_File_Path();\n'
        '\t\tArgs_dummy_unused:\n'
        '\t\t;\n'
        '\t\tQString fmt = QStringLiteral( "raw" );\n'
        '\t\t// detect qcow2 by extension\n'
        '\t\tif( device.Get_File_Path().endsWith( QLatin1String( ".qcow2" ), Qt::CaseInsensitive ) )\n'
        '\t\t\tfmt = QStringLiteral( "qcow2" );\n'
        '\t\tQString fmt_bd = QStringLiteral( "driver=" ) + fmt + QStringLiteral( ",node-name=" ) +\n'
        '\t\t                 node + QStringLiteral( ",file=" ) + file_node;\n'
        '\t\tQStringList bd_args;\n'
        '\t\tbd_args << "-blockdev" << file_bd;\n'
        '\t\tbd_args << "-blockdev" << fmt_bd;\n'
        '\t\t// Replace -drive with drive=node on already-queued -device lines\n'
        '\t\tfor( int ai = 0; ai < args.count(); ++ai )\n'
        '\t\t{\n'
        '\t\t\tif( args[ai].contains( QStringLiteral( "drive=" ) + vsname ) )\n'
        '\t\t\t\targs[ai].replace( QStringLiteral( "drive=" ) + vsname,\n'
        '\t\t\t\t                  QStringLiteral( "drive=" ) + node );\n'
        '\t\t}\n'
        '\t\tbd_args << args;\n'
        '\t\treturn bd_args;\n'
        '\t}\n'
        '\targs << "-drive" << driveStr;\n'
        '\treturn args;\n',
        1,
    )
    # Remove Accidental Args_dummy
    new_fn = new_fn.replace('\t\tArgs_dummy_unused:\n\t\t;\n', '')
    t = t[:idx] + new_fn + t[end:]
    print('blockdev path added')

p.write_text(t, encoding='utf-8')
print('VM.cpp OK')
