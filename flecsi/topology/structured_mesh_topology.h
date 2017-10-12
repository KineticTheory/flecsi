/*~--------------------------------------------------------------------------~*
 *  @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
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

#ifndef flecsi_structured_mesh_topology_h
#define flecsi_structured_mesh_topology_h


#include <cassert>
#include <iostream>
#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <functional>
#include <type_traits>

#include "flecsi/utils/common.h"
#include "flecsi/utils/set_intersection.h"
#include "flecsi/utils/static_verify.h"
#include "flecsi/topology/structured_mesh_types.h"

namespace flecsi {
namespace topology {
namespace verify_mesh {

  FLECSI_MEMBER_CHECKER(num_dimensions);
  FLECSI_MEMBER_CHECKER(num_domains);
  FLECSI_MEMBER_CHECKER(lower_bounds);
  FLECSI_MEMBER_CHECKER(upper_bounds);
  FLECSI_MEMBER_CHECKER(entity_types);
} // namespace verify_mesh

template<
  class MT
>
class structured_mesh_topology_t : public structured_mesh_topology_base_t
{
 
  /*
  * Verify the existence of following fields in the mesh policy MT
  * num_dimensions
  * num_domains
  * entity_types
  * mesh_bounds 
  */
  // static verification of mesh policy

  static_assert(verify_mesh::has_member_num_dimensions<MT>::value,
                "mesh policy missing num_dimensions size_t");
  
  static_assert(std::is_convertible<decltype(MT::num_dimensions),
    size_t>::value, "mesh policy num_dimensions must be size_t");              

  static_assert(verify_mesh::has_member_num_domains<MT>::value,
                "mesh policy missing num_domains size_t");
  
  static_assert(std::is_convertible<decltype(MT::num_domains),
    size_t>::value, "mesh policy num_domains must be size_t");

  static_assert(verify_mesh::has_member_lower_bounds<MT>::value,
                "mesh policy missing lower_bounds array");
  
  static_assert(std::is_convertible<decltype(MT::lower_bounds),
    std::array<size_t,MT::num_dimensions>>::value,
    "mesh policy lower_bounds is not an array");

  static_assert(verify_mesh::has_member_upper_bounds<MT>::value,
                "mesh policy missing upper_bounds array");
  
  static_assert(std::is_convertible<decltype(MT::upper_bounds),
    std::array<size_t,MT::num_dimensions>>::value,
    "mesh policy upper_bounds is not an array");

  static_assert(MT::lower_bounds.size() == MT::upper_bounds.size(),
     "mesh bounds have inconsistent sizes");

  static_assert(verify_mesh::has_member_entity_types<MT>::value,
                "mesh policy missing entity_types tuple");
  
  static_assert(utils::is_tuple<typename MT::entity_types>::value,
                "mesh policy entity_types is not a tuple");


