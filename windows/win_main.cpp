// adding a line to see if cr get added to the end
#include <d3d11.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#define _USE_MATH_DEFINES
#include "../common.h"
#include "../platform_main.h"
#include "windows_common.cpp"
#include "../common.cpp"
#include "../main.cpp"

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")

LRESULT CALLBACK wnd_proc(HWND window_handle, UINT message_code, WPARAM wparam, LPARAM lparam) {
  switch (message_code) {
      case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
      case WM_MOUSEMOVE: {
        S32 x_coord = GET_X_LPARAM(lparam);
        S32 y_coord = GET_Y_LPARAM(lparam);
        game_mouse_move(x_coord, y_coord);
        //printf("mouse moved %d, %d\n", x_coord, y_coord);
        return 0;
     }
      default:
        return DefWindowProc(window_handle, message_code, wparam, lparam);
  }
}

struct Vertex {
    union {
        F32 position[3];
    };
    F32 color[4];
};

ID3D11DeviceContext *ctx = nullptr;
ID3D11Device *device = nullptr;
ID3D11Buffer *buffer_arr[BUFFER_MAX] = {nullptr};
U8 *buffer_str_arr[BUFFER_MAX]; 
ID3D11VertexShader *vertex_shader = nullptr;
ID3D11PixelShader *pixel_shader = nullptr;
D3D11_VIEWPORT viewport;
ID3D11RasterizerState *rasterizer = nullptr;
ID3D11RenderTargetView *rtv = nullptr;

