# Wave 3 CLI parity: SMBIOS, fw_cfg, audiodev, NUMA memdev, icount/sandbox, chardev gaps
from pathlib import Path

vm_h = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\VM.h")
h = vm_h.read_text(encoding="utf-8")

api = r'''
		/** -smbios type=0 / type=1 / file= (empty fields omitted). */
		bool Use_SMBIOS_Type0() const;
		void Use_SMBIOS_Type0( bool use );
		const QString &Get_SMBIOS_Vendor() const;
		void Set_SMBIOS_Vendor( const QString &v );
		const QString &Get_SMBIOS_Version() const;
		void Set_SMBIOS_Version( const QString &v );
		const QString &Get_SMBIOS_Date() const;
		void Set_SMBIOS_Date( const QString &v );
		bool Use_SMBIOS_Type1() const;
		void Use_SMBIOS_Type1( bool use );
		const QString &Get_SMBIOS_Manufacturer() const;
		void Set_SMBIOS_Manufacturer( const QString &v );
		const QString &Get_SMBIOS_Product() const;
		void Set_SMBIOS_Product( const QString &v );
		const QString &Get_SMBIOS_Type1_Version() const;
		void Set_SMBIOS_Type1_Version( const QString &v );
		const QString &Get_SMBIOS_Serial() const;
		void Set_SMBIOS_Serial( const QString &v );
		const QString &Get_SMBIOS_File() const;
		void Set_SMBIOS_File( const QString &path );

		/** Extra -fw_cfg lines (one per line: name=X,file=Y or name=X,string=Z). */
		const QString &Get_FW_CFG_Lines() const;
		void Set_FW_CFG_Lines( const QString &lines );

		/** Per-VM -audiodev backend (empty = use Advanced Settings global). */
		const QString &Get_Audiodev_Backend() const;
		void Set_Audiodev_Backend( const QString &backend );
		/** Optional timer-period=N microseconds for -audiodev (0 = omit). */
		int Get_Audiodev_Timer_Period() const;
		void Set_Audiodev_Timer_Period( int us );

		/** Prefer -object memory-backend-ram + -numa node,memdev=… */
		bool Use_NUMA_Memdev() const;
		void Use_NUMA_Memdev( bool use );

		/** -icount (empty = off). e.g. auto or shift=7 */
		const QString &Get_ICount() const;
		void Set_ICount( const QString &icount );

		/** -sandbox (empty = off). e.g. on */
		const QString &Get_Sandbox() const;
		void Set_Sandbox( const QString &sandbox );

'''

needle = '\t\tbool Use_Blockdev() const;\n\t\tvoid Use_Blockdev( bool use );\n'
if 'Use_SMBIOS_Type0()' not in h:
    if needle not in h:
        raise SystemExit('API needle missing')
    h = h.replace(needle, needle + '\n' + api)

members = '''\t\tbool Use_Blockdev_Flag;
\t\tbool Use_SMBIOS_Type0_Flag;
\t\tQString SMBIOS_Vendor;
\t\tQString SMBIOS_Version;
\t\tQString SMBIOS_Date;
\t\tbool Use_SMBIOS_Type1_Flag;
\t\tQString SMBIOS_Manufacturer;
\t\tQString SMBIOS_Product;
\t\tQString SMBIOS_Type1_Version;
\t\tQString SMBIOS_Serial;
\t\tQString SMBIOS_File;
\t\tQString FW_CFG_Lines;
\t\tQString Audiodev_Backend;
\t\tint Audiodev_Timer_Period;
\t\tbool Use_NUMA_Memdev_Flag;
\t\tQString ICount;
\t\tQString Sandbox;
\t\tQString Display_Window_Mode; // auto | embedded | native'''

old_m = '\t\tbool Use_Blockdev_Flag;\n\t\tQString Display_Window_Mode; // auto | embedded | native'
if 'Use_SMBIOS_Type0_Flag' not in h:
    if old_m not in h:
        raise SystemExit('members needle missing')
    h = h.replace(old_m, members)
vm_h.write_text(h, encoding='utf-8')
print('VM.h OK')

p = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\VM.cpp")
t = p.read_text(encoding='utf-8')

def must_replace(old, new, label, count=1):
    global t
    if old not in t:
        raise SystemExit('missing ' + label)
    t = t.replace(old, new, count)

