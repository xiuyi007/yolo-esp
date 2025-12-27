#pragma once
#include "dl_detect_base.hpp"
#include "dl_detect_yolo11_postprocessor.hpp"

namespace pest_detect {
class Yolo11n : public dl::detect::DetectImpl {
public:
    static inline constexpr float default_score_thr = 0.25;
    static inline constexpr float default_nms_thr = 0.7;
    Yolo11n(const char *model_name, float score_thr, float nms_thr);
};
} // namespace pest_detect

class PestDetect : public dl::detect::DetectWrapper {
public:
    typedef enum {
        YOLO11N_S8_V1,
        YOLO11N_S8_V2,
        YOLO11N_S8_V3,
        YOLO11N_320_S8_V3,
        YOLO11S_S8_V1,
    } model_type_t;
    PestDetect(model_type_t model_type = static_cast<model_type_t>(CONFIG_DEFAULT_PEST_DETECT_MODEL),
               bool lazy_load = true);

private:
    void load_model() override;

    model_type_t m_model_type;
};