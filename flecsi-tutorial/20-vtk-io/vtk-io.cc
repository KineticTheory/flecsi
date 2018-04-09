/*
    @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
   /@@/////  /@@          @@////@@ @@////// /@@
   /@@       /@@  @@@@@  @@    // /@@       /@@
   /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
   /@@////   /@@/@@@@@@@/@@       ////////@@/@@
   /@@       /@@/@@//// //@@    @@       /@@/@@
   /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
   //       ///  //////   //////  ////////  //

   Copyright (c) 2016, Los Alamos National Security, LLC
   All rights reserved.
                                                                              */

#include <iostream>

#include<flecsi-tutorial/specialization/mesh/mesh.h>
#include<flecsi/data/data.h>
#include<flecsi/execution/execution.h>
#include<flecsi-tutorial/specialization/io/vtk/unstructuredGrid.h>
#include <vtkQuad.h>

using namespace flecsi;
using namespace flecsi::tutorial;

flecsi_register_field(mesh_t, example, field, double, dense, 1, cells);

namespace example {

void initialize_field(mesh<ro> mesh, field<rw> f) {
  for(auto c: mesh.cells(owned)) {
    f(c) = double(c->id());
  } // for
} // initialize_field

flecsi_register_task(initialize_field, example, loc, single);

void print_field(mesh<ro> mesh, field<ro> f) {
  for(auto c: mesh.cells(owned)) {
    std::cout << "cell id: " << c->id() << " has value " <<
      f(c) << std::endl;
  } // for
} // print_field

flecsi_register_task(print_field, example, loc, single);



void output_field(mesh<ro> mesh, field<ro> f) 
{
  vtkOutput::UnstructuredGrid temp;
  double *cellData = new double[256];
  double *cellID = new double[256];

  int count = 0;
  int vertexCount = 0;


  vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
  for(auto c: mesh.cells(owned)) {

    double * pointCoords = new double[8];
    

    vtkCell *cell;
    vtkSmartPointer<vtkPoints> pnts;

    vtkIdType pointIds[4];

    for(auto v: mesh.vertices(c)) 
    {
      int localVertexCount = 0;

      auto p = v->coordinates();
      double pt[3] = {0, 0, 0};

      pt[0] = std::get<0>(p);
      pt[1] = std::get<1>(p);
      std::cout << pt[0] << ", " << pt[0] << std::endl;

      temp.addPoint(pt);

      pointIds[localVertexCount] = vertexCount;
      vertexCount++;  
      localVertexCount++;
    }
    temp.uGrid->InsertNextCell( VTK_QUAD, 4, pointIds);
    std::cout << std::endl << std::endl;

    // // Line
    // double pnt[3];
    // pnt[0]=c->id(); pnt[1]=0; pnt[2]=0;
    // temp.addPoint(pnt);
    
    cellID[count] = c->id();
    cellData[count] = f(c);

    count++;
  } // for
  
  // // Line
  // temp.pushPointsToGrid(VTK_VERTEX);

  temp.pushPointsToGrid(VTK_QUAD);

  temp.addScalarData("cell-id", 256, cellID);
  temp.addScalarData("cell-data-scalar", 256, cellData);
  temp.write("testVTK");

  if (cellData != NULL)
   delete []cellData;
  cellData = NULL;
} // print_field


flecsi_register_task(output_field, example, loc, single);

} // namespace example

namespace flecsi {
namespace execution {

void driver(int argc, char ** argv) {

  auto m = flecsi_get_client_handle(mesh_t, clients, mesh);
  auto f = flecsi_get_handle(m, example, field, double, dense, 0);

  flecsi_execute_task(initialize_field, example, single, m, f);
  flecsi_execute_task(print_field, example, single, m, f);
  flecsi_execute_task(output_field, example, single, m, f);
} // driver

} // namespace execution
} // namespace flecsi
