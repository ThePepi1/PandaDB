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
    const std::string mergedFilePath = filePath + "_merged" + ".dump";
        LinearStorage<Node> mergedStorage(mergedFilePath, metadata.vectorDim, 0, metadata.NeighborsSize);

        std::vector<std::unique_ptr<LinearNodeStorage>> inputs;
        std::vector<std::unique_ptr<INodeStream>> streams;
        inputs.reserve(files.size());
        streams.reserve(files.size());

        for (const auto& file : files) {
            inputs.push_back(std::make_unique<LinearNodeStorage>(file, OpenMode::Read, true));
            streams.push_back(inputs.back()->getVectorStream(factory));
        }

        std::vector<std::shared_ptr<Node>> nodes(streams.size(), nullptr);
        while (true) {
            bool any = false;
            std::size_t minId = std::numeric_limits<std::size_t>::max();

            for (std::size_t i = 0; i < streams.size(); ++i) {
                if (!nodes[i] && streams[i]->hasNext()) nodes[i] = streams[i]->next();
                if (nodes[i] && (!any || nodes[i]->getId() < minId)) {
                    minId = nodes[i]->getId();
                    any = true;
                }
            }
            if (!any) break;

            std::shared_ptr<Node> mergedNode;
            for (std::size_t i = 0; i < nodes.size(); ++i) {
                if (nodes[i] && nodes[i]->getId() == minId) {
                    if (!mergedNode) mergedNode = nodes[i];
                    else             mergedNode->extend(*nodes[i]);
                    nodes[i] = nullptr;
                }
            }
            mergedStorage.saveVector(*mergedNode);
        }

        mergedStorage.commit();    
        return mergedFilePath;
    }
    bool Vamana::buildIndex(LinearStorage<Node> &storage, const KMeans::CodeBook &codebook, 
         data::FactoryVector &factory, const Metadata &metadata, metrics::IMetrics &metric) {    
        std::string tempFilePath = filePath + "_temp";
        std::vector<LinearNodeStorage*> tempStorages;
        std::vector<std::string> tempFilePaths;
        for (int i = 0; i < codebook.getSize(); ++i) {
            std::string tempFile = tempFilePath + "_cluster_" + std::to_string(i) + ".dump";
            tempFilePaths.push_back(tempFile);
            tempStorages.emplace_back(new LinearNodeStorage(tempFile , metadata.vectorDim, 0, metadata.NeighborsSize));
        }
        std::unique_ptr<INodeStream> stream = storage.getVectorStream(factory);
        auto startTime = std::chrono::high_resolution_clock::now();
        while (stream->hasNext()){ 
            std::shared_ptr<Node> node = stream->next();
            std::vector<long long> assignments = codebook.belongs(node->getVector(), metric, metadata.K);
            for (long long clusterId : assignments) {
                if (clusterId >= 0 && clusterId < codebook.getSize()) {
                    tempStorages[clusterId]->saveVector(*node);
                }
            }
        }
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        std::cout << "Finished saving nodes to temporary cluster files in " << elapsed.count() << " seconds ." << std::endl;

        for (auto& tempStorage : tempStorages) {
            tempStorage->commit();
        }
        for (auto& tempStorage : tempStorages) {
            if(tempStorage->getSize() == 0){ 
                continue;
            } 
            auto startTime = std::chrono::high_resolution_clock::now();
            auto g = ClusterGraph::load(*tempStorage, factory, metadata.NeighborsSize);
            std::cerr << "Building index for cluster file: " << tempStorage->getFilePath() << "with " << tempStorage->getSize() << " nodes." << std::endl; 
            g.randomizeNeighbors(metadata.R);
            std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - startTime;
            std::cerr << "Building index for cluster file: " << tempStorage->getFilePath() << " in " << elapsed.count() << " seconds." << std::endl;
            g.buildPass(g.ids().front(), 1.0, metadata.NeighborsSize , metadata.L, metric); // For now i use first so i dont wast time on medoid search but if needed i can change it to it
            std::chrono::duration<double> elapsed2 = std::chrono::high_resolution_clock::now() - startTime;
            std::cerr << "Finished first pass for cluster file: " << tempStorage->getFilePath() << " in " << elapsed2.count() << " seconds." << std::endl;
            g.buildPass(g.ids().front(), metadata.alpha, metadata.NeighborsSize , metadata.L, metric);
            std::chrono::duration<double> elapsed4 = std::chrono::high_resolution_clock::now() - startTime;
            std::cerr << "Finished second pass for cluster file: " << tempStorage->getFilePath() << " in " << elapsed4.count() << " seconds." << std::endl;
            g.save(*tempStorage);
            auto endTime2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed3 = endTime2 - startTime; 
            std::cerr << "Finished building index for cluster file: " << tempStorage->getFilePath() << " in " << elapsed3.count() << " seconds." << std::endl;
        }
        std::cerr << "Merging temporary cluster files into final index file: " << filePath + ".vamana" << std::endl;
         for (auto& tempStorage : tempStorages) {
            tempStorage->commit();
        }
        int size = tempStorages.size(); 
        for (int i = 0; i < size; ++i) {
            delete tempStorages[i]; 
            std::cerr << "Deleted temporary storage for cluster file: " << tempFilePaths[i] << std::endl;
        }
        std::string mergedFilePath = merge(tempFilePaths, factory, metadata);
        if(mergedFilePath.empty()){
            return false;
        }
        namespace fs = std::filesystem;
        fs::rename(mergedFilePath, filePath + ".vamana");
        removeDumpFiles(filePath);
        return true;  
    }
