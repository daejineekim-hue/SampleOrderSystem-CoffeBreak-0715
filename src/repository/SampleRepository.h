#pragma once

#include <string>
#include <vector>

#include "model/Sample.h"

namespace sos::repository {

// JSON-file-backed CRUD for Sample entities (docs/FEATURES/sample-management.md).
// Loads the whole file into memory on construction; every mutating call writes
// the full list back to disk immediately (same pattern as the DataPersistence PoC).
class SampleRepository {
public:
    explicit SampleRepository(std::string filePath);

    // Throws std::invalid_argument if the sample fails field validation
    // (Sample::validateFields) or if its id is already registered.
    const model::Sample& create(const model::Sample& sample);

    std::vector<model::Sample> findAll() const;

    // Substring match (case-sensitive) against id or name. Empty keyword
    // matches everything.
    std::vector<model::Sample> search(const std::string& keyword) const;

    // Not part of the sample-management feature's public operations, but
    // needed so other features (order-approval/production-line) and this
    // feature's own "list reflects latest stock" acceptance criterion have a
    // way to adjust stock after registration. Returns false if id not found.
    bool updateStock(const std::string& id, int newStock);

private:
    std::string filePath_;
    std::vector<model::Sample> samples_;

    void load();
    void save() const;
};

}  // namespace sos::repository
