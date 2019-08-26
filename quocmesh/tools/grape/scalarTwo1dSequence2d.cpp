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

//! @file
//!
//! @brief Load two scalar functions with 1d domain and a sequence of scalar function with 2d domain into GRAPE as two different objects.
//!
//! Usage: \code ./scalarTwo1dSequence2d data1d data1d data2d-step1 [data2d-step2 ...] \endcode
//!
//! GRAPE is started with two 1d triang objects and one time dependent 2d mesh object each having one scalar function.
//! The different 2d functions need to reside on the same grid, as they are interpreted as time steps (for times 0, 1, 2, ...) of one scalar function.
//!
//! @author Lenz

#include "grapeInterface1d.h"
#include "grapeInterface2d.h"
#include <qmException.h>
#include <fstream>
using namespace std;

#ifdef USE_EXTERNAL_GRAPE

int main (int argc, char** argv)
{
  try {
    if (argc < 4) {
      cerr << "usage: " << argv [0] << " data1d data1d data2d-step1 [data2d-step2 ...]" << endl;
      return 23;
    }

    // load data into scalar arrays
    qc::ScalarArray<double, qc::QC_1D> data1d (argv [1]);
    qc::ScalarArray<double, qc::QC_1D> data1d2 (argv [2]);
    qc::ScalarArray<double, qc::QC_2D> data2d (argv [3]);

    // convert these to a genmesh (it will automatically be tested
    // whether the data is quadratic or not)
    GENMESH2D* mesh2d = quocmesh_convert_to_gmesh2d (&data2d, "mesh2d");
    TRIANG1D*  triang = quocmesh_convert_to_triang1d (&data1d, "triang1d");
    TRIANG1D*  triang2 = quocmesh_convert_to_triang1d (&data1d2, "triang1d");

    // For all timesteps
    for (int i = 4; i < argc; ++i) {

      // load data into scalar array and add to time sequence
      qc::ScalarArray<double, qc::QC_2D>* data = new qc::ScalarArray<double, qc::QC_2D> (argv [i]);
      addTimestep (mesh2d, data, "mesh2d");
    }

    // Connect objects via scenes
    addMethodsAndProjects1d();
    addMethodsAndProjects2d(); // Has to be done before mesh softcopy

    SCENE* scene1d = reinterpret_cast<SCENE*> (GRAPE (Scene, "new-instance") ("scene1d"));
    SCENE* scene1d2 = reinterpret_cast<SCENE*> (GRAPE (Scene, "new-instance") ("scene1d2"));
    TIMESCENE* tsc = reinterpret_cast<TIMESCENE*> (GRAPE (TimeScene, "new-instance") ("timescene2d"));
    tsc->dynamic = reinterpret_cast<TREEOBJECT*> (mesh2d);
    tsc->object = reinterpret_cast<TREEOBJECT*> (GRAPE (mesh2d, "softcopy") (NULL));
    ASSIGN (scene1d->object, reinterpret_cast<TREEOBJECT*> (triang));
    ASSIGN (scene1d2->object, reinterpret_cast<TREEOBJECT*> (triang2));
    ASSIGN (scene1d->next_scene, scene1d2);
    ASSIGN (scene1d2->next_scene, reinterpret_cast<SCENE*> (tsc));

    // and then start GRAPE, thats it!
    GRAPE (GRAPE (Manager, "get-stdmgr") (), "handle") (scene1d);
  }
  catch(aol::Exception e) {
    e.dump();
    return 42;
  }

  return 0;
}

#else

int main ( int, char** ) {
  cerr << "Without grape external, this program is useless" << endl;
  return ( 0 ) ;
}

#endif
