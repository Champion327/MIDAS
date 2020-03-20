#pragma once

#include <unordered_set>
#include <random>

namespace RejectMIDAS::CPU {
struct NodeHash {
	// Fields
	// --------------------------------------------------------------------------------

	const int r, c;
	const int lenData;
	int* const param1 = nullptr; // Delete an uninitialized pointer is not a good idea
	int* const param2 = nullptr;
	float* const data = nullptr;
	std::unordered_set<int> indexModified;

	// Methods
	// --------------------------------------------------------------------------------

	NodeHash() = delete;
	NodeHash& operator=(const NodeHash& b) = delete;

	NodeHash(int numRow, int numColumn) :
		r(numRow), c(numColumn), lenData(r * c), param1(new int[r]), param2(new int[r]), data(new float[lenData]) {
		std::random_device generator;
		const std::uniform_int_distribution<> distribution;
#pragma omp parallel for
		for (int i = 0; i < r; i++) {
			param1[i] = distribution(generator) + 1; // ×0 is not a good idea, see Hash()
			param2[i] = distribution(generator);
		}
		std::fill(data, data + lenData, 0);
	}

	NodeHash(const NodeHash& b) :
		r(b.r), c(b.c), lenData(b.lenData), param1(new int[r]), param2(new int[r]), data(new float[lenData]) {
		std::copy(b.param1, b.param1 + r, param1);
		std::copy(b.param2, b.param2 + r, param2);
		std::copy(b.data, b.data + lenData, data);
	}

	~NodeHash() {
		delete[] param1;
		delete[] param2;
		delete[] data;
	}

	float operator()(int a) const {
		float least = std::numeric_limits<float>::infinity();
#pragma omp parallel for reduction(min:least)
		for (int i = 0; i < r; i++)
			least = std::min(least, data[i * c + Hash(a, i)]);
		return least;
	}

	int Hash(int a, int r) const {
		return abs(a * param1[r] + param2[r]) % c;
	}

	float Assign(int a, float to) const {
#pragma omp parallel for
		for (int i = 0; i < r; i++)
			data[i * c + Hash(a, i)] = to;
		return to;
	}

	void Add(int a, float by = 1) {
#pragma omp parallel for
		for (int i = 0; i < r; i++) {
			const int index = i * c + Hash(a, i);
			data[index] += by;
			indexModified.insert(index);
		}
	}

	void MultiplyAll(float by) const {
#pragma omp parallel for
		for (int i = 0; i < lenData; i++)
			data[i] *= by;
	}

	void Clear() {
		std::fill(data, data + lenData, 0);
		indexModified.clear();
	}
};
}
