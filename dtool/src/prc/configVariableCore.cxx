// Filename: configVariableCore.cxx
// Created by:  drose (15Oct04)
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

#include "configVariableCore.h"
#include "configDeclaration.h"
#include "configPage.h"
#include "pset.h"
#include "notify.h"
#include "config_prc.h"

#include <algorithm>


////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::Constructor
//       Access: Private
//  Description: Use the ConfigVariableManager::make_variable() 
//               interface to create a new ConfigVariableCore.
////////////////////////////////////////////////////////////////////
ConfigVariableCore::
ConfigVariableCore(const string &name) :
  _name(name),
  _is_used(false),
  _value_type(VT_undefined),
  _flags(0),
  _default_value(NULL),
  _local_value(NULL),
  _declarations_sorted(true),
  _value_queried(false),
  _value_seq(0)
{
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::Copy Constructor
//       Access: Private
//  Description: This is used by ConfigVariableManager to create the
//               variable from a template--basically, another variable
//               with all of the initial properties pre-defined.
////////////////////////////////////////////////////////////////////
ConfigVariableCore::
ConfigVariableCore(const ConfigVariableCore &templ, const string &name) :
  _name(name),
  _is_used(templ._is_used),
  _value_type(templ._value_type),
  _description(templ._description),
  _flags(templ._flags),
  _default_value(NULL),
  _local_value(NULL),
  _declarations_sorted(false),
  _value_queried(false),
  _value_seq(0)
{
  if (templ._default_value != (ConfigDeclaration *)NULL) {
    set_default_value(templ._default_value->get_string_value());
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::Destructor
//       Access: Private
//  Description: The destructor should never be called;
//               ConfigVariableCore objects live forever and never get
//               destructed.
////////////////////////////////////////////////////////////////////
ConfigVariableCore::
~ConfigVariableCore() {
  prc_cat->error()
    << "Internal error--ConfigVariableCore destructor called!\n";
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::set_value_type
//       Access: Public
//  Description: Specifies the type of this variable.  See
//               get_value_type().  It is not an error to call this
//               multiple times, but if the value changes once
//               get_declaration() has been called, a warning is printed.
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
set_value_type(ConfigVariableCore::ValueType value_type) {
  if (_value_queried && _value_type != value_type) {
    if (_description == "DConfig") {
      // As a special exception, if the current description is
      // "DConfig", we don't report a warning for changing the type,
      // assuming the variable is being defined through the older
      // DConfig interface.
      
    } else {
      prc_cat->warning()
        << "changing type for ConfigVariable " 
        << get_name() << " from " << _value_type << " to " 
        << value_type << ".\n";
    }
  }

  _value_type = value_type;
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::set_flags
//       Access: Public
//  Description: Specifies the trust level of this variable.  See
//               get_flags().  It is not an error to call this
//               multiple times, but if the value changes once
//               get_declaration() has been called, a warning is printed.
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
set_flags(int flags) {
  if (_value_queried && _flags != flags) {
    prc_cat->warning()
      << "changing trust level for ConfigVariable " 
      << get_name() << " from " << _flags << " to " 
      << flags << ".\n";
  }

  _flags = flags;

  // Changing the trust level will require re-sorting the
  // declarations.
  _declarations_sorted = false;
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::set_description
//       Access: Public
//  Description: Specifies the one-line description of this variable.
//               See get_description().  It is not an error to call
//               this multiple times, but if the value changes once
//               get_declaration() has been called, a warning is printed.
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
set_description(const string &description) {
  if (_value_queried && _description != description) {
    if (description == "DConfig") {
      // As a special exception, if the new description is "DConfig",
      // we don't change it, since this is presumably coming from the
      // older DConfig interface.
      return;
    }
      
    prc_cat->warning()
      << "changing description for ConfigVariable " 
      << get_name() << ".\n";
  }

  _description = description;
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::set_default_value
//       Access: Public
//  Description: Specifies the default value for this variable if it
//               is not defined in any prc file.
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
set_default_value(const string &default_value) {
  if (_default_value == (ConfigDeclaration *)NULL) {
    // Defining the default value for the first time.
    ConfigPage *default_page = ConfigPage::get_default_page();
    _default_value = default_page->make_declaration(this, default_value);

  } else {
    // Modifying an existing default value.

    if (_default_value->get_string_value() != default_value) {
      if (_description == "DConfig") {
        // As a special exception, if the current description is
        // "DConfig", we don't report a warning for changing the
        // default value, assuming the variable is being defined
        // through the older DConfig interface.

      } else {
        prc_cat->warning()
          << "changing default value for ConfigVariable " 
          << get_name() << " from '" << _default_value->get_string_value()
          << "' to '" << default_value << "'.\n";
      }
      _default_value->set_string_value(default_value);
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::make_local_value
//       Access: Public
//  Description: Creates a new local value for this variable, if there
//               is not already one specified.  This will shadow any
//               values defined in the various .prc files.
//
//               If there is already a local value defined for this
//               variable, simply returns that one.
//
//               Use clear_local_value() to remove the local value
//               definition.
////////////////////////////////////////////////////////////////////
ConfigDeclaration *ConfigVariableCore::
make_local_value() {
  if (_local_value == (ConfigDeclaration *)NULL) {
    ConfigPage *local_page = ConfigPage::get_local_page();
    string string_value = get_declaration(0)->get_string_value();
    _local_value = local_page->make_declaration(this, string_value);

    if (is_closed()) {
      prc_cat.warning()
        << "Assigning a local value to a \"closed\" ConfigVariable.  "
        "This is legal in a development build, but illegal in a release "
        "build and may result in a compilation error or exception.\n";
    }
  }

  // Assume that everytime someone asks for the local value, they're
  // about to change it; further assume that no one changes the local
  // value without calling this method immediately before.
  _value_seq++;

  return _local_value;
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::clear_local_value
//       Access: Public
//  Description: Removes the local value defined for this variable,
//               and allows its value to be once again retrieved from
//               the .prc files.
//
//               Returns true if the value was successfully removed,
//               false if it did not exist in the first place.
////////////////////////////////////////////////////////////////////
bool ConfigVariableCore::
clear_local_value() {
  if (_local_value != (ConfigDeclaration *)NULL) {
    ConfigPage::get_local_page()->delete_declaration(_local_value);
    _local_value = NULL;
    _value_seq++;
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::has_value
//       Access: Public
//  Description: Returns true if this variable has an explicit value,
//               either from a prc file or locally set, or false if
//               variable has its default value.
////////////////////////////////////////////////////////////////////
bool ConfigVariableCore::
has_value() const {
  if (has_local_value()) {
    return true;
  }
  check_sort_declarations();
  return (!_trusted_declarations.empty());
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::get_num_declarations
//       Access: Public
//  Description: Returns the number of declarations that contribute to
//               this variable's value.  If the variable has been
//               defined, this will always be at least 1 (for the
//               default value, at least).
////////////////////////////////////////////////////////////////////
int ConfigVariableCore::
get_num_declarations() const {
  if (has_local_value()) {
    return 1;
  }
  check_sort_declarations();
  if (!_trusted_declarations.empty()) {
    return _trusted_declarations.size();
  }

  // We always have at least one: the default value.
  return 1;
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::get_declaration
//       Access: Public
//  Description: Returns the nth declarations that contributes to
//               this variable's value.  The declarations are arranged
//               in order such that earlier declarations shadow later
//               declarations; thus, get_declaration(0) is always
//               defined and always returns the current value of the
//               variable.
////////////////////////////////////////////////////////////////////
const ConfigDeclaration *ConfigVariableCore::
get_declaration(int n) const {
  ((ConfigVariableCore *)this)->_value_queried = true;
  if (_default_value == (ConfigDeclaration *)NULL) {
    prc_cat->warning()
      << "value queried before default value set for "
      << get_name() << ".\n";
    ((ConfigVariableCore *)this)->set_default_value("");
  }

  if (has_local_value()) {
    return _local_value;
  }
  check_sort_declarations();
  if (n >= 0 && n < (int)_trusted_declarations.size()) {
    return _trusted_declarations[n];
  }
  return _default_value;
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::output
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
output(ostream &out) const {
  out << get_declaration(0)->get_string_value();
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::write
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
write(ostream &out) const {
  out << "ConfigVariable " << get_name() << ":\n";

  check_sort_declarations();

  if (has_local_value()) {
    out << "  " << *_local_value << "  (defined locally)\n";
  }

  Declarations::const_iterator di;
  for (di = _trusted_declarations.begin(); 
       di != _trusted_declarations.end(); 
       ++di) {
    out << "  " << *(*di) 
        << "  (from " << (*di)->get_page()->get_name() << ")\n";
  }

  if (_default_value != (ConfigDeclaration *)NULL) {
    out << "  " << *_default_value << "  (default value)\n";
  }

  for (di = _untrusted_declarations.begin(); 
       di != _untrusted_declarations.end(); 
       ++di) {
    out << "  " << *(*di) 
        << "  (from " << (*di)->get_page()->get_name() << ", untrusted)\n";
  }

  if (!_description.empty()) {
    out << "\n" << _description << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::add_declaration
//       Access: Private
//  Description: Called only by the ConfigDeclaration constructor,
//               this adds the indicated declaration to the list of
//               declarations that reference this variable.
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
add_declaration(ConfigDeclaration *decl) {
  _declarations.push_back(decl);

  _declarations_sorted = false;

  if (!has_local_value()) {
    _value_seq++;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::remove_declaration
//       Access: Private
//  Description: Called only by the ConfigDeclaration destructor,
//               this removes the indicated declaration from the list
//               of declarations that reference this variable.
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
remove_declaration(ConfigDeclaration *decl) {
  Declarations::iterator di;
  for (di = _declarations.begin(); di != _declarations.end(); ++di) {
    if ((*di) == decl) {
      // Rather than deleting the declaration from the middle of the
      // list, we maybe save a bit of time by swapping in the one at
      // the end of the list (although this will unsort the list).
      Declarations::iterator di2 = _declarations.end();
      di2--;
      (*di) = (*di2);
      _declarations.erase(di2);
      _declarations_sorted = false;
      if (!has_local_value()) {
        _value_seq++;
      }
      return;
    }
  }

  // Hmm, it wasn't here.  Oh well.
}

// This class is used in sort_declarations, below.
class CompareConfigDeclarations {
public:
  bool operator () (const ConfigDeclaration *a, const ConfigDeclaration *b) const {
    return (*a) < (*b);
  }
};

////////////////////////////////////////////////////////////////////
//     Function: ConfigVariableCore::sort_declarations
//       Access: Private
//  Description: Sorts the list of declarations into priority order,
//               so that the declaration at the front of the list is
//               the one that shadows all following declarations.
////////////////////////////////////////////////////////////////////
void ConfigVariableCore::
sort_declarations() {
  sort(_declarations.begin(), _declarations.end(), CompareConfigDeclarations());
  Declarations::iterator di;

  // Now that they're sorted, divide them into either trusted or
  // untrusted declarations.
#ifdef PRC_RESPECT_TRUST_LEVEL
  // In this mode, normally for a release build, we sort the
  // declarations honestly according to whether the prc file that
  // defines them meets the required trust level.
  _trusted_declarations.clear();
  _untrusted_declarations.clear();
  for (di = _declarations.begin(); di != _declarations.end(); ++di) {
    const ConfigDeclaration *decl = (*di);
    if (!is_closed() &&
        get_trust_level() <= decl->get_page()->get_trust_level()) {
      _trusted_declarations.push_back(decl);
    } else {
      _untrusted_declarations.push_back(decl);
    }
  }

#else  // PRC_RESPECT_TRUST_LEVEL
  // In this mode, normally for the development environment, all
  // declarations are trusted, regardless of the trust level.
  _trusted_declarations = _declarations;
  _untrusted_declarations.clear();

#endif  // PRC_RESPECT_TRUST_LEVEL

  // Finally, determine the set of unique, trusted
  // declarations--trusted declarations that have a unique string
  // value.  This is usually unneeded, but what the heck, it doesn't
  // need to be recomputed all that often.
  _unique_declarations.clear();

  pset<string> already_added;
  for (di = _trusted_declarations.begin(); 
       di != _trusted_declarations.end(); 
       ++di) {
    const ConfigDeclaration *decl = (*di);
    if (already_added.insert(decl->get_string_value()).second) {
      _unique_declarations.push_back(decl);
    }
  }

  _declarations_sorted = true;
}
