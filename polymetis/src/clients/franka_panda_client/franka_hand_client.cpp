#include "polymetis/clients/franka_hand_client.hpp"

#include "spdlog/spdlog.h"
#include <string>
#include <thread>
#include <time.h>
#include <franka/exception.h>

#include <grpc/grpc.h>

#include "polymetis.grpc.pb.h"
#include "polymetis/utils.h"

using grpc::ClientContext;

FrankaHandClient::FrankaHandClient(std::shared_ptr<grpc::Channel> channel,
                                   YAML::Node config)
    : stub_(GripperServer::NewStub(channel)) {
  // Connect to gripper
  std::string robot_ip = config["robot_ip"].as<std::string>();
  spdlog::info("Connecting to robot_ip {}", robot_ip);
  gripper_.reset(new franka::Gripper(robot_ip));

  // Initialize gripper
  gripper_->homing();
  is_moving_ = false;

  // Initialize server connection
  franka::GripperState franka_gripper_state = gripper_->readOnce();

  GripperMetadata metadata;
  metadata.set_max_width(franka_gripper_state.max_width);
  metadata.set_hz(GRIPPER_HZ);

  ClientContext context;
  Empty empty;
  stub_->InitRobotClient(&context, metadata, &empty);

  spdlog::info("Connected.", robot_ip);
}

void FrankaHandClient::getGripperState(void) {
  franka::GripperState franka_gripper_state = gripper_->readOnce();

  gripper_state_.set_width(franka_gripper_state.width);
  gripper_state_.set_is_grasped(franka_gripper_state.is_grasped);
  gripper_state_.set_is_moving(is_moving_);
  gripper_state_.set_prev_command_successful(prev_cmd_successful_);

  // gripper_state.time();  // Use current timestamp instead!
  setTimestampToNow(gripper_state_.mutable_timestamp());
}

void FrankaHandClient::applyGripperCommand(void) {
  is_moving_ = true;

  if (gripper_cmd_.grasp()) {
    spdlog::info("Grasping at width {} at speed={}", gripper_cmd_.width(),
                 gripper_cmd_.speed());
    double eps_inner = (gripper_cmd_.epsilon_inner() < 0)
                           ? EPSILON_INNER
                           : gripper_cmd_.epsilon_inner();
    double eps_outer = (gripper_cmd_.epsilon_outer() < 0)
                           ? EPSILON_OUTER
                           : gripper_cmd_.epsilon_outer();
    prev_cmd_successful_ =
        gripper_->grasp(gripper_cmd_.width(), gripper_cmd_.speed(),
                        gripper_cmd_.force(), eps_inner, eps_outer);

  } else {
    spdlog::info("Moving to width {} at speed={}", gripper_cmd_.width(),
                 gripper_cmd_.speed());
    try{prev_cmd_successful_ =
      gripper_->move(gripper_cmd_.width(), gripper_cmd_.speed());}
      catch (const franka::Exception& e) {
        // 捕获异常并打印警告
        spdlog::warn("Gripper command exception: {}", e.what());
    }

    
  }

  is_moving_ = false;
}

void FrankaHandClient::run(void) {
  int period = 1.0 / GRIPPER_HZ;
  int period_ns = period * 1.0e9;

  int timestamp_ns;

  struct timespec abs_target_time;
  clock_gettime(CLOCK_REALTIME, &abs_target_time);
  while (true) {
    // Run control step
    getGripperState();

    grpc::ClientContext context;
    status_ = stub_->ControlUpdate(&context, gripper_state_, &gripper_cmd_);

    // if (!is_moving_) {
    //   // Skip if command not updated
    //   timestamp_ns = gripper_cmd_.timestamp().nanos();
    //   if (timestamp_ns != prev_cmd_timestamp_ns_ && timestamp_ns) {
    //     // applyGripperCommand() in separate thread
    //     std::thread th(&FrankaHandClient::applyGripperCommand, this);
    //     th.detach();
    //     prev_cmd_timestamp_ns_ = timestamp_ns;
    //   }
    // }

    if (!is_moving_) {
      // Skip if command not updated
      timestamp_ns = gripper_cmd_.timestamp().nanos();
      if (timestamp_ns != prev_cmd_timestamp_ns_ && timestamp_ns) {
          // applyGripperCommand() in separate thread
          std::thread th([this]() {
              try {
                  applyGripperCommand();  // 执行指令
              } catch (const franka::Exception& e) {
                  // 捕获异常并打印警告
                  spdlog::warn("Gripper command exception: {}", e.what());
              } catch (const std::exception& e) {
                  // 捕获其他标准异常
                  spdlog::warn("Standard exception: {}", e.what());
              } 
          });
          th.detach();  // 分离线程，允许继续执行
          prev_cmd_timestamp_ns_ = timestamp_ns;
      }
  }
  
    // if (!is_moving_) {
    //   timestamp_ns = gripper_cmd_.timestamp().nanos();
      
    //   // 只有在 timestamp 变了 && 有效命令（timestamp != 0）
    //   if (timestamp_ns != prev_cmd_timestamp_ns_ && timestamp_ns) {
    //     // 备份当前命令
    //     GripperCommand cached_cmd = gripper_cmd_;
    
    //     // 单独开线程执行
    //     std::thread th([this, cached_cmd, timestamp_ns]() {
    //       try {
    //         gripper_cmd_ = cached_cmd;
    //         applyGripperCommand();
    //         if (prev_cmd_successful_) {
    //           // ✅ 只有执行成功才更新 timestamp
    //           prev_cmd_timestamp_ns_ = timestamp_ns;
    //         } else {
    //           spdlog::warn("Gripper command failed, will retry on next update");
    //         }
    //       } catch (const franka::Exception& e) {
    //         spdlog::warn("Gripper command exception: {}", e.what());
    //       }
    //       // prev_cmd_timestamp_ns_ = timestamp_ns;
    //     });
    //     th.detach();
    //     prev_cmd_timestamp_ns_ = timestamp_ns;
    //   }
    // }


    // Spin once
    abs_target_time.tv_nsec += period_ns;
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &abs_target_time, nullptr);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    spdlog::error("Usage: franka_hand_client /path/to/cfg.yaml");
    return 1;
  }
  YAML::Node config = YAML::LoadFile(argv[1]);

  // Launch client
  std::string control_address = config["control_ip"].as<std::string>() + ":" +
                                config["control_port"].as<std::string>();
  FrankaHandClient franka_hand_client(
      grpc::CreateChannel(control_address, grpc::InsecureChannelCredentials()),
      config);
  franka_hand_client.run();

  // Termination
  spdlog::info("Wait for shutdown; press CTRL+C to close.");

  return 0;
}
