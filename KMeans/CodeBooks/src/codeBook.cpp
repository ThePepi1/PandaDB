#include "../includes/codeBook.h"
#include "Vector/FactoryVector.h"
#include "Vector/IVector.h"
#include <random>

namespace KMeans {
    CodeBook::CodeBook(int size, int dim, data::FactoryVector* factory) : coodeBookSize(size), codeBookDim(dim), factory(factory) {
        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<double> dist(-1000, 1000);
        for (int i = 0; i < size; ++i) {
            centroids.push_back(factory->createVector(dim));
            for (int j = 0; j < dim; ++j) {
                (*centroids[i])(j) = dist(rng); // Initialize to random values
            }
        }   
    }
    long long CodeBook::belongs(const data::IVector& vector, const metrics::IMetrics& metrics) const {
        long long closestIndex = -1;
        double closestDistance = std::numeric_limits<double>::max();
        for (long long i = 0; i < coodeBookSize; ++i) {
            double distance = metrics(*centroids[i], vector);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestIndex = i;
            }
        }
        return closestIndex;
    }
}
