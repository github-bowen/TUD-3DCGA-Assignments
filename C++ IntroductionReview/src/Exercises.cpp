#include <cmath>
#include <iostream>
#include <list>
#include <vector>

struct Tree {
    float value;
    std::list<Tree> children;
};
#include "Exercises.h"

////////////////////////////// Exercise 2
float TreeTraversal(const Tree& t, TreeVisitor& visitor, bool countOnlyEvenLevels)
{
    return visitor.visitTree(t, countOnlyEvenLevels);
}

///////////////////////////// Exercise 3

Complex ComplexOperation(const Complex& c1, const Complex& c2)
{
    //return Complex(0, 0);
    return (c1 - c2) * (c1 + c2);
}

void fillTree(Tree& tree, int level)
{
    tree.value = 2.0f * static_cast<float>(level);
    if (level >= 4)
        return;

    Tree child;
    fillTree(child, level + 1);

    for (int i = 0; i < level + 2; ++i) {
        tree.children.push_back(child);
    }
}

static constexpr float EPSILON = 0.01f;
bool equalFloats(float f1, float f2)
{
    return std::abs(f1 - f2) < EPSILON;
}

/////////////////////////////////
///////////////////////////////// Main function
/////////////////////////////////

int main()
{
    ////////////// Test functions

    ////////////////////////// Exercise 1
    std::vector<float> stats;
    stats.push_back(-3.f);
    stats.push_back(-1.f);
    stats.push_back(1.f);
    stats.push_back(1.f);
    stats.push_back(1.f);
    stats.push_back(3.f);
    stats.push_back(3.f);
    stats.push_back(3.f);

    std::pair<float, float> statsAnswer = Statistics(stats);

    if (equalFloats(statsAnswer.first, 1.f) && equalFloats(statsAnswer.second, 2.0)) {
        std::cout << "Exercise 1: Correct" << std::endl;
    } else {
        std::cout << "Exercise 1: Incorrect" << std::endl;
    }
    ///////////////////////////////////////////////////////////

    ////////////////////////// Exercise 2
    Tree root;
    fillTree(root, 0);
    TreeVisitor visitor;
    float visitAnswer1 = visitor.visitTree(root, false);
    float visitAnswer2 = visitor.visitTree(root, true);

    if (equalFloats(visitAnswer1, 1132.f) && equalFloats(visitAnswer2, 984.f)) {
        std::cout << "Exercise 2: Correct" << std::endl;
    } else {
        std::cout << "Exercise 2: Incorrect" << std::endl;
    }
    ///////////////////////////////////////////////////////////

    ////////////////////////// Exercise 3
    Complex c1(1.f, 2.f);
    Complex c2(5.f, -3.f);
    Complex complexAnswer = ComplexOperation(c1, c2);

    if (equalFloats(complexAnswer.real, -19.f) && equalFloats(complexAnswer.im, 34.f)) {
        std::cout << "Exercise 3: Correct" << std::endl;
    } else {
        std::cout << "Exercise 3: Incorrect" << std::endl;
    }
    ///////////////////////////////////////////////////////////

    ////////////////////////// Exercise 4
    std::vector<float> heights;
    heights.push_back(7);
    heights.push_back(6);
    heights.push_back(4);
    heights.push_back(7);
    heights.push_back(6);
    heights.push_back(9);
    heights.push_back(3);
    heights.push_back(1);
    heights.push_back(5);
    heights.push_back(3);

    float waterAnswer = WaterLevels(heights);

    if (equalFloats(waterAnswer, 11.f)) {
        std::cout << "Exercise 4: Correct" << std::endl;
    } else {
        std::cout << "Exercise 4: Incorrect" << std::endl;
    }
    ///////////////////////////////////////////////////////////

    ////////////////////////// Exercise 4
    std::set<std::pair<location, location>> labyrinth;
    labyrinth.insert({ location(0, 0), location(1, 0) });
    labyrinth.insert({ location(0, 1), location(1, 1) });
    labyrinth.insert({ location(0, 2), location(0, 3) });
    labyrinth.insert({ location(1, 1), location(1, 2) });
    labyrinth.insert({ location(1, 2), location(2, 2) });
    labyrinth.insert({ location(2, 3), location(3, 3) });
    labyrinth.insert({ location(2, 2), location(3, 2) });
    labyrinth.insert({ location(2, 1), location(3, 1) });

    int labAnswer = Labyrinth(labyrinth, 4);
    if (labAnswer == 13) {
        std::cout << "Exercise 5: Correct" << std::endl;
    } else {
        std::cout << "Exercise 5: Incorrect" << std::endl;
    }
    ///////////////////////////////////////////////////////////

    std::cout << "All done!" << std::endl;
    return 0;
}
