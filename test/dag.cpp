#include <catch.hpp>
#include <percy/dag.hpp>
#include <fmt/format.h>
#include <iostream>

using namespace percy;

template<int FI>
void to_dot( dag<FI> const& d, std::ostream& os )
{
  /* converts graph ids to dot ids */
  auto const to_dot_id = []( int id ) { return id + 1; };

  os << "graph{\n";
  os << "rankdir = BT\n";
  d.foreach_input( [&os,&to_dot_id]( int index ){
      os << fmt::format( "x{} [shape=none, label=<x<sub>{}</sub>>];\n", to_dot_id( index ), to_dot_id( index ) );
    });

  os << "node [shape=circle];\n";
  d.foreach_vertex( [&d,&os,&to_dot_id]( auto const& v, int index ){
      const auto dot_idx = to_dot_id( d.num_inputs() + index );
      os << fmt::format( "x{} [label=<x<sub>{}</sub>>];\n", dot_idx, dot_idx );

      d.foreach_fanin( v, [&os, &dot_idx,&to_dot_id]( auto const& fi ){
          os << fmt::format( "x{} -- x{};\n", to_dot_id( fi ), dot_idx );
        });
    });

  /* Group inputs on the same level. */
  os << "{rank = same; ";
  for ( int i = 0; i < d.num_inputs(); ++i )
    os << "x" << i+1 << "; ";
  os << "}\n";

  /* Add invisible edges between PIs to enforce order. */
  os << "edge[style=invisible];\n";
  for ( int i = d.num_inputs(); i > 1; --i )
    os << "x" << i-1 << " -- x" << i << ";\n";
  os << "}\n";
}

TEST_CASE( "binary_dag", "[dag]" )
{
  int32_t num_inputs = 2u;
  int32_t num_vertices = 2u;

  binary_dag d( num_inputs, num_vertices );
  CHECK( d.num_inputs() == num_inputs );
  CHECK( d.num_vertices() == num_vertices );

  d.set_vertex( 0, 0, 1 );
  d.set_vertex( 1, 2, 1 );

  to_dot( d, std::cout );
}

TEST_CASE( "ternary_dag", "[dag]" )
{
  int32_t num_inputs = 3u;
  int32_t num_vertices = 2u;

  ternary_dag d( num_inputs, num_vertices );
  CHECK( d.num_inputs() == num_inputs );
  CHECK( d.num_vertices() == num_vertices );

  d.set_vertex( 0, std::vector{ 0, 1, 2 } );
  d.set_vertex( 1, std::vector{ 3, 1, 2 } );

  to_dot( d, std::cout );
}
