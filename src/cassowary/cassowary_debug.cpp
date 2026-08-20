#include "LinearSystem.h"
#include "SolverVariable.h"
#include <iostream>
#include <fstream>

using namespace setu::cassowary;

int main() {
    std::cout << "Starting Test 8" << std::endl;
    LinearSystem system;
    SolverVariable* x = system.getVariable("x", SolverVariable::Type::UNRESTRICTED);
    SolverVariable* y = system.getVariable("y", SolverVariable::Type::UNRESTRICTED);
    system.addEquality(y, 10);
    std::cout << "Added y=10" << std::endl;
    system.addEquality(x, 20);
    std::cout << "Added x=20" << std::endl;
    system.addGreaterThan(x, y, 5, SolverVariable::STRENGTH_HIGH);
    std::cout << "Added x >= y + 5" << std::endl;
    
    std::cout << "Minimizing..." << std::endl;
    system.minimize();
    std::cout << "Done minimizing." << std::endl;
    return 0;
}
