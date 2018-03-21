

/* include */

// 기본 header (ROS & C/C++)
#include <mavlink/v2.0/common/mavlink.h>
#include <ros/ros.h>
#include <iostream>
#include <std_msgs/String.h>
#include <vector>
#include <stdlib.h>

// 토픽 선언 header 
#include <eDrone_msgs/Target.h>

// 서비스 선언 header
#include <eDrone_msgs/CheckState.h>
#include <eDrone_msgs/CheckPosition.h>
#include <eDrone_msgs/Arming.h>
#include <eDrone_msgs/Takeoff.h>
#include <eDrone_msgs/Landing.h>
#include <eDrone_msgs/Goto.h>
#include <eDrone_msgs/RTL.h>
#include <eDrone_msgs/Target.h>

// 파라미터 선언 header
#include <eDrone_examples/params.h>

using namespace std;

/* 포인터 변수 선언  */

ros::NodeHandle* nh_ptr; // node handle pointer (서버/클라이언트 또는 퍼블리셔/서브스크라이버 선언에 사용)

eDrone_msgs::Target* cur_target_ptr; // cur_target 변수 접근을 위한 포인터 변수 



/* 콜백 함수 정의 */

// 토픽 콜백 함수

void cur_target_cb(const eDrone_msgs::Target::ConstPtr& msg)
{
	*cur_target_ptr = *msg;

	// 현재 목적지 도달 여부 확인
	ROS_INFO("cur_target_cb(): \n");
	ROS_INFO("current target: %d \n", cur_target_ptr->target_seq_no);
	
	if (cur_target_ptr->reached == true)
	{
		ROS_INFO("we reached at the current target\n");
	} 
}

// 서비스 콜백 함수 (내용 없음) 


/* main 함수 */

