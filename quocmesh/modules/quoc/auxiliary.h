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

#ifndef __AUXILIARY_H
#define __AUXILIARY_H

#include <scalarArray.h>
#include <gridBase.h>
#include <FEOpInterface.h>

namespace qc {

inline void ReadArrayHeader ( istream &in, ArrayHeader &header ) {
  in.getline ( header.magic, 7 ); aol::READ_COMMENTS ( in );
  if ( !in )
    throw aol::Exception ( "ReadArrayHeader: Cannot read array header", __FILE__, __LINE__ );
  if ( ( header.magic[0] != 'O' ) && ( header.magic[0] != 'P' ) && ( header.magic[0] != 'Q' ) )
    throw aol::Exception ( "ReadArrayHeader: Invalid array header, doesn't start with 'O', 'P' or 'Q'", __FILE__, __LINE__ );
  in >> header.numX; aol::READ_COMMENTS ( in );
  if ( header.magic[0] == 'P' || header.magic[0] == 'Q' ) {
    in >> header.numY; aol::READ_COMMENTS ( in );
  } else if ( header.magic[0] == 'O' ) {
    header.numY = 1;
  }
  // A ScalarArray<QC_2D> header doesn't contain numZ.
  if ( header.magic[0] == 'P' || header.magic[0] == 'O' )
    header.numZ = 1;
  else
    in >> header.numZ; aol::READ_COMMENTS ( in );
  in >> header.max; aol::READ_COMMENTS ( in );
}


template <typename RealType>
void WriteVectorField ( const char *FileName,
                        Array<RealType> &D1,
                        Array<RealType> &D2,
                        bool Rescale = true ) {

  ofstream out ( FileName );
  if ( !out ) {
    throw aol::FileException ( "cannot open file for output.", __FILE__, __LINE__ );
  }
  int X, Y;
  const int numX = D1.getNumX(), numY = D1.getNumY();
  const int step = aol::Max ( 1, ( numX - 1 ) / 64 );

  RealType h_x = 1.0 / static_cast<RealType> ( numX - 1 );
  RealType h_y = 1.0 / static_cast<RealType> ( numY - 1 );

  RealType max = 0.;
  if ( Rescale ) {
    for ( X = 0; X < numX; X += step ) {
      for ( Y = 0; Y < numY; Y += step ) {
        RealType v = aol::Sqr ( D1.get ( X, Y ) ) + aol::Sqr ( D2.get ( X, Y ) );
        max = aol::Max ( max, v );
      }
    }
    max = 16.0 * h_x / sqrt ( max );
  } else {
    max = 1.0;
  }

  max *= 0.4;
  for ( X = 0; X < numX; X += step ) {
    for ( Y = 0; Y < numY; Y += step ) {
      out << X - 0.5 * max * D1.get ( X, Y ) / h_x << " " << numY - 1. - Y + 0.5 * max * D2.get ( X, Y ) / h_y << " ";
      out << static_cast<RealType> ( X ) + 0.5 * max * D1.get ( X, Y ) / h_x  << " "
      << numY - 1. - ( static_cast<RealType> ( Y ) + 0.5 * max * D2.get ( X, Y ) / h_y ) << endl;
    }
  }
}

/**
 * Given x-component D1 and y-component D2 of a vector field,
 * writes a gnuplot compatible dat file of the vector field
 * Format per line: x, 1-y, D1(x,y), -1.*D2(x,y)
 * This kind of mirroring has to be done, because image viewers
 * and the qouc meshes interpret pgms differently
 *
 * If a Mask pointer is supplied, only Vectors at position where
 * the corresponding mask value is true are written.
 *
 * \author Berkels
 */
template <typename RealType>
void WriteVectorFieldAsGnuplotFile ( ofstream &Outfile,
                                     const Array<RealType> &D1,
                                     const Array<RealType> &D2,
                                     const RealType Spacing,
                                     const qc::BitArray<qc::QC_2D> *Mask = NULL ) {

  int X, Y;
  const int numX = D1.getNumX(), numY = D1.getNumY();
  const int step = aol::Max ( 1, static_cast<int>(( numX - 1 ) * Spacing) );

  RealType h_x = 1.0 / static_cast<RealType> ( numX - 1 );
  RealType h_y = 1.0 / static_cast<RealType> ( numY - 1 );

  for ( X = 0; X < numX; X += step ) {
    for ( Y = 0; Y < numY; Y += step ) {
      if ( !Mask || Mask->get( X, Y ) )
        Outfile << X*h_x << " " << 1.-Y*h_y << " " << D1.get ( X, Y ) << " " << -1.*D2.get ( X, Y ) << endl;
    }
  }
}

template <typename RealType>
void WriteVectorFieldAsGnuplotFile ( ofstream &Outfile,
                                     const qc::GridDefinition &Grid,
                                     const aol::Vector<RealType> &D1,
                                     const aol::Vector<RealType> &D2,
                                     const RealType Spacing ) {
  qc::Array<RealType> d1Array( D1, Grid );
  qc::Array<RealType> d2Array( D2, Grid );
  qc::WriteVectorFieldAsGnuplotFile<RealType> ( Outfile, d1Array, d2Array, Spacing );
}

/**
 * Given x-component D1 and y-component D2 of a deformation (stored as displacement),
 * writes a gnuplot compatible dat file (gnuplot command 'plot "filename" w l') of
 * a grid deformed by the specified deformation.
 *
 * \author Droske, Berkels
 */
template <typename RealType>
void WriteDeformedGrid ( const char *fileName,
                         const Array<RealType> &D1,
                         const Array<RealType> &D2,
                         const int LineDensity = 64 ) {
  const int numX = D1.getNumX(), numY = D1.getNumY();
  const int step = aol::Max ( 1, ( numX - 1 ) / LineDensity );

  const RealType h_x = 1.0 / static_cast<RealType> ( numX - 1 );
  const RealType h_y = 1.0 / static_cast<RealType> ( numY - 1 );

  ofstream out ( fileName );
  if ( !out ) {
    throw aol::FileException ( "cannot open file for output.", __FILE__, __LINE__ );
  }

  // Write vertical grid lines.
  for ( int X = 0; X < numX; X += step ) {
    for ( int Y = 0; Y < numY; ++Y ) {
      const RealType dxs = X + D1.get ( X, Y ) / h_x;
      const RealType dys = Y + D2.get ( X, Y ) / h_y;

      out << dxs << " " << numY - 1 - dys << endl;
    }
    out << endl;
  }

  // Write horizontal grid lines.
  for ( int Y = 0; Y < numY; Y += step ) {
    for ( int X = 0; X < numX; ++X ) {
      const RealType dxs = X + D1.get ( X, Y ) / h_x;
      const RealType dys = Y + D2.get ( X, Y ) / h_y;

      out << dxs << " " << numY - 1 - dys << endl;
    }
    out << endl;
  }
}


template <typename ConfType>
class InterpolateFunctionOp :
      public aol::FENonlinOpInterface<ConfType, InterpolateFunctionOp<ConfType> > {
public:
  typedef typename ConfType::RealType RealType;
  const aol::DiscreteVectorFunctionDefault<ConfType, 2> _def;
  const qc::GridDefinition &_grid;
protected:
public:
  InterpolateFunctionOp ( const qc::GridDefinition &Grid,
                          const aol::MultiVector<RealType> &Def  )
      : aol::FENonlinOpInterface<ConfType, InterpolateFunctionOp<ConfType> > ( Grid ), _def ( Grid, Def ), _grid ( Grid ) {}

  void getNonlinearity ( const aol::DiscreteFunctionDefault<ConfType> &DiscFunc,
                         const typename ConfType::ElementType &El,
                         int QuadPoint, const typename ConfType::VecType &RefCoord,
                         typename ConfType::RealType &NL ) const {
    typename ConfType::VecType offset, coord, transformed_coord, transformed_local_coord;
    qc::Element transformed_el;

    _def.evaluateAtQuadPoint ( El, QuadPoint, offset );

    for ( int i = 0; i < ConfType::Dim; i++ ) {
      coord [i] = El[i] + RefCoord[i];
      transformed_coord[i] = coord[i] + offset[i] / _grid.H();
      transformed_el[i] = static_cast<short> ( transformed_coord[i] );
      transformed_local_coord[i] = transformed_coord[i] - transformed_el[i];
    }

    if ( transformed_el[0] < 0 || ( transformed_el[0] >= ( _grid.getWidth() - 1 ) ) ||
         transformed_el[1] < 0 || ( transformed_el[1] >= ( _grid.getWidth() - 1 ) ) ) {
      NL = 0;
    } else {
      NL = DiscFunc.evaluate ( transformed_el, transformed_local_coord );
    }
  }
};

template <typename _RealType, qc::Dimension Dim = QC_2D>
class TransformFunction {
};

template <typename RealType>
class TransformFunction<RealType, qc::QC_2D> : public aol::BiOp<aol::MultiVector<RealType> > {
protected:
  const qc::RectangularGrid<qc::QC_2D> _grid;
  const aol::MultiVector<RealType> *_def;
  const qc::BitArray<qc::QC_2D> *_mask;
public:
  TransformFunction ( const qc::GridStructure &Grid, const qc::BitArray<qc::QC_2D> *Mask = NULL ) : _grid ( Grid.getSize() ), _def ( NULL ), _mask( Mask ) {}

