#include "Store_Trial.h"

#include <QDesktopServices>
#include <QObject>
#include <QUrl>

#if defined( Q_OS_WIN ) && defined( AQEMU_STORE_BUILD )

static const char *kStoreUrl = "ms-windows-store://pdp/?productid=9P0HGKHQ9W19";

void AQ_Store_Buy()
{
	QDesktopServices::openUrl( QUrl( QLatin1String( kStoreUrl ) ) );
}

#if defined( AQEMU_HAVE_STORE_WINRT )
#include <chrono>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Services.Store.h>

QString AQ_Store_Status_Text()
{
	try
	{
		winrt::init_apartment( winrt::apartment_type::multi_threaded );
		auto ctx = winrt::Windows::Services::Store::StoreContext::GetDefault();
		auto lic = ctx.GetAppLicenseAsync().get();
		if( ! lic.IsActive() || ! lic.IsTrial() )
			return QString();
		const auto exp = lic.ExpirationDate();
		const auto now = winrt::clock::now();
		const auto left = std::chrono::duration_cast<std::chrono::hours>( exp - now ).count() / 24;
		if( left <= 0 )
			return QObject::tr( "Trial ends today" );
		return QObject::tr( "Trial: %1 days left" ).arg( left );
	}
	catch( ... )
	{
		return QString();
	}
}

bool AQ_Store_Licence_Inactive()
{
	try
	{
		winrt::init_apartment( winrt::apartment_type::multi_threaded );
		auto ctx = winrt::Windows::Services::Store::StoreContext::GetDefault();
		auto lic = ctx.GetAppLicenseAsync().get();
		return ! lic.IsActive();
	}
	catch( ... )
	{
		return false;
	}
}
#else
QString AQ_Store_Status_Text()
{
	return QString();
}

bool AQ_Store_Licence_Inactive()
{
	return false;
}
#endif

#else

void AQ_Store_Buy() {}
QString AQ_Store_Status_Text() { return QString(); }
bool AQ_Store_Licence_Inactive() { return false; }

#endif
