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

#ifndef flecsi_legion_color_h
#define flecsi_legion_color_h

#include <ostream>

//----------------------------------------------------------------------------//
// POLICY_NAMESPACE must be defined before including storage_class.h!!!
// Using this approach allows us to have only one storage_class__
// definintion that can be used by all data policies -> code reuse...
#define POLICY_NAMESPACE legion
#include "flecsi/data/storage_class.h"
#undef POLICY_NAMESPACE
//----------------------------------------------------------------------------//

#include "flecsi/utils/const_string.h"
#include "flecsi/data/data_client.h"
#include "flecsi/data/data_handle.h"
#include "flecsi/data/storage.h"
#include "flecsi/utils/const_string.h"
#include "flecsi/utils/index_space.h"
#include "flecsi/execution/context.h"
#include "flecsi/data/common/privilege.h"

///
// \file legion/color.h
// \authors Demeshko
// \date Initial file creation: July, 2017
///

namespace flecsi {
namespace data {
namespace legion {

//----------------------------------------------------------------------------//
// Global handle.
//----------------------------------------------------------------------------//

///-------------------------------------------------------------------------//
//! The color_handle_t provide an access to color variables that have
//! been registered in data model
//!
//! \tparam T The type of the data variable. If this type is not
//!           consistent with the type used to register the data, bad things
//!           can happen. However, it can be useful to reinterpret the type,
//!           e.g., when writing raw bytes. This class is part of the
//!           low-level \e flecsi interface, so it is assumed that you
//!           know what you are doing...
//!
//! @tparam PERMISSIONS The permissions to the handle
//!
//! @ingroup data
///--------------------------------------------------------------------------//

template<
  typename T,
  size_t PERMISSIONS
>
struct color_handle_t :
  public data_handle__<
    T,
    PERMISSIONS,
    0,
    0
    >
{
  //--------------------------------------------------------------------------//
  // Type definitions.
  //--------------------------------------------------------------------------//

  using base_t =
    data_handle__<
    T,
    PERMISSIONS,
    0,
    0
    >;

  //--------------------------------------------------------------------------//
  // Constructors.
  //--------------------------------------------------------------------------//

   color_handle_t()
   {
    base_t::color=true;
   }

  //--------------------------------------------------------------------------//
  // Destructor.
  //--------------------------------------------------------------------------//

  ~color_handle_t(){}

  ///
  // Copy constructor.
  ///
  template<size_t P2>
  color_handle_t(const color_handle_t<T, P2> & a)
    : base_t(reinterpret_cast<const base_t&>(a)),
      label_(a.label()),
      size_(a.size())
    {
      static_assert(P2 == 0, 
        "passing mapped handle to task args");
    }


  //--------------------------------------------------------------------------//
  // Member data interface.
  //--------------------------------------------------------------------------//

  ///
  // \brief Return a std::string containing the label of the data variable
  //        reference by this handle.
  ///
  const std::string &
  label() const
  { 
    return label_;
  } // label

  ///
  // \brief Return the index space size of the data variable
  //        referenced by this handle.
  ///
  size_t
  size() const
  { 
    return size_;
  } // size

  ///
  // \brief Test to see if this handle is empty
  //
  // \return true if registered.
  ///
  operator bool() const
  {
   return base_t::combined_data !=nullptr;
  } // operator bool

  private:
    std::string label_ = "";
    size_t size_=1;
}; // struct color_handle_t

//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=//
// Main type definition.
//+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=//

//----------------------------------------------------------------------------//
// Global storage type.
//----------------------------------------------------------------------------//

///
// FIXME: Color storage type.
///
template<>
struct storage_class__<color> {

  //--------------------------------------------------------------------------//
  // Type definitions.
  //--------------------------------------------------------------------------//

  template<
    typename T,
    size_t P
  >
  using handle_t = color_handle_t<T, P>;

  //--------------------------------------------------------------------------//
  // Data handles.
  //--------------------------------------------------------------------------//
  template<
    typename DATA_CLIENT_TYPE,
    typename DATA_TYPE,
    size_t NAMESPACE,
    size_t NAME,
    size_t VERSION,
    size_t PERMISSIONS 
  >
  static
  handle_t<DATA_TYPE, 0>
  get_handle(
    const data_client_handle__<DATA_CLIENT_TYPE, PERMISSIONS>& client_handle
  )
  {
    static_assert(VERSION < utils::hash::field_max_versions,
      "max field version exceeded");
    handle_t<DATA_TYPE, 0> h;
    auto& context = execution::context_t::instance();

    auto& field_info =
      context.get_field_info_from_name(client_handle.key,
      utils::hash::field_hash<NAMESPACE, NAME>(VERSION));

    size_t index_space = field_info.index_space;
    auto& ism = context.index_space_data_map();
    h.client_key = client_handle.key;
//    h.data_client_name_hash = client_handle.data_client_name_hash;
    h.color_region = ism[index_space].color_region;
    h.fid = field_info.fid;
    h.index_space = field_info.index_space;
    h.color = true;
    h.state = context.execution_state();

    return h;
  }


}; // struct storage_class__

} // namespace legion
} // namespace data
} // namespace flecsi

#endif // flecsi_legion_color_h

/*~-------------------------------------------------------------------------~-*
 * Formatting options
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/
