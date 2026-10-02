#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../../MathCompponents/includes/Vector/IVector.h"
#include "../includes/Vector/FactoryVector.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "../../KMeans/includes/codeBook.h"
#include "../../Data/Space/includes/metadata.h"

namespace DataStructures{
    class Vamana{ 
        std::string filePath;
        private:
        std::string merge(std::vector<std::string> &files,  data::FactoryVector &factory, const Metadata &metadata);
        bool RobusePune(const Node &node, const KMeans::ProductQuantizer &pq, std::vector<std::vector<long long>>& codebookAssignments, 
            data::FactoryVector &factory, const Metadata &metadata, std::string &filePath); 
        public:
        std::vector<std::unique_ptr<Node>> search(const data::IVector& query, 
            const KMeans::ProductQuantizer &pq, std::vector<std::vector<long long>>& codebookAssignments,
             data::FactoryVector &factory, size_t K, const Metadata &metadata, metrics::IMetrics &metric);
        Vamana(const std::string& filePath);
        bool add(data::IVector &vector, const KMeans::ProductQuantizer &pq, 
            std::vector<std::vector<long long>>& codebookAssignments, data::FactoryVector &factory, const Metadata &metadata);
        bool buildIndex(LinearStorage<Node> &storage, const KMeans::CodeBook &codebook, data::FactoryVector &factory, const Metadata &metadata, metrics::IMetrics &distance);
    };
    void removeDumpFiles(const std::string& filePath) ;
}