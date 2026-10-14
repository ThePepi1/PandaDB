#pragma once
#include "IStream.h"
#include "ReaderStream.h"
#include "LoadStream.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "Vector/IVector.h"

namespace DataStructures {
    using IVectorStream       = IStream<data::IVector>;
    using IVectorReaderStream = ReaderStream<data::IVector>;
    using IVectorLoadStream   = LoadStream<data::IVector>;
    using LinearVectorStorage = LinearStorage<data::IVector>;
}