//
// Created by frp on 2026/9/1.
//

#include <random>


#include "HLVCDecoder.hpp"

std::string generate_alphanumeric(size_t length) {
    static const char charset[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static std::random_device rd;
    thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, sizeof(charset) - 2);

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(charset[dist(gen)]);
    }
    return std::move(result);
}

int main(int argc, char* argv[]) {

    auto v_obj = std::make_shared<hlvc::VerifAGL_CRC16>();

    uint16_t frame_count = 0;

    uint16_t fix_length = 1000;
    while (true) {
        auto message = generate_alphanumeric(fix_length);
        v_obj->Reset();
        auto verif_value = v_obj->UpdateCalculate(reinterpret_cast<uint8_t*>(&fix_length), 2);
        verif_value = v_obj->UpdateCalculate(reinterpret_cast<uint8_t*>(message.data()), message.size());

        std::array<uint8_t, 4> head_message{0x5A, 0xA5, static_cast<uint8_t>(fix_length&0xFF), static_cast<uint8_t>(fix_length>>8)};

        hlvc::HLVCDecoder<uint16_t, 2, 2> hlvcDecoder({0x5A, 0xA5}, 1024, std::make_shared<hlvc::VerifAGL_CRC16>());
        hlvcDecoder.SetFrameCallback([&frame_count](const uint8_t* frame, size_t length) {
           printf("decode frame length %d, frame count %d\n", length, frame_count++);
        });
        hlvcDecoder.SyncDecodeRawData(head_message.data(), head_message.size());
        hlvcDecoder.SyncDecodeRawData(reinterpret_cast<uint8_t*>(message.data()), message.size());
        hlvcDecoder.SyncDecodeRawData(verif_value.data(), verif_value.size());
    }



    // hlvcDecoder.SyncInputDataStream(reinterpret_cast<const uint8_t*>(raw_data.data()), raw_data.size());



    return 0;
}