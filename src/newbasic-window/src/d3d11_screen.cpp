#include "newbasic/window/d3d11_screen.h"

#include <windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "newbasic/screen/glyph_atlas.h"
#include "newbasic/screen/text_render.h"

namespace newbasic::screen {
namespace {

constexpr wchar_t kWindowClass[] = L"NewbasicD3D11Screen";
constexpr wchar_t kWindowTitle[] = L"newbasic";

// One vertex as the shader expects it: position in NDC, atlas uv, colour.
struct RenderVertex {
  float x;
  float y;
  float u;
  float v;
  float r;
  float g;
  float b;
  float a;
};

const char kShaderSource[] = R"(
struct VSIn  { float2 pos : POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };

VSOut VSMain(VSIn input) {
  VSOut output;
  output.pos = float4(input.pos, 0.0f, 1.0f);
  output.uv = input.uv;
  output.color = input.color;
  return output;
}

Texture2D    atlas : register(t0);
Texture2D    canvas : register(t1);
SamplerState samp  : register(s0);

float4 PSMain(VSOut input) : SV_TARGET {
  float ink = atlas.Sample(samp, input.uv).r;
  return float4(input.color.rgb, input.color.a * ink);
}

// Graphics modes (M12 phase 4): the canvas arrives as an RGBA texture already
// palette-converted on the CPU, so this is a plain blit.
float4 PSPresent(VSOut input) : SV_TARGET {
  return canvas.Sample(samp, input.uv);
}
)";

inline float ToUnit(std::uint8_t value) {
  return static_cast<float>(value) / 255.0f;
}

// Optional per-stage timing, on when NEWBASIC_TIMING is set in the environment. Reports
// averaged microseconds every sixty presents, which is how the present path was found to
// be the cost of a plotting program rather than the interpreter.
struct PresentTiming {
  static bool Enabled() {
    static const bool enabled = [] {
      char* value = nullptr;
      std::size_t length = 0;
      const bool present = _dupenv_s(&value, &length, "NEWBASIC_TIMING") == 0;
      const bool set = present && value != nullptr && *value != '\0';
      std::free(value);
      return set;
    }();
    return enabled;
  }
  static void Add(double convert_us, double upload_us, double draw_us) {
    if (!Enabled())
      return;
    convert_ += convert_us;
    upload_ += upload_us;
    draw_ += draw_us;
    ++count_;
    if (count_ % 20 == 0) {
      std::fprintf(stderr,
                   "newbasic-timing: presents=%lld avg convert=%.2fms upload=%.2fms "
                   "draw+present=%.2fms\n",
                   static_cast<long long>(count_), convert_ / count_ / 1000.0,
                   upload_ / count_ / 1000.0, draw_ / count_ / 1000.0);
    }
  }
  static inline double convert_ = 0.0;
  static inline double upload_ = 0.0;
  static inline double draw_ = 0.0;
  static inline std::int64_t count_ = 0;
};

inline double NowMicroseconds() {
  static const double frequency = static_cast<double>([] {
    LARGE_INTEGER value;
    QueryPerformanceFrequency(&value);
    return value.QuadPart;
  }());
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return static_cast<double>(counter.QuadPart) * 1000000.0 / frequency;
}

} // namespace

struct D3D11Screen::Impl {
  HWND window = nullptr;
  // Back-pointer so the window procedure can re-present on WM_PAINT.
  D3D11Screen* owner = nullptr;
  ID3D11Device* device = nullptr;
  ID3D11DeviceContext* context = nullptr;
  IDXGISwapChain* swap_chain = nullptr;
  ID3D11RenderTargetView* target = nullptr;
  ID3D11VertexShader* vertex_shader = nullptr;
  ID3D11PixelShader* pixel_shader = nullptr;
  ID3D11InputLayout* input_layout = nullptr;
  ID3D11Buffer* vertex_buffer = nullptr;
  ID3D11ShaderResourceView* atlas_view = nullptr;
  // Graphics mode (M12 phase 4): an RGBA copy of the canvas and the shader that
  // blits it behind the text.
  ID3D11Texture2D* canvas_texture = nullptr;
  ID3D11ShaderResourceView* canvas_view = nullptr;
  ID3D11PixelShader* canvas_shader = nullptr;
  int canvas_width = 0;
  int canvas_height = 0;
  ID3D11SamplerState* sampler = nullptr;
  ID3D11BlendState* blend = nullptr;
  UINT vertex_capacity = 0;
  GlyphAtlas atlas;
  std::vector<wchar_t> keys;
  // Clicks waiting to be read, in canvas pixels.
  std::vector<Screen::MouseClick> clicks;
  bool closed = false;
  bool device_lost = false;
  // Resize is deferred to Present: ResizeBuffers must not run inside the message
  // handler while the swap chain may still be referenced.
  bool resize_pending = false;
  UINT pending_width = 0;
  UINT pending_height = 0;
};

