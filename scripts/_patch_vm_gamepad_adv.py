# Patch VM.cpp for gamepad filter / emulate / nodefaults / RTC clock
from pathlib import Path

p = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\VM.cpp")
t = p.read_text(encoding="utf-8")

old = "\tthis->Pass_Through_Gamepads = vm.Use_Pass_Through_Gamepads();"
new = (
    "\tthis->Pass_Through_Gamepads = vm.Use_Pass_Through_Gamepads();\n"
    "\tthis->Gamepad_Filter_IDs = vm.Get_Gamepad_Filter_IDs();\n"
    "\tthis->Emulate_USB_Gamepad = vm.Use_Emulate_USB_Gamepad();\n"
    "\tthis->No_Defaults = vm.Use_No_Defaults();\n"
    "\tthis->RTC_Clock = vm.Get_RTC_Clock();"
)
assert t.count(old) >= 1
t = t.replace(old, new)

old2 = "\tPass_Through_Gamepads = false;"
new2 = (
    "\tPass_Through_Gamepads = false;\n"
    "\tGamepad_Filter_IDs.clear();\n"
    "\tEmulate_USB_Gamepad = false;\n"
    "\tNo_Defaults = false;\n"
    '\tRTC_Clock = QStringLiteral( "host" );'
)
assert old2 in t
t = t.replace(old2, new2, 1)

old3 = "\t\tthis->Pass_Through_Gamepads == vm.Use_Pass_Through_Gamepads() &&"
new3 = (
    "\t\tthis->Pass_Through_Gamepads == vm.Use_Pass_Through_Gamepads() &&\n"
    "\t\tthis->Gamepad_Filter_IDs == vm.Get_Gamepad_Filter_IDs() &&\n"
    "\t\tthis->Emulate_USB_Gamepad == vm.Use_Emulate_USB_Gamepad() &&\n"
    "\t\tthis->No_Defaults == vm.Use_No_Defaults() &&\n"
    "\t\tthis->RTC_Clock == vm.Get_RTC_Clock() &&"
)
assert old3 in t
t = t.replace(old3, new3, 1)

old4 = "\tPass_Through_Gamepads = vm.Use_Pass_Through_Gamepads();"
new4 = (
    "\tPass_Through_Gamepads = vm.Use_Pass_Through_Gamepads();\n"
    "\tGamepad_Filter_IDs = vm.Get_Gamepad_Filter_IDs();\n"
    "\tEmulate_USB_Gamepad = vm.Use_Emulate_USB_Gamepad();\n"
    "\tNo_Defaults = vm.Use_No_Defaults();\n"
    "\tRTC_Clock = vm.Get_RTC_Clock();"
)
assert old4 in t
t = t.replace(old4, new4)

save_old = (
    '\tDom_Element = New_Dom_Document.createElement( "Pass_Through_Gamepads" );\n'
    "\tVM_Element.appendChild( Dom_Element );\n"
    '\tDom_Text = New_Dom_Document.createTextNode( Pass_Through_Gamepads ? "true" : "false" );\n'
    "\tDom_Element.appendChild( Dom_Text );"
)
save_new = save_old + (
    "\n\n"
    '\tDom_Element = New_Dom_Document.createElement( "Emulate_USB_Gamepad" );\n'
    "\tVM_Element.appendChild( Dom_Element );\n"
    '\tDom_Text = New_Dom_Document.createTextNode( Emulate_USB_Gamepad ? "true" : "false" );\n'
    "\tDom_Element.appendChild( Dom_Text );\n\n"
    '\tDom_Element = New_Dom_Document.createElement( "No_Defaults" );\n'
    "\tVM_Element.appendChild( Dom_Element );\n"
    '\tDom_Text = New_Dom_Document.createTextNode( No_Defaults ? "true" : "false" );\n'
    "\tDom_Element.appendChild( Dom_Text );\n\n"
    '\tDom_Element = New_Dom_Document.createElement( "RTC_Clock" );\n'
    "\tVM_Element.appendChild( Dom_Element );\n"
    '\tDom_Text = New_Dom_Document.createTextNode( RTC_Clock.isEmpty() ? "host" : RTC_Clock );\n'
    "\tDom_Element.appendChild( Dom_Text );\n\n"
    '\tDom_Element = New_Dom_Document.createElement( "Gamepad_Filter_IDs" );\n'
    "\tVM_Element.appendChild( Dom_Element );\n"
    '\tDom_Text = New_Dom_Document.createTextNode( Gamepad_Filter_IDs.join( "," ) );\n'
    "\tDom_Element.appendChild( Dom_Text );"
)
assert save_old in t
t = t.replace(save_old, save_new, 1)