# copy ctor
must_replace(
    '\tthis->Use_Blockdev_Flag = vm.Use_Blockdev();',
    '\tthis->Use_Blockdev_Flag = vm.Use_Blockdev();\n'
    '\tthis->Use_SMBIOS_Type0_Flag = vm.Use_SMBIOS_Type0();\n'
    '\tthis->SMBIOS_Vendor = vm.Get_SMBIOS_Vendor();\n'
    '\tthis->SMBIOS_Version = vm.Get_SMBIOS_Version();\n'
    '\tthis->SMBIOS_Date = vm.Get_SMBIOS_Date();\n'
    '\tthis->Use_SMBIOS_Type1_Flag = vm.Use_SMBIOS_Type1();\n'
    '\tthis->SMBIOS_Manufacturer = vm.Get_SMBIOS_Manufacturer();\n'
    '\tthis->SMBIOS_Product = vm.Get_SMBIOS_Product();\n'
    '\tthis->SMBIOS_Type1_Version = vm.Get_SMBIOS_Type1_Version();\n'
    '\tthis->SMBIOS_Serial = vm.Get_SMBIOS_Serial();\n'
    '\tthis->SMBIOS_File = vm.Get_SMBIOS_File();\n'
    '\tthis->FW_CFG_Lines = vm.Get_FW_CFG_Lines();\n'
    '\tthis->Audiodev_Backend = vm.Get_Audiodev_Backend();\n'
    '\tthis->Audiodev_Timer_Period = vm.Get_Audiodev_Timer_Period();\n'
    '\tthis->Use_NUMA_Memdev_Flag = vm.Use_NUMA_Memdev();\n'
    '\tthis->ICount = vm.Get_ICount();\n'
    '\tthis->Sandbox = vm.Get_Sandbox();',
    'copy',
)

# init defaults
must_replace(
    '\tUse_Blockdev_Flag = false;',
    '\tUse_Blockdev_Flag = false;\n'
    '\tUse_SMBIOS_Type0_Flag = false;\n'
    '\tSMBIOS_Vendor.clear();\n'
    '\tSMBIOS_Version.clear();\n'
    '\tSMBIOS_Date.clear();\n'
    '\tUse_SMBIOS_Type1_Flag = false;\n'
    '\tSMBIOS_Manufacturer.clear();\n'
    '\tSMBIOS_Product.clear();\n'
    '\tSMBIOS_Type1_Version.clear();\n'
    '\tSMBIOS_Serial.clear();\n'
    '\tSMBIOS_File.clear();\n'
    '\tFW_CFG_Lines.clear();\n'
    '\tAudiodev_Backend.clear();\n'
    '\tAudiodev_Timer_Period = 0;\n'
    '\tUse_NUMA_Memdev_Flag = false;\n'
    '\tICount.clear();\n'
    '\tSandbox.clear();',
    'init',
)

# operator==
must_replace(
    '\t\tthis->Use_Blockdev_Flag == vm.Use_Blockdev() &&',
    '\t\tthis->Use_Blockdev_Flag == vm.Use_Blockdev() &&\n'
    '\t\tthis->Use_SMBIOS_Type0_Flag == vm.Use_SMBIOS_Type0() &&\n'
    '\t\tthis->SMBIOS_Vendor == vm.Get_SMBIOS_Vendor() &&\n'
    '\t\tthis->SMBIOS_Version == vm.Get_SMBIOS_Version() &&\n'
    '\t\tthis->SMBIOS_Date == vm.Get_SMBIOS_Date() &&\n'
    '\t\tthis->Use_SMBIOS_Type1_Flag == vm.Use_SMBIOS_Type1() &&\n'
    '\t\tthis->SMBIOS_Manufacturer == vm.Get_SMBIOS_Manufacturer() &&\n'
    '\t\tthis->SMBIOS_Product == vm.Get_SMBIOS_Product() &&\n'
    '\t\tthis->SMBIOS_Type1_Version == vm.Get_SMBIOS_Type1_Version() &&\n'
    '\t\tthis->SMBIOS_Serial == vm.Get_SMBIOS_Serial() &&\n'
    '\t\tthis->SMBIOS_File == vm.Get_SMBIOS_File() &&\n'
    '\t\tthis->FW_CFG_Lines == vm.Get_FW_CFG_Lines() &&\n'
    '\t\tthis->Audiodev_Backend == vm.Get_Audiodev_Backend() &&\n'
    '\t\tthis->Audiodev_Timer_Period == vm.Get_Audiodev_Timer_Period() &&\n'
    '\t\tthis->Use_NUMA_Memdev_Flag == vm.Use_NUMA_Memdev() &&\n'
    '\t\tthis->ICount == vm.Get_ICount() &&\n'
    '\t\tthis->Sandbox == vm.Get_Sandbox() &&',
    'eq',
)