namespace {

D3D11Screen::Impl* ImplFor(HWND window) {
  return reinterpret_cast<D3D11Screen::Impl*>(GetWindowLongPtrW(window, GWLP_USERDATA));
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
  D3D11Screen::Impl* impl = ImplFor(window);
  switch (message) {
  case WM_CLOSE:
    if (impl != nullptr)
      impl->closed = true;
    DestroyWindow(window);
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  case WM_SIZE:
    if (impl != nullptr) {
      impl->pending_width = LOWORD(lparam);
      impl->pending_height = HIWORD(lparam);
      impl->resize_pending = true;
      return 0;
    }
    break;
  case WM_ERASEBKGND:
    // The swap chain owns every pixel; erasing here would paint over the frame.
    return 1;
  case WM_PAINT:
    // A swap chain keeps its last present, so DWM reuses it; validating is enough.
    // Presenting here as well used to be an invitation to an endless loop: present
    // repaints, repaint asks for a paint, paint presents again. A plotting program
    // spent 20 seconds on that while the same program on the console backend took
    // 0.14.
    if (impl != nullptr) {
      ValidateRect(window, nullptr);
      return 0;
    }
    break;
  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
    if (impl != nullptr && impl->owner != nullptr) {
      // Map the client point through the same letterbox the presenter draws into, so a
      // click lands on the canvas pixel being pointed at.
      RECT client;
      std::memset(&client, 0, sizeof(client));
      GetClientRect(window, &client);
      const int client_width = client.right - client.left;
      const int client_height = client.bottom - client.top;
      const Canvas& canvas = impl->owner->Graphics();
      if (client_width > 0 && client_height > 0 && canvas.Width() > 0 && canvas.Height() > 0) {
        const float aspect = static_cast<float>(impl->owner->DisplayAspect());
        int frame_width = client_width;
        int frame_height = static_cast<int>(static_cast<float>(client_width) / aspect);
        if (frame_height > client_height) {
          frame_height = client_height;
          frame_width = static_cast<int>(static_cast<float>(client_height) * aspect);
        }
        const int left = (client_width - frame_width) / 2;
        const int top = (client_height - frame_height) / 2;
        const int click_x = static_cast<int>(static_cast<short>(LOWORD(lparam)));
        const int click_y = static_cast<int>(static_cast<short>(HIWORD(lparam)));
        const int canvas_x = (click_x - left) * canvas.Width() / frame_width;
        const int canvas_y = (click_y - top) * canvas.Height() / frame_height;
        if (canvas_x >= 0 && canvas_y >= 0 && canvas_x < canvas.Width() &&
            canvas_y < canvas.Height()) {
          Screen::MouseClick click;
          click.x = canvas_x;
          click.y = canvas_y;
          click.button = message == WM_RBUTTONDOWN ? 2 : 1;
          impl->clicks.push_back(click);
        }
      }
      return 0;
    }
    break;
  case WM_CHAR:
    if (impl != nullptr) {
      impl->keys.push_back(static_cast<wchar_t>(wparam));
      return 0;
    }
    break;
  case WM_KEYDOWN:
    if (impl != nullptr) {
      // Only keys that produce no character are mapped here. Enter and Escape are delivered
      // as WM_CHAR, and queueing them from WM_KEYDOWN as well put two in the queue: the
      // second Enter became an empty line, which is the blank line between typed lines.
      // Arrow keys are delivered as key-down rather than characters; map them to
      // the dialect's extended codes the same way the console backend does.
      if (wparam == VK_LEFT)
        impl->keys.push_back(0x00EB);
      if (wparam == VK_RIGHT)
        impl->keys.push_back(0x00EC);
      if (wparam == VK_UP)
        impl->keys.push_back(0x00E9);
      if (wparam == VK_DOWN)
        impl->keys.push_back(0x00EA);
      return 0;
    }
    break;
  default:
    break;
  }
  return DefWindowProcW(window, message, wparam, lparam);
}

bool EnsureWindowClass() {
  static bool registered = false;
  if (registered)
    return true;
  WNDCLASSEXW info;
  std::memset(&info, 0, sizeof(info));
  info.cbSize = sizeof(info);
  info.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  info.lpfnWndProc = WindowProc;
  info.hInstance = GetModuleHandleW(nullptr);
  info.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)); // IDC_ARROW, wide
  info.hbrBackground = nullptr;                                 // D3D paints every pixel
  info.lpszClassName = kWindowClass;
  if (RegisterClassExW(&info) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    return false;
  registered = true;
  return true;
}

// Creates the device and swap chain, trying hardware first, then WARP - the
// software rasteriser the plan requires as a fallback.
bool CreateDevice(D3D11Screen::Impl* impl, int width, int height, std::string* error) {
  DXGI_SWAP_CHAIN_DESC description;
  std::memset(&description, 0, sizeof(description));
  description.BufferCount = 2;
  description.BufferDesc.Width = static_cast<UINT>(width);
  description.BufferDesc.Height = static_cast<UINT>(height);
  description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  description.BufferDesc.RefreshRate.Numerator = 60;
  description.BufferDesc.RefreshRate.Denominator = 1;
  description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  description.OutputWindow = impl->window;
  description.SampleDesc.Count = 1;
  description.Windowed = TRUE;

  const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                      D3D_FEATURE_LEVEL_10_0};
  D3D_FEATURE_LEVEL obtained = D3D_FEATURE_LEVEL_11_0;
  const D3D_DRIVER_TYPE drivers[] = {D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP};

  HRESULT result = E_FAIL;
  for (const D3D_DRIVER_TYPE driver : drivers) {
    result = D3D11CreateDeviceAndSwapChain(nullptr, driver, nullptr, 0, levels,
                                           static_cast<UINT>(sizeof(levels) / sizeof(levels[0])),
                                           D3D11_SDK_VERSION, &description, &impl->swap_chain,
                                           &impl->device, &obtained, &impl->context);
    if (SUCCEEDED(result))
      return true;
    // Release whatever the failed attempt produced before trying the next one.
    if (impl->swap_chain != nullptr) {
      impl->swap_chain->Release();
      impl->swap_chain = nullptr;
    }
    if (impl->context != nullptr) {
      impl->context->Release();
      impl->context = nullptr;
    }
    if (impl->device != nullptr) {
      impl->device->Release();
      impl->device = nullptr;
    }
  }
  if (error != nullptr)
    *error = "D3D11CreateDeviceAndSwapChain failed (hardware and WARP)";
  return false;
}

