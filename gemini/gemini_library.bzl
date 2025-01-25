load("@bazel_skylib//lib:paths.bzl", "paths")

MessageInfo = provider(fields = ["messages"])

def _gemini_serdes_action(
        ctx,
        srcs,
        out_dir,
        package_name,
        imports,
        other_srcs,
        outputs,
        add_namespace):
    inputs = depset(direct = srcs, transitive = [depset(imports + other_srcs)])
    gemini_args = ["--ros", "--out={}/{}/serdes".format(out_dir, package_name), "--runtime_path=", "--msg_path={}".format(package_name)]
    if add_namespace:
        gemini_args.append("--add_namespace=" + add_namespace)
    if imports:
        imports_arg = "--imports="
        sep = ""
        for file in imports:
            imports_arg += sep + paths.dirname(paths.dirname(file.path))
            sep = ","
        gemini_args.append(imports_arg)

    args = ctx.actions.args()
    args.add_all(gemini_args)
    args.add_all(inputs)

    ctx.actions.run(
        inputs = inputs,
        executable = ctx.executable.gemini_exe,
        outputs = outputs,
        arguments = [args],
        progress_message = "Generating Gemini message files %s" % ctx.label,
        mnemonic = "Gemini",
    )

def _gemini_serdes_impl(ctx):
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

        _gemini_serdes_action(
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

_gemini_serdes_gen = rule(
    attrs = {
        "gemini_exe": attr.label(
            executable = True,
            default = Label(":gemini"),
            cfg = "exec",
        ),
        "srcs": attr.label_list(allow_files = [".msg"]),
        "deps": attr.label_list(
        ),
        "package_name": attr.string(),
        "add_namespace": attr.string(),
    },
    implementation = _gemini_serdes_impl,
)

def _gemini_zeros_action(
        ctx,
        srcs,
        out_dir,
        package_name,
        imports,
        other_srcs,
        outputs,
        add_namespace):
    inputs = depset(direct = srcs, transitive = [depset(imports + other_srcs)])
    gemini_args = ["--zeros", "--out={}/{}/zeros".format(out_dir, package_name), "--runtime_path=", "--msg_path={}".format(package_name)]
    if add_namespace:
        gemini_args.append("--add_namespace=" + add_namespace)

    if imports:
        imports_arg = "--imports="
        sep = ""
        for file in imports:
            imports_arg += sep + paths.dirname(paths.dirname(file.path))
            sep = ","
        gemini_args.append(imports_arg)
    args = ctx.actions.args()
    args.add_all(gemini_args)
    args.add_all(inputs)

    ctx.actions.run(
        inputs = inputs,
        executable = ctx.executable.gemini_exe,
        outputs = outputs,
        arguments = [args],
        progress_message = "Generating Gemini zeros message files %s" % ctx.label,
        mnemonic = "Gemini",
    )

def _gemini_impl(ctx):
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
        dir = paths.join("zeros", paths.basename(paths.dirname(paths.dirname(file.path))))

        filename = paths.replace_extension(filename, ".cc")
        cc_out = ctx.actions.declare_file(paths.join(dir, filename))
        outputs.append(cc_out)

        filename = paths.replace_extension(filename, ".h")
        h_out = ctx.actions.declare_file(paths.join(dir, filename))
        outputs.append(h_out)

        output_files.append(cc_out)
        output_files.append(h_out)

        _gemini_zeros_action(
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

_gemini_zeros_gen = rule(
    attrs = {
        "gemini_exe": attr.label(
            executable = True,
            default = Label(":gemini"),
            cfg = "exec",
        ),
        "srcs": attr.label_list(allow_files = [".msg"]),
        "deps": attr.label_list(
        ),
        "package_name": attr.string(),
        "add_namespace": attr.string(),
    },
    implementation = _gemini_impl,
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

def gemini_serdes_library(name, srcs = [], deps = [], runtime = "@gemini//gemini:serdes_runtime", add_namespace = ""):
    """
    Generate a cc_libary for ROS messages specified in srcs.

    Args:
        name: name
        srcs: source .msg file
        deps: dependencies
        runtime: label for serdes runtime.
        add_namespace: add namespace to the message types
    """
    gemini = name + "_gemini_serdes"
    gemini_deps = []
    for d in deps:
        gemini_deps.append(d + "_gemini_serdes")

    _gemini_serdes_gen(
        name = gemini,
        srcs = srcs,
        deps = deps + gemini_deps,
        package_name = native.package_name(),
        add_namespace = add_namespace,
    )

    srcs = name + "_srcs"
    _split_files(
        name = srcs,
        ext = "cc",
        deps = [gemini],
    )

    hdrs = name + "_hdrs"
    _split_files(
        name = hdrs,
        ext = "h",
        deps = [gemini],
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

def gemini_zeros_library(name, srcs = [], deps = [], runtime = "@gemini//gemini:zeros_runtime", add_namespace = ""):
    """
    Generate a cc_libary for ROS messages specified in srcs.

    Args:
        name: name
        srcs: source .msg file
        deps: dependencies
        runtime: label for zeros runtime.
        add_namespace: add given namespace to the message output
    """
    gemini = name + "_gemini_zeros"
    gemini_deps = []
    for d in deps:
        gemini_deps.append(d + "_gemini_zeros")

    _gemini_zeros_gen(
        name = gemini,
        srcs = srcs,
        deps = deps + gemini_deps,
        package_name = native.package_name(),
        add_namespace = add_namespace,
    )

    srcs = name + "_srcs"
    _split_files(
        name = srcs,
        ext = "cc",
        deps = [gemini],
    )

    hdrs = name + "_hdrs"
    _split_files(
        name = hdrs,
        ext = "h",
        deps = [gemini],
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