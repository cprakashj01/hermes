#define HERMES_REPORT_WARN
#define HERMES_REPORT_INFO
#define HERMES_REPORT_VERBOSE

//#include <getopt.h>
#include "common.h"
#include "config.h"

#include "solver.h"
#include "umfpack_solver.h"
#include "superlu_solver.h"
#include "petsc_solver.h"
#include "epetra.h"
#include "amesos.h"
#include "aztecoo.h"
#include "mumps_solver.h"

#include <iostream>

// Test of linear solvers.
// Read matrix and RHS from a file.

// Max row length in input file.
#define MAX_ROW_LEN	1024

template<typename Scalar>
class MatrixEntry 
{
public:
  MatrixEntry() { }
  MatrixEntry(int m, int n, Scalar value) {
    this->m = m;
    this->n = n;
    this->value = value;
  }

  int m, n;
  Scalar value;
};

template<typename Scalar>
void show_mat(const char *msg, std::map<unsigned int, MatrixEntry<Scalar> > mp) {
  std::map<unsigned int, MatrixEntry<Scalar> >::iterator itr;

  std::cout << msg << std::endl;

  for(itr=mp.begin(); itr != mp.end(); ++itr)
    std::cout << " " << (int) itr->first << ": " << 
    (int) itr->second.m << " " << 
    (int) itr->second.n << " " << 
    (Scalar) itr->second.value << 
    std::endl;

  std::cout << std::endl;
}

template<typename Scalar>
void show_rhs(const char *msg, std::map<unsigned int,Scalar> mp) {
  std::map<unsigned int, Scalar>::iterator itr;

  std::cout << msg << std::endl;

  for(itr=mp.begin(); itr != mp.end(); ++itr)
    std::cout << " " << (int) itr->first << ": " << (Scalar) itr->second << std::endl;

  std::cout << std::endl;
}

bool testPrint(bool value, const char *msg, bool correct) {
  info("%s...", msg);
  if (value == correct) {
    info("OK.");
    return true;
  }
  else {
    info("failed.");
    return false;
  }
}

bool read_n_nums(char *row, int n, double values[]) {
  int i = 0;
  char delims[] = " \t\n\r";
  char *token = strtok(row, delims);
  while (token != NULL && i < n) {
    double entry_buffer;
    sscanf(token, "%lf", &entry_buffer);
    values[i++] = entry_buffer;

    token = strtok(NULL, delims);
  }

  return (i == n);
}