bool CreateRenderTarget(D3D11Screen::Impl* impl, std::string* error) {
  ID3D11Texture2D* back_buffer = nullptr;
  HRESULT result = impl->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                               reinterpret_cast<void**>(&back_buffer));
  if (FAILED(result) || back_buffer == nullptr) {
    if (error != nullptr)
      *error = "swap chain has no back buffer";
    return false;
  }
  result = impl->device->CreateRenderTargetView(back_buffer, nullptr, &impl->target);
  back_buffer->Release();
  if (FAILED(result)) {
    if (error != nullptr)
      *error = "CreateRenderTargetView failed";
    return false;
  }
  return true;
}

bool CompileShader(const char* entry, const char* profile, ID3DBlob** blob) {
  ID3DBlob* errors = nullptr;
  const HRESULT result = D3DCompile(kShaderSource, sizeof(kShaderSource) - 1, nullptr, nullptr,
                                    nullptr, entry, profile, 0, 0, blob, &errors);
  if (errors != nullptr)
    errors->Release();
  return SUCCEEDED(result);
}

bool CreatePipeline(D3D11Screen::Impl* impl, std::string* error) {
  ID3DBlob* vertex_blob = nullptr;
  ID3DBlob* pixel_blob = nullptr;
  if (!CompileShader("VSMain", "vs_4_0", &vertex_blob)) {
    if (error != nullptr)
      *error = "vertex shader compilation failed";
    return false;
  }
  if (!CompileShader("PSMain", "ps_4_0", &pixel_blob)) {
    vertex_blob->Release();
    if (error != nullptr)
      *error = "pixel shader compilation failed";
    return false;
  }

  bool ok = true;
  ok = ok && SUCCEEDED(impl->device->CreateVertexShader(vertex_blob->GetBufferPointer(),
                                                        vertex_blob->GetBufferSize(), nullptr,
                                                        &impl->vertex_shader));
  ok = ok && SUCCEEDED(impl->device->CreatePixelShader(pixel_blob->GetBufferPointer(),
                                                       pixel_blob->GetBufferSize(), nullptr,
                                                       &impl->pixel_shader));
  // The graphics-mode blit shader shares the vertex shader and sampler. A failure
  // here must be fatal rather than silent: without it a graphics mode renders as a
  // blank window with only the text overlay, which looks like a drawing bug rather
  // than a shader that never compiled.
  ID3DBlob* canvas_blob = nullptr;
  if (ok && !CompileShader("PSPresent", "ps_4_0", &canvas_blob)) {
    ok = false;
    if (error != nullptr)
      *error = "canvas shader compilation failed";
  }
  if (ok) {
    ok = SUCCEEDED(impl->device->CreatePixelShader(canvas_blob->GetBufferPointer(),
                                                   canvas_blob->GetBufferSize(), nullptr,
                                                   &impl->canvas_shader));
  }
  if (canvas_blob != nullptr)
    canvas_blob->Release();

  if (ok) {
    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    ok = SUCCEEDED(impl->device->CreateInputLayout(
        layout, static_cast<UINT>(sizeof(layout) / sizeof(layout[0])),
        vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize(), &impl->input_layout));
  }
  vertex_blob->Release();
  pixel_blob->Release();
  if (!ok) {
    if (error != nullptr)
      *error = "shader or input layout creation failed";
    return false;
  }

  // The atlas as an R8 texture plus a linear sampler.
  D3D11_TEXTURE2D_DESC texture;
  std::memset(&texture, 0, sizeof(texture));
  texture.Width = static_cast<UINT>(impl->atlas.width);
  texture.Height = static_cast<UINT>(impl->atlas.height);
  texture.MipLevels = 1;
  texture.ArraySize = 1;
  texture.Format = DXGI_FORMAT_R8_UNORM;
  texture.SampleDesc.Count = 1;
  texture.Usage = D3D11_USAGE_IMMUTABLE;
  texture.BindFlags = D3D11_BIND_SHADER_RESOURCE;

  D3D11_SUBRESOURCE_DATA data;
  std::memset(&data, 0, sizeof(data));
  data.pSysMem = impl->atlas.pixels.data();
  data.SysMemPitch = static_cast<UINT>(impl->atlas.width);

  ID3D11Texture2D* atlas_texture = nullptr;
  if (FAILED(impl->device->CreateTexture2D(&texture, &data, &atlas_texture))) {
    if (error != nullptr)
      *error = "atlas texture creation failed";
    return false;
  }
  const HRESULT view_result =
      impl->device->CreateShaderResourceView(atlas_texture, nullptr, &impl->atlas_view);
  atlas_texture->Release();
  if (FAILED(view_result)) {
    if (error != nullptr)
      *error = "atlas shader resource view failed";
    return false;
  }

  D3D11_SAMPLER_DESC sampler;
  std::memset(&sampler, 0, sizeof(sampler));
  sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampler.MaxLOD = D3D11_FLOAT32_MAX;
  if (FAILED(impl->device->CreateSamplerState(&sampler, &impl->sampler))) {
    if (error != nullptr)
      *error = "sampler creation failed";
    return false;
  }

  D3D11_BLEND_DESC blend_desc;
  std::memset(&blend_desc, 0, sizeof(blend_desc));
  blend_desc.RenderTarget[0].BlendEnable = TRUE;
  blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
  blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
  blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  if (FAILED(impl->device->CreateBlendState(&blend_desc, &impl->blend))) {
    if (error != nullptr)
      *error = "blend state creation failed";
    return false;
  }
  return true;
}

