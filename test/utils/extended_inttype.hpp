#include <catch.hpp>
#include <copycat/utils/extended_inttype.hpp>

using namespace copycat;

TEST_CASE( "Pair of two euint32_t", "[extended_inttype]" )
{
  using euint32_pair = extended_inttype_pair<uint32_t>;
  euint32_t impossible = euint32_t::impossible();
  euint32_t infinite = euint32_t::infinite();

  CHECK( euint32_pair( 0, 0 ).swap() == euint32_pair( 0, 0 ) );
  CHECK( euint32_pair( 0, 0 ).increment() == euint32_pair( 1, 1 ) );
  CHECK( euint32_pair( 0, 0 ).minmax( euint32_pair( impossible, 1 ) ) == euint32_pair( 0, 1 ) );
  CHECK( euint32_pair( 0, 0 ).maxmin( euint32_pair( impossible, 1 ) ) == euint32_pair( impossible, 0 ) );

  CHECK( euint32_pair( infinite, 1 ).swap() == euint32_pair( 1, infinite ) );
  CHECK( euint32_pair( infinite, 1 ).increment() == euint32_pair( infinite, 2 ) );
  CHECK( euint32_pair( infinite, 1 ).minmax( euint32_pair( 7, impossible ) ) == euint32_pair( 7, impossible ) );
  CHECK( euint32_pair( infinite, 1 ).maxmin( euint32_pair( 7, impossible ) ) == euint32_pair( infinite, 1 ) );
}
