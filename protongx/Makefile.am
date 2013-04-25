bin_PROGRAMS = protongx

noinst_HEADERS = \
	ngx_flags.h \
	ngx_generator.h

protongx_SOURCES = \
	ngx_add.cc \
	ngx_descriptor.cc \
	ngx_descriptor_util.cc \
	ngx_extension.cc \
	ngx_field_util.cc \
	ngx_flags.cc \
	ngx_generate.cc \
	ngx_is_initialized.cc \
	ngx_main.cc \
	ngx_methods.cc \
	ngx_module.cc \
	ngx_name.cc \
	ngx_pack.cc \
	ngx_print.cc \
	ngx_size.cc \
	ngx_typedef.cc \
	ngx_unpack.cc

protongx_CXXFLAGS = -Wall
protongx_LDADD = -lprotobuf -lprotoc -lpthread