// Rebuilds the device chain after DXGI_ERROR_DEVICE_REMOVED. Everything except
// the window is recreated; returns false when that fails, in which case the
// caller stops rather than presenting to a dead device.
bool RecreateDevice(D3D11Screen::Impl* impl) {
  if (impl->device != nullptr && impl->context != nullptr) {
    impl->context->OMSetRenderTargets(0, nullptr, nullptr);
  }
  if (impl->blend != nullptr) {
    impl->blend->Release();
    impl->blend = nullptr;
  }
  if (impl->sampler != nullptr) {
    impl->sampler->Release();
    impl->sampler = nullptr;
  }
  if (impl->atlas_view != nullptr) {
    impl->atlas_view->Release();
    impl->atlas_view = nullptr;
  }
  if (impl->vertex_buffer != nullptr) {
    impl->vertex_buffer->Release();
    impl->vertex_buffer = nullptr;
  }
  impl->vertex_capacity = 0;
  if (impl->input_layout != nullptr) {
    impl->input_layout->Release();
    impl->input_layout = nullptr;
  }
  if (impl->pixel_shader != nullptr) {
    impl->pixel_shader->Release();
    impl->pixel_shader = nullptr;
  }
  if (impl->vertex_shader != nullptr) {
    impl->vertex_shader->Release();
    impl->vertex_shader = nullptr;
  }
  if (impl->target != nullptr) {
    impl->target->Release();
    impl->target = nullptr;
  }
  if (impl->swap_chain != nullptr) {
    impl->swap_chain->Release();
    impl->swap_chain = nullptr;
  }
  if (impl->context != nullptr) {
    impl->context->Release();
    impl->context = nullptr;
  }
  if (impl->device != nullptr) {
    impl->device->Release();
    impl->device = nullptr;
  }

  RECT client;
  std::memset(&client, 0, sizeof(client));
  GetClientRect(impl->window, &client);
  const int width = client.right - client.left;
  const int height = client.bottom - client.top;
  if (width <= 0 || height <= 0)
    return false;
  if (!CreateDevice(impl, width, height, nullptr))
    return false;
  if (!CreateRenderTarget(impl, nullptr))
    return false;
  if (!CreatePipeline(impl, nullptr))
    return false;
  impl->device_lost = false;
  return true;
}

void ReleaseImpl(D3D11Screen::Impl* impl) {
  if (impl->canvas_shader != nullptr)
    impl->canvas_shader->Release();
  if (impl->canvas_view != nullptr)
    impl->canvas_view->Release();
  if (impl->canvas_texture != nullptr)
    impl->canvas_texture->Release();
  if (impl->blend != nullptr)
    impl->blend->Release();
  if (impl->sampler != nullptr)
    impl->sampler->Release();
  if (impl->atlas_view != nullptr)
    impl->atlas_view->Release();
  if (impl->vertex_buffer != nullptr)
    impl->vertex_buffer->Release();
  if (impl->input_layout != nullptr)
    impl->input_layout->Release();
  if (impl->pixel_shader != nullptr)
    impl->pixel_shader->Release();
  if (impl->vertex_shader != nullptr)
    impl->vertex_shader->Release();
  if (impl->target != nullptr)
    impl->target->Release();
  if (impl->swap_chain != nullptr)
    impl->swap_chain->Release();
  if (impl->context != nullptr)
    impl->context->Release();
  if (impl->device != nullptr)
    impl->device->Release();
}

} // namespace

D3D11Screen::D3D11Screen() : impl_(std::make_unique<Impl>()) {}

D3D11Screen::~D3D11Screen() {
  if (impl_ == nullptr)
    return;
  ReleaseImpl(impl_.get());
  if (impl_->window != nullptr)
    DestroyWindow(impl_->window);
}

