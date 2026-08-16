#pragma once

#include "../graph/Graph.hpp"
#include <string>

class Storage {
public:
    // Save graph to a .webtnk file (SQLite db compressed with zstd)
    static bool save(const Graph& graph, const std::string& path);

    // Load graph from a .webtnk file
    static bool load(Graph& graph, const std::string& path);

private:
    static bool writeSqlite(const Graph& graph, const std::string& dbPath);
    static bool readSqlite(Graph& graph, const std::string& dbPath);

    static bool compressFile(const std::string& inPath, const std::string& outPath);
    static bool decompressFile(const std::string& inPath, const std::string& outPath);
};
