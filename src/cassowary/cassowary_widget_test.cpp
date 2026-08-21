#include "ConstraintWidgetContainer.h"
#include "ConstraintWidget.h"
#include "ConstraintAnchor.h"
#include "LinearSystem.h"
#include <iostream>
#include <memory>
#include <cmath>

using namespace setu::cassowary;

void check(bool condition, const std::string& message) {
    if (condition) {
        std::cout << "  [PASS] " << message << std::endl;
    } else {
        std::cout << "  [FAIL] " << message << std::endl;
        exit(1);
    }
}

// Helper to assert floats
bool approxEqual(float a, float b, float epsilon = 0.001f) {
    return std::abs(a - b) < epsilon;
}

void test1_basic_layout() {
    std::cout << "Test 1: Basic two-widget layout" << std::endl;
    
    ConstraintWidgetContainer root;
    root.setX(0);
    root.setY(0);
    root.setWidth(1000);
    root.setHeight(1000);
    
    auto a = std::make_unique<ConstraintWidget>(100, 50);
    a->setDebugName("A");
    
    auto b = std::make_unique<ConstraintWidget>(200, 50);
    b->setDebugName("B");
    
    // A is connected to left of root
    a->mLeft.connect(&root.mLeft, 0);
    
    // B is connected to right of A
    b->mLeft.connect(&a->mRight, 0);
    
    ConstraintWidget* ptrA = a.get();
    ConstraintWidget* ptrB = b.get();
    
    root.add(std::move(a));
    root.add(std::move(b));
    
    root.layout(); // This will run the solver
    
    check(ptrA->getX() == 0, "A x is 0");
    check(ptrA->getWidth() == 100, "A width is 100");
    check(ptrB->getX() == 100, "B x is 100 (after A)");
    check(ptrB->getWidth() == 200, "B width is 200");
}

void test2_centering() {
    std::cout << "Test 2: Centering a widget" << std::endl;
    
    ConstraintWidgetContainer root;
    root.setX(0);
    root.setY(0);
    root.setWidth(1000);
    root.setHeight(1000);
    
    auto a = std::make_unique<ConstraintWidget>(100, 50);
    a->setDebugName("A");
    
    // A is centered in root
    a->mLeft.connect(&root.mLeft, 0);
    a->mRight.connect(&root.mRight, 0);
    
    ConstraintWidget* ptrA = a.get();
    root.add(std::move(a));
    
    root.layout(); // This will run the solver
    
    // Root is 1000 wide, A is 100 wide. Centered x should be (1000 - 100) / 2 = 450
    check(ptrA->getX() == 450, "A x is 450 (centered)");
}

int main() {
    std::cout << "=== Cassowary Tier 2 Widget Tests ===" << std::endl;
    
    test1_basic_layout();
    test2_centering();
    
    std::cout << "=== All tests passed! ===" << std::endl;
    return 0;
}
