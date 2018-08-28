def _impl(repository_ctx):
  # Create a symlink to the src conanfile.
  repository_ctx.template(
    "conanfile.py",
    repository_ctx.attr.conanfile,
    substitutions = repository_ctx.attr.template_substitutions,
    executable = False,
  )

  # Install the conanfile. This will create the necessary BUILD file.
  result = repository_ctx.execute(["conan", "install", "."])
  if result.stderr:
    print("Failed to install conan dependency: ", repository_ctx.name)
    print(result.stderr)

custom_conan_repository = repository_rule(
  implementation=_impl,
  attrs = {
    "conanfile": attr.label(
      mandatory = True,
      allow_single_file = True,
    ),
    "template_substitutions": attr.string_dict(),
  },
  local = True,
)

def conan_repository(name, requirement):
  custom_conan_repository(
    name = name,
    conanfile = "//third_party:conanfile.template.py",
    template_substitutions = {
      "<REQUIREMENT>": requirement,
    }
  )