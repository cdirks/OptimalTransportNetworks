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

#include <convolution.h>

namespace qc {

#ifdef USE_LIB_FFTW

//! 2D Fourier transform (complex-to-complex)
//! Unnecessarily complicated because ScalarArray<complex> is not possible
//! (otherwise copying would not be necessary)
template <typename RealType>
void FourierTransform ( const qc::MultiArray<RealType, 2, 2>& function, qc::MultiArray<RealType, 2, 2>& transform, enum FourierTransformDirection direction ) {
  typedef ConvolutionTrait<qc::QC_2D,RealType> ConvType;
  typedef typename ConvType::FFTWComplex FFTWComplex;


  // Prepare transformation
  int numX = function [0].getNumX (), numY = function [0].getNumY ();
  if ( function [1].getNumX () != numX || transform [0].getNumX () != numX ||  transform [1].getNumX () != numX ||
       function [1].getNumY () != numY || transform [0].getNumY () != numY ||  transform [1].getNumY () != numY )
    throw aol::Exception ( "Array sizes not equal in FourierTransform", __FILE__, __LINE__ );
  FFTWComplex* f = static_cast<FFTWComplex*> ( ConvType::fftwMalloc ( sizeof ( FFTWComplex ) * numX * numY ) );
  FFTWComplex* t = static_cast<FFTWComplex*> ( ConvType::fftwMalloc ( sizeof ( FFTWComplex ) * numX * numY ) );
  typename ConvType::FFTWPlan plan = ConvType::fftwPlan_dft ( aol::Vec2<int> ( numX, numY ), f, t, direction, FFTW_ESTIMATE );

  // Copy data, transform and copy back
  for ( int j = 0; j < numY; ++j ) {
    for ( int i = 0; i < numX; ++i ) {
      const int ind = qc::ILexCombine2 ( i, j, numX );
      const aol::Vec2<short int> pos ( i, j );
      aol::Vec2<RealType> val = function.get ( pos );
      for ( int k = 0; k < 2; ++k ) f [ind] [k] = val [k];
    }
  }
  ConvType::fftwExecute ( plan );
  for ( int j = 0; j < numY; ++j ) {
    for ( int i = 0; i < numX; ++i ) {
      const int ind = qc::ILexCombine2 ( i, j, numX );
      const aol::Vec2<short int> pos ( i, j );
      aol::Vec2<RealType> val ( t [ind] [0], t [ind] [1] );
      transform.set ( pos, val );
    }
  }

  // Cleanup
  ConvType::fftwDestroy_plan ( plan );
  ConvType::fftwFree ( f );
  ConvType::fftwFree ( t );
}

#else

//! 2D Fourier transform (complex-to-complex)
template <typename RealType>
void FourierTransform ( const qc::MultiArray<RealType, 2, 2>& /*function*/, qc::MultiArray<RealType, 2, 2>& /*transform*/, enum FourierTransformDirection /*direction*/ )
// this (unnamed parameter but default value) seems to work with gcc-4.2.0 and VC++ 2005. if it does not work with other compilers, write separate function with two parameters.
{
  throw aol::Exception ( "FourierTransform needs libfftw! Compile with -DUSE_LIB_FFTW", __FILE__, __LINE__ );
}

#endif

template void FourierTransform<float> ( const qc::MultiArray<float, 2, 2>&, qc::MultiArray<float, 2, 2>&, enum FourierTransformDirection );
template void FourierTransform<double> ( const qc::MultiArray<double, 2, 2>&, qc::MultiArray<double, 2, 2>&, enum FourierTransformDirection );
// Since fftw3l seems to be missing on our Linux machines, we can't use the long double version.
//template void FourierTransform<long double> ( const qc::MultiArray<long double, 2, 2>&, qc::MultiArray<long double, 2, 2>&, enum FourierTransformDirection );

void addMotionBlurToArray ( const aol::Vec2<double> &Velocity, const qc::ScalarArray<double, qc::QC_2D> &Arg, qc::ScalarArray<double, qc::QC_2D> &Dest ) {
  qc::Convolution<qc::QC_2D> conv ( aol::Vec2<int>( Arg.getNumX(), Arg.getNumY() ) );
  qc::ScalarArray<double, qc::QC_2D> kernel ( Arg, aol::STRUCT_COPY );
  qc::generateMotionBlurKernel<double> ( Velocity, kernel );
  conv.convolve ( Arg, kernel, Dest );
}


} // end namespace
