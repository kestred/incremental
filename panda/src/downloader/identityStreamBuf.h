// Filename: identityStreamBuf.h
// Created by:  drose (09Oct02)
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

#ifndef IDENTITYSTREAMBUF_H
#define IDENTITYSTREAMBUF_H

#include "pandabase.h"

// This module is not compiled if OpenSSL is not available.
#ifdef HAVE_SSL

#include "httpDocument.h"
#include "bioStreamPtr.h"
#include "pointerTo.h"

////////////////////////////////////////////////////////////////////
//       Class : IdentityStreamBuf
// Description : The streambuf object that implements
//               IIdentityStream.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEXPRESS IdentityStreamBuf : public streambuf {
public:
  IdentityStreamBuf();
  virtual ~IdentityStreamBuf();

  void open_read(BioStreamPtr *source, HTTPDocument *doc,
                 bool has_content_length, size_t content_length);
  void close_read();

protected:
  virtual int underflow(void);

private:
  size_t read_chars(char *start, size_t length);

  PT(BioStreamPtr) _source;
  bool _has_content_length;
  size_t _bytes_remaining;

  PT(HTTPDocument) _doc;
  int _read_index;

  friend class IIdentityStream;
};

#endif  // HAVE_SSL

#endif
