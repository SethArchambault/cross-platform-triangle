#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#include <Metal/Metal.hpp>
#include <AppKit/AppKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include <simd/simd.h>

#pragma clang diagnostic pop

#pragma region Declarations {

class SethMtkViewDelegate : public MTK::ViewDelegate
{
    public:
        SethMtkViewDelegate( MTL::Device* pDevice );
        virtual void drawInMTKView( MTK::View* pView ) override;

    private:
        MTL::Device* _pDevice;
        MTL::CommandQueue* _pCommandQueue;
        MTL::RenderPipelineState* _pPSO;
        MTL::Buffer* _pVertexPositionsBuffer;
        MTL::Buffer* _pVertexColorsBuffer;
};

class SethDelegate : public NS::ApplicationDelegate
{
    public:
        //~SethDelegate();

        NS::Menu* createMenuBar();

        virtual void applicationWillFinishLaunching( NS::Notification* pNotification ) override;
        virtual void applicationDidFinishLaunching( NS::Notification* pNotification ) override;
        virtual bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override;

    private:
        NS::Window* _pWindow;
        MTK::View* _pMtkView;
        MTL::Device* _pDevice;
        SethMtkViewDelegate* _pViewDelegate = nullptr;
};

#pragma endregion Declarations }

/// :main
int main( int argc, char* argv[] )
{
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    SethDelegate seth_delegate;

    NS::Application* shared_app = NS::Application::sharedApplication();
    shared_app->setDelegate( &seth_delegate );
    shared_app->run();

    //pool->release(); //not needed

    return 0;
}



#pragma mark - AppDelegate
#pragma region AppDelegate {

/* not needed
SethDelegate::~SethDelegate()
{
    _pMtkView->release();
    _pWindow->release();
    _pDevice->release();
    delete _pViewDelegate;
}
*/

/// :menubar
NS::Menu* SethDelegate::createMenuBar()
{
    using NS::StringEncoding::UTF8StringEncoding;

    NS::Menu* pMainMenu = NS::Menu::alloc()->init();
    NS::MenuItem* pAppMenuItem = NS::MenuItem::alloc()->init();
    NS::Menu* pAppMenu = NS::Menu::alloc()->init( NS::String::string( "Appname", UTF8StringEncoding ) );
    NS::String* quitItemName = NS::String::string( "Quit", UTF8StringEncoding );
    SEL quitCb = NS::MenuItem::registerActionCallback( "appQuit", [](void*,SEL,const NS::Object* pSender){
        auto pApp = NS::Application::sharedApplication();
        pApp->terminate( pSender );
    } );

    NS::MenuItem* pAppQuitItem = pAppMenu->addItem( quitItemName, quitCb, NS::String::string( "q", UTF8StringEncoding ) );
    pAppQuitItem->setKeyEquivalentModifierMask( NS::EventModifierFlagCommand );
    pAppMenuItem->setSubmenu( pAppMenu );

    pMainMenu->addItem( pAppMenuItem );

    pAppMenuItem->release();
    pAppMenu->release();

    return pMainMenu->autorelease();
}

void SethDelegate::applicationWillFinishLaunching( NS::Notification* pNotification )
{
    NS::Menu* pMenu = createMenuBar();
    NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
    pApp->setMainMenu( pMenu );
    pApp->setActivationPolicy( NS::ActivationPolicy::ActivationPolicyRegular );
}

/// define app name 
void SethDelegate::applicationDidFinishLaunching( NS::Notification* pNotification )
{
    /// window origin, window size
    CGRect frame = { 
        .origin = { .x =20.0,           .y = 100.0 }, 
        .size   = { .width = 1000.0,    .height = 600.0 } 
    };

    _pWindow = NS::Window::alloc()->init(
        frame,
        NS::WindowStyleMaskClosable|NS::WindowStyleMaskTitled,
        NS::BackingStoreBuffered,
        false );

    _pDevice = MTL::CreateSystemDefaultDevice();

    _pMtkView = MTK::View::alloc()->init( frame, _pDevice );
    _pMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    _pMtkView->setClearColor( MTL::ClearColor::Make( 0.0, 0.0, 0.0, 1.0 ) );

    _pViewDelegate = new SethMtkViewDelegate( _pDevice );
    _pMtkView->setDelegate( _pViewDelegate );

    _pWindow->setContentView( _pMtkView );
    _pWindow->setTitle( NS::String::string( "Seth App", NS::StringEncoding::UTF8StringEncoding ) );

    _pWindow->makeKeyAndOrderFront( nullptr );

    NS::Application* pApp = reinterpret_cast< NS::Application* >( pNotification->object() );
    pApp->activateIgnoringOtherApps( true );
}

