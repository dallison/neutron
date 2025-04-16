load("@bazel_skylib//lib:paths.bzl", "paths")

MessageInfo = provider(fields = ["messages"])

def _neutron_serdes_action(
        ctx,
        srcs,
        out_dir,
        package_name,
        imports,
        other_srcs,
        outputs,
        add_namespace,
        lang):
    inputs = depset(direct = srcs, transitive = [depset(imports + other_srcs)])
    prefix = "serdes" if lang == "c++" else "c_serdes"
    neutron_args = ["--ros", "--out={}/{}/{}".format(out_dir, package_name, prefix), "--runtime_path=", "--msg_path={}".format(package_name), "--lang=" + lang]
    if add_namespace:
        neutron_args.append("--add_namespace=" + add_namespace)
    if imports:
        imports_arg = "--imports="
        sep = ""
        for file in imports:
            imports_arg += sep + paths.dirname(paths.dirname(file.path))
            sep = ","
        neutron_args.append(imports_arg)

    args = ctx.actions.args()
    args.add_all(neutron_args)
    args.add_all(inputs)

    ctx.actions.run(
        inputs = inputs,
        executable = ctx.executable.neutron_exe,
        outputs = outputs,
        arguments = [args],
        progress_message = "Generating Neutron message files %s" % ctx.label,
        mnemonic = "Neutron",
    )

def _neutron_serdes_impl(ctx):
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
        prefix = "serdes" if ctx.attr.lang == "c++" else "c_serdes"
        dir = paths.join(prefix, paths.basename(paths.dirname(paths.dirname(file.path))))
        print(dir)
        filename = paths.replace_extension(filename, ".cc" if ctx.attr.lang == "c++" else ".c")

        cc_out = ctx.actions.declare_file(paths.join(dir, filename))
        outputs.append(cc_out)

        filename = paths.replace_extension(filename, ".h")
        h_out = ctx.actions.declare_file(paths.join(dir, filename))
        outputs.append(h_out)

        output_files.append(cc_out)
        output_files.append(h_out)

        _neutron_serdes_action(
            ctx,
            [file],
            out_dir,
            ctx.attr.package_name,
            imports,
            srcs,
            outputs,
            ctx.attr.add_namespace,
            ctx.attr.lang,
        )

    return [DefaultInfo(files = depset(output_files + srcs)), MessageInfo(messages = srcs + imports)]

_neutron_serdes_gen = rule(
    attrs = {
        "neutron_exe": attr.label(
            executable = True,
            default = Label(":neutron"),
            cfg = "exec",
        ),
        "srcs": attr.label_list(allow_files = [".msg"]),
        "deps": attr.label_list(
        ),
        "package_name": attr.string(),
        "add_namespace": attr.string(),
        "lang": attr.string(default = "c++"),
    },
    implementation = _neutron_serdes_impl,
)

def _neutron_zeros_action(
        ctx,
        srcs,
        out_dir,
        package_name,
        imports,
        other_srcs,
        outputs,
        add_namespace):
    inputs = depset(direct = srcs, transitive = [depset(imports + other_srcs)])
    neutron_args = ["--zeros", "--out={}/{}/zeros".format(out_dir, package_name), "--runtime_path=", "--msg_path={}".format(package_name)]
    if add_namespace:
        neutron_args.append("--add_namespace=" + add_namespace)

    if imports:
        imports_arg = "--imports="
        sep = ""
        for file in imports:
            imports_arg += sep + paths.dirname(paths.dirname(file.path))
            sep = ","
        neutron_args.append(imports_arg)
    args = ctx.actions.args()
    args.add_all(neutron_args)
    args.add_all(inputs)

    ctx.actions.run(
        inputs = inputs,
        executable = ctx.executable.neutron_exe,
        outputs = outputs,
        arguments = [args],
        progress_message = "Generating Neutron zeros message files %s" % ctx.label,
        mnemonic = "Neutron",
    )

def _neutron_impl(ctx):
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

        _neutron_zeros_action(
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

_neutron_zeros_gen = rule(
    attrs = {
        "neutron_exe": attr.label(
            executable = True,
            default = Label(":neutron"),
            cfg = "exec",
        ),
        "srcs": attr.label_list(allow_files = [".msg"]),
        "deps": attr.label_list(
        ),
        "package_name": attr.string(),
        "add_namespace": attr.string(),
    },
    implementation = _neutron_impl,
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

def neutron_serdes_library(name, srcs = [], deps = [], runtime = "@neutron//neutron:serdes_runtime", add_namespace = "", lang = "c++"):
    """
    Generate a cc_libary for ROS messages specified in srcs.

    Args:
        name: name
        srcs: source .msg file
        deps: dependencies
        runtime: label for serdes runtime.
        add_namespace: add namespace to the message types
        lang: language to generate (only c and c++ supported)
    """
    neutron = name + "_neutron_serdes"
    neutron_deps = []
    for d in deps:
        neutron_deps.append(d + "_neutron_serdes")

    _neutron_serdes_gen(
        name = neutron,
        srcs = srcs,
        deps = deps + neutron_deps,
        package_name = native.package_name(),
        add_namespace = add_namespace,
        lang = lang,
    )

    srcs = name + "_srcs"
    _split_files(
        name = srcs,
        ext = "cc" if lang == "c++" else "c",
        deps = [neutron],
    )

    hdrs = name + "_hdrs"
    _split_files(
        name = hdrs,
        ext = "h",
        deps = [neutron],
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

def neutron_zeros_library(name, srcs = [], deps = [], runtime = "@neutron//neutron:zeros_runtime", add_namespace = ""):
    """
    Generate a cc_libary for ROS messages specified in srcs.

    Args:
        name: name
        srcs: source .msg file
        deps: dependencies
        runtime: label for zeros runtime.
        add_namespace: add given namespace to the message output
    """
    neutron = name + "_neutron_zeros"
    neutron_deps = []
    for d in deps:
        neutron_deps.append(d + "_neutron_zeros")

    _neutron_zeros_gen(
        name = neutron,
        srcs = srcs,
        deps = deps + neutron_deps,
        package_name = native.package_name(),
        add_namespace = add_namespace,
    )

    srcs = name + "_srcs"
    _split_files(
        name = srcs,
        ext = "cc",
        deps = [neutron],
    )

    hdrs = name + "_hdrs"
    _split_files(
        name = hdrs,
        ext = "h",
        deps = [neutron],
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