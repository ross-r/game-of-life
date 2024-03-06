#include <game/snake.hpp>

#include <ext/imgui/imgui.h>

#include <window.hpp>
#include <application.hpp>

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>

#include <random>

namespace {
  const ImDrawFlags dir_to_rounding_flags( const uint8_t direction ) {
    ImDrawFlags flags = 0;

    switch( direction ) {
    case game::dir_north:
      flags = ImDrawFlags_RoundCornersTop;
      break;
    case game::dir_east:
      flags = ImDrawFlags_RoundCornersRight;
      break;
    case game::dir_south:
      flags = ImDrawFlags_RoundCornersBottom;
      break;
    case game::dir_west:
      flags = ImDrawFlags_RoundCornersLeft;
      break;
    }

    return flags;
  }
}

const auto& random_number = []( const int min, const int max ) -> int {
  // https://en.cppreference.com/w/cpp/numeric/random
  static std::random_device r;

  std::default_random_engine e1( r() );
  std::uniform_int_distribution<int> uniform_dist( min, max );
  return uniform_dist( e1 );
};

game::Snake::Snake() : m_bot( *this ) {
  m_max_tiles_x = 0;
  m_max_tiles_y = 0;

  m_screen_offset = {};
  m_snake_head = { 8,0 };
  m_snake_dir = dir_east;
  m_tail_segs.resize( MIN_SNAKE_LENGTH );

  m_next_position_update = 0.0;

  m_score = 0;
  m_high_score = 0;
  m_game_over = false;
}

void game::Snake::init( const app::Window& window ) {
  m_max_tiles_x = ( int ) floor( ( float ) ( window.width() / 32 ) * GAME_AREA_SCALE );
  m_max_tiles_y = ( int ) floor( ( float ) ( window.height() / 32 ) * GAME_AREA_SCALE );

  m_screen_offset.x = floor( ( window.width() - ( m_max_tiles_x * TILE_SIZE ) ) / 2.F );
  m_screen_offset.y = floor( ( window.height() - ( m_max_tiles_y * TILE_SIZE ) ) / 2.F );

  m_fruits.push_back( { m_max_tiles_x / 2, m_max_tiles_y / 2 } );
}

void game::Snake::update( app::Application& app, const double t, const double dt ) {
  //
  // Check game over conditions before anything.
  //
  if( m_game_over || check_game_over_conditions() ) {
    return;
  }

  //
  // Always update the direction as so as possible so it feels responsive.
  //
  update_direction();

  //
  // Update the snake every X ticks.
  //
  constexpr int MOVE_TICKS = 55;

  if( t < m_next_position_update ) {
    return;
  }

  m_bot.update();

  // Pop the end of the snake
  // Push to the front of the snake
  if( !m_tail_segs.empty() ) {
    m_tail_segs.pop_back();
    m_tail_segs.insert( m_tail_segs.begin(), m_snake_head );
  }

  update_position();

  fruits_check_collisions();

  m_next_position_update = t + ( ( 60.0 - MOVE_TICKS ) / 60.0 );
}

void game::Snake::draw( const app::Application& app, const app::Window& window ) {
  ImDrawList* draw_list = ImGui::GetForegroundDrawList();

  const float window_center_x = ( window.width() / 2 );
  const float window_center_y = ( window.height() / 2 );

  draw_list->AddRectFilled( { 0.F, 0.F }, { ( float ) window.width(), ( float ) window.height() }, bg_colour.argb() );

  float current_x = m_screen_offset.x;
  float current_y = m_screen_offset.y;

  for( int i = 0; i < m_max_tiles_x; ++i ) {
    for( int j = 0; j < m_max_tiles_y; ++j ) {
      const auto& colour = ( i % 2 == j % 2 ) ? tile_colour2 : tile_colour1;

      draw_list->AddRectFilled( { current_x, current_y }, { current_x + TILE_SIZE, current_y + TILE_SIZE }, colour.argb() );

      current_y += TILE_SIZE;
    }

    current_x += TILE_SIZE;
    current_y = m_screen_offset.y;
  }

  draw_fruits();
  draw_snake();
}

