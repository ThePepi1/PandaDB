#include "../includes/IVectorLoadStream.h"

namespace DataStructures {

    IVectorLoadStream::IVectorLoadStream(size_t start,size_t end,IVectorStream& stream):currentIndex(0),end(end - start)
    {
        loadedVectors = stream.getVectors(end - start);
    }

    bool IVectorLoadStream::hasNext()
    {
        return currentIndex < end;
    }

    std::shared_ptr<data::IVector> IVectorLoadStream::next()
    {
        if (!hasNext()) {
            return nullptr;
        }

        return loadedVectors[currentIndex++];
    }

    void IVectorLoadStream::reset()
    {
        currentIndex = 0;
    }

    int IVectorLoadStream::getDim() const
    {
        if (loadedVectors.empty()) {
            return 0;
        }

        return loadedVectors[0]->Dimension();
    }

    std::vector<std::shared_ptr<data::IVector>>
    IVectorLoadStream::getVectors(size_t count)
    {
        if (count == end) {
            return loadedVectors;
        }
        std::vector<std::shared_ptr<data::IVector>> result;
        result.reserve(std::min(count, end - currentIndex));

        for (size_t i = 0;i < count && hasNext();++i)
        {
            result.push_back(next());
        }

        return result;
    }

    long long IVectorLoadStream::getSize() const
    {
        return static_cast<long long>(end);
    }

}