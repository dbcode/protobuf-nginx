#include <ngx_generator.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateFieldDescriptor(const FieldDescriptor *field,
                                   io::Printer& printer)
{
  std::string type(TypedefRoot(field->containing_type()->full_name()));
  std::string name;
  
  if (field->is_extension()) {
    name = ExtensionFieldName(field);
  } else {
    name = field->name();
  }

  std::map<std::string, std::string> vars;

  vars["type"] = type;
  vars["name"] = name;
  vars["fname"] = field->full_name();
  vars["fnum"] = Number(field->number());
  vars["ftype"] = FieldRealType(field);
  vars["wire"] = WireType(field);
  vars["label"] = Label(field);
  vars["tname"] = Type(field);

  if (field->is_packable() && field->options().packed()) {
    vars["packed"] = "1";
  } else {
    vars["packed"] = "0";
  }

  if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
    std::string froot(TypedefRoot(field->message_type()->full_name()));

    vars["pack"] = froot + "__pack";
    vars["unpack"] = froot + "__unpack";
    vars["size"] = froot + "__size";
  } else {
    vars["pack"] = "NULL";
    vars["unpack"] = "NULL";
    vars["size"] = "NULL";
  }

  printer.Print(vars,
                "ngx_protobuf_field_descriptor_t\n"
                "$type$_$name$ = {\n");
  Indent(printer);
  printer.Print(vars,
                "ngx_string(\"$name$\"),\n"
                "$fnum$,\n"
                "$wire$,\n"
                "$label$,\n"
                "$tname$,\n"
                "sizeof($ftype$),\n"
                "$packed$,\n"
                "(ngx_protobuf_pack_pt)\n"
                "$pack$,\n"
                "(ngx_protobuf_unpack_pt)\n"
                "$unpack$,\n"
                "(ngx_protobuf_size_pt)\n"
                "$size$\n");
  Outdent(printer);
  printer.Print("};\n"
                "\n");
}

void
Generator::GenerateDescriptors(const Descriptor *desc,
                               io::Printer& printer)
{
  for (int i = 0; i < desc->nested_type_count(); ++i) {
    GenerateDescriptors(desc->nested_type(i), printer);
  }

  printer.Print("/* $type$ field descriptors */\n"
                "\n",
                "type", desc->full_name());

  for (int i = 0; i < desc->field_count(); ++i) {
    GenerateFieldDescriptor(desc->field(i), printer);
  }

  if (desc->extension_count() > 0) {
    printer.Print("/* $type$ extension field descriptors */\n"
                  "\n",
                  "type", desc->full_name());

    for (int i = 0; i < desc->extension_count(); ++i) {
      GenerateFieldDescriptor(desc->extension(i), printer);
    }
  }

  // TODO: generate message descriptor
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