public:
  // used to find the entity type of topological dimension D and domain M
  template<size_t D, size_t M = 0>
  using entity_type = typename find_entity_<MT, D, M>::type;

  using id_vector_t = std::array<size_t, MT::num_dimensions>;  
  using id_array_t  = std::vector<std::vector<size_t>>;

  // Don't allow the mesh to be copied or copy constructed
  structured_mesh_topology_t(const structured_mesh_topology_t &) = delete;
  structured_mesh_topology_t & operator=(const structured_mesh_topology_t &)
  = delete;

  // Allow move operations
  structured_mesh_topology_t(structured_mesh_topology_t && o) = default;
  structured_mesh_topology_t & operator=(structured_mesh_topology_t && o) 
  = default;

  //! Constructor
  structured_mesh_topology_t()
  {
      meshdim_ = MT::num_dimensions;  
      //meshbnds_low_.insert(meshbnds_low_.begin(),  MT::lower_bounds.begin(), 
      //MT::lower_bounds.end());
      //meshbnds_up_.insert(meshbnds_up_.begin(),  MT::upper_bounds.begin(), 
      //MT::upper_bounds.end());

      for (size_t i = 0; i <= meshdim_; ++i)
      {
        meshbnds_low_[i] = MT::lower_bounds[i];
        meshbnds_up_[i]  = MT::upper_bounds[i];
      }

      bool primary = false;
      id_array_t vec;
      for (size_t i = 0; i <= meshdim_; ++i)
      {
        if ( i == meshdim_) primary = true;
        vec = get_bnds(meshdim_, i);
        ms_.index_spaces[0][i].init(primary, meshbnds_low_, meshbnds_up_, vec);
      }
  }

  // mesh destructor
  virtual ~structured_mesh_topology_t(){}
 

  /*!
   Return the number of entities contained in specified topological dimension
   and domain.
   */
  size_t
  num_entities(
    size_t dim,
    size_t domain=0
  ) const override
  {
    return num_entities_(dim, domain);
  } // num_entities

  template<
    size_t D,
    size_t M = 0
    >
  decltype(auto)
  num_entities() const
  {
    return ms_.index_spaces[M][D].size();
  } // num_entities
 
 /******************************************************************************
 *                Representation Methods for Cartesian Block                   *
 * ****************************************************************************/ 
  /* Method Description: Returns lower bounds of basic/cartesian block 
  *  IN: 
  *  OUT: 
  */
  auto lower_bounds()
  {
    return meshbnds_low_;
  }

  /* Method Description: Returns upper bounds of basic/cartesian block 
  *  IN: 
  *  OUT: 
  */
  auto upper_bounds()
  {
    return meshbnds_up_;
  }

  /* Method Description: Given an entity id, returns the cartesian box
  *                      the entity belongs. For intermediate, the value
  *                      could be non-zero. 
  *  IN: 
  *  OUT: 
  */
  template<
    size_t D,
    size_t M = 0 
  >
  auto get_box_id(id_t entity_id)
  { 
    return ms_.index_spaces[M][D].find_box_id(entity_id);
  }

  template<
    size_t D,
    size_t M,
    class E
  >
  auto get_box_id(E* e)
  {
    return ms_.index_spaces[M][D].find_box_id(e.id()); 
  }

  /* Method Description: Returns upper bounds of basic/cartesian topology 
  *  IN: 
  *  OUT: 
  */
  template<
    size_t D,
    size_t M = 0
  >
  auto get_indices(id_t entity_id) 
  {
    return ms_.index_spaces[M][D].get_indices_from_offset(entity_id);
  }
  
  template<
    size_t D,
    size_t M,
    class E
  >
  auto get_indices(E* e)
  {
    return ms_.index_spaces[M][D].get_indices_from_offset(e.id());
  }
  
  /* Method Description: Returns upper bounds of basic/cartesian topology 
  *  IN: 
  *  OUT: 
  */
  template<
    size_t D,
    size_t M = 0
  >
  auto get_global_offset(size_t box_id, id_vector_t &idv) 
  {
    return ms_.index_spaces[M][D].template get_global_offset_from_indices<box_id>(idv);
  }

  template<
    size_t D,
    size_t M = 0
  >
  auto get_local_offset(size_t box_id, id_vector_t &idv) 
  {
    return ms_.index_spaces[M][D].template get_local_offset_from_indices<box_id>(idv);
  }
  

 /******************************************************************************
 *                      Query Methods for Cartesian Block                      *
 * ****************************************************************************/ 
  /* Method Description: Provides traversal over the entire index space for a 
  *                      given dimension e.g., cells of the mesh.
  *  IN: 
  *  OUT: 
  */
  template<
    size_t D,
    size_t M = 0
  >
  auto
  entities()
  {
    using etype = entity_type<D,M>;
    return ms_.index_spaces[M][D].template iterate<etype>(); 
  }
 // entities
 
  template<
    size_t D,
    size_t M = 0
  >
  auto
  entities() const
  {
    using etype = entity_type<D,M>;
    return ms_.index_spaces[M][D].template iterate<etype>();
  } // entities


  /* Method Description: Provides FEM-type adjacency queries. Given an entity
  *                      ,find all entities incident on it from dimension TD.
  *                      Supports queries between non-equal dimensions. The queries
  *                      for entities from the same dimension can be obtained through 
  *                      the more finer level queries for intra-index space. 
  *  IN: 
  *  OUT: 
  */

  template<size_t TD, size_t FM , size_t TM = FM,  class E>
  auto entities(E* e)
  {
    size_t FD = E::dimension;
    assert(FD != TD);
    id_t id = e->id(0);
    id_t BD = ms_.index_spaces[FM][FD].template find_box_id(id);
    auto indices = ms_.index_spaces[FM][FD].template get_indices_from_offset(id);
    using etype = entity_type<TD,FM>;
    return ms_.index_spaces[FM][TD].template traverse<TD,etype>(FD,BD,indices);
  } //entities

  /* Method Description: Provides FD-type adjacency queries. Given an entity
  *                      ,find the entity using the stencil provided..
  *  IN: 
  *  OUT: 
  */
  
  template<size_t FM, std::intmax_t xoff, std::intmax_t yoff, class E>
  auto entities(E* e)
  {
  }

  /* Query type 3: FD-type stencil queries between entities
   *               of the same dimension. 
   * D : dimension, N: domain
   * xoff, yoff, zoff: offsets w.r.t current entity
  */
  /*template<size_t D, size_t N, std::intmax_t xoff, typename E>
  auto entities(E* e)
  {
    assert(xoff != 0); 
    size_t value = e->id();
    auto indices = ms_.index_spaces[N][D].template get_indices_from_offset(ent);
    if (ms_.index_spaces[N][D].template check_index_limits<0>(xoff+indices[0]))
      value += xoff;
   // else 
     // value = -1;
    return value;
  } //entities

  template<size_t D, size_t N, std::intmax_t xoff, std::intmax_t yoff>
  auto entities(size_t ent)
  {
    assert(!((xoff == 0) && (yoff == 0))); 
    size_t ind = get_index_in_storage(ent,D,N);
    size_t value = ent;
    size_t nx;

    if (ind == 2)
      ent = ent - ms_.index_spaces[N][1].template size();

    auto indices = ms_.index_spaces[N][ind].template 
                   get_indices_from_offset(ent);
    
    if((ms_.index_spaces[N][ind].template 
       check_index_limits<0>(xoff+indices[0]))&& 
       (ms_.index_spaces[N][ind].template 
       check_index_limits<1>(yoff+indices[1])))
    {
      nx = ms_.index_spaces[N][ind].template get_size_in_direction(0);
      value += xoff + nx*yoff;
    }
    return value; 
   }  //entities

   template<size_t D, size_t N, std::intmax_t xoff, std::intmax_t yoff, 
         std::intmax_t zoff>
   auto entities(size_t ent)
   {
    assert(!((xoff == 0) && (yoff == 0) && (zoff == 0))); 
    size_t ind = get_index_in_storage(ent,D,N);
    size_t value = ent;
    size_t nx, ny;
    
    if (ind == 2) //edge-y
    {
      ent = ent - ms_.index_spaces[N][1].template size();
    }
    else if (ind == 3)//edge-z
    {
      ent = ent - (ms_.index_spaces[N][1].template size()) - 
                  (ms_.index_spaces[N][2].template size());
    }
    else if (ind == 5)//face-y
    {
      ent = ent - ms_.index_spaces[N][4].template size();
    }
    else if (ind == 6)//face-z
    {
      ent = ent - (ms_.index_spaces[N][4].template size()) - 
                  (ms_.index_spaces[N][5].template size());
    }
    else
      ent = ent;

    auto indices = ms_.index_spaces[N][ind].template 
                   get_indices_from_offset(ent);

    if((ms_.index_spaces[N][ind].template 
       check_index_limits<0>(xoff+indices[0]))&& 
       (ms_.index_spaces[N][ind].template 
       check_index_limits<1>(yoff+indices[1]))&& 
       (ms_.index_spaces[N][ind].template 
       check_index_limits<2>(zoff+indices[2])))
     {
         nx = ms_.index_spaces[N][ind].template get_size_in_direction(0);
         ny = ms_.index_spaces[N][ind].template get_size_in_direction(1);
         value += xoff + nx*yoff+nx*ny*zoff;
     }
     //else
     //   value = -1;
    return value;
   } //entities
   */


