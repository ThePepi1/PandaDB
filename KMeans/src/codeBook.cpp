#include "../includes/codeBook.h"
#include "Vector/FactoryVector.h"
#include "Vector/IVector.h"
#include <random>
#include <vector>
#include <algorithm>

namespace KMeans {
    CodeBook::CodeBook(int size, int dim, data::FactoryVector& factory) : coodeBookSize(size), codeBookDim(dim), factory(factory) {
        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<double> dist(-1000, 1000);
        for (int i = 0; i < size; ++i) {
            centroids.push_back(factory.createVector(dim));
            averages.push_back(factory. createVector(dim, 0.0));
            for (int j = 0; j < dim; ++j) {
                (*centroids[i])(j) = dist(rng); // Initialize to random values
            }
        }
        counts.resize(size, 0);   
        centroidMutexes = std::vector<std::mutex>(size); // Initialize mutexes for each centroid
    }
    CodeBook::CodeBook(int size, int dim, data::FactoryVector& factory, bool generate) : coodeBookSize(size), codeBookDim(dim), factory(factory) {
        if (generate) {
            std::mt19937 rng{std::random_device{}()};
            std::uniform_real_distribution<double> dist(-1000, 1000);
            for (int i = 0; i < size; ++i) {
                centroids.push_back(factory.createVector(dim));
                averages.push_back(factory.createVector(dim, 0.0));
                for (int j = 0; j < dim; ++j) {
                    (*centroids[i])(j) = dist(rng); // Initialize to random values
                }
            }
        } else {
            for (int i = 0; i < size; ++i) {
                centroids.push_back(factory.createVector(dim));
                averages.push_back(factory.createVector(dim, 0.0));
            }
        }
        
        centroidMutexes = std::vector<std::mutex>(size); // Initialize mutexes for each centroid
        counts.resize(size, 0);
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
    int CodeBook::getDim() const {
        return codeBookDim;
    }
    long long CodeBook::getSize() const {
        return coodeBookSize;
    }
    file::fileReader &operator>>(file::fileReader& reader, CodeBook& codeBook) {
        reader >> codeBook.coodeBookSize;
        reader >> codeBook.codeBookDim; 
        codeBook.centroids.clear();
        for (long long i = 0; i < codeBook.coodeBookSize; ++i) {
            std::unique_ptr<data::IVector> vec = codeBook.factory.createVector(codeBook.codeBookDim);
            reader >> *vec;
            codeBook.centroids.push_back(std::move(vec));
        }
        return reader;
    }
    file::fileReaderWriter &operator<<(file::fileReaderWriter& writer, const CodeBook& codeBook) {
        writer << codeBook.coodeBookSize;
        writer << codeBook.codeBookDim;
        for (const auto& centroid : codeBook.centroids) {
            writer << *centroid;
        }
        return writer;
    }
    file::fileReaderWriter &operator>>(file::fileReaderWriter& rw, CodeBook& codeBook) {
        rw >> codeBook.coodeBookSize;
        rw >> codeBook.codeBookDim;
        codeBook.centroids.clear(); 
        for (long long i = 0; i < codeBook.coodeBookSize; ++i) {
            std::unique_ptr<data::IVector> vec = codeBook.factory.createVector();
            rw >> *vec;
            codeBook.centroids.push_back(std::move(vec));
        }
        return rw;
    }
    void CodeBook::fit(data::IVector& point, const metrics::IMetrics& metrics) {
        long long index = belongs(point, metrics);
        if (index >= 0 && index < coodeBookSize) {
            std::lock_guard<std::mutex> lock(centroidMutexes[index]);
            (*averages[index]) += point;
            counts[index]++;
        }
    }
    #include <random>

bool CodeBook::updateAverages(const std::vector<std::unique_ptr<data::IVector>>& reseedCandidates,
                              const metrics::IMetrics& metrics,
                              double eps) {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    bool updated = false;

    for (long long i = 0; i < coodeBookSize; ++i) {
        if (counts[i] > 0) {
            (*averages[i]) /= static_cast<double>(counts[i]);

            // pomeraj centroida (sa tolerancijom umesto != na double-ovima)
            if (metrics(*centroids[i], *averages[i]) > eps) {
                updated = true;
            }
            *centroids[i] = *averages[i];
        } else if (!reseedCandidates.empty()) {
            // Prazan klaster -> skoci na nasumicnu tacku iz podataka
            std::uniform_int_distribution<size_t> dist(0, reseedCandidates.size() - 1);
            *centroids[i] = *reseedCandidates[dist(rng)];
            updated = true; // treba jos iteracija da se stabilizuje
        }

        // UVEK resetuj, inace se akumulira preko iteracija
        (*averages[i]) *= 0.0;
        counts[i] = 0;
    }
    return updated;
}
    std::vector<long long> CodeBook::belongs(const data::IVector& vector, const metrics::IMetrics& metrics, int k) const {
        std::vector<std::pair<double, long long>> distances;
        for (long long i = 0; i < coodeBookSize; ++i) {
            double distance = metrics(*centroids[i], vector);
            distances.emplace_back(distance, i);
        }
        std::sort(distances.begin(), distances.end());
        std::vector<long long> closestIndices;
        for (int i = 0; i < k && i < static_cast<int>(distances.size()); ++i) {
            closestIndices.push_back(distances[i].second);
        }
        return closestIndices;
    }
    const data::IVector& CodeBook::getCentroid(long long index) const {
        if (index < 0 || index >= coodeBookSize) {
            throw std::out_of_range("Index out of range in getCentroid");
        }
        return *centroids[index];
    }
    const data::IVector& CodeBook::operator[](long long index) const {
        return getCentroid(index);
    }
}
