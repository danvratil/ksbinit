set(ksbinit_SRCS
    main.cpp
    launcher.cpp
    proxyowner.cpp
    sandboxproxy.cpp
    worldproxy.cpp
    utils.cpp
    stdinreader.cpp
)

add_executable(ksbinit ${ksbinit_SRCS})
target_link_libraries(ksbinit
    Qt5::Core
    Qt5::DBus
)


install(TARGETS ksbinit ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
