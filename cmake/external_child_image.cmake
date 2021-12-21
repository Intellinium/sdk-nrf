if (CONFIG_BUILD_EXTERNAL_CHILD_IMAGE)
    if (NOT CONFIG_CHILD_IMAGE_NAME)
        message(FATAL_ERROR "CONFIG_CHILD_IMAGE_NAME not set")
    endif()

    if (NOT CONFIG_CHILD_IMAGE_PATH)
        message(FATAL_ERROR "CONFIG_CHILD_IMAGE_PATH not set")
    endif()

    # allow the use of cmake variables in kconfig or prj.conf
    string(CONFIGURE ${CONFIG_CHILD_IMAGE_PATH} CHILD_IMAGE_PATH)

    message("Adding child image ${CONFIG_CHILD_IMAGE_NAME}\n"
    "located at ${CHILD_IMAGE_PATH}\n"
    "since CONFIG_BUILD_EXTERNAL_CHILD_IMAGE is set to 'y'")

    if (${CONFIG_CHILD_IMAGE_DOMAIN} STREQUAL "CPUNET")
        set(CHILD_IMAGE_BOARD_TARGET ${CONFIG_DOMAIN_CPUNET_BOARD})
    elseif (${CONFIG_CHILD_IMAGE_DOMAIN} STREQUAL "CPUAPP")
        set(CHILD_IMAGE_BOARD_TARGET ${CONFIG_DOMAIN_CPUAPP_BOARD})
    else()
        message(FATAL_ERROR "CHILD_IMAGE_DOMAIN not set to CPUNET or CPUAPP")
    endif()

    add_child_image(
    NAME ${CONFIG_CHILD_IMAGE_NAME}
    SOURCE_DIR ${CHILD_IMAGE_PATH}
    DOMAIN ${CONFIG_CHILD_IMAGE_DOMAIN}
    BOARD ${CHILD_IMAGE_BOARD_TARGET})
endif()
