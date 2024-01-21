//#include <cassert>

#define NS_PRIVATE_IMPLEMENTATION
#define UI_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#include <Metal/Metal.hpp>
#include <UIKit/UIKit.hpp>
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
class MyMTKViewDelegate : public MTK::ViewDelegate
{
public:
    MyMTKViewDelegate( MTL::Device* pDevice );
    virtual void drawInMTKView( MTK::View* pView ) override;
    
private:
    MTL::Device* _pDevice;
};

class MyAppDelegate : public UI::ApplicationDelegate
{
    public:
        bool applicationDidFinishLaunching( UI::Application *pApp, NS::Value *options ) override;
        void applicationWillTerminate( UI::Application *pApp ) override;

    private:
        UI::Window* _pWindow;
        UI::ViewController* _pViewController;
        MTK::View* _pMtkView;
        MTL::Device* _pDevice;
        MyMTKViewDelegate* _pViewDelegate = nullptr;
};

#pragma endregion Declarations }

Arena * arena;

int main( int argc, char* argv[] )
{
    NS::AutoreleasePool* pool= NS::AutoreleasePool::alloc()->init();

    MyAppDelegate delegate;
    UI::ApplicationMain(argc, argv, &delegate);

    //pAutoreleasePool->release();

    return 0;
}

#pragma mark - AppDelegate
#pragma region AppDelegate {

bool MyAppDelegate::applicationDidFinishLaunching( UI::Application *pApp, NS::Value *options )
{
    CGRect frame = UI::Screen::mainScreen()->bounds();

    _pWindow = UI::Window::alloc()->init(frame);

    _pViewController = UI::ViewController::alloc()->init( nil, nil );

    _pDevice = MTL::CreateSystemDefaultDevice();

    _pMtkView = MTK::View::alloc()->init( frame, _pDevice );
    _pMtkView->setColorPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    _pMtkView->setClearColor( MTL::ClearColor::Make( 0.0, 0.0, 0.0, 1.0 ) );

    _pViewDelegate = new MyMTKViewDelegate( _pDevice );
    _pMtkView->setDelegate( _pViewDelegate );

    UI::View *mtkView = (UI::View *)_pMtkView;
    mtkView->setAutoresizingMask( UI::ViewAutoresizingFlexibleWidth | UI::ViewAutoresizingFlexibleHeight );
    _pViewController->view()->addSubview( mtkView );
    _pWindow->setRootViewController( _pViewController );

    _pWindow->makeKeyAndVisible();

    return true;
}

void MyAppDelegate::applicationWillTerminate( UI::Application *pApp )
{
}

#pragma endregion AppDelegate }


#pragma mark - ViewDelegate
#pragma region ViewDelegate {

MyMTKViewDelegate::MyMTKViewDelegate( MTL::Device* pDevice )
    : MTK::ViewDelegate() , _pDevice( pDevice->retain() )
{
    arena = arena_init();
    
    _pCommandQueue = _pDevice->newCommandQueue();
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
                o.position.x /= 300; // pixel to percent
                o.position.y /= 1000;
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

    // @simplify - this can be moved out of the platform layer


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
    { // buffer
        
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
    
        if(!_pVertexPositionsBuffer[buffer_idx]) {
            
            buffer_str_arr[hash_idx] = (U8 *)id_str;
             _pVertexPositionsBuffer[buffer_idx] = seth_pDevice->newBuffer( sizeof(positions), MTL::ResourceStorageModeShared );
            _pVertexColorsBuffer[buffer_idx] = seth_pDevice->newBuffer( sizeof(colors), MTL::ResourceStorageModeShared );
            
        }
        memcpy( _pVertexPositionsBuffer[buffer_idx]->contents(), positions, sizeof(positions));
        memcpy( _pVertexColorsBuffer[buffer_idx]->contents(), colors, sizeof(colors));
        
        _pVertexPositionsBuffer[buffer_idx]->didModifyRange( NS::Range::Make( 0, _pVertexPositionsBuffer[buffer_idx]->length() ) );
        _pVertexColorsBuffer[buffer_idx]->didModifyRange( NS::Range::Make( 0, _pVertexColorsBuffer[buffer_idx]->length() ) );

    }// buffer

    /// draw
    {
        render_cmd_encoder->setVertexBuffer( _pVertexPositionsBuffer[buffer_idx], 0, 0 );
        render_cmd_encoder->setVertexBuffer( _pVertexColorsBuffer[buffer_idx], 0, 1 );
        render_cmd_encoder->drawPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3) );
        
    }
}

void MyMTKViewDelegate::drawInMTKView( MTK::View* view )
{
    seth_pDevice = _pDevice;
    seth_view = view;
    NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

    MTL::CommandBuffer* command_buffer = _pCommandQueue->commandBuffer();
    MTL::RenderPassDescriptor* render_pass_desc = seth_view->currentRenderPassDescriptor();
    render_cmd_encoder = command_buffer->renderCommandEncoder( render_pass_desc );
    render_cmd_encoder->setRenderPipelineState( render_pipeline_state );

    game_loop();

    render_cmd_encoder->endEncoding();
    command_buffer->presentDrawable( seth_view->currentDrawable() );
    command_buffer->commit();

    pool->release();
}

#pragma endregion ViewDelegate }