# operator=
must_replace(
    '\tUse_Blockdev_Flag = vm.Use_Blockdev();',
    '\tUse_Blockdev_Flag = vm.Use_Blockdev();\n'
    '\tUse_SMBIOS_Type0_Flag = vm.Use_SMBIOS_Type0();\n'
    '\tSMBIOS_Vendor = vm.Get_SMBIOS_Vendor();\n'
    '\tSMBIOS_Version = vm.Get_SMBIOS_Version();\n'
    '\tSMBIOS_Date = vm.Get_SMBIOS_Date();\n'
    '\tUse_SMBIOS_Type1_Flag = vm.Use_SMBIOS_Type1();\n'
    '\tSMBIOS_Manufacturer = vm.Get_SMBIOS_Manufacturer();\n'
    '\tSMBIOS_Product = vm.Get_SMBIOS_Product();\n'
    '\tSMBIOS_Type1_Version = vm.Get_SMBIOS_Type1_Version();\n'
    '\tSMBIOS_Serial = vm.Get_SMBIOS_Serial();\n'
    '\tSMBIOS_File = vm.Get_SMBIOS_File();\n'
    '\tFW_CFG_Lines = vm.Get_FW_CFG_Lines();\n'
    '\tAudiodev_Backend = vm.Get_Audiodev_Backend();\n'
    '\tAudiodev_Timer_Period = vm.Get_Audiodev_Timer_Period();\n'
    '\tUse_NUMA_Memdev_Flag = vm.Use_NUMA_Memdev();\n'
    '\tICount = vm.Get_ICount();\n'
    '\tSandbox = vm.Get_Sandbox();',
    'assign',
)

# XML save — after Use_Blockdev
save_old = '''\tDom_Element = New_Dom_Document.createElement( "Use_Blockdev" );
\tVM_Element.appendChild( Dom_Element );
\tDom_Text = New_Dom_Document.createTextNode( Use_Blockdev_Flag ? "true" : "false" );
\tDom_Element.appendChild( Dom_Text );'''

def xml_field(name, value_expr):
    return (
        f'\tDom_Element = New_Dom_Document.createElement( "{name}" );\n'
        f'\tVM_Element.appendChild( Dom_Element );\n'
        f'\tDom_Text = New_Dom_Document.createTextNode( {value_expr} );\n'
        f'\tDom_Element.appendChild( Dom_Text );\n'
    )

save_new = save_old + '\n' + ''.join([
    xml_field('Use_SMBIOS_Type0', 'Use_SMBIOS_Type0_Flag ? "true" : "false"'),
    xml_field('SMBIOS_Vendor', 'SMBIOS_Vendor'),
    xml_field('SMBIOS_Version', 'SMBIOS_Version'),
    xml_field('SMBIOS_Date', 'SMBIOS_Date'),
    xml_field('Use_SMBIOS_Type1', 'Use_SMBIOS_Type1_Flag ? "true" : "false"'),
    xml_field('SMBIOS_Manufacturer', 'SMBIOS_Manufacturer'),
    xml_field('SMBIOS_Product', 'SMBIOS_Product'),
    xml_field('SMBIOS_Type1_Version', 'SMBIOS_Type1_Version'),
    xml_field('SMBIOS_Serial', 'SMBIOS_Serial'),
    xml_field('SMBIOS_File', 'SMBIOS_File'),
    xml_field('FW_CFG_Lines', 'FW_CFG_Lines'),
    xml_field('Audiodev_Backend', 'Audiodev_Backend'),
    xml_field('Audiodev_Timer_Period', 'QString::number( Audiodev_Timer_Period )'),
    xml_field('Use_NUMA_Memdev', 'Use_NUMA_Memdev_Flag ? "true" : "false"'),
    xml_field('ICount', 'ICount'),
    xml_field('Sandbox', 'Sandbox'),
])
must_replace(save_old, save_new, 'save')

