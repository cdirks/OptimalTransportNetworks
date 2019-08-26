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

/**
 * \file
 * \brief Implementation of anisotropic diffusion filtering in 3D according to Perona Malik
 *
 * Implementation of anisotropic diffusion filtering in 3D according to Perona Malik (Level Set case), i.e. solving
 * $$  \partial_t \Phi - \|\nabla \Phi\| \div \left( g(\|\nabla \Phi\|) \frac{ \nabla \Phi}{\| \nabla \Phi\|} \right) =0, \quad g(s)=\frac{1}{1+\left(\frac{s}{\lambda}\right)^2}. $$
 *
 * \author Oliver Nemitz
 *
 * \todo Seems to be broken (2011-09-28)
*/

#include <configurators.h>
#include <quoc.h>
#include <FEOpInterface.h>
#include <solver.h>
#include <preconditioner.h>

#include <fastUniformGridMatrix.h>
#include <qmException.h>
#include <aol.h>
#include <parameterParser.h>

#include <timestepSaver.h>
#include <anisoStiffOps.h>
#include <mcm.h>
#include <configurators.h>

// a typedef for the configurator which contains the RealType, the dimension, the kind of quadrature etc.
typedef qc::QuocConfiguratorTraitMultiLin<double, qc::QC_3D, aol::GaussQuadrature<double,qc::QC_3D,3> > ConfigType;

using namespace aol::color;


// now the main program
int main( int argc, char **argv ) {

  // read the parameters from a parameter-file which is given as an argument
  if ( argc != 2 ) {
    string s = "USAGE: ";
    s += argv[0];
    s += " <parameterfile>";
    throw aol::Exception( s.c_str(), __FILE__, __LINE__  );
  }

  try {
    aol::ParameterParser parser( argv[1] );         // open the parameterfile

    // -------------- load image into scalar array -----------------------
    char loadName[ 1024 ];
    parser.getString( "loadName", loadName );       // read the content of the entry "loadName" from the parameterfile
    cerr<<"Restoring image...";
    qc::ScalarArray<double, qc::QC_3D> img( loadName );      // declare a ScalarArray<QC_3D> and load the desired content
    qc::ScalarArray<double, qc::QC_3D> rhs( img );

    int N = img.getNumX ();                         // width of the ScalarArray<QC_3D> (=width of one side of the cube)
    int d = qc::logBaseTwo (N);                   // get the level from this width
    qc::GridDefinition grid( d, qc::QC_3D );        // the grid is the underlying structure

    // ---------------- the timestep-saver -----------------------------
    char saveName[ 1024 ];                          // declare the timestepSaver which just helps you saving every
    parser.getString( "saveName", saveName );       // n-th timestep.
    aol::TimestepSaver<double> tsSaver( parser.getInt("timeOffset"), saveName );    // timeOffset = n
    tsSaver.setNumberSaveFirstPics ( parser.getInt( "numberSaveFirstPics" ) );

    // ------------------ Operators ------------------------------------
    // AnisotropicDiffusion3DLevelSetStiffOp is the operator which computes
    // \f$ int g(\nabla \Phi) \frac{ \nabla \Phi \nabla \Theta}{\| \nabla \Phi\|} \f$
    qc::PeronaMalikLevelSetStiffOp< ConfigType > L( grid );
    // MCMMassOp is the operator which computes \f$ int \frac{ \Phi \Theta}{\| \nabla \Phi\|} \f$
    qc::MCMMassOp< ConfigType > M( grid );
    // \nabla \Phi in the denominator of the operators is computed from the last timestep which is
    // stored in "img", therefore the operators must have a reference to "img".
    M.setImageReference( img );
    L.setImageReference( img );

    double tau = 0.5 * parser.getDouble("tau") * grid.H();      // get the timestep-Size from the parameterfile

    qc::FastUniformGridMatrix<double, qc::QC_3D> mat( grid );   // this is matrix in which the operators are assembled

    // Here an SSOR-preconditioner is used which gets the matrix as an argument
    aol::SSORPreconditioner<aol::Vector<double>, qc::FastUniformGridMatrix<double,qc::QC_3D> > precond( mat );
    // this is the solver for our linear system of equations, here a pcg-method is used. It gets the matrix,
    // the preconditioner, the accuracy and a maximal number of iterations.
    aol::PCGInverse<aol::Vector<double> > solver( mat, precond, 1e-16, 1000 );


    // ---------------------------------------------------------------------
    aol::StopWatch watch;           // stopwatch for the whole evolution
    watch.start();                  // is started now.

    // ------------- timestep-loop -----------------------------------
    for ( int iter=0; iter<parser.getInt("timesteps"); ++iter ) {

      cerr << blue << "step " << iter <<": "<<red;

      cerr << "assembling and applying (M + Tau*L).";
      mat.setZero();
      L.assembleAddMatrix( mat );       // just as it says... (mat=L)
      cerr << ".";
      mat *= tau;                       // multiply L with tau (mat = tau*L)
      M.assembleAddMatrix( mat );       // assembled M (mat = M + tau*L)
      cerr << ".done, finishing rhs ...";

      M.apply( img, rhs );
      cerr << "done.\nSolving ...";

      solver.apply( rhs, img );         // solve the linear system of equations
      cerr << "done!\n";

      // ------------------ save every timeOffset'th timestep ------------------------
      tsSaver.saveTimestepBZ2( iter, img, grid );   // this will only save, if iter % timeOffset == 0.

    }


    cerr<<"ready!\n";

    watch.stop();
    cerr << "elapsed = " << watch.elapsedCpuTime() << "s.\n";    // how long took this program?


  } catch ( aol::Exception &ex ) {
    ex.dump();
  }

  return EXIT_SUCCESS;

}