std::unique_ptr<WindowScreen> D3D11Screen::Create(std::string* error, int scale) {
  std::unique_ptr<D3D11Screen> screen(new D3D11Screen());
  Impl* impl = screen->impl_.get();
  if (scale < 1)
    scale = 1;
  if (scale > 8)
    scale = 8;

  // Scale for the display DPI so the window is legible on high-DPI monitors. The
  // default system DPI is 96; querying the device caps works on every Windows
  // version the project targets.
  HDC screen_dc = GetDC(nullptr);
  const int dpi = screen_dc != nullptr ? GetDeviceCaps(screen_dc, LOGPIXELSX) : 96;
  if (screen_dc != nullptr)
    ReleaseDC(nullptr, screen_dc);
  const int requested = scale;
  scale = (dpi > 0 ? (dpi * requested + 48) / 96 : requested);

  // Rasterise the atlas at the scaled cell size, so glyphs stay crisp instead of
  // being magnified from an 8x16 bitmap by the sampler.
  impl->atlas = RenderGlyphAtlas(8 * scale, 16 * scale);
  if (!impl->atlas.ok) {
    if (error != nullptr)
      *error = "no GDI font available for the glyph atlas";
    return nullptr;
  }
  if (!EnsureWindowClass()) {
    if (error != nullptr)
      *error = "window class registration failed";
    return nullptr;
  }

  const int client_width = kCols * impl->atlas.glyph_width;
  const int client_height = kRows * impl->atlas.glyph_height;
  RECT rect = {0, 0, client_width, client_height};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
  impl->window = CreateWindowExW(0, kWindowClass, kWindowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                 CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
                                 nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
  if (impl->window == nullptr) {
    if (error != nullptr)
      *error = "CreateWindowExW failed";
    return nullptr;
  }
  SetWindowLongPtrW(impl->window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
  ShowWindow(impl->window, SW_SHOW);
  UpdateWindow(impl->window);

  if (!CreateDevice(impl, client_width, client_height, error))
    return nullptr;
  if (!CreateRenderTarget(impl, error))
    return nullptr;
  if (!CreatePipeline(impl, error))
    return nullptr;
  impl->owner = screen.get();
  return screen;
}

bool D3D11Screen::Closed() const {
  return impl_ == nullptr || impl_->closed;
}

bool D3D11Screen::ReadBackCanvasPixel(int canvas_x, int canvas_y, std::uint8_t* rgba) {
  if (impl_ == nullptr || impl_->target == nullptr || rgba == nullptr)
    return false;
  const screen::Canvas& canvas = Graphics();
  if (canvas_x < 0 || canvas_y < 0 || canvas_x >= canvas.Width() || canvas_y >= canvas.Height())
    return false;

  // The same letterbox the presenter uses, so a canvas pixel maps to the client pixel
  // it is drawn at.
  RECT client;
  std::memset(&client, 0, sizeof(client));
  GetClientRect(impl_->window, &client);
  const int client_width = client.right - client.left;
  const int client_height = client.bottom - client.top;
  if (client_width <= 0 || client_height <= 0)
    return false;
  const float aspect = static_cast<float>(DisplayAspect());
  int frame_width = client_width;
  int frame_height = static_cast<int>(static_cast<float>(client_width) / aspect);
  if (frame_height > client_height) {
    frame_height = client_height;
    frame_width = static_cast<int>(static_cast<float>(client_height) * aspect);
  }
  const int left = (client_width - frame_width) / 2;
  const int top = (client_height - frame_height) / 2;
  const int x = left + static_cast<int>((canvas_x + 0.5) * frame_width / canvas.Width());
  const int y = top + static_cast<int>((canvas_y + 0.5) * frame_height / canvas.Height());

  ID3D11Texture2D* back_buffer = nullptr;
  if (FAILED(impl_->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                          reinterpret_cast<void**>(&back_buffer)))) {
    return false;
  }
  D3D11_TEXTURE2D_DESC description;
  back_buffer->GetDesc(&description);
  if (x < 0 || y < 0 || x >= static_cast<int>(description.Width) ||
      y >= static_cast<int>(description.Height)) {
    back_buffer->Release();
    return false;
  }
  D3D11_TEXTURE2D_DESC staging = description;
  staging.Usage = D3D11_USAGE_STAGING;
  staging.BindFlags = 0;
  staging.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  staging.MiscFlags = 0;
  ID3D11Texture2D* readable = nullptr;
  bool ok = false;
  if (SUCCEEDED(impl_->device->CreateTexture2D(&staging, nullptr, &readable))) {
    // Presents no longer wait for vertical sync, so the draw may still be queued when the
    // copy is issued. Flush submits it; without this the readback sometimes saw an empty
    // buffer and reported black.
    impl_->context->Flush();
    impl_->context->CopyResource(readable, back_buffer);
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(impl_->context->Map(readable, 0, D3D11_MAP_READ, 0, &mapped))) {
      const auto* row = static_cast<const std::uint8_t*>(mapped.pData) +
                        static_cast<std::size_t>(y) * mapped.RowPitch +
                        static_cast<std::size_t>(x) * 4;
      rgba[0] = row[0];
      rgba[1] = row[1];
      rgba[2] = row[2];
      rgba[3] = row[3];
      impl_->context->Unmap(readable, 0);
      ok = true;
    }
    readable->Release();
  }
  back_buffer->Release();
  return ok;
}

bool D3D11Screen::TakeMouseClick(MouseClick* click) {
  if (impl_ == nullptr || click == nullptr || impl_->clicks.empty())
    return false;
  *click = impl_->clicks.front();
  impl_->clicks.erase(impl_->clicks.begin());
  return true;
}

void D3D11Screen::InjectClick(int canvas_x, int canvas_y, int button) {
  if (impl_ == nullptr)
    return;
  MouseClick click;
  click.x = canvas_x;
  click.y = canvas_y;
  click.button = button;
  impl_->clicks.push_back(click);
}

bool D3D11Screen::TakeBreak() {
  if (impl_ == nullptr)
    return false;
  // Ctrl-C arrives as WM_CHAR 0x03, queued like any other key. It is consumed here rather
  // than left for INKEY$, so it interrupts a run the way a control handler does on the
  // console. Escape is deliberately NOT a break: a program that wants to react to it polls
  // INKEY$ and decides for itself.
  for (std::size_t index = 0; index < impl_->keys.size(); ++index) {
    if (impl_->keys[index] == 0x03) {
      impl_->keys.erase(impl_->keys.begin() + static_cast<std::ptrdiff_t>(index));
      return true;
    }
  }
  return false;
}

