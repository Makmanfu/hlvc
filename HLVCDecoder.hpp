//
// Created by frp on 2026/8/27.
//

/*******************************************************************************
    |    HEAD       | length  |  PAYLOAD  | verif |
    | e.g 0x5A 0xA5 | 2Byte   |  NByte    | NByte |
说明：
1. 本工程是为了高效解析如上协议开发的so
2. 各个部分说明且都可配置，
    HEAD：固定头，内容可指定，长度无限制。常用 0x5A 0xA5
    length：表示数据的长度，本身的长度可配置。如2Byte PAYLOAD最大可放入65535Byte
    verif：校验值(length+PAYLOAD)，算法可指定，本身长度根据算法指定。如使用CRC-32算法，设置4
*******************************************************************************/



#ifndef PROJECT1_HLVCDECODER_HPP
#define PROJECT1_HLVCDECODER_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>


namespace hlvc {

template<uint16_t verif_bytes>
class VerifAGLInterface {
public:
    using verif_type = std::array<uint8_t, verif_bytes>;
    VerifAGLInterface(){ verif_value_ = std::make_shared<verif_type>(); }
    virtual ~VerifAGLInterface() = default;
    virtual void Reset() = 0;
    virtual verif_type UpdateCalculate(const uint8_t* data, int length) = 0;
protected:
    std::shared_ptr<verif_type> verif_value_{nullptr};
};

class VerifAGL_CRC16 : public VerifAGLInterface<2> {
public:
    VerifAGL_CRC16();
    void Reset() override;
    verif_type UpdateCalculate(const uint8_t* data, int length) override ;
private:
    const uint16_t init_value_{0xFFFF};
};


template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
class HLVCDecoder {
    using IVerifAGLPtr = std::shared_ptr<VerifAGLInterface<verif_bytes>>;
public:
    /**
     * @brief 构造函数
     * @param head 固定头
     * @param max_length 协议数据中的最大长度，一次分配内存
     * @param verif_obj 校验方法，可以通过继承VerifAGL实现自己的算法
     */
    explicit HLVCDecoder(std::array<uint8_t, head_bytes> head, LEN_TYPE max_length, IVerifAGLPtr verif_obj);

    /**
     * @brief 构造函数, note!! 不设置最大长度有损性能
     * @param head 固定头
     * @param verif_obj
     */
    explicit HLVCDecoder(std::array<uint8_t, head_bytes> head, IVerifAGLPtr verif_obj);


    using FrameCallback = std::function<void(const uint8_t* data, int length)>;
    void SetFrameCallback(FrameCallback &&callback);

    void SyncDecodeRawData(const uint8_t* data, const int length);
private:
    void InitDecodeState();

    /**
     * @brief 获取当前frame的长度信息，内部在解析完长度后才有效
     * @return 当前frame的数据长度
     */
    LEN_TYPE GetFrameDataLength();

    std::array<uint8_t, verif_bytes> GetFrameVerifValue();

    void safe_memcpy(uint8_t* dest, const uint8_t* src, const size_t count) const;

private:
    std::array<uint8_t, head_bytes> head_;
    int max_data_length_{0};
    IVerifAGLPtr verif_obj_{nullptr};

    enum DECODE_STATE_E : uint8_t {IDLE = 0, HEAD, LENGTH, DATA, VERIF};
    DECODE_STATE_E decode_state_{IDLE};
    uint64_t frame_bytes_{0};
    std::vector<uint8_t> frame_data_;

