#include "flag.h"
#include "nndeploy/base/glic_stl_include.h"
#include "nndeploy/base/time_profiler.h"
#include "nndeploy/codec/codec.h"
#include "nndeploy/dag/node.h"
#include "nndeploy/detect/drawbox.h"
#include "nndeploy/detect/yolo/yolo.h"
#include "nndeploy/device/device.h"
#include "nndeploy/thread_pool/thread_pool.h"
#include "nndeploy/framework.h"

using namespace nndeploy;

// dag::TypeGraphRegister g_register_yolov8_graph("test",
//                                                detect::createYoloV8Graph);

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
  if (demo::FLAGS_usage) {
    demo::showUsage();
    return -1;
  }

  int ret = nndeployFrameworkInit();
  if (ret != 0) {
    NNDEPLOY_LOGE("nndeployFrameworkInit failed. ERROR: %d\n", ret);
    return ret;
  }

  // 检测模型的有向无环图graph名称，例如:
  // NNDEPLOY_YOLOV5/NNDEPLOY_YOLOV6/NNDEPLOY_YOLOV8
  std::string name = demo::getName();
  // 推理后端类型，例如:
  // kInferenceTypeOpenVino / kInferenceTypeTensorRt / kInferenceTypeOnnxRuntime
  base::InferenceType inference_type = demo::getInferenceType();
  // 推理设备类型，例如:
  // kDeviceTypeCodeX86:0/kDeviceTypeCodeCuda:0/...
  base::DeviceType device_type = demo::getDeviceType();
  // 模型类型，例如:
  // kModelTypeOnnx/kModelTypeMnn/...
  base::ModelType model_type = demo::getModelType();
  printf("model_type = %d\n", model_type);
  // 模型是否是路径
  bool is_path = demo::isPath();
  // 模型路径或者模型字符串
  std::vector<std::string> model_value = demo::getModelValue();
  // input path
  std::string input_path = demo::getInputPath();
  // input path
  base::CodecFlag codec_flag = demo::getCodecFlag();
  // output path
  std::string ouput_path = demo::getOutputPath();
  // base::kParallelTypePipeline / base::kParallelTypeSequential
  base::ParallelType pt = demo::getParallelType();

  // 有向无环图graph的输入边packert
  dag::Edge input("detect_in");
  // 有向无环图graph的输出边packert
  dag::Edge output("detect_out");

  // graph
  dag::Graph *graph = new dag::Graph("demo", nullptr, &output);
  if (graph == nullptr) {
    NNDEPLOY_LOGE("graph is nullptr");
    return -1;
  }

  // 创建检测模型有向无环图graph
  dag::Graph *detect_graph =
      dag::createGraph(name, inference_type, device_type, &input, &output,
                       model_type, is_path, model_value);
  if (detect_graph == nullptr) {
    NNDEPLOY_LOGE("detect_graph is nullptr");
    return -1;
  }
  // detect_graph->setTimeProfileFlag(true);
  graph->addNode(detect_graph);

  // 解码节点
  codec::DecodeNode *decode_node = codec::createDecodeNode(
      base::kCodecTypeOpenCV, codec_flag, "decode_node", &input);
  decode_node->setPath(input_path);
  graph->addNode(decode_node);

  // draw box
  dag::Edge *draw_output = graph->createEdge("draw_output");
  dag::Node *draw_box_node;
  if (name == "NNDEPLOY_YOLOV5_MULTI_CONV_OUTPUT") {
    draw_box_node = graph->createNode<detect::YoloMultiConvDrawBoxNode>(
        "DrawBoxNode", {&input, &output}, {draw_output});
  } else {
    draw_box_node = graph->createNode<detect::DrawBoxNode>(
        "DrawBoxNode", {&input, &output}, {draw_output});
  }

  // 解码节点
  codec::EncodeNode *encode_node = codec::createEncodeNode(
      base::kCodecTypeOpenCV, codec_flag, "encode_node", draw_output);
  encode_node->setRefPath(input_path);
  encode_node->setPath(ouput_path);
  graph->addNode(encode_node);

  // 设置pipeline并行
  base::Status status = graph->setParallelType(pt);
  if (status != base::kStatusCodeOk) {
    NNDEPLOY_LOGE("graph setParallelType failed");
    return -1;
  }

  graph->setTimeProfileFlag(true);

  // 初始化有向无环图graph
  NNDEPLOY_TIME_POINT_START("graph->init()");
  status = graph->init();
  if (status != base::kStatusCodeOk) {
    NNDEPLOY_LOGE("graph init failed");
    return -1;
  }
  NNDEPLOY_TIME_POINT_END("graph->init()");

  status = graph->dump();
  status = detect_graph->dump();

  NNDEPLOY_TIME_POINT_START("graph->run");
  int size = decode_node->getSize();
  // size = 2;
  // decode_node->setSize(size);
  NNDEPLOY_LOGE("size = %d.\n", size);
  for (int i = 0; i < size; ++i) {
    status = graph->run();
    if (status != base::kStatusCodeOk) {
      NNDEPLOY_LOGE("graph deinit failed");
      return -1;
    }

    if (pt != base::kParallelTypePipeline) {
      detect::DetectResult *result =
          (detect::DetectResult *)output.getGraphOutputParam();
      if (result == nullptr) {
        NNDEPLOY_LOGE("result is nullptr");
        return -1;
      }
    }
  }

  if (pt == base::kParallelTypePipeline) {
    // NNDEPLOY_LOGE("size = %d.\n", size);
    for (int i = 0; i < size; ++i) {
      detect::DetectResult *result =
          (detect::DetectResult *)output.getGraphOutputParam();
      // NNDEPLOY_LOGE("%p.\n", result);
      if (result == nullptr) {
        NNDEPLOY_LOGE("result is nullptr");
        return -1;
      }
    }
  }
  NNDEPLOY_TIME_POINT_END("graph->run");

  NNDEPLOY_LOGI("hello world!\n");

  // 有向无环图graph反初始化
  status = graph->deinit();
  if (status != base::kStatusCodeOk) {
    NNDEPLOY_LOGE("graph deinit failed");
    return -1;
  }

  NNDEPLOY_TIME_PROFILER_PRINT("demo");

  // 有向无环图graph销毁
  delete encode_node;
  delete decode_node;
  delete detect_graph;
  delete graph;

  NNDEPLOY_LOGI("hello world!\n");

  ret = nndeployFrameworkDeinit();
  if (ret != 0) {
    NNDEPLOY_LOGE("nndeployFrameworkInit failed. ERROR: %d\n", ret);
    return ret;
  }
  return 0;

  return 0;
}
