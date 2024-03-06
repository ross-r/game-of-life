#pragma once

#include <array>
#include <vector>
#include <memory>
#include <functional>

#include <game/types.hpp>


namespace game {

  class PathFinder {
    struct Node {
      std::shared_ptr< Node > parent;

      Tile tile;

      // F = total cost of the node
      // G is the distance between this node and the start node
      // H is the heuristic, estimated distance from the current node to the end node
      float F, G, H;

      Node( std::shared_ptr< Node > p, const Tile& t ) : parent( p ), tile( t ) {
        F = G = H = 0.F;
      }

      Node( const Tile& t ) : Node( nullptr, t ) {}
    };

  private:
    using tiles_t = std::vector< Tile >;
    using nodes_t = std::vector< std::shared_ptr< Node > >;

    // A list of move directions.
    // This doesn't support diagonal as we don't need it.
    std::array< Tile, 4 > m_directions = {
      Tile{ 0, 1 },
      Tile{ 1, 0 },
      Tile{ 0, -1 },
      Tile{ -1, 0 }
    };

    tiles_t m_colliders;
    Vec2i m_grid_size;

  public:
    PathFinder();

    void init( const Vec2i size, const std::initializer_list< Tile > colliders );

    void set_colliders( const std::vector< Tile >& colliders );
    void set_colliders( const std::initializer_list< Tile > colliders );

    const tiles_t find( const Tile& src, const Tile& dest );

  private:
    const bool check_collision( const Tile& tile );

    const bool does_node_exist( const nodes_t& nodes, const Node& node ) const;
  };

}