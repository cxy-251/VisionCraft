add_library(vc_teaching_vision STATIC ProjectionMath.cpp ProjectionMath.h)
set_target_properties(vc_teaching_vision PROPERTIES AUTOMOC OFF AUTORCC OFF AUTOUIC OFF)
target_include_directories(vc_teaching_vision PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(vc_teaching_vision PUBLIC ${OpenCV_LIBS})