bool SethDelegate::applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender )
{
    return true;
}

#pragma endregion AppDelegate }


#pragma mark - ViewDelegate
#pragma region ViewDelegate {

SethMtkViewDelegate::SethMtkViewDelegate( MTL::Device* pDevice )
: MTK::ViewDelegate()
, _pDevice( pDevice->retain() )
{
    /// old renderer
    _pCommandQueue = _pDevice->newCommandQueue();
    /// Shader
    {
        using NS::StringEncoding::UTF8StringEncoding;

        const char* shaderSrc = R"(
            #include <metal_stdlib>
            using namespace metal;

            struct v2f
            {
                float4 position [[position]];
                half3 color;
            };

            v2f vertex vertexMain( uint vertexId [[vertex_id]],
                                   device const float3* positions [[buffer(0)]],
                                   device const float3* colors [[buffer(1)]] )
            {
                v2f o;
                o.position = float4( positions[ vertexId ], 1.0 );
                o.position.x /= 1000; // pixel to percent
                o.position.y /= 600;
                o.position.xy *= 2; // scale
                o.position.xy -= 1; // adjust
                o.position.y *= -1; // flip
                o.color = half3 ( colors[ vertexId ] );
                return o;
            }

            half4 fragment fragmentMain( v2f in [[stage_in]] )
            {
                return half4( in.color, 1.0 );
            }
        )";

        NS::Error* pError = nullptr;
        MTL::Library* pLibrary = _pDevice->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError );
        if ( !pLibrary )
        {
            __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
            assert( false );
        }

        MTL::Function* pVertexFn = pLibrary->newFunction( NS::String::string("vertexMain", UTF8StringEncoding) );
        MTL::Function* pFragFn = pLibrary->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );

        MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
        pDesc->setVertexFunction( pVertexFn );
        pDesc->setFragmentFunction( pFragFn );
        pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );

        _pPSO = _pDevice->newRenderPipelineState( pDesc, &pError );
        if ( !_pPSO )
        {
            __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
            assert( false );
        }

        pVertexFn->release();
        pFragFn->release();
        pDesc->release();
        pLibrary->release();
    }
 
    // buffers
    {
        const size_t NumVertices = 3;

        simd::float3 positions[NumVertices] =
        {
            { 0.0f, 600.0f, 0.0f },
            {  1000.0f, 600.0f, 0.0f },
            {  500.0f,  0.0f, 0.0f }
        };

        simd::float3 colors[NumVertices] =
        {
            {  1.0f, 0.0f, 0.0f },
            {  1.0f, 0.0f, 0.0f },
            {  1.0f, 0.0f, 0.0f }
        };

        const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
        const size_t colorDataSize = NumVertices * sizeof( simd::float3 );

        MTL::Buffer* pVertexPositionsBuffer = _pDevice->newBuffer( positionsDataSize, MTL::ResourceStorageModeManaged );
        MTL::Buffer* pVertexColorsBuffer = _pDevice->newBuffer( colorDataSize, MTL::ResourceStorageModeManaged );

        _pVertexPositionsBuffer = pVertexPositionsBuffer;
        _pVertexColorsBuffer = pVertexColorsBuffer;

        memcpy( _pVertexPositionsBuffer->contents(), positions, positionsDataSize );
        memcpy( _pVertexColorsBuffer->contents(), colors, colorDataSize );

        _pVertexPositionsBuffer->didModifyRange( NS::Range::Make( 0, _pVertexPositionsBuffer->length() ) );
        _pVertexColorsBuffer->didModifyRange( NS::Range::Make( 0, _pVertexColorsBuffer->length() ) );
    }

}

    /* not needed 
SethMtkViewDelegate::~SethMtkViewDelegate()
{
    _pVertexPositionsBuffer->release();
    _pVertexColorsBuffer->release();
    _pPSO->release();
    _pCommandQueue->release();
    _pDevice->release();
}

    */
void SethMtkViewDelegate::drawInMTKView( MTK::View* pView )
{
    /// draw
    {
        NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();

        MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();
        MTL::RenderPassDescriptor* pRpd = pView->currentRenderPassDescriptor();
        MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( pRpd );

        pEnc->setRenderPipelineState( _pPSO );
        pEnc->setVertexBuffer( _pVertexPositionsBuffer, 0, 0 );
        pEnc->setVertexBuffer( _pVertexColorsBuffer, 0, 1 );
        pEnc->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );
        
        pEnc->endEncoding();
        pCmd->presentDrawable( pView->currentDrawable() );
        pCmd->commit();

        pPool->release();
    }
}

#pragma endregion ViewDelegate }


