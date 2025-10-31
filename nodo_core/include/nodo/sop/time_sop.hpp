#pragma once

#include "../core/geometry_container.hpp"
#include "../core/standard_attributes.hpp"
#include "sop_node.hpp"
#include <memory>

namespace nodo::sop {

/**
 * @brief Time SOP node - provides time/frame information for animation
 *
 * The Time node is a source node that generates a single point with
 * time-related attributes. These attributes can be used by downstream
 * nodes (especially Wrangle) to drive procedural animation.
 *
 * Attributes created:
 * - @frame (int): Current frame number
 * - @time (float): Time in seconds (frame / FPS)
 * - @fps (float): Frames per second
 *
 * Example Wrangle usage:
 *   rotation = @time * 90;  // Rotate 90 degrees per second
 */
class TimeSOP : public SOPNode {
public:
  static constexpr int NODE_VERSION = 1;

  explicit TimeSOP(const std::string &node_name = "time")
      : SOPNode(node_name, "Time") {
    // No input ports - this is a source node

    // Frame number
    register_parameter(define_int_parameter("frame", 1)
                           .label("Frame")
                           .range(1, 240)
                           .category("Time")
                           .build());

    // FPS
    register_parameter(define_float_parameter("fps", 24.0F)
                           .label("FPS")
                           .range(1.0, 120.0)
                           .category("Time")
                           .build());

    // Start frame (for normalized time)
    register_parameter(define_int_parameter("start_frame", 1)
                           .label("Start Frame")
                           .range(1, 1000)
                           .category("Time Range")
                           .build());

    // End frame (for normalized time)
    register_parameter(define_int_parameter("end_frame", 240)
                           .label("End Frame")
                           .range(1, 10000)
                           .category("Time Range")
                           .build());
  }

protected:
  std::shared_ptr<core::GeometryContainer> execute() override {
    const int frame = get_parameter<int>("frame", 1);
    const float fps = get_parameter<float>("fps", 24.0F);
    const int start_frame = get_parameter<int>("start_frame", 1);
    const int end_frame = get_parameter<int>("end_frame", 240);

    // Calculate time values
    const float time = static_cast<float>(frame) / fps;
    const float frame_range = static_cast<float>(end_frame - start_frame);
    const float normalized_time =
        frame_range > 0.0F
            ? static_cast<float>(frame - start_frame) / frame_range
            : 0.0F;

    // Create output geometry with a single point at origin
    auto output = std::make_shared<core::GeometryContainer>();

    // Set point count
    output->set_point_count(1);

    // Add position attribute (standard practice)
    output->add_point_attribute("P", core::AttributeType::VEC3F);
    auto *positions = output->get_point_attribute_typed<core::Vec3f>("P");
    (*positions)[0] = core::Vec3f(0.0F, 0.0F, 0.0F);

    // Create time-related point attributes
    output->add_point_attribute("time", core::AttributeType::FLOAT);
    auto *time_attr = output->get_point_attribute_typed<float>("time");
    (*time_attr)[0] = time;

    output->add_point_attribute("fps", core::AttributeType::FLOAT);
    auto *fps_attr = output->get_point_attribute_typed<float>("fps");
    (*fps_attr)[0] = fps;

    output->add_point_attribute("frame", core::AttributeType::INT);
    auto *frame_attr = output->get_point_attribute_typed<int>("frame");
    (*frame_attr)[0] = frame;

    output->add_point_attribute("normalized_time", core::AttributeType::FLOAT);
    auto *norm_time_attr =
        output->get_point_attribute_typed<float>("normalized_time");
    (*norm_time_attr)[0] = normalized_time;

    return output;
  }
};

} // namespace nodo::sop
