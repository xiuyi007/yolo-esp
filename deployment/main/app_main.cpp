#include "pest_detect.hpp"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "esp_timer.h"

extern const uint8_t pest_jpg_start[] asm("_binary_pest_jpg_start");
extern const uint8_t pest_jpg_end[] asm("_binary_pest_jpg_end");
const char *TAG = "pest";

extern "C" void app_main(void)
{
#if CONFIG_COCO_DETECT_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_mount());
#endif

    dl::image::jpeg_img_t jpeg_img = {.data = (void *)pest_jpg_start, .data_len = (size_t)(pest_jpg_end - pest_jpg_start)};
    auto img = sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);

    PestDetect *detect = new PestDetect();

    // 记录推理开始时间 (微秒)
    int64_t start_time = esp_timer_get_time();

    auto &detect_results = detect->run(img);

    // 记录结束时间并计算耗时
    int64_t end_time = esp_timer_get_time();
    float latency = (end_time - start_time) / 1000.0f;
    float fps = 1000.0f / latency;
    ESP_LOGW("PERF", "infer-time: %.2f ms | speed: %.2f FPS", latency, fps);

    for (const auto &res : detect_results) {
        ESP_LOGI(TAG,
                 "[category: %d, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                 res.category,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);
    }
    delete detect;
    heap_caps_free(img.data);

#if CONFIG_COCO_DETECT_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_unmount());
#endif
}
