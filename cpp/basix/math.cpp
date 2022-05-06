// Copyright (C) 2021 Igor Baratta
//
// This file is part of DOLFINx (https://www.fenicsproject.org)
//
// SPDX-License-Identifier:    LGPL-3.0-or-later

#include "math.h"
#include <vector>

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#else
#include <cblas.h>
extern "C"
{
  void dsyevd_(char* jobz, char* uplo, int* n, double* a, int* lda, double* w,
               double* work, int* lwork, int* iwork, int* liwork, int* info);

  void dgesv_(int* N, int* NRHS, double* A, int* LDA, int* IPIV, double* B,
              int* LDB, int* INFO);
}
#endif

//------------------------------------------------------------------
std::pair<xt::xtensor<double, 1>,
          xt::xtensor<double, 2, xt::layout_type::column_major>>
basix::math::eigh(const xt::xtensor<double, 2>& A)
{
  // Copy to column major matrix
  xt::xtensor<double, 2, xt::layout_type::column_major> M(A.shape());
  M.assign(A);
  int N = A.shape(0);
  xt::xtensor<double, 1> w = xt::zeros<double>({N});

  char jobz = 'V'; // Compute eigenvalues and eigenvectors
  char uplo = 'L'; // Lower
  int ldA = A.shape(1);
  int lwork = -1;
  int liwork = -1;
  int info;
  std::vector<double> work(1);
  std::vector<int> iwork(1);

  // Query optimal workspace size
  dsyevd_(&jobz, &uplo, &N, M.data(), &ldA, w.data(), work.data(), &lwork,
          iwork.data(), &liwork, &info);
  if (info != 0)
    throw std::runtime_error("Could not find workspace size for syevd.");

  // Solve eigen problem
  work.resize(work[0]);
  iwork.resize(iwork[0]);
  lwork = work.size();
  liwork = iwork.size();
  dsyevd_(&jobz, &uplo, &N, M.data(), &ldA, w.data(), work.data(), &lwork,
          iwork.data(), &liwork, &info);
  if (info != 0)
    throw std::runtime_error("Eigenvalue computation did not converge.");

  return {std::move(w), std::move(M)};
}
//------------------------------------------------------------------
xt::xarray<double, xt::layout_type::column_major>
basix::math::solve(const xt::xtensor<double, 2>& A, const xt::xarray<double>& B)
{
  assert(A.dimension() == 2);
  assert(B.dimension() == 1 or B.dimension() == 2);

  // Copy to column major matrix
  xt::xtensor<double, 2, xt::layout_type::column_major> _A(A.shape());
  _A.assign(A);
  xt::xarray<double, xt::layout_type::column_major> _B(B.shape());
  _B.assign(B);

  int N = _A.shape(0);
  int nrhs = _B.dimension() == 1 ? 1 : _B.shape(1);
  int LDA = _A.shape(0);
  int LDB = B.shape(0);
  std::vector<int> IPIV(N);
  int info;
  dgesv_(&N, &nrhs, _A.data(), &LDA, IPIV.data(), _B.data(), &LDB, &info);
  if (info != 0)
    throw std::runtime_error("Call to dgesv failed: " + std::to_string(info));

  return _B;
}
//------------------------------------------------------------------
bool basix::math::is_singular(const xt::xtensor<double, 2>& A)
{
  assert(A.dimension() == 2);

  // Copy to column major matrix
  xt::xtensor<double, 2, xt::layout_type::column_major> _A(A.shape());
  _A.assign(A);
  const std::array<std::size_t, 1> sh = {A.shape(1)};
  xt::xtensor<double, 1> B(sh);
  B.fill(1.0);

  int N = _A.shape(0);
  int nrhs = 1;
  int LDA = _A.shape(0);
  int LDB = B.shape(0);
  std::vector<int> IPIV(N);
  int info;
  dgesv_(&N, &nrhs, _A.data(), &LDA, IPIV.data(), B.data(), &LDB, &info);
  if (info < 0)
    throw std::runtime_error("dgesv failed due to invalid value: "
                             + std::to_string(info));

  if (info > 0)
    return true;
  return false;
}
//------------------------------------------------------------------
void basix::math::dot(const xt::xtensor<double, 2>& A,
                      const xt::xtensor<double, 2>& B,
                      xt::xtensor<double, 2>& C)
{
  int m = A.shape(0);
  int n = B.shape(1);
  int k = A.shape(1);

  assert(A.shape(1) == B.shape(0));
  assert(C.shape(0) == C.shape(0));
  assert(C.shape(1) == B.shape(1));

  if (m * n * k < 4096)
    for (std::size_t i = 0; i < A.shape(0); i++)
      for (std::size_t j = 0; j < B.shape(1); j++)
        for (std::size_t k = 0; k < A.shape(1); k++)
          C(i, j) += A(i, k) * B(k, j);
  else
  {
    double alpha = 1;
    double beta = 0;
    cblas_dgemm(CBLAS_ORDER::CblasRowMajor, CBLAS_TRANSPOSE::CblasNoTrans,
                CBLAS_TRANSPOSE::CblasNoTrans, m, n, k, alpha,
                const_cast<double*>(A.data()), k, const_cast<double*>(B.data()),
                n, beta, const_cast<double*>(C.data()), n);
  }
}

xt::xtensor<double, 2> basix::math::dot(const xt::xtensor<double, 2>& A,
                                        const xt::xtensor<double, 2>& B)
{
  xt::xtensor<double, 2> C = xt::zeros<double>({A.shape(0), B.shape(1)});
  dot(A, B, C);
  return C;
}