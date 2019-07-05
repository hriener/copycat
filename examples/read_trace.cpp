#include <copycat/io/traces.hpp>
#include <copycat/trace.hpp>

struct ltl_synthesis_spec
{
  std::vector<copycat::trace> good_traces;
  std::vector<copycat::trace> bad_traces;
  std::vector<std::string> operators;
  std::vector<std::string> parameters;
  std::vector<std::string> formulas;
}; /* ltl_synthesis_spec */

class ltl_synthesis_spec_reader : public copycat::trace_reader
{
public:
  explicit ltl_synthesis_spec_reader( ltl_synthesis_spec& spec )
    : _spec( spec )
  {
  }

  void on_good_trace( std::vector<std::vector<int>> const& prefix,
                      std::vector<std::vector<int>> const& suffix ) const override
  {
    copycat::trace t;
    for ( const auto& p : prefix )
      t.emplace_prefix( p );
    for ( const auto& s : suffix )
      t.emplace_suffix( s );
    _spec.good_traces.emplace_back( t );
  }

  void on_bad_trace( std::vector<std::vector<int>> const& prefix,
                     std::vector<std::vector<int>> const& suffix ) const override
  {
    copycat::trace t;
    for ( const auto& p : prefix )
      t.emplace_prefix( p );
    for ( const auto& s : suffix )
      t.emplace_suffix( s );
    _spec.bad_traces.emplace_back( t );
  }

  void on_operator( std::string const& op ) const override
  {
    _spec.operators.emplace_back( op );
  }

  void on_parameter( std::string const& parameter ) const override
  {
    _spec.parameters.emplace_back( parameter );
  }
  
  void on_formula( std::string const& formula ) const override
  {
    _spec.formulas.emplace_back( formula );
  }
  
private:
  ltl_synthesis_spec& _spec;
}; /* ltl_synthesis_spec_reader */

bool read_ltl_synthesis_spec( std::istream& is, ltl_synthesis_spec& spec )
{
  return copycat::read_traces( is, ltl_synthesis_spec_reader( spec ) );
}

bool read_ltl_synthesis_spec( std::string const& filename, ltl_synthesis_spec& spec )
{
  return copycat::read_traces( filename, ltl_synthesis_spec_reader( spec ) );
}

int main( int argc, char* argv[] )
{
  assert( argc == 2u );

  std::string const filename{argv[1]};

  ltl_synthesis_spec spec;
  if ( read_ltl_synthesis_spec( filename, spec ) )
  {
    std::cout << filename << " success" << std::endl;
    for ( const auto& t : spec.good_traces )
    {
      std::cout << "[+] "; t.print();
    }
    for ( const auto& t : spec.bad_traces )
    {
      std::cout << "[-] "; t.print();
    }
  }
  else
  {
    std::cout << filename << " failure" << std::endl;
  }

  return 0;
}
