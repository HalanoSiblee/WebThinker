#include "Storage.hpp"
#include <sqlite3.h>
#include <zstd.h>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

bool Storage::writeSqlite(const Graph& graph, const std::string& dbPath) {
    sqlite3* db = nullptr;
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        return false;
    }

    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS meta (
            key   TEXT PRIMARY KEY,
            value TEXT
        );
        CREATE TABLE IF NOT EXISTS nodes (
            id    INTEGER PRIMARY KEY,
            x     REAL NOT NULL,
            y     REAL NOT NULL,
            title TEXT,
            text  TEXT,
            color INTEGER
        );
        CREATE TABLE IF NOT EXISTS edges (
            id   INTEGER PRIMARY KEY,
            "from" INTEGER NOT NULL,
            "to"   INTEGER NOT NULL,
            color INTEGER
        );
        CREATE TABLE IF NOT EXISTS squares (
            id    INTEGER PRIMARY KEY,
            x     REAL NOT NULL,
            y     REAL NOT NULL,
            w     REAL NOT NULL,
            h     REAL NOT NULL,
            title TEXT,
            text  TEXT,
            color INTEGER,
            z     INTEGER DEFAULT 0
        );
        CREATE TABLE IF NOT EXISTS vectors (
            id    INTEGER PRIMARY KEY,
            x     REAL NOT NULL,
            y     REAL NOT NULL,
            scale REAL NOT NULL,
            title TEXT,
            svg   TEXT
        );
    )";

    char* err = nullptr;
    if (sqlite3_exec(db, schema, nullptr, nullptr, &err) != SQLITE_OK) {
        sqlite3_free(err);
        sqlite3_close(db);
        return false;
    }

    // Clear existing
    sqlite3_exec(db, "ALTER TABLE edges ADD COLUMN color INTEGER;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "ALTER TABLE squares ADD COLUMN z INTEGER DEFAULT 0;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "DELETE FROM nodes; DELETE FROM edges; DELETE FROM squares; DELETE FROM vectors;", nullptr, nullptr, nullptr);

    // Insert nodes
    sqlite3_stmt* stmt = nullptr;
    const char* insNode = "INSERT INTO nodes (id, x, y, title, text, color) VALUES (?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, insNode, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    for (const auto* n : graph.nodes()) {
        sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(n->id));
        sqlite3_bind_double(stmt, 2, n->x);
        sqlite3_bind_double(stmt, 3, n->y);
        sqlite3_bind_text(stmt, 4, n->title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, n->text.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 6, static_cast<int>(n->color));
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    // Insert squares
    const char* insSq = "INSERT INTO squares (id, x, y, w, h, title, text, color, z) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, insSq, -1, &stmt, nullptr) == SQLITE_OK) {
        for (const auto* s : graph.squares()) {
            sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(s->id));
            sqlite3_bind_double(stmt, 2, s->x);
            sqlite3_bind_double(stmt, 3, s->y);
            sqlite3_bind_double(stmt, 4, s->w);
            sqlite3_bind_double(stmt, 5, s->h);
            sqlite3_bind_text(stmt, 6, s->title.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, s->text.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 8, static_cast<int>(s->color));
            sqlite3_bind_int(stmt, 9, s->z);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }


    // Insert vectors
    {
        const char* insV = "INSERT INTO vectors (id, x, y, scale, title, svg) VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* vst = nullptr;
        if (sqlite3_prepare_v2(db, insV, -1, &vst, nullptr) == SQLITE_OK) {
            for (const auto* v : graph.vectors()) {
                sqlite3_bind_int64(vst, 1, static_cast<sqlite3_int64>(v->id));
                sqlite3_bind_double(vst, 2, v->x);
                sqlite3_bind_double(vst, 3, v->y);
                sqlite3_bind_double(vst, 4, v->scale);
                sqlite3_bind_text(vst, 5, v->title.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(vst, 6, v->svg.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(vst);
                sqlite3_reset(vst);
            }
            sqlite3_finalize(vst);
        }
    }

    // Insert edges
    const char* insEdge = "INSERT INTO edges (id, \"from\", \"to\", color) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, insEdge, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }

    for (const auto* e : graph.edges()) {
        sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(e->id));
        sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(e->from));
        sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(e->to));
        sqlite3_bind_int(stmt, 4, static_cast<int>(e->color));
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    // Store next IDs
    char buf[64];
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)graph.nextNodeId());
    sqlite3_exec(db, ("INSERT OR REPLACE INTO meta (key,value) VALUES ('next_node_id','" + std::string(buf) + "');").c_str(),
                 nullptr, nullptr, nullptr);
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)graph.nextEdgeId());
    sqlite3_exec(db, ("INSERT OR REPLACE INTO meta (key,value) VALUES ('next_edge_id','" + std::string(buf) + "');").c_str(),
                 nullptr, nullptr, nullptr);
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)graph.nextSquareId());
    sqlite3_exec(db, ("INSERT OR REPLACE INTO meta (key,value) VALUES ('next_square_id','" + std::string(buf) + "');").c_str(),
                 nullptr, nullptr, nullptr);
    snprintf(buf, sizeof(buf), "%u", graph.defaultNodeColor());
    sqlite3_exec(db, ("INSERT OR REPLACE INTO meta (key,value) VALUES ('def_node_color','" + std::string(buf) + "');").c_str(),
                 nullptr, nullptr, nullptr);
    snprintf(buf, sizeof(buf), "%u", graph.defaultSquareColor());
    sqlite3_exec(db, ("INSERT OR REPLACE INTO meta (key,value) VALUES ('def_square_color','" + std::string(buf) + "');").c_str(),
                 nullptr, nullptr, nullptr);
    snprintf(buf, sizeof(buf), "%u", graph.edgeColor());
    sqlite3_exec(db, ("INSERT OR REPLACE INTO meta (key,value) VALUES ('edge_color','" + std::string(buf) + "');").c_str(),
                 nullptr, nullptr, nullptr);

    sqlite3_close(db);
    return true;
}

