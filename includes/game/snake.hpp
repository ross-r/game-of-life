#pragma once

#include <cstdint>
#include <Vector>

#include <game/types.hpp>
#include <game/colour.hpp>

#include <game/bot.hpp>

namespace game {
  
  // How much of the window space the game should take up.
  constexpr float GAME_AREA_SCALE = 0.85F;

  constexpr float TILE_SIZE = 32.F;
  
  // Alternating tile colours.
  const Colour tile_colour1 = Colour( 0xFFA2D149 );
  const Colour tile_colour2 = Colour( 0xFFAAD751 );

  // The snakes colours.
  const Colour snake_colour1 = Colour( 0xFF4775EA );
  const Colour snake_colour2 = Colour( 0xFF205163 );

  // The background colour.
  const Colour bg_colour = Colour( 0xFF578A34 );

  // How long the snake is by default.
  constexpr size_t MIN_SNAKE_LENGTH = 3;

  enum : uint8_t {
    dir_north = 0,
    dir_east,
    dir_south,
    dir_west
  };

  class Snake {
  private:
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

    Bot m_bot;

  public:
    Snake();

    void init( const app::Window& window );

    void update( app::Application& app, const double t, const double dt );

    void draw( const app::Application& app, const app::Window& window );

    const SizeI grid_size() const {
      return SizeI{ m_max_tiles_x, m_max_tiles_y };
    };

    const std::vector< Tile > snake_segments() const {
      // TODO: refactor code and omit "m_snake_head" - merge it into m_tail_segs.
      std::vector< Tile > segments{ m_snake_head };
      segments.insert( segments.end(), m_tail_segs.begin(), m_tail_segs.end() );
      return segments;
    }

    const std::vector< Tile >& fruits() const {
      return m_fruits;
    }

    const uint8_t get_dir( const Tile& p1, const Tile& p2, const float angle_offset = 0.F ) const;

    void set_dir( const uint8_t dir );

  private:
    const Point tile_to_screen( const Tile& tile ) const;

    void update_direction();
    void update_position();

    void spawn_fruit();
    void fruits_check_collisions();

    bool check_game_over_conditions();

    void draw_fruits();
    void draw_snake();
  };

}