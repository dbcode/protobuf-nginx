// -*-c++-*-

#ifndef NGX_GENERATOR_H_
#define NGX_GENERATOR_H_

#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/compiler/code_generator.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

class Generator : public CodeGenerator {
public:
  Generator() {}
  virtual ~Generator() {}

private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Generator);

public:
  // ngx_generate.cc
  virtual bool Generate(const FileDescriptor* file,
                        const std::string& parameter,
                        OutputDirectory* output_directory,
                        std::string* error) const;

private:
  // ngx_add.cc
  static void GenerateAdd(const Descriptor* desc,
                          io::Printer& printer);

  // ngx_descriptor.cc
  static void GenerateFieldDescriptor(const FieldDescriptor *field,
                                      io::Printer& printer);
  static void GenerateDescriptors(const Descriptor *desc,
                                  io::Printer& printer);

  // ngx_descriptor_util.cc
  static bool HasUnknownFields(const Descriptor *desc);
  static bool HasExtensionFields(const Descriptor *desc);

  // ngx_extension.cc
  static void GenerateExtendeeDecls(const Descriptor *desc,
                                    io::Printer& printer);
  static void GenerateExtendeeMethods(const Descriptor *desc,
                                      io::Printer& printer);
  static void GenerateExtensionFieldDecls(const FieldDescriptor *field,
                                          io::Printer& printer);
  static void GenerateExtensionFieldMethods(const FieldDescriptor *field,
                                            io::Printer& printer);
  static void GenerateExtensionMethods(const Descriptor *desc,
                                       io::Printer& printer);

  // ngx_field_util.cc
  static std::string ExtensionSetterType(const FieldDescriptor *field,
                                         int *primitive);
  static std::string ExtensionFieldName(const FieldDescriptor *field);
  static std::string FieldTypeName(const FieldDescriptor *field);
  static std::string FieldRealType(const FieldDescriptor *field);
  static std::string WireType(const FieldDescriptor *field);
  static std::string Label(const FieldDescriptor *field);
  static std::string Type(const FieldDescriptor *field);
  static bool IsFixedWidth(const FieldDescriptor *field);
  static bool FieldIsPointer(const FieldDescriptor *field); 

  // ngx_is_initialized.cc
  static void GenerateIsInitialized(const Descriptor *desc,
				    io::Printer& printer);

  // ngx_methods.cc
  static void GenerateMethodDecls(const Descriptor* desc,
                                  io::Printer& printer);
  static void GenerateMethods(const Descriptor* desc,
                              io::Printer& printer);

  // ngx_module.cc
  static void GenerateRegisterExtension(const FieldDescriptor *extension,
                                        io::Printer& printer);
  static void GenerateRegisterMessageExtensions(const Descriptor* desc,
                                                io::Printer& printer);
  static int CountMessageExtensions(const Descriptor *desc);
  static int CountExtensions(const FileDescriptor *file);
  static void GenerateModule(const FileDescriptor *file,
                             io::Printer& printer);

  // ngx_name.cc
  static std::string FileRoot(const std::string& input);
  static std::string IncludeGuard(const std::string& input);
  static std::string BareRoot(const std::string& input);
  static std::string TypedefRoot(const std::string& input);
  static std::string EnumType(const std::string& input);
  static std::string EnumValue(const std::string& input);
  static std::string StructType(const std::string& input);

  // ngx_pack.cc
  static void GeneratePackField(const FieldDescriptor *field,
                                io::Printer& printer);
  static void GeneratePackRange(const Descriptor::ExtensionRange *range,
                                io::Printer& printer);
  static void GeneratePackUnknown(const Descriptor *desc,
				  io::Printer& printer);
  static void GeneratePack(const Descriptor* desc, io::Printer& printer);

  // ngx_print.cc
  static void Indented(io::Printer& printer,
                       const std::map<std::string, std::string>& vars,
                       const char *body);
  static void OpenBrace(io::Printer& printer);
  static void CloseBrace(io::Printer& printer);
  static void SimpleIf(io::Printer& printer,
                       const std::map<std::string, std::string>& vars,
                       const char *cond);
  static void CuddledIf(io::Printer& printer,
                        const std::map<std::string, std::string>& vars,
                        const char *cond1,
                        const char *cond2);
  static void FullSimpleIf(io::Printer& printer,
                           const std::map<std::string, std::string>& vars,
                           const char *cond,
                           const char *body);
  static void FullCuddledIf(io::Printer& printer,
                            const std::map<std::string, std::string>& vars,
                            const char *cond1,
                            const char *cond2,
                            const char *body);

  static void Else(io::Printer& printer);
  static void SkipUnknown(io::Printer& printer);
  static void BracedReturn(io::Printer& printer,
                           const char *value);
  static std::string Spaces(int count);
  static std::string Number(int number);
  static void Indent(io::Printer& printer);
  static void Outdent(io::Printer& printer);

  // ngx_size.cc
  static void GenerateSize(const Descriptor* desc,
                           io::Printer& printer);

  // ngx_typedef.cc
  static void GenerateEnumDecl(const EnumDescriptor* desc,
                               io::Printer& printer);
  static void GenerateTypedef(const Descriptor* desc,
                              io::Printer& printer);

  // ngx_unpack.cc
  static void GenerateUnpackUnknown(const Descriptor *desc,
				    io::Printer& printer);
  static void GenerateUnpack(const Descriptor* desc,
                             io::Printer& printer);
};

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google

#endif // NGX_GENERATOR_H_
