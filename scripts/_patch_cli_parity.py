# Add CLI-parity VM fields and Build_QEMU_Args wiring
from pathlib import Path

vm_h = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\VM.h")
h = vm_h.read_text(encoding="utf-8")

api = '''
		/** QEMU -display backend: empty=policy, sdl|gtk|none|curses|nographic */
		const QString &Get_Display_Backend() const;
		void Set_Display_Backend( const QString &backend );

		/** Emit -object iothread and attach to virtio block devices. */
		bool Use_IOThread() const;
		void Use_IOThread( bool use );

		/** -mem-path / optional -mem-prealloc (hugepages / pinned RAM). */
		const QString &Get_Mem_Path() const;
		void Set_Mem_Path( const QString &path );
		bool Use_Mem_Prealloc() const;
		void Use_Mem_Prealloc( bool use );

		/** Guest UUID (-uuid). Empty = omit. */
		const QString &Get_UUID() const;
		void Set_UUID( const QString &uuid );

		/** Legacy BIOS image (-bios). Empty = SeaBIOS/default. */
		const QString &Get_BIOS_File() const;
		void Set_BIOS_File( const QString &path );

		/** Extra -machine properties (e.g. gic-version=host,highmem=on). */
		const QString &Get_Machine_Extra_Props() const;
		void Set_Machine_Extra_Props( const QString &props );

		/** Prefer modern -netdev + -device over legacy -net for native cards. */
		bool Use_Modern_Netdev() const;
		void Use_Modern_Netdev( bool use );

'''

needle = '\t\t/** Guest RTC clock source: host | vm | rt (empty = host). */\n\t\tconst QString &Get_RTC_Clock() const;\n\t\tvoid Set_RTC_Clock( const QString &clock );\n'
if api.strip() not in h:
    if needle not in h:
        raise SystemExit('API insert point missing')
    h = h.replace(needle, needle + '\n' + api)

members = '''\t\tQString RTC_Clock; // host | vm | rt
\t\tQString Display_Backend; // empty | sdl | gtk | none | curses | nographic
\t\tbool Use_IOThread_Flag;
\t\tQString Mem_Path;
\t\tbool Mem_Prealloc;
\t\tQString UUID;
\t\tQString BIOS_File;
\t\tQString Machine_Extra_Props;
\t\tbool Modern_Netdev;
\t\tQString Display_Window_Mode; // auto | embedded | native'''

old_m = '\t\tQString RTC_Clock; // host | vm | rt\n\t\tQString Display_Window_Mode; // auto | embedded | native'
if old_m not in h:
    raise SystemExit('members missing')
h = h.replace(old_m, members)
vm_h.write_text(h, encoding='utf-8')
print('VM.h OK')

# --- VM.cpp ---
p = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\VM.cpp")
t = p.read_text(encoding="utf-8")

def insert_after(old, new, count=1):
    global t
    if old not in t:
        raise SystemExit('missing: ' + old[:60])
    t = t.replace(old, old + new, count)

# copy ctor this->
insert_after(
    '\tthis->RTC_Clock = vm.Get_RTC_Clock();',
    '\n\tthis->Display_Backend = vm.Get_Display_Backend();\n'
    '\tthis->Use_IOThread_Flag = vm.Use_IOThread();\n'
    '\tthis->Mem_Path = vm.Get_Mem_Path();\n'
    '\tthis->Mem_Prealloc = vm.Use_Mem_Prealloc();\n'
    '\tthis->UUID = vm.Get_UUID();\n'
    '\tthis->BIOS_File = vm.Get_BIOS_File();\n'
    '\tthis->Machine_Extra_Props = vm.Get_Machine_Extra_Props();\n'
    '\tthis->Modern_Netdev = vm.Use_Modern_Netdev();',
)

