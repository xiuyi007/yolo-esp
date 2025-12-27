#pragma once
#include "dl_detect_base.hpp"
#include "dl_detect_yolo11_postprocessor.hpp"

namespace pest_detect {
class Yolo11n : public dl::detect::DetectImpl {
public:
    Yolo11n(const char *model_name);
};
} // namespace coco_detect

class PestDetect : public dl::detect::DetectWrapper {
public:
    typedef enum {
        YOLO11N_S8_V1,
        YOLO11N_S8_V2,
        YOLO11N_S8_V3,
        YOLO11N_320_S8_V3,
    } model_type_t;
    PestDetect(model_type_t model_type = static_cast<model_type_t>(CONFIG_DEFAULT_COCO_DETECT_MODEL));
};
