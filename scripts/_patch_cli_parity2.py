from pathlib import Path

p = Path(r"c:\Users\chron\CURSOR-PROJECTS\aqemu\src\VM.cpp")
t = p.read_text(encoding="utf-8")

# mem-path / prealloc / uuid after -m
old = '\tArgs << "-m" << QString::number( Memory_Size, 10 );\n\t\n\t// full screen'
new = '''\tArgs << "-m" << QString::number( Memory_Size, 10 );
	if( ! Mem_Path.trimmed().isEmpty() )
	{
		if( Build_QEMU_Args_for_Script_Mode )
			Args << "-mem-path" << "\\"" + Mem_Path + "\\"";
		else
			Args << "-mem-path" << Mem_Path;
		if( Mem_Prealloc )
			Args << "-mem-prealloc";
	}
	if( ! UUID.trimmed().isEmpty() )
		Args << "-uuid" << UUID.trimmed();
	
	// full screen'''
if old not in t:
    raise SystemExit('mem insert fail')
t = t.replace(old, new, 1)

# BIOS before or after UEFI - after name is fine; put before SPICE
old = '\t// SPICE\n\t// FIXME. VNC and SPICE together?'
new = '''\t// BIOS (SeaBIOS alternative / board firmware)
	if( ! BIOS_File.trimmed().isEmpty() )
	{
		if( Build_QEMU_Args_for_Script_Mode )
			Args << "-bios" << "\\"" + BIOS_File + "\\"";
		else
			Args << "-bios" << BIOS_File;
	}

	// SPICE
	// FIXME. VNC and SPICE together?'''
if old not in t:
    raise SystemExit('bios insert fail')
t = t.replace(old, new, 1)

# Display backend override: before embedded_session display logic, compute force
# Insert right before `if( embedded_session )` for display section - the one with -display none
old = '''\tif( embedded_session )
	{
		// Headless: no QEMU SDL/GTK chrome ? AQEMU owns the window
		Args << "-display" << "none";'''
new = '''\tconst QString disp_backend = Display_Backend.trimmed().toLower();
	const bool force_nographic = ( disp_backend == QLatin1String( "nographic" ) );
	if( force_nographic )
	{
		Args << "-nographic";
	}
	else if( embedded_session )
	{
		// Headless: no QEMU SDL/GTK chrome ? AQEMU owns the window
		Args << "-display" << "none";'''
if old not in t:
    raise SystemExit('display insert fail')
t = t.replace(old, new, 1)

# Fix native_vga_window branch to honour explicit backend
old = '''\telse if( native_vga_window )
	{
		QString system_name = Current_Emulator_Devices.System.QEMU_Name;
		if( system_name.isEmpty() )
			system_name = Computer_Type;
		const QString preferred = Get_Current_Emulator_Binary_Path( system_name );
		const QString found = AQ_Find_QEMU_Binary_With_Native_Display( system_name, preferred );
		QString disp = AQ_QEMU_Pick_Native_Display( found );
		if( disp.isEmpty() )
			disp = QStringLiteral( "sdl" );
		Args << "-display" << disp;
	}'''
new = '''\telse if( native_vga_window ||
	         ( ! disp_backend.isEmpty() &&
	           disp_backend != QLatin1String( "nographic" ) &&
	           ! SPICE.Use_SPICE() && ! VNC ) )
	{
		QString disp = disp_backend;
		if( disp.isEmpty() || disp == QLatin1String( "auto" ) || disp == QLatin1String( "default" ) )
		{
			QString system_name = Current_Emulator_Devices.System.QEMU_Name;
			if( system_name.isEmpty() )
				system_name = Computer_Type;
			const QString preferred = Get_Current_Emulator_Binary_Path( system_name );
			const QString found = AQ_Find_QEMU_Binary_With_Native_Display( system_name, preferred );
			disp = AQ_QEMU_Pick_Native_Display( found );
			if( disp.isEmpty() )
				disp = QStringLiteral( "sdl" );
		}
		if( disp == QLatin1String( "none" ) || disp == QLatin1String( "curses" ) ||
		    disp == QLatin1String( "sdl" ) || disp == QLatin1String( "gtk" ) ||
		    disp == QLatin1String( "egl-headless" ) || disp == QLatin1String( "spice-app" ) )
			Args << "-display" << disp;
		else
			Args << "-display" << QStringLiteral( "sdl" );
	}'''