# init
insert_after(
    '\tRTC_Clock = QStringLiteral( "host" );',
    '\n\tDisplay_Backend.clear();\n'
    '\tUse_IOThread_Flag = false;\n'
    '\tMem_Path.clear();\n'
    '\tMem_Prealloc = false;\n'
    '\tUUID.clear();\n'
    '\tBIOS_File.clear();\n'
    '\tMachine_Extra_Props.clear();\n'
    '\tModern_Netdev = true;',
)

# compare
insert_after(
    '\t\tthis->RTC_Clock == vm.Get_RTC_Clock() &&',
    '\n\t\tthis->Display_Backend == vm.Get_Display_Backend() &&\n'
    '\t\tthis->Use_IOThread_Flag == vm.Use_IOThread() &&\n'
    '\t\tthis->Mem_Path == vm.Get_Mem_Path() &&\n'
    '\t\tthis->Mem_Prealloc == vm.Use_Mem_Prealloc() &&\n'
    '\t\tthis->UUID == vm.Get_UUID() &&\n'
    '\t\tthis->BIOS_File == vm.Get_BIOS_File() &&\n'
    '\t\tthis->Machine_Extra_Props == vm.Get_Machine_Extra_Props() &&\n'
    '\t\tthis->Modern_Netdev == vm.Use_Modern_Netdev() &&',
)

# operator=
insert_after(
    '\tRTC_Clock = vm.Get_RTC_Clock();',
    '\n\tDisplay_Backend = vm.Get_Display_Backend();\n'
    '\tUse_IOThread_Flag = vm.Use_IOThread();\n'
    '\tMem_Path = vm.Get_Mem_Path();\n'
    '\tMem_Prealloc = vm.Use_Mem_Prealloc();\n'
    '\tUUID = vm.Get_UUID();\n'
    '\tBIOS_File = vm.Get_BIOS_File();\n'
    '\tMachine_Extra_Props = vm.Get_Machine_Extra_Props();\n'
    '\tModern_Netdev = vm.Use_Modern_Netdev();',
)

# XML save
insert_after(
    '\tDom_Text = New_Dom_Document.createTextNode( Gamepad_Filter_IDs.join( "," ) );\n'
    '\tDom_Element.appendChild( Dom_Text );',
    '\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Display_Backend" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Display_Backend );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Use_IOThread" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Use_IOThread_Flag ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Mem_Path" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Mem_Path );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Mem_Prealloc" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Mem_Prealloc ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "UUID" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( UUID );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "BIOS_File" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( BIOS_File );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Machine_Extra_Props" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Machine_Extra_Props );\n'
    '\tDom_Element.appendChild( Dom_Text );\n\n'
    '\tDom_Element = New_Dom_Document.createElement( "Modern_Netdev" );\n'
    '\tVM_Element.appendChild( Dom_Element );\n'
    '\tDom_Text = New_Dom_Document.createTextNode( Modern_Netdev ? "true" : "false" );\n'
    '\tDom_Element.appendChild( Dom_Text );',
)

# XML load
insert_after(
    '\t\t\t\tGamepad_Filter_IDs = filt.isEmpty() ? QStringList() : filt.split( QLatin1Char( \',\' ), QString::SkipEmptyParts );\n'
    '\t\t\t}',
    '\n'
    '\t\t\tDisplay_Backend = Child_Element.firstChildElement( "Display_Backend" ).text().trimmed().toLower();\n'
    '\t\t\tUse_IOThread_Flag = ( Child_Element.firstChildElement( "Use_IOThread" ).text() == "true" );\n'
    '\t\t\tMem_Path = Child_Element.firstChildElement( "Mem_Path" ).text();\n'
    '\t\t\tMem_Prealloc = ( Child_Element.firstChildElement( "Mem_Prealloc" ).text() == "true" );\n'
    '\t\t\tUUID = Child_Element.firstChildElement( "UUID" ).text().trimmed();\n'
    '\t\t\tBIOS_File = Child_Element.firstChildElement( "BIOS_File" ).text();\n'
    '\t\t\tMachine_Extra_Props = Child_Element.firstChildElement( "Machine_Extra_Props" ).text().trimmed();\n'
    '\t\t\t{\n'
    '\t\t\t\tconst QString mn = Child_Element.firstChildElement( "Modern_Netdev" ).text();\n'
    '\t\t\t\tModern_Netdev = ( mn.isEmpty() || mn == "true" );\n'
    '\t\t\t}',
)

