

#include "config.hpp"

#if defined( DEBUG )

	#include <stdio.h>
	#define Trace( fmt, ... )		fprintf(stderr, "%s(%d) " fmt "\n", __FILE__, __LINE__, __VA_ARGS__ ); fflush(stderr)

#else
	#error DEBUG ONLY
	#define Trace( fmt, ... )		/* __VA_ARGS__ */

#endif


#include <cppuhelper/queryinterface.hxx> // helper for queryInterface() impl
#include <cppuhelper/factory.hxx> // helper for component factory

#include <cppuhelper/queryinterface.hxx> // helper for queryInterface() impl
#include <cppuhelper/factory.hxx> // helper for component factory

// generated c++ interfaces
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>
#include <com/sun/star/registry/XRegistryKey.hpp>

#include <br/com/bb/I3270.hpp>


/*---[ Object implementation ]-----------------------------------------------------------------------------*/

using namespace ::rtl;
using namespace ::osl;
using namespace ::cppu;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::registry;
using namespace ::br::com::bb;

class I3270Impl : public br::com::bb::I3270, public lang::XServiceInfo, public lang::XTypeProvider
{
public:
	I3270Impl( const Reference< XMultiServiceFactory > & xServiceManager );
	~I3270Impl();

	// XInterface implementation
	virtual void SAL_CALL acquire() throw ();
	virtual void SAL_CALL release() throw ();
	virtual Any SAL_CALL queryInterface( const Type & rType ) throw (RuntimeException);

	// XServiceInfo	implementation
    virtual OUString SAL_CALL getImplementationName(  ) throw(RuntimeException);
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) throw(RuntimeException);
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) throw(RuntimeException);
    static Sequence< OUString > SAL_CALL getSupportedServiceNames_Static(  );

	// XTypeProvider implementation
	virtual Sequence<sal_Int8> SAL_CALL getImplementationId(void) throw (RuntimeException);
	virtual Sequence< Type > SAL_CALL getTypes() throw (RuntimeException);

	// I3270 implementation - Main
	virtual OUString SAL_CALL getVersion() throw (RuntimeException);

	// I3270 implementation - Connect/Disconnect
	virtual sal_Int16 SAL_CALL getConnectionState() throw (RuntimeException);
	virtual sal_Int16 SAL_CALL Connect( const OUString& hostinfo, ::sal_Int16 wait ) throw (RuntimeException);
	virtual sal_Int16 SAL_CALL Disconnect() throw (RuntimeException);

	// I3270 implementation - Screen
    virtual ::rtl::OUString SAL_CALL getScreenContentAt( ::sal_Int16 row, ::sal_Int16 col, ::sal_Int16 size ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::rtl::OUString SAL_CALL getScreenContent() throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Int16 SAL_CALL waitForScreen( ::sal_Int16 timeout ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Int16 SAL_CALL waitForStringAt( ::sal_Int16 row, ::sal_Int16 col, const ::rtl::OUString& key, ::sal_Int16 timeout ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Bool SAL_CALL queryStringAt( ::sal_Int16 row, ::sal_Int16 col, const ::rtl::OUString& key ) throw (::com::sun::star::uno::RuntimeException);

	// I3270 implementation - Actions
    virtual ::sal_Int16 SAL_CALL sendEnterKey( ::sal_Int16 wait ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Int16 SAL_CALL setStringAt( ::sal_Int16 row, ::sal_Int16 col, const ::rtl::OUString& str ) throw (::com::sun::star::uno::RuntimeException);

private:
	sal_Int32 m_nRefCount;

	::sal_Int16 waitForUpdate( ::sal_Int16 timeout );


};

