//
// Created by pepi on 5/8/26.
//

#include "../includes/IMetrics.h"
#include <vector>
#include "../../KMeans/includes/codeBook.h"
#include "../../KMeans/includes/productquntizier.h"
#include "Vector/IVector.h"
#include <math.h>
namespace metrics {
    double IMetrics::distance(std::vector<long long> vectorSegment,const KMeans::ProductQuantizer &quantizaer, const data::IVector &compear) {
        double distance = 0.0;
        for(int i = 0, j = 0; i < vectorSegment.size() || j < vectorSegment.size(); ++j){
            const data::IVector& centroid = quantizaer[j][vectorSegment[j]];
            distance += pow(this->operator()(centroid, (*compear.clone(i, centroid.Dimension()))), 2);
            i += centroid.Dimension();
        }
        return sqrt(distance);
    }
}