int D3D11Screen::GlyphWidth() const {
  return impl_ == nullptr ? 8 : impl_->atlas.glyph_width;
}

int D3D11Screen::GlyphHeight() const {
  return impl_ == nullptr ? 16 : impl_->atlas.glyph_height;
}

void D3D11Screen::PumpMessages() {
  if (impl_ == nullptr || impl_->closed)
    return;
  MSG message;
  while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
}

void D3D11Screen::OnGridChanged() {
  // A mode can change the grid and the canvas (SCREEN 720 is 160x45 and 1280x720), so
  // the window has to follow, otherwise the canvas is shown letterboxed into a window
  // sized for the previous mode.
  if (impl_ == nullptr || impl_->window == nullptr)
    return;
  const int client_width = Width() * impl_->atlas.glyph_width;
  const int client_height = Height() * impl_->atlas.glyph_height;
  if (client_width <= 0 || client_height <= 0)
    return;
  RECT rect = {0, 0, client_width, client_height};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
  SetWindowPos(impl_->window, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void D3D11Screen::Present() {
  if (impl_ == nullptr || impl_->context == nullptr || impl_->closed || impl_->device_lost)
    return;

  PumpMessages();
  if (impl_->closed)
    return;

  // Deferred resize: drop the target, resize the buffers, rebuild the target.
  if (impl_->resize_pending && impl_->pending_width > 0 && impl_->pending_height > 0) {
    impl_->resize_pending = false;
    if (impl_->target != nullptr) {
      impl_->context->OMSetRenderTargets(0, nullptr, nullptr);
      impl_->target->Release();
      impl_->target = nullptr;
    }
    if (SUCCEEDED(impl_->swap_chain->ResizeBuffers(0, impl_->pending_width, impl_->pending_height,
                                                   DXGI_FORMAT_UNKNOWN, 0))) {
      CreateRenderTarget(impl_.get(), nullptr);
    }
  }
  if (impl_->target == nullptr)
    return; // a failed resize skips this frame

  // Stage timings, filled in where the canvas is uploaded (see PresentTiming).
  double convert_begin = 0.0;
  double convert_end = 0.0;
  double upload_end = 0.0;

  // Clear and set the viewport before anything is drawn: the canvas blit and the
  // text overlay both land on the cleared surface, rather than the clear wiping the
  // canvas out afterwards.
  const Color background = BackgroundColor(CellAttrAt(0, 0));
  const FLOAT clear[4] = {ToUnit(background.r), ToUnit(background.g), ToUnit(background.b), 1.0f};
  impl_->context->OMSetRenderTargets(1, &impl_->target, nullptr);
  impl_->context->ClearRenderTargetView(impl_->target, clear);
  {
    D3D11_VIEWPORT viewport;
    std::memset(&viewport, 0, sizeof(viewport));
    // The viewport follows the actual client area, so a user resize keeps the whole
    // grid visible (positions are in normalised device coordinates).
    RECT client;
    std::memset(&client, 0, sizeof(client));
    GetClientRect(impl_->window, &client);
    viewport.Width = static_cast<FLOAT>(client.right - client.left);
    viewport.Height = static_cast<FLOAT>(client.bottom - client.top);
    viewport.MaxDepth = 1.0f;
    impl_->context->RSSetViewports(1, &viewport);
  }

  // Graphics modes (M12 phase 4): blit the canvas first, then overlay the text
  // glyphs without their background quads so the drawing shows through.
  const bool graphics = GraphicsMode() != screen::Screen::kTextMode;
  if (graphics && impl_->canvas_shader != nullptr) {
    const screen::Canvas& canvas = Graphics();
    if (impl_->canvas_texture == nullptr || impl_->canvas_width != canvas.Width() ||
        impl_->canvas_height != canvas.Height()) {
      if (impl_->canvas_texture != nullptr) {
        impl_->canvas_texture->Release();
        impl_->canvas_texture = nullptr;
      }
      if (impl_->canvas_view != nullptr) {
        impl_->canvas_view->Release();
        impl_->canvas_view = nullptr;
      }
      D3D11_TEXTURE2D_DESC description;
      std::memset(&description, 0, sizeof(description));
      description.Width = static_cast<UINT>(canvas.Width());
      description.Height = static_cast<UINT>(canvas.Height());
      description.MipLevels = 1;
      description.ArraySize = 1;
      description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      description.SampleDesc.Count = 1;
      description.Usage = D3D11_USAGE_DYNAMIC;
      description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      if (SUCCEEDED(
              impl_->device->CreateTexture2D(&description, nullptr, &impl_->canvas_texture))) {
        impl_->device->CreateShaderResourceView(impl_->canvas_texture, nullptr,
                                                &impl_->canvas_view);
        impl_->canvas_width = canvas.Width();
        impl_->canvas_height = canvas.Height();
      }
    }
    if (impl_->canvas_texture != nullptr && impl_->canvas_view != nullptr) {
      convert_begin = PresentTiming::Enabled() ? NowMicroseconds() : 0.0;
      const std::vector<std::uint8_t> rgba = screen::CanvasToRgba(canvas, PaletteRgb());
      convert_end = PresentTiming::Enabled() ? NowMicroseconds() : 0.0;
      D3D11_MAPPED_SUBRESOURCE mapped;
      if (SUCCEEDED(
              impl_->context->Map(impl_->canvas_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        const std::size_t row_bytes = static_cast<std::size_t>(canvas.Width()) * 4;
        for (int y = 0; y < canvas.Height(); ++y) {
          std::memcpy(static_cast<std::uint8_t*>(mapped.pData) +
                          static_cast<std::size_t>(y) * mapped.RowPitch,
                      rgba.data() + static_cast<std::size_t>(y) * row_bytes, row_bytes);
        }
        impl_->context->Unmap(impl_->canvas_texture, 0);
      }
      upload_end = PresentTiming::Enabled() ? NowMicroseconds() : 0.0;
      // One quad covering the largest 4:3 area of the client region, drawn with the
      // blit shader. The dialect's graphics modes are 4:3 displays (320x200 on a 4:3
      // screen, and the CIRCLE aspect correction assumes it), so stretching the canvas
      // to a differently shaped window would distort every figure.
      RECT client;
      std::memset(&client, 0, sizeof(client));
      GetClientRect(impl_->window, &client);
      const float client_width = static_cast<float>(client.right - client.left);
      const float client_height = static_cast<float>(client.bottom - client.top);
      const float kDisplayAspect = static_cast<float>(DisplayAspect());
      float half_x = 1.0f;
      float half_y = 1.0f;
      if (client_width > 0.0f && client_height > 0.0f) {
        if (client_width / client_height > kDisplayAspect) {
          half_x = (client_height * kDisplayAspect) / client_width;
        } else {
          half_y = (client_width / kDisplayAspect) / client_height;
        }
      }
      const RenderVertex quad[4] = {
          {-half_x, half_y, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f},
          {half_x, half_y, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f},
          {-half_x, -half_y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
          {half_x, -half_y, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
      };
      ID3D11Buffer* blit = nullptr;
      D3D11_BUFFER_DESC description;
      std::memset(&description, 0, sizeof(description));
      description.ByteWidth = sizeof(quad);
      description.Usage = D3D11_USAGE_DEFAULT;
      description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
      D3D11_SUBRESOURCE_DATA data;
      std::memset(&data, 0, sizeof(data));
      data.pSysMem = quad;
      if (SUCCEEDED(impl_->device->CreateBuffer(&description, &data, &blit))) {
        const UINT stride = sizeof(RenderVertex);
        const UINT offset = 0;
        impl_->context->IASetInputLayout(impl_->input_layout);
        impl_->context->IASetVertexBuffers(0, 1, &blit, &stride, &offset);
        impl_->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        impl_->context->VSSetShader(impl_->vertex_shader, nullptr, 0);
        impl_->context->PSSetShader(impl_->canvas_shader, nullptr, 0);
        impl_->context->PSSetShaderResources(1, 1, &impl_->canvas_view);
        impl_->context->PSSetSamplers(0, 1, &impl_->sampler);
        const FLOAT blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        impl_->context->OMSetBlendState(nullptr, blend_factor, 0xFFFFFFFF);
        impl_->context->Draw(4, 0);
        blit->Release();
        // Restore the text pipeline for the overlay below.
        const FLOAT text_blend[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        impl_->context->OMSetBlendState(impl_->blend, text_blend, 0xFFFFFFFF);
      }
    }
  }

  // Two quads per cell: a background sampled from the solid block glyph (fully
  // inked, so the shader paints the attribute's background colour) and the
  // glyph itself. One shader handles both.
  std::vector<RenderVertex> vertices;
  vertices.reserve(static_cast<std::size_t>(Width()) * Height() * 8);
  const auto push_quad = [&vertices](const GlyphQuad& quad, Color color) {
    const float r = ToUnit(color.r);
    const float g = ToUnit(color.g);
    const float b = ToUnit(color.b);
    const float a = ToUnit(color.a);
    vertices.push_back(
        {quad.top_left.x, quad.top_left.y, quad.top_left.u, quad.top_left.v, r, g, b, a});
    vertices.push_back(
        {quad.top_right.x, quad.top_right.y, quad.top_right.u, quad.top_right.v, r, g, b, a});
    vertices.push_back({quad.bottom_left.x, quad.bottom_left.y, quad.bottom_left.u,
                        quad.bottom_left.v, r, g, b, a});
    vertices.push_back({quad.bottom_right.x, quad.bottom_right.y, quad.bottom_right.u,
                        quad.bottom_right.v, r, g, b, a});
  };

  for (int row = 0; row < Height(); ++row) {
    for (int column = 0; column < Width(); ++column) {
      const std::uint8_t attribute = CellAttrAt(row, column);
      // The cursor cell is drawn in inverted video, which is what a text-mode
      // cursor looks like and needs no extra resources.
      const bool cursor_cell = CursorVisible() && (row == CursorRow() && column == CursorCol());
      const Color background_color =
          cursor_cell ? ForegroundColor(attribute) : BackgroundColor(attribute);
      const Color glyph_color =
          cursor_cell ? BackgroundColor(attribute) : ForegroundColor(attribute);
      // In a graphics mode the canvas is already behind the text, so cells drawn as spaces
      // must not paint an opaque background over it. The cursor cell is the exception: its
      // block is the only thing marking where input goes, and an empty field is a space, so
      // skipping it in a graphics mode left the cursor invisible.
      if (!graphics || cursor_cell) {
        push_quad(CellQuad(column, row, Width(), Height(), 0xDB), background_color);
      }
      const wchar_t cell = CellAt(row, column);
      const unsigned char glyph = UnicodeToCp437(cell == 0 ? L' ' : cell);
      if (!graphics || glyph != ' ') {
        push_quad(CellQuad(column, row, Width(), Height(), glyph), glyph_color);
      }
    }
  }

  const UINT vertex_count = static_cast<UINT>(vertices.size());
  // With no text there is nothing to upload, but a graphics mode must still present
  // the canvas: a program that draws without printing has an empty text grid, and
  // returning here rendered it as a blank window.
  if (vertex_count == 0 && !graphics)
    return;
  const UINT needed_bytes = static_cast<UINT>(vertices.size() * sizeof(RenderVertex));

  if (vertex_count > 0 &&
      (impl_->vertex_buffer == nullptr || needed_bytes > impl_->vertex_capacity)) {
    if (impl_->vertex_buffer != nullptr) {
      impl_->vertex_buffer->Release();
      impl_->vertex_buffer = nullptr;
    }
    D3D11_BUFFER_DESC description;
    std::memset(&description, 0, sizeof(description));
    description.ByteWidth = needed_bytes;
    description.Usage = D3D11_USAGE_DYNAMIC;
    description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(impl_->device->CreateBuffer(&description, nullptr, &impl_->vertex_buffer))) {
      impl_->device_lost = true;
      return;
    }
    impl_->vertex_capacity = needed_bytes;
  }

  if (vertex_count > 0) {
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(impl_->context->Map(impl_->vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
      return;
    }
    std::memcpy(mapped.pData, vertices.data(), needed_bytes);
    impl_->context->Unmap(impl_->vertex_buffer, 0);
  }

  const UINT stride = sizeof(RenderVertex);
  const UINT offset = 0;
  if (vertex_count > 0) {
    impl_->context->IASetInputLayout(impl_->input_layout);
    impl_->context->IASetVertexBuffers(0, 1, &impl_->vertex_buffer, &stride, &offset);
    impl_->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    impl_->context->VSSetShader(impl_->vertex_shader, nullptr, 0);
    impl_->context->PSSetShader(impl_->pixel_shader, nullptr, 0);
    impl_->context->PSSetShaderResources(0, 1, &impl_->atlas_view);
    impl_->context->PSSetSamplers(0, 1, &impl_->sampler);
    const FLOAT blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    impl_->context->OMSetBlendState(impl_->blend, blend_factor, 0xFFFFFFFF);

    // Four vertices per quad as a strip: two triangles.
    const UINT quad_count = vertex_count / 4;
    for (UINT quad = 0; quad < quad_count; ++quad) {
      impl_->context->Draw(4, quad * 4);
    }
  } // vertex_count > 0

  // Present without waiting for vertical sync. Waiting looked smooth but meant every
  // present cost a frame, and since each drawn instruction marks the screen dirty, a
  // plotting program spent almost all of its time waiting for the monitor: 10,000 PSETs
  // took 93 seconds. The 10 ms throttle in Screen::PresentIfDirty already limits how
  // often this runs, so the window stays responsive without pacing the interpreter.
  const HRESULT result = impl_->swap_chain->Present(0, 0);
  if (PresentTiming::Enabled()) {
    const double draw_end = NowMicroseconds();
    PresentTiming::Add(convert_end - convert_begin, upload_end - convert_end,
                       draw_end - upload_end);
  }
  if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET) {
    // Try once to rebuild; failing that the run ends cleanly instead of spinning
    // on a dead device.
    if (!RecreateDevice(impl_.get()))
      impl_->closed = true;
  } else if (FAILED(result)) {
    // A present that fails for any other reason leaves the window showing whatever the
    // desktop paints, so say so once and stop pretending to render: without this a
    // broken swapchain looks exactly like a program that draws nothing.
    static bool reported = false;
    if (!reported) {
      reported = true;
      std::fprintf(stderr, "newbasic: the windowed backend cannot present (0x%08lX)\n",
                   static_cast<unsigned long>(result));
    }
    impl_->closed = true;
  }
}

bool D3D11Screen::TryGetKey(wchar_t* key) {
  if (impl_ == nullptr)
    return false;
  MSG message;
  while (PeekMessageW(&message, impl_->window, 0, 0, PM_REMOVE) != 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  if (impl_->keys.empty())
    return false;
  *key = impl_->keys.front();
  impl_->keys.erase(impl_->keys.begin());
  return true;
}

namespace {
// Line editing over the window message queue: TryGetKey pumps messages, so this blocks
// without burning a core.
class D3DKeySource : public KeySource {
public:
  explicit D3DKeySource(D3D11Screen* owner) : owner_(owner) {}
  bool WaitKey(wchar_t* key) override {
    for (;;) {
      if (owner_->TryGetKey(key))
        return true;
      if (owner_->Closed())
        return false;
      Sleep(5);
    }
  }

private:
  D3D11Screen* owner_;
};
} // namespace

std::wstring D3D11Screen::ReadLine() {
  if (impl_ == nullptr)
    return {};
  // The editor owns the echo, so the grid holds what was typed and a repaint keeps it.
  D3DKeySource keys(this);
  std::wstring line;
  if (!EditLine(keys, &line))
    return {};
  return line;
}

void D3D11Screen::InjectKeys(const std::wstring& keys) {
  // Feeds the same queue the window procedure fills, so a scripted session and a typed one
  // take exactly the same path. Used by --keys to drive the terminal without a keyboard.
  if (impl_ == nullptr)
    return;
  impl_->keys.insert(impl_->keys.end(), keys.begin(), keys.end());
}

} // namespace newbasic::screen