# XML load
load_old = '\t\t\tUse_Blockdev_Flag = ( Child_Element.firstChildElement( "Use_Blockdev" ).text() == "true" );'
load_new = load_old + '''
\t\t\tUse_SMBIOS_Type0_Flag = ( Child_Element.firstChildElement( "Use_SMBIOS_Type0" ).text() == "true" );
\t\t\tSMBIOS_Vendor = Child_Element.firstChildElement( "SMBIOS_Vendor" ).text();
\t\t\tSMBIOS_Version = Child_Element.firstChildElement( "SMBIOS_Version" ).text();
\t\t\tSMBIOS_Date = Child_Element.firstChildElement( "SMBIOS_Date" ).text();
\t\t\tUse_SMBIOS_Type1_Flag = ( Child_Element.firstChildElement( "Use_SMBIOS_Type1" ).text() == "true" );
\t\t\tSMBIOS_Manufacturer = Child_Element.firstChildElement( "SMBIOS_Manufacturer" ).text();
\t\t\tSMBIOS_Product = Child_Element.firstChildElement( "SMBIOS_Product" ).text();
\t\t\tSMBIOS_Type1_Version = Child_Element.firstChildElement( "SMBIOS_Type1_Version" ).text();
\t\t\tSMBIOS_Serial = Child_Element.firstChildElement( "SMBIOS_Serial" ).text();
\t\t\tSMBIOS_File = Child_Element.firstChildElement( "SMBIOS_File" ).text().trimmed();
\t\t\tFW_CFG_Lines = Child_Element.firstChildElement( "FW_CFG_Lines" ).text();
\t\t\tAudiodev_Backend = Child_Element.firstChildElement( "Audiodev_Backend" ).text().trimmed();
\t\t\t{
\t\t\t\tbool ok = false;
\t\t\t\tconst int tp = Child_Element.firstChildElement( "Audiodev_Timer_Period" ).text().toInt( &ok );
\t\t\t\tAudiodev_Timer_Period = ok ? tp : 0;
\t\t\t}
\t\t\tUse_NUMA_Memdev_Flag = ( Child_Element.firstChildElement( "Use_NUMA_Memdev" ).text() == "true" );
\t\t\tICount = Child_Element.firstChildElement( "ICount" ).text().trimmed();
\t\t\tSandbox = Child_Element.firstChildElement( "Sandbox" ).text().trimmed();'''
must_replace(load_old, load_new, 'load')

# NUMA emit — replace equal mem= with optional memdev=
numa_old = '''\t// NUMA (equal split of -m across nodes)
\tif( Use_NUMA_Flag && NUMA_Nodes >= 2 && Memory_Size > 0 )
\t{
\t\tconst int nodes = qMin( NUMA_Nodes, 8 );
\t\tconst int per = Memory_Size / nodes;
\t\tint rem = Memory_Size - ( per * nodes );
\t\tfor( int ni = 0; ni < nodes; ++ni )
\t\t{
\t\t\tint mb = per + ( ni == 0 ? rem : 0 );
\t\t\tArgs << "-numa" << QStringLiteral( "node,nodeid=%1,mem=%2" ).arg( ni ).arg( mb );
\t\t}
\t}'''

numa_new = '''\t// NUMA (equal split of -m across nodes)
\tif( Use_NUMA_Flag && NUMA_Nodes >= 2 && Memory_Size > 0 )
\t{
\t\tconst int nodes = qMin( NUMA_Nodes, 8 );
\t\tconst int per = Memory_Size / nodes;
\t\tint rem = Memory_Size - ( per * nodes );
\t\tfor( int ni = 0; ni < nodes; ++ni )
\t\t{
\t\t\tint mb = per + ( ni == 0 ? rem : 0 );
\t\t\tif( Use_NUMA_Memdev_Flag )
\t\t\t{
\t\t\t\tconst QString mid = QStringLiteral( "aqmem%1" ).arg( ni );
\t\t\t\tArgs << "-object" << QStringLiteral( "memory-backend-ram,id=%1,size=%2M" ).arg( mid ).arg( mb );
\t\t\t\tArgs << "-numa" << QStringLiteral( "node,nodeid=%1,memdev=%2" ).arg( ni ).arg( mid );
\t\t\t}
\t\t\telse
\t\t\t{
\t\t\t\tArgs << "-numa" << QStringLiteral( "node,nodeid=%1,mem=%2" ).arg( ni ).arg( mb );
\t\t\t}
\t\t}
\t}'''
must_replace(numa_old, numa_new, 'numa')

