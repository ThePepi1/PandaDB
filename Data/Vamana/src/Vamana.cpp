#include "../includes/Vamana.h"
#include "../../Data/VectorStream/includes/NodeStream.h"
#include "Vector/FactoryVector.h"
#include "../../Utils/helperFunctions/includes/array.h"
#include <filesystem>
#include <memory>
#include <limits>
#include <algorithm>
#include <utility>
#include <vector>
#include "../../Data/Vamana/includes/ClusterNode.h"
#include <map>
#include <queue>
#include <filesystem>
#include <iostream>

namespace DataStructures { 

    Vamana::Vamana(const std::string& filePath) : filePath(filePath) {}
    std::string Vamana::merge(std::vector<std::string> &files, data::FactoryVector &factory, const Metadata &metadata) {
        std::string mergedFilePath = filePath + "_merged" + ".dump";
        LinearStorage<Node> mergedStorage(mergedFilePath,   metadata.vectorDim, 0, metadata.NeighborsSize);
        std::vector<std::unique_ptr<LinearNodeStorage>> LinearStorageVectors;
        std::vector<std::unique_ptr<INodeStream>> storages;
        for (const auto& file : files) {
            LinearStorageVectors.push_back(std::make_unique<LinearNodeStorage>(file, true));
            storages.push_back(LinearStorageVectors.back()->getVectorStream(factory));
        }
        std::vector<std::shared_ptr<Node>> nodes(storages.size(), nullptr);
        while (true) { 
            long long minId = std::numeric_limits<long long>::max();
            for (int i = 0; i < storages.size(); ++i) {
                if (storages[i]->hasNext() && nodes[i] == nullptr) {
                    nodes[i] = storages[i]->next();
                } 
                if (nodes[i] && nodes[i]->getId() < minId) {
                        minId = nodes[i]->getId();
                    }
            }
            if (minId == std::numeric_limits<long long>::max()) {
                break; 
            }          
            std::shared_ptr<Node> mergedNode = nullptr;
            for (int i = 0; i < nodes.size(); ++i) {
                if (nodes[i] && nodes[i]->getId() == minId) {
                    if (!mergedNode) {
                        mergedNode = nodes[i];
                    } else {
                        mergedNode->extend(*nodes[i]);
                    }
                    nodes[i] = nullptr;
                }
            }
            if (mergedNode) {
                mergedStorage.saveVector(*mergedNode, true);
            } else {
                break; 
            }
        }
        return mergedFilePath;
    }
    bool Vamana::buildIndex(LinearStorage<Node> &storage, const KMeans::CodeBook &codebook, 
         data::FactoryVector &factory, const Metadata &metadata, metrics::IMetrics &metric) {    
        std::string tempFilePath = filePath + "_temp";
        std::vector<LinearNodeStorage> tempStorages;
        std::vector<std::string> tempFilePaths;
        for (int i = 0; i < codebook.getSize(); ++i) {
            std::string tempFile = tempFilePath + "_cluster_" + std::to_string(i) + ".dump";
            tempFilePaths.push_back(tempFile);
            tempStorages.emplace_back(tempFile , metadata.vectorDim, 0, metadata.NeighborsSize);
        }
        std::unique_ptr<INodeStream> stream = storage.getVectorStream(factory);
        while (stream->hasNext()){ 
            std::shared_ptr<Node> node = stream->next();
            std::vector<long long> assignments = codebook.belongs(node->getVector(), metric, metadata.K);
            for (long long clusterId : assignments) {
                if (clusterId >= 0 && clusterId < codebook.getSize()) {
                    tempStorages[clusterId].saveVector(*node);
                }
            }
        }
        for (auto& tempStorage : tempStorages) {
            if(tempStorage.getSize() == 0) continue;
            std::cerr << "Building cluster with " << tempStorage.getSize() << " nodes\n";
            auto g = ClusterGraph::load(tempStorage, factory, metadata.NeighborsSize);
            std::cerr << "Cluster loaded with " << g.size() << " nodes\n";
            g.randomizeNeighbors(metadata.R);
            std::cerr << "Building cluster with " << g.size() << "first id: " << g.ids().front() << " nodes\n";
            g.buildPass(g.ids().front(), 1.0, metadata.NeighborsSize , metadata.L, metric); // For now i use first so i dont wast time on medoid search but if needed i can change it to it
            g.buildPass(g.ids().front(), metadata.alpha, metadata.NeighborsSize , metadata.L, metric);
            g.save(tempStorage);
        }
        std::string mergedFilePath = merge(tempFilePaths, factory, metadata);
        // Change mearged file into index file and delete temp files :D 
        if(mergedFilePath.empty()){
            return false;
        }
        namespace fs = std::filesystem;
        fs::rename(mergedFilePath, filePath + ".vamana");
        removeDumpFiles(filePath);
        return true;  
    }
    std::vector<std::unique_ptr<Node>> Vamana::search(const data::IVector& query, const KMeans::ProductQuantizer &pq, std::vector<std::vector<long long>>& codebookAssignments,
        data::FactoryVector &factory, size_t K, const Metadata &metadata, metrics::IMetrics &metric) {
            auto cmp = [](const auto& a, const auto& b) { return a.first > b.first; };
            long long candidatesSize = std::max(metadata.Ef, K);
            std::vector<std::pair<double, long long>> candidates;
            std::priority_queue<std::pair<double, std::size_t>> topK;
            std::map<std::size_t, bool> visited;
            LinearNodeStorage storage(filePath + ".vamana", true);
            topK.push({std::numeric_limits<double>::max(), 1});
            while(topK.empty())
            { 
               std::vector<long long> beamSearchIndexes; 
               for(int i = 0; i < metadata.beamSize && !topK.empty(); ++i){
                    auto [dist, id] = topK.top();
                    topK.pop();
                    beamSearchIndexes.push_back(id);
               }
               auto nodes = storage.loadBeam(beamSearchIndexes, &factory, metadata.NeighborsSize);
                for(auto &node : nodes){
                    double calculatedDistance = metric(node->getVector(), query);
                    // update in candidates his distance
                    for (int i = 0; i < candidates.size(); ++i) {
                        if (candidates[i].second == node->getId()) {
                            candidates[i].first = calculatedDistance;
                            Utils::repositionElement(candidates, i, cmp);
                            break;
                        }
                    }
                    for (auto &neighbour : node->getNeighbors()){
                        if(!visited[neighbour]){
                            visited[neighbour] = true;
                            double predictedDistance = metric.distance(codebookAssignments[neighbour], pq, query);
                            if(predictedDistance < candidates.back().first || candidates.size() < candidatesSize){
                                topK.push({predictedDistance, neighbour});
                                if(candidates.size() < candidatesSize){
                                    candidates.push_back({predictedDistance, neighbour});
                                } else {
                                   candidates.back() = {predictedDistance, neighbour}; 
                                                            }
                                Utils::repositionElement(candidates, candidates.size() - 1, cmp);
                            }
                        }
                    }
                }
            }
            std::vector<std::unique_ptr<Node>> result;
            for (const auto& [dist, id] : candidates) {
                std::unique_ptr<Node> node = std::make_unique<Node>(metadata.NeighborsSize, factory);
                storage.loadVector(id, *node);
                result.push_back(std::move(node));
            }
            return result;
        }
    void removeDumpFiles(const std::string& filePath) {
    namespace fs = std::filesystem;

    fs::path dir = fs::path(filePath).parent_path();
    if (dir.empty()) dir = ".";              // filePath bez foldera = trenutni folder

    std::error_code ec;
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
        std::error_code rmEc;                // poseban ec, da neuspelo brisanje ne prekine petlju
        if (it->is_regular_file(rmEc) && it->path().extension() == ".dump")
            fs::remove(it->path(), rmEc);
    }
}
    

}