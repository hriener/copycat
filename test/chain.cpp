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
  c.set_inputs( num_inputs ); // 0, 1, 2
  CHECK( c.num_inputs() == num_inputs );

  auto const s0 = c.add_step( "AND", {  0, 1 } );
  auto const s1 = c.add_step( "OR",  { -1, 2 } );
  c.add_step( "XOR", { s0, s1 } );
  CHECK( c.num_steps() == 3u );

  // write_chain( c );
  // write_dot( c, "chain0.dot" );
}

namespace copycat::detail
{
  template<>
  std::string label_to_string( kitty::dynamic_truth_table const& label )
  {
    return kitty::to_binary( label );
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

  auto const s0 = c.add_step( x & y,    {  0, 1 } );
  auto const s1 = c.add_step( (~y) | z, { -1, 2 } );
  c.add_step( c.label_at( s0 ) ^ c.label_at( s1 ), { s0, s1 } );
  CHECK( c.num_steps() == 3u );

  // write_chain( c );
  // write_dot( c, "chain1.dot" );
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

  auto const s0 = c.add_step( x & y,    {  0, 1 } );
  auto const s1 = c.add_step( (~y) | z, { -1, 2 } );
  c.add_step( c.label_at( s0 - num_inputs ) ^ c.label_at( s1 - num_inputs ), { s0, s1 } );
  CHECK( c.num_steps() == 3u );

  // write_chain( c );
  // write_dot( c, "chain3.dot" );
}

TEST_CASE( "LTL chain", "[chain]" )
{
  chain<std::string, std::vector<int>> c;
  CHECK( c.num_inputs() == 0u );
  CHECK( c.num_steps() == 0u );

  auto const num_inputs = 2u;
  c.set_inputs( num_inputs );
  CHECK( c.num_inputs() == num_inputs );

  auto const s0 = c.add_step( "G",  { 1 } );
  auto const s1 = c.add_step( "U",  { 0, s0 } );
  auto const s2 = c.add_step( "F",  { s0 } );
  c.add_step( "or",  { s1, s2 } );
  CHECK( c.num_steps() == 4u );

  // write_chain( c );
  // write_dot( c, "chain4.dot" );
}