bool Storage::readSqlite(Graph& graph, const std::string& dbPath) {
    sqlite3* db = nullptr;
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        return false;
    }

    graph.clear();

    // Load nodes
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT id, x, y, title, text, color FROM nodes;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint64_t id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
            double x = sqlite3_column_double(stmt, 1);
            double y = sqlite3_column_double(stmt, 2);
            const char* title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* text  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            uint32_t color = static_cast<uint32_t>(sqlite3_column_int(stmt, 5));

            Node& n = graph.addNodeWithId(id, x, y, title ? title : "");
            n.text = text ? text : "";
            n.color = color;
        }
        sqlite3_finalize(stmt);
    }

        // Load squares
    if (sqlite3_prepare_v2(db, "SELECT id, x, y, w, h, title, text, color, z FROM squares;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint64_t id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
            double x = sqlite3_column_double(stmt, 1);
            double y = sqlite3_column_double(stmt, 2);
            double w = sqlite3_column_double(stmt, 3);
            double h = sqlite3_column_double(stmt, 4);
            const char* title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            const char* text  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            uint32_t color = static_cast<uint32_t>(sqlite3_column_int(stmt, 7));
            int z = sqlite3_column_int(stmt, 8);
            Square& s = graph.addSquareWithId(id, x, y, w, h, title ? title : "");
            s.text = text ? text : "";
            s.color = color;
            if (z < -10) z = -10;
            if (z > 10) z = 10;
            s.z = z;
        }
        sqlite3_finalize(stmt);
    }

    // Load vectors
    if (sqlite3_prepare_v2(db, "SELECT id, x, y, scale, title, svg FROM vectors;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint64_t id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
            double x = sqlite3_column_double(stmt, 1);
            double y = sqlite3_column_double(stmt, 2);
            float sc = static_cast<float>(sqlite3_column_double(stmt, 3));
            const char* title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            const char* svg = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            graph.addVectorWithId(id, x, y, sc, title ? title : "", svg ? svg : "");
        }
        sqlite3_finalize(stmt);
    }

    // Load edges
    if (sqlite3_prepare_v2(db, "SELECT id, \"from\", \"to\", color FROM edges;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            uint64_t from = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
            uint64_t to   = static_cast<uint64_t>(sqlite3_column_int64(stmt, 2));
            uint32_t col  = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
            if (Edge* e = graph.addEdge(from, to))
                e->color = col;
        }
        sqlite3_finalize(stmt);
    }

    // Defaults from meta
    if (sqlite3_prepare_v2(db, "SELECT key, value FROM meta;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (!key || !val) continue;
            if (strcmp(key, "def_node_color") == 0)
                graph.setDefaultNodeColor(static_cast<uint32_t>(strtoul(val, nullptr, 10)));
            else if (strcmp(key, "def_square_color") == 0)
                graph.setDefaultSquareColor(static_cast<uint32_t>(strtoul(val, nullptr, 10)));
            else if (strcmp(key, "edge_color") == 0)
                graph.setEdgeColor(static_cast<uint32_t>(strtoul(val, nullptr, 10)));
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    return true;
}

bool Storage::compressFile(const std::string& inPath, const std::string& outPath) {
    std::ifstream in(inPath, std::ios::binary);
    if (!in) return false;

    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<char> src(size);
    in.read(src.data(), size);
    in.close();

    size_t bound = ZSTD_compressBound(size);
    std::vector<char> dst(bound);
    size_t compressed = ZSTD_compress(dst.data(), bound, src.data(), size, 3);
    if (ZSTD_isError(compressed)) return false;

    std::ofstream out(outPath, std::ios::binary);
    if (!out) return false;
    out.write(dst.data(), compressed);
    return true;
}

bool Storage::decompressFile(const std::string& inPath, const std::string& outPath) {
    std::ifstream in(inPath, std::ios::binary);
    if (!in) return false;

    in.seekg(0, std::ios::end);
    size_t size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<char> src(size);
    in.read(src.data(), size);
    in.close();

    unsigned long long const rSize = ZSTD_getFrameContentSize(src.data(), size);
    if (rSize == ZSTD_CONTENTSIZE_ERROR || rSize == ZSTD_CONTENTSIZE_UNKNOWN) return false;

    std::vector<char> dst(rSize);
    size_t decompressed = ZSTD_decompress(dst.data(), rSize, src.data(), size);
    if (ZSTD_isError(decompressed)) return false;

    std::ofstream out(outPath, std::ios::binary);
    if (!out) return false;
    out.write(dst.data(), decompressed);
    return true;
}

bool Storage::save(const Graph& graph, const std::string& path) {
    std::string tmpDb = path + ".tmp.db";
    if (!writeSqlite(graph, tmpDb)) return false;
    bool ok = compressFile(tmpDb, path);
    fs::remove(tmpDb);
    return ok;
}

bool Storage::load(Graph& graph, const std::string& path) {
    std::string tmpDb = path + ".tmp.db";
    if (!decompressFile(path, tmpDb)) return false;
    bool ok = readSqlite(graph, tmpDb);
    fs::remove(tmpDb);
    return ok;
}
