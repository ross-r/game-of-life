#pragma once

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <vector>

#include <game/types.hpp>
#include <game/colour.hpp>
#include <game/snake.hpp>

// forward delcarations.
namespace app {
  class Application;
  class Window;
}

namespace game {

  class Game {
  private:
    bool m_draw_metrics;
    bool m_paused;

    Snake m_snake;

  public:
    Game();

    void init( const app::Application& app, const app::Window& window );

    void resize( const app::Application& app, const app::Window& window );

    void update( app::Application& app, const double t, const double dt );

    void draw( const app::Application& app, const app::Window& window );
  };

}