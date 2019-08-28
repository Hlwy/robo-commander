cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 11)

if(VERBOSE)
message(STATUS "------ BUILDING Tests")
endif(VERBOSE)

# foreach(dir ${EXTERNAL_PACKAGE_LIBRARIES})
#      message(STATUS "         included='${dir}'")
# endforeach()

# Utilities
add_executable(test_utils ${TEST_DIR}/utilities/test_utils.cpp)
target_link_libraries(test_utils PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
set(RoboCommander_TESTS test_utils)

# # JSON Test
# add_executable(test_json ${TEST_DIR}/utils/test_json.cpp)
# target_link_libraries(test_json utils)
# list(APPEND RoboCommander_TESTS test_json)

# Serial
add_executable(test_serial ${TEST_DIR}/communication/test_serial.cpp)
target_link_libraries(test_serial PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_serial)

# UDP
add_executable(test_udp ${TEST_DIR}/communication/test_udp.cpp)
target_link_libraries(test_udp PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_udp)

# I2C
add_executable(test_i2c ${TEST_DIR}/communication/test_i2c.cpp)
target_link_libraries(test_i2c PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_i2c)

# UART
add_executable(test_uart ${TEST_DIR}/communication/test_uart.cpp)
target_link_libraries(test_uart PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_uart)

# DC Motor
add_executable(test_dcmotor ${TEST_DIR}/actuators/test_dc.cpp)
target_link_libraries(test_dcmotor PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_dcmotor)

# Servo
add_executable(test_servo ${TEST_DIR}/actuators/test_servo.cpp)
target_link_libraries(test_servo PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_servo)

# Encoder
add_executable(test_encoder ${TEST_DIR}/sensors/test_encoder.cpp)
target_link_libraries(test_encoder PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_encoder)

# IMU
add_executable(test_imu ${TEST_DIR}/sensors/test_imu.cpp)
target_link_libraries(test_imu PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_imu)

# MPU-9250
add_executable(test_mpu9250 ${TEST_DIR}/sensors/test_mpu9250.cpp)
target_link_libraries(test_mpu9250 PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_mpu9250)

# # Camera
# add_executable(test_camera ${TEST_DIR}/sensors/test_camera.cpp)
# target_link_libraries(test_camera camera utils)
# list(APPEND RoboCommander_TESTS test_camera)

# BNO-055
add_executable(test_bno055 ${TEST_DIR}/sensors/test_bno055.cpp)
target_link_libraries(test_bno055 PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_bno055)

# EKF
add_executable(test_ekf ${TEST_DIR}/algorithms/test_ekf.cpp)
target_link_libraries(test_ekf PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_ekf)

# ISAM2
# add_executable(test_isam ${TEST_DIR}/algorithms/test_isam.cpp)
# target_link_libraries(test_isam PUBLIC ${RoboCommander_LIBRARIES})
# list(APPEND RoboCommander_TESTS test_isam)

# PID
add_executable(test_pid ${TEST_DIR}/controllers/test_pid.cpp)
target_link_libraries(test_pid PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_pid)

# Pure Pursuit
add_executable(test_purepursuit ${TEST_DIR}/controllers/test_waypoint_follower.cpp)
target_link_libraries(test_purepursuit PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_purepursuit)

# RoboClaw
add_executable(test_roboclaw ${TEST_DIR}/devices/test_roboclaw.cpp)
target_link_libraries(test_roboclaw PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_roboclaw)

# PWM Driver - PCA-9685
add_executable(test_pca9685 ${TEST_DIR}/devices/test_pca9685.cpp)
target_link_libraries(test_pca9685 PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_pca9685)

# Camera Gimbal Angle Controller
add_executable(test_camera_gimbal ${TEST_DIR}/controllers/test_camera_gimbal.cpp)
target_link_libraries(test_camera_gimbal PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_camera_gimbal)

# LED Color Test
add_executable(test_leds ${TEST_DIR}/devices/test_leds.cpp)
target_link_libraries(test_leds PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_leds)

# Camera Gimbal Assembly Test
add_executable(test_camera_gimbal_assembly ${TEST_DIR}/devices/test_camera_gimbal_assembly.cpp)
target_link_libraries(test_camera_gimbal_assembly PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_camera_gimbal_assembly)

# DualClaw
add_executable(test_swansonV2 ${TEST_DIR}/robots/test_swansonV2.cpp)
target_link_libraries(test_swansonV2 PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_swansonV2)

# # 4WD
# add_executable(test_4WD ${TEST_DIR}/profiles/test_4wd.cpp)
# target_link_libraries(test_4WD 4WD)
# list(APPEND RoboCommander_TESTS test_4WD)

# DualClaw
add_executable(test_dualclaw ${TEST_DIR}/vehicle_profiles/test_dual_roboclaw.cpp)
target_link_libraries(test_dualclaw PUBLIC ${RoboCommander_LIBRARIES} ${RoboCommander_EXTERNAL_LIBRARIES})
list(APPEND RoboCommander_TESTS test_dualclaw)

# ===========================
#          INSTALL
# ===========================
message(STATUS " Tests to install ------")
foreach(dir ${RoboCommander_TESTS})
     message(STATUS "         included='${dir}'")
endforeach()

install(TARGETS ${RoboCommander_TESTS} EXPORT RoboCommander_TARGETS DESTINATION "${EXEC_DEST_DIR}")