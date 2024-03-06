#include <game/pathfinder.hpp>

#include <memory>
#include <iostream>

game::PathFinder::PathFinder() {
  m_grid_size = {};
}

void game::PathFinder::init( const Vec2i size, const std::initializer_list<Tile> colliders ) {
  m_grid_size = size;
  set_colliders( colliders );
}

void game::PathFinder::set_colliders( const std::vector< Tile >& colliders ) {
  m_colliders.clear();
  m_colliders.insert( m_colliders.begin(), colliders.begin(), colliders.end() );
}

void game::PathFinder::set_colliders( const std::initializer_list< Tile > colliders ) {
  m_colliders.clear();
  m_colliders.insert( m_colliders.begin(), colliders.begin(), colliders.end() );
}

const game::PathFinder::tiles_t game::PathFinder::find( const Tile& src, const Tile& dst ) {
  tiles_t solution;

  nodes_t open_nodes;
  nodes_t closed_nodes;

  open_nodes.insert( open_nodes.begin(), std::make_shared< Node >( nullptr, src ) );

  while( !open_nodes.empty() ) {
    auto current_it = open_nodes.begin();
    auto current_node = *current_it;
    
    for( auto it = open_nodes.begin(); it != open_nodes.end(); ++it ) {
      auto node = *it;
      if( node->F < current_node->F ) {
        current_node = node;
        current_it = it;
      }
    }

    // Remove the current node from the open nodes list.
    open_nodes.erase( current_it );

    // Add the working node to the closed nodes list as we're done with it.
    closed_nodes.insert( closed_nodes.begin(), current_node );

    // Check to see if the current node has reached the destination node.
    if( current_node->tile == dst ) {
      // Solution is found - backtrack to find the path.

      auto current = current_node;
      while( current != nullptr ) {
        solution.push_back( current->tile );
        current = current->parent;
      }

      return solution;
    }

    // Generate child nodes of adjacent tiles.
    for( const auto& dir_offset : m_directions ) {
      // Create the child node and apply the directional offset to the current working tile.
      Node child(
        current_node,
        Tile{ current_node->tile.x + dir_offset.x, current_node->tile.y + dir_offset.y }
      );

      // Make sure this node doesn't collide with any colliders defined and that it is within the bounds of the grid.
      if( check_collision( child.tile ) ) {
        continue;
      }

      if( does_node_exist( closed_nodes, child ) ) {
        continue;
      }

      // Create the F, G, H, values..
      child.G = pow( current_node->tile.x - src.x, 2.F ) + pow( current_node->tile.y - src.y, 2.F );
      child.H = sqrt( child.tile.x * dst.x + child.tile.y * dst.y );
      child.F = child.G + child.H;

      if( does_node_exist( open_nodes, child ) ) {
        continue;
      }

      open_nodes.push_back( std::make_shared< Node >( child ) );
    }
  }

  return {};
}

const bool game::PathFinder::check_collision( const Tile& tile ) {
  if( tile.x < 0 || tile.y < 0 || tile.x >= m_grid_size.x || tile.y >= m_grid_size.y ) {
    return true;
  }

  if( m_colliders.empty() ) {
    return false;
  }

  const auto& it = std::find_if( m_colliders.begin(), m_colliders.end(), [&]( const Tile& collider ) {
    return tile == collider;
  } );

  return !( it == m_colliders.end() );
}

const bool game::PathFinder::does_node_exist( const nodes_t& nodes, const Node& node ) const {
  if( nodes.empty() ) {
    return false;
  }

  return std::find_if( nodes.begin(), nodes.end(), [ & ]( const std::shared_ptr< Node >& n ) {
    return ( node.tile == n->tile );
  } ) != nodes.end();
}