#include "repository/SampleRepository.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace sos::repository {

using json::JsonValue;
using model::Sample;

SampleRepository::SampleRepository(std::string filePath) : filePath_(std::move(filePath)) {
    load();
}

void SampleRepository::load() {
    samples_.clear();
    std::ifstream in(filePath_);
    if (!in.is_open()) return;

    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string text = buffer.str();
    if (text.empty()) return;

    JsonValue root = JsonValue::parse(text);
    for (const auto& item : root.items()) {
        samples_.push_back(Sample::fromJson(item));
    }
}

void SampleRepository::save() const {
    JsonValue root = JsonValue::makeArray();
    for (const auto& sample : samples_) {
        root.push_back(sample.toJson());
    }
    std::ofstream out(filePath_, std::ios::trunc);
    out << root.dump();
}

const Sample& SampleRepository::create(const Sample& sample) {
    Sample::validateFields(sample);

    bool duplicate = std::any_of(samples_.begin(), samples_.end(),
                                  [&](const Sample& s) { return s.id == sample.id; });
    if (duplicate) {
        throw std::invalid_argument("Sample id already registered: " + sample.id);
    }

    samples_.push_back(sample);
    save();
    return samples_.back();
}

std::vector<Sample> SampleRepository::findAll() const { return samples_; }

std::vector<Sample> SampleRepository::search(const std::string& keyword) const {
    std::vector<Sample> result;
    std::copy_if(samples_.begin(), samples_.end(), std::back_inserter(result),
                 [&](const Sample& s) {
                     return s.id.find(keyword) != std::string::npos ||
                            s.name.find(keyword) != std::string::npos;
                 });
    return result;
}

bool SampleRepository::updateStock(const std::string& id, int newStock) {
    auto it = std::find_if(samples_.begin(), samples_.end(),
                            [&](const Sample& s) { return s.id == id; });
    if (it == samples_.end()) return false;
    it->stock = newStock;
    save();
    return true;
}

}  // namespace sos::repository
