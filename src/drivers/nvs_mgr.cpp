#define __NVS_MGR__ 1
#include "nvs_mgr.hpp"
#include "esp_log.h"

#include <string.h>
#include <stdlib.h>

NVSMgr NVSMgr::singleton;

bool
NVSMgr::setup(bool force_erase)
{
  esp_err_t err;

  initialized = false;

  if (force_erase) {
    if ((err = nvs_flash_erase()) == ESP_OK) {
      err    = nvs_flash_init();
    }
  }
  else {
    err = nvs_flash_init();
    if (err != ESP_OK) {
      if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_LOGI(TAG, "Erasing NVS Partition... (Because of %s)", esp_err_to_name(err));
        if ((err = nvs_flash_erase()) == ESP_OK) {
          err    = nvs_flash_init();
        }
      }
    }
  } 

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "NVS Error: %s", esp_err_to_name(err));
  }
  else {
    initialized = true;
  }

  return initialized;
}

bool 
NVSMgr::get(char * segment_name, uint8_t * data, size_t data_size) 
{
  nvs_handle_t nvs_handle;
  esp_err_t err;
  size_t segment_size;
  bool result = false;

  ESP_LOGI(TAG, "Reading data size %d from segment %s...", data_size, segment_name);

  if (!initialized) {
    ESP_LOGE(TAG, "NVS not initialized.");
    return false;
  }

  if ((err = nvs_open(segment_name, NVS_READONLY, &nvs_handle)) == ESP_OK) {
    if ((err = nvs_get_blob(nvs_handle, segment_name, nullptr, &segment_size)) == ESP_OK) {
      if (segment_size >= data_size) {
        uint8_t * segment_data = (uint8_t *) malloc(segment_size);
        if (segment_data != nullptr) {
          if ((err = nvs_get_blob(nvs_handle, segment_name, segment_data, &segment_size)) == ESP_OK) {
            memcpy(data, segment_data, data_size);
            result = true;
          }
          else {
            ESP_LOGE(TAG, "Unable to read NVS data: %s.", esp_err_to_name(err));
          }
          free(segment_data);
        }
        else {
          ESP_LOGE(TAG, "Unable to allocate memory (%d bytes) to read segment content.", segment_size);
        }
      }
      else {
        ESP_LOGE(TAG, "Segment length %d is to small to contain requested data of length %d.", segment_size, data_size);
      }
    }
    else {
      ESP_LOGE(TAG, "Unable to read NVS data length of segment %s: %s.", segment_name, esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
  }
  else {
    ESP_LOGE(TAG, "Unable to open NVS segment %s: %s.", segment_name, esp_err_to_name(err));
  }

  return result;
}

bool 
NVSMgr::put(char * segment_name, size_t segment_size, uint8_t * data, size_t data_size) 
{
  nvs_handle_t nvs_handle;
  esp_err_t err;
  bool result = false;

  ESP_LOGI(TAG, "Writing segment %s of size %d with data size %d...", segment_name, segment_size, data_size);

  if (!initialized) {
    ESP_LOGE(TAG, "NVS not initialized.");
    return false;
  }

  if (data_size <= segment_size) {
    if ((err = nvs_open(segment_name, NVS_READWRITE, &nvs_handle)) == ESP_OK) {
      uint8_t * segment_data = (uint8_t *) malloc(segment_size);
      if (segment_data != nullptr) {
        memset(segment_data, 0, segment_size);
        memcpy(segment_data, data, data_size);
        if ((err = nvs_set_blob(nvs_handle, segment_name, segment_data, segment_size)) == ESP_OK) {
          result = true;
        }
        else {
          ESP_LOGE(TAG, "Unable to write NVS data: %s.", esp_err_to_name(err));
        }
        free(segment_data);
      }
      else {
        ESP_LOGE(TAG, "Unable to allocate memory (%d bytes) to write segment content.", segment_size);
      }
      nvs_close(nvs_handle);
    }
    else {
      ESP_LOGE(TAG, "Unable to open NVS segment %s: %s.", segment_name, esp_err_to_name(err));
    }
  }
  else {
    ESP_LOGE(TAG, "Segment size %d to small for the requested data size %d.", segment_size, data_size);
  }

  return result;
}