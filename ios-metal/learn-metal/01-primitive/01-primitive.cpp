/*
 *
 * Copyright 2022 Apple Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>

#define NS_PRIVATE_IMPLEMENTATION
#define UI_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <UIKit/UIKit.hpp>
#include <MetalKit/MetalKit.hpp>

#include <simd/simd.h>


#pragma region Declarations {
class MyMTKViewDelegate : public MTK::ViewDelegate
{
public:
    MyMTKViewDelegate( MTL::Device* pDevice );
    virtual void drawInMTKView( MTK::View* pView ) override;
    
private:
    MTL::Device* _pDevice;
    MTL::CommandQueue* _pCommandQueue;
    MTL::RenderPipelineState* _pPSO;
    MTL::Buffer* _pVertexPositionsBuffer;
    MTL::Buffer* _pVertexColorsBuffer;

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

int main( int argc, char* argv[] )
{
    NS::AutoreleasePool* pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

    MyAppDelegate del;
    UI::ApplicationMain(argc, argv, &del);

    pAutoreleasePool->release();

    return 0;
}

#pragma mark - AppDelegate
#pragma region AppDelegate {

/*
MyAppDelegate::~MyAppDelegate()
{
    _pMtkView->release();
    _pWindow->release();
    _pViewController->release();
    _pDevice->release();
    delete _pViewDelegate;
}
 */

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
: MTK::ViewDelegate()
, _pDevice( pDevice->retain() )

{
    
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
                o.position.xy *= 2.0;
                o.position.xy -= 1.0;
                o.position.y *= -1;
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

    {
        const size_t NumVertices = 3;

        simd::float3 positions[NumVertices] =
        {
            { 0.0f,  1.0f, 0.0f },
            {  1.0f, 1.0f, 0.0f },
            { 0.5f,  0.0f, 0.0f }
        };

        simd::float3 colors[NumVertices] =
        {
            {  1.0f, 0.0f, 0.0f },
            {  1.0f, 0.0f, 0.0f },
            {  1.0f, 0.0f, 0.0f }
        };

        const size_t positionsDataSize = NumVertices * sizeof( simd::float3 );
        const size_t colorDataSize = NumVertices * sizeof( simd::float3 );

        MTL::Buffer* pVertexPositionsBuffer = _pDevice->newBuffer( positionsDataSize, MTL::ResourceStorageModeShared );
        MTL::Buffer* pVertexColorsBuffer = _pDevice->newBuffer( colorDataSize, MTL::ResourceStorageModeShared );

        _pVertexPositionsBuffer = pVertexPositionsBuffer;
        _pVertexColorsBuffer = pVertexColorsBuffer;

        memcpy( _pVertexPositionsBuffer->contents(), positions, positionsDataSize );
        memcpy( _pVertexColorsBuffer->contents(), colors, colorDataSize );
    }

    
    
}

void MyMTKViewDelegate::drawInMTKView( MTK::View* pView )
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

    //pPool->release();
    
}

#pragma endregion ViewDelegate }
