#include <ngx_generator.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

std::string
Generator::ExtensionSetterType(const FieldDescriptor *field, int *primitive)
{
  std::string type;

  if (field->is_repeated()) {
    // repeated fields can be set from an array of the right type.
    // it is the caller's responsibility to ensure that the contents
    // of the array are legal for the type.
    type = "ngx_array_t *";
    *primitive = 0;
  } else if (field->type() == FieldDescriptor::TYPE_MESSAGE ||
             field->type() == FieldDescriptor::TYPE_BYTES ||
             field->type() == FieldDescriptor::TYPE_STRING) {
    type = FieldRealType(field) + " *";
    *primitive = 0;
  } else {
    type = FieldRealType(field);
    *primitive = 1;
  }

  return type;
}

std::string
Generator::ExtensionFieldName(const FieldDescriptor *field)
{
  std::string name;

  if (field->extension_scope() != NULL) {
    std::string package(field->containing_type()->file()->package());

    name = BareRoot(field->extension_scope()->full_name());
    if (!package.empty() &&
        package == field->extension_scope()->file()->package()) {
      // strip the package from the name
      std::string prefix(BareRoot(package) + "_");

      if (prefix.compare(0, prefix.length(), name) == 0) {
        name = name.substr(prefix.length());
      }
    }    
    name += "_";
  }
  name += field->name();

  return name;
}

std::string
Generator::FieldTypeName(const FieldDescriptor *field)
{
  std::string type;

  if (field->is_repeated()) {
    type = "ngx_array_t";
  } else {
    type = FieldRealType(field);
  }

  return type;
}

std::string
Generator::FieldRealType(const FieldDescriptor *field)
{
  std::string type;

  switch (field->type()) {
  case FieldDescriptor::TYPE_BOOL:     type = "ngx_flag_t"; break;
  case FieldDescriptor::TYPE_UINT32:
  case FieldDescriptor::TYPE_FIXED32:  type = "uint32_t";   break;
  case FieldDescriptor::TYPE_INT32:
  case FieldDescriptor::TYPE_SINT32:
  case FieldDescriptor::TYPE_SFIXED32: type = "int32_t";    break;
  case FieldDescriptor::TYPE_UINT64:
  case FieldDescriptor::TYPE_FIXED64:  type = "uint64_t";   break;
  case FieldDescriptor::TYPE_INT64:
  case FieldDescriptor::TYPE_SINT64:
  case FieldDescriptor::TYPE_SFIXED64: type = "int64_t";    break;
  case FieldDescriptor::TYPE_FLOAT:    type = "float";      break;
  case FieldDescriptor::TYPE_DOUBLE:   type = "double";     break;
  case FieldDescriptor::TYPE_STRING:
  case FieldDescriptor::TYPE_BYTES:    type = "ngx_str_t";  break;
  case FieldDescriptor::TYPE_ENUM:
    type = EnumType(field->enum_type()->full_name());
    break;
  case FieldDescriptor::TYPE_MESSAGE:
    type = StructType(field->message_type()->full_name());
    break;
  default:
    type = "FIXME";
    break;
  }

  return type;
}

std::string
Generator::WireType(const FieldDescriptor *field)
{
  std::string type("NGX_PROTOBUF_WIRETYPE_");

  switch (field->type()) {
  case FieldDescriptor::TYPE_INT64:
  case FieldDescriptor::TYPE_UINT64:
  case FieldDescriptor::TYPE_INT32:
  case FieldDescriptor::TYPE_BOOL:
  case FieldDescriptor::TYPE_UINT32:
  case FieldDescriptor::TYPE_ENUM:
  case FieldDescriptor::TYPE_SINT32:
  case FieldDescriptor::TYPE_SINT64:   type += "VARINT";           break;
  case FieldDescriptor::TYPE_DOUBLE:
  case FieldDescriptor::TYPE_FIXED64:
  case FieldDescriptor::TYPE_SFIXED64: type += "FIXED64";          break;
  case FieldDescriptor::TYPE_STRING:
  case FieldDescriptor::TYPE_MESSAGE:
  case FieldDescriptor::TYPE_BYTES:    type += "LENGTH_DELIMITED"; break;
  case FieldDescriptor::TYPE_FLOAT:
  case FieldDescriptor::TYPE_FIXED32:
  case FieldDescriptor::TYPE_SFIXED32: type += "FIXED32";          break;
  default:                             type += "FIXME";            break;
  }

  return type;
}

std::string
Generator::Label(const FieldDescriptor *field)
{
  std::string label("NGX_PROTOBUF_LABEL_");

  switch (field->label()) {
  case FieldDescriptor::LABEL_REQUIRED: label += "REQUIRED"; break;
  case FieldDescriptor::LABEL_OPTIONAL: label += "OPTIONAL"; break;
  case FieldDescriptor::LABEL_REPEATED: label += "REPEATED"; break;
  default:                              label += "FIXME";    break;
  }

  return label;
}

std::string
Generator::Type(const FieldDescriptor *field)
{
  std::string type("NGX_PROTOBUF_TYPE_");

  switch (field->type()) {
  case FieldDescriptor::TYPE_DOUBLE:   type += "DOUBLE";   break;
  case FieldDescriptor::TYPE_FLOAT:    type += "FLOAT";    break;
  case FieldDescriptor::TYPE_INT64:    type += "INT64";    break;
  case FieldDescriptor::TYPE_UINT64:   type += "UINT64";   break;
  case FieldDescriptor::TYPE_INT32:    type += "INT32";    break;
  case FieldDescriptor::TYPE_FIXED64:  type += "FIXED64";  break;
  case FieldDescriptor::TYPE_FIXED32:  type += "FIXED32";  break;
  case FieldDescriptor::TYPE_BOOL:     type += "BOOL";     break;
  case FieldDescriptor::TYPE_STRING:   type += "STRING";   break;
  case FieldDescriptor::TYPE_MESSAGE:  type += "MESSAGE";  break;
  case FieldDescriptor::TYPE_BYTES:    type += "BYTES";    break;
  case FieldDescriptor::TYPE_UINT32:   type += "UINT32";   break;
  case FieldDescriptor::TYPE_ENUM:     type += "ENUM";     break;
  case FieldDescriptor::TYPE_SFIXED32: type += "SFIXED32"; break;
  case FieldDescriptor::TYPE_SFIXED64: type += "SFIXED64"; break;
  case FieldDescriptor::TYPE_SINT32:   type += "SINT32";   break;
  case FieldDescriptor::TYPE_SINT64:   type += "SINT64";   break;
  default:                             type += "FIXME";    break;
  }

  return type;
}

bool
Generator::IsFixedWidth(const FieldDescriptor *field)
{
  bool fixed = false;

  switch (field->type()) {
  case FieldDescriptor::TYPE_FIXED32:
  case FieldDescriptor::TYPE_SFIXED32:
  case FieldDescriptor::TYPE_FLOAT:
  case FieldDescriptor::TYPE_FIXED64:
  case FieldDescriptor::TYPE_SFIXED64:
  case FieldDescriptor::TYPE_DOUBLE:
    fixed = true;
    break;
  default:
    break;
  }

  return fixed;
}

bool
Generator::FieldIsPointer(const FieldDescriptor *field)
{
  if (field->is_repeated()) {
    return true;
  } else if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
    return true;
  }

  return false;
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
