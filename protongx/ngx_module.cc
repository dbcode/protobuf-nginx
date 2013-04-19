#include <ngx_generator.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

void
Generator::GenerateRegisterExtension(const FieldDescriptor *extension,
                                     io::Printer& printer)
{
  std::string type(TypedefRoot(extension->containing_type()->full_name()));
  std::string name(ExtensionFieldName(extension));

  printer.Print("if ($type$__import_extension(\n", "type", type);
  Indent(printer);
  printer.Print("&$type$_$name$, cycle) != NGX_OK) {\n",
                "type", type, "name", name);
  printer.Print("return NGX_ERROR;\n");
  CloseBrace(printer);
}

void
Generator::GenerateRegisterMessageExtensions(const Descriptor* desc,
                                             io::Printer& printer)
{
  for (int i = 0; i < desc->nested_type_count(); ++i) {
    GenerateRegisterMessageExtensions(desc->nested_type(i), printer);
  }
  
  for (int i = 0; i < desc->extension_count(); ++i) {
    GenerateRegisterExtension(desc->extension(i), printer);
  }
}

int
Generator::CountMessageExtensions(const Descriptor *desc)
{
  int count = 0;

  for (int i = 0; i < desc->nested_type_count(); ++i) {
    count += CountMessageExtensions(desc->nested_type(i));
  }

  return count + desc->extension_count();
}

int
Generator::CountExtensions(const FileDescriptor *file)
{
  int count = 0;

  for (int i = 0; i < file->message_type_count(); ++i) {
    count += CountMessageExtensions(file->message_type(i));
  }

  return count + file->extension_count();
}

void
Generator::GenerateModule(const FileDescriptor *file, io::Printer& printer)
{
  std::string root(FileRoot(file->name()));
  int ecount = CountExtensions(file);

  if (ecount > 0) {
    printer.Print("static ngx_int_t\n"
                  "$root$__register_extensions(ngx_cycle_t *cycle)\n"
                  "{\n",
                  "root", root);
    Indent(printer);

    for (int i = 0; i < file->message_type_count(); ++i) {
      GenerateRegisterMessageExtensions(file->message_type(i), printer);
    }
  
    for (int i = 0; i < file->extension_count(); ++i) {
      GenerateRegisterExtension(file->extension(i), printer);
    }

    printer.Print("\n"
                  "return NGX_OK;\n");
    Outdent(printer);
    printer.Print("}\n"
                  "\n");

    printer.Print("static ngx_protobuf_module_t\n"
                  "$root$_module_ctx = {\n",
                  "root", root);
    Indent(printer);
    printer.Print("$root$__register_extensions\n",
                  "root", root);
    Outdent(printer);
    printer.Print("};\n"
                  "\n");
  } else {
    printer.Print("static ngx_protobuf_module_t\n"
                  "$root$_module_ctx = {\n",
                  "root", root);
    Indent(printer);
    printer.Print("NULL\n");
    Outdent(printer);
    printer.Print("};\n"
                  "\n");
  }

  printer.Print("ngx_module_t $root$_module = {\n",
                "root", root);
  Indent(printer);
  printer.Print("NGX_MODULE_V1,\n"
                "&$root$_module_ctx,\n"
                "NULL,\n"
                "NGX_PROTOBUF_MODULE,\n"
                "NULL,\n"
                "NULL,\n"
                "NULL,\n"
                "NULL,\n"
                "NULL,\n"
                "NULL,\n"
                "NULL,\n"
                "NGX_MODULE_V1_PADDING\n",
                "root", root);
  Outdent(printer);
  printer.Print("};\n"
                "\n");
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
