/*
 * Copyright (c) 2001-2014 AG Rumpf, INS, Universitaet Bonn                      *
 *                                                                               *
 * The contents of this file are subject to the terms of the Common Development  *
 * and Distribution License Version 1.0 (the "License"); you may not use       *
 * this file except in compliance with the License. You may obtain a copy of     *
 * the License at http://www.opensource.org/licenses/CDDL-1.0                    *
 *                                                                               *
 * Software distributed under the License is distributed on an "AS IS" basis,  *
 * WITHOUT WARRANTY OF ANY KIND, either expressed or implied.                    *
 */

#ifndef __RANDOMGENERATOR_H
#define __RANDOMGENERATOR_H

#include <aol.h>

namespace aol{

/** A class for generating uniformly distributed pseudo-random data
 *  for different data types, using the Mersenne twister.
 *
 *  For the same seed, the same sequence of pseudo-random numbers is
 *  generated, independent of the platform (provided unsigned int is
 *  32 bit). For floating point data types, 32 bits are
 *  pseudo-random. This class is not suitable for cryptographic use.
 *
 *  See M. Matsumoto, T. Nishimura: Mersenne Twister: A
 *  623-Dimensionally Equidistributed Uniform Pseudo-Random Number
 *  Generator; ACM Transactions on Modeling and Computer Simulation;
 *  Vol. 8, No 1, January 1998, pp. 3-30 (doi: 10.1145/272991.272995).
 *
 *  As floating point arithmetics can be troublesome, we explicitly
 *  check for return values being in the specified range.
 *
 *  Note that the code slows down by a factor 2 if compiled with
 *  openmp, but only run with one parallel thread.
 *
 *  \author Schwen
 */
class RandomGenerator {
protected:
  unsigned int _seed, _index;
  std::vector< unsigned int > _y;

public:
  //! Standard constructor
  RandomGenerator ( ) : _seed ( 0 ), _index ( 624 ), _y ( 624 ) {
    initializeY();
  }

  //! Constructor specifying a random seed
  explicit RandomGenerator ( const unsigned int seed ) : _seed ( seed ), _index ( 624 ), _y ( 624 ) {
    initializeY();
  }

  //! Set new seed based on current time, seed changes every millisecond. This will lead to different random numbers in each program run in most cases.
  void randomize ( );

  unsigned int getSeed ( ) const {
    return ( _seed );
  }

  void reSeed ( const unsigned int newSeed ) {
    _seed = newSeed;
    initializeY();
  }

public:
  //! get random boolean
  bool rBool ( ) {
    return ( ( getNextRnd() % 2 ) == 1 );
  }


  //! get random nonnegative unsigned integer
  unsigned int rUnsignedInt ( ) {
    return ( getNextRnd() );
  }

  //! get random int in the range [0, max)
  unsigned int rUnsignedInt ( const unsigned int max ) {
    unsigned int ret = 0;
    do {
      ret = static_cast<unsigned int> ( rReal<double> ( max ) );
    } while ( !rangeCheck ( ret, 0u, max ) );
    return ( ret );
  }

  //! get random int in the range [min, max)
  unsigned int rUnsignedInt ( const unsigned int min, const unsigned int max ) {
    unsigned int ret = 0;
    do {
      ret = static_cast<unsigned int> ( rReal<double> ( min, max ) );
    } while ( !rangeCheck ( ret, min, max ) );
    return ( ret );
  }


  //! get random int in the range [0, max)
  int rInt ( const int max ) {
    int ret = 0;
    do {
      ret = static_cast<int> ( rReal<double> ( max ) );
    } while ( !rangeCheck ( ret, 0, max ) );
    return ( ret );
  }

  //! get random int in the range [min, max)
  int rInt ( const int min, const int max ) {
    int ret = 0;
    do {
      ret = static_cast<int> ( rReal<double> ( min, max ) );
    } while ( !rangeCheck ( ret, min, max ) );
    return ( ret );
  }


  //! get random RealType in the range [0, 1)
  template< typename RealType >
  RealType rReal ( ) {
    RealType ret = aol::NumberTrait<RealType>::zero;
    do {
      const RealType
        denom   = static_cast<RealType>( 0xFFFFFFFF ) + aol::NumberTrait<RealType>::one,
        moresig = static_cast<RealType> ( getNextRnd() ),
        lesssig = static_cast<RealType> ( getNextRnd() ) / denom;
      ret = ( moresig + lesssig ) / denom;
    } while ( !rangeCheck ( ret, aol::NumberTrait<RealType>::zero, aol::NumberTrait<RealType>::one ) );
    return ( ret );
  }

  //! get random RealType in the range [0, max)
  template< typename RealType >
  RealType rReal ( const RealType max ) {
    RealType ret = aol::NumberTrait<RealType>::zero;
    do {
      ret = max * rReal<RealType>();
    } while ( !rangeCheck ( ret, aol::NumberTrait<RealType>::zero, max ) );
    return ( ret );
  }

  //! get random RealType in the range [min, max)
  template< typename RealType >
  RealType rReal ( const RealType min, const RealType max ) {
    RealType ret = aol::NumberTrait<RealType>::zero;
    do {
      ret = min + rReal<RealType>() * ( max - min );
    } while ( !rangeCheck ( ret, min, max ) );
    return ( ret );
  }


