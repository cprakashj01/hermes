// This file is part of HermesCommon
//
// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Email: hpfem-group@unr.edu, home page: http://hpfem.org/.
//
// Hermes2D is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
/*! \file epetra_solver.h
    \brief EpetraMatrix and EpetraVector storage classes for Amesos, AztecOO, ... .
*/
#ifndef __HERMES_COMMON_SOLVER_EPETRA_H_
#define __HERMES_COMMON_SOLVER_EPETRA_H_

#include "matrix.h"

#ifdef HAVE_EPETRA
  #include <Epetra_SerialComm.h>
  #include <Epetra_Map.h>
  #include <Epetra_Vector.h>
  #include <Epetra_CrsGraph.h>
  #include <Epetra_CrsMatrix.h>

template <typename Scalar> class AmesosSolver;
template <typename Scalar> class AztecOOSolver;
template <typename Scalar> class NoxSolver;
template <typename Scalar> class IfpackPrecond;
template <typename Scalar> class MlPrecond;

template <typename Scalar>
class HERMES_API EpetraMatrix : public SparseMatrix<Scalar> {
public:
  EpetraMatrix();
  EpetraMatrix(Epetra_RowMatrix &mat);
  virtual ~EpetraMatrix();

  virtual void prealloc(unsigned int n);
  virtual void pre_add_ij(unsigned int row, unsigned int col);
  virtual void finish();

  virtual void alloc();
  virtual void free();
  virtual Scalar get(unsigned int m, unsigned int n);
  virtual int get_num_row_entries(unsigned int row);
  virtual void extract_row_copy(unsigned int row, unsigned int len, unsigned int &n_entries, double *vals, unsigned int *idxs);
  virtual void zero();
  virtual void add(unsigned int m, unsigned int n, Scalar v);
  virtual void add_to_diagonal(Scalar v);
  virtual void add(unsigned int m, unsigned int n, Scalar **mat, int *rows, int *cols);
  virtual bool dump(FILE *file, const char *var_name, EMatrixDumpFormat fmt = DF_MATLAB_SPARSE);
  virtual unsigned int get_matrix_size() const;
  virtual unsigned int get_nnz() const;
  virtual double get_fill_in() const;

protected:
  Epetra_BlockMap *std_map;
  Epetra_CrsGraph *grph;
  Epetra_CrsMatrix *mat;
  Epetra_CrsMatrix *mat_im;		// imaginary part of the matrix, mat holds the real part
  bool owner;

  friend class AmesosSolver<Scalar>;
  friend class AztecOOSolver<Scalar>;
  friend class NoxSolver<Scalar>;
  friend class IfpackPrecond<Scalar>;
  friend class MlPrecond<Scalar>;
};

template <typename Scalar>
class HERMES_API EpetraVector : public Vector<Scalar> {
public:
  EpetraVector();
  EpetraVector(const Epetra_Vector &v);
  virtual ~EpetraVector();

  virtual void alloc(unsigned int ndofs);
  virtual void free();
  virtual Scalar get(unsigned int idx) { return (*vec)[idx]; }
  virtual void extract(Scalar *v) const { vec->ExtractCopy((double *)v); }
  virtual void zero();
  virtual void change_sign();
  virtual void set(unsigned int idx, Scalar y);
  virtual void add(unsigned int idx, Scalar y);
  virtual void add(unsigned int n, unsigned int *idx, Scalar *y);
  virtual void add_vector(Vector<Scalar>* vec) {
    assert(this->length() == vec->length());
    for (unsigned int i = 0; i < this->length(); i++) this->add(i, vec->get(i));
  };
  virtual void add_vector(Scalar* vec) {
    for (unsigned int i = 0; i < this->length(); i++) this->add(i, vec[i]);
  };
  virtual bool dump(FILE *file, const char *var_name, EMatrixDumpFormat fmt = DF_MATLAB_SPARSE);

protected:
  Epetra_BlockMap *std_map;
  Epetra_Vector *vec;
  Epetra_Vector *vec_im;		// imaginary part of the vector, vec holds the real part
  bool owner;

  friend class AmesosSolver<Scalar>;
  friend class AztecOOSolver<Scalar>;
  friend class NoxSolver<Scalar>;
};

#endif
#endif