std::vector<std::unique_ptr<Node>> Vamana::search(
    const data::IVector& query, const KMeans::ProductQuantizer& pq,
    std::vector<std::vector<long long>>& codebookAssignments,
    data::FactoryVector& factory, size_t K, const Metadata& metadata,
    metrics::IMetrics& metric)
    {
        struct Cand { double dist; size_t id; bool expanded; };
        auto byDist = [](const Cand& a, const Cand& b) { return a.dist < b.dist; };
        const size_t n    = codebookAssignments.size();
        const size_t L    = std::max<size_t>(metadata.Ef, K);
        const size_t beam = std::max<size_t>(1, metadata.beamSize);

        std::vector<std::unique_ptr<Node>> result;
        if (n == 0 || K == 0) return result;


        LinearNodeStorage storage(filePath + ".vamana", OpenMode::Read, false);
        std::vector<Cand> pool;
        pool.reserve(L + 1);
        std::vector<std::pair<double, std::unique_ptr<Node>>> expanded;
        std::vector<bool> seen(n, false);
        const size_t startId = 0;   
        seen[startId] = true;
        pool.push_back({ metric.distance(codebookAssignments[startId], pq, query), startId, false });
        while (true) {
            std::vector<long long> ids;
            for (auto& c : pool) {
                if (c.expanded) continue;
                c.expanded = true;
                ids.push_back(static_cast<long long>(c.id));
                if (ids.size() == beam) break;
            }
            if (ids.empty()) break;                       
            auto nodes = storage.loadBeam(ids, &factory, metadata.NeighborsSize);
            const size_t firstNew = expanded.size();
            for (auto& node : nodes) {
                const double exact = metric(node->getVector(), query);
                const size_t id = static_cast<size_t>(node->getId());
                for (auto& c : pool) {
                    if (c.id == id) { c.dist = exact; break; }
                }
                expanded.emplace_back(exact, std::move(node));
            }
            std::sort(pool.begin(), pool.end(), byDist);
            for (size_t e = firstNew; e < expanded.size(); ++e) {
                for (size_t nb : expanded[e].second->getNeighbors()) {
                    if (nb == EMPTY_NEIGHBOR || nb >= n || seen[nb]) continue;
                    seen[nb] = true;

                    const double d = metric.distance(codebookAssignments[nb], pq, query);
                    if (pool.size() >= L && d >= pool.back().dist) continue;

                    Cand c{ d, nb, false };
                    pool.insert(std::upper_bound(pool.begin(), pool.end(), c, byDist), c);
                    if (pool.size() > L) pool.pop_back();
                }
            }
        }

        std::sort(expanded.begin(), expanded.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
        const size_t take = std::min(K, expanded.size());
        result.reserve(take);
        for (size_t i = 0; i < take; ++i)
            result.push_back(std::move(expanded[i].second));
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