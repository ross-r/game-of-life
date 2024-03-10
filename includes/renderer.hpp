#pragma once

#include <dxgi.h>
#include <d3d11.h>

namespace app {

  // Forward declarations to avoid pointless includes.
  class Window;
  
  class Renderer {
  private:
    // Reference to the window the renderer is initialized for.
    Window* m_window;

    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    ID3D11RenderTargetView* m_render_target;
    IDXGISwapChain* m_swapchain;

    void* m_imgui_context;

  private:
    float m_clear_color[ 4 ];

  private:
    bool init_render_target();
    bool init_imgui();

  public:
    Renderer();
    
    bool initialize( const app::Window& window );
    void shutdown();

  public:
    void begin();
    void end();

    void set_clear_color( const float* clear_color );

    void* imgui_context() const;

  public:
    ID3D11Device* device() const {
      return m_device;
    }

    ID3D11DeviceContext* context() const {
      return m_context;
    }

    ID3D11RenderTargetView* render_target() const {
      return m_render_target;
    }

    IDXGISwapChain* swapchain() const {
      return m_swapchain;
    }
  };

}