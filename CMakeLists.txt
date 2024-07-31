# cmake version
cmake_minimum_required(VERSION 3.12)

# include the sdk.cmake file
include(pico_sdk_import.cmake)

# give the project a name (anything you want)
project(pt_testing)
#set(CMAKE_C_STANDARD 11)
#set(CMAKE_CXX_STANDARD 17)

# initialize the sdk
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()
#
set(PICO_BOARD pico_w)
#
pico_sdk_init()

#######

# name anything you want
add_executable(pt_test)

# must match with executable name and source file names
target_sources(pt_test PRIVATE 
	Protothreads_test_1_3.c
	#Protothreads_test_counts.c
)

target_include_directories(pt_test PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
)

# must match with executable name
target_link_libraries(pt_test PRIVATE 
	pico_cyw43_arch_lwip_threadsafe_background 
	pico_cyw43_arch
	
	pico_stdlib 
	pico_bootsel_via_double_reset 

	hardware_pwm
	hardware_pio 
	hardware_dma 
	hardware_adc 
	hardware_sync
	hardware_irq
	hardware_gpio
	hardware_spi
    hardware_clocks
	hardware_rtc
	pico_multicore
	pico_time
)

# must match with executable name
pico_add_extra_outputs(pt_test)

add_compile_options(-O2)

#  Choose Build or Build Solution, or press Ctrl+Shift+B  #
