#include <catch.hpp>
#include <copycat/chain/chain.hpp>
#include <copycat/chain/print.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/constructors.hpp>
#include <kitty/print.hpp>

using namespace copycat;

namespace copycat::detail
{
  template<>
  std::string label_to_string( std::string const& label )
  {
    return label;
  }
}

TEST_CASE( "Boolean chain with string operators", "[chain]" )
{
  chain<std::string, std::vector<int>> c;
  CHECK( c.num_inputs() == 0u );
  CHECK( c.num_steps() == 0u );

  auto const num_inputs = 3u;
  c.set_inputs( num_inputs ); // 1, 2, 3
  CHECK( c.num_inputs() == num_inputs );

  auto const s0 = c.add_step( "AND", {  1, 2 } );
  auto const s1 = c.add_step( "OR",  { -2, 3 } );
  auto const s2 = c.add_step( "XOR", { s0, s1 } );
  CHECK( s0 == 4 );
  CHECK( s1 == 5 );
  CHECK( s2 == 6 );
  CHECK( c.num_steps() == 3u );

  std::stringstream os;
  write_chain( c, os );
  CHECK( os.str() ==
         "4 := AND( 1,2 )\n"
         "5 := OR( -2,3 )\n"
         "6 := XOR( 4,5 )\n" );

  os.str( std::string() );
  write_dot( c, os );
  CHECK( os.str() ==
         "graph{\n"
         "rankdir = BT;\n"
         "x1 [shape=none,label=<x<sub>1</sub>>];\n"
         "x2 [shape=none,label=<x<sub>2</sub>>];\n"
         "x3 [shape=none,label=<x<sub>3</sub>>];\n"
         "x4 [label=<x<sub>4</sub>: AND>];\n"
         "x1 -- x4;\n"
         "x2 -- x4;\n"
         "x5 [label=<x<sub>5</sub>: OR>];\n"
         "x2 -- x5 [style=dashed];\n"
         "x3 -- x5;\n"
         "x6 [label=<x<sub>6</sub>: XOR>];\n"
         "x4 -- x6;\n"
         "x5 -- x6;\n"
         "{rank = same; x1; x2; x3; }\n"
         "edge[style=invisible];\n"
         "x2 -- x3;\n"
         "x1 -- x2;\n"
         "}\n" );
}

namespace copycat::detail
{
  template<>
  std::string label_to_string( kitty::dynamic_truth_table const& label )
  {
    return "0x" + kitty::to_hex( label );
  }
}

TEST_CASE( "Boolean chain with truth table operators", "[chain]" )
{
  chain<kitty::dynamic_truth_table, std::vector<int>> c;
  CHECK( c.num_inputs() == 0u );
  CHECK( c.num_steps() == 0u );

  kitty::dynamic_truth_table x( 3 ), y( 3 ), z( 3 );
  kitty::create_nth_var( x, 0 );
  kitty::create_nth_var( y, 1 );
  kitty::create_nth_var( z, 2 );

  auto const num_inputs = 3u;
  c.set_inputs( num_inputs );
  CHECK( c.num_inputs() == num_inputs );

  auto const s0 = c.add_step( x & y,    {  1, 2 } );
  auto const s1 = c.add_step( (~y) | z, { -2, 3 } );
  auto const s2 = c.add_step( c.label_at( s0 ) ^ c.label_at( s1 ), { s0, s1 } );
  CHECK( s0 == 4 );
  CHECK( s1 == 5 );
  CHECK( s2 == 6 );
  CHECK( c.num_steps() == 3u );

  std::stringstream os;
  write_chain( c, os );
  CHECK( os.str() ==
         "4 := 0x88( 1,2 )\n"
         "5 := 0xf3( -2,3 )\n"
         "6 := 0x7b( 4,5 )\n" );

  os.str( std::string() );
  write_dot( c, os );
  CHECK( os.str() ==
         "graph{\n"
         "rankdir = BT;\n"
         "x1 [shape=none,label=<x<sub>1</sub>>];\n"
         "x2 [shape=none,label=<x<sub>2</sub>>];\n"
         "x3 [shape=none,label=<x<sub>3</sub>>];\n"
         "x4 [label=<x<sub>4</sub>: 0x88>];\n"
         "x1 -- x4;\n"
         "x2 -- x4;\n"
         "x5 [label=<x<sub>5</sub>: 0xf3>];\n"
         "x2 -- x5 [style=dashed];\n"
         "x3 -- x5;\n"
         "x6 [label=<x<sub>6</sub>: 0x7b>];\n"
         "x4 -- x6;\n"
         "x5 -- x6;\n"
         "{rank = same; x1; x2; x3; }\n"
         "edge[style=invisible];\n"
         "x2 -- x3;\n"
         "x1 -- x2;\n"
         "}\n" );
}