int read_matrix_and_rhs(char *file_name, int &n, int &nnz,
  std::map<unsigned int, MatrixEntry<double> > &mat, std::map<unsigned int, double> &rhs, bool &cplx_2_real) 
{
  FILE *file = fopen(file_name, "r");
  if (file == NULL) return TEST_FAILURE;

  enum EState {
    STATE_N,
    STATE_MATRIX,
    STATE_RHS,
    STATE_NNZ
  } 
  state = STATE_N;

  // Variables needed to turn complex matrix into real.
  int k = 0; 
  int l = 0;
  double* rhs_buffer = NULL;

  double buffer[4]; 
  char row[MAX_ROW_LEN];
  while (fgets(row, MAX_ROW_LEN, file) != NULL) {
    switch (state) {
    case STATE_N:
      if (read_n_nums(row, 1, buffer)) {
        if (cplx_2_real) {
          n = 2*((int) buffer[0]);
          rhs_buffer = new double[n];
          for (int i = 0; i < n; i++) {
            rhs_buffer[i] = 0.0;              
          }
        }   
        else
          n = (int) buffer[0];

        state = STATE_NNZ;
      } 
      break;

    case STATE_NNZ:
      if (read_n_nums(row, 1, buffer))
        nnz = (int) buffer[0];

      state = STATE_MATRIX; 
      break;

    case STATE_MATRIX:
      if (cplx_2_real) {
        if (read_n_nums(row, 4, buffer)) {
          mat.insert(std::pair<unsigned int, MatrixEntry<double> >(k,   MatrixEntry<double> ((int) buffer[0],     (int) buffer[1],     buffer[2])));
          mat.insert(std::pair<unsigned int, MatrixEntry<double> >(k+1, MatrixEntry<double> ((int) buffer[0] + n/2, (int) buffer[1],     buffer[3])));
          mat.insert(std::pair<unsigned int, MatrixEntry<double> >(k+2*nnz, MatrixEntry<double> ((int) buffer[0],     (int) buffer[1] + n/2, (-1)*buffer[3])));
          mat.insert(std::pair<unsigned int, MatrixEntry<double> >(k+2*nnz+1, MatrixEntry<double> ((int) buffer[0] + n/2, (int) buffer[1] + n/2, buffer[2])));
          k=k+2;
        }
        else        
          state = STATE_RHS;
      }
      else { // if cplx_2_real is false.
        if (read_n_nums(row, 3, buffer)) 
          mat[mat.size()] = (MatrixEntry<double> ((int) buffer[0], (int) buffer[1], buffer[2]));

        else        
          state = STATE_RHS;
      }
      break; //case STATE_MATRIX break.

    case STATE_RHS:
      if (cplx_2_real) {
        if (read_n_nums(row, 3, buffer)) {

          if (buffer[0] != (int) n/2-1) // Then this is not the last line in the input file
          {
            rhs[((int) buffer[0])] = (double) buffer[1];
            rhs_buffer[l] = (double) buffer[2];
            l=l+1;
          }

          else // This is last line in the file.
          {
            // First read last line entry
            rhs[((int) buffer[0])] = (double) buffer[1];
            rhs_buffer[l] = (double) buffer[2];
            l=l+1;
            // Take imaginary parts you saved, 
            // and fill the rest of the rhs vector.
            for (int i=0; i < l; i++) 
            {
              rhs[rhs.size()] = rhs_buffer[i];                  
            }
          }
        }
      }
      else { // if cplx_2_real is false.
        if (read_n_nums(row, 2, buffer)) 
          rhs[(int) buffer[0]] = (double) buffer[1];
      }                 
      break;
    }
  }

  fclose(file);

  // Free memory
  delete [] rhs_buffer;

  // Clear pointer.
  rhs_buffer = NULL;

  return TEST_SUCCESS;
}

int read_matrix_and_rhs(char *file_name, int &n, int &nnz,
  std::map<unsigned int, MatrixEntry<std::complex<double> > > &mat, std::map<unsigned int, std::complex<double> > &rhs, bool &cplx_2_real) 
{
  FILE *file = fopen(file_name, "r");
  if (file == NULL) return TEST_FAILURE;

  enum EState {
    STATE_N,
    STATE_MATRIX,
    STATE_RHS,
    STATE_NNZ
  } 
  state = STATE_N;

  // Variables needed to turn complex matrix into real.
  int k = 0; 
  int l = 0;
  double* rhs_buffer = NULL;

  double buffer[4]; 
  char row[MAX_ROW_LEN];
  while (fgets(row, MAX_ROW_LEN, file) != NULL) {
    switch (state) {
    case STATE_N:
      if (read_n_nums(row, 1, buffer)) {
        if (cplx_2_real) {
          n = 2*((int) buffer[0]);
          rhs_buffer = new double[n];
          for (int i = 0; i < n; i++) {
            rhs_buffer[i] = 0.0;              
          }
        }   
        else
          n = (int) buffer[0];

        state = STATE_NNZ;
      } 
      break;

    case STATE_NNZ:
      if (read_n_nums(row, 1, buffer))
        nnz = (int) buffer[0];

      state = STATE_MATRIX; 
      break;

    case STATE_MATRIX:
      if (read_n_nums(row, 4, buffer)) {
        std::complex<double> cmplx_buffer(buffer[2], buffer[3]);
        mat[mat.size()] = (MatrixEntry<std::complex<double> >((int) buffer[0], (int) buffer[1], (std::complex<double>) cmplx_buffer));
      }
      else
        state = STATE_RHS;
      break;

    case STATE_RHS:
      if (read_n_nums(row, 3, buffer)) {
        std::complex<double> cmplx_buffer(buffer[1], buffer[2]);
        rhs[(int) buffer[0]] = (std::complex<double>) cmplx_buffer;
      }
      break;
    }
  }

  fclose(file);

  // Free memory
  delete [] rhs_buffer;

  // Clear pointer.
  rhs_buffer = NULL;

  return TEST_SUCCESS;
}

