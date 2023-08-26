/**
 * @github: https://github.com/facebookresearch/detr
 * @huggingface: https://github.com/facebookresearch/detr
 * 这里要改
 */
#ifndef _NNDEPLOY_MODEL_DETECT_0PENCV_DETR_DETR_H_
#define _NNDEPLOY_MODEL_DETECT_0PENCV_DETR_DETR_H_

#include "nndeploy/base/common.h"
#include "nndeploy/base/glic_stl_include.h"
#include "nndeploy/base/log.h"
#include "nndeploy/base/macro.h"
#include "nndeploy/base/object.h"
#include "nndeploy/base/opencv_include.h"
#include "nndeploy/base/param.h"
#include "nndeploy/base/status.h"
#include "nndeploy/base/string.h"
#include "nndeploy/base/value.h"
#include "nndeploy/device/buffer.h"
#include "nndeploy/device/buffer_pool.h"
#include "nndeploy/device/device.h"
#include "nndeploy/device/tensor.h"
#include "nndeploy/model/detect/result.h"
#include "nndeploy/pipeline/packet.h"
#include "nndeploy/pipeline/pipeline.h"
#include "nndeploy/pipeline/task.h"

namespace nndeploy {
namespace model {
namespace opencv {

class NNDEPLOY_CC_API Yolov5PostParam : public base::Param {  // param要改
 public:
  float conf_thres = 0.4;
  float iou_thres = 0.5;
  // yolov5s model configurations
  std::vector<float> strides_ = {32.f, 16.f, 8.f};
  std::vector<float> anchor_grids_ = {116.f, 90.f, 156.f, 198.f, 373.f, 326.f,
                                      30.f,  61.f, 62.f,  45.f,  59.f,  119.f,
                                      10.f,  13.f, 16.f,  30.f,  33.f,  23.f};
  float score_threshold_ = 0.5f;
  int num_anchor_ = 3;
  int detect_dim_ = 85;
  int grid_per_input_ = 6;
};

class NNDEPLOY_CC_API Yolov5PostProcess : public pipeline::Task {
 public:
  Yolov5PostProcess(const std::string& name, pipeline::Packet* input,
                    pipeline::Packet* output)
      : Task(name, input, output) {
    param_ = std::make_shared<Yolov5PostParam>();
  }
  virtual ~Yolov5PostProcess() {}

  virtual base::Status run();

 private:
  DetectResults results_;
};

extern NNDEPLOY_CC_API pipeline::Pipeline* creatYolov5Pipeline(
    const std::string& name, base::InferenceType type, pipeline::Packet* input,
    pipeline::Packet* output, bool is_path,
    std::vector<std::string>& model_value);

}  // namespace opencv
}  // namespace model
}  // namespace nndeploy

#endif /* _NNDEPLOY_DETECT_0PENCV_POSTPROCESS_H_ */
