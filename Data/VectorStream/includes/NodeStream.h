#pragma once
#include "IStream.h"
#include "../../Data/LinearStorage/includes/linearStorage.h"
#include "LoadStream.h"
#include "ReaderStream.h"
#include "../../Data/Vamana/includes/Node.h"

namespace DataStructures {
    using INodeStream       = IStream<Node>;
    using INodeReaderStream = ReaderStream<Node>;
    using INodeLoadStream   = LoadStream<Node>;
    using LinearNodeStorage = LinearStorage<Node>;
}