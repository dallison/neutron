load("@bazel_skylib//lib:paths.bzl", "paths")

MessageInfo = provider(fields = ["messages"])

def _davros_serdes_action(
        ctx,
        srcs,
        out_dir,
        package_name,
        imports,
        other_srcs,
        outputs,
        add_namespace):
    inputs = depset(direct = srcs, transitive = [depset(imports + other_srcs)])
    davros_args = ["--ros", "--out={}/{}/serdes".format(out_dir, package_name), "--runtime_path=", "--msg_path={}".format(package_name)]
    if add_namespace:
        davros_args.append("--add_namespace=" + add_namespace)
    if imports:
        imports_arg = "--imports="
        sep = ""
        for file in imports:
            imports_arg += sep + paths.dirname(paths.dirname(file.path))
            sep = ","
        davros_args.append(imports_arg)

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

def _davros_serdes_impl(ctx):
    out_dir = ctx.bin_dir.path
    imports = []
    for dep in ctx.attr.deps:
        if MessageInfo in dep:
            for msg in dep[MessageInfo].messages:
                imports.append(msg)

    srcs = ctx.files.srcs
    output_files = []
    for file in srcs:
        outputs = []

        # file is something like std_msgs/msg/Header.msg
        filename = paths.basename(file.path)
        dir = paths.join("serdes", paths.basename(paths.dirname(paths.dirname(file.path))))
        
        filename = paths.replace_extension(filename, ".cc")
        cc_out = ctx.actions.declare_file(paths.join(dir, filename))
        outputs.append(cc_out)

        filename = paths.replace_extension(filename, ".h")
        h_out = ctx.actions.declare_file(paths.join(dir, filename))
        outputs.append(h_out)

        output_files.append(cc_out)
        output_files.append(h_out)

        _davros_serdes_action(
            ctx,
            [file],
            out_dir,
            ctx.attr.package_name,
            imports,
            srcs,
            outputs,
            ctx.attr.add_namespace,
        )

    return [DefaultInfo(files = depset(output_files + srcs)), MessageInfo(messages = srcs + imports)]

_davros_serdes_gen = rule(
    attrs = {
        "davros_exe": attr.label(
            executable = True,
            default = Label(":davros"),
            cfg = "exec",
        ),
        "srcs": attr.label_list(allow_files = [".msg"]),
        "deps": attr.label_list(
        ),
        "package_name": attr.string(),
        "add_namespace": attr.string(),
    },
    implementation = _davros_serdes_impl,
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

def davros_serdes_library(name, srcs = [], deps = [], runtime = "@davros//davros:serdes_runtime", add_namespace = ""):
    """
    Generate a cc_libary for ROS messages specified in srcs.

    Args:
        name: name
        srcs: source .msg file
        deps: dependencies
        runtime: label for serdes runtime.
        add_namespace: add namespace to the message types
    """
    davros = name + "_davros_serdes"
    davros_deps = []
    for d in deps:
        davros_deps.append(d + "_davros_serdes")

    _davros_serdes_gen(
        name = davros,
        srcs = srcs,
        deps = deps + davros_deps,
        package_name = native.package_name(),
        add_namespace = add_namespace,
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

    libdeps = deps
    if runtime != "":
        libdeps = libdeps + [runtime]

    native.cc_library(
        name = name,
        srcs = [srcs],
        hdrs = [hdrs],
        deps = libdeps,
    )
