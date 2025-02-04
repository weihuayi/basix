# Copyright (c) 2022 Matthew Scroggs
# FEniCS Project
# SPDX-License-Identifier: MIT

import pytest

import basix

# from .utils import parametrize_over_elements

cells = [
    basix.CellType.point,
    basix.CellType.interval,
    basix.CellType.triangle,
    basix.CellType.quadrilateral,
    basix.CellType.tetrahedron,
    basix.CellType.hexahedron,
    basix.CellType.prism,
    basix.CellType.pyramid,
]

elements = [
    basix.ElementFamily.P,
    basix.ElementFamily.RT,
    basix.ElementFamily.BDM,
    basix.ElementFamily.N1E,
    basix.ElementFamily.N2E,
    basix.ElementFamily.Regge,
    basix.ElementFamily.HHJ,
    basix.ElementFamily.bubble,
    basix.ElementFamily.serendipity,
    basix.ElementFamily.DPC,
    basix.ElementFamily.CR,
    basix.ElementFamily.Hermite,
    basix.ElementFamily.iso,
    basix.ElementFamily.custom,
]

variants = [
    [basix.LagrangeVariant.gll_isaac],
    [basix.LagrangeVariant.gll_warped],
    [basix.LagrangeVariant.legendre],
    [basix.LagrangeVariant.bernstein],
    [basix.LagrangeVariant.unset, basix.DPCVariant.diagonal_gll],
    [basix.LagrangeVariant.unset, basix.DPCVariant.legendre],
    [basix.LagrangeVariant.legendre, basix.DPCVariant.legendre],
]


def test_all_cells_included():
    all_cells = [getattr(basix.CellType, c) for c in dir(basix.CellType) if c[0].isalpha() and c != "name"]
    assert sorted(all_cells) == sorted(cells)


def test_all_elements_included():
    all_elements = [getattr(basix.ElementFamily, e) for e in dir(basix.ElementFamily)
                    if e[0].isalpha() and e != "name"]
    assert sorted(all_elements) == sorted(elements)


# def test_all_cells_included_in_paramerize():
#     all_cells = set()
#     for c in dir(basix.CellType):
#         if not c.startswith("_") and c not in ["name", "value", "point"]:
#             all_cells.add(getattr(basix.CellType, c))
#     assert all_cells == set(i[0] for i in parametrize_over_elements(4).mark.args[1])


# def test_all_elements_included_in_parametrize():
#     all_elements = set()
#     for c in dir(basix.ElementFamily):
#         if not c.startswith("_") and c not in ["name", "value", "custom"]:
#             all_elements.add(getattr(basix.ElementFamily, c))
#     assert all_elements == set([i[1] for i in parametrize_over_elements(4).mark.args[1]] + [i[1]
#                                for i in parametrize_over_elements(4, discontinuous=True).mark.args[1]])


@pytest.mark.parametrize("cell", cells)
@pytest.mark.parametrize("degree", range(-1, 5))
@pytest.mark.parametrize("family", elements)
@pytest.mark.parametrize("variant", variants)
def test_create_element(cell, degree, family, variant):
    """Check that either the element is created or a RuntimeError is thrown."""
    try:
        element = basix.create_element(family, cell, degree, *variant)
        assert element.degree == degree
    except RuntimeError as e:
        # Don't allow cryptic "dgesv failed" messages
        if len(e.args) == 0 or "dgesv" in e.args[0]:
            raise e

    try:
        element = basix.create_element(family, cell, degree, *variant, discontinuous=True)
        assert element.degree == degree
    except RuntimeError as e:
        if len(e.args) == 0 or "dgesv" in e.args[0]:
            raise e


def test_create_high_degree_lagrange():
    basix.create_element(basix.ElementFamily.P, basix.CellType.hexahedron, 7,
                         basix.LagrangeVariant.gll_isaac)
