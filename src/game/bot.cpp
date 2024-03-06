#include <game/bot.hpp>
#include <game/snake.hpp>

game::Bot::Bot( Snake& snake ) : m_snake( snake ) {
  
}

void game::Bot::update() {

  const auto& snake_segments = m_snake.snake_segments();
  const auto& snake_head = snake_segments.front();

  // TODO: Select the closest fruit OR if there is no viable path try all available fruits.
  const auto& fruit = m_snake.fruits().front();

  // TODO: init function that takes in vector.
  m_path_finder.init( m_snake.grid_size(), {} );
  m_path_finder.set_colliders( snake_segments );

  const auto& path = m_path_finder.find( snake_head, fruit );
  if( path.empty() ) {
    // TODO: Hand off control back to the user or maybe implement a simple algorithm to make the snake
    //       go into a continuous circle until a solution can be found
    printf( "no solution\n" );
    return;
  }

  // Compute the direction from the snakes current head to the next path position.
  const auto& dir = m_snake.get_dir( snake_head, path[ path.size() - 2 ] );
  m_snake.set_dir( dir );
}
