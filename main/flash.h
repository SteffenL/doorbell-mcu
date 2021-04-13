#pragma once

typedef enum { NVS_FLASH_STATUS_OK, NVS_FLASH_STATUS_ERASED } NvsFlashStatus;

NvsFlashStatus initFlash(void);