  //! get normally distributed RealType with specified mean and standard deviation
  template< typename RealType >
  RealType normalrReal ( const RealType mean = 0, const RealType stddev = 1 ) {
    aol::AssertAtCompileTime < !(std::numeric_limits<RealType>::is_integer) >();
    // From http://www.agner.org/random/ ??????
    RealType x1, x2, w;
    do {
      x1 = rReal<RealType> ( -1, 1 );
      x2 = rReal<RealType> ( -1, 1 );
      w = x1 * x1 + x2 * x2;
    } while ( w >= 1 || w < 1E-30 );
    w = sqrt ( ( -2 * log ( w ) ) / w );
    x1 *= w;
    return ( x1 * stddev + mean );
  }

  //! get Poisson distributed int with specified mean
  //! \author Mevenkamp
  template< typename RealType >
  int poissonrInt ( const RealType Lambda ) {
    // From http://www.agner.org/random/
    const double SHAT1 = 2.943035529371538573;    // 8/e
    const double SHAT2 = 0.8989161620588987408;   // 3-sqrt(12/e)
    if ( Lambda < 17) {
      if ( Lambda < 1.E-6) {
        if ( Lambda < 0 ) throw aol::Exception ( "Mean value must be greater than or equal to zero!", __FILE__, __LINE__ );
        else if ( Lambda == 0) return 0;
        else {
          const RealType d = sqrt( Lambda );
          if ( this->rReal<RealType> ( ) >= d ) return 0;
          const RealType r = this->rReal<RealType> ( ) * d;
          if ( r > Lambda * (1.-Lambda) ) return 0;
          if ( r > 0.5 * aol::Sqr<RealType> ( Lambda ) * (1.-Lambda) ) return 1;
          return 2;
        }
      } else {
        const int bound = 130;
        double r, f;
        int x;
        const RealType pois_f0 = exp ( -Lambda );
        while ( true ) {
          r = this->rReal<RealType> ( ); x = 0; f = pois_f0;
          do {
            r -= f;
            if ( r <= 0 ) return x;
            x++;
            f *= Lambda;
            r *= x;
          }
          while ( x <= bound );
        }
      }
    } else {
      if ( Lambda > 2.E9 )
        throw aol::Exception ( "Mean value is too large to generate random samples from it!", __FILE__, __LINE__ );
      double u, lf, x;
      int k;
      const RealType pois_a = Lambda + 0.5;
      const int mode = Lambda;
      const RealType pois_g = log ( Lambda );
      const RealType pois_f0 = mode * pois_g - LnFac ( mode );
      const RealType pois_h = sqrt ( SHAT1 * ( pois_a ) ) + SHAT2;
      const int pois_bound = pois_a + 6.0 * pois_h;
      while ( true ) {
        u = this->rReal<RealType> ( );
        if ( u == 0 ) continue;
        x = pois_a + pois_h * ( this->rReal<RealType> ( ) - 0.5 ) / u;
        if ( x < 0 || x >= pois_bound ) continue;
        k = x;
        lf = k * pois_g - LnFac ( k ) - pois_f0;
        if ( lf >= u * (4.0 - u) - 3.0 ) break;
        if ( u * ( u - lf ) > 1.0 ) continue;
        if ( 2.0 * log ( u ) <= lf ) break;
      }
      return k;
    }
  }
  
  double LnFac ( const int n ) {
    const int FAK_LEN = 100;
    static const double C0 =  0.918938533204672722;
    static const double C1 =  1./12.;
    static const double C3 = -1./360.;
    static double fac_table[FAK_LEN];
    static int initialized = 0;
    
    if ( n < FAK_LEN ) {
      if ( n <= 1 ) {
        if ( n < 0 ) throw aol::Exception ( "Negative parameter in LnFac function!", __FILE__, __LINE__ );
        return 0;
      }
      if ( !initialized ) {
        double sum = fac_table[0] = 0.;
        for ( int i=1; i<FAK_LEN ; ++i ) {
          sum += log ( static_cast<double> ( i ) );
          fac_table[i] = sum;
        }
        initialized = true;
      }
      return fac_table[n];
    }
    const double r  = 1. / n;
    return ( n + 0.5 ) * log ( static_cast<double> ( n ) ) - n + C0 + r * ( C1 + r*r*C3 ); // not found in table. use Stirling approximation
  }


protected:
  //! initialize by some simple random numbers
  void initializeY();

  //! Mersenne twister
  unsigned int getNextRnd ( );


private:
  RandomGenerator ( const RandomGenerator& ) {
    throw aol::UnimplementedCodeException ( "aol::RandomGenerator copy constructor not implemented yet", __FILE__, __LINE__ );
  }

  RandomGenerator& operator= ( const RandomGenerator& ) {
    throw aol::UnimplementedCodeException ( "aol::RandomGenerator::operator= not implemented yet", __FILE__, __LINE__ );
  }

  //! check whether Value lies in [Min, Max)
  template< typename DataType >
  inline bool rangeCheck ( const DataType Value, const DataType Min, const DataType Max ) {
    return ( ( Value < Max ) && ( Value >= Min ) );
  }

}; // end of class RandomGenerator


}

#endif
