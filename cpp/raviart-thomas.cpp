// Copyright (c) 2020 Chris Richardson
// FEniCS Project
// SPDX-License-Identifier:    MIT

#include "raviart-thomas.h"
#include "polynomial-set.h"
#include "polynomial.h"
#include "quadrature.h"
#include <Eigen/Dense>
#include <numeric>
#include <vector>

RaviartThomas::RaviartThomas(Cell::Type celltype, int k)
    : FiniteElement(celltype, k - 1)
{
  if (celltype != Cell::Type::triangle and celltype != Cell::Type::tetrahedron)
    throw std::runtime_error("Unsupported cell type");

  const int tdim = Cell::topological_dimension(celltype);
  Cell simplex_cell(celltype);
  Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> simplex
      = simplex_cell.geometry();

  // Create orthonormal basis on simplex
  std::vector<Polynomial> Pkp1
      = PolynomialSet::compute_polynomial_set(celltype, _degree + 1);
  int psize = Pkp1.size();

  // Vector subsets
  int nv;
  int ns0;
  int ns;
  if (tdim == 2)
  {
    nv = (_degree + 1) * (_degree + 2) / 2;
    ns0 = _degree * (_degree + 1) / 2;
    ns = (_degree + 1);
  }
  else
  {
    assert(tdim == 3);
    nv = (_degree + 1) * (_degree + 2) * (_degree + 3) / 6;
    ns0 = _degree * (_degree + 1) * (_degree + 2) / 6;
    ns = (_degree + 1) * (_degree + 2) / 2;
  }

  auto [Qpts, Qwts] = make_quadrature(tdim, 2 * _degree + 2);
  Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      Pkp1_at_Qpts(psize, Qpts.rows());
  for (int j = 0; j < psize; ++j)
    Pkp1_at_Qpts.row(j) = Pkp1[j].tabulate(Qpts);

  // Create initial coefficients of Pkp1.
  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      wcoeffs(nv * tdim + ns, psize * tdim);
  wcoeffs.setZero();
  for (int j = 0; j < tdim; ++j)
    wcoeffs.block(nv * j, psize * j, nv, nv)
        = Eigen::MatrixXd::Identity(nv, nv);

  for (int i = 0; i < ns; ++i)
    for (int k = 0; k < psize; ++k)
      for (int j = 0; j < tdim; ++j)
      {
        auto w = Qwts * Pkp1_at_Qpts.row(ns0 + i).transpose() * Qpts.col(j)
                 * Pkp1_at_Qpts.row(k).transpose();
        wcoeffs(nv * tdim + i, k + psize * j) = w.sum();
      }

  // Dual space

  Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
      dualmat(nv * tdim + ns, psize * tdim);
  dualmat.setZero();

  // dof counter
  int c = 0;

  // Create a polynomial set on a reference facet
  Cell::Type facettype
      = (tdim == 2) ? Cell::Type::interval : Cell::Type::triangle;
  std::vector<Polynomial> Pq
      = PolynomialSet::compute_polynomial_set(facettype, _degree);
  // Create quadrature scheme on the facet
  int quad_deg = 5 * (_degree + 1);
  auto [QptsE, QwtsE] = make_quadrature(tdim - 1, quad_deg);

  for (int i = 0; i < (tdim + 1); ++i)
  {
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> facet
        = simplex_cell.sub_entity_geometry(tdim - 1, i);

    // FIXME: get normal from the cell class
    Eigen::VectorXd normal;
    if (tdim == 2)
    {
      normal.resize(2);
      normal << facet(1, 1) - facet(0, 1), facet(0, 0) - facet(1, 0);
      if (i == 1)
        normal *= -1;
    }
    else if (tdim == 3)
    {
      Eigen::Vector3d e0 = facet.row(1) - facet.row(0);
      Eigen::Vector3d e1 = facet.row(2) - facet.row(0);
      normal = e1.cross(e0);
    }

    // Map reference facet to cell
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
        QptsE_scaled(QptsE.rows(), tdim);
    for (int j = 0; j < QptsE.rows(); ++j)
    {
      QptsE_scaled.row(j) = facet.row(0);
      for (int k = 0; k < (tdim - 1); ++k)
        QptsE_scaled.row(j) += QptsE(j, k) * (facet.row(k + 1) - facet.row(0));
    }

    // Tabulate Pkp1 at facet quadrature points
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
        Pkp1_at_QptsE(psize, QptsE_scaled.rows());
    for (int j = 0; j < psize; ++j)
      Pkp1_at_QptsE.row(j) = Pkp1[j].tabulate(QptsE_scaled);

    // Compute facet normal integral moments by quadrature
    for (std::size_t j = 0; j < Pq.size(); ++j)
    {
      Eigen::ArrayXd phi = Pq[j].tabulate(QptsE);
      for (int k = 0; k < tdim; ++k)
      {
        Eigen::VectorXd q = phi * QwtsE * normal[k];
        Eigen::RowVectorXd qcoeffs = Pkp1_at_QptsE.matrix() * q;
        dualmat.block(c, psize * k, 1, psize) = qcoeffs;
      }
      ++c;
    }
  }

  // Should work for 2D and 3D
  if (_degree > 0)
  {
    // Interior integral moment
    std::vector<Polynomial> Pkm1
        = PolynomialSet::compute_polynomial_set(celltype, _degree - 1);
    for (std::size_t i = 0; i < Pkm1.size(); ++i)
    {
      Eigen::ArrayXd phi = Pkm1[i].tabulate(Qpts);
      Eigen::VectorXd q = phi * Qwts;
      Eigen::RowVectorXd qcoeffs = Pkp1_at_Qpts.matrix() * q;
      assert(qcoeffs.size() == psize);
      for (int j = 0; j < tdim; ++j)
      {
        dualmat.block(c, psize * j, 1, psize) = qcoeffs;
        ++c;
      }
    }
  }

  apply_dualmat_to_basis(wcoeffs, dualmat, Pkp1, tdim);
}
//-----------------------------------------------------------------------------