template<typename Scalar>
void build_matrix(int n, std::map<unsigned int, MatrixEntry<Scalar>> &ar_mat, 
  std::map<unsigned int, Scalar> &ar_rhs, SparseMatrix<Scalar> *mat,
  Vector<Scalar> *rhs) {
    mat->prealloc(n);
    for (std::map<unsigned int, MatrixEntry<Scalar> >::iterator it = ar_mat.begin(); it != ar_mat.end(); it++) {
      MatrixEntry<Scalar> &me = it->second;
      mat->pre_add_ij(me.m, me.n);
    }

    mat->alloc();
    for (std::map<unsigned int, MatrixEntry<Scalar> >::iterator it = ar_mat.begin(); it != ar_mat.end(); it++) {
      MatrixEntry<Scalar> &me = it->second;
      mat->add(me.m, me.n, me.value);
    }
    mat->finish();

    rhs->alloc(n);
    for (std::map<unsigned int, Scalar>::iterator it = ar_rhs.begin(); it != ar_rhs.end(); it++) {
      rhs->add(it->first, it->second);
    }
    rhs->finish();
}

template<typename Scalar>
void build_matrix_block(int n, std::map<unsigned int, MatrixEntry<Scalar> > &ar_mat, std::map<unsigned int, Scalar> &ar_rhs,
  SparseMatrix<Scalar> *matrix, Vector<Scalar> *rhs) {
    matrix->prealloc(n);
    for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++)
        matrix->pre_add_ij(i, j);

    matrix->alloc();
    Scalar **mat = new_matrix<Scalar>(n, n);
    int *cols = new int[n];
    int *rows = new int[n];
    for (int i = 0; i < n; i++) {
      cols[i] = i;
      rows[i] = i;
    }
    for (std::map<unsigned int, MatrixEntry<Scalar> >::iterator it = ar_mat.begin(); it != ar_mat.end(); it++) {
      MatrixEntry<Scalar> &me = it->second;
      mat[me.m][me.n] = me.value;
    }
    matrix->add(n, n, mat, rows, cols);
    matrix->finish();

    rhs->alloc(n);
    Scalar *rs = new Scalar[n];
    for (std::map<unsigned int, Scalar>::iterator it = ar_rhs.begin(); it != ar_rhs.end(); it++) {
      rs[it->first] = it->second;
    }
    unsigned int *u_rows = new unsigned int[n];
    for (int i = 0; i < n; i++)
      u_rows[i] = rows[i] >= 0 ? rows[i] : 0;
    rhs->add(n, u_rows, rs);
    rhs->finish();
}

// Test code.
void solve(Solver<std::complex<double> > &solver, int n) {
  if (solver.solve()) {
    std::complex<double> *sln = solver.get_solution();
    for (int i = 0; i < n; i++)
      if(sln[i].imag() < 0.0)
        std::cout << std::endl << sln[i].real() << sln[i].imag();
      else
        std::cout << std::endl << sln[i].real() << '+' << sln[i].imag();
  }
  else
    printf("Unable to solve.\n");
}

void solve(Solver<double> &solver, int n) {
  if (solver.solve()) {
    double *sln = solver.get_solution();
    for (int i = 0; i < n; i++)
      std::cout << std::endl << sln[i];
  }
  else
    printf("Unable to solve.\n");
}

