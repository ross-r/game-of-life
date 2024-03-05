#pragma once

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <vector>

#include <game/colour.hpp>

// forward delcarations.
namespace app {
  class Application;
  class Window;
}

namespace game {

  // How much of the window space the game should take up.
  const float GAME_AREA_SCALE = 0.85F;

  // Alternating tile colours.
  const Colour tile_colour1 = Colour( 0xFFA2D149 );
  const Colour tile_colour2 = Colour( 0xFFAAD751 );

  // The snakes colours.
  const Colour snake_colour = Colour( 0xFF4775EA );

  enum : uint8_t {
    dir_north = 0,
    dir_east,
    dir_south,
    dir_west
  };

  struct Tile {
    int x;
    int y;

    const bool operator==( const Tile& other ) const {
      return x == other.x && y == other.y;
    }
  };

  struct Point {
    float x;
    float y;
  };

  class Game {
  private:
    bool m_draw_metrics;
    bool m_paused;

    int m_max_tiles_x;
    int m_max_tiles_y;

    Point m_screen_offset;

    Tile m_snake_head;
    
    uint8_t m_snake_dir;
    std::vector< Tile > m_tail_segs;
    
    std::vector< Tile > m_fruits;

    int m_score;
    int m_high_score;

    double m_next_position_update;

    bool m_game_over;
    
  private:
    const Point tile_to_screen( const Tile& tile ) const;

    const uint8_t get_dir( const Tile& p1, const Tile& p2, const float angle_offset = 0.F ) const;

    const bool tile_intersects_with_snake( const Tile& tile ) const;

    void update_direction();
    void update_position();

    void spawn_fruit();
    void fruits_check_collisions();

    bool check_game_over_conditions();

    void draw_fruits();
    void draw_snake( const float lerp, const float lerp1 );

  public:
    Game();

    void init( const app::Application& app, const app::Window& window );

    void resize( const app::Application& app, const app::Window& window );

    void update( app::Application& app, const double t, const double dt );

    void draw( const app::Application& app, const app::Window& window );
  };

}