int main(int argc, char** argv)
{
	ROS_INFO("==ex_goto==\n");

	ros::init(argc, argv, "ex_goto"); 
	ros::NodeHandle nh;
	nh_ptr = &nh; // node handle 주소 저장 


	/* 주요 변수 선언 */

	// main argument 값 저장을 위한 변수 선언 (서비스 파라미터 설정)

	double altitude = 0;

	vector <double> x_vector; // x 좌표 벡터 
	vector <double> y_vector; // y 좌표 벡터

	if (argc < 2)
	{
		ROS_ERROR("ex_goto: the number of arguments should be at least 2!!" );
		return -1;
	}


	for (int arg_index = 0; arg_index < argc; arg_index++)
	{
		ROS_INFO("main arg[%d]: %s", arg_index, argv[arg_index] );
	}

	altitude = atof (argv[1]); 
	
	ROS_INFO("Altitude: %lf", altitude);

	for (int arg_index = 2; arg_index < argc; arg_index+=2)
	{
		double x = atof (argv[arg_index]) ;
		double y = atof (argv[arg_index+1]);

		x_vector.push_back(x);
		y_vector.push_back(y);
/*
		ROS_INFO("(X, Y): (%lf, %lf) ", x_vector[arg_index], y_vector[arg_index]);
		cout << "(X, Y): " << x_vector[arg_index] << ", " << y_vector[arg_index] << endl;
		ROS_INFO("(X, Y): (%s, %s) ", argv[arg_index], argv[arg_index+1]);
*/
	}

	for (int vector_index = 0; vector_index < x_vector.size(); vector_index++)
	{
		ROS_INFO("(X, Y): (%lf, %lf) ", x_vector[vector_index], y_vector[vector_index]);
		
	}

	// 토픽 메시지 변수 선언  
	eDrone_msgs::Target cur_target; // 무인기가 현재 향하고 있는 목적지 (경유지)
	cur_target_ptr = &cur_target; // cur_target 변수 주소 저장 
	eDrone_msgs::Target next_target; // 다음 목적지 

	// 서비스 메시지 변수 선언 
	eDrone_msgs::CheckState checkState_cmd;
	eDrone_msgs::CheckPosition checkPosition_cmd;
	eDrone_msgs::Arming arming_cmd;
	eDrone_msgs::Takeoff takeoff_cmd;
	eDrone_msgs::Landing landing_cmd;
	eDrone_msgs::Goto goto_cmd;
	eDrone_msgs::RTL rtl_cmd;

	// 토픽 publisher 초기화 (내용 없음)

	// rate 설정 
	ros::Rate rate(20.0);

	// 토픽 subscriber 선언 & 초기화 

	ros::Subscriber cur_target_sub = nh.subscribe("eDrone_msgs/current_target", 10, cur_target_cb); // 

	// 서비스 서버 선언 & 초기화 (내용 없음)

	// 서비스 클라이언트 선언 & 초기화

	ros::ServiceClient checkState_client =nh.serviceClient<eDrone_msgs::CheckState>("srv_checkState");  
        ros::ServiceClient checkPosition_client =nh.serviceClient<eDrone_msgs::CheckPosition>("srv_checkPosition"); 
	ros::ServiceClient arming_client =nh.serviceClient<eDrone_msgs::Arming>("srv_arming");
	ros::ServiceClient takeoff_client =nh.serviceClient<eDrone_msgs::Takeoff>("srv_takeoff");
	ros::ServiceClient landing_client =nh.serviceClient<eDrone_msgs::Landing>("srv_landing");
	ros::ServiceClient goto_client = nh.serviceClient<eDrone_msgs::Goto>("srv_goto");
	ros::ServiceClient rtl_client = nh.serviceClient<eDrone_msgs::RTL>("srv_rtl");
	
	// 무인기 자율 비행 경로 
	std::vector<eDrone_msgs::Target> path; 


	// 서비스 요청

	  

	    // path 설정
	
		int cur_target_seq_no = -1; // 현재 target 순번 (0, 1, 2, ...)
		
		for (int vector_index = 0 ; vector_index < x_vector.size(); vector_index++)
		{
			next_target.target_seq_no = vector_index;
			next_target.is_global = false;
			next_target.x_lat = x_vector[vector_index];
			next_target.y_long = y_vector[vector_index];
			next_target.z_alt = altitude;
			next_target.reached = false;
			path.push_back(next_target);
		}

		/*


		// target#1	
		next_target.target_seq_no = 0;
		next_target.is_global = false;
		next_target.x_lat = GOTO_1_X_LAT;  
		next_target.y_long = GOTO_1_Y_LONG;
		next_target.z_alt = altitude;
		next_target.reached = false;
		path.push_back(next_target);

		// target#2	
		next_target.target_seq_no = 1;
		next_target.is_global = false;
		next_target.x_lat = GOTO_2_X_LAT;    
		next_target.y_long = GOTO_2_Y_LONG;
		next_target.z_alt = altitude;
		next_target.reached = false;
		path.push_back(next_target);

		// target#3 	
		next_target.target_seq_no = 2;
		next_target.is_global = false;
		next_target.x_lat = GOTO_3_X_LAT;                                                                           
		next_target.y_long = GOTO_3_Y_LONG;
		next_target.z_alt = altitude;
		next_target.reached = false;
		path.push_back(next_target);
		*/


	  sleep(10);	

	    // 연결 상태 확인


		ROS_INFO("Send checkState command ... \n");
		ROS_INFO("Checking the connection ... \n");
		
		if (checkState_client.call(checkState_cmd))
		{
			ROS_INFO ("CheckState service was requested");
			
			while (checkState_cmd.response.connected == false)
			{
				if (checkState_client.call(checkState_cmd))
				{
					ROS_INFO ("Checking state...");
				}

				ros::spinOnce();
				rate.sleep();
			}

			ROS_INFO("UAV connection established!");
		}


	    // 무인기 위치 확인

		 ROS_INFO("Send checkPosition command ... \n");
		 ROS_INFO("Checking the position ... \n");

		 if (checkPosition_client.call(checkPosition_cmd))
		 {
			ROS_INFO ("CheckPosition service was requested");

			while (checkPosition_cmd.response.value == false)
			{
				if (checkPosition_client.call(checkPosition_cmd));
				{
					ROS_INFO ("Checking position...");
				}

				ros::spinOnce();
				sleep(10);
			}


			cout <<"global frame: (" << checkPosition_cmd.response.latitude << ", ";
			cout << checkPosition_cmd.response.longitude << ", ";
			cout << checkPosition_cmd.response.altitude << ") " << endl << endl;

			cout <<"local frame: (" << checkPosition_cmd.response.x << ", ";
			cout << checkPosition_cmd.response.y << ", " << checkPosition_cmd.response.z << ") " << endl;
			
			}
			
			ROS_INFO("UAV position was checked!");
		
		 sleep(10); 
		// Arming

		ROS_INFO("Send arming command ... \n"); 
		arming_client.call(arming_cmd);
		
		if (arming_cmd.response.value == true)
		{
			ROS_INFO("Arming command was sent\n");
		}


		// Takeoff

		ROS_INFO("Send takeoff command ... \n");
		takeoff_cmd.request.altitude = TAKEOFF_1_ALTITUDE; // 서비스 파라미터 설정
		takeoff_client.call(takeoff_cmd); // 서비스 호출

		if (takeoff_cmd.response.value == true) // 서비스 호출 결과 확인 
		{
			ROS_INFO("Takeoff command was sent\n");
		}

		sleep(10);


	    // 경로 비행 (임무 수행)

	//// Goto


	ROS_INFO("Send goto command ...\n");
	ROS_INFO("let's start a mission! \n");


	goto_cmd.request.is_global = false;
	goto_cmd.request.x_lat = x_vector[0];
	goto_cmd.request.y_long = y_vector[0];
	goto_cmd.request.z_alt = altitude;
	
	goto_client.call(goto_cmd);
	ROS_INFO("Goto command was sent\n");
	    while(ros::ok() )
	    {
		
		if (cur_target.reached== true) // 현재 목적지에 도착한 경우
		{
			if (cur_target.target_seq_no < path.size()-1) // 경로 벡터에서 다음 목적지 정보 획득, goto 서비스 요청 
			{

				ROS_INFO("we reached at the current target. Go to the next target\n");

				//int next_target_seq_no = cur_target.target_seq_no +1;
				
				cur_target_seq_no = cur_target.target_seq_no+1;

				next_target = path[cur_target_seq_no];

				goto_cmd.request.target_seq_no = cur_target_seq_no; // target seq no 설정				
				goto_cmd.request.is_global = next_target.is_global;
				goto_cmd.request.x_lat = next_target.x_lat;				
				goto_cmd.request.y_long = next_target.y_long;				
				goto_cmd.request.z_alt = next_target.z_alt;				
				goto_client.call(goto_cmd); // 새로운 경유지로 goto service 호출	
			} 

			else
				break;
		}


		ros::spinOnce();
		rate.sleep();
		// 경유지 추가 (필요 시)			 

	    }
	 		
 	   rtl_client.call(rtl_cmd); // rtl service 호출 (복귀)
	   if (rtl_cmd.response.value == true)
	   {
		ROS_INFO("RTL command was sent\n");
	   }

	return 0; 
}


