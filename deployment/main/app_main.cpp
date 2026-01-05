#include "pest_detect.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "dl_image_jpeg.hpp"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"

const char *TAG = "pest";

// 引用模型数据
extern const uint8_t pest_jpg_start[] asm("_binary_pest_jpg_start");
extern const uint8_t pest_jpg_end[] asm("_binary_pest_jpg_end");

// WiFi 配置
#define WIFI_SSID "togethf"
#define WIFI_PASS "lyh123456"

// 本地 Python 服务器地址 (请确保端口和 IP 与电脑一致)
#define SERVER_URL "http://172.20.10.3:8000/upload"

// --- WiFi 事件管理 ---
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// WiFi 事件回调函数 (自动处理连接状态)
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi 连接断开，正在尝试重连...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "成功获取 IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// --- WiFi 初始化函数 (稳健版) ---
bool wifi_init_for_exp() {
    // 1. NVS 初始化
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 创建事件组和网络接口
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // 3. 注册事件处理程序
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // 4. 配置 WiFi
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    // 兼容性配置 (针对手机热点优化)
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 【关键】关闭节能模式，确保电流波形平稳且网络响应最快
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); 
    ESP_LOGW(TAG, "WiFi 启动完成 (节能模式已关闭)，等待连接...");

    // 5. 阻塞等待连接结果 (最多等 20 秒)
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(20000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi 连接就绪，可以开始传输实验");
        return true;
    } else {
        ESP_LOGE(TAG, "WiFi 连接失败或超时 (请检查热点是否开启)");
        return false;
    }
}

// --- 模拟云端上传函数 ---
void simulate_cloud_transfer(const uint8_t* data, size_t len) {
    esp_http_client_config_t config = {};
    config.url = SERVER_URL; // 使用宏定义的地址
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 10000;
    config.skip_cert_common_name_check = true;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP Client 初始化失败");
        return;
    }
    
    // 设置 POST 数据
    esp_http_client_set_post_field(client, (const char*)data, len);
    
    ESP_LOGI(TAG, "正在上传数据 (%d bytes)...", len);
    int64_t start = esp_timer_get_time();
    
    // 执行阻塞上传
    esp_err_t err = esp_http_client_perform(client);
    
    int64_t end = esp_timer_get_time();
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status >= 200 && status < 300) {
            ESP_LOGW(TAG, "上传成功 [HTTP %d] 耗时: %lld us", status, end - start);
        } else {
            ESP_LOGE(TAG, "服务器收到数据但返回错误 [HTTP %d]", status);
        }
    } else {
        ESP_LOGE(TAG, "网络传输失败: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

extern "C" void app_main(void)
{
    // 准备数据
    dl::image::jpeg_img_t jpeg_img = {.data = (void *)pest_jpg_start, .data_len = (size_t)(pest_jpg_end - pest_jpg_start)};
    auto img = sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);
    PestDetect *detect = new PestDetect();

    // ---------------------------------------------------------
    // 阶段 A: 静态标定 (P_static)
    // ---------------------------------------------------------
    ESP_LOGW("BENCH", "Step A: 静态待机标定 (10s)...");
    vTaskDelay(pdMS_TO_TICKS(10000));
    // ---------------------------------------------------------
    // 阶段 B: 本地算力测试 (保留原逻辑)
    // ---------------------------------------------------------
    ESP_LOGI("BENCH", "Step B: 正在预热...");
    detect->run(img);

    const int LOOP_COUNT = 5; // 建议次数多一点以方便测电流
    ESP_LOGI("BENCH", "开始本地性能测试，循环 %d 次...", LOOP_COUNT);
    
    int64_t start_time = esp_timer_get_time();
    for (int i = 0; i < LOOP_COUNT; i++) {
        detect->run(img);
    }
    int64_t end_time = esp_timer_get_time();

    // 计算并保留原输出格式
    int64_t total_time_us = end_time - start_time;
    float avg_time_ms = (float)total_time_us / LOOP_COUNT / 1000.0f;
    float fps = 1000.0f / avg_time_ms;

    printf("\n================ ESP32-S3 Benchmark ================\n");
    printf("Model Loop:   %d times\n", LOOP_COUNT);
    printf("Avg Latency:  %.2f ms\n", avg_time_ms);
    printf("FPS:          %.2f\n", fps);
    printf("====================================================\n\n");
    
    vTaskDelay(pdMS_TO_TICKS(5000)); // 冷却

    // ---------------------------------------------------------
    // 阶段 C: 网络传输测试 (E_cloud)
    // ---------------------------------------------------------
    ESP_LOGW("BENCH", "Step C: 网络传输标定开始...");
    
    // 尝试初始化 WiFi
    if (wifi_init_for_exp()) {
        // 只有 WiFi 连接成功才执行上传循环
        for(int i=0; i<10; i++) {
            simulate_cloud_transfer(pest_jpg_start, (size_t)(pest_jpg_end - pest_jpg_start));
            vTaskDelay(pdMS_TO_TICKS(2000)); // 间隔 2 秒
        }
    } else {
        ESP_LOGE("BENCH", "跳过 Step C，因为网络初始化失败");
    }

    // ---------------------------------------------------------
    // 结果回收
    // ---------------------------------------------------------
    ESP_LOGI(TAG, "实验数据采集完成。");
    delete detect;
    heap_caps_free(img.data);
}