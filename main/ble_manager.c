
#include "ble_manager.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "string.h"

#define GATTS_TAG "NEXUS_GATTS"

///Declare the static function
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static uint16_t cccd_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

#define GATTS_TABLE_TAG "GATTS_TABLE_DEMO"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "NEXUS_PENDANT"
#define SVC_INST_ID                 0

/* The max length of characteristic value. When you want to modify the value this max length should be greater than GATT_MAX_ATTR_LEN. */
#define GATT_MAX_ATTR_LEN           ESP_GATT_MAX_ATTR_LEN
#define PREPARE_BUF_MAX_SIZE        1024

static uint8_t adv_config_flag = 0;

#define adv_config_src_flag        (1 << 0)
#define adv_config_rsp_flag        (1 << 1)

static uint8_t raw_adv_data[] = {
        0x02, 0x01, 0x06,
        0x02, 0x0a, 0xeb, 0x03, 0x03, 0xab, 0xcd,
        0x0f, 0x09, 'N', 'E', 'X', 'U', 'S', '_', 'P', 'E', 'N', 'D', 'A', 'N', 'T',
};
static uint8_t raw_scan_rsp_data[] = {
        0x01, 0x01, 0x00,
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle[5]; // 5 characteristics
    esp_bt_uuid_t char_uuid[5];
    esp_gatt_perm_t perm[5];
    esp_gatt_char_prop_t property[5];
    uint16_t descr_handle[5];
    esp_bt_uuid_t descr_uuid[5];
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if created by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_NEXUS = NEXUS_SERVICE_UUID;

/* Characteristics */
static const uint16_t GATTS_CHAR_UUID_AUDIO_DATA = AUDIO_DATA_CHAR_UUID;
static const uint16_t GATTS_CHAR_UUID_COMMAND_RX = COMMAND_RX_CHAR_UUID;
static const uint16_t GATTS_CHAR_UUID_COMMAND_TX = COMMAND_TX_CHAR_UUID;
static const uint16_t GATTS_CHAR_UUID_STATUS     = STATUS_CHAR_UUID;
static const uint16_t GATTS_CHAR_UUID_BATTERY    = BATTERY_CHAR_UUID;

static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static esp_attr_value_t gatts_char_val_audio_data = {
    .attr_max_len = GATT_MAX_ATTR_LEN,
    .attr_len     = 0,
    .attr_value   = NULL,
};

static esp_attr_value_t gatts_char_val_command_rx = {
    .attr_max_len = GATT_MAX_ATTR_LEN,
    .attr_len     = 0,
    .attr_value   = NULL,
};

static esp_attr_value_t gatts_char_val_command_tx = {
    .attr_max_len = GATT_MAX_ATTR_LEN,
    .attr_len     = 0,
    .attr_value   = NULL,
};

static esp_attr_value_t gatts_char_val_status = {
    .attr_max_len = GATT_MAX_ATTR_LEN,
    .attr_len     = 0,
    .attr_value   = NULL,
};

static esp_attr_value_t gatts_char_val_battery = {
    .attr_max_len = GATT_MAX_ATTR_LEN,
    .attr_len     = 0,
    .attr_value   = NULL,
};

static esp_attr_desc_t gatts_char_descr_audio_data = {
    .uuid_length = ESP_UUID_LEN_16,
    .uuid_p = (uint8_t *)&cccd_uuid,
    .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    .max_length = 2,
    .length = 0,
    .value = NULL,
};

static esp_attr_desc_t gatts_char_descr_command_tx = {
    .uuid_length = ESP_UUID_LEN_16,
    .uuid_p = (uint8_t *)&cccd_uuid,
    .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    .max_length = 2,
    .length = 0,
    .value = NULL,
};

static esp_attr_desc_t gatts_char_descr_status = {
    .uuid_length = ESP_UUID_LEN_16,
    .uuid_p = (uint8_t *)&cccd_uuid,
    .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    .max_length = 2,
    .length = 0,
    .value = NULL,
};

static esp_attr_desc_t gatts_char_descr_battery = {
    .uuid_length = ESP_UUID_LEN_16,
    .uuid_p = (uint8_t *)&cccd_uuid,
    .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    .max_length = 2,
    .length = 0,
    .value = NULL,
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_flag &= (~adv_config_src_flag);
        if (adv_config_flag == 0) {
            //esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_flag &= (~adv_config_rsp_flag);
        if (adv_config_flag == 0) {
            //esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }else{
            ESP_LOGI(GATTS_TAG, "Advertising start successfully\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        }else {
            ESP_LOGI(GATTS_TAG, "Advertising stop successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
            gl_profile_tab[PROFILE_APP_IDX].service_id.is_primary = true;
            gl_profile_tab[PROFILE_APP_IDX].service_id.id.inst_id = 0x00;
            gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_APP_IDX].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_NEXUS;

            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            adv_config_flag = 0;
            /*esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
            if (raw_adv_ret){
                ESP_LOGE(GATTS_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
            }
            adv_config_flag |= adv_config_src_flag;
            esp_err_t raw_scan_rsp_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if (raw_scan_rsp_ret){
                ESP_LOGE(GATTS_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_rsp_ret);
            }
            adv_config_flag |= adv_config_rsp_flag;*/
            esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_APP_IDX].service_id, GATT_MAX_ATTR_LEN);
            break;
        }
        case ESP_GATTS_READ_EVT: {
            ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %lu, trans_id %lu, handle %lu\n", (unsigned long)param->read.conn_id, (unsigned long)param->read.trans_id, (unsigned long)param->read.handle);
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;
            // TODO: Populate response based on characteristic being read
            esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            break;
        }
        case ESP_GATTS_WRITE_EVT: {
            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %lu, trans_id %lu, handle %lu", (unsigned long)param->write.conn_id, (unsigned long)param->write.trans_id, (unsigned long)param->write.handle);
            if (!param->write.is_prep){
                ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value %.*s", param->write.len, param->write.len, param->write.value);
                // Handle COMMAND_RX characteristic write
                if (gl_profile_tab[PROFILE_APP_IDX].char_handle[1] == param->write.handle) { // COMMAND_RX
                    // Process incoming command
                    ESP_LOGI(GATTS_TAG, "Received command: %.*s", param->write.len, param->write.value);
                    // TODO: Parse and execute command
                }
                // Handle CCCD write for notifications
                if (gl_profile_tab[PROFILE_APP_IDX].descr_handle[0] == param->write.handle) { // AUDIO_DATA CCCD
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TAG, "notify enable");
                    }else if (descr_value == 0x0002){
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                    }else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                    }else{
                        ESP_LOGE(GATTS_TAG, "unknown descr value");
                    }
                }
                 if (gl_profile_tab[PROFILE_APP_IDX].descr_handle[1] == param->write.handle) { // COMMAND_TX CCCD
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TAG, "notify enable");
                    }else if (descr_value == 0x0002){
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                    }else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                    }else{
                        ESP_LOGE(GATTS_TAG, "unknown descr value");
                    }
                }
                 if (gl_profile_tab[PROFILE_APP_IDX].descr_handle[2] == param->write.handle) { // STATUS CCCD
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TAG, "notify enable");
                    }else if (descr_value == 0x0002){
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                    }else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                    }else{
                        ESP_LOGE(GATTS_TAG, "unknown descr value");
                    }
                }
                 if (gl_profile_tab[PROFILE_APP_IDX].descr_handle[3] == param->write.handle) { // BATTERY CCCD
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TAG, "notify enable");
                    }else if (descr_value == 0x0002){
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                    }else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                    }else{
                        ESP_LOGE(GATTS_TAG, "unknown descr value");
                    }
                }
            }
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            break;
        }
        case ESP_GATTS_EXEC_WRITE_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            esp_ble_gatts_send_response(gatts_if, param->exec_write.conn_id, param->exec_write.trans_id, ESP_GATT_OK, NULL);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d", param->conf.status);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d\n", param->connect.conn_id);
            gl_profile_tab[PROFILE_APP_IDX].conn_id = param->connect.conn_id;
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;
            conn_params.min_int = 0x10;
            conn_params.timeout = 400;
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d\n", param->disconnect.reason);
            //esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_OPEN_EVT:
            break;
        case ESP_GATTS_CANCEL_OPEN_EVT:
            break;
        case ESP_GATTS_CLOSE_EVT:
            break;
        case ESP_GATTS_LISTEN_EVT:
            break;
        case ESP_GATTS_CONGEST_EVT:
            break;
        case ESP_GATTS_CREATE_EVT: {
            ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d, service_handle %d\n", param->create.status, param->create.service_handle);
            gl_profile_tab[PROFILE_APP_IDX].service_handle = param->create.service_handle;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[0].len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[0].uuid.uuid16 = GATTS_CHAR_UUID_AUDIO_DATA;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[1].len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[1].uuid.uuid16 = GATTS_CHAR_UUID_COMMAND_RX;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[2].len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[2].uuid.uuid16 = GATTS_CHAR_UUID_COMMAND_TX;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[3].len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[3].uuid.uuid16 = GATTS_CHAR_UUID_STATUS;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[4].len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_APP_IDX].char_uuid[4].uuid.uuid16 = GATTS_CHAR_UUID_BATTERY;

            esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_IDX].service_handle,
                                   &gl_profile_tab[PROFILE_APP_IDX].char_uuid[0],
                                   ESP_GATT_PERM_READ,
                                   char_prop_read_notify,
                                   &gatts_char_val_audio_data, NULL);
            esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_IDX].service_handle,
                                   &gl_profile_tab[PROFILE_APP_IDX].char_uuid[1],
                                   ESP_GATT_PERM_WRITE,
                                   char_prop_write,
                                   &gatts_char_val_command_rx, NULL);
            esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_IDX].service_handle,
                                   &gl_profile_tab[PROFILE_APP_IDX].char_uuid[2],
                                   ESP_GATT_PERM_READ | 0,
                                   char_prop_notify,
                                   &gatts_char_val_command_tx, NULL);
            esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_IDX].service_handle,
                                   &gl_profile_tab[PROFILE_APP_IDX].char_uuid[3],
                                   ESP_GATT_PERM_READ | 0,
                                   char_prop_read_notify,
                                   &gatts_char_val_status, NULL);
            esp_ble_gatts_add_char(gl_profile_tab[PROFILE_APP_IDX].service_handle,
                                   &gl_profile_tab[PROFILE_APP_IDX].char_uuid[4],
                                   ESP_GATT_PERM_READ | 0,
                                   char_prop_read_notify,
                                   &gatts_char_val_battery, NULL);
            break;
        }
        case ESP_GATTS_ADD_CHAR_EVT: {
            uint16_t handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_APP_IDX].char_handle[param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_AUDIO_DATA ? 0 : \
                                                        (param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_COMMAND_RX ? 1 : \
                                                        (param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_COMMAND_TX ? 2 : \
                                                        (param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_STATUS ? 3 : 4)))] = handle;
            ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d, attr_handle %d, service_handle %d\n",
                    param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

            if (param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_AUDIO_DATA ||
                param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_COMMAND_TX ||
                param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_STATUS ||
                param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_BATTERY) {
                esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_APP_IDX].service_handle,
                                             &gl_profile_tab[PROFILE_APP_IDX].descr_uuid[0],
                                             ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                             NULL, NULL);
            }
            break;
        }
        /*case ESP_GATTS_ADD_DESCR_EVT: {
            uint16_t handle = param->add_descr.attr_handle;
            // This logic needs to be more robust for multiple descriptors
            if (gl_profile_tab[PROFILE_APP_IDX].char_uuid[0].uuid.uuid16 == GATTS_CHAR_UUID_AUDIO_DATA) {
                gl_profile_tab[PROFILE_APP_IDX].descr_handle[0] = handle;
            } else if (gl_profile_tab[PROFILE_APP_IDX].char_uuid[2].uuid.uuid16 == GATTS_CHAR_UUID_COMMAND_TX) {
                gl_profile_tab[PROFILE_APP_IDX].descr_handle[1] = handle;
            } else if (gl_profile_tab[PROFILE_APP_IDX].char_uuid[3].uuid.uuid16 == GATTS_CHAR_UUID_STATUS) {
                gl_profile_tab[PROFILE_APP_IDX].descr_handle[2] = handle;
            } else if (gl_profile_tab[PROFILE_APP_IDX].char_uuid[4].uuid.uuid16 == GATTS_CHAR_UUID_BATTERY) {
                gl_profile_tab[PROFILE_APP_IDX].descr_handle[3] = handle;
            }

            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_descr.status, param->add_descr.attr_handle, param->add_descr.service_handle);
            if (param->add_descr.status == ESP_GATT_OK) {
                esp_ble_gatts_start_service(gl_profile_tab[PROFILE_APP_IDX].service_handle);
            }
            break;
        }*/
        case ESP_GATTS_DELETE_EVT:
            break;
        case ESP_GATTS_STOP_EVT:
            break;
        //case ESP_GATTS_BUILD_ADV_DATA_EVT:
        //    break;
        case ESP_GATTS_SET_ATTR_VAL_EVT:
            break;
        case ESP_GATTS_SEND_SERVICE_CHANGE_EVT:
            break;
        case ESP_GATTS_ADD_INCL_SRVC_EVT:
            break;
        //case ESP_GATTS_ADD_CHAR_FMT_EVT:
        //    break;
        //case ESP_GATTS_SET_LOCAL_MTU_EVT:
        //    ESP_LOGI(GATTS_TAG, "SET_LOCAL_MTU_EVT, status %d, mtu %d", param->set_local_mtu.status, param->set_local_mtu.mtu);
        //    break;
        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id, param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler, otherwise call profile B cb handler. */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not get the gatt_if, so must have an app id */
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void ble_manager_init(void)
{
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(ESP_APP_ID);

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}

