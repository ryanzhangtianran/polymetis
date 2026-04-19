# Copyright (c) Facebook, Inc. and its affiliates.

# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
import torch
import time
from polymetis import RobotInterface,GripperInterface
import math
# 夹爪的最大和最小开口
MAX_OPEN = 0.08   # 例如 8cm
MIN_CLOSE = 0.02  # 例如 2cm
FREQ = 5        # 正弦运动频率 (Hz)
DURATION = 6.0    # 夹爪正弦运动总时长 (秒)

if __name__ == "__main__":
    # Initialize robot interface
    robot = RobotInterface(
        ip_address="localhost",
    )
    gripper = GripperInterface(
        ip_address="localhost",
    )
# gello   7.88466125  6.23716588 -1.53704875  3.22749558  6.17120471  4.76914627 -1.51864098  3.51741795
#          [ 7.82330202  5.97945711  0.01994175  1.81163131  4.63415596  3.07256352 -1.46648563  3.51895193
# arm [-0.1349, -0.0151, -0.0534, -2.0547,  0.0347,  1.9798, -0.9089]
    # gripper.goto(width=MAX_OPEN, speed=255, force=255)
    
    # gripper.goto(width=MAX_OPEN, speed=255, force=255)
    
    # gripper.grasp(10,10,0.0, -1.0, -1.0)
    gripper.grasp(
    speed=10,
    force=100.0,
    grasp_width=0.04,
    epsilon_inner=0.01,
    epsilon_outer=0.01,
    
)
    time.sleep(10)
    # for i in range(10):
    #     print(f"Iteration {i + 1}/20")
    #     # Reset
    #     # robot.go_home()
    #     gripper.goto(width=MAX_OPEN, speed=255, force=255)
    #     # Get joint positions
    #     joint_positions = robot.get_joint_positions()
    #     print(f"Current joint positions: {joint_positions}")
    #     # joint_positions_desired=[ joint_positions[0], joint_positions[1],  6.2601756 ,  2.02332066  ,4.67557344 , 3.16920431,6.31079696]
    #     # Command robot to pose (move 4th and 6th joint)
    #     # joint_positions_desired = torch.Tensor(
    #     #     [0, 0, 0, 0.0, 0, 0, 0.0]
    #     # )
    #     # print(f"\nMoving joints to: {joint_positions_desired} ...\n")
    #     # state_log = robot.move_to_joint_positions(joint_positions_desired, time_to_go=10.0)
    #     print("Done moving joints")
    #     # gripper.goto(width=MIN_CLOSE, speed=255, force=255)
    #     # **正弦运动控制夹爪**
    #     # time.sleep(1)
    #     # start_time = time.time()
    #     # while time.time() - start_time < DURATION:
    #     #     t = time.time() - start_time
    #     #     width = MIN_CLOSE + (MAX_OPEN - MIN_CLOSE) * (  math.sin(2 * math.pi * FREQ * t))
    #     #     gripper.goto(width=width, speed=255, force=255)
    #     #     time.sleep(0.001)  # 控制频率 20Hz
    #     # Get updated joint positions
    #     joint_positions = robot.get_joint_positions()
    #     print(f"New joint positions: {joint_positions}")