const game::Point game::Snake::tile_to_screen( const Tile& tile ) const {
  return {
    m_screen_offset.x + ( std::min( std::max( 0, tile.x ), m_max_tiles_x - 1 ) * TILE_SIZE ),
    m_screen_offset.y + ( std::min( std::max( 0, tile.y ), m_max_tiles_y - 1 ) * TILE_SIZE )
  };
}

const uint8_t game::Snake::get_dir( const Tile& p1, const Tile& p2, const float angle_offset ) const {
  const float dir = atan2f( p2.y - p1.y, p2.x - p1.x ) * ( 180.F / M_PI ) + angle_offset;
  const uint16_t angle = static_cast< uint16_t >( fmod( dir < 0 ? dir + 360.F : dir, 360.F ) );

  // TODO: Check to make sure the 45 deg (diagonal) movement translates nicely into 0/90/180/270 in gameplay.
  //       (does it feel smooth and correct)
  switch( angle ) {
  case 0:
  case 45:
    return dir_east;
  case 135:
  case 180:
    return dir_west;
  case 90:
  case 315:
    return dir_south;
  case 225:
  case 270:
    return dir_north;
  }

  // This statement will never be met.
  return dir_north;
}

void game::Snake::set_dir( const uint8_t dir ) {
  m_snake_dir = dir;
}

void game::Snake::update_direction() {
  // No bounds checks are needed, this is purely to update the direction.
  //
  // NOTE: This could be completely rewritten.
  //       There is no need to calculate the direction between two points.
  //       We could simply write each direction change to m_snake_dir manually.

  Tile next{ m_snake_head };

  if( ImGui::IsKeyDown( ImGuiKey_A ) ) {
    next.x = m_snake_head.x - 1;
  }

  if( ImGui::IsKeyDown( ImGuiKey_D ) ) {
    next.x = m_snake_head.x + 1;
  }

  if( ImGui::IsKeyDown( ImGuiKey_W ) ) {
    next.y = m_snake_head.y - 1;
  }

  if( ImGui::IsKeyDown( ImGuiKey_S ) ) {
    next.y = m_snake_head.y + 1;
  }

  // If there is no difference between the current and next position, we don't update the direction.
  if( next.x == m_snake_head.x && next.y == m_snake_head.y ) {
    return;
  }

  // Don't allow the direction to change back into the snakes tail.
  const Tile& prev = m_tail_segs.front();
  if( next == prev ) {
    return;
  }

  m_snake_dir = get_dir( m_snake_head, next );
}

void game::Snake::update_position() {
  switch( m_snake_dir ) {
  case dir_east:
    m_snake_head.x = m_snake_head.x + 1;
    break;
  case dir_west:
    m_snake_head.x = m_snake_head.x - 1;
    break;
  case dir_south:
    m_snake_head.y = m_snake_head.y + 1;
    break;
  case dir_north:
    m_snake_head.y = m_snake_head.y - 1;
    break;
  }
}

void game::Snake::spawn_fruit() {
  bool retry = false;

  // NOTE: This will eventually freeze the game if there are no positions available for the fruit to spawn.. but.. that just shouldn't happen.
  //       There would need to be x*y fruits at that point.
  // 
  // The below counter is just a safety measure for this.
  int attempts = 0;

  Tile tile{};

  do {
    tile.x = random_number( 0, m_max_tiles_x - 1 );
    tile.y = random_number( 0, m_max_tiles_y - 1 );

    // Check if the random tile would be inside of the snakes body.
    retry = tile == m_snake_head || std::find_if( m_tail_segs.begin(), m_tail_segs.end(), [&]( const Tile& other ) {
      return other == tile;
    } ) != m_tail_segs.end();

    // Check if this position would be a duplicate.
    retry = std::find_if( m_fruits.begin(), m_fruits.end(), [&]( const Tile& other ) {
      return other == tile;
    } ) != m_fruits.end();
  }
  while( retry && ++attempts < 69 );

  // Add a new fruit at a random position.
  m_fruits.push_back( tile );
}

