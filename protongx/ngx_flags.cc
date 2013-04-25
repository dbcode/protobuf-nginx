// -*-c++-*-

#include <ngx_flags.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

Flags::Flags(const Descriptor *desc) :
  has_required_(false),
  has_array_(false),
  has_message_(false),
  has_packed_(false),
  has_string_(false),
  has_bool_(false),
  has_float_(false),
  has_double_(false),
  has_int32_(false),
  has_int64_(false),
  has_repnm_(false)
{
  for (int i = 0; i < desc->field_count(); ++i) {
    const FieldDescriptor *field = desc->field(i);

    if (field->is_repeated()) {
      has_array_ = true;
      if (field->type() != FieldDescriptor::TYPE_MESSAGE) {
        has_repnm_ = true;
      }
    } else if (field->is_required()) {
      has_required_ = true;
    }
    if (field->is_packable() && field->options().packed()) {
      has_packed_ = true;
    }
    switch (field->type()) {
    case FieldDescriptor::TYPE_MESSAGE:  has_message_ = true; break;
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING:   has_string_  = true; break;
    case FieldDescriptor::TYPE_BOOL:     has_bool_    = true; break;
    case FieldDescriptor::TYPE_FLOAT:    has_float_   = true; break;
    case FieldDescriptor::TYPE_DOUBLE:   has_double_  = true; break;
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SFIXED32: has_int32_   = true; break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_SFIXED64: has_int64_   = true; break;
    default:
      break;
    }
  }
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
