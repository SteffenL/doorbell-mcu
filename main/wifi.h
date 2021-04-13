#pragma once

typedef enum { WIFI_WAIT_RESULT_OK, WIFI_WAIT_RESULT_FAIL } WifiWaitResult;

void initWifi(void);
void setWifiConfig(void);
void deinitWifi(void);
void startWifi(void);
void stopWifi(void);
WifiWaitResult waitForWifiConnection(void);
