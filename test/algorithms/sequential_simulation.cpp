#include <catch.hpp>
#include <copycat/algorithms/sequential_simulation.hpp>
#include <copycat/generators/waveform_generator.hpp>
#include <copycat/waveform.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/io/write_verilog.hpp>
#include <random>

TEST_CASE( "AIG random simulation", "[sequential_simulator]" )
{
  using namespace mockturtle;
  aig_network aig;

  /* create a 2-bit counter */
  auto l0_out = aig.create_ro();
  auto l1_out = aig.create_ro();
  auto s0 = !l0_out;
  auto s1 = aig.create_xor( l0_out, l1_out );
  aig.create_ri( s0 );
  aig.create_ri( s1 );

  using namespace copycat;

  /* initial parameters */
  uint32_t const time_steps = 100;
  std::default_random_engine::result_type seed( 0 );

  /* setup random number generator */
  auto gen = std::bind( std::uniform_int_distribution<>(0,1), std::default_random_engine( seed ) );

  /* simulation */
  random_simulator sim( aig, gen );
  waveform wf( aig.num_cis() + aig.num_pos(), time_steps );
  waveform_generator waveform_gen( aig, wf );
  simulate( aig, sim, time_steps, waveform_gen );
  // wf.print();

  /* check property on the waveform */
  std::vector<bool> prev, curr;
  wf.foreach_value( [&]( const auto& value, uint32_t const time_step ){
      /* update curr and prev */
      if ( time_step == 0 )
        curr = prev = value;
      else
      {
        prev = curr;
        curr = value;
      }

      /* check the values */
      if ( time_step > 2 )
      {
        CHECK( curr[0u] == !prev[0u] );
        CHECK( curr[1u] == (prev[0u] ^ prev[1u]) );
      }
    } );
}