void game::Snake::fruits_check_collisions() {
  // Check all fruits and see if the snake has eaten one.
  for( auto it = m_fruits.begin(); it != m_fruits.end(); ++it ) {
    const auto& fruit = *it;
    if( m_snake_head == fruit ) {

      // Delete the fruit, update the score, and spawn a new fruit.
      m_fruits.erase( it );

      ++m_score;

      spawn_fruit();

      // Add to the tail.
      m_tail_segs.insert( m_tail_segs.end(), m_tail_segs.back() );

      break;
    }
  }

  if( m_score > m_high_score ) m_high_score = m_score;
}

bool game::Snake::check_game_over_conditions() {
  const auto& it = std::find_if( m_tail_segs.begin(), m_tail_segs.end(), [&]( const Tile& other ) {
    return other == m_snake_head;
  } );

  if( it != m_tail_segs.end() ) {
    m_game_over = true;
  }

  if( m_snake_head.x < 0 || m_snake_head.x >= m_max_tiles_x ) {
    m_game_over = true;
  }

  if( m_snake_head.y < 0 || m_snake_head.y >= m_max_tiles_y ) {
    m_game_over = true;
  }

  return m_game_over;
}

void game::Snake::draw_fruits() {
  ImDrawList* draw_list = ImGui::GetForegroundDrawList();

  const Colour fruit_colour{ 0xFFE7471D };

  for( const auto& fruit : m_fruits ) {
    const auto& pos = tile_to_screen( fruit );
    draw_list->AddRectFilled( { pos.x, pos.y }, { pos.x + TILE_SIZE, pos.y + TILE_SIZE }, fruit_colour.argb(), 16.F );
  }
}

void game::Snake::draw_snake() {
  ImDrawList* draw_list = ImGui::GetForegroundDrawList();

  const auto& tail_seg = m_tail_segs.back();

  const auto& head = tile_to_screen( m_snake_head );
  const auto& tail = tile_to_screen( tail_seg );

  const ImDrawFlags draw_flags_head = dir_to_rounding_flags( m_snake_dir );
  const ImDrawFlags draw_flags_tail = dir_to_rounding_flags( get_dir( m_tail_segs.back(), m_tail_segs.at( m_tail_segs.size() - 2 ), 180.F ) );

  draw_list->AddRectFilled( { head.x - 1.F, head.y - 1.F }, { head.x + TILE_SIZE + 1.F, head.y + TILE_SIZE + 1.F }, snake_colour2.argb(), 16.F, draw_flags_head );
  draw_list->AddRectFilled( { tail.x - 1.F, tail.y - 1.F }, { tail.x + TILE_SIZE + 1.F, tail.y + TILE_SIZE + 1.F }, snake_colour2.argb(), 16.F, draw_flags_tail );

  for( size_t i{ 0 }; i < m_tail_segs.size() - 1; ++i ) {
    const auto& seg = m_tail_segs.at( i );
    const auto& pos = tile_to_screen( seg );
    draw_list->AddRectFilled( { pos.x - 1.F, pos.y - 1.F }, { pos.x + TILE_SIZE + 1.F, pos.y + TILE_SIZE + 1.F }, snake_colour2.argb() );
  }

  draw_list->AddRectFilled( { head.x, head.y }, { head.x + TILE_SIZE, head.y + TILE_SIZE }, snake_colour1.argb(), 16.F, draw_flags_head );
  draw_list->AddRectFilled( { tail.x, tail.y }, { tail.x + TILE_SIZE, tail.y + TILE_SIZE }, snake_colour1.argb(), 16.F, draw_flags_tail );

  for( size_t i{ 0 }; i < m_tail_segs.size() - 1; ++i ) {
    const auto& seg = m_tail_segs.at( i );
    const auto& pos = tile_to_screen( seg );
    draw_list->AddRectFilled( { pos.x, pos.y }, { pos.x + TILE_SIZE, pos.y + TILE_SIZE }, snake_colour1.argb() );
  }
}
