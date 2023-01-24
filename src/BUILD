load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "behavior_net_lib",
    srcs = [
        "behavior_net/BehaviorNet.cpp"
    ],
    hdrs = [
        "behavior_net/BehaviorNet.hpp",
        "behavior_net/Common.hpp",
        "behavior_net/Config.hpp",
        "behavior_net/Token.hpp",
        "behavior_net/Place.hpp",
        "behavior_net/Transition.hpp",
        "3rd_party/nlohmann/json.hpp",
    ],
    copts = ["-std=c++20"],
    includes = ["./"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "behavior_net_app",
    srcs = ["app/main.cpp"],
    copts = ["-std=c++20"],
    deps = [
        ":behavior_net_lib",
    ],
)