//
// Created by titto on 2022/5/6.
//

#ifndef MOJO_IMPL_PROTOCOL_H
#define MOJO_IMPL_PROTOCOL_H

#include <memory>
#include <sstream>
#include <string>

#include "core/byte_array.h"

namespace tit {
namespace mojo {

// private protocol for mojo ipc
// 1st byte is magic number
// 2nd byte is version number
// 3rd byte is message type
// 4 bytes from 4th to 8th is content length
// Other bytes is data
class Protocol {
 public:
  using Ptr = std::shared_ptr<Protocol>;

  static constexpr uint8_t kMagic = 0xcc;
  static constexpr uint8_t kDefaultVersion = 0x01;
  static constexpr uint8_t kBaseLen = 7;

  enum class MsgType : uint8_t {
    kNormal,
    kUpgradeOffer,
    kUpgradeAccept,
    kUpgradeReject
  };

  static Protocol::Ptr Create(uint8_t type, const std::string& content) {
    Protocol::Ptr proto = std::make_shared<Protocol>();
    proto->set_msg_type(type);
    proto->set_content(content);
    return proto;
  }

  uint8_t magic() const { return magic_; }
  uint8_t version() const { return version_; }
  MsgType msg_type() const { return static_cast<MsgType>(type_); }
  uint32_t content_length() const { return content_length_; }
  const std::string& content() const { return data_; }

  void set_magic(uint8_t magic) { magic_ = magic; }
  void set_version(uint8_t version) { version_ = version; }
  void set_msg_type(uint8_t type) { type_ = static_cast<uint8_t>(type); }
  void set_content_length(uint32_t len) { content_length_ = len; }
  void set_content(const std::string& content) { data_ = content; }

  ByteArray::Ptr EncodeMeta() {
    ByteArray::Ptr bt = std::make_shared<ByteArray>();
    bt->writeFuint8(magic_);
    bt->writeFuint8(version_);
    bt->writeFuint8(type_);
    bt->writeFuint32(data_.size());
    bt->setPosition(0);
    return bt;
  }

  ByteArray::Ptr Encode() {
    ByteArray::Ptr bt = std::make_shared<ByteArray>();
    bt->writeFuint8(magic_);
    bt->writeFuint8(version_);
    bt->writeFuint8(type_);
    bt->writeStringF32(data_);
    bt->setPosition(0);
    return bt;
  }

  void DecodeMeta(ByteArray::Ptr bt) {
    magic_ = bt->readFuint8();
    version_ = bt->readFuint8();
    type_ = bt->readFuint8();
    content_length_ = bt->readFuint32();
  }

  void Decode(ByteArray::Ptr bt) {
    magic_ = bt->readFuint8();
    version_ = bt->readFuint8();
    type_ = bt->readFuint8();
    data_ = bt->readStringF32();
    content_length_ = data_.size();
  }

  std::string ToString() {
    std::stringstream ss;
    ss << "[ magic=" << magic_ << " version=" << version_
       << " type=" << type_ << " length=" << content_length_
       << " content=" << data_ << " ]";
    return ss.str();
  }

 private:
  uint8_t magic_ { kMagic };
  uint8_t version_ { kDefaultVersion };
  uint8_t type_ {static_cast<uint8_t>(MsgType::kNormal)};
  uint32_t content_length_ { 0 };
  std::string data_;
};

}  // namespace mojo
}  // namespace tit

#endif //MOJO_IMPL_PROTOCOL_H
