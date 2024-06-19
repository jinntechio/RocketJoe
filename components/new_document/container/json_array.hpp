#pragma once

#include <components/new_document/base.hpp>

namespace components::new_document::json {

    class json_array {
        using iterator = std::pmr::vector<boost::intrusive_ptr<json_trie_node>>::iterator;
        using const_iterator = std::pmr::vector<boost::intrusive_ptr<json_trie_node>>::const_iterator;

    public:
        using allocator_type = std::pmr::memory_resource;

        explicit json_array(allocator_type* allocator) noexcept;

        ~json_array() = default;

        json_array(json_array&&) noexcept = default;

        json_array(const json_array&) = delete;

        json_array& operator=(json_array&&) noexcept = default;

        json_array& operator=(const json_array&) = delete;

        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;

        const json_trie_node* get(uint32_t index) const;

        void set(uint32_t index, json_trie_node* value);

        void set(uint32_t index, boost::intrusive_ptr<json_trie_node>&& value);

        boost::intrusive_ptr<json_trie_node> remove(uint32_t index);

        uint32_t size() const noexcept;

        json_array* make_deep_copy() const;

        std::pmr::string to_json(std::pmr::string (*)(const immutable_part*, std::pmr::memory_resource*),
                                 std::pmr::string (*)(const mutable_part*, std::pmr::memory_resource*)) const;

        std::pmr::string to_binary(std::pmr::string (*)(const immutable_part*, std::pmr::memory_resource*),
                                   std::pmr::string (*)(const mutable_part*, std::pmr::memory_resource*)) const;

        bool equals(const json_array& other,
                    bool (*)(const immutable_part*, const immutable_part*),
                    bool (*)(const mutable_part*, const mutable_part*),
                    bool (*)(const immutable_part*, const mutable_part*)) const;

    private:
        std::pmr::vector<boost::intrusive_ptr<json_trie_node>> items_;
    };

} // namespace components::new_document::json