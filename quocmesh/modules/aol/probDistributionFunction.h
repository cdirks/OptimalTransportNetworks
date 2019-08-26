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

#ifndef __PROBDISTRIBUTIONFUNCTION_H
#define __PROBDISTRIBUTIONFUNCTION_H

#include <vec.h>
#include <smallMat.h>
#include <multiVector.h>
#include <scalarArray.h>


namespace aol {

/** Helper class containing utilities for probability distributions
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class ProbDistFuncHelper {
public:
  typedef aol::Vec2<RealType> Pt2d;

  //! probability that two samples were drawn from the same 1D distribution for a given Kolmogorov-Smirnov distance,
  //! code adapted from the CERN root library TMash::KolmogorovProb(Double_t z)
  //! \attention this formula is valid for large samples only
  static RealType KolmogorovProb ( const RealType z );

  /** Kolmogorov-Smirnov distribution evaluated according to (the m!=n extension of)
   *  Massey: The Distribution of the Maximum Deviation Between two Sample Cumulative Step Functions, The Annals of Mathematical Statistics 22 (1951), http://www.jstor.org/stable/2236712
   *  Exact formula primarily useful for "small" N0, N1
   *  Successfully tested against implementation in R (which cannot be used here due to being GPL)
   *
   *  \author Schwen (MEVIS)
   */
  static RealType KolmogorovProbTwoSmallSamples ( const RealType X, const unsigned int N0, const unsigned int N1 );

  //! probability that two samples were drawn from the same 1D distribution for a given Cramer-von Mises distance,
  //! code adapted from a matlab script by Juan Cardelino (cmtest2.m); values from Anderson, Darling: Asymptotic theory of certain "Goodness of fit" criteria based on stochastic processes, The Annals of Mathematical Statistics 23, 1952.
  static RealType CramerVonMisesProb ( const RealType z, const unsigned int n0, const unsigned int n1 );

  template< typename DataType >
  static void samplesToHisto1D ( const aol::Vector<DataType> &Samples, std::map< RealType, unsigned int > &Histogram ) {
    Histogram.clear();
    for ( int i = 0; i < Samples.size(); ++i ) {
      if ( aol::isFinite ( Samples[i] ) ) {
        ++( Histogram[ Samples[i] ] );
#ifdef VERBOSE
      } else {
        cout << "ignoring value " << Samples[i] << " at " << i << endl;
#endif
      }
    }
  }

  template< typename DataType >
  static void samplesToHisto2D ( const aol::MultiVector< DataType > &Samples, std::map< Pt2d, unsigned int > &Histogram ) {
    if ( Samples.numComponents() != 2 ) {
      throw aol::Exception ( "Illegal MultiVector for samples", __FILE__, __LINE__ );
    }
    Histogram.clear();
    for ( int i = 0; i < Samples.getEqualComponentSize(); ++i ) {
      if ( aol::isFinite ( Samples[0][i] ) && aol::isFinite ( Samples[1][i] ) ) {
        const Pt2d posn ( Samples[0][i], Samples[1][i] );
        ++( Histogram[ posn ] );
      }
    }
  }

  template< typename DataType >
  static void discreteHistoToHisto ( const aol::Vector<DataType> &DiscreteHisto, std::map< RealType, unsigned int > &Histogram ) {
    Histogram.clear();
    for ( int i = 0; i < DiscreteHisto.size(); ++i ) {
      Histogram[ static_cast<RealType> ( i ) ] = DiscreteHisto[i];
    }
  }

  template< typename DataType >
  static void vecHistoToHisto ( const aol::Vector<DataType> &Xvalues, const aol::Vector<unsigned int> &Hvalues, std::map< RealType, unsigned int > &Histogram ) {
    if ( Xvalues.size() != Hvalues.size() ) {
      throw ( aol::Exception ( "Vector size mismatch.", __FILE__, __LINE__ ) );
    }

    // this way, we sort for x values and keep track of corresponding h values
    for ( int i = 0; i < Xvalues.size(); ++i ) {
      Histogram[ Xvalues[i] ] += Hvalues[i];
    }
  }

  static void doCompute2DPDFdistTo ( const aol::MultiVector< RealType > &thisXYCo, const aol::MultiVector< RealType > &otherXYCo,
                                     const qc::ScalarArray< RealType, qc::QC_2D > &thisDPdf, const qc::ScalarArray< RealType, qc::QC_2D > &otherDPdf,
                                     const unsigned int thisNSamples, const unsigned int otherNSamples,
                                     RealType &L2Dist, RealType& LInfDist, RealType &CvMDist );
};


