load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "behavior_net_lib",
    srcs = [
        "behavior_net/ActionRegistry.cpp",
        "behavior_net/Config.cpp",
        "behavior_net/Place.cpp",
        "behavior_net/action_impl/TimerAction.cpp",
        "behavior_net/action_impl/HttpGetAction.cpp",
    ],
    hdrs = [
        "behavior_net/PetriNet.hpp",
        "behavior_net/Action.hpp",
        "behavior_net/ActionRegistry.hpp",
        "behavior_net/Common.hpp",
        "behavior_net/Config.hpp",
        "behavior_net/ConfigParameter.hpp",
        "behavior_net/Token.hpp",
        "behavior_net/Controller.hpp",
        "behavior_net/Place.hpp",
        "behavior_net/Transition.hpp",
        "behavior_net/ThreadPool.hpp",
        "behavior_net/Types.hpp",
        "behavior_net/action_impl/TimerAction.hpp",
        "behavior_net/action_impl/HttpGetAction.hpp",
    ] + glob(["3rd_party/**/*.hpp"]) + glob(["3rd_party/**/*.h"]),
    copts = ["-std=c++20"],
    linkopts = ["-lpthread"],
    includes = ["./"],
    visibility = ["//visibility:public"],
    alwayslink=True,
)

cc_binary(
    name = "behavior_net_app",
    srcs = ["app/main.cpp"],
    copts = ["-std=c++20"],
    deps = [
        ":behavior_net_lib",
    ],
    data = [
        "//config_samples:config_samples",
    ],
)