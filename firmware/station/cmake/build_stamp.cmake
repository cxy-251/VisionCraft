# 每次编译都重新生成 build_stamp.c，让固件里的「编译时间」真的是最近一次编译的时间
string(TIMESTAMP stamp "%b %d %Y %H:%M:%S")
file(WRITE "${OUT}" "/* 由 cmake/build_stamp.cmake 在每次编译时生成，不要手改 */\nconst char app_build_stamp[] = \"${stamp}\";\n")
