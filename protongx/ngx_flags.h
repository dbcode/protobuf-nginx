// -*-c++-*-

#ifndef NGX_FLAGS_H_
#define NGX_FLAGS_H_

#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

class Flags {
public:
  explicit Flags(const Descriptor *desc);
  ~Flags() {}

public:
  bool has_required() const { return has_required_; }
  bool has_array() const { return has_array_; }
  bool has_message() const { return has_message_; }
  bool has_packed() const { return has_packed_; }
  bool has_string() const { return has_string_; }
  bool has_bool() const { return has_bool_; }
  bool has_float() const { return has_float_; }
  bool has_double() const { return has_double_; }
  bool has_int32() const { return has_int32_; }
  bool has_int64() const { return has_int64_; }
  bool has_repnm() const { return has_repnm_; }

private:
  bool has_required_;
  bool has_array_;
  bool has_message_;
  bool has_packed_;
  bool has_string_;
  bool has_bool_;
  bool has_float_;
  bool has_double_;
  bool has_int32_;
  bool has_int64_;
  bool has_repnm_;
};

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google

#endif // NGX_FLAGS_H_
