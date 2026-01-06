#include "../include/laws/LoiMetabolisme.h"
#include <cassert>
#include <cmath>
#include <iostream>


void test_metabolism_costs() {
  Cellule cell;
  cell.is_alive = true;
  cell.E = 1.0f;
  cell.E_cost = 0.5f;
  cell.D = 0.0f;
  cell.L = 0.0f;
  cell.A = 0;

  ParametresGlobaux params;
  params.K_THERMO = 0.1f;
  params.D_PER_TICK = 0.05f;
  params.L_PER_TICK = 0.05f;

  LoiMetabolisme loi;
  loi.Apply(0, 0, 0, cell, params);

  // Check calculations
  // Total cost = 0.1 + 0.5 = 0.6
  // E = 1.0 - 0.6 = 0.4
  if (std::abs(cell.E - 0.4f) > 0.0001f) {
    std::cerr << "Energy Fail: Expected 0.4, got " << cell.E << std::endl;
    std::exit(1);
  }

  if (std::abs(cell.D - 0.05f) > 0.0001f) {
    std::cerr << "Dette Fail: Expected 0.05, got " << cell.D << std::endl;
    std::exit(1);
  }

  if (cell.A != 1) {
    std::cerr << "Age Fail: Expected 1, got " << cell.A << std::endl;
    std::exit(1);
  }

  if (cell.E_cost != 0.0f) {
    std::cerr << "ECost Reset Fail: Expected 0.0, got " << cell.E_cost
              << std::endl;
    std::exit(1);
  }

  std::cout << "test_metabolism_costs PASSED" << std::endl;
}

int main() {
  test_metabolism_costs();
  return 0;
}
