// Filename: bioStreamPtr.h
// Created by:  drose (15Oct02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#ifndef BIOSTREAMPTR_H
#define BIOSTREAMPTR_H

#include "pandabase.h"

// This module is not compiled if OpenSSL is not available.
#ifdef HAVE_SSL

#include "bioStream.h"
#include "referenceCount.h"
#include <openssl/ssl.h>

////////////////////////////////////////////////////////////////////
//       Class : BioStreamPtr
// Description : A wrapper around an IBioStream object to make a
//               reference-counting pointer to it.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS BioStreamPtr : public ReferenceCount {
public:
  INLINE BioStreamPtr(IBioStream *stream);
  virtual ~BioStreamPtr();

  INLINE IBioStream &operator *() const;
  INLINE IBioStream *operator -> () const;
  INLINE operator IBioStream * () const;

  INLINE void set_stream(IBioStream *stream);
  INLINE IBioStream *get_stream() const;

  bool connect() const;
  
private:
  IBioStream *_stream;
};

#include "bioStreamPtr.I"

#endif  // HAVE_SSL


#endif