load_old = (
    '\t\t\tPass_Through_Gamepads = ( Child_Element.firstChildElement( "Pass_Through_Gamepads" ).text() == "true" );'
)
load_new = (
    load_old
    + "\n"
    + '\t\t\tEmulate_USB_Gamepad = ( Child_Element.firstChildElement( "Emulate_USB_Gamepad" ).text() == "true" );\n'
    + '\t\t\tNo_Defaults = ( Child_Element.firstChildElement( "No_Defaults" ).text() == "true" );\n'
    + "\t\t\t{\n"
    + '\t\t\t\tconst QString clk = Child_Element.firstChildElement( "RTC_Clock" ).text().trimmed().toLower();\n'
    + '\t\t\t\tRTC_Clock = clk.isEmpty() ? QStringLiteral( "host" ) : clk;\n'
    + "\t\t\t}\n"
    + "\t\t\t{\n"
    + '\t\t\t\tconst QString filt = Child_Element.firstChildElement( "Gamepad_Filter_IDs" ).text().trimmed();\n'
    + "\t\t\t\tGamepad_Filter_IDs = filt.isEmpty() ? QStringList() : filt.split( QLatin1Char( ',' ), QString::SkipEmptyParts );\n"
    + "\t\t\t}"
)
assert load_old in t
t = t.replace(load_old, load_new, 1)

get_old = (
    "void Virtual_Machine::Use_Pass_Through_Gamepads( bool use )\n"
    "{\n"
    "\tPass_Through_Gamepads = use;\n"
    "}"
)
get_new = get_old + """

const QStringList &Virtual_Machine::Get_Gamepad_Filter_IDs() const
{
	return Gamepad_Filter_IDs;
}

void Virtual_Machine::Set_Gamepad_Filter_IDs( const QStringList &ids )
{
	Gamepad_Filter_IDs = ids;
}

bool Virtual_Machine::Use_Emulate_USB_Gamepad() const
{
	return Emulate_USB_Gamepad;
}

void Virtual_Machine::Use_Emulate_USB_Gamepad( bool use )
{
	Emulate_USB_Gamepad = use;
}

bool Virtual_Machine::Use_No_Defaults() const
{
	return No_Defaults;
}

void Virtual_Machine::Use_No_Defaults( bool use )
{
	No_Defaults = use;
}

const QString &Virtual_Machine::Get_RTC_Clock() const
{
	return RTC_Clock;
}

void Virtual_Machine::Set_RTC_Clock( const QString &clock )
{
	RTC_Clock = clock.trimmed().toLower();
	if( RTC_Clock.isEmpty() )
		RTC_Clock = QStringLiteral( "host" );
}
"""
assert get_old in t
t = t.replace(get_old, get_new, 1)

# Bridge case in Build_QEMU_Args
bridge_old = (
    "                    case VM::Net_Mode_Native_Chanel:\n"
    '\t\t\t\t\t\tnic_str += "channel";\n'
    "\t\t\t\t\t\tu_port_dev = true;\n"
    "\t\t\t\t\t\tbreak;\n"
    "\t\t\t\t\t\t\n"
    "\t\t\t\t\t// -net tap[,vlan=n][,name=str][,fd=h][,ifname=name][,script=file][,downscript=dfile]\n"
)
bridge_new = (
    "                    case VM::Net_Mode_Native_Chanel:\n"
    '\t\t\t\t\t\tnic_str += "channel";\n'
    "\t\t\t\t\t\tu_port_dev = true;\n"
    "\t\t\t\t\t\tbreak;\n"
    "\n"
    "\t\t\t\t\t// -net bridge[,vlan=n][,name=str][,br=bridge][,helper=helper]\n"
    "                    case VM::Net_Mode_Native_Bridge:\n"
    '\t\t\t\t\t\tnic_str += "bridge";\n'
    "\t\t\t\t\t\tu_vlan = u_name = u_bridge = u_helper = true;\n"
    "\t\t\t\t\t\tbreak;\n"
    "\t\t\t\t\t\t\n"
    "\t\t\t\t\t// -net tap[,vlan=n][,name=str][,fd=h][,ifname=name][,script=file][,downscript=dfile]\n"
)
if bridge_old not in t:
    raise SystemExit("bridge insert point missing")
t = t.replace(bridge_old, bridge_new, 1)

# RTC clock= in rtc_list
rtc_old = """    rtc_list << "-rtc";
	if ( Local_Time )
		rtc_list << "base=localtime";
	else if ( Start_Date )
		rtc_list << "base=" + Start_DateTime.toString( "yyyy-MM-ddTHH:mm:ss" ); // QEMU Format
    else
        rtc_list << "base=utc";
	if( Current_Emulator_Devices.PSO_RTC_TD_Hack && RTC_TD_Hack )
	    rtc_list.last() += ",driftfix=slew";

    Args << rtc_list;"""
