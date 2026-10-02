#include <iostream>
#include "aiws/processing_core.hpp"
#include "aiws/chunking_strategy.hpp"
#include "aiws/retrieval_strategy.hpp"
#include "aiws/context_strategy.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

// Student-written M2 tests
//
// Add your own tests to this file. Your tests are part of the submitted work
// and are evaluated under Student-Written Testing & Validation.
//
// Do not modify tests/public_tests.cpp.
//
// Your tests should exercise important M2 behavior beyond the supplied public
// tests. Consider default compatibility, custom strategies, runtime dispatch,
// invalid configuration, ownership/move behavior, and component interactions.

// simple custom chunker for testing
namespace {

int failures = 0;

void check(bool ok, const char* name) {
    if (!ok) {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}

// custom chunker
class OneChunk final : public aiws::ChunkingStrategy {
public:
    std::vector<aiws::Chunk> chunk(
        const aiws::Document& d,
        std::size_t order) const override {

        return {{
            d.id() + "#custom",
            d.id(),
            order,
            0,
            "custom chunk",
            2,
            0,
            d.text().size()
        }};
    }
};

// custom retrieval
class FirstOnly final : public aiws::RetrievalStrategy {
public:
    std::vector<aiws::SearchResult> search(
        const std::string&,
        int k,
        const std::vector<aiws::Chunk>& chunks,
        const aiws::CorpusIndex&) const override {

        if (k <= 0 || chunks.empty()) {
            return {};
        }

        const auto& c = chunks.front();

        return {{
            c.id,
            c.document_id,
            c.sequence,
            c.text,
            99.0,
            1
        }};
    }
};

// custom context builder
class PrefixContext final : public aiws::ContextStrategy {
public:
    std::vector<aiws::ContextItem> build(
        const std::vector<aiws::SearchResult>& ranked,
        std::size_t budget) const override {

        if (ranked.empty() || budget == 0) {
            return {};
        }

        const auto& r = ranked.front();

        return {{
            r.chunk_id,
            r.document_id,
            r.chunk_sequence,
            "custom",
            1,
            r.score,
            true
        }};
    }
};

} // namespace

int main() {
    using namespace aiws;

    // check default behavior
    Workspace ws;
    ws.add_document(Document{"a", "", "Alpha beta beta."});
    ws.add_document(Document{"b", "", "Gamma alpha."});

    ProcessingCore normal;
    normal.rebuild(ws);

    check(
        ProcessingCore::normalize("Search... SEARCH!! 42-times")
            == "search search 42 times",
        "normalization preserved"
    );

    check(
        normal.chunk_count() == 2,
        "default chunking preserved"
    );

    auto ranked = normal.search("beta", 2);

    check(
        ranked.size() == 1 &&
        ranked[0].document_id == "a",
        "default retrieval preserved"
    );

    // check custom strategies
    try {
        ProcessingCore custom(
            std::make_unique<OneChunk>(),
            std::make_unique<FirstOnly>(),
            std::make_unique<PrefixContext>()
        );

        custom.rebuild(ws);

        check(
            custom.chunk_count() == 2,
            "custom chunker invoked"
        );

        check(
            custom.chunks()[0].id == "a#custom",
            "custom chunk output retained"
        );

        auto r = custom.search("anything", 1);

        check(
            r.size() == 1 &&
            r[0].score == 99.0,
            "custom retrieval invoked"
        );

        auto c = custom.build_context("anything", 1, 5);

        check(
            c.size() == 1 &&
            c[0].text == "custom",
            "custom context invoked"
        );
    }
    catch (const std::exception& e) {
        std::cerr << "FAIL: strategy injection threw: "
                  << e.what() << '\n';
        ++failures;
    }

    // check null strategy
    bool threw = false;

    try {
        ProcessingCore bad(
            nullptr,
            std::make_unique<FirstOnly>(),
            std::make_unique<PrefixContext>()
        );
    }
    catch (const std::invalid_argument&) {
        threw = true;
    }
    catch (...) {
    }

    check(
        threw,
        "null strategy rejected"
    );

    // check move behavior
    check(
        !std::is_copy_constructible_v<ProcessingCore>,
        "processing core is not copyable"
    );

    check(
        std::is_move_constructible_v<ProcessingCore>,
        "processing core is movable"
    );

    ProcessingCore first;
    ProcessingCore second(std::move(first));

    check(
        second.chunk_count() == 0,
        "moved core still works"
    );

    // check failed rebuild keeps old data
    ProcessingCore rebuild_test;

    Workspace good;
    good.add_document(Document{"good", "", "hello world"});
    rebuild_test.rebuild(good);

    std::size_t old_count = rebuild_test.chunk_count();

    Workspace bad;
    bad.add_document(Document{"same", "", "first"});
    bad.add_document(Document{"same", "", "second"});

    threw = false;

    try {
        rebuild_test.rebuild(bad);
    }
    catch (const std::invalid_argument&) {
        threw = true;
    }

    check(
        threw,
        "duplicate document id rejected"
    );

    check(
        rebuild_test.chunk_count() == old_count,
        "failed rebuild keeps old corpus"
    );

    if (failures) {
        return 1;
    }

    std::cout << "student tests passed\n";
    return 0;
}