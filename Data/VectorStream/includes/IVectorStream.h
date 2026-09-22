#pragma once 
#include "../includes/Vector/IVector.h"
#include <vector>
namespace DataStructures { 
    class IVectorStream {
        private:
        protected:
        public:
            virtual ~IVectorStream() = default;
            virtual std::vector<std::shared_ptr<data::IVector>> getVectors(size_t count) = 0;
            virtual bool hasNext() = 0;
            /*
                This returns shered pointer idea is idk if vector is used by IVectorStream as structure of wich i have some type of cash 
                this way idk i create it make it shered and if it is only copy it will be destroyed if it cahses it will be kept alive 
                TIme that i will wast by it is minor in comparison to headace that i will have later
            */
            virtual std::shared_ptr<data::IVector> next() = 0; 
            virtual void reset() = 0;
            virtual int getDim() const = 0;
    };
}