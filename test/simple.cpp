#include <catch.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/algorithms/simulation.hpp>

using namespace mockturtle;

TEST_CASE( "Dummy test", "[simple]" )
{
  aig_network aig;
  aig.create_ro();
  
  CHECK( true );
}
