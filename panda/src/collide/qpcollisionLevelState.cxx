// Filename: qpcollisionLevelState.cxx
// Created by:  drose (16Mar02)
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

#include "qpcollisionLevelState.h"
#include "collisionSolid.h"

////////////////////////////////////////////////////////////////////
//     Function: qpCollisionLevelState::clear
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void qpCollisionLevelState::
clear() {
  _colliders.clear();
  _local_bounds.clear();
  _current = 0;
  _colliders_with_geom = 0;
}

////////////////////////////////////////////////////////////////////
//     Function: qpCollisionLevelState::reserve
//       Access: Public
//  Description:
////////////////////////////////////////////////////////////////////
void qpCollisionLevelState::
reserve(int max_colliders) {
  _colliders.reserve(max_colliders);
  _local_bounds.reserve(max_colliders);
}

////////////////////////////////////////////////////////////////////
//     Function: qpCollisionLevelState::prepare_collider
//       Access: Public
//  Description: Adds the indicated Collider to the set of Colliders
//               in the current level state.
////////////////////////////////////////////////////////////////////
void qpCollisionLevelState::
prepare_collider(const ColliderDef &def) {
  int index = (int)_colliders.size();
  _colliders.push_back(def);

  CollisionSolid *collider = def._collider;
  const BoundingVolume &bv = collider->get_bound();
  if (!bv.is_of_type(GeometricBoundingVolume::get_class_type())) {
    _local_bounds.push_back((GeometricBoundingVolume *)NULL);
  } else {
    GeometricBoundingVolume *gbv;
    DCAST_INTO_V(gbv, bv.make_copy());
    gbv->xform(def._space);
    _local_bounds.push_back(gbv);
  }

  _current |= get_mask(index);

  if (def._node->get_collide_geom()) {
    _colliders_with_geom |= get_mask(index);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpCollisionLevelState::any_in_bounds
//       Access: Public
//  Description: Checks the bounding volume of the current node
//               against each of our colliders.  Eliminates from the
//               current collider list any that are outside of the
//               bounding volume.  Returns true if any colliders
//               remain, false if all of them fall outside this node's
//               bounding volume.
////////////////////////////////////////////////////////////////////
bool qpCollisionLevelState::
any_in_bounds() {
  const BoundingVolume &node_bv = node()->get_bound();
  if (node_bv.is_of_type(GeometricBoundingVolume::get_class_type())) {
    const GeometricBoundingVolume *node_gbv;
    DCAST_INTO_R(node_gbv, &node_bv, false);

    int num_colliders = get_num_colliders();
    for (int c = 0; c < num_colliders; c++) {
      if (has_collider(c)) {
        qpCollisionNode *collider = get_node(c);
        bool is_in = false;

        // Don't even bother testing the bounding volume if there are
        // no collide bits in common between our collider and this
        // node.
        CollideMask from_mask = collider->get_from_collide_mask();
        if (collider->get_collide_geom() ||
            (from_mask & node()->get_net_collide_mask()) != 0) {
          // There are bits in common, so go ahead and try the
          // bounding volume.
          const GeometricBoundingVolume *col_gbv =
            get_local_bound(c);
          if (col_gbv != (GeometricBoundingVolume *)NULL) {
            is_in = (node_gbv->contains(col_gbv) != 0);
          }
        }

        if (!is_in) {
          // This collider cannot intersect with any geometry at
          // this node or below.
          omit_collider(c);
        }
      }
    }
  }

  return has_any_collider();
}

////////////////////////////////////////////////////////////////////
//     Function: qpCollisionLevelState::apply_transform
//       Access: Public
//  Description: Applies the inverse transform from the current node,
//               if any, onto all the colliders in the level state.
////////////////////////////////////////////////////////////////////
void qpCollisionLevelState::
apply_transform() {
  const TransformState *node_transform = node()->get_transform();
  if (!node_transform->is_identity()) {
    CPT(TransformState) inv_transform = 
      node_transform->invert_compose(TransformState::make_identity());
    const LMatrix4f &mat = inv_transform->get_mat();

    // Now build the new bounding volumes list.
    BoundingVolumes new_bounds;

    int num_colliders = get_num_colliders();
    new_bounds.reserve(num_colliders);
    for (int c = 0; c < num_colliders; c++) {
      if (!has_collider(c) ||
          get_local_bound(c) == (GeometricBoundingVolume *)NULL) {
        new_bounds.push_back((GeometricBoundingVolume *)NULL);
      } else {
        const GeometricBoundingVolume *old_bound = get_local_bound(c);
        GeometricBoundingVolume *new_bound = 
          DCAST(GeometricBoundingVolume, old_bound->make_copy());
        new_bound->xform(mat);
        new_bounds.push_back(new_bound);
      }
    }
    
    _local_bounds = new_bounds;
  }    
}