# Audiodev backend override
audio_old = '''\t\tArgs << "-audiodev" << ( audiodev_backend + ",id=snd0" );'''
audio_new = '''\t\tif( ! Audiodev_Backend.trimmed().isEmpty() )
\t\t\taudiodev_backend = Audiodev_Backend.trimmed();
\t\tQString audiodev_arg = audiodev_backend + ",id=snd0";
\t\tif( Audiodev_Timer_Period > 0 )
\t\t\taudiodev_arg += QStringLiteral( ",timer-period=%1" ).arg( Audiodev_Timer_Period );
\t\tArgs << "-audiodev" << audiodev_arg;'''
must_replace(audio_old, audio_new, 'audio')

# Chardev: extend missing backends before default: clear
char_old = '''\t\t\t\tcase VM::PR_vc:
\t\t\t\t\tcdev = QStringLiteral( "vc,id=" ) + cid;
\t\t\t\t\tif( ! params.isEmpty() ) cdev += QLatin1Char( ',' ) + params;
\t\t\t\t\tbreak;
\t\t\t\tdefault:
\t\t\t\t\tcdev.clear();
\t\t\t\t\tbreak;'''

char_new = '''\t\t\t\tcase VM::PR_vc:
\t\t\t\t\tcdev = QStringLiteral( "vc,id=" ) + cid;
\t\t\t\t\tif( ! params.isEmpty() ) cdev += QLatin1Char( ',' ) + params;
\t\t\t\t\tbreak;
\t\t\t\tcase VM::PR_udp:
\t\t\t\t\tcdev = QStringLiteral( "udp,id=" ) + cid;
\t\t\t\t\tif( ! params.isEmpty() )
\t\t\t\t\t{
\t\t\t\t\t\t// params: host:port[@localaddr:localport] or raw chardev props
\t\t\t\t\t\tif( params.contains( QLatin1Char( '=' ) ) )
\t\t\t\t\t\t\tcdev += QLatin1Char( ',' ) + params;
\t\t\t\t\t\telse
\t\t\t\t\t\t{
\t\t\t\t\t\t\tconst QString host = params.section( QLatin1Char( ':' ), 0, 0 );
\t\t\t\t\t\t\tconst QString port = params.section( QLatin1Char( ':' ), 1 );
\t\t\t\t\t\t\tcdev += QStringLiteral( ",host=" ) + host + QStringLiteral( ",port=" ) + port;
\t\t\t\t\t\t}
\t\t\t\t\t}
\t\t\t\t\tbreak;
\t\t\t\tcase VM::PR_telnet:
\t\t\t\t\tif( params.contains( QLatin1Char( ':' ) ) )
\t\t\t\t\t{
\t\t\t\t\t\tconst QString host = params.section( QLatin1Char( ':' ), 0, 0 );
\t\t\t\t\t\tconst QString port = params.section( QLatin1Char( ':' ), 1 );
\t\t\t\t\t\tcdev = QStringLiteral( "socket,id=" ) + cid + QStringLiteral( ",host=" )
\t\t\t\t\t\t       + host + QStringLiteral( ",port=" ) + port
\t\t\t\t\t\t       + QStringLiteral( ",server=on,wait=off,telnet=on" );
\t\t\t\t\t}
\t\t\t\t\telse
\t\t\t\t\t\tcdev = QStringLiteral( "socket,id=" ) + cid + QStringLiteral( ",host=,port=" )
\t\t\t\t\t\t       + params + QStringLiteral( ",server=on,wait=off,telnet=on" );
\t\t\t\t\tbreak;
\t\t\t\tcase VM::PR_none:
\t\t\t\t\tcdev = QStringLiteral( "null,id=" ) + cid;
\t\t\t\t\tbreak;
\t\t\t\tcase VM::PR_dev:
\t\t\t\tcase VM::PR_com:
\t\t\t\t\tcdev = QStringLiteral( "serial,id=" ) + cid + QStringLiteral( ",path=" ) + params;
\t\t\t\t\tbreak;
\t\t\t\tcase VM::PR_msmouse:
\t\t\t\t\tcdev = QStringLiteral( "msmouse,id=" ) + cid;
\t\t\t\t\tbreak;
\t\t\t\tcase VM::PR_braille:
\t\t\t\t\tcdev = QStringLiteral( "braille,id=" ) + cid;
\t\t\t\t\tbreak;
\t\t\t\tcase VM::PR_mon:
\t\t\t\t\t// Mux monitor onto this chardev after create — fall back to legacy
\t\t\t\t\tcdev.clear();
\t\t\t\t\tbreak;
\t\t\t\tdefault:
\t\t\t\t\tcdev.clear();
\t\t\t\t\tbreak;'''
must_replace(char_old, char_new, 'chardev')

