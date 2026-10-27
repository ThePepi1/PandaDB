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
#include "../../KMeans/includes/productquntizier.h"
#include <algorithm>
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
    bool Vamana::buildIndex(INodeStream &stream, const KMeans::CodeBook &codebook, 
         data::FactoryVector &factory, const Metadata &metadata, metrics::IMetrics &metric) {    
        std::string tempFilePath = filePath + "_temp";
        std::vector<LinearNodeStorage*> tempStorages;
        std::vector<std::string> tempFilePaths;
        for (int i = 0; i < codebook.getSize(); ++i) {
            std::string tempFile = tempFilePath + "_cluster_" + std::to_string(i) + ".dump";
            tempFilePaths.push_back(tempFile);
            tempStorages.emplace_back(new LinearNodeStorage(tempFile , metadata.vectorDim, 0, metadata.NeighborsSize / metadata.K));
        }
        auto startTime = std::chrono::high_resolution_clock::now();
        while (stream.hasNext()){ 
            std::shared_ptr<Node> node = stream.next();
            std::vector<long long> assignments = codebook.belongs(node->getVector(), metric, metadata.K);
            for (long long clusterId : assignments) {
                if (clusterId >= 0 && clusterId < codebook.getSize()) {
                    node->setNeighbors(std::vector<std::size_t>(metadata.NeighborsSize / metadata.K , EMPTY_NEIGHBOR));
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
            auto g = ClusterGraph::load(*tempStorage, factory, metadata.NeighborsSize/ metadata.K);
            std::cerr << "Building index for cluster file: " << tempStorage->getFilePath() << "with " << tempStorage->getSize() << " nodes." << std::endl; 
            g.randomizeNeighbors(metadata.R);
            std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - startTime;
            std::cerr << "Building index for cluster file: " << tempStorage->getFilePath() << " in " << elapsed.count() << " seconds." << std::endl;
            g.buildPass(g.ids().front(), 1.0, metadata.NeighborsSize / metadata.K  , metadata.L, metric); // For now i use first so i dont wast time on medoid search but if needed i can change it to it
            std::chrono::duration<double> elapsed2 = std::chrono::high_resolution_clock::now() - startTime;
            std::cerr << "Finished first pass for cluster file: " << tempStorage->getFilePath() << " in " << elapsed2.count() << " seconds." << std::endl;
            g.buildPass(g.ids().front(), metadata.alpha, metadata.NeighborsSize / metadata.K , metadata.L, metric);
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
bool Vamana::RobusePune(Node &node, data::FactoryVector &factory, const Metadata &metadata,
                        LinearStorage<Node> &storage, metrics::IMetrics &metric) {
    const std::size_t R = metadata.NeighborsSize;
    const double alpha  = metadata.alpha;

    struct Cand { double dist; std::size_t id; std::unique_ptr<Node> node; };
    std::vector<Cand> cands;
    std::unordered_set<std::size_t> seen;
    seen.insert(node.getId());                       // ne sme sam sebi da bude sused

    for (std::size_t nbId : node.getNeighbors()) {
        if (nbId == EMPTY_NEIGHBOR || !seen.insert(nbId).second) continue;
        auto nb = std::make_unique<Node>(metadata.NeighborsSize, factory);
        storage.loadVector(static_cast<long long>(nbId), *nb);
        const double d = metric(node.getVector(), nb->getVector());
        cands.push_back({ d, nbId, std::move(nb) });
    }

    std::sort(cands.begin(), cands.end(),
              [](const Cand& a, const Cand& b) { return a.dist < b.dist; });

    std::vector<std::size_t> out;
    std::vector<bool> removed(cands.size(), false);
    for (std::size_t i = 0; i < cands.size() && out.size() < R; ++i) {
        if (removed[i]) continue;
        out.push_back(cands[i].id);                  // p* = najblizi preostali
        for (std::size_t j = i + 1; j < cands.size(); ++j) {
            if (removed[j]) continue;
            if (alpha * metric(cands[i].node->getVector(), cands[j].node->getVector())
                    <= cands[j].dist)
                removed[j] = true;                   // p* "pokriva" j
        }
    }
    out.resize(R, EMPTY_NEIGHBOR);
    node.setNeighbors(std::move(out));
    return true;
}
bool Vamana::add(data::IVector &vector, const KMeans::ProductQuantizer &pq,
                 std::vector<std::vector<long long>>& codebookAssignments,
                 data::FactoryVector &factory, const Metadata &metadata,
                 metrics::IMetrics &metric) {
    const std::size_t R     = metadata.NeighborsSize;
    const std::size_t n     = codebookAssignments.size();
    const std::size_t newId = n;                        // gusti id-evi: id == pozicija

    // 1. kandidati preko search-a (search sam otvara i zatvara fajl, pa ide pre pisanja)
    std::vector<std::unique_ptr<Node>> found;
    if (n > 0)
        found = search(vector, pq, codebookAssignments, factory,
                       std::max<std::size_t>(metadata.L, R), metadata, metric);

    std::vector<std::size_t> cand;
    cand.reserve(found.size());
    for (auto& f : found) cand.push_back(static_cast<std::size_t>(f->getId()));
    found.clear();

    LinearNodeStorage storage(filePath + ".vamana", OpenMode::Read, true);

    // 2. novi cvor, kandidati kao privremeni susedi pa RobusePune skracuje na R
    Node newNode(R, factory);
    newNode.setID(newId);                               // <-- prilagodi Node API-ju
    newNode.setVector(vector);                          // <-- prilagodi Node API-ju
    newNode.setNeighbors(cand);                         // moze biti duze od R, vidi napomenu
    if (!cand.empty() && !RobusePune(newNode, factory, metadata, storage, metric))
        return false;
    if (cand.empty()) newNode.setNeighbors(std::vector<std::size_t>(R, EMPTY_NEIGHBOR));

    // 3. upis na kraj fajla (mora pre povratnih grana, jer ih RobusePune ucitava iz storage-a)
    storage.saveVector(newNode);
    storage.commit();

    // 4. povratne grane j -> novi
    for (std::size_t j : newNode.getNeighbors()) {
        if (j == EMPTY_NEIGHBOR) continue;

        Node jn(R, factory);
        storage.loadVector(static_cast<long long>(j), jn);

        std::vector<std::size_t> nbrs;
        for (std::size_t x : jn.getNeighbors())
            if (x != EMPTY_NEIGHBOR) nbrs.push_back(x);
        if (std::find(nbrs.begin(), nbrs.end(), newId) != nbrs.end()) continue;

        nbrs.push_back(newId);
        if (nbrs.size() <= R) {
            nbrs.resize(R, EMPTY_NEIGHBOR);
            jn.setNeighbors(std::move(nbrs));
        } else {
            jn.setNeighbors(std::move(nbrs));           // R+1 suseda
            if (!RobusePune(jn, factory, metadata, storage, metric)) return false;
        }
        storage.overwriteVector(static_cast<long long>(j), jn);
    }
    storage.commit();
    codebookAssignments.push_back(pq.quantize(vector, metric));
     return true;
}
std::vector<std::unique_ptr<Node>> Vamana::search(const data::IVector& query, const KMeans::ProductQuantizer &pq, std::vector<std::vector<long long>>& codebookAssignments,
        data::FactoryVector &factory, size_t K, const Metadata &metadata, metrics::IMetrics &metric) {
            if(K == 0 || codebookAssignments.empty()){
                return {};
            }

            LinearNodeStorage storage(filePath + ".vamana", OpenMode::Read, true);

            if(storage.getSize() == 0 || 1 >= storage.getSize() || 1 >= codebookAssignments.size()){
                return {};
            }

            auto cmp = [](const auto& a, const auto& b) {
                return a.first < b.first;
            };

            size_t candidatesSize = std::min(
                static_cast<size_t>(std::max(metadata.Ef, K)),
                codebookAssignments.size()
            );

            std::vector<std::pair<double, long long>> topK;

            std::priority_queue<
                std::pair<double, std::size_t>,
                std::vector<std::pair<double, std::size_t>>,
                std::greater<std::pair<double, std::size_t>>
            > candidates;

            std::map<std::size_t, bool> visited;

            double startDistance = metric.distance(
                codebookAssignments[1],
                pq,
                query
            );

            candidates.push({startDistance, 1});
            visited[1] = true;
            topK.push_back({startDistance, 1});

            while(!candidates.empty()){
                if(topK.size() >= candidatesSize &&
                   candidates.top().first >= topK.back().first){
                    break;
                }

                std::vector<long long> beamSearchIndexes;

                for(int i = 0; i < metadata.beamSize && !candidates.empty(); ++i){
                    if(topK.size() >= candidatesSize &&
                       candidates.top().first >= topK.back().first){
                        break;
                    }

                    auto [dist, id] = candidates.top();
                    candidates.pop();

                    beamSearchIndexes.push_back(id);
                }

                if(beamSearchIndexes.empty()){
                    break;
                }

                auto nodes = storage.loadBeam(
                    beamSearchIndexes,
                    &factory,
                    metadata.NeighborsSize
                );

                for(auto &node : nodes){
                    for(auto &neighbour : node->getNeighbors()){
                        if(neighbour == EMPTY_NEIGHBOR){
                            continue;
                        }

                        if(neighbour >= codebookAssignments.size()){
                            continue;
                        }

                        if(visited[neighbour]){
                            continue;
                        }

                        visited[neighbour] = true;

                        double predictedDistance = metric.distance(
                            codebookAssignments[neighbour],
                            pq,
                            query
                        );

                        if(topK.size() < candidatesSize ||
                           predictedDistance < topK.back().first){

                            candidates.push({
                                predictedDistance,
                                neighbour
                            });

                            if(topK.size() < candidatesSize){
                                topK.push_back({
                                    predictedDistance,
                                    neighbour
                                });
                            } else {
                                topK.back() = {
                                    predictedDistance,
                                    neighbour
                                };
                            }

                            Utils::repositionElement(
                                topK,
                                topK.size() - 1,
                                cmp
                            );
                        }
                    }
                }
            }

            std::vector<std::pair<double, std::size_t>> exactResults;

            for(const auto& [dist, id] : topK){
                if(id >= storage.getSize()){
                    continue;
                }

                std::unique_ptr<Node> node =
                    std::make_unique<Node>(
                        metadata.NeighborsSize,
                        factory
                    );

                storage.loadVector(id, *node);

                double exactDistance = metric(
                    node->getVector(),
                    query
                );

                exactResults.push_back({
                    exactDistance,
                    id
                });
            }

            std::sort(
                exactResults.begin(),
                exactResults.end(),
                [](const auto& a, const auto& b){
                    return a.first < b.first;
                }
            );

            std::vector<std::unique_ptr<Node>> result;

            size_t resultSize = std::min(K, exactResults.size());

            for(size_t i = 0; i < resultSize; ++i){
                std::unique_ptr<Node> node =
                    std::make_unique<Node>(
                        metadata.NeighborsSize,
                        factory
                    );

                storage.loadVector(
                    exactResults[i].second,
                    *node
                );

                result.push_back(std::move(node));
            }

            return result;
        }
}