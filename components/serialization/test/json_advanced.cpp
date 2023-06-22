#include <catch2/catch.hpp>

#include <map>
#include <vector>

#include <components/serialization/serialization.hpp>
#include <components/serialization/stream/json.hpp>

using components::serialization::serialize;
using components::serialization::stream::stream_json;

/*
    template<typename InputType>
    bool from_msgpack(InputType&& i ) ;

    template<typename IteratorType>
    bool from_msgpack(IteratorType first, IteratorType last) ;

    std::vector<std::uint8_t> to_msgpack(const document_t& j);
    void to_msgpack(const document_t& j, output_adapter<std::uint8_t> o);
    void to_msgpack(const document_t& j, output_adapter<char> o);
*/
template<class T>
class output_adapter {

};

template<class C,class O>
bool to_json(C& c,output_adapter<O>) {
    stream_json ar;
    serialize(ar, c);
    return true;
}

class no_dto_t {
public:
    no_dto_t() = default;

    void serialize(stream_json& ar) {
        ::serialize(ar, example_0_);
        ::serialize(ar, example_1_);
        ::serialize(ar, example_2_);
        ::serialize(ar, example_3_);
    }

private:
    std::uint64_t example_0_;
    std::string example_1_;
    std::vector<uint64_t> example_2_;
    std::map<std::string_view, uint64_t> example_3_;
};

struct dto_t {
    std::uint64_t example_0_;
    std::string example_1_;
    std::vector<uint64_t> example_2_;
    std::map<std::string_view, uint64_t> example_3_;
};

void serialize(stream_json& ar, dto_t& dto) {
    serialize(ar, dto.example_0_);
    serialize(ar, dto.example_1_);
    serialize(ar, dto.example_2_);
    serialize(ar, dto.example_3_);
}

TEST_CASE("big_example_json 1") {
    no_dto_t no_dto;
    stream_json flat_stream;
    no_dto.serialize(flat_stream);
}

TEST_CASE("big_example_json 2") {
    dto_t dto;
    stream_json flat_stream;
    serialize(flat_stream, dto);
}

TEST_CASE("big_example_json 3") {
    dto_t dto;
    to_json(dto, output_adapter<char>{});

}