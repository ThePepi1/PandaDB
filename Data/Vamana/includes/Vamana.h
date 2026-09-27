#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../../MathCompponents/includes/Vector/IVector.h"
#include "../includes/Vector/FactoryVector.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "../../KMeans/includes/codeBook.h"


namespace DataStructures{
    class Vamana{ 
        std::string filePath;
        public:
        std::vector<std::unique_ptr<data::IVector>> search(const data::IVector& query, const KMeans::ProductQuantizer &pq, std::vector<std::vector<long long>>& codebookAssignments,const data::FactoryVector &factory, size_t K);
        std::string merge(std::vector<std::string> &files);
        bool buildIndex(LinearStorage<data::IVector> &storage, const KMeans::CodeBook &codebook, const KMeans::ProductQuantizer &pq , 
            std::vector<std::vector<long long>> &codebookAssignments, const data::FactoryVector &factory);
    };
}