# After TPM / before BIOS — emit SMBIOS, fw_cfg, icount, sandbox
# Also fix the Intel mac type=2 smbios? leave it.
bios_marker = '''\t// BIOS (SeaBIOS alternative / board firmware)
\tif( ! BIOS_File.trimmed().isEmpty() )'''

wave3_emit = '''\t// SMBIOS (qemu-doc)
\tif( ! SMBIOS_File.trimmed().isEmpty() )
\t{
\t\tif( Build_QEMU_Args_for_Script_Mode )
\t\t\tArgs << "-smbios" << "file=\\"" + SMBIOS_File + "\\"";
\t\telse
\t\t\tArgs << "-smbios" << QStringLiteral( "file=" ) + SMBIOS_File;
\t}
\tif( Use_SMBIOS_Type0_Flag )
\t{
\t\tQStringList parts;
\t\tparts << QStringLiteral( "type=0" );
\t\tif( ! SMBIOS_Vendor.trimmed().isEmpty() )
\t\t\tparts << QStringLiteral( "vendor=" ) + SMBIOS_Vendor.trimmed();
\t\tif( ! SMBIOS_Version.trimmed().isEmpty() )
\t\t\tparts << QStringLiteral( "version=" ) + SMBIOS_Version.trimmed();
\t\tif( ! SMBIOS_Date.trimmed().isEmpty() )
\t\t\tparts << QStringLiteral( "date=" ) + SMBIOS_Date.trimmed();
\t\tArgs << "-smbios" << parts.join( QLatin1Char( ',' ) );
\t}
\tif( Use_SMBIOS_Type1_Flag )
\t{
\t\tQStringList parts;
\t\tparts << QStringLiteral( "type=1" );
\t\tif( ! SMBIOS_Manufacturer.trimmed().isEmpty() )
\t\t\tparts << QStringLiteral( "manufacturer=" ) + SMBIOS_Manufacturer.trimmed();
\t\tif( ! SMBIOS_Product.trimmed().isEmpty() )
\t\t\tparts << QStringLiteral( "product=" ) + SMBIOS_Product.trimmed();
\t\tif( ! SMBIOS_Type1_Version.trimmed().isEmpty() )
\t\t\tparts << QStringLiteral( "version=" ) + SMBIOS_Type1_Version.trimmed();
\t\tif( ! SMBIOS_Serial.trimmed().isEmpty() )
\t\t\tparts << QStringLiteral( "serial=" ) + SMBIOS_Serial.trimmed();
\t\tif( ! UUID.trimmed().isEmpty() )
\t\t\tparts << QStringLiteral( "uuid=" ) + UUID.trimmed();
\t\tArgs << "-smbios" << parts.join( QLatin1Char( ',' ) );
\t}

\t// fw_cfg (one entry per line)
\t{
\t\tconst QStringList lines = FW_CFG_Lines.split( QLatin1Char( '\\n' ), QString::SkipEmptyParts );
\t\tfor( const QString &raw : lines )
\t\t{
\t\t\tconst QString line = raw.trimmed();
\t\t\tif( line.isEmpty() || line.startsWith( QLatin1Char( '#' ) ) ) continue;
\t\t\tArgs << "-fw_cfg" << line;
\t\t}
\t}

\t// Instruction counter / sandbox
\tif( ! ICount.trimmed().isEmpty() )
\t\tArgs << "-icount" << ICount.trimmed();
\tif( ! Sandbox.trimmed().isEmpty() )
\t\tArgs << "-sandbox" << Sandbox.trimmed();

''' + bios_marker

must_replace(bios_marker, wave3_emit, 'smbios_fw')

