// Filename: xFileDataObjectArray.cxx
// Created by:  drose (07Oct04)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#include "xFileDataObjectArray.h"
#include "string_utils.h"
#include "indent.h"

TypeHandle XFileDataObjectArray::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: XFileDataObjectArray::is_complex_object
//       Access: Public, Virtual
//  Description: Returns true if this kind of data object is a complex
//               object that can hold nested data elements, false
//               otherwise.
////////////////////////////////////////////////////////////////////
bool XFileDataObjectArray::
is_complex_object() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: XFileDataObjectArray::add_element
//       Access: Public, Virtual
//  Description: Adds the indicated element as a nested data element,
//               if this data object type supports it.  Returns true
//               if added successfully, false if the data object type
//               does not support nested data elements.
////////////////////////////////////////////////////////////////////
bool XFileDataObjectArray::
add_element(XFileDataObject *element) {
  _nested_elements.push_back(element);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: XFileDataObjectArray::write_data
//       Access: Public, Virtual
//  Description: Writes a suitable representation of this node to an
//               .x file in text mode.
////////////////////////////////////////////////////////////////////
void XFileDataObjectArray::
write_data(ostream &out, int indent_level, const char *separator) const {
  if (!_nested_elements.empty()) {
    bool indented = false;
    for (size_t i = 0; i < _nested_elements.size() - 1; i++) {
      XFileDataObject *object = _nested_elements[i];
      if (object->is_complex_object() ||
          _nested_elements.size() > 16) {
        // If we have a "complex" nested object, or more than 16
        // elements in the array, output it on its own line.
        if (indented) {
          out << "\n";
          indented = false;
        }
        object->write_data(out, indent_level, ",");

      } else {
        // Otherwise, output them all on the same line.
        if (!indented) {
          indent(out, indent_level);
          indented = true;
        }
        out << *object << ", ";
      }
    }

    // The last object is the set is different, because it gets
    // separator instead of a semicolon, and it always gets a newline.
    XFileDataObject *object = _nested_elements.back();
    if (object->is_complex_object()) {
      if (indented) {
        out << "\n";
      }
      object->write_data(out, indent_level, separator);

    } else {
      if (!indented) {
        indent(out, indent_level);
      }
      out << *object << separator << "\n";
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: XFileDataObjectArray::get_num_elements
//       Access: Protected, Virtual
//  Description: Returns the number of nested data elements within the
//               object.  This may be, e.g. the size of the array, if
//               it is an array.
////////////////////////////////////////////////////////////////////
int XFileDataObjectArray::
get_num_elements() const {
  return _nested_elements.size();
}

////////////////////////////////////////////////////////////////////
//     Function: XFileDataObjectArray::get_element
//       Access: Protected, Virtual
//  Description: Returns the nth nested data element within the
//               object.
////////////////////////////////////////////////////////////////////
const XFileDataObject *XFileDataObjectArray::
get_element(int n) const {
  nassertr(n >= 0 && n < (int)_nested_elements.size(), NULL);
  return _nested_elements[n];
}
