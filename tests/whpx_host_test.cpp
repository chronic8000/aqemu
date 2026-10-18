#include "WHPX_Host.h"

#include <cstdio>

int main()
{
	const std::uint64_t mask = AQ_WHPX_Arm64Support_Mask();
	if( mask != ( std::uint64_t( 1 ) << 11 ) )
	{
		std::fprintf( stderr, "Arm64Support bit is 0x%llx\n",
		              static_cast<unsigned long long>( mask ) );
		return 1;
	}
	if( ! AQ_WHPX_Features_Have_Arm64Support( mask ) )
		return 1;
	if( AQ_WHPX_Features_Have_Arm64Support( 0 ) )
		return 1;
	return 0;
}
