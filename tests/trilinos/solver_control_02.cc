// ---------------------------------------------------------------------
//
// Copyright (C) 2004 - 2017 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the deal.II distribution.
//
// ---------------------------------------------------------------------


// test ReductionControl with Trilinos solver
// This test is adapted from tests/trilinos/solver_03.cc


#include "../tests.h"
#include <deal.II/base/utilities.h>
#include "../testmatrix.h"
#include <iostream>
#include <deal.II/lac/trilinos_sparse_matrix.h>
#include <deal.II/lac/trilinos_vector.h>
#include <deal.II/lac/trilinos_solver.h>
#include <deal.II/lac/trilinos_precondition.h>
#include <deal.II/lac/vector_memory.h>
#include <typeinfo>


int main(int argc, char **argv)
{
  std::ofstream logfile("output");
  logfile.precision(4);
  deallog.attach(logfile);

  Utilities::MPI::MPI_InitFinalize mpi_initialization (argc, argv, testing_max_num_threads());


  {
    const unsigned int size = 32;
    unsigned int dim = (size-1)*(size-1);

    deallog << "Size " << size << " Unknowns " << dim << std::endl;

    // Make matrix
    FDMatrix testproblem(size, size);
    DynamicSparsityPattern csp (dim, dim);
    testproblem.five_point_structure(csp);
    TrilinosWrappers::SparseMatrix  A;
    A.reinit(csp);
    testproblem.five_point(A);

    TrilinosWrappers::MPI::Vector f;
    f.reinit(complete_index_set(dim),MPI_COMM_WORLD);
    TrilinosWrappers::MPI::Vector u;
    u.reinit(complete_index_set(dim),MPI_COMM_WORLD);
    f = 1.;
    A.compress (VectorOperation::insert);
    f.compress (VectorOperation::insert);
    u.compress (VectorOperation::insert);

    TrilinosWrappers::PreconditionJacobi preconditioner;
    preconditioner.initialize(A);

    deallog.push("Rel tol");
    {
      // Expects success
      ReductionControl control(2000, 1.e-30, 1e-6);
      TrilinosWrappers::SolverCG solver(control);
      check_solver_within_range(solver.solve(A,u,f,preconditioner),
                                control.last_step(), 49, 51);
    }
    deallog.pop();

    u = 0.;
    deallog.push("Abs tol");
    {
      // Expects success
      ReductionControl control(2000, 1.e-3, 1e-9);
      TrilinosWrappers::SolverCG solver(control);
      check_solver_within_range(solver.solve(A,u,f,preconditioner),
                                control.last_step(), 42, 44);
    }
    deallog.pop();

    u = 0.;
    deallog.push("Iterations");
    {
      // Expects failure
      ReductionControl control(20, 1.e-30, 1e-6);
      TrilinosWrappers::SolverCG solver(control);
      check_solver_within_range(solver.solve(A,u,f,preconditioner),
                                control.last_step(), 0, 19);
    }
    deallog.pop();
  }
}
