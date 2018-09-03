def _impl(ctx):
  output_files = []
  for input_file in ctx.files.srcs:
    output_file = ctx.actions.declare_file(input_file.path)
    ctx.actions.run_shell(
      outputs = [output_file],
      inputs = [input_file],
      command = "cp {} {}".format(input_file.path, output_file.path)
    )
    output_files.append(output_file)
  return DefaultInfo(
    files = depset(output_files),
    runfiles = ctx.runfiles(files = output_files),
  )

static_files = rule(
    implementation = _impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True),
    },
)