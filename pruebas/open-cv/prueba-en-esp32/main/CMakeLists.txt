idf_component_register(
    SRCS "src/main.cpp" "src/CameraVision.cpp" "src/Network.cpp"
    INCLUDE_DIRS "include" "./opencv"
    REQUIRES espressif__esp32-camera nvs_flash esp_http_server esp_wifi
)

# Agregar librerías precompiladas de OpenCV
add_prebuilt_library(opencv_imgcodecs "opencv/libopencv_imgcodecs.a")
add_prebuilt_library(libpng "opencv/3rdparty/liblibpng.a")
add_prebuilt_library(libzlib "opencv/3rdparty/libzlib.a")
add_prebuilt_library(opencv_imgproc "opencv/libopencv_imgproc.a")
add_prebuilt_library(opencv_core "opencv/libopencv_core.a")
add_prebuilt_library(ade "opencv/libade.a")

# Dependencias para imgcodecs
set(IMGCODEC_DEP libpng libzlib)
target_link_libraries(opencv_imgcodecs INTERFACE ${IMGCODEC_DEP})

# Dependencias para core
set(CORE_DEP libzlib)
target_link_libraries(opencv_core INTERFACE ${CORE_DEP})

# Enlazar todas las librerías OpenCV al componente
set(OPENCV_DEP opencv_imgcodecs opencv_imgproc opencv_core ade)
target_link_libraries(${COMPONENT_LIB} ${OPENCV_DEP})