  void setDeformation ( const aol::MultiVector<RealType> &Def ) {
    _def = &Def;
  }

  void transform ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest, qc::BitArray<qc::QC_2D> &ValuesSet ) const;

  void transform ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest, qc::BitArray<qc::QC_2D> &ValuesSet, const aol::MultiVector<RealType> &ExtendImage ) const;

  void transform ( const aol::Vector<RealType> &Arg, aol::Vector<RealType> &Dest, qc::BitArray<qc::QC_2D> &ValuesSet ) const;

  void transform ( const aol::Vector<RealType> &Arg, aol::Vector<RealType> &Dest, qc::BitArray<qc::QC_2D> &ValuesSet, const aol::Vector<RealType> &ExtendImage ) const;

  void transformMultiLin ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest, qc::BitArray<qc::QC_2D> &ValuesSet, const aol::MultiVector<RealType> &ExtendImage ) const;

  void apply ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest ) const {
    BitArray<QC_2D> valuesSet( GridSize<QC_2D>::createFrom ( _grid ) );
    transform ( Arg, Dest, valuesSet );
  }

  void applyAdd ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest ) const {
    aol::MultiVector<RealType> tmp ( Dest.numComponents(), Dest[0].size() );
    apply ( Arg, tmp );
    Dest += tmp;
  }

  using aol::BiOp<aol::MultiVector<RealType> >::apply;