/** Base class for probability distributions in 1D or 2D.
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class ProbDistributionFunctionAnyD {
protected:
  unsigned int _nSamples;
  RealType _L2Dist, _LInfDist, _CvMDist;

protected:
  ProbDistributionFunctionAnyD ( );

  RealType getScaledKSDistanceTo ( const ProbDistributionFunctionAnyD<RealType> &other ) const;

public:
  unsigned int numSamples ( ) const { return ( _nSamples ); }

  //! unscaled L2 distance to another PDF (if it has been computed on a derived class)
  RealType getL2Dist ( ) const { return ( _L2Dist ); };

  //! unscaled L infinity (Kolmogorov-Smirnov) distance to another PDF (if it has been computed on a derived class)
  RealType getLInfDist ( ) const { return ( _LInfDist ); };

  //! unscaled L infinity (Kolmogorov-Smirnov) distance to another PDF (if it has been computed on a derived class)
  RealType getKSDist ( ) const { return ( _LInfDist ); };

  //! unscaled Cramer-von Mises (L2-type) distance to another PDF (if it has been computed on a derived class)
  RealType getCvMDist ( ) const { return ( _CvMDist ); };
};


/** Probability distributions in 1D.
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class ProbDistributionFunction1D : public ProbDistributionFunctionAnyD<RealType> {
protected:
  struct PDFDiffStep1D {
    aol::Vec2<RealType> _val;
    aol::Vec2<RealType> _step;
  };

  std::map < RealType, RealType > _pdf;

public:
  void dump ( ostream& out = cout ) const;

  //! this method must be called prior to getScaled*DistanceTo
  void computePDFdistTo ( const ProbDistributionFunction1D<RealType> &other );

  //! measure used in Kolmogorov-Smirnov test: L-infinity distance of distribution functions scaled by factor depending on sample-size
  RealType getScaledKSDistanceTo ( const ProbDistributionFunction1D<RealType> &other ) const {
    return ( ProbDistributionFunctionAnyD<RealType>::getScaledKSDistanceTo ( other ) );
  }

  //! measure used in Cramer-von-Mises test: square of difference times d cumulative density
  RealType getScaledCvMDistanceTo ( const ProbDistributionFunction1D<RealType> &other ) const;

  //! L2 distance for domain interpreted as [0,1]
  RealType getDomainScaledL2DistanceTo ( const  ProbDistributionFunction1D<RealType> &other ) const;

  //! L2 distance for domain interpreted as [0,1], scaled by factor depending on sample sizes
  RealType getScaledL2DistanceTo ( const  ProbDistributionFunction1D<RealType> &other ) const;

  //! return const reference to the probability density function data
  const std::map< RealType, RealType >& getPdf ( ) const { return ( _pdf ); }

  //! print the probability distribution for gnuplot plotting
  void printPDFforGnuplot ( ostream & gpout ) const;

protected:
  ProbDistributionFunction1D ( );

  void initialize ( const std::map< RealType, unsigned int > &Histogram );
};


/** Probability distributions in 1D computed from a sample.
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class ProbDistributionFunction1DForSample : public ProbDistributionFunction1D<RealType> {
public:
  template< typename DataType >
  explicit ProbDistributionFunction1DForSample ( const aol::Vector<DataType> &Samples ) {
    std::map < RealType, unsigned int > histogram;
    ProbDistFuncHelper<RealType>::samplesToHisto1D ( Samples, histogram );
    this->initialize ( histogram );
  }
};


/** Probability distributions in 1D computed from a discrete histogram for integer events.
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class ProbDistributionFunction1DForDiscreteHisto : public ProbDistributionFunction1D<RealType> {
public:
  explicit ProbDistributionFunction1DForDiscreteHisto ( const aol::Vector<unsigned int> &DiscreteHisto ) {
    std::map < RealType, unsigned int > histogram;
    ProbDistFuncHelper<RealType>::discreteHistoToHisto ( DiscreteHisto, histogram );
    this->initialize ( histogram );
  }
};

/** Probability distributions in 1D computed from a discrete histogram.
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class ProbDistributionFunction1DForVecHisto : public ProbDistributionFunction1D<RealType> {
public:
  ProbDistributionFunction1DForVecHisto ( const aol::Vector<RealType> &Xvalues, const aol::Vector<unsigned int> &Hvalues ) {
    std::map < RealType, unsigned int > histogram;
    ProbDistFuncHelper<RealType>::vecHistoToHisto ( Xvalues, Hvalues, histogram );
    this->initialize ( histogram );
  }
};


/** Probability distributions in 2D.
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class ProbDistributionFunction2D : public ProbDistributionFunctionAnyD<RealType> {
protected:
  typedef aol::Vec2<RealType> Pt2d;

  aol::Vector< RealType > _xyCo[2];
  qc::ScalarArray< RealType, qc::QC_2D > _dPdf[2][2];

public:
  struct PDFDiffStep2D {
    aol::Vec2<RealType>     _val;  // index corresponds to {this, other}
    aol::Matrix22<RealType> _step; // first index corresponds to {this, other}, second index corresponds to (x,y)
  };

  void dump ( ostream& out = cout ) const;

  //! this method must be called prior to getScaled*DistanceTo
  void computePDFdistTo ( const ProbDistributionFunction2D<RealType> &other );

  //! measure used in Kolmogorov-Smirnov test: L-infinity distance of distribution functions scaled by factor depending on sample-size
  RealType getScaledKSDistanceTo ( const ProbDistributionFunction2D<RealType> &other ) const {
    return ( ProbDistributionFunctionAnyD<RealType>::getScaledKSDistanceTo ( other ) );
  }

  //! measure used in Cramer-von-Mises test: square of difference times d cumulative density
  RealType getScaledCvMDistanceTo ( const ProbDistributionFunction2D<RealType> &/*other*/ ) const;

  //! L2 distance for domain interpreted as [0,1]^2, scaled by factor depending on sample sizes
  RealType getScaledL2DistanceTo ( const  ProbDistributionFunction2D<RealType> &other ) const;

  inline Pt2d getCoord ( const aol::Vec2<short> & Ind ) const {
    return ( Pt2d ( _xyCo[0][ Ind[0] ], _xyCo[1][ Ind[1] ] ) );
  }

