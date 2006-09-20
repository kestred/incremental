// Filename: sequenceNode.cxx
// Created by:  drose (06Mar02)
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

#include "pandabase.h"
#include "sequenceNode.h"
#include "cullTraverser.h"

TypeHandle SequenceNode::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::Copy Constructor
//       Access: Protected
//  Description:
////////////////////////////////////////////////////////////////////
SequenceNode::
SequenceNode(const SequenceNode &copy) :
  SelectiveChildNode(copy),
  AnimInterface(copy)
{
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::get_num_frames
//       Access: Published, Virtual
//  Description: Returns the number of frames in the animation.  This
//               is a property of the animation and may not be
//               directly adjusted by the user (although it may change
//               without warning with certain kinds of animations,
//               since this is a virtual method that may be
//               overridden).
////////////////////////////////////////////////////////////////////
int SequenceNode::
get_num_frames() const {
  return get_num_children();
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::safe_to_combine
//       Access: Public, Virtual
//  Description: Returns true if it is generally safe to combine this
//               particular kind of PandaNode with other kinds of
//               PandaNodes, adding children or whatever.  For
//               instance, an LODNode should not be combined with any
//               other PandaNode, because its set of children is
//               meaningful.
////////////////////////////////////////////////////////////////////
bool SequenceNode::
safe_to_combine() const {
  return false;
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::make_copy
//       Access: Public, Virtual
//  Description: Returns a newly-allocated Node that is a shallow copy
//               of this one.  It will be a different Node pointer,
//               but its internal data may or may not be shared with
//               that of the original Node.
////////////////////////////////////////////////////////////////////
PandaNode *SequenceNode::
make_copy() const {
  return new SequenceNode(*this);
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::cull_callback
//       Access: Public, Virtual
//  Description: This function will be called during the cull
//               traversal to perform any additional operations that
//               should be performed at cull time.  This may include
//               additional manipulation of render state or additional
//               visible/invisible decisions, or any other arbitrary
//               operation.
//
//               Note that this function will *not* be called unless
//               set_cull_callback() is called in the constructor of
//               the derived class.  It is necessary to call
//               set_cull_callback() to indicated that we require
//               cull_callback() to be called.
//
//               By the time this function is called, the node has
//               already passed the bounding-volume test for the
//               viewing frustum, and the node's transform and state
//               have already been applied to the indicated
//               CullTraverserData object.
//
//               The return value is true if this node should be
//               visible, or false if it should be culled.
////////////////////////////////////////////////////////////////////
bool SequenceNode::
cull_callback(CullTraverser *, CullTraverserData &) {
  select_child(get_frame());
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::has_single_child_visibility
//       Access: Public, Virtual
//  Description: Should be overridden by derived classes to return
//               true if this kind of node has the special property
//               that just one of its children is visible at any given
//               time, and furthermore that the particular visible
//               child can be determined without reference to any
//               external information (such as a camera).  At present,
//               only SequenceNodes and SwitchNodes fall into this
//               category.
//
//               If this function returns true, get_visible_child()
//               can be called to return the index of the
//               currently-visible child.
////////////////////////////////////////////////////////////////////
bool SequenceNode::
has_single_child_visibility() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::output
//       Access: Published, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
void SequenceNode::
output(ostream &out) const {
  out << get_type() << " " << get_name() << ": ";
  AnimInterface::output(out);
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::register_with_read_factory
//       Access: Public, Static
//  Description: Tells the BamReader how to create objects of type
//               SequenceNode.
////////////////////////////////////////////////////////////////////
void SequenceNode::
register_with_read_factory() {
  BamReader::get_factory()->register_factory(get_class_type(), make_from_bam);
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::write_datagram
//       Access: Public, Virtual
//  Description: Writes the contents of this object to the datagram
//               for shipping out to a Bam file.
////////////////////////////////////////////////////////////////////
void SequenceNode::
write_datagram(BamWriter *manager, Datagram &dg) {
  SelectiveChildNode::write_datagram(manager, dg);
  AnimInterface::write_datagram(manager, dg);
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::make_from_bam
//       Access: Protected, Static
//  Description: This function is called by the BamReader's factory
//               when a new object of type SequenceNode is encountered
//               in the Bam file.  It should create the SequenceNode
//               and extract its information from the file.
////////////////////////////////////////////////////////////////////
TypedWritable *SequenceNode::
make_from_bam(const FactoryParams &params) {
  SequenceNode *node = new SequenceNode("");
  DatagramIterator scan;
  BamReader *manager;

  parse_params(params, scan, manager);
  node->fillin(scan, manager);

  return node;
}

////////////////////////////////////////////////////////////////////
//     Function: SequenceNode::fillin
//       Access: Protected
//  Description: This internal function is called by make_from_bam to
//               read in all of the relevant data from the BamFile for
//               the new SequenceNode.
////////////////////////////////////////////////////////////////////
void SequenceNode::
fillin(DatagramIterator &scan, BamReader *manager) {
  SelectiveChildNode::fillin(scan, manager);
  AnimInterface::fillin(scan, manager);
}