# getters/setters at end near Use_Blockdev
getters = '''
bool Virtual_Machine::Use_SMBIOS_Type0() const { return Use_SMBIOS_Type0_Flag; }
void Virtual_Machine::Use_SMBIOS_Type0( bool use ) { Use_SMBIOS_Type0_Flag = use; }
const QString &Virtual_Machine::Get_SMBIOS_Vendor() const { return SMBIOS_Vendor; }
void Virtual_Machine::Set_SMBIOS_Vendor( const QString &v ) { SMBIOS_Vendor = v; }
const QString &Virtual_Machine::Get_SMBIOS_Version() const { return SMBIOS_Version; }
void Virtual_Machine::Set_SMBIOS_Version( const QString &v ) { SMBIOS_Version = v; }
const QString &Virtual_Machine::Get_SMBIOS_Date() const { return SMBIOS_Date; }
void Virtual_Machine::Set_SMBIOS_Date( const QString &v ) { SMBIOS_Date = v; }
bool Virtual_Machine::Use_SMBIOS_Type1() const { return Use_SMBIOS_Type1_Flag; }
void Virtual_Machine::Use_SMBIOS_Type1( bool use ) { Use_SMBIOS_Type1_Flag = use; }
const QString &Virtual_Machine::Get_SMBIOS_Manufacturer() const { return SMBIOS_Manufacturer; }
void Virtual_Machine::Set_SMBIOS_Manufacturer( const QString &v ) { SMBIOS_Manufacturer = v; }
const QString &Virtual_Machine::Get_SMBIOS_Product() const { return SMBIOS_Product; }
void Virtual_Machine::Set_SMBIOS_Product( const QString &v ) { SMBIOS_Product = v; }
const QString &Virtual_Machine::Get_SMBIOS_Type1_Version() const { return SMBIOS_Type1_Version; }
void Virtual_Machine::Set_SMBIOS_Type1_Version( const QString &v ) { SMBIOS_Type1_Version = v; }
const QString &Virtual_Machine::Get_SMBIOS_Serial() const { return SMBIOS_Serial; }
void Virtual_Machine::Set_SMBIOS_Serial( const QString &v ) { SMBIOS_Serial = v; }
const QString &Virtual_Machine::Get_SMBIOS_File() const { return SMBIOS_File; }
void Virtual_Machine::Set_SMBIOS_File( const QString &path ) { SMBIOS_File = path.trimmed(); }
const QString &Virtual_Machine::Get_FW_CFG_Lines() const { return FW_CFG_Lines; }
void Virtual_Machine::Set_FW_CFG_Lines( const QString &lines ) { FW_CFG_Lines = lines; }
const QString &Virtual_Machine::Get_Audiodev_Backend() const { return Audiodev_Backend; }
void Virtual_Machine::Set_Audiodev_Backend( const QString &backend ) { Audiodev_Backend = backend.trimmed(); }
int Virtual_Machine::Get_Audiodev_Timer_Period() const { return Audiodev_Timer_Period; }
void Virtual_Machine::Set_Audiodev_Timer_Period( int us ) { Audiodev_Timer_Period = us < 0 ? 0 : us; }
bool Virtual_Machine::Use_NUMA_Memdev() const { return Use_NUMA_Memdev_Flag; }
void Virtual_Machine::Use_NUMA_Memdev( bool use ) { Use_NUMA_Memdev_Flag = use; }
const QString &Virtual_Machine::Get_ICount() const { return ICount; }
void Virtual_Machine::Set_ICount( const QString &icount ) { ICount = icount.trimmed(); }
const QString &Virtual_Machine::Get_Sandbox() const { return Sandbox; }
void Virtual_Machine::Set_Sandbox( const QString &sandbox ) { Sandbox = sandbox.trimmed(); }
'''

anchor = 'bool Virtual_Machine::Use_Blockdev() const { return Use_Blockdev_Flag; }\nvoid Virtual_Machine::Use_Blockdev( bool use ) { Use_Blockdev_Flag = use; }'
if 'Use_SMBIOS_Type0()' not in t:
    if anchor not in t:
        raise SystemExit('getters anchor missing')
    t = t.replace(anchor, anchor + getters, 1)

p.write_text(t, encoding='utf-8')
print('VM.cpp OK')
print('wave3 core done')
