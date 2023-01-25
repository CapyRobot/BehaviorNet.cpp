load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "behavior_net_lib",
    srcs = [
        "petri_net/PetriNet.cpp"
    ],
    hdrs = [
        "petri_net/PetriNet.hpp",
        "petri_net/Common.hpp",
        "petri_net/Config.hpp",
        "petri_net/Token.hpp",
        "petri_net/Place.hpp",
        "petri_net/Transition.hpp",
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