private:

  size_t meshdim_; 
  id_vector_t meshbnds_low_;
  id_vector_t meshbnds_up_;
  structured_mesh_storage_t<MT::num_dimensions, MT::num_domains> ms_;


  // Get the number of entities in a given domain and topological dimension
  size_t
  num_entities_(
    size_t dim,
    size_t domain=0
  ) const
  {
    return ms_.index_spaces[domain][dim].size();
  } // num_entities_


  //Utility methods
  auto get_bnds(size_t mdim, size_t entdim)
  {
   id_array_t bnds; 
  
   if (mdim == 1){
    switch(entdim){
      case 0:   
         bnds.push_back({1});
         break;
      case 1: 
         bnds.push_back({0});
         break;
      default:
        std::cerr<<"Requesting non-valid entity bounds for dimension 1";
    }
   }
   else if (mdim == 2){
    switch(entdim){
      case 0:   
         bnds.push_back({1,1});
         break;
      case 1:
         bnds.push_back({1,0});
         bnds.push_back({0,1});
         break;
      case 2:
         bnds.push_back({0,0});
         break;
      default:
        std::cerr<<"Requesting non-valid entity bounds for dimension 2";
    }
   }
   else if (mdim == 3){
    switch(entdim){
      case 0:
         bnds.push_back({1,1,1});
         break;
      case 1:
         bnds.push_back({1,0,1});
         bnds.push_back({0,1,1});
         bnds.push_back({1,1,0});
         break;
      case 2:
         bnds.push_back({1,0,0});
         bnds.push_back({0,1,0});
         bnds.push_back({0,0,1});
         break;
      case 3:
         bnds.push_back({0,0,0});
         break;
      default:
        std::cerr<<"Requesting non-valid entity bounds for dimension 3";
     }
   }
   else 
    std::cerr<<"Requesting entity bounds for dimension greater than 3";
      
   return bnds;
  }//get_bnds


}; // class structured_mesh_topology_t

} // namespace topology
} // namespace flecsi

#endif // flecsi_structured_mesh_topology_h

/*~-------------------------------------------------------------------------~-*
 * Formatting options
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/
