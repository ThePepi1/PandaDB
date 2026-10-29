#include "../includes/Space.h"
#include <functional> 
#include <filesystem>
#include <memory>
#include "../../File/includes/FileMenager.h"
#include "../../File/includes/fileHendler.h"
#include "../includes/metadata.h"
#include "../../Metrics/includes/MetricFactory.h"
#include "../../KMeans/includes/trainer.h"
#include "../../Data/VectorStream/includes/StreamTransletor.h"
namespace DataStructures {



    Space::Space(const std::string& name, data::FactoryVector& factory) : name(name), factory(factory) {
        metric = nullptr;
        vamana = nullptr;
        codeBook = nullptr;
        productQuantizer = nullptr;
        std::hash<std::string> hasher;
        std::size_t hashValue = hasher(name);
        path = "Data/Space/" + std::to_string(hashValue) ;
        if(!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
        
    }
    bool Space::SaveSpace() {
        std::string metadataFilePath = path + "/metadata.meta";
        auto metadataFile = file::FileMenager::getInstance().subbscribeFileReaderWriter(metadataFilePath);
        auto CodeBookFile = file::FileMenager::getInstance().subbscribeFileReaderWriter(path + "/codeBook.code");
        auto ProductQuantizerFile = file::FileMenager::getInstance().subbscribeFileReaderWriter(path + "/productQuantizer.prod");
        *metadataFile << metadata;
        if(codeBook) {
            *CodeBookFile << *codeBook;
        }
        if(productQuantizer) {
            *ProductQuantizerFile << *productQuantizer;
        }
        auto vectorFile = file::FileMenager::getInstance().subbscribeFileReaderWriter(path + "/quantizations.vec");
        *vectorFile << dataQuantizations;
        return true;
    }
    bool Space::LoadSpace() {
        std::string metadataFilePath = path + "/metadata.meta";
        auto metadataFile = file::FileMenager::getInstance().subbscribeFileReaderWriter(metadataFilePath);
        *metadataFile >> metadata;
        auto CodeBookFile = file::FileMenager::getInstance().subbscribeFileReaderWriter(path + "/codeBook.code");
        codeBook = std::make_unique<KMeans::CodeBook>(metadata.vectorDim, metadata.NumberOfClasters, this->factory, false);
        *CodeBookFile >> *codeBook;
        auto ProductQuantizerFile = file::FileMenager::getInstance().subbscribeFileReaderWriter(path + "/productQuantizer.prod");
        productQuantizer = std::make_unique<KMeans::ProductQuantizer>(this->factory);
        *ProductQuantizerFile >> *productQuantizer;
        auto vectorFile = file::FileMenager::getInstance().subbscribeFileReaderWriter(path + "/quantizations.vec");
        *vectorFile >> dataQuantizations;
        this->vamana = std::make_unique<DataStructures::Vamana>(path + "/vamana");
        return true;
    }
    Space& Space::GenerateSpace(const LinearStorage<data::IVector>& storage) {
        if(!metric) {
            this->metric = metrics::MetricFactory::instance().create_metric(metadata.metrixSystem);
        }
        if(!vamana) {
            vamana = std::make_unique<DataStructures::Vamana>(path + "/vamana");
        }
        if(!codeBook) {
            codeBook = std::make_unique<KMeans::CodeBook>(metadata.NumberOfClasters, metadata.vectorDim, this->factory, true);
        }
        if(!productQuantizer) {
            productQuantizer = std::make_unique<KMeans::ProductQuantizer>(this->factory);
        }
        KMeans::Trainer trainer;
        {
        auto stream = storage.getVectorStream(metadata.TrainingProportion, factory); // This line will hurt potensialy on large datasets
        trainer.train(*productQuantizer, *codeBook, *stream, *metric,metadata.NumberOfIterations);
        }
        
        long long count = 0;
        auto stream = storage.getVectorStream(factory);
        StreamTransletor transletor(std::move(stream), metadata.NeighborsSize);
        
        this->vamana->buildIndex(transletor, *codeBook, factory, metadata, *metric);
        
        stream->reset();
        while(stream->hasNext()){
            auto quant = productQuantizer->quantize(*stream->next(), *metric);
            dataQuantizations.push_back(quant);
        }        
        this->SaveSpace();
        return *this;
    } 

    Space& Space::AddVector( data::IVector& vector) {
        if(!metric) {
            this->metric = metrics::MetricFactory::instance().create_metric(metadata.metrixSystem);
        }
        if(!vamana) {
            vamana = std::make_unique<DataStructures::Vamana>(path + "/vamana");
        }
        if(!codeBook) {
            codeBook = std::make_unique<KMeans::CodeBook>(metadata.NumberOfClasters, metadata.vectorDim, this->factory, false);
        }
        if(!productQuantizer) {
            productQuantizer = std::make_unique<KMeans::ProductQuantizer>(this->factory);
        }
        this->vamana->add(vector, *productQuantizer, dataQuantizations, this->factory, this->metadata, *this->metric);
        return *this;
    }
    Space& Space::addMetric( std::string& metricName) {
        this->metadata.metrixSystem = metricName;
        return *this;
    }
    Space& Space::addVecDim(std::size_t dim) {
        this->metadata.vectorDim = dim;
        return *this;
    }
    Space& Space::addNeighborsSize(std::size_t size) {
        this->metadata.NeighborsSize = size;
        return *this;
    }

    Space& Space::addBeamSize(std::size_t size) {
        this->metadata.beamSize = size;
        return *this;
    }
    Space& Space::addK(std::size_t k) {
        this->metadata.K = k;
        return *this;
    }
    Space& Space::addL(std::size_t l) {
        this->metadata.L = l;
        return *this;
    }
    Space& Space::addR(std::size_t r) {
        this->metadata.R = r;
        return *this;
    }
    Space& Space::addEf(std::size_t ef) {
        this->metadata.Ef = ef;
        return *this; 
    }
    Space& Space::addAlpha(double alpha) {
        this->metadata.alpha = alpha;
        return *this;
    }
    std::vector<std::unique_ptr<Node>> Space::getNeighbors(const data::IVector& query, std::size_t k) {
        if(!metric) {
            this->metric = metrics::MetricFactory::instance().create_metric(metadata.metrixSystem);
        }
        if(!vamana) {
            vamana = std::make_unique<DataStructures::Vamana>(path + "/vamana");
        }
        if(!codeBook) {
            codeBook = std::make_unique<KMeans::CodeBook>(metadata.NumberOfClasters, metadata.vectorDim, this->factory, false);
        }
        if(!productQuantizer) {
            productQuantizer = std::make_unique<KMeans::ProductQuantizer>(this->factory);
        }

        auto result = vamana->search(query, *productQuantizer, dataQuantizations, factory, k, metadata, *metric);
        return result;
    }   


}