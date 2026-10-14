#pragma once
#include <string>
#include <vector>
#include "../../Data/Vamana/includes/Vamana.h"
#include "../includes/metadata.h"
#include "../../KMeans/includes/codeBook.h"
#include "../../KMeans/includes/productquntizier.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "../includes/Vector/IVector.h"

namespace DataStructures { 
    class Space { 
        private:
            std::string name;
            std::string path;
            Metadata metadata;
            std::unique_ptr<metrics::IMetrics> metric;
            std::unique_ptr<DataStructures::Vamana> vamana;
            std::unique_ptr<KMeans::CodeBook> codeBook;
            std::unique_ptr<KMeans::ProductQuantizer> productQuantizer;
            std::vector<std::vector<long long>> dataQuantizations;
            bool SaveSpace();
            data::FactoryVector &factory;
        public:
            Space(const std::string& name, data::FactoryVector& factory);
            //Set all staff in metadata
            Space& GenerateSpace(const LinearStorage<data::IVector>& storage);
            bool LoadSpace();
            Space& AddVector( data::IVector& vector);
            Space& addMetric( std::string& metricName);
            Space& addVecDim(std::size_t dim);
            Space& addNeighborsSize(std::size_t size);
            Space& addBeamSize(std::size_t size);
            Space& addK(std::size_t k);
            Space& addL(std::size_t l);
            Space& addR(std::size_t r);
            Space& addEf(std::size_t ef);
            Space& addAlpha(double alpha);
            std::vector<std::unique_ptr<Node>> getNeighbors(const data::IVector& query, std::size_t k);
            
    };
}