    FrameCallback frame_callback_{nullptr};

};

template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>::HLVCDecoder(std::array<uint8_t, head_bytes> head, LEN_TYPE max_length,
    IVerifAGLPtr verif_obj) : head_(head), max_data_length_(max_length), verif_obj_(verif_obj)
{
    frame_data_.reserve(max_data_length_);
}

template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>::HLVCDecoder(std::array<uint8_t, head_bytes> head,
    IVerifAGLPtr verif_obj) : head_(head), verif_obj_(verif_obj)
{
}

template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
void HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>::SetFrameCallback(FrameCallback&& callback) {
    frame_callback_ = std::move(callback);
}

template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
void HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>::SyncDecodeRawData(const uint8_t* data, const int length) {
    for (int i = 0; i < length; i++) {
        uint8_t cur_byte = data[i];
        switch (decode_state_) {
            case IDLE: {
                if (cur_byte == head_.at(0)) {
                    frame_data_[frame_bytes_] = cur_byte;
                    decode_state_ = HEAD;
                    ++frame_bytes_;
                    // verif_obj_->UpdateCalculate(&cur_byte, 1);
                }
                break;
            }
            case HEAD: {
                if (frame_bytes_ < head_.size()) {
                    if (cur_byte == head_.at(frame_bytes_)) {
                        frame_data_[frame_bytes_] = cur_byte;
                        ++frame_bytes_;
                        // verif_obj_->UpdateCalculate(&cur_byte, 1);
                    }
                    else {
                        InitDecodeState();
                        --i;    //让当前字节重复一次switch
                        verif_obj_->Reset();
                    }
                }
                else {
                    frame_data_[frame_bytes_] = cur_byte;
                    ++frame_bytes_;
                    decode_state_ = LENGTH;
                    verif_obj_->UpdateCalculate(&cur_byte, 1);
                }
                break;
            }
            case LENGTH: {
                if (frame_bytes_ == head_.size() + sizeof(LEN_TYPE)) {
                    decode_state_ = DATA;
                    if (max_data_length_ != 0) {
                        if (GetFrameDataLength() > max_data_length_) {
                            InitDecodeState();
                            --i;
                            break;
                        }
                    }
                }
                frame_data_[frame_bytes_] = cur_byte;
                ++frame_bytes_;
                verif_obj_->UpdateCalculate(&cur_byte, 1);
                break;
            }
            case DATA: {
                auto need_len = GetFrameDataLength() - (frame_bytes_ - head_.size() - sizeof(LEN_TYPE));
                auto unused_len = length - i;
                if (unused_len >= need_len) {
                    // std::copy(data+i, data+i+need_len, frame_data_.begin()+frame_bytes_);
                    safe_memcpy(frame_data_.data()+frame_bytes_, data+i, need_len);
                    verif_obj_->UpdateCalculate(data+i, need_len);
                    i = i + need_len - 1;
                    frame_bytes_ += need_len;
                    decode_state_ = VERIF;
                }
                else {
                    // std::copy(data+i, data+i+unused_len, frame_data_.begin()+frame_bytes_);
                    safe_memcpy(frame_data_.data()+frame_bytes_, data+i, unused_len);
                    verif_obj_->UpdateCalculate(data+i, unused_len);
                    i += unused_len;
                    frame_bytes_ += unused_len;
                }
                break;
            }
            case VERIF: {
                auto total_len = head_.size() + sizeof(LEN_TYPE) + GetFrameDataLength() + verif_bytes;
                if (frame_bytes_ < total_len) {
                    frame_data_[frame_bytes_] = cur_byte;
                    ++frame_bytes_;
                }
                if (frame_bytes_ == total_len) {
                    auto v1 = verif_obj_->UpdateCalculate(nullptr, 0);
                    if (v1 == GetFrameVerifValue()) {
                        if (frame_callback_) frame_callback_(frame_data_.data(), frame_bytes_);
                        InitDecodeState();
                    }
                    else {
                        SyncDecodeRawData(frame_data_.data()+1, frame_bytes_-1);
                        InitDecodeState();
                    }
                }
                break;
            }
            default: {

                break;
            }
        }
    }
}

template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
void HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>::InitDecodeState() {
    decode_state_ = IDLE;
    frame_bytes_ = 0;
}

template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
LEN_TYPE HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>::GetFrameDataLength() {
    return reinterpret_cast<LEN_TYPE*>(frame_data_.data()+head_bytes)[0];
}

template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
std::array<uint8_t, verif_bytes> HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>::GetFrameVerifValue() {
    auto start_iter = frame_data_.begin() + head_.size() + sizeof(LEN_TYPE) + GetFrameDataLength();
    std::array<uint8_t, verif_bytes> result{};
    std::copy(start_iter, start_iter + verif_bytes, result.begin());
    return result;
}

template <typename LEN_TYPE, uint16_t head_bytes, uint16_t verif_bytes>
void HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>::safe_memcpy(uint8_t* dest, const uint8_t* src, const size_t count) const {
    if (src >= frame_data_.data() && src < frame_data_.data() + max_data_length_) {
        memmove(dest, src, count);
    }
    else {
        memcpy(dest, src, count);
    }
}

}

#endif //PROJECT1_HLVCDECODER_HPP