TEST_CASE( "Boolean chain with truth table operators and fanin restriction", "[chain]" )
{
  chain<kitty::dynamic_truth_table, std::array<int, 2u>> c;
  CHECK( c.num_inputs() == 0u );
  CHECK( c.num_steps() == 0u );

  kitty::dynamic_truth_table x( 3 ), y( 3 ), z( 3 );
  kitty::create_nth_var( x, 0 );
  kitty::create_nth_var( y, 1 );
  kitty::create_nth_var( z, 2 );

  auto const num_inputs = 3u;
  c.set_inputs( num_inputs );
  CHECK( c.num_inputs() == num_inputs );

  auto const s0 = c.add_step( x & y,    {  1, 2 } );
  auto const s1 = c.add_step( (~y) | z, { -2, 3 } );
  auto const s2 = c.add_step( c.label_at( s0 ) ^ c.label_at( s1 ), { s0, s1 } );
  CHECK( s0 == 4 );
  CHECK( s1 == 5 );
  CHECK( s2 == 6 );
  CHECK( c.num_steps() == 3u );

  std::stringstream os;
  write_chain( c, os );
  CHECK( os.str() ==
         "4 := 0x88( 1,2 )\n"
         "5 := 0xf3( -2,3 )\n"
         "6 := 0x7b( 4,5 )\n" );

  os.str( std::string() );
  write_dot( c, os );
  CHECK( os.str() ==
         "graph{\n"
         "rankdir = BT;\n"
         "x1 [shape=none,label=<x<sub>1</sub>>];\n"
         "x2 [shape=none,label=<x<sub>2</sub>>];\n"
         "x3 [shape=none,label=<x<sub>3</sub>>];\n"
         "x4 [label=<x<sub>4</sub>: 0x88>];\n"
         "x1 -- x4;\n"
         "x2 -- x4;\n"
         "x5 [label=<x<sub>5</sub>: 0xf3>];\n"
         "x2 -- x5 [style=dashed];\n"
         "x3 -- x5;\n"
         "x6 [label=<x<sub>6</sub>: 0x7b>];\n"
         "x4 -- x6;\n"
         "x5 -- x6;\n"
         "{rank = same; x1; x2; x3; }\n"
         "edge[style=invisible];\n"
         "x2 -- x3;\n"
         "x1 -- x2;\n"
         "}\n" );
}

TEST_CASE( "LTL chain", "[chain]" )
{
  chain<std::string, std::vector<int>> c;
  CHECK( c.num_inputs() == 0u );
  CHECK( c.num_steps() == 0u );

  auto const num_inputs = 2u;
  c.set_inputs( num_inputs );
  CHECK( c.num_inputs() == num_inputs );

  auto const s0 = c.add_step( "G",  { 2 } );
  auto const s1 = c.add_step( "U",  { 1, s0 } );
  auto const s2 = c.add_step( "F",  { s0 } );
  c.add_step( "OR",  { s1, s2 } );
  CHECK( c.num_steps() == 4u );

  std::stringstream os;
  write_chain( c, os );
  CHECK( os.str() ==
         "3 := G( 2 )\n"
         "4 := U( 1,3 )\n"
         "5 := F( 3 )\n"
         "6 := OR( 4,5 )\n" );

  os.str( std::string() );
  write_dot( c, os );  
  CHECK( os.str() ==
         "graph{\n"
         "rankdir = BT;\n"
         "x1 [shape=none,label=<x<sub>1</sub>>];\n"
         "x2 [shape=none,label=<x<sub>2</sub>>];\n"
         "x3 [label=<x<sub>3</sub>: G>];\n"
         "x2 -- x3;\n"
         "x4 [label=<x<sub>4</sub>: U>];\n"
         "x1 -- x4;\n"
         "x3 -- x4;\n"
         "x5 [label=<x<sub>5</sub>: F>];\n"
         "x3 -- x5;\n"
         "x6 [label=<x<sub>6</sub>: OR>];\n"
         "x4 -- x6;\n"
         "x5 -- x6;\n"
         "{rank = same; x1; x2; }\n"
         "edge[style=invisible];\n"
         "x1 -- x2;\n"
         "}\n" );
}
