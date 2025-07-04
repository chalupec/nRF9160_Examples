add_custom_target(mcuboot_shared_property_target)
set_property(TARGET mcuboot_shared_property_target  PROPERTY KERNEL_HEX_NAME;zephyr.hex)
set_property(TARGET mcuboot_shared_property_target  PROPERTY ZEPHYR_BINARY_DIR;C:/ADWITECH/nRF9160_Examples/nrf_cloud_rest_fota/build/mcuboot/zephyr)
set_property(TARGET mcuboot_shared_property_target  PROPERTY KERNEL_ELF_NAME;zephyr.elf)
set_property(TARGET mcuboot_shared_property_target  PROPERTY BUILD_BYPRODUCTS;C:/ADWITECH/nRF9160_Examples/nrf_cloud_rest_fota/build/mcuboot/zephyr/zephyr.hex;C:/ADWITECH/nRF9160_Examples/nrf_cloud_rest_fota/build/mcuboot/zephyr/zephyr.elf)
set_property(TARGET mcuboot_shared_property_target  PROPERTY SIGNATURE_KEY_FILE;root-ec-p256.pem)
set_property(TARGET mcuboot_shared_property_target APPEND PROPERTY PM_YML_DEP_FILES;C:/ncs/v2.6.0/bootloader/mcuboot/boot/zephyr/pm.yml)
set_property(TARGET mcuboot_shared_property_target APPEND PROPERTY PM_YML_FILES;C:/ADWITECH/nRF9160_Examples/nrf_cloud_rest_fota/build/mcuboot/zephyr/include/generated/pm.yml)