protected:
  ProbDistributionFunction2D ( );

  void initialize ( const std::map< Pt2d, unsigned int > &Histogram );
};


/** Probability distributions in 2D computed from a sample.
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class ProbDistributionFunction2DForSample : public ProbDistributionFunction2D<RealType> {
public:
  typedef typename ProbDistributionFunction2D<RealType>::Pt2d Pt2d;

  template< typename DataType >
  explicit ProbDistributionFunction2DForSample ( const aol::MultiVector< DataType > &Samples ) {
    std::map < Pt2d, unsigned int > histogram;
    ProbDistFuncHelper<RealType>::samplesToHisto2D ( Samples, histogram );
    this->initialize ( histogram );
  }
};


/** Pseudo-Random Number Generator that produces random numbers for a given 1D probability distribution.
 *  \author Schwen (MEVIS)
 */
template< typename RealType >
class PRNGForGivenDistr1D {
protected:
  aol::RandomGenerator _prng;
  aol::DiscreteValueInterpolator<RealType,RealType> _dvi;

public:
  PRNGForGivenDistr1D ( const ProbDistributionFunction1D<RealType> &GivenDistr, const unsigned int Seed = 0 ) : _prng ( Seed ) {
    init ( GivenDistr );
  }

  PRNGForGivenDistr1D ( const aol::Vector<RealType> &ModelValues, const unsigned int Seed = 0 ) : _prng ( Seed ) {
    const aol::ProbDistributionFunction1DForSample<RealType> ModelDistr ( ModelValues );
    init ( ModelDistr );
  }

  void randomize ( ) {
    _prng.randomize();
  }

  inline RealType rReal ( ) {
    return ( _dvi.evaluate ( _prng.rReal<RealType> ( ) ) );
  }

protected:
  void init ( const ProbDistributionFunction1D<RealType> &GivenDistr ) {
    std::map<RealType, RealType> pdf ( GivenDistr.getPdf() );
    RealType zval = pdf.begin()->first;
    pdf [ zval - 1.0e-6 ] = 0;
    _dvi.setVals ( pdf );
    _dvi.invertIfMonotonic();
  }

};

}

#endif