int main(int argc, char *argv[]) {
  int ret = TEST_SUCCESS;

  if (argc < 3) error("Not enough parameters.");

  int n;
  int nnz;
  bool cplx_2_real;

  std::map<unsigned int, MatrixEntry<double> > ar_mat_double;
  std::map<unsigned int, MatrixEntry<std::complex<double> > > ar_mat_cplx;
  std::map<unsigned int, double > ar_rhs_double;
  std::map<unsigned int, std::complex<double> > ar_rhs_cplx;

  if (argc == 4 && strcasecmp(argv[3],"complex-matrix-to-real") == 0)
    cplx_2_real = true;
  else
    cplx_2_real = false;

  if (read_matrix_and_rhs(argv[2], n, nnz, ar_mat_double, ar_rhs_double, cplx_2_real) != TEST_SUCCESS)
    error("Failed to read the matrix and rhs.");

  if (strcasecmp(argv[1], "petsc") == 0) {
#ifdef WITH_PETSC
    PetscMatrix<Scalar> mat;
    PetscVector<Scalar> rhs;
    build_matrix(n, ar_mat, ar_rhs, &mat, &rhs);

    PetscLinearSolver solver(&mat, &rhs);
    solve(solver, n);
#endif
  }
  else if (strcasecmp(argv[1], "petsc-block") == 0) {
#ifdef WITH_PETSC
    PetscMatrix<Scalar> mat;
    PetscVector<Scalar> rhs;
    build_matrix_block(n, ar_mat, ar_rhs, &mat, &rhs);

    PetscLinearSolver solver(&mat, &rhs);
    solve(solver, n);
#endif
  }
  else if (strcasecmp(argv[1], "umfpack") == 0) {
#ifdef WITH_UMFPACK
    UMFPackMatrix<Scalar> mat;
    UMFPackVector<Scalar> rhs;
    build_matrix(n, ar_mat, ar_rhs, &mat, &rhs);

    UMFPackLinearSolver solver(&mat, &rhs);
    solve(solver, n);
#endif
  }
  else if (strcasecmp(argv[1], "umfpack-block") == 0) {
#ifdef WITH_UMFPACK
    UMFPackMatrix<Scalar> mat;
    UMFPackVector<Scalar> rhs;
    build_matrix_block(n, ar_mat, ar_rhs, &mat, &rhs);

    UMFPackLinearSolver solver(&mat, &rhs);
    solve(solver, n);
#endif
  }
  else if (strcasecmp(argv[1], "aztecoo") == 0) {
#ifdef WITH_TRILINOS
    EpetraMatrix<Scalar> mat;
    EpetraVector<Scalar> rhs;
    build_matrix(n, ar_mat, ar_rhs, &mat, &rhs);

    AztecOOSolver solver(&mat, &rhs);
    solve(solver, n);
#endif
  }
  else if (strcasecmp(argv[1], "aztecoo-block") == 0) {
#ifdef WITH_TRILINOS
    EpetraMatrix<Scalar> mat;
    EpetraVector<Scalar> rhs;
    build_matrix_block(n, ar_mat, ar_rhs, &mat, &rhs);

    AztecOOSolver solver(&mat, &rhs);
    solve(solver, n);
#endif
  }
  else if (strcasecmp(argv[1], "amesos") == 0) {
#ifdef WITH_TRILINOS
    EpetraMatrix<Scalar> mat;
    EpetraVector<Scalar> rhs;
    build_matrix(n, ar_mat, ar_rhs, &mat, &rhs);

    if (AmesosSolver::is_available("Klu")) {
      AmesosSolver solver("Klu", &mat, &rhs);
      solve(solver, n);
    }
#endif
  }
  else if (strcasecmp(argv[1], "amesos-block") == 0) {
#ifdef WITH_TRILINOS
    EpetraMatrix<Scalar> mat;
    EpetraVector<Scalar> rhs;
    build_matrix_block(n, ar_mat, ar_rhs, &mat, &rhs);

    if (AmesosSolver::is_available("Klu")) {
      AmesosSolver solver("Klu", &mat, &rhs);
      solve(solver, n);
    } 
#endif
  }
  else if (strcasecmp(argv[1], "mumps") == 0) {
#ifdef WITH_MUMPS
    MumpsMatrix<Scalar> mat;
    MumpsVector<Scalar> rhs;
    build_matrix(n, ar_mat, ar_rhs, &mat, &rhs);

    MumpsSolver solver(&mat, &rhs);
    solve(solver, n);
#endif
  }
  else if (strcasecmp(argv[1], "mumps-block") == 0) {
#ifdef WITH_MUMPS
    MumpsMatrix<Scalar> mat;
    MumpsVector<Scalar> rhs;
    build_matrix_block(n, ar_mat, ar_rhs, &mat, &rhs);

    MumpsSolver solver(&mat, &rhs);
    solve(solver, n);
#endif
  }  
  else
    ret = ERR_FAILURE;

  return ret;
}
