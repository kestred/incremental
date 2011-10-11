// Filename: pta_LMatrix4.h
// Created by:  drose (27Feb10)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#ifndef PTA_LMATRIX4_H
#define PTA_LMATRIX4_H

#include "pandabase.h"
#include "luse.h"
#include "pointerToArray.h"

////////////////////////////////////////////////////////////////////
//       Class : PTA_LMatrix4f
// Description : A pta of LMatrix4fs.  This class is defined once here,
//               and exported to PANDA.DLL; other packages that want
//               to use a pta of this type (whether they need to
//               export it or not) should include this header file,
//               rather than defining the pta again.
////////////////////////////////////////////////////////////////////

EXPORT_TEMPLATE_CLASS(EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL, PointerToBase<ReferenceCountedVector<LMatrix4f> >)
EXPORT_TEMPLATE_CLASS(EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL, PointerToArrayBase<LMatrix4f>)
EXPORT_TEMPLATE_CLASS(EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL, PointerToArray<LMatrix4f>)
EXPORT_TEMPLATE_CLASS(EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL, ConstPointerToArray<LMatrix4f>)

typedef PointerToArray<LMatrix4f> PTA_LMatrix4f;
typedef ConstPointerToArray<LMatrix4f> CPTA_LMatrix4f;

////////////////////////////////////////////////////////////////////
//       Class : PTA_LMatrix4d
// Description : A pta of LMatrix4ds.  This class is defined once here,
//               and exported to PANDA.DLL; other packages that want
//               to use a pta of this type (whether they need to
//               export it or not) should include this header file,
//               rather than defining the pta again.
////////////////////////////////////////////////////////////////////

EXPORT_TEMPLATE_CLASS(EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL, PointerToBase<ReferenceCountedVector<LMatrix4d> >)
EXPORT_TEMPLATE_CLASS(EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL, PointerToArrayBase<LMatrix4d>)
EXPORT_TEMPLATE_CLASS(EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL, PointerToArray<LMatrix4d>)
EXPORT_TEMPLATE_CLASS(EXPCL_PANDA_MATHUTIL, EXPTP_PANDA_MATHUTIL, ConstPointerToArray<LMatrix4d>)

typedef PointerToArray<LMatrix4d> PTA_LMatrix4d;
typedef ConstPointerToArray<LMatrix4d> CPTA_LMatrix4d;

#ifndef STDFLOAT_DOUBLE
typedef PTA_LMatrix4f PTA_LMatrix4;
typedef CPTA_LMatrix4f CPTA_LMatrix4;
#else
typedef PTA_LMatrix4d PTA_LMatrix4;
typedef CPTA_LMatrix4d CPTA_LMatrix4;
#endif  // STDFLOAT_DOUBLE

// Bogus typedefs for interrogate and legacy Python code.
#ifdef CPPPARSER
typedef PTA_LMatrix4 PTAMat4;
typedef CPTA_LMatrix4 CPTAMat4;
typedef PTA_LMatrix4d PTAMat4d;
typedef CPTA_LMatrix4d CPTAMat4d;
#endif  // CPPPARSER

// Tell GCC that we'll take care of the instantiation explicitly here.
#ifdef __GNUC__
#pragma interface
#endif

#endif