//  draw_triangle("basic_triangle", {0.0, 100.0}, {100.0, 100.0}, {50.0, 0.0}, color);
void platform_draw_triangle(String * id_str, V2F32 p1, V2F32 p2, V2F32 p3, V3F32 color) {

    //
    // get hash_idx
    //
    S32 hash_idx = hash_key(id_str);
    S32 hash_idx_start = hash_idx;
    for (;;) {
        if (!buffer_arr[hash_idx]) {
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

    //
    /// build buffer
    //
    Vertex vertices[] = {
        {{p1.x, p1.y, 0.0f}, {color.r, color.g, color.b, 1.0f}},
        {{p3.x, p3.y, 0.0f}, {color.r, color.g, color.b, 1.0f}},
        {{p2.x, p2.y, 0.0f}, {color.r, color.g, color.b, 1.0f}},
    };
    if (!buffer_arr[hash_idx]) {
        //string_print(id_str);
        // @danger - we should free up buffer_str_arr[hash_idx] in case it's already been used?
        

        buffer_str_arr[hash_idx] = (U8 *)id_str;
        D3D11_BUFFER_DESC desc = {
            .ByteWidth      = sizeof(vertices),
            .Usage          = D3D11_USAGE_DYNAMIC,
            .BindFlags      = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        device->CreateBuffer(&desc, NULL, &buffer_arr[hash_idx]);
    }

    //
    /// Map Buffer
    //
    //string_print(buffer_str_arr[hash_idx]);
    D3D11_MAPPED_SUBRESOURCE mapped_subresource;
    ctx->Map(buffer_arr[hash_idx], NULL, D3D11_MAP_WRITE_DISCARD, NULL, 
        &mapped_subresource);   
    memcpy(mapped_subresource.pData, vertices, sizeof(vertices));
    ctx->Unmap(buffer_arr[hash_idx], NULL);

    // 
    // Set Vertex Buffers
    //
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        ctx->IASetVertexBuffers(0, 1, &buffer_arr[hash_idx], &stride, &offset);
    }
    ctx->VSSetShader(vertex_shader, nullptr, 0);
    ctx->RSSetViewports(1, &viewport);
    ctx->RSSetState(rasterizer);
    ctx->PSSetShader(pixel_shader, nullptr, 0);
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    ctx->Draw(sizeof(vertices) / sizeof(Vertex), 0);
};

int main() {
  const char *title = "Shared Triangle";

  WNDCLASSA wc = {};
  wc.lpfnWndProc = wnd_proc;
  wc.hInstance = nullptr;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.lpszClassName = title;
  RegisterClassA(&wc);

  int screen_width = 800, screen_height = 600;
  HWND hwnd = CreateWindowExA(0, title, title, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              CW_USEDEFAULT, CW_USEDEFAULT, screen_width, screen_height,
                              nullptr, nullptr, nullptr, nullptr);

  IDXGISwapChain *swapchain = nullptr;
  {
    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Windowed = true;

    D3D_FEATURE_LEVEL features[] = {D3D_FEATURE_LEVEL_11_0};
    D3D11CreateDeviceAndSwapChain(
        0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        features, 1, D3D11_SDK_VERSION, &desc, &swapchain, &device, 0, &ctx);
  }

  ID3D11Texture2D *render_target = nullptr;
  swapchain->GetBuffer(0, IID_PPV_ARGS(&render_target));

  device->CreateRenderTargetView(render_target, 0, &rtv);

  {
    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_BACK;

    device->CreateRasterizerState(&desc, &rasterizer);
  }


  // okay this neds to move.

  HINSTANCE dll = LoadLibraryA("d3dcompiler_47.dll");
  pD3DCompile d3d_compile_proc = (pD3DCompile)GetProcAddress(dll, "D3DCompile");
#define D3DCompile d3d_compile_proc

  const char *hlsl = R"(
    struct VertexIn {
      float3 position : POS;
      float4 color : COL;
    };

    struct VertexOut {
      float4 position : SV_POSITION;
      float4 color : COL;
    };

    VertexOut vs_main(VertexIn input) {
      VertexOut output;
      output.position = float4(input.position, 1.0);
      output.position.x  /= 800.;
      output.position.y  /= 600.;
      output.position.xy *= 2.0;
      output.position.xy -= 1.0;
      output.position.y *= -1.0;
      input.color.r /= 255;
      input.color.g /= 255;
      input.color.b /= 255;
      output.color = input.color;
      return output;
    }

    float4 ps_main(VertexOut input) : SV_TARGET {
      return input.color;
    }
  )";

  UINT compile_flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR |
                       D3DCOMPILE_ENABLE_STRICTNESS |
                       D3DCOMPILE_WARNINGS_ARE_ERRORS;
  compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

  ID3D11InputLayout *input_layout = nullptr;
  {
    ID3DBlob *vs_blob = nullptr;
    D3DCompile(hlsl, strlen(hlsl), nullptr, nullptr,
               D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs_main", "vs_5_0",
               compile_flags, 0, &vs_blob, nullptr);

    device->CreateVertexShader(vs_blob->GetBufferPointer(),
                               vs_blob->GetBufferSize(), nullptr,
                               &vertex_shader);

    D3D11_INPUT_ELEMENT_DESC desc[] = {
        {"POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
         D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    device->CreateInputLayout(desc, ARRAYSIZE(desc),
                              vs_blob->GetBufferPointer(),
                              vs_blob->GetBufferSize(), &input_layout);
  }

  {
    ID3DBlob *ps_blob = nullptr;
    D3DCompile(hlsl, strlen(hlsl), nullptr, nullptr,
               D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps_main", "ps_5_0",
               compile_flags, 0, &ps_blob, nullptr);

    device->CreatePixelShader(ps_blob->GetBufferPointer(),
                              ps_blob->GetBufferSize(), nullptr, &pixel_shader);
    ps_blob->Release();
  }

  int swapchain_width = screen_width, swapchain_height = screen_height;

  bool should_quit = false;
  while (!should_quit) {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (msg.message == WM_QUIT) {
        should_quit = true;
      }
    }

    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    if (width != swapchain_width || height != swapchain_height) {
      swapchain_width = width;
      swapchain_height = height;

      render_target->Release();
      rtv->Release();

      swapchain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);

      swapchain->GetBuffer(0, IID_PPV_ARGS(&render_target));
      device->CreateRenderTargetView(render_target, 0, &rtv);
    }

    float bg_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};


    viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};

    ctx->ClearRenderTargetView(rtv, bg_color);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(input_layout);
    
    S32 result = game_loop();

    swapchain->Present(1, 0);
  }
}
