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

#define _USE_MATH_DEFINES
#include "../common.h"
#include "../platform_main.h"
#include "../macos_platform.cpp"
#include "../common.cpp"
#include "../main.cpp"

#pragma clang diagnostic pop

#pragma region Declarations {

    // previously in the private below
MTL::RenderPipelineState* render_pipeline_state;
MTL::CommandQueue* _pCommandQueue;
MTL::Buffer* _pVertexPositionsBuffer[BUFFER_MAX];
MTL::Buffer* _pVertexColorsBuffer[BUFFER_MAX];

MTL::Device* seth_pDevice;
class SethMtkViewDelegate : public MTK::ViewDelegate
{
    public:
        SethMtkViewDelegate( MTL::Device* pDevice );
        virtual void drawInMTKView( MTK::View* pView ) override;
        virtual void mouseMoved(NSEvent *) event override;

    private:
        MTL::Device* _pDevice;
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

Arena * arena;

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

    arena = arena_init();

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

        render_pipeline_state = _pDevice->newRenderPipelineState( pDesc, &pError );
        if ( !render_pipeline_state )
        {
            __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
            assert( false );
        }

        pVertexFn->release();
        pFragFn->release();
        pDesc->release();
        pLibrary->release();
    }
 

}

U8 *buffer_str_arr[BUFFER_MAX]; 
MTK::View* seth_view;
MTL::RenderCommandEncoder* render_cmd_encoder;
void platform_draw_triangle(String * id_str, V2F32 p1, V2F32 p2, V2F32 p3, V3F32 color) {


    //
    // get hash_idx - seth - this can probably be compressed
    //
    S32 hash_idx = (S32)hash_key(id_str);
    S32 hash_idx_start = hash_idx;
    for (;;) {
        if (!_pVertexPositionsBuffer[hash_idx]) {
            //string_print(id_str);
            debug_hash("id placed in bucket %d\n", hash_idx);
            break;
        }
        if (string_compare((String *)buffer_str_arr[hash_idx], id_str)) {
            //string_print(id_str);
            debug_hash("id found in bucket %d\n", hash_idx);
            break;
        }
        //string_print(id_str);
        debug_hash("id can't be placed in bucket %d, moving on\n",  hash_idx);
        hash_idx++;
        // wrap around
        if (hash_idx >= BUFFER_MAX) {
            debug_hash("at the end of list starting over\n");
            hash_idx = 0;
        }
        if (hash_idx == hash_idx_start) {
            debug_hash("Scanned all possibilities, couldn't find place for buffer, increase BUFFER_MAX\n");

            debug_hash("id: ");
            //string_print(id_str);
            debug_hash("buffer_str_arr:\n");
            for (S32 idx = 0; idx < BUFFER_MAX; ++idx) {
                debug_hash("%d:\t", idx);
                //string_print(buffer_str_arr[idx]);
            }
            assert(0);
            // print contents of buffer_str_arr
        }
    }


    S32 buffer_idx = hash_idx;


    // buffers
        if(!_pVertexPositionsBuffer[buffer_idx]) {

            buffer_str_arr[hash_idx] = (U8 *)id_str;
            const size_t NumVertices = 3;
            simd::float3 positions[NumVertices] = {
                {p1.x, p1.y, 0.0f}, 
                {p3.x, p3.y, 0.0f},
                {p2.x, p2.y, 0.0f},
            };

            simd::float3 colors[NumVertices] = {
                {color.r, color.g, color.b},
                {color.r, color.g, color.b},
                {color.r, color.g, color.b},
            };
            _pVertexPositionsBuffer[buffer_idx] = seth_pDevice->newBuffer( sizeof(positions), MTL::ResourceStorageModeManaged );
            _pVertexColorsBuffer[buffer_idx] = seth_pDevice->newBuffer( sizeof(colors), MTL::ResourceStorageModeManaged );

            memcpy( _pVertexPositionsBuffer[buffer_idx]->contents(), positions, sizeof(positions));
            memcpy( _pVertexColorsBuffer[buffer_idx]->contents(), colors, sizeof(colors));

            _pVertexPositionsBuffer[buffer_idx]->didModifyRange( NS::Range::Make( 0, _pVertexPositionsBuffer[0]->length() ) );
            _pVertexColorsBuffer[buffer_idx]->didModifyRange( NS::Range::Make( 0, _pVertexColorsBuffer[0]->length() ) );
        } // buffer

    /// draw
    {
        render_cmd_encoder->setVertexBuffer( _pVertexPositionsBuffer[buffer_idx], 0, 0 );
        render_cmd_encoder->setVertexBuffer( _pVertexColorsBuffer[buffer_idx], 0, 1 );
        render_cmd_encoder->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );
        
    }
}

/* not needed 
SethMtkViewDelegate::~SethMtkViewDelegate()
{
    _pVertexPositionsBuffer->release();
    _pVertexColorsBuffer->release();
    render_pipeline_state->release();
    _pCommandQueue->release();
    _pDevice->release();
}
*/
void SethMtkViewDelegate::drawInMTKView( MTK::View* view)
{
    seth_pDevice = _pDevice;
    seth_view = view;
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* command_buffer = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* render_pass_desc = seth_view ->currentRenderPassDescriptor();
    render_cmd_encoder = command_buffer->renderCommandEncoder( render_pass_desc );
    render_cmd_encoder->setRenderPipelineState( render_pipeline_state );
    game_loop();
    render_cmd_encoder->endEncoding();
    command_buffer->presentDrawable( seth_view->currentDrawable() );
    command_buffer->commit();

    pool->release();
}

void SethMtkViewDelegate::mouseMoved(NSEvent * event) 
{
    printf("mouse moved\n");
}


#pragma endregion ViewDelegate }


