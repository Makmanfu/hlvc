# HLVC Decoder

一个高效的C++协议解码器库，用于解析HLVC（Head-Length-Value-Check）格式的二进制数据流。

## 协议格式

```
|    HEAD       | length  |  PAYLOAD  | verif   |
| e.g 0x5A 0xA5 | 2Byte   |  NByte    | NByte   |
```

- **HEAD**: 固定头部，内容可指定，长度无限制（常用 `0x5A 0xA5`）
- **length**: 表示PAYLOAD数据的长度，本身长度可配置（如2Byte可表示最大65535Byte）
- **PAYLOAD**: 实际数据内容
- **verif**: 校验值（length+PAYLOAD），算法可指定，长度根据算法确定（如CRC-32使用4字节）

## 特性

- 模板化设计，支持灵活配置
- 支持自定义校验算法（已实现CRC-16）
- 流式解析，支持分段输入数据
- 回调机制处理解析完成的帧
- 内存安全，使用`memmove`处理重叠内存

## 构建

```bash
mkdir build && cd build
cmake ..
make
```

## 使用示例

```cpp
#include "HLVCDecoder.hpp"

// 创建CRC-16校验对象
auto verif_obj = std::make_shared<hlvc::VerifAGL_CRC16>();

// 创建解码器：头部2字节，长度类型uint16_t，校验2字节
hlvc::HLVCDecoder<uint16_t, 2, 2> decoder({0x5A, 0xA5}, 1024, verif_obj);

// 设置帧回调
decoder.SetFrameCallback([](const uint8_t* data, int length) {
    // 处理解析完成的帧数据
    printf("Received frame, length: %d\n", length);
});

// 输入数据流
decoder.SyncDecodeRawData(raw_data, raw_data_length);
```

## 类说明

### `HLVCDecoder<LEN_TYPE, head_bytes, verif_bytes>`

主解码器类，模板参数：
- `LEN_TYPE`: 长度字段类型（如`uint16_t`、`uint32_t`）
- `head_bytes`: 头部字节数
- `verif_bytes`: 校验字节数

### `VerifAGLInterface<verif_bytes>`

校验算法接口，需继承实现：
- `Reset()`: 重置校验状态
- `UpdateCalculate(data, length)`: 更新并计算校验值

### `VerifAGL_CRC16`

CRC-16校验算法实现

## 依赖

- C++17
- CMake 4.3+

## 许可证

MIT License