if old not in t:
    raise SystemExit('native display replace fail')
t = t.replace(old, new, 1)

# Machine extra props — find machine arg building. Search for Set_Machine or Args << "-machine"
# Append Machine_Extra_Props when emitting -machine
# Common pattern: Args << "-machine" << something
# We'll append after machine string is built. Look for highmem / gic patterns.

# iothread on virtio-blk device lines
old = '''\t\targs << "-device" << With_Bootindex(
			"virtio-blk-pci,drive=" + vsname, boot_idx );'''
new = '''\t\tQString vblk = QStringLiteral( "virtio-blk-pci,drive=" ) + vsname;
		if( Use_IOThread_Flag )
			vblk += QStringLiteral( ",iothread=aq-iothread0" );
		args << "-device" << With_Bootindex( vblk, boot_idx );'''
if old not in t:
    # try tabs
    print('virtio-blk pattern not exact')
else:
    t = t.replace(old, new, 1)

# Also for NVMe? skip. For scsi virtio maybe later.

# Network path URI auto format=raw when iscsi/rbd — in Build_Native_Device_Args when file path set
old = '''\t// Cache ? on Windows, cache=none with qcow2 fails ("Image is not in qcow2 format")
	if( device.Use_Cache() )'''
# Before cache, if file looks like network protocol, ensure format
uri_patch = '''\t// Network block protocols (qemu-doc: iscsi://, rbd:, nbd:, gluster://, ssh://)
	{
		const QString fp = device.Get_File_Path();
		const bool net_proto =
			fp.startsWith( QLatin1String( "iscsi://" ), Qt::CaseInsensitive ) ||
			fp.startsWith( QLatin1String( "rbd:" ), Qt::CaseInsensitive ) ||
			fp.startsWith( QLatin1String( "nbd:" ), Qt::CaseInsensitive ) ||
			fp.startsWith( QLatin1String( "nbd+" ), Qt::CaseInsensitive ) ||
			fp.startsWith( QLatin1String( "gluster" ), Qt::CaseInsensitive ) ||
			fp.startsWith( QLatin1String( "ssh://" ), Qt::CaseInsensitive );
		if( net_proto && ! device.Use_Format() )
			opt << "format=raw";
	}

	// Cache ? on Windows, cache=none with qcow2 fails ("Image is not in qcow2 format")
	if( device.Use_Cache() )'''
if old not in t:
    raise SystemExit('cache marker missing')
t = t.replace(old, uri_patch, 1)

# Modern netdev for native network cards — replace the final Args << "-net" << nic_str for User/TAP/Bridge
# This is complex. Simpler approach: after the whole native loop builds nic_str and does Args << "-net",
# change the emit section.

