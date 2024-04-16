#include <fstream>
#include <memory>
#include <iostream>
#include <vector>
#include <cmath>

#define TARGET_PATH "ahri.bmp"
#define DUMP_PATH "ahri_blur.bmp"


struct rgb {
    char b = 0;
    char g = 0;
    char r = 0;
};


class Filter {
public:
    using Container = std::vector< float >;
    using Sequence  = std::vector< std::pair< char, char > >;

public:
    Filter() = default;

    Filter( size_t n, Container vals )
    : _n{ n }, _vals{ std::move( vals ) }
    {
        auto seq = gen_seq( _n );

        size_t at = 1;

        for( auto& s : seq ) {
            std::cout << this->operator()( s.first, s.second ) << ' ';

            if( at++ % _n == 0 )
                std::cout << '\n';
        }

        std::cout << '\n';
    }

private:
    Container   _vals   = {};
    size_t      _n      = 0;

public:
    inline float operator () ( size_t y, size_t x ) const {
        return _vals[ ( y + _n / 2 ) * _n + ( x + _n / 2 ) ];
    }

public:
    auto n() const { return _n; }

public:
    /// Expand as itarator tuple, this does for now
    static Sequence gen_seq( size_t n ) { 
        Sequence seq{};

        for( size_t i = 0; i < n; ++i )
            for( size_t j = 0; j < n; ++j )
                seq.emplace_back( i - n / 2, j - n / 2 );

        return seq;
    }

public:
    static Filter gaussian( size_t n, float s ) {
        static constexpr auto PI = 3.14159265359;
        static constexpr auto E  = 2.71828182846;

        Container cnt{};
        cnt.reserve( n * n );

        auto seq = gen_seq( n );

        float m = 1.0 / ( 2.0 * PI * s * s );

        float sum = 0.0;

        for( auto& p : seq )
            sum += cnt.emplace_back(
                m * std::pow( E, -( p.first * p.first + p.second * p.second ) / ( 2 * s * s ) )
            );

        for( auto& v : cnt )
            v /= sum;

        return { n, std::move( cnt ) };
    }

};


auto conv( const char* raw_buf, size_t w, size_t h, size_t sz ) {
    size_t pad = ( 4 - ( ( w * 3 ) % 4 ) ) * ( ( w * 3 ) % 4 != 0 );

    rgb* res{ ( rgb* ) new char[ w * h * 3 + pad * h ] };

    const rgb* buf = reinterpret_cast< const rgb* >( raw_buf );

    auto at = [ & ] ( const rgb* arr, size_t y, size_t x ) -> rgb* {
        void* ptr = const_cast< void* >( reinterpret_cast< const void* >( arr ) );

        ptr += y * ( w * 3 + pad ) + x * 3;

        return reinterpret_cast< rgb* >( ptr );
    };


    auto filter = Filter::gaussian( 7, 2 );


    for( size_t y = 0; y < h; ++y ) {
        for( size_t x = 0; x < w; ++x ) {
            float sum[] = { 0, 0, 0 };

            for( auto s : Filter::gen_seq( filter.n() ) ) {
                size_t ny = y + s.first;
                size_t nx = x + s.second;

                if( ny >= h || nx >= w ) continue;

                auto px = at( buf, ny, nx ); 
                sum[ 0 ] += filter( s.first, s.second ) * px->r;
                sum[ 1 ] += filter( s.first, s.second ) * px->g;
                sum[ 2 ] += filter( s.first, s.second ) * px->b;
                
            }

            auto px = at( res, y, x );
            px->r = ( x == 500 && y == 500 ) ? 255 : sum[ 0 ];
            px->g = ( x == 500 && y == 500 ) ? 255 : sum[ 1 ];
            px->b = ( x == 500 && y == 500 ) ? 255 : sum[ 2 ]; 
        }
    }

    return ( char* ) res;
}



int main() {
    std::ifstream file{ TARGET_PATH, std::ios::binary | std::ios::ate };

    if( !file ) {
        std::cout << "TARGET FAULT";
        return 0;
    }


    size_t sz = file.tellg();
    file.seekg( std::ios::beg );
    

    char* stream = new char[ sz ];
    file.read( stream, sz );

    
    
    size_t width = *reinterpret_cast< int* >( stream + 18 );
    size_t height = *reinterpret_cast< int* >( stream + 22 );

  
    auto res = conv( stream + 54, width, height, sz );


    std::ofstream file_blurred{ DUMP_PATH, std::ios::binary };

    file_blurred.write( stream, 54 );
    file_blurred.write( res, sz - 54 );

    file_blurred.close();


    delete[] res;
    delete[] stream;

    return 0;
}