void ble_task(void *pvParameters)
{
    // This task will primarily handle BLE events and notifications
    // The actual GATT callbacks are handled by gatts_event_handler
    // This task can be used for any periodic BLE related checks or processing
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        // Example: Check connection status, send periodic notifications
    }
}

void ble_send_command_tx(nx_packet_t *packet)
{
    if (gl_profile_tab[PROFILE_APP_IDX].conn_id != ESP_GATT_IF_NONE) {
        esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_APP_IDX].gatts_if,
                                    gl_profile_tab[PROFILE_APP_IDX].conn_id,
                                    gl_profile_tab[PROFILE_APP_IDX].char_handle[2], // COMMAND_TX
                                    sizeof(nx_packet_t), (uint8_t *)packet, false);
    }
}

void ble_send_status_update(uint8_t *status_data, uint16_t len)
{
    if (gl_profile_tab[PROFILE_APP_IDX].conn_id != ESP_GATT_IF_NONE) {
        esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_APP_IDX].gatts_if,
                                    gl_profile_tab[PROFILE_APP_IDX].conn_id,
                                    gl_profile_tab[PROFILE_APP_IDX].char_handle[3], // STATUS
                                    len, status_data, false);
    }
}

void ble_send_battery_level(uint8_t battery_level)
{
    if (gl_profile_tab[PROFILE_APP_IDX].conn_id != ESP_GATT_IF_NONE) {
        esp_ble_gatts_send_indicate(gl_profile_tab[PROFILE_APP_IDX].gatts_if,
                                    gl_profile_tab[PROFILE_APP_IDX].conn_id,
                                    gl_profile_tab[PROFILE_APP_IDX].char_handle[4], // BATTERY
                                    sizeof(uint8_t), &battery_level, false);
    }
}
