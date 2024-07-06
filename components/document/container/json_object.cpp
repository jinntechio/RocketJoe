#include "json_object.hpp"
#include <components/document/json_trie_node.hpp>

namespace components::document::json {

    bool operator==(const boost::intrusive_ptr<json_trie_node>& lhs, const std::string_view& rhs) {
        if (lhs->is_immut()) {
            return lhs->get_immut()->get_string().value() == rhs;
        } else if (lhs->is_mut()) {
            return lhs->get_mut()->get_string().value() == rhs;
        } else {
            return false;
        }
    }

    bool json_trie_node_less::operator()(const boost::intrusive_ptr<json_trie_node>& lhs,
                                         const std::string_view& rhs) const noexcept {
        if (lhs->is_immut()) {
            return lhs->get_immut()->get_string().value() < rhs;
        } else if (lhs->is_mut()) {
            return lhs->get_mut()->get_string().value() < rhs;
        } else {
            return false;
        }
    }
    bool json_trie_node_less::operator()(const std::string_view& lhs,
                                         const boost::intrusive_ptr<json_trie_node>& rhs) const noexcept {
        if (rhs->is_immut()) {
            return lhs < rhs->get_immut()->get_string().value();
        } else if (rhs->is_mut()) {
            return lhs < rhs->get_mut()->get_string().value();
        } else {
            return false;
        }
    }
    bool json_trie_node_less::operator()(const boost::intrusive_ptr<json_trie_node>& lhs,
                                         const boost::intrusive_ptr<json_trie_node>& rhs) const noexcept {
        if (rhs->is_immut()) {
            return json_trie_node_less()(lhs, rhs->get_immut()->get_string().value());
        } else if (rhs->is_mut()) {
            return json_trie_node_less()(lhs, rhs->get_mut()->get_string().value());
        } else {
            return false;
        }
    }

    json_object::json_object(json_object::allocator_type* allocator)
        : resource_(allocator) {}

    json_object::iterator json_object::begin() { return map_.begin(); }

    json_object::iterator json_object::end() { return map_.end(); }

    json_object::const_iterator json_object::begin() const { return map_.begin(); }

    json_object::const_iterator json_object::end() const { return map_.end(); }

    json_object::const_iterator json_object::cbegin() const { return map_.cbegin(); }

    json_object::const_iterator json_object::cend() const { return map_.cend(); }

    const json_trie_node* json_object::get(std::string_view key) const {
        auto res = std::lower_bound(map_.begin(), map_.end(), key, [](const auto& lhs, const std::string_view& rhs) {
            return json_trie_node_less()(lhs.first, rhs);
        });
        if (res == map_.end() || !(res->first == key)) {
            return nullptr;
        }
        return res->second.get();
    }

    void json_object::set(json_trie_node* key, json_trie_node* value) {
        map_.emplace(boost::intrusive_ptr<json_trie_node>(key), boost::intrusive_ptr<json_trie_node>(value));
    }
    void json_object::set(std::string_view key, json_trie_node* value) {
        auto res = std::lower_bound(map_.begin(), map_.end(), key, [](const auto& lhs, const std::string_view& rhs) {
            return json_trie_node_less()(lhs.first, rhs);
        });
        if (res == map_.end() || !(res->first == key)) {
            return;
        }
        map_.at(res->first) = boost::intrusive_ptr<json_trie_node>(value);
    }

    void json_object::set(boost::intrusive_ptr<json_trie_node>&& key, boost::intrusive_ptr<json_trie_node>&& value) {
        map_.emplace(std::move(key), std::move(value));
    }
    void json_object::set(std::string_view key, boost::intrusive_ptr<json_trie_node>&& value) {
        auto res = std::lower_bound(map_.begin(), map_.end(), key, [](const auto& lhs, const std::string_view& rhs) {
            return json_trie_node_less()(lhs.first, rhs);
        });
        if (res == map_.end() || !(res->first == key)) {
            return;
        }
        map_.at(res->first) = std::move(value);
    }

    boost::intrusive_ptr<json_trie_node> json_object::remove(std::string_view key) {
        auto found = std::lower_bound(map_.begin(), map_.end(), key, [](const auto& lhs, const std::string_view& rhs) {
            return json_trie_node_less()(lhs.first, rhs);
        });
        if (found == map_.end() || !(found->first == key)) {
            return nullptr;
        }
        auto copy = found->second;
        map_.erase(found);
        return copy;
    }

    bool json_object::contains(std::string_view key) const noexcept {
        auto res = std::lower_bound(map_.begin(), map_.end(), key, [](const auto& lhs, const std::string_view& rhs) {
            return json_trie_node_less()(lhs.first, rhs);
        });
        if (res == map_.end()) {
            return false;
        }
        if (res->first->is_immut()) {
            return res->first->get_immut()->get_string() == key;
        }
        return res->first->get_mut()->get_string() == key;
    }

    size_t json_object::size() const noexcept { return map_.size(); }

    json_object* json_object::make_deep_copy() const {
        auto copy = new (resource_->allocate(sizeof(json_object))) json_object(resource_);
        for (auto& it : map_) {
            copy->map_.emplace(it.first, it.second->make_deep_copy());
        }
        return copy;
    }

    std::pmr::string
    json_object::to_json(std::pmr::string (*to_json_immut)(const immutable_part*, std::pmr::memory_resource*),
                         std::pmr::string (*to_json_mut)(const mutable_part*, std::pmr::memory_resource*)) const {
        std::pmr::string res(resource_);
        res.append("{");
        for (auto& it : map_) {
            auto key = it.first;
            if (res.size() > 1) {
                res.append(",");
            }
            res.append(key->to_json(to_json_immut, to_json_mut))
                .append(":")
                .append(it.second->to_json(to_json_immut, to_json_mut));
        }
        return res.append("}");
    }

    bool json_object::equals(const json_object& other,
                             bool (*immut_equals_immut)(const immutable_part*, const immutable_part*),
                             bool (*mut_equals_mut)(const mutable_part*, const mutable_part*),
                             bool (*immut_equals_mut)(const immutable_part*, const mutable_part*)) const {
        if (size() != other.size()) {
            return false;
        }
        for (auto& it : map_) {
            auto next_node2 = other.map_.find(it.first);
            if (next_node2 == other.map_.end() ||
                !it.second->equals(next_node2->second.get(), immut_equals_immut, mut_equals_mut, immut_equals_mut)) {
                return false;
            }
        }
        return true;
    }

    json_object* json_object::make_copy_except_deleter(allocator_type* allocator) const {
        auto copy = new (allocator->allocate(sizeof(json_object))) json_object(allocator);
        for (auto& it : map_) {
            if (it.second->is_deleter()) {
                continue;
            }
            copy->map_.emplace(it);
        }
        return copy;
    }

    json_object*
    json_object::merge(json_object& object1, json_object& object2, json_object::allocator_type* allocator) {
        auto res = new (allocator->allocate(sizeof(json_object))) json_object(allocator);
        for (auto& it : object2.map_) {
            if (it.second->is_deleter()) {
                continue;
            }
            auto next = object1.map_.find(it.first);
            if (next == object1.map_.end()) {
                res->map_.emplace(it);
            } else {
                res->map_.emplace(it.first,
                                  std::move(json_trie_node::merge(next->second.get(), it.second.get(), allocator)));
            }
        }
        for (auto& it : object1.map_) {
            if (object2.map_.find(it.first) == object2.map_.end()) {
                res->map_.emplace(it);
            }
        }
        return res;
    }

} // namespace components::document::json