// Filename: extractor.cxx
// Created by:  mike (09Jan97)
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
#if defined(WIN32_VC) && !defined(NO_PCH)
#include "downloader_headers.h"
#endif

#pragma hdrstop

#if !defined(WIN32_VC) || defined(NO_PCH)
#include "config_downloader.h"

#include <filename.h>
#include <error_utils.h>

#include <errno.h>
#endif

#include "extractor.h"

////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//     Function: Extractor::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
Extractor::
Extractor(void) {
  PT(Buffer) buffer = new Buffer(extractor_buffer_size);
  init(buffer);
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::Constructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
Extractor::
Extractor(PT(Buffer) buffer) {
  init(buffer);
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::Constructor
//       Access: Private
//  Description:
////////////////////////////////////////////////////////////////////
void Extractor::
init(PT(Buffer) buffer) {
  _initiated = false;
  nassertv(!buffer.is_null());
  _buffer = buffer;
  _mfile = NULL;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::Destructor
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
Extractor::
~Extractor(void) {
  if (_initiated == true)
    cleanup();
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::initiate
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
int Extractor::
initiate(Filename &source_file, const Filename &rel_path) {

  if (_initiated == true) {
    downloader_cat.error()
      << "Extractor::initiate() - Extraction has already been initiated"
      << endl;
    return EU_error_abort;
  }

  // Open source file
  _source_file = source_file;
  _source_file.set_binary();
  if (!_source_file.open_read(_read_stream)) {
    downloader_cat.error()
      << "Extractor::extract() - Error opening source file: "
      << _source_file << " : " << strerror(errno) << endl;
    return get_write_error();
  }

  _rel_path = rel_path;

  // Determine source file length
  _read_stream.seekg(0, ios::end);
  _source_file_length = _read_stream.tellg();
  _read_stream.seekg(0, ios::beg);

  _total_bytes_read = 0;
  _read_all_input = false;
  _handled_all_input = false;
  _mfile = new Multifile();
  _initiated = true;
  return EU_success;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::cleanup
//       Access: Private
//  Description:
////////////////////////////////////////////////////////////////////
void Extractor::
cleanup(void) {
  if (_initiated == false) {
    downloader_cat.error()
      << "Extractor::cleanup() - Extraction has not been initiated"
      << endl;
    return;
  }

  delete _mfile;
  _mfile = NULL;
  _read_stream.close();
  _source_file.unlink();
  _initiated = false;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::run
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
int Extractor::
run(void) {
  if (_initiated == false) {
    downloader_cat.error()
      << "Extractor::run() - Extraction has not been initiated"
      << endl;
    return EU_error_abort;
  }

  // See if there is anything left in the source file
  if (_read_all_input == false) {
    _read_stream.read(_buffer->_buffer, _buffer->get_length());
    _source_buffer_length = _read_stream.gcount();
    _total_bytes_read += _source_buffer_length;
    if (_read_stream.eof()) {
      nassertr(_total_bytes_read == _source_file_length, false);
      _read_all_input = true;
    }
  }

  char *buffer_start = _buffer->_buffer;
  int buffer_size = _source_buffer_length;

  // Write to the out file
  int write_ret = _mfile->write(buffer_start, buffer_size, _rel_path);
  if (write_ret == EU_success) {
    cleanup();
    return EU_success;
  } else if (write_ret < 0) {
    downloader_cat.error()
      << "Extractor::run() - got error from Multifile: " << write_ret
      << endl;
    return write_ret;
  }
  return EU_ok;
}

////////////////////////////////////////////////////////////////////
//     Function: Extractor::extract
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
bool Extractor::
extract(Filename &source_file, const Filename &rel_path) {
  int ret = initiate(source_file, rel_path);
  if (ret < 0)
    return false;
  for (;;) {
    ret = run();
    if (ret == EU_success)
      return true;
    if (ret < 0)
      return false;
  }
  return false;
}