protected:
  void interpolateTriangle ( const aol::Vec2<RealType> Coords[3], const RealType *Values, std::vector<qc::Array<RealType>* > &Array, const int NumComponents, qc::BitArray<qc::QC_2D> &ValuesSet ) const;

  void interpolateQuad ( const aol::MultiVector<RealType> &PreImages, const aol::MultiVector<RealType> &Images,
      const std::vector< const qc::ScalarArray<RealType, qc::QC_2D> * > &Arg,
      const std::vector< qc::ScalarArray<RealType, qc::QC_2D> * > &Dest, qc::BitArray<qc::QC_2D> &ValuesSet ) const;

  inline void getLocalPreImage ( const aol::Vec2<RealType> &Image, const aol::MultiVector<RealType> &NodalPreImages, const aol::MultiVector<RealType> &NodalImages, aol::Vec2<RealType> &PreImage ) const;
};

template <typename RealType>
class TransformFunction<RealType, qc::QC_3D> : public aol::BiOp<aol::MultiVector<RealType> > {
protected:
  const qc::RectangularGrid<qc::QC_3D> _grid;
  const aol::MultiVector<RealType> *_def;
  const qc::BitArray<qc::QC_3D> *_mask;
public:
  TransformFunction ( const qc::GridStructure &Grid, const qc::BitArray<qc::QC_3D> *Mask = NULL ) : _grid ( Grid.getSize() ), _def ( NULL ), _mask( Mask ) {}

  void setDeformation ( const aol::MultiVector<RealType> &Def ) {
    _def = &Def;
  }

  void transform ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest, qc::BitArray<qc::QC_3D> &ValuesSet ) const;

  void transform ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest, qc::BitArray<qc::QC_3D> &ValuesSet, const aol::MultiVector<RealType> &ExtendImage ) const;

  void transform ( const aol::Vector<RealType> &Arg, aol::Vector<RealType> &Dest, qc::BitArray<qc::QC_3D> &ValuesSet ) const;

  void transform ( const aol::Vector<RealType> &Arg, aol::Vector<RealType> &Dest, qc::BitArray<qc::QC_3D> &ValuesSet, const aol::Vector<RealType> &ExtendImage ) const;

  void apply ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest ) const {
    qc::BitArray<qc::QC_3D> valuesSet ( GridSize<QC_3D>::createFrom ( _grid ) );
    transform ( Arg, Dest, valuesSet );
  }

  void applyAdd ( const aol::MultiVector<RealType> &Arg, aol::MultiVector<RealType> &Dest ) const {
    aol::MultiVector<RealType> tmp ( Dest.numComponents(), Dest[0].size() );
    apply ( Arg, tmp );
    Dest += tmp;
  }

  using aol::BiOp<aol::MultiVector<RealType> >::apply;
protected:
  void interpolateTetrahedron ( const aol::Vec3<RealType> Coords[4], const RealType *Values, std::vector<qc::Array<RealType>* > &Array, const int NumComponents, qc::BitArray<qc::QC_3D> &ValuesSet ) const;
};

int getGridLevelFromArrayFile ( const string &ArrayFileName );

aol::Vec3<int> getSizeFromArrayFile ( const string &ArrayFileName );
Dimension getDimensionFromArrayFile ( const string &ArrayFileName );

