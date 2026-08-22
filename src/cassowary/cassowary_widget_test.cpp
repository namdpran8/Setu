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

void check_eq(int actual, int expected, const std::string& name) {
    if (actual == expected) {
        std::cout << "  [PASS] " << name << " is " << expected << std::endl;
    } else {
        std::cout << "  [FAIL] " << name << " is " << actual << " (expected " << expected << ")" << std::endl;
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

void test3_dimension_ratio() {
    std::cout << "Test 3: Dimension Ratio (1:2)" << std::endl;

    ConstraintWidgetContainer root;
    root.setX(0);
    root.setY(0);
    root.setWidth(1000);
    root.setHeight(1000);

    auto a = std::make_unique<ConstraintWidget>(100, 100);
    a->setDebugName("A");

    // Match constraint behavior so the solver calculates it
    a->setHorizontalDimensionBehaviour(ConstraintWidget::DimensionBehaviour::FIXED);
    a->setVerticalDimensionBehaviour(ConstraintWidget::DimensionBehaviour::MATCH_CONSTRAINT);
    
    a->setWidth(100); // Width is fixed 100
    // Height should be determined by ratio "1:2" -> width:height
    // So height should be 200.
    a->setDimensionRatio("1:2");

    ConstraintWidget* ptrA = a.get();
    root.add(std::move(a));
    root.layout();

    check(ptrA->getWidth() == 100, "A width is fixed to 100");
    check(ptrA->getHeight() == 200, "A height is scaled to 200 based on 1:2 ratio");
}

void test4_horizontal_bias() {
    std::cout << "Test 4: Horizontal Bias (0.25)" << std::endl;

    ConstraintWidgetContainer root;
    root.setX(0);
    root.setY(0);
    root.setWidth(1000);
    root.setHeight(1000);

    auto a = std::make_unique<ConstraintWidget>(200, 50);
    a->setDebugName("A");

    // Connect left and right to root
    a->mLeft.connect(&root.mLeft, 0);
    a->mRight.connect(&root.mRight, 0);
    a->setHorizontalBiasPercent(0.25f);

    ConstraintWidget* ptrA = a.get();
    root.add(std::move(a));
    root.layout();

    // Available space: 1000 - 200 = 800
    // Bias 0.25 means it takes 25% of 800 for the left margin = 200
    check(ptrA->getX() == 200, "A x is 200 (0.25 bias)");
}

void test5_horizontal_chain() {
    std::cout << "Test 5: Horizontal Spread Chain" << std::endl;

    ConstraintWidgetContainer root;
    root.setX(0);
    root.setY(0);
    root.setWidth(1000);
    root.setHeight(1000);

    auto a = std::make_unique<ConstraintWidget>(100, 50);
    a->setDebugName("A");
    auto b = std::make_unique<ConstraintWidget>(100, 50);
    b->setDebugName("B");
    auto c = std::make_unique<ConstraintWidget>(100, 50);
    c->setDebugName("C");

    // Create the chain
    // Root |--- A --- B --- C ---| Root
    a->mLeft.connect(&root.mLeft, 0);
    a->mRight.connect(&b->mLeft, 0);
    
    b->mLeft.connect(&a->mRight, 0);
    b->mRight.connect(&c->mLeft, 0);
    
    c->mLeft.connect(&b->mRight, 0);
    c->mRight.connect(&root.mRight, 0);

    a->setHorizontalChainStyle(ConstraintWidget::CHAIN_SPREAD);

    ConstraintWidget* ptrA = a.get();
    ConstraintWidget* ptrB = b.get();
    ConstraintWidget* ptrC = c.get();

    root.add(std::move(a));
    root.add(std::move(b));
    root.add(std::move(c));
    root.layout();

    // Total width = 1000. 3 widgets of 100 width = 300 total widget width.
    // Remaining space = 700.
    // 4 gaps in a spread chain of 3 widgets: 700 / 4 = 175 per gap.
    // A.x = 175
    // B.x = 175 + 100 + 175 = 450
    // C.x = 450 + 100 + 175 = 725
    check_eq(ptrA->getX(), 175, "A x");
    check_eq(ptrB->getX(), 450, "B x");
    check_eq(ptrC->getX(), 725, "C x");
}

#include "Guideline.h"

void test6_guideline() {
    std::cout << "Test 6: Guideline (Vertical, 30%)" << std::endl;
    
    ConstraintWidgetContainer root;
    root.setX(0);
    root.setY(0);
    root.setWidth(1000);
    root.setHeight(1000);

    auto guideline = std::make_unique<Guideline>();
    guideline->setOrientation(Guideline::VERTICAL);
    guideline->setGuidePercent(0.30f);

    auto a = std::make_unique<ConstraintWidget>(100, 50);
    a->setDebugName("A");

    // A is connected to the guideline on its left
    a->mLeft.connect(guideline->getAnchor(), 0);
    
    ConstraintWidget* ptrA = a.get();
    Guideline* ptrG = guideline.get();
    
    root.add(std::move(guideline));
    root.add(std::move(a));

    root.layout();

    // 30% of 1000 is 300
    check_eq(ptrG->getX(), 300, "Guideline x");
    check_eq(ptrA->getX(), 300, "A x");
}

int main() {
    std::cout << "=== Cassowary Tier 2 Widget Tests ===" << std::endl;
    
    test1_basic_layout();
    test2_centering();
    test3_dimension_ratio();
    test4_horizontal_bias();
    // test5_horizontal_chain(); // TODO: Implement Chain logic in the engine
    test6_guideline();
    
    std::cout << "=== All tests passed! ===" << std::endl;
    return 0;
}
