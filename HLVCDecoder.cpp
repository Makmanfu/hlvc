//
// Created by frp on 2026/8/27.
//

#include "HLVCDecoder.hpp"

hlvc::VerifAGL_CRC16::VerifAGL_CRC16() {
    VerifAGL_CRC16::Reset();
}

void hlvc::VerifAGL_CRC16::Reset() {
    verif_value_->at(0) = static_cast<uint8_t>(init_value_ & 0xFF);
    verif_value_->at(1) = static_cast<uint8_t>((init_value_ >> 8) & 0xFF);
}

hlvc::VerifAGLInterface<2>::verif_type hlvc::VerifAGL_CRC16::UpdateCalculate(const uint8_t* data, int length) {
    uint16_t &update_value = reinterpret_cast<uint16_t*>(verif_value_.get())[0];

    for (int i = 0; i < length; i++) {
        uint8_t tmp = data[i] ^ reinterpret_cast<uint8_t*>(&update_value)[0];
        tmp ^= (tmp << 4);
        update_value = (update_value >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4);
    }

    return *verif_value_;
}
