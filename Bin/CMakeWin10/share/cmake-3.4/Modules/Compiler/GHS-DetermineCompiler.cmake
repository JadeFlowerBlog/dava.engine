set(_compiler_id_pp_test "defined(__INTEGRITY)")

set(_compiler_id_version_compute "
# define @PREFIX@COMPILER_VERSION_MAJOR @MACRO_DEC@(__INTEGRITY_MAJOR_VERSION)
# define @PREFIX@COMPILER_VERSION_MINOR @MACRO_DEC@(__INTEGRITY_MINOR_VERSION)
# define @PREFIX@COMPILER_VERSION_PATCH @MACRO_DEC@(__INTEGRITY_PATCH_VERSION)")
