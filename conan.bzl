def _impl(repository_ctx):
  # Create a symlink to the src conanfile.
  repository_ctx.symlink(repository_ctx.attr.conanfile, "conanfile.py")

  # Install the conanfile. This will create the necessary BUILD file.
  result = repository_ctx.execute(["conan", "install", "."])

conan_repository = repository_rule(
  implementation=_impl,
  attrs = {
    "conanfile": attr.label(mandatory = True, allow_single_file = True)
  },
  local = True,
)