//! Checks the filename suffix of a 2D image and saves it accordingly. PNGs and PGMs are saved
//! as such, everything else is saved as binary quoc array with a precision corresponding to the template RealType.
template<typename RealType>
void recognizeEndingAndSave2d ( const qc::ScalarArray<RealType, qc::QC_2D> &Img, const char *FileName, const bool PGMPNGClipThenScale01 = false ) {
  if ( ( aol::fileNameEndsWith ( FileName, ".pgm" ) == false ) && ( aol::fileNameEndsWith ( FileName, ".png" ) == false ) ) {
    Img.save ( FileName, qc::SaveTypeTrait<RealType>::BinarySaveType );
  }  else {
    // It is necessary to make a copy of Img to call setOverflowHandling, because Img is const
    qc::ScalarArray<RealType, qc::QC_2D> tempImg ( Img, aol::FLAT_COPY );
    if ( PGMPNGClipThenScale01 )
      tempImg.setOverflowHandling ( aol::CLIP_THEN_SCALE, 0., 1. );
    if ( aol::fileNameEndsWith ( FileName, ".pgm" ) )
      tempImg.save ( FileName, qc::PGM_UNSIGNED_CHAR_BINARY );
    else
      tempImg.savePNG ( FileName );
  }
}

// operator which determines the smallest and biggest absolute value of the gradient of a FE-function
template <typename ConfiguratorType>
class FindMinMaxNormOfGradientOp : public aol::Op< aol::Vector<typename ConfiguratorType::RealType>,
                                                   aol::Vec2<typename ConfiguratorType::RealType> > {
  public:
    typedef typename ConfiguratorType::RealType RealType;
  protected:
    const typename ConfiguratorType::InitType &_initializer;

  public:
    FindMinMaxNormOfGradientOp( const typename ConfiguratorType::InitType &Initializer )
    : _initializer( Initializer ) { }

    ~FindMinMaxNormOfGradientOp( ) {  }

    // Dest is an aol::Vec2, first component will contain the minimum, second the maximum.
    void apply( const aol::Vector<RealType> &Arg, aol::Vec2<RealType> &Dest ) const {
      // define a discrete img with the data of the argument. It is needed to evaluate the gradient.
      aol::DiscreteFunctionDefault<ConfiguratorType> *discrImg = new aol::DiscreteFunctionDefault<ConfiguratorType>( _initializer, Arg );

      // iterate all elements in the narrwo band, compute the desired values on the quadrature
      // points and assemble them in to the vectors.
      typedef typename ConfiguratorType::ElementIteratorType IteratorType;
      typedef typename ConfiguratorType::VecType VecType;
      typedef typename ConfiguratorType::BaseFuncSetType BaseFuncSetType;
      ConfiguratorType config( _initializer );
      const IteratorType end_it =  config.end();
      int globalDofs[ config.maxNumLocalDofs ];

      RealType minGrad = std::numeric_limits<RealType>::max();
      RealType maxGrad = 0.;

      // traverse the elements of the grid
      for ( IteratorType it = config.begin(); it != end_it; ++it ) {
        // assemble the local direction and weights belonging to the current element
        const BaseFuncSetType &bfs = config.getBaseFunctionSet ( *it );
        const int numQuadPoints = bfs.numQuadPoints( );

        VecType gradient;                         // evaluate the gradient
        RealType normGradient;                    // and compute the norm of it
        Dest[0] = 0.;                             // this will contain the maximum gradient norm

        const int numLocalDofs = config.getNumLocalDofs ( *it );
        for ( int i = 0; i < numLocalDofs; ++i ) {
          globalDofs[ i ] = config.localToGlobal ( *it, i );
        }

        for ( int q = 0; q < numQuadPoints; q++ ) {
          discrImg->evaluateGradientAtQuadPoint( (*it), q, gradient );
          normGradient = gradient.norm();

          if ( normGradient > maxGrad ) maxGrad = normGradient;
          if ( normGradient < minGrad ) minGrad = normGradient;

        }   // end quadrature-point-loop
      }   // end iterator-loop

      Dest[0] = minGrad;
      Dest[1] = maxGrad;
    }   // end apply-method

    void applyAdd( const aol::Vector<RealType> &, aol::Vec2<RealType> & ) const{
      throw aol::Exception( "FindBiggestGradientOp: applyAdd not implemented", __FILE__, __LINE__);
    }

};

/**
 * \author Berkels
 */
template <typename DataType, qc::Dimension Dim>
aol::Vec<Dim, typename aol::RealTrait<DataType>::RealType> getCenterOfMassOfArray ( const qc::Array<DataType> &Array ) {
  aol::Vec<Dim, typename aol::RealTrait<DataType>::RealType> centerOfMass;
  for ( qc::RectangularIterator<Dim> it ( Array ); it.notAtEnd(); ++it ) {
    for ( int i = 0; i < Dim; ++i )
      centerOfMass[i] += Array.get( *it ) * (*it)[i];
  }
  centerOfMass /= Array.size ();
  return centerOfMass;
}

} // end namespace qc

#endif
