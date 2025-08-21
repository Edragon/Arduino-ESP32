

# write-clock.md

esptool --chip esp32 --port COM21 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0xe000 "boot_app0.bin" 0x1000 "DAOLEDCLOCK-SPFF.ino.bootloader.bin" 0x10000 "DAOLEDCLOCK-SPFF.2023-5-2-v6.2.bin" 0x8000 "DAOLEDCLOCK-SPFF.ino.partitions.bin"


- [[]]