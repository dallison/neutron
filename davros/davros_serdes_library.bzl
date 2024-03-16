load("@bazel_skylib//lib:paths.bzl", "paths")

def _davros_action(
        ctx,
        srcs,
        out_dir,
        package_name,
        outputs):
    inputs = depset(direct = srcs)
    davros_args = ["--out={}/{}".format(out_dir, package_name), "--runtime_path=", "--msg_path={}".format(package_name)]

    args = ctx.actions.args()
    args.add_all(davros_args)
    args.add_all(inputs)

    ctx.actions.run(
        inputs = inputs,
        executable = ctx.executable.davros_exe,
        outputs = outputs,
        arguments = [args],
        progress_message = "Generating Davros message files %s" % ctx.label,
        mnemonic = "Davros",
    )

def _davros_impl(ctx):
    out_dir = ctx.bin_dir.path

    srcs = ctx.files.srcs
    output_files = []
    for file in srcs:
        outputs = []

        # file is something like std_msgs/msg/Header.msg
        filename = paths.basename(file.path)
        dir = paths.basename(paths.dirname(paths.dirname(file.path)))

        filename = paths.replace_extension(filename, ".cc")
        cc_out = ctx.actions.declare_file(paths.join(dir, filename))
        outputs.append(cc_out)

        filename = paths.replace_extension(filename, ".h")
        h_out = ctx.actions.declare_file(paths.join(dir, filename))
        outputs.append(h_out)

        output_files.append(cc_out)
        output_files.append(h_out)

        _davros_action(
            ctx,
            [file],
            out_dir,
            ctx.attr.package_name,
            outputs,
        )

    return [DefaultInfo(files = depset(output_files))]

_davros_gen = rule(
    attrs = {
        "davros_exe": attr.label(
            executable = True,
            default = Label(":davros"),
            cfg = "exec",
        ),
        "srcs": attr.label_list(allow_files = [".msg"]),
        "deps": attr.label_list(),
        "package_name": attr.string(),
    },
    implementation = _davros_impl,
)

def _split_files_impl(ctx):
    files = []
    for file in ctx.files.deps:
        if file.extension == ctx.attr.ext:
            files.append(file)

    return [DefaultInfo(files = depset(files))]

_split_files = rule(
    attrs = {
        "deps": attr.label_list(mandatory = True),
        "ext": attr.string(mandatory = True),
    },
    implementation = _split_files_impl,
)

def davros_serdes_library(name, srcs = [], deps = [], runtime = "@davros//davros:serdes_runtime"):
    """
    Generate a cc_libary for ROS messages specified in srcs.

    Args:
        name: name
        srcs: source .msg file
        deps: dependencies
        runtime: label for serdes runtime.
    """
    davros = name + "_davros"
    _davros_gen(
        name = davros,
        srcs = srcs,
        deps = deps,
        package_name = native.package_name(),
    )

    srcs = name + "_srcs"
    _split_files(
        name = srcs,
        ext = "cc",
        deps = [davros],
    )

    hdrs = name + "_hdrs"
    _split_files(
        name = hdrs,
        ext = "h",
        deps = [davros],
    )

    native.cc_library(
        name = name,
        srcs = [srcs],
        hdrs = [hdrs],
        deps = deps + [runtime],
    )
