/*~--------------------------------------------------------------------------~*
 *  @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
 * /@@/////  /@@          @@////@@ @@////// /@@
 * /@@       /@@  @@@@@  @@    // /@@       /@@
 * /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
 * /@@////   /@@/@@@@@@@/@@       ////////@@/@@
 * /@@       /@@/@@//// //@@    @@       /@@/@@
 * /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
 * //       ///  //////   //////  ////////  //
 *
 * Copyright (c) 2016 Los Alamos National Laboratory, LLC
 * All rights reserved
 *~--------------------------------------------------------------------------~*/

#ifndef flecsi_mpi_dense_h
#define flecsi_mpi_dense_h

//----------------------------------------------------------------------------//
// POLICY_NAMESPACE must be defined before including storage_class.h!!!
// Using this approach allows us to have only one storage_class_t
// definintion that can be used by all data policies -> code reuse...
#define POLICY_NAMESPACE mpi
#include "flecsi/data/storage_class.h"
#undef POLICY_NAMESPACE
//----------------------------------------------------------------------------//

#include "flecsi/data/common/data_types.h"
#include "flecsi/data/common/privilege.h"
#include "flecsi/data/data_client.h"
#include "flecsi/data/data_handle.h"
#include "flecsi/execution/context.h"
#include "flecsi/utils/const_string.h"
#include "flecsi/utils/index_space.h"

#include <algorithm>
#include <memory>

///
/// \file
/// \date Initial file creation: Apr 7, 2016
///

namespace flecsi {
namespace data {
namespace mpi {

//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=//
// Helper type definitions.
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=//

//----------------------------------------------------------------------------//
// Dense handle.
//----------------------------------------------------------------------------//

//----------------------------------------------------------------------------//
// Dense accessor.
//----------------------------------------------------------------------------//

///
/// \brief dense_accessor_t provides logically array-based access to data
///        variables that have been registered in the data model.
///
/// \tparam T The type of the data variable. If this type is not
///           consistent with the type used to register the data, bad things
///           can happen. However, it can be useful to reinterpret the type,
///           e.g., when writing raw bytes. This class is part of the
///           low-level \e flecsi interface, so it is assumed that you
///           know what you are doing...
///
template<
  typename T,
  size_t EP,
  size_t SP,
  size_t GP
>
struct dense_handle_t : public data_handle__<T, EP, SP, GP>
{
  //--------------------------------------------------------------------------//
  // Type definitions.
  //--------------------------------------------------------------------------//

  using base = data_handle__<T, EP, SP, GP>;

  //--------------------------------------------------------------------------//
  // Constructors.
  //--------------------------------------------------------------------------//

  // FIXME: calling to base class constructor?
  ///
  /// Default constructor.
  ///
  dense_handle_t() {}

  template<typename, size_t, size_t, size_t>
  friend class dense_handle_t;
}; // struct dense_handle_t

//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=//
// Main type definition.
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=//

//----------------------------------------------------------------------------//
// Dense storage type.
//----------------------------------------------------------------------------//

///
/// FIXME: Dense storage type.
///
template<>
struct storage_class__<dense>
{
  //--------------------------------------------------------------------------//
  // Type definitions.
  //--------------------------------------------------------------------------//

  template<
    typename T,
    size_t EP,
    size_t SP,
    size_t GP
  >
  using handle_t = dense_handle_t<T, EP, SP, GP>;

  template<
    typename DATA_CLIENT_TYPE,
    typename DATA_TYPE,
    size_t NAMESPACE,
    size_t NAME,
    size_t VERSION,
    size_t PERMISSIONS
  >
  static
  handle_t<DATA_TYPE, 0, 0, 0>
  get_handle(
    const data_client_handle__<DATA_CLIENT_TYPE, PERMISSIONS>& client_handle
  )
  {
    handle_t<DATA_TYPE, 0, 0, 0> h;

    auto& context = execution::context_t::instance();
  
    using client_type = typename DATA_CLIENT_TYPE::type_identifier_t;

    // get field_info for this data handle
    auto& field_info =
      context.get_field_info_from_name(client_handle.key,
      utils::hash::field_hash<NAMESPACE, NAME>(VERSION));

    // get color_info for this field.
    auto& color_info = (context.coloring_info(field_info.index_space)).at(context.color());
    auto &index_coloring = context.coloring(field_info.index_space);

    auto& registered_field_data = context.registered_field_data();
    auto fieldDataIter = registered_field_data.find(field_info.fid);
    if (fieldDataIter == registered_field_data.end()) {
      size_t size = field_info.size * (color_info.exclusive +
                                       color_info.shared +
                                       color_info.ghost);
      // TODO: deal with VERSION
      context.register_field_data(field_info.fid,
                                  size);
      context.register_field_metadata<DATA_TYPE>(field_info.fid,
                                                 color_info,
                                                 index_coloring);
    }

    auto data = registered_field_data[field_info.fid].data();
    // populate data member of data_handle_t
    auto &hb = dynamic_cast<data_handle__<DATA_TYPE, 0, 0, 0>&>(h);

    hb.fid = field_info.fid;
    hb.index_space = field_info.index_space;
    hb.client_key = client_handle.key;

    hb.exclusive_size = color_info.exclusive;
    hb.combined_data = hb.exclusive_buf = hb.exclusive_data =
      reinterpret_cast<DATA_TYPE *>(data);
    hb.combined_size = color_info.exclusive;

    hb.shared_size = color_info.shared;
    hb.shared_data = hb.shared_buf = hb.exclusive_data + hb.exclusive_size;
    hb.combined_size += color_info.shared;

    hb.ghost_size = color_info.ghost;
    hb.ghost_data = hb.ghost_buf = hb.shared_data + hb.shared_size;
    hb.combined_size += color_info.ghost;

    return h;
  }

}; // struct storage_class_t

} // namespace mpi
} // namespace data
} // namespace flecsi

#endif // flecsi_mpi_dense_h

/*~-------------------------------------------------------------------------~-*
 * Formatting options
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/
