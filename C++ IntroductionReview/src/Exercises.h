#pragma once
#include <algorithm>
#include <map>
#include <numeric>
#include <set>
#include <span>
#include <tuple>
#include <vector>

#include <queue>

struct Tree;

////////////////// Exercise 1 ////////////////////////////////////
std::pair<float, float> Statistics(std::span<const float> values) {
	if (values.empty()) return { 0.0f, 0.0f };
	long size = values.size();
	double avg = std::accumulate(values.begin(), values.end(), 0) / (double)size;
	double sd = 0;
	for (const float f : values) {
		sd += std::pow(f - avg, 2);
	}
	sd = std::sqrt(sd / size);
	return { avg, sd };
}
//////////////////////////////////////////////////////////////////

////////////////// Exercise 2 ////////////////////////////////////
class TreeVisitor {
public:
	float visitTree(const Tree& tree, bool countOnlyEvenLevels) {
		return this->visit(tree, countOnlyEvenLevels, true);  // first layer is 0, which is even
	}

	float visit(const Tree& tree, bool countOnlyEvenLevels, bool isEvenLevel) {
		// the node itself
		float ret = 0;
		if (!countOnlyEvenLevels || isEvenLevel)
			ret += tree.value;
		// the children
		for (const Tree& child : tree.children)
			ret += visit(child, countOnlyEvenLevels, !isEvenLevel);
		return ret;
	}
};
//////////////////////////////////////////////////////////////////

////////////////// Exercise 3 ////////////////////////////////////
class Complex {
public:
	Complex(float real_, float imaginary_) : real(real_), im(imaginary_) {
	}

	Complex operator+(const Complex& c) const {
		return Complex(this->real + c.real, this->im + c.im);
	}

	Complex operator-(const Complex& c) const {
		return Complex(this->real - c.real, this->im - c.im);
	}

	Complex operator*(const Complex& c) const {
		return Complex(
			this->real * c.real - this->im * c.im,
			this->real * c.im + this->im * c.real
		);
	}

	float real, im;
};
//////////////////////////////////////////////////////////////////

////////////////// Exercise 4 ////////////////////////////////////
float WaterLevels(std::span<const float> heights) {
	float currLMax = 0, currRMax = 0;
	long n = heights.size();
	std::vector<float> lMax(n), rMax(n);
	for (int i = 0; i < n; ++i) {
		currLMax = std::max(currLMax, heights[i]);
		lMax[i] = currLMax;
		currRMax = std::max(currRMax, heights[n - i - 1]);
		rMax[n - i - 1] = currRMax;
	}
	float res = 0;
	for (int i = 1; i < n - 1; ++i) {
		res += std::min(lMax[i], rMax[i]) - heights[i];
	}
	return res;
}
//////////////////////////////////////////////////////////////////

////////////////// Exercise 5 ////////////////////////////////////
using location = std::pair<int, int>;

bool isInbound(const location& loc, const int size) {
	return loc.first >= 0 && loc.first < size &&
		loc.second >= 0 && loc.second < size;
}

bool accessible(const location& src, const location& dst,
	const std::set<std::pair<location, location>>& labyrinth) {
	return (!labyrinth.contains({ src, dst })) && (!labyrinth.contains({ dst, src }));
}

int Labyrinth(const std::set<std::pair<location, location>>& labyrinth, int size) {
	std::set<location> visited;
	std::queue<std::pair<location, int>> q;

	location start = { 0, 0 }, exit = { size - 1, size - 1 };
	location directions[] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
	q.push({ start, 1 });

	while (!q.empty()) {
		location cur = q.front().first;
		int distance = q.front().second;
		q.pop();

		if (cur == exit) return distance;

		for (location dir : directions) {
			location next = { cur.first + dir.first, cur.second + dir.second };
			if (isInbound(next, size) && accessible(cur, next, labyrinth) && !visited.contains(next)) {
				q.push({ next, distance + 1 });
				visited.insert(next);
			}
		}
	}

	return 0;
}
//////////////////////////////////////////////////////////////////