# getters after Set_RTC_Clock
get_old = '''void Virtual_Machine::Set_RTC_Clock( const QString &clock )
{
	RTC_Clock = clock.trimmed().toLower();
	if( RTC_Clock.isEmpty() )
		RTC_Clock = QStringLiteral( "host" );
}
'''
get_new = get_old + '''
const QString &Virtual_Machine::Get_Display_Backend() const
{
	return Display_Backend;
}

void Virtual_Machine::Set_Display_Backend( const QString &backend )
{
	Display_Backend = backend.trimmed().toLower();
}

bool Virtual_Machine::Use_IOThread() const
{
	return Use_IOThread_Flag;
}

void Virtual_Machine::Use_IOThread( bool use )
{
	Use_IOThread_Flag = use;
}

const QString &Virtual_Machine::Get_Mem_Path() const
{
	return Mem_Path;
}

void Virtual_Machine::Set_Mem_Path( const QString &path )
{
	Mem_Path = path;
}

bool Virtual_Machine::Use_Mem_Prealloc() const
{
	return Mem_Prealloc;
}

void Virtual_Machine::Use_Mem_Prealloc( bool use )
{
	Mem_Prealloc = use;
}

const QString &Virtual_Machine::Get_UUID() const
{
	return UUID;
}

void Virtual_Machine::Set_UUID( const QString &uuid )
{
	UUID = uuid.trimmed();
}

const QString &Virtual_Machine::Get_BIOS_File() const
{
	return BIOS_File;
}

void Virtual_Machine::Set_BIOS_File( const QString &path )
{
	BIOS_File = path;
}

const QString &Virtual_Machine::Get_Machine_Extra_Props() const
{
	return Machine_Extra_Props;
}

void Virtual_Machine::Set_Machine_Extra_Props( const QString &props )
{
	Machine_Extra_Props = props.trimmed();
}

bool Virtual_Machine::Use_Modern_Netdev() const
{
	return Modern_Netdev;
}

void Virtual_Machine::Use_Modern_Netdev( bool use )
{
	Modern_Netdev = use;
}
'''
if get_old not in t:
    raise SystemExit('Set_RTC_Clock missing')
t = t.replace(get_old, get_new, 1)

# iothread object early after nodefaults
t = t.replace(
    '\tif( No_Defaults )\n\t\tArgs << "-nodefaults";\n',
    '\tif( No_Defaults )\n\t\tArgs << "-nodefaults";\n\n'
    '\tif( Use_IOThread_Flag )\n'
    '\t\tArgs << "-object" << "iothread,id=aq-iothread0";\n',
    1,
)

# mem-path after -m
# Find Args << "-m" pattern - look for memory emission
import re
m = re.search(r'(Args << "-m".*?\n)', t)
# Better: after memory size is set
mem_marker = None
for cand in [
    '\tArgs << "-m" << QString::number( Memory_Size );\n',
    '\t\tArgs << "-m" << QString::number( Get_Memory_Size() );\n',
]:
    if cand in t:
        mem_marker = cand
        break
# search more loosely
idx = t.find('Args << "-m"')
if idx < 0:
    raise SystemExit('no -m')
# find end of that statement line and maybe next few related lines
line_end = t.find('\n', idx)
# include following lines that are still part of memory until blank or other
chunk = t[idx:idx+400]
print('MEM CHUNK:', repr(chunk[:200]))

p.write_text(t, encoding='utf-8')
print('VM.cpp partial OK — continue mem/display/netdev in next script')
