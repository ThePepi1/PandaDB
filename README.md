<div align="center">

# 🐼 PandaDB
### ⚡ High-performance vector search engine in C++20

**Fast approximate nearest-neighbor search, modular indexing, and vector-oriented infrastructure.**

![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-4.2%2B-064F8C?logo=cmake&logoColor=white)
![Status](https://img.shields.io/badge/status-in%20development-orange)
![Recall](https://img.shields.io/badge/Recall%4010-92%25-brightgreen)
![Latency](https://img.shields.io/badge/query_latency-~8%20ms-blue)
![ANN](https://img.shields.io/badge/search-ANN-8A2BE2)
![Vamana](https://img.shields.io/badge/index-Vamana-ff69b4)
![Tests](https://img.shields.io/badge/tests-GoogleTest-34A853?logo=google)

**🕸️ Graph indexing · 📐 Similarity metrics · 🧠 Quantization · 🚀 Optimized C++**

</div>

---

## 🌟 Why PandaDB?

> **Find the nearest vectors without searching every vector.** PandaDB brings together graph-based indexing, efficient distance computation, and modular storage in a performance-focused C++ codebase.

## 🚀 Overview

**PandaDB** is a C++20 project focused on efficient vector storage, similarity search, and approximate nearest-neighbor (ANN) indexing. Its modular design separates search algorithms, distance metrics, storage, quantization, and mathematical primitives.

### 🏎️ Performance snapshot

| Metric | Result |
|:--|--:|
| **Recall@10** | **92%** |
| **Average query latency** | **~8 ms / query** |
| **Approximate throughput (1 / latency)** | **~125 queries/s** |

> 📌 These are developer-reported measurements, not independently reproduced benchmarks. Dataset, hardware, index configuration, concurrency, and benchmark methodology have not been specified. The throughput figure is a simple reciprocal of latency, not a measured concurrent throughput.

## ✨ Features at a glance

- 🕸️ **Vamana graph index** — graph-based ANN search components.
- 🔎 **Search engine** — dedicated search and neighbor abstractions.
- 📐 **Similarity metrics** — Euclidean distance, cosine similarity, and dot product.
- 🧱 **Flat index and linear storage** — alternative data access and storage components.
- 🧠 **K-means and product quantization** — training and codebook modules.
- 📦 **Vector streams and file handling** — data loading and persistence-related infrastructure.
- 🧮 **CPU vector and matrix primitives** — numerical building blocks.
- ⚙️ **Performance-oriented build** — optional AVX2/FMA compilation flags, TBB, and liburing integration.
- 🧪 **GoogleTest suite** — tests covering core modules and search functionality.

## 🧩 Inside the engine

| Component | Role |
|:--|:--|
| 🕸️ **Vamana** | Graph-based approximate nearest-neighbor indexing |
| 🎯 **Metrics** | Vector distance and similarity scoring |
| 🧠 **K-means / PQ** | Clustering and quantization components |
| 💾 **Storage** | Flat and linear storage abstractions |
| 🧮 **Math** | CPU vector and matrix operations |
| 🔍 **Search** | Neighbor discovery and query processing |

## 🗂️ Project structure

```text
PandaDB/
├── Algoritams/       # Search and neighbor logic
├── Data/
│   ├── FlatIndex/    # Flat indexing
│   ├── LinearStorage/# Linear storage
│   ├── Space/        # Space and metadata
│   ├── Vamana/       # Graph-based ANN index
│   └── VectorStream/ # Vector input and loading
├── File/             # File management
├── KMeans/           # K-means, codebooks and product quantization
├── MathCompponents/  # CPU vector and matrix operations
├── Metrics/          # Distance and similarity metrics
├── Utils/            # Configuration and helpers
├── CMakeLists.txt
└── main.cpp
```

## 🛠️ Build from source

### 📋 Requirements

- C++20-compatible compiler
- CMake **4.2+**
- Ninja or another supported CMake generator
- Intel oneTBB development package
- `liburing` development package and `pkg-config`
- HDF5 C development package (for the test target)
- Internet access during first configuration to fetch GoogleTest v1.14.0

On Debian/Ubuntu, the system dependencies can typically be installed with:

```bash
sudo apt update
sudo apt install build-essential ninja-build pkg-config libtbb-dev liburing-dev libhdf5-dev
```

Install a suitable CMake 4.2+ version separately if your distribution ships an older version.

### 🔧 Configure and compile

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### 🧪 Run tests

```bash
ctest --test-dir build --output-on-failure
```

> 💡 The current `main.cpp` is a minimal entry point. PandaDB is primarily represented by its `panda_lib` library and module tests; a full CLI or server interface is not documented here.

## 🧭 Architecture at a glance

```text
              Input vectors
                   │
           VectorStream / File
                   │
             Storage / Space
                   │
       ┌───────────┴───────────┐
       │                       │
   FlatIndex                Vamana
       │                       │
       └───────────┬───────────┘
                   │
          Search + Metrics
                   │
             Top-k results

    K-means / Product Quantization
       (supporting components)
```

## 📊 Benchmark transparency

The headline **92% Recall@10** and **~8 ms per query** describe the reported current result. To reproduce or compare it fairly, publish the dataset and vector dimensionality, number of indexed vectors, ground-truth method, hardware, compiler flags, graph/index parameters, query count, warm-up policy, and latency distribution (mean/p50/p95/p99).

## 🚧 Development status

PandaDB is under active development. Public API stability, production readiness, supported platforms, and licensing should not be assumed from this README.

---

<div align="center">

### 🐼 PandaDB

**Built for fast vector search. Designed for experimentation.**

⚡ Speed &nbsp; · &nbsp; 🎯 Accuracy &nbsp; · &nbsp; 🧩 Modularity

</div>
