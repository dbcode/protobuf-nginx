#include <ngx_generator.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace nginx {

bool
Generator::HasUnknownFields(const Descriptor *desc)
{
  return (desc->file()->options().optimize_for() != FileOptions::LITE_RUNTIME);
}

bool
Generator::HasExtensionFields(const Descriptor *desc)
{
  for (int i = 0; i < desc->field_count(); ++i) {
    if (desc->field(i)->is_extension()) {
      return true;
    }
  }

  return false;
}

} // namespace nginx
} // namespace compiler
} // namespace protobuf
} // namespace google
