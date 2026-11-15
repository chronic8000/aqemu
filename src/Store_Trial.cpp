#include "Store_Trial.h"

#include <QDesktopServices>
#include <QObject>
#include <QUrl>
#include <QWidget>

#if defined( Q_OS_WIN ) && defined( AQEMU_STORE_BUILD )

static const char *kStoreUrl = "ms-windows-store://pdp/?productid=9P0HGKHQ9W19";
static const wchar_t *kStoreProductId = L"9P0HGKHQ9W19";

static void Open_Store_Page()
{
	QDesktopServices::openUrl( QUrl( QLatin1String( kStoreUrl ) ) );
}

#if defined( AQEMU_HAVE_STORE_WINRT )
#include <chrono>
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shobjidl.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Services.Store.h>

using winrt::Windows::Services::Store::StoreContext;
using winrt::Windows::Services::Store::StoreAppLicense;
using winrt::Windows::Services::Store::StorePurchaseStatus;

static StoreContext AQ_Store_Context( QWidget *parent )
{
	auto ctx = StoreContext::GetDefault();
	if( parent )
	{
		try
		{
			HWND hwnd = reinterpret_cast<HWND>( parent->winId() );
			if( hwnd )
			{
				winrt::com_ptr<IInitializeWithWindow> init = ctx.as<IInitializeWithWindow>();
				if( init )
					init->Initialize( hwnd );
			}
		}
		catch( ... )
		{
		}
	}
	return ctx;
}

static StoreAppLicense AQ_Store_License( QWidget *parent )
{
	auto ctx = AQ_Store_Context( parent );
	return ctx.GetAppLicenseAsync().get();
}

void AQ_Store_Buy( QWidget *parent )
{
	try
	{
		auto ctx = AQ_Store_Context( parent );
		const auto result = ctx.RequestPurchaseAsync( kStoreProductId ).get();
		const auto status = result.Status();
		if( status == StorePurchaseStatus::Succeeded
		    || status == StorePurchaseStatus::AlreadyPurchased )
			return;
		// Network / cancelled / not available — fall back to the Store PDP.
		Open_Store_Page();
	}
	catch( ... )
	{
		Open_Store_Page();
	}
}

QString AQ_Store_Status_Text()
{
	try
	{
		winrt::init_apartment( winrt::apartment_type::multi_threaded );
		auto lic = AQ_Store_License( nullptr );
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
		auto lic = AQ_Store_License( nullptr );
		return ! lic.IsActive();
	}
	catch( ... )
	{
		return false;
	}
}

bool AQ_Store_Is_Fully_Licensed()
{
	try
	{
		winrt::init_apartment( winrt::apartment_type::multi_threaded );
		auto lic = AQ_Store_License( nullptr );
		return lic.IsActive() && ! lic.IsTrial();
	}
	catch( ... )
	{
		return false;
	}
}
#else
void AQ_Store_Buy( QWidget *parent )
{
	Q_UNUSED( parent );
	Open_Store_Page();
}

QString AQ_Store_Status_Text()
{
	return QString();
}

bool AQ_Store_Licence_Inactive()
{
	return false;
}

bool AQ_Store_Is_Fully_Licensed()
{
	return false;
}
#endif

#else

void AQ_Store_Buy( QWidget *parent ) { Q_UNUSED( parent ); }
QString AQ_Store_Status_Text() { return QString(); }
bool AQ_Store_Licence_Inactive() { return false; }
bool AQ_Store_Is_Fully_Licensed() { return false; }

#endif