old_emit = '''\t\t\t\t// Add to Args
\t\t\t\tArgs << "-net";
\t\t\t\tArgs << nic_str;
\t\t\t}'''
new_emit = '''\t\t\t\t// Add to Args — modern -netdev+device when enabled (qemu-doc § Network)
				const VM::Network_Mode ntype = Network_Cards_Nativ[nc].Get_Network_Type();
				const bool can_modern =
					Modern_Netdev &&
					( ntype == VM::Net_Mode_Native_User ||
					  ntype == VM::Net_Mode_Native_TAP ||
					  ntype == VM::Net_Mode_Native_Bridge );
				if( can_modern )
				{
					const QString nid = QStringLiteral( "aqnet%1" ).arg( nc );
					QString nd = nic_str;
					// nic_str is like "user,..." or "tap,..." without type prefix issues — already has type
					const int comma = nd.indexOf( QLatin1Char( ',' ) );
					QString ntype_s = comma > 0 ? nd.left( comma ) : nd;
					QString rest = comma > 0 ? nd.mid( comma + 1 ) : QString();
					QString netdev = ntype_s + QStringLiteral( ",id=" ) + nid;
					if( ! rest.isEmpty() )
					{
						// strip NIC-only props that don't belong on netdev
						QStringList parts = rest.split( QLatin1Char( ',' ) );
						QStringList keep;
						for( int pi = 0; pi < parts.count(); ++pi )
						{
							const QString &p = parts[pi];
							if( p.startsWith( QLatin1String( "model=" ) ) ||
							    p.startsWith( QLatin1String( "macaddr=" ) ) ||
							    p.startsWith( QLatin1String( "vectors=" ) ) ||
							    p.startsWith( QLatin1String( "addr=" ) ) )
								continue;
							if( p.startsWith( QLatin1String( "vlan=" ) ) )
								continue; // vlan is legacy -net
							keep << p;
						}
						if( ! keep.isEmpty() )
							netdev += QLatin1Char( ',' ) + keep.join( QLatin1Char( ',' ) );
					}
					Args << "-netdev" << netdev;

					QString model = Network_Cards_Nativ[nc].Get_Card_Model();
					if( model.isEmpty() )
						model = QStringLiteral( "virtio-net-pci" );
					if( model == QLatin1String( "virtio" ) || model == QLatin1String( "virtio-net" ) )
						model = QStringLiteral( "virtio-net-pci" );
					QString nic_dev = model + QStringLiteral( ",netdev=" ) + nid;
					if( Network_Cards_Nativ[nc].Use_MAC_Address() &&
					    ! Network_Cards_Nativ[nc].Get_MAC_Address().isEmpty() )
						nic_dev += QStringLiteral( ",mac=" ) + Network_Cards_Nativ[nc].Get_MAC_Address();
					Args << "-device" << nic_dev;
				}
				else
				{
					Args << "-net";
					Args << nic_str;
				}
			}'''
if old_emit not in t:
    raise SystemExit('net emit missing')
t = t.replace(old_emit, new_emit, 1)

# Machine extra props: find where -machine is added with comma-joined props
# Search for "Args << \"-machine\""
import re
matches = list(re.finditer(r'Args << "-machine" << ([^;]+);', t))
print('machine emits', len(matches))
for m in matches[:5]:
    print(' ', m.group(0)[:120])

# Append Machine_Extra_Props into machine string builders is hard.
# Simpler: after all machine args, if Machine_Extra_Props set and Args contains -machine, append
# Or emit additional: many QEMU allow -machine prop again... Actually second -machine merges in some versions.
# Safest: post-process Args list before return? Too late.
# Insert after machine emission block a helper that finds -machine value and appends.

# Find Prefer_Accelerator / machine building around line with "accel="
# After building machine for x86, look for pattern like:
# Args << "-machine" << mach;
# We'll add a function call at end of Build_QEMU_Args before Additional Args:

marker = '\t// Other Tab\n\tif( Linux_Boot )'
# Actually Additional Args is near end. Find:
add_mark = None
for cand in ['\tif( ! Get_Additional_Args().isEmpty()', '\tif( ! Additional_Args.isEmpty()', 'Get_Additional_Args()']:
    pass

# Search Additional Args section
idx = t.find('Additional_Args')
print('Additional_Args idx samples')
for m in re.finditer(r'.{0,40}Additional_Args.{0,80}', t[8000:9000] if False else t):
    if 'Build_QEMU' in m.group(0) or 'Args <<' in m.group(0) or 'Only_User' in m.group(0):
        print(m.group(0)[:120])
        break

# Find the block that appends additional args
pat = re.search(r'(\tif\( Get_Only_User_Args\(\) \).*?\n(?:.*\n){0,25})', t)
# Simpler string
old_add = None
for cand in [
    '\tif( Only_User_Args )\n',
    '\tif( Get_Only_User_Args() )\n',
]:
    if cand in t:
        old_add = cand
        break
print('only_user', old_add)

p.write_text(t, encoding='utf-8')
print('wrote')
