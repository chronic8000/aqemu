#ifndef WHPX_HOST_H
#define WHPX_HOST_H

#include <cstdint>
#include <cstring>

/*
 * Microsoft WHV_CAPABILITY_FEATURES for an ARM64 host.
 * Arm64Support replaces the x64 ReservedX640 bit.
 * https://learn.microsoft.com/en-us/virtualization/api/hypervisor-platform/funcs/whvgetcapability
 */
struct AQ_WHPX_Arm_Features
{
	std::uint64_t PartialUnmap : 1;
	std::uint64_t ReservedArm0 : 2;
	std::uint64_t DirtyPageTracking : 1;
	std::uint64_t SpeculationControl : 1;
	std::uint64_t ReservedArm1 : 1;
	std::uint64_t IdleSuspend : 1;
	std::uint64_t VirtualPciDeviceSupport : 1;
	std::uint64_t IommuSupport : 1;
	std::uint64_t VpHotAddRemove : 1;
	std::uint64_t DeviceAccessTracking : 1;
	std::uint64_t Arm64Support : 1;
	std::uint64_t Reserved : 52;

	AQ_WHPX_Arm_Features()
	{
		std::memset( this, 0, sizeof( *this ) );
	}
};

inline std::uint64_t AQ_WHPX_Arm64Support_Mask()
{
	AQ_WHPX_Arm_Features features;
	features.Arm64Support = 1;
	std::uint64_t raw = 0;
	std::memcpy( &raw, &features, sizeof( raw ) );
	return raw;
}

inline bool AQ_WHPX_Features_Have_Arm64Support( std::uint64_t raw )
{
	return ( raw & AQ_WHPX_Arm64Support_Mask() ) != 0;
}

#endif
