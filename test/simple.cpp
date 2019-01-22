#include <catch.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/algorithms/simulation.hpp>
#include <mockturtle/io/aiger_reader.hpp>
#include <copycat/protocol_extractor.hpp>
#include <lorina/aiger.hpp>
#include <kitty/print.hpp>

using namespace mockturtle;

template<typename TT>
class value_simulator
{
public:
  TT compute_constant( bool value ) const
  {
    TT tt;
    return value ? ~tt : tt;
  }

  TT compute_ci( uint32_t index ) const
  {
    return values.at( index );
  }

  TT compute_not( TT const& value ) const
  {
    return ~value;
  }

public:
  std::vector<TT> values;
};

TEST_CASE( "Dummy test", "[simple]" )
{
  auto const num_vars = 8u; // simulate 2^num_vars innputs in parallel
  auto const num_time_steps = 100u;

  // total number of samples = 2^num_vars * num_time_steps

  copycat::protocol_graph g;
  aig_network aig;
  auto const result = lorina::read_aiger( "", aiger_reader( aig ) );
  CHECK( result == lorina::return_code::success );

  using tt_t = kitty::static_truth_table<num_vars>;
  using simulator_t = value_simulator<tt_t>;
  simulator_t sim;

  /* initialize values with random numbers */
  std::default_random_engine::result_type seed = 0xcafeaffe;

  sim.values.resize( aig.num_cis() );
  aig.foreach_ci( [&]( const auto& node, auto index ){
      tt_t tt;
      kitty::create_random( tt, seed + index );
      sim.values[index] = tt;
    });

  auto prev_node = g.get_root();
  auto curr_node = g.get_root();
  for ( auto k = 0u; k < num_time_steps; ++k )
  {
    ++seed;
    auto const v = simulate_nodes<tt_t, aig_network, simulator_t>( aig, sim );

    auto counter = 0;
    // std::cout << "===--- inputs ---===" << std::endl;
    aig.foreach_pi( [&]( const auto& n, const auto index ){
        if ( 0 )
        {
          std::cout << "pi" << index << "[" << k << "]" << ' ';
          kitty::print_binary( v[ n ] );
          std::cout << std::endl;
        }

        tt_t tt;
        kitty::create_random( tt, seed + counter );
        sim.values[counter++] = tt;
      });

#if 0
    // aig.foreach_ro( [&]( const auto& n, const auto index ){
    //     std::cout << "ro" << index << "[" << k << "]" << ' ';
    //     kitty::print_binary( v[ n ] );
    //     std::cout << std::endl;
    //   });
#endif

    if ( 0 )
    {
      std::cout << "===--- outputs ---===" << std::endl;
      aig.foreach_po( [&]( const auto& f, const auto index ){
          std::cout << "po" << index << "[" << k << "]" << ' ';
          kitty::print_binary( aig.is_complemented( f ) ? ~v[ aig.get_node( f )] : v[ aig.get_node( f )] );
          std::cout << std::endl;
        });
    }

    aig.foreach_ri( [&]( const auto& f, const auto index ){
        auto const& tt = aig.is_complemented( f ) ? ~v[ aig.get_node( f )] : v[ aig.get_node( f )];

#if 0
        // std::cout << "ri" << index << "[" << k << "]" << ' ';
        // kitty::print_binary( tt );
        // std::cout << std::endl;
#endif

        sim.values[counter++] = tt;
      });

    for ( auto i = 0; i < (1 << num_vars); ++i )
    {
      std::string input;
      aig.foreach_ci( [&]( const auto& n ){
          auto const bit = kitty::get_bit( v[ n ], i );
          input += bit ? '1' : '0';
        });

      std::string output;
      aig.foreach_co( [&]( const auto& f ){
          auto const tt = aig.is_complemented( f ) ? ~v[ aig.get_node( f ) ] : v[ aig.get_node( f ) ];
          auto const bit = kitty::get_bit( tt, i );
          output += bit ? '1' : '0';
        });

      // std::cout << input << ' ' << output << std::endl;
      curr_node = g.add_node( input, output );
      g.add_edge( prev_node, curr_node );
      prev_node = curr_node;
    }
  }

  std::cout << "#nodes = " << g.size() << '/' << ( 1 << num_vars ) * num_time_steps << std::endl;
  std::cout << "ratio = " << ( double(g.size()) / ( ( 1 << num_vars ) * num_time_steps ) * 100 ) << std::endl;
}
