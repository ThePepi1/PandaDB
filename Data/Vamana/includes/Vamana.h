#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../../MathCompponents/includes/Vector/IVector.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "../../KMeans/includes/codeBook.h"
#include "../includes/Vector/FactoryVector.h"
#include "../../KMeans/includes/productquntizier.h"
namespace DataStructures{
    class Vamana{ 
        std::string filePath;
        public:
        std::vector<std::unique_ptr<data::IVector>> search(const data::IVector& query, const KMeans::ProductQuantizer &pq, std::vector<std::vector<long long>>& codebookAssignments,const data::FactoryVector &factory, size_t K);
        std::string merge(std::vector<std::string> &files);
        bool buildIndex(LinearStorage &storage, const KMeans::CodeBook &codebook, const KMeans::ProductQuantizer &pq , 
            std::vector<std::vector<long long>> &codebookAssignments, const data::FactoryVector &factory);
    };
}