rtc_new = """    rtc_list << "-rtc";
	if ( Local_Time )
		rtc_list << "base=localtime";
	else if ( Start_Date )
		rtc_list << "base=" + Start_DateTime.toString( "yyyy-MM-ddTHH:mm:ss" ); // QEMU Format
    else
        rtc_list << "base=utc";
	{
		const QString clk = RTC_Clock.trimmed().toLower();
		if( clk == QLatin1String( "vm" ) || clk == QLatin1String( "rt" ) )
			rtc_list.last() += ",clock=" + clk;
		else if( ! clk.isEmpty() && clk != QLatin1String( "host" ) )
			rtc_list.last() += ",clock=" + clk;
	}
	if( Current_Emulator_Devices.PSO_RTC_TD_Hack && RTC_TD_Hack )
	    rtc_list.last() += ",driftfix=slew";

    Args << rtc_list;"""
assert rtc_old in t
t = t.replace(rtc_old, rtc_new, 1)

# nodefaults early after Args created
nd_old = """	QStringList Args;

	// Introducing a second argument collector for the Storage-part,"""
nd_new = """	QStringList Args;

	if( No_Defaults )
		Args << "-nodefaults";

	// Introducing a second argument collector for the Storage-part,"""
assert nd_old in t
t = t.replace(nd_old, nd_new, 1)

# gamepad filter in Build_QEMU_Args
filt_old = """			if( Pass_Through_Gamepads )
			{
				QList<VM_USB> pads = System_Info::Get_Host_Gamepads();
				QSet<QString> already;
				for( int ux = 0; ux < ports.count(); ++ux )
				{
					already.insert( ports[ux].Get_Vendor_ID().toLower() + QLatin1Char( ':' ) +
					                ports[ux].Get_Product_ID().toLower() );
				}
				for( int pi = 0; pi < pads.count(); ++pi )
				{
					const QString key = pads[pi].Get_Vendor_ID().toLower() + QLatin1Char( ':' ) +
					                    pads[pi].Get_Product_ID().toLower();
					if( already.contains( key ) )
						continue;
					already.insert( key );
					ports.append( pads[pi] );
				}
			}"""
filt_new = """			if( Pass_Through_Gamepads )
			{
				QList<VM_USB> pads = System_Info::Get_Host_Gamepads();
				QSet<QString> already;
				QSet<QString> allow;
				for( int fi = 0; fi < Gamepad_Filter_IDs.count(); ++fi )
					allow.insert( Gamepad_Filter_IDs[fi].trimmed().toLower() );
				for( int ux = 0; ux < ports.count(); ++ux )
				{
					already.insert( ports[ux].Get_Vendor_ID().toLower() + QLatin1Char( ':' ) +
					                ports[ux].Get_Product_ID().toLower() );
				}
				for( int pi = 0; pi < pads.count(); ++pi )
				{
					const QString key = pads[pi].Get_Vendor_ID().toLower() + QLatin1Char( ':' ) +
					                    pads[pi].Get_Product_ID().toLower();
					if( ! allow.isEmpty() && ! allow.contains( key ) )
						continue;
					if( already.contains( key ) )
						continue;
					already.insert( key );
					ports.append( pads[pi] );
				}
			}"""
assert filt_old in t
t = t.replace(filt_old, filt_new, 1)

# After USB block, or with USB: emulate usb-gamepad
# Find a good insertion point after the USB ports section ends (before // Other Tab)
emu_marker = "\t// Other Tab\n\tif( Linux_Boot )"
emu_insert = """\t// Emulated USB gamepad (QEMU usb-gamepad — maps host joystick when supported)
	if( Emulate_USB_Gamepad )
	{
		if( ! Args.contains( QStringLiteral( "-usb" ) ) )
			Args << "-usb";
		Args << "-device" << "usb-gamepad";
	}

""" + emu_marker
if emu_marker not in t:
    raise SystemExit("emu marker missing")
t = t.replace(emu_marker, emu_insert, 1)

# USB ports condition also for Emulate
usb_cond_old = "\t\tif( USB_Ports.count() > 0 || Pass_Through_Gamepads )"
usb_cond_new = "\t\tif( USB_Ports.count() > 0 || Pass_Through_Gamepads || Emulate_USB_Gamepad )"
t = t.replace(usb_cond_old, usb_cond_new, 1)

p.write_text(t, encoding="utf-8")
print("patched VM.cpp OK")
