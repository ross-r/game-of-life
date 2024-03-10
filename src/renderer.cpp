#include <renderer.hpp>

#include <window.hpp>

#include <ext/imgui/imgui.h>
#include <imgui/imgui_impl_win32.hpp>
#include <imgui/imgui_impl_dx11.hpp>

#pragma comment( lib, "d3d11.lib" )

app::Renderer::Renderer() :
  m_window{},
  m_device{},
  m_context{},
  m_render_target{},
  m_swapchain{},
  m_imgui_context{},
  m_clear_color{ 0.F, 0.F, 0.F, 1.F } {}

bool app::Renderer::init_render_target() {
  // Obtain the back buffer pointer and initialize the render target.
  ID3D11Texture2D* buffer = nullptr;
  if( FAILED( m_swapchain->GetBuffer( 0, IID_PPV_ARGS( &buffer ) ) ) ) {
    return false;
  }

  // Get the back buffer texture description data.
  D3D11_TEXTURE2D_DESC texdesc{};
  buffer->GetDesc( &texdesc );

  // Create and set the viewport.
  D3D11_VIEWPORT viewport{};
  memset( &viewport, 0, sizeof( D3D11_VIEWPORT ) );
  viewport.MinDepth = 0;
  viewport.MaxDepth = 1;
  viewport.Width = static_cast< float >( texdesc.Width );
  viewport.Height = static_cast< float >( texdesc.Height );
  m_context->RSSetViewports( 1, &viewport );

  // Finally, create the render target.
  D3D11_RENDER_TARGET_VIEW_DESC rtvdesc = {};
  rtvdesc.Format = texdesc.Format;
  rtvdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  if( FAILED( m_device->CreateRenderTargetView( buffer, &rtvdesc, &m_render_target ) ) ) {
    buffer->Release();
    return false;
  }

  // GetBuffer increments ref count, so we need to release the pointer once we're done with it.
  buffer->Release();

  return true;
}

bool app::Renderer::init_imgui() {
  IMGUI_CHECKVERSION();

  // Create a context for the renderer instance and set it so ImGui knows which data to reference.
  m_imgui_context = ImGui::CreateContext();
  ImGui::SetCurrentContext( ( ImGuiContext* ) m_imgui_context );

  ImGuiIO& io = ImGui::GetIO(); ( void ) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsLight();

  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF( "VCR_OSD_MONO_1.001.ttf", 32.F );

  // Setup Platform/Renderer backends
  if( !ImGui_ImplWin32_Init( m_window->handle() ) ) {
    // TODO: raise error
    return false;
  }

  if( !ImGui_ImplDX11_Init( m_device, m_context ) ) {
    // TODO: raise error
    return false;
  }

  return true;
}

bool app::Renderer::initialize( const app::Window& window ) {
  m_window = const_cast< app::Window* >( &window );

  D3D_FEATURE_LEVEL feature_levels[] = {
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_11_1
  };

  DXGI_SWAP_CHAIN_DESC desc{};
  desc.BufferCount = 1;
  desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.BufferDesc.Width = 0; // window.width();
  desc.BufferDesc.Height = 0; // window.height();
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.OutputWindow = window.handle();
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Windowed = TRUE;

  // Allow the user to press ALT+ENTER to enter/exit fullscreen exclusive mode.
  desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  // TODO: Make class member.
  D3D_FEATURE_LEVEL feature_level;

  if( FAILED( D3D11CreateDeviceAndSwapChain(
    nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,
    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
    feature_levels,
    ARRAYSIZE( feature_levels ),
    D3D11_SDK_VERSION,
    &desc,
    &m_swapchain,
    &m_device,
    &feature_level,
    &m_context
  ) ) ) {
    return false;
  }

  // Initialize the render target we'll be using.
  if( !init_render_target() ) {
    // TODO: raise error
    return false;
  }

  // Initialize ImGui context for this render instance.
  if( !init_imgui() ) {
    // TODO: raise error
    return false;
  }

  return true;
}

void app::Renderer::shutdown() {
  m_window = nullptr;
  m_imgui_context = nullptr;

  if( m_device ) {
    m_device->Release();
    m_device = nullptr;
  }

  if( m_context ) {
    m_context->Release();
    m_context = nullptr;
  }

  if( m_swapchain ) {
    m_swapchain->Release();
    m_swapchain = nullptr;
  }

  if( m_render_target ) {
    m_render_target->Release();
    m_render_target = nullptr;
  }
}

void app::Renderer::begin() {
  m_context->OMSetRenderTargets( 1, &m_render_target, nullptr );
  m_context->ClearRenderTargetView( m_render_target, m_clear_color );

  ImGui::SetCurrentContext( ( ImGuiContext* ) m_imgui_context );
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}

void app::Renderer::end() {
  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );

  m_swapchain->Present( 1, 0 );
}

void app::Renderer::set_clear_color( const float* clear_color ) {
  m_clear_color[ 0 ] = clear_color[ 0 ];
  m_clear_color[ 1 ] = clear_color[ 1 ];
  m_clear_color[ 2 ] = clear_color[ 2 ];
  m_clear_color[ 3 ] = clear_color[ 3 ];
}

void* app::Renderer::imgui_context() const {
  return m_imgui_context;
}
