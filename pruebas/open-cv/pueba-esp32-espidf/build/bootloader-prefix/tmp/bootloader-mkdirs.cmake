# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/taci/esp/esp-idf/components/bootloader/subproject"
  "/home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/pruebas/open-cv/pueba-esp32-espidf/build/bootloader"
  "/home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/pruebas/open-cv/pueba-esp32-espidf/build/bootloader-prefix"
  "/home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/pruebas/open-cv/pueba-esp32-espidf/build/bootloader-prefix/tmp"
  "/home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/pruebas/open-cv/pueba-esp32-espidf/build/bootloader-prefix/src/bootloader-stamp"
  "/home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/pruebas/open-cv/pueba-esp32-espidf/build/bootloader-prefix/src"
  "/home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/pruebas/open-cv/pueba-esp32-espidf/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/pruebas/open-cv/pueba-esp32-espidf/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/taci/unlp/taller-de-proyecto-ii/2025-A1-AutoDobleCamara/pruebas/open-cv/pueba-esp32-espidf/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
