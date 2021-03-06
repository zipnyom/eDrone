// this is a test statement
#include <mavlink/v2.0/common/mavlink.h>
#include <ros/ros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <std_msgs/String.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <sensor_msgs/NavSatFix.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Vector3Stamped.h>
#include <mavros_msgs/HomePosition.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/CommandTOL.h>
#include <mavros_msgs/CommandLong.h>
#include <mavros_msgs/StreamRate.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/GlobalPositionTarget.h>
#include <eDrone_lib/Vehicle.h>
#include <eDrone_lib/GeoInfo.h>
#include <eDrone_lib/GeoUtils.h>
#include <eDrone_msgs/Arming.h> // 시동 서비스 헤더 파일
#include <eDrone_msgs/Goto.h> // 무인기 위치 이동 서비스 헤더 파일 포함
#include <eDrone_msgs/ModeChange.h> // 비행 모드 변경 서비스 헤더 파일
#include <eDrone_msgs/RTL.h> // RTL
#include <eDrone_msgs/Target.h> // 현재 목적지 topic 메시지가 선언된 헤더 파일 포함

#include <eDrone_msgs/GeofenceSet.h> // GeofenceSet 서비스 헤더 파일
#include <eDrone_msgs/GeofenceCheck.h> // GeofenceCheck 서비스 헤더 파일
#include <eDrone_msgs/GeofenceReset.h> // GeofenceReset 서비스 헤더 파일


//#include <eDrone_msgs/Geofence.h> // Geofence 서비스 헤더 파일
#include <eDrone_msgs/NoflyZoneSet.h> // NoflyZone 서비스 헤더 파일 
#include <eDrone_msgs/NoflyZoneCheck.h>
#include <eDrone_msgs/NoflyZoneReset.h>
#include <eDrone_msgs/Survey.h> // Survey 서비스 헤더 파일
#include <eDrone_msgs/Survey_New.h> // Survey 서비스 (다각형 영역) 헤더 파일 
#include <eDrone_msgs/MissionAddItem.h> // 미션 아이템 추가 서비스 호출 
#include <eDrone_msgs/MissionUpload.h> // 미션 업로드 서비스 호출
#include <eDrone_msgs/MissionDownload.h> // 미션 다운로드 서비스 호출
#include <eDrone_msgs/MissionClear.h> // 미션 제거 서비스 호출
#include <eDrone_lib/GeoUtils.h> // 좌표변환함수 선언 헤더파일 
#include <eDrone_lib/params.h> // 파라미터 목록

using namespace std;
using namespace Mission_API; 
using namespace mavros_msgs;


typedef struct _str_target_position
{
	int target_seq_no; // 목적지 순번 (1, 2, 3, ...)
	bool is_global; // 좌표 유형 (true: global coord, false: local coord)
	
	Point pos_local; // (x, y, z)
	GeoPoint pos_global; // (lat, long, alt)

	bool reached; // 목적지 도달 여부 (true: 목적지 도착, false: 목적지로 이동 중)
	bool geofence_breach; // geofence 영역 위반 여부 (true: geofence 영역 밖, false: 영역 내) 
} Target_Position;

// (2018.03.22)

typedef struct c_str_geofence
{
	bool isSet; // geofence 설정 여부
	double radius; 

} Geofence_Info;

// double geofence_radius;



//(2018.02.26) 

typedef struct c_str_nofly_Zone
{
        string ref_system; // 비행금지구역 저장 좌표계 (현재는 WGS84로 저장)
        bool isSet; // 비행 금지 구역 설정 여부 (true: 설정, false: 미 설정)
        double pt1_arg1;
        double pt1_arg2;
        double pt1_arg3;
        double pt2_arg1;
        double pt2_arg2;
        double pt2_arg3;

} Nofly_Zone;


//// 주요 변수


// survey 관련

// noflyZone 관련

// geofence 관련



//std::vector<eDrone_msgs::Target> path; // 무인기 자율 비행 경로 


vector<mavros_msgs::Waypoint> boundary_points; // 영역 경계점 목록 
vector<mavros_msgs::Waypoint> flightPath; // 무인기 비행 경로
vector<mavros_msgs::Waypoint> waypoints_outside; // 영역 밖 웨이포인트 목록 (초기 비행 경로)

Geofence_Info geofence_info; // 가상 울타리 정보 
Nofly_Zone nofly_zone; // 비행 금지 구역 변수 


vector<Point> polygon_area;
Point point;

double path_width; // 비행 경로 너비


double min_x_lat; // survey 범위 - 최소 x (또는 위도) 값
double max_x_lat; // 	"	- 최대 x (또는 위도) 값

double min_y_long; // 	"	- 최소 y (또는 경도) 값

double max_y_long; // 	"	- 최대 y (또는 경도) 값


//// Survey 서비스 피요청 여부
bool survey_srv_called = false;


//// topic 메시지 변수 선언

// (무인기 상태 정보 수신 목적)
mavros_msgs::State current_state; // 무인기 상태 정보

// (무인기 위치 확인 목적)
geometry_msgs::PoseStamped current_pos_local; // 현재 위치 및 자세 정보 (지역 좌표)
sensor_msgs::NavSatFix current_pos_global; // 현재 위치 정보 (전역 좌표)


// (홈 위치 획득 목적)
mavros_msgs::HomePosition home_position; // home position



// (무인기 위치 이동 목적)
geometry_msgs::PoseStamped target_pos_local; // 목적지 위치 정보 (지역 좌표)
mavros_msgs::GlobalPositionTarget target_pos_global; // 목적지 위치 정보 (전역 좌표)
eDrone_msgs::Target cur_target; // 현재 목적지 정보 (publisher: eDrone_control_node, subscriber: eDrone_application_node)

eDrone_msgs::Target next_target; // 다음  목적지 정보 (goto 서비스 요청에 사용)
int cur_target_seq_no = -1; // survey 기능 수행 시 목적지 순번 (0, 1, 2, ...)


//// 서비스 요청 메시지 선언 
// (mavros)
mavros_msgs::CommandBool arming_cmd;
mavros_msgs::CommandLong commandLong_cmd;// 무인기 제어에 사용될 서비스 선언
mavros_msgs::SetMode modeChange_cmd; // 모드 변경에 사용될 서비스 요청 메시지
mavros_msgs::SetMode rtl_cmd; // 복귀 명령에 사용될 서비스 요청 메시지i


// (eDrone_msgs)
//eDrone_msgs::Goto goto_cmd; // goto 요청 메시지
eDrone_msgs::MissionAddItem missionAddItem_cmd;
eDrone_msgs::MissionUpload missionUpload_cmd;
eDrone_msgs::MissionDownload missionDownload_cmd;
eDrone_msgs::MissionClear missionClear_cmd;
vector<mavros_msgs::Waypoint> waypoints;


// publisher 선언

//ros::Publisher state_pub;
ros::Publisher pos_pub_local;
ros::Publisher pos_pub_global;


// subscriber 선언

ros::Subscriber state_sub;
ros::Subscriber pos_sub_local;
ros::Subscriber pos_sub_global;
ros::Subscriber home_sub; 
ros::Subscriber cur_target_sub; // 현재 목적지 정보 구독 

// 서비스 서버 선언

/*
ros::ServiceServer noflyZoneSet_srv_server;
ros::ServiceServer noflyZoneCheck_srv_server;
ros::ServiceServer noflyZoneReset_srv_server;
ros::ServiceServer survey_srv_server;
ros::ServiceServer survey_new_srv_server;
*/



//서비스 클라이언트 선언
/*
ros::ServiceClient arming_client; // 서비스 클라이언트 선언
ros::ServiceClient modeChange_client; // 모드 변경 서비스 클라이언트 
ros::ServiceClient rtl_client; // 모드 변경 서비스 클라이언트 
//ros::ServiceClient goto_client; // goto 서비스 클라이언트 
ros::ServiceClient missionAddItem_client; // 미션 아이템 추가 클라이언트
ros::ServiceClient missionUpload_client; // 미션 업로드  클라이언트
ros::ServiceClient missionDownload_client; // 미션 다운로드  클라이언트
ros::ServiceClient missionClear_client; // 미션 제거  클라이언트
*/

// home position

 float HOME_LAT ;
 float HOME_LON;
 float HOME_ALT;


 double survey_altitude = 10;

/*
void cur_target_cb (const eDrone_msgs::Target::ConstPtr& msg)
{
	cur_target = *msg;
		
}
*/

//// callback 함수 


// topic 구독 
void state_cb(const mavros_msgs::State::ConstPtr& msg){
	
	current_state = *msg;

		

	if (current_state.mode.compare("AUTO.RTL") ==0)
	{
//		ROS_INFO("state_cb(): FLIGHT MODE = RTL");
	}
/*
	if (cState.connected)
	{
 	  printf("A UAV is connected\n");
	}
	

	else
	{
  	  printf("A UAV is not connected\n");
	}

	if (cState.armed)
	{
	  printf("A UAV is armed\n");
	} 
	else
	{
  	  printf("A UAV is not armed\n");
	}
	
	cout << "flight mode:" <<  cState.flight_mode << endl;
*/
}


void pos_cb_local(const geometry_msgs::PoseStamped::ConstPtr& msg){

	current_pos_local = *msg;

}

void pos_cb_global(const sensor_msgs::NavSatFix::ConstPtr& msg){

	

	current_pos_global = *msg;

}



void homePosition_cb(const mavros_msgs::HomePosition::ConstPtr& msg)
{
	home_position = *msg;
	
//	printf("home position: (%f, %f, %f) \n", home_position.position.x, home_position.position.y, home_position.position.z);

	HOME_LAT = home_position.geo.latitude;
	HOME_LON = home_position.geo.longitude;		
	HOME_ALT = home_position.geo.altitude;

	
//	printf("home position: (%f, %f, %f) \n", home.latitude, home.longitude, home.altitude);	
}


// 서비스 요청 응답 

bool srv_geofenceSet_cb (eDrone_msgs::GeofenceSet::Request & req, eDrone_msgs::GeofenceSet::Response & res)
{
  ROS_INFO ("eDrone_application_node: GeofenceSet servive was called " ) ;

  if (req.radius <= 0)
  { 	
	res.value = false;
	return true;
  }


  geofence_info.isSet = true;
  geofence_info.radius = req.radius;

   

/*
  if ( )
  {
	
  }
*/
  res.value = true;
  return true;
}

bool srv_geofenceReset_cb ( eDrone_msgs::GeofenceReset::Request & req, eDrone_msgs::GeofenceReset::Response & res)
{
 ROS_INFO ("eDrone_application_node: GeofenceReset servive was called " ) ;
 geofence_info.isSet = false;
 res.value = true;
 return true; 
}

bool srv_geofenceCheck_cb (eDrone_msgs::GeofenceCheck::Request & req, eDrone_msgs::GeofenceCheck::Response & res )
{

 // 

 // HOME 으로부터의 거리 계산


  ROS_INFO ("ex_application_node: GeofenceCheck service was called: ");

  res.violation = false;

   if ( req.ref_system.compare("WGS84") ==0)
  {

  // WGS84 좌표를 ENU 좌표로 변환

	Point point = convertGeoToENU(req.arg1, req.arg2, req.arg3, HOME_LAT, HOME_LON, HOME_ALT);


	double distance = pow ( point.x, (double) 2.0) + pow ( point.y , (double) 2.0 ); 

	distance = sqrt (distance);

	ROS_INFO ("ex_application_node: the distance from HOME is %lf", distance);


	if (geofence_info.isSet == true && distance >= geofence_info.radius)
	{
		res.violation = true;
	}

	// 해당 거리가 Home으로부터 가상 울타리까지의 거리보다 멀 경우, 응답 메시지 내 violation 필드를 false로 설정

  }


 res.value = true;
 return true;
}



bool srv_noflyZoneSet_cb (eDrone_msgs::NoflyZoneSet::Request &req, eDrone_msgs::NoflyZoneSet::Response &res )
{
	 // reference system: WGS84 지원

	  ROS_INFO ("eDrone_application_node: NoflyZoneSet service was called");

	  if ( req.ref_system.compare("WGS84") ==0)
	  {
		nofly_zone.pt1_arg1 = req.pt1_arg1;
		nofly_zone.pt1_arg2 = req.pt1_arg2;
		nofly_zone.pt1_arg3 = req.pt1_arg3;
		nofly_zone.pt2_arg1 = req.pt2_arg1;
		nofly_zone.pt2_arg2 = req.pt2_arg2;
		nofly_zone.pt2_arg3 = req.pt2_arg3;
	  }
	  nofly_zone.isSet = true;
	  res.value = true;

	  return true;

	
}


bool srv_noflyZoneCheck_cb(eDrone_msgs::NoflyZoneCheck::Request &req, eDrone_msgs::NoflyZoneCheck::Response &res)
{
	res.value= false;
	ROS_INFO ("eDrone_application_node: NoflyZoneCheck service was called");

	// 요청 메시지 포함된 좌표가  

	  if ( (nofly_zone.isSet == true) &&  ( req.ref_system.compare("WGS84") ==0) )
	  {
//			ROS_INFO ("Fist condition");

		if ( (req.arg1 < nofly_zone.pt1_arg1 ) !=  (req.arg1 < nofly_zone.pt2_arg1 ) )
		{

//			ROS_INFO ("Second condition");

			if ((req.arg2 < nofly_zone.pt1_arg2) != (req.arg2 < nofly_zone.pt2_arg2) )
			{

				ROS_INFO ("eDrone_control_node: NoflyZone violation!");
				res.violation = true;
			}
		}
	  }

	  res.value = true;
	  return true;

}


bool srv_noflyZoneReset_cb(eDrone_msgs::NoflyZoneReset::Request &req, eDrone_msgs::NoflyZoneReset::Response &res) 
{
	
	ROS_INFO ("eDrone_application_node: NoflyZoneReset service was called");
	nofly_zone.isSet = false;

	res.value = true;
	return true;
}


/*
bool srv_survey_cb(eDrone_msgs::Survey::Request &req, eDrone_msgs::Survey::Response &res)
{
	
	std::cout << "srv_survey_cb(): survey a target area"  << endl; 
	
	survey_srv_called = true;

	cur_target_seq_no = 0;

	// 변수 선언

	double row_col_width = 0;

	double min_x_lat = 0;

	double min_y_long = 0;

	double max_x_lat = 0;

	double max_y_long = 0;

	double x_lat = 0;

	double y_long = 0;

	int row = 0; // 행 번호

	int col = 0;	// 열 번호

	int num_rows = 0; // 행 개수 

	int num_cols = 0; // 열 개수 

	int target_seq_no = 0; // 목표 지점 순번

	min_x_lat = req.min_x_lat;
	
	min_y_long = req.min_y_long;
	
	max_x_lat = req.max_x_lat;

	max_y_long = req.max_y_long;
	
	row_col_width = req.row_col_width;

	num_rows = (max_x_lat - min_x_lat) / row_col_width;

	num_cols = (max_y_long - min_y_long) / row_col_width;

	
	ROS_INFO("min_x_lat: %lf", min_x_lat);
	ROS_INFO("min_y_long: %lf", min_y_long);
	ROS_INFO("max_x_lat: %lf", max_x_lat);
	ROS_INFO("max_y_long: %lf", max_y_long);

	ROS_INFO("row_col_width: %lf", row_col_width);

	ROS_INFO("num_rows: %d", num_rows);
	ROS_INFO("num_cols: %d", num_cols);

	//sleep(10);


	// 최초 목적지 설정

	next_target.target_seq_no = 0;

	next_target.is_global = req.is_global;

	next_target.x_lat = req.min_x_lat;
	
	next_target.y_long = req.min_y_long;
		
	if (next_target.is_global == true) // 절대 고도일 경우 높이 지정
	{
		next_target.z_alt = HOME_ALT + ALTITUDE;
	}
	else // 상대 고도일 경우 높이 지정
	{
		next_target.z_alt = ALTITUDE;
	}


	next_target.reached = false;
		

	// 경로 좌표 계산 & 경로 변수 값  설정

			
	for (col = 0; col <= num_cols; col++)
	{

		x_lat = min_x_lat + col * row_col_width; 


		for (row = 0; row <= num_rows; row++)
		{
			if (col %2 ==0)
			{
				y_long = min_y_long + row * row_col_width;
			}
			else
			{
				y_long = max_y_long - row * row_col_width;
			}
	

			eDrone_msgs::Target target;

			target.is_global = req.is_global;
			target.x_lat = x_lat;
			target.y_long = y_long;
			
			if (target.is_global == true) // 절대 고도일 경우 높이 지정
			{
				target.z_alt = HOME_ALT + ALTITUDE;
			}
			else // 상대 고도일 경우 높이 지정
			{
				target.z_alt = ALTITUDE;
			}
			
			target.reached = false;
		
			path.push_back(target);
			
			ROS_INFO("target seq no. %d (%lf, %lf)", target_seq_no, x_lat, y_long);			

			target_seq_no++;
		}
	}



	return true;

}
*/

bool srv_survey_new_cb(eDrone_msgs::Survey_New::Request &req, eDrone_msgs::Survey_New::Response &res)
{
	
	survey_srv_called = true;
	boundary_points = req.boundary_points;
	path_width = req.path_width;
	survey_altitude = req.altitude;

	std::cout << "srv_survey_new_cb(): survey a target area"  << endl; 
	return true;
}


// 비행경로 생성 함수
// generateFlightPath

vector<mavros_msgs::Waypoint> generateFlightPath (vector<mavros_msgs::Waypoint> boundary_points)
{
	vector<mavros_msgs::Waypoint> flightPath;

	// 비행 경로 계산  
	/* 다각형 구성  - x(또는 위도) & y (또는 경도) 범위 */
	
	cout << " generateFlightpath() was called" << endl;

	for (int i = 0; i < boundary_points.size(); i++)
	{

		cout << " boundary points["<< i << "]: " << endl;

		cout << " x_lat: " << boundary_points[i].x_lat << endl;
		cout << " y_long: " << boundary_points[i].y_long << endl << endl;

		if (i==0)
		{
			min_x_lat = boundary_points[i].x_lat;
			max_x_lat = boundary_points[i].x_lat;
			min_y_long = boundary_points[i].y_long;
			max_y_long = boundary_points[i].y_long;
		}

		else 
		{
			if (boundary_points[i].x_lat < min_x_lat)
			{
				min_x_lat = boundary_points[i].x_lat;
			}
			if (boundary_points[i].x_lat > max_x_lat)
			{
				max_x_lat = boundary_points[i].x_lat;
			}
			if (boundary_points[i].y_long < min_y_long)
			{
				min_y_long = boundary_points[i].y_long;
			}
			if (boundary_points[i].y_long > max_y_long)
			{
				max_y_long = boundary_points[i].y_long;
			}
		}

	}

	/* 가상 영역(사각형)  생성 */

	cout << "min_x_lat: " << min_x_lat << endl;
	cout << "max_x_lat: " << max_x_lat << endl;
	cout << "min_y_long: " << min_y_long << endl;
	cout << "max_y_long: " << max_y_long << endl;


	/* 초기 경로 생성 */

	double x_lat = min_x_lat;
	double y_long = min_y_long;

	int path_count = 0; // 초기 경로를 구성하는 직선 구간 번호

	while (x_lat < max_x_lat + path_width) // min_x에서 max_x까지 진행 
	{

		mavros_msgs::Waypoint waypoint;

		if (path_count %2 ==0) // 짝수 번째는 북쪽 방향, 홀수 번째는 남쪽 방향 
		{
			y_long = min_y_long;

			while (y_long < max_y_long + path_width)
			{
				waypoint.x_lat = x_lat;
				waypoint.y_long = y_long;
				waypoint.z_alt = survey_altitude;
				y_long += path_width;
	
				flightPath.push_back (waypoint);
				/*
				cout << "waypoint[" << flightPath.size()  << "]: " << endl;
				cout << " path_count: " << path_count << endl;	
				cout << " x_lat: " << waypoint.x_lat << endl;
				cout << " y_long: " << waypoint.y_long << endl << endl;
				*/
			}  
		}

		else  // if (path_count %2 ==1)
		{
			y_long = max_y_long;

			while (y_long > min_y_long- path_width )
			{
				
				waypoint.x_lat = x_lat;
				waypoint.y_long = y_long;

				y_long -= path_width;
				flightPath.push_back(waypoint);
				/*
				cout << "waypoint[" << flightPath.size()  << "]: " << endl;
				cout << " path_count: " << path_count << endl;	
				cout << " x_lat: " << waypoint.x_lat << endl;
				cout << " y_long: " << waypoint.y_long << endl << endl;
				*/
			}
				
		}

		x_lat += path_width;
		path_count++;

	}// 비행 경로 구성

	/* 비행경로 화면 출력  */

	cout << "initial flight path: " <<endl;
	
	for (int i = 0; i < flightPath.size(); i++)
	{
		
		mavros_msgs::Waypoint wp = flightPath[i];

		cout << "waypoint[" << i << "]: " ;

		cout << " x_lat: " << wp.x_lat  ;
		cout << ", y_long: " << wp.y_long << endl;
	}



	/* 영역 밖 웨이포인트 제거 */
	

	// polygon 구성 (ENU 좌표)

	//vector<Point> polygon_area;

	Point point;
	
	for (int i =0; i < boundary_points.size(); i++)
	{
		Point point;

		point.x= boundary_points[i].x_lat;
		point.y = boundary_points[i].y_long;
		polygon_area.push_back(point);
	}

	//Vector<Point> waypoints_outside; // 영역 밖 웨이포인트 목록 

	for (int i = 0; i < flightPath.size(); i++)
	{
		Point point;

		point.x = flightPath[i].x_lat;
		point.y = flightPath[i].y_long;

		if ( isInside (point, polygon_area) != true)
		{
			waypoints_outside.push_back(flightPath[i]);	
		}

	} 


		// 영역 밖 웨이포인트 목록 구성 

	return flightPath;
}


int main(int argc, char** argv)

{
	cout << "eDrone_application_node" << endl;
	ros::init(argc, argv, "eDrone_application_node");

 	ros::NodeHandle nh; 
	
	ros::Rate rate (20.0); 
	
	
	// publisher 초기화
	pos_pub_local = nh.advertise<geometry_msgs::PoseStamped>("mavros/setpoint_position/local", 10);
        pos_pub_global = nh.advertise<mavros_msgs::GlobalPositionTarget>("mavros/setpoint_raw/global", 10);

	// subscriber 초기화
	state_sub = nh.subscribe<mavros_msgs::State> ("mavros/state", 10, state_cb);
	pos_sub_local = nh.subscribe<geometry_msgs::PoseStamped> ("mavros/local_position/pose",10,  pos_cb_local); 
	pos_sub_global = nh.subscribe<sensor_msgs::NavSatFix> ("mavros/global_position/global",10,  pos_cb_global); 
	home_sub = nh.subscribe<mavros_msgs::HomePosition> ("mavros/home_position/home", 10, homePosition_cb);
	
	//cur_target_sub = nh.subscribe<eDrone_msgs::Target> ("eDrone_msgs/current_target", 10, cur_target_cb);


	//// 서비스 서버 선언 & 초기화
	
	ros::ServiceServer noflyZoneSet_srv_server = nh.advertiseService("srv_noflyZoneSet", srv_noflyZoneSet_cb);

	ros::ServiceServer noflyZoneCheck_srv_server = nh.advertiseService("srv_noflyZoneCheck", srv_noflyZoneCheck_cb);

	ros::ServiceServer noflyZoneReset_srv_server = nh.advertiseService("srv_noflyZoneReset", srv_noflyZoneReset_cb);

	ros::ServiceServer survey_new_srv_server = nh.advertiseService("srv_survey_new", srv_survey_new_cb);


	ros::ServiceServer geofenceSet_srv_server = nh.advertiseService("srv_geofenceSet", srv_geofenceSet_cb);
	ros::ServiceServer geofenceReset_srv_server = nh.advertiseService("srv_geofenceReset", srv_geofenceReset_cb);
	ros::ServiceServer geofenceCheck_srv_server = nh.advertiseService("srv_geofenceCheck", srv_geofenceCheck_cb);
	
//	int cur_target_seq_no = -1; // survey 기능 수행 시 목적지 순번 (0, 1, 2, ...)

	//modeChange_srv_server = nh.advertiseService("srv_modeChange", srv_modeChange_cb);

	//// 서비스 클라이언트 선언
	ros::ServiceClient arming_client = nh.serviceClient<mavros_msgs::CommandBool> ("mavros/cmd/arming");	
 // 서비스 클라이언트 선언
	ros::ServiceClient modeChange_client; // 모드 변경 서비스 클라이언트 
	ros::ServiceClient rtl_client = nh.serviceClient<mavros_msgs::SetMode> ("/mavros/set_mode");

 // 모드 변경 서비스 클라이언트 
	//ros::ServiceClient goto_client; // goto 서비스 클라이언트 

	
 	ros::ServiceClient missionAddItem_client =nh.serviceClient<eDrone_msgs::MissionAddItem>("srv_missionAddItem"); // 미션 아이템 추가 클라이언트

	ros::ServiceClient missionUpload_client =nh.serviceClient<eDrone_msgs::MissionUpload>("srv_missionUpload");// 미션 아이템  클라이언트

	ros::ServiceClient missionDownload_client =nh.serviceClient<eDrone_msgs::MissionDownload>("srv_missionDownload"); // 미션목록 다운로드
	ros::ServiceClient missionClear_client =nh.serviceClient<eDrone_msgs::MissionClear>("srv_missionClear"); // 미션 제거 





	//goto_client = nh.serviceClient<eDrone_msgs::Goto>("srv_goto");
	
			
	while ( ros::ok() ) // 탐색 서비스 요청 메시지 처리
	{

		if (survey_srv_called == true)
		{
			
			cout << "eDrone_application_node: main(): surveying mission is ongoing" << endl;
			// 비행 경로 생성			
			flightPath = generateFlightPath (boundary_points);


			// FC 미션 제거
			ROS_INFO ("Send missionClear command...\n");
			
			if (missionClear_client.call (missionClear_cmd ) )
			{
				cout << ("missionClear command was sent... \n");
			}

 
			// 미션 목록 구성 (missionAddItem 서비스 호출)
					
			ROS_INFO ("Send missionAddItem command...\n");
			
			 // takeoff
			missionAddItem_cmd.request.frame = 3;
		        missionAddItem_cmd.request.command = MAV_CMD_NAV_TAKEOFF;
		        missionAddItem_cmd.request.is_current = 1;
		        missionAddItem_cmd.request.autocontinue = 1;
		        missionAddItem_cmd.request.param1 = 0;
		        missionAddItem_cmd.request.param2 = 0;
		        missionAddItem_cmd.request.param3 = 0;
        		missionAddItem_cmd.request.z_alt = survey_altitude;
						
		        if (missionAddItem_client.call(missionAddItem_cmd)!= true)
			{
				ROS_ERROR ("missionAddItem service failed: " ); 
			}

			 // navigation

			for (int i = 0; i < flightPath.size(); i++)
			{
				//std::vector<mavros_msgs::Waypoint>::iterator it  = std::find(waypoints_outside.begin(), waypoints_outside.end(), flightPath[i]);
				// 영역 밖 웨이포인트의 경우 제외 
				
				Point point;

				point.x = flightPath[i].x_lat;
				point.y = flightPath[i].y_long;

				if ( isInside (point, polygon_area) != true)
				{
					waypoints_outside.push_back(flightPath[i]);
					continue;	
				
				}
			/*
				if (it != waypoints_outside.end())
				{
					continue;
				}
			*/
			        missionAddItem_cmd.request.frame = 3;
			        missionAddItem_cmd.request.command = MAV_CMD_NAV_WAYPOINT;

			        missionAddItem_cmd.request.is_current = 0;

			        missionAddItem_cmd.request.autocontinue = 1;
			        missionAddItem_cmd.request.param1 = 0;
			        missionAddItem_cmd.request.param2 = 0;
			        missionAddItem_cmd.request.param3 = 0;

			        missionAddItem_cmd.request.is_global = false;

			        missionAddItem_cmd.request.x_lat = flightPath[i].x_lat;
			        missionAddItem_cmd.request.y_long = flightPath[i].y_long;
			        missionAddItem_cmd.request.z_alt = survey_altitude;
			
				if (missionAddItem_client.call(missionAddItem_cmd)!=true)
				{
					ROS_ERROR("missionAddItem service failed " );
				}
			}


			 // return			
			        missionAddItem_cmd.request.frame = 3;
			        missionAddItem_cmd.request.command = MAV_CMD_NAV_WAYPOINT;

			        missionAddItem_cmd.request.is_current = 0;
			        missionAddItem_cmd.request.autocontinue = 1;
			        missionAddItem_cmd.request.param1 = 0;
			        missionAddItem_cmd.request.param2 = 0;
			        missionAddItem_cmd.request.param3 = 0;

			        missionAddItem_cmd.request.is_global = true;

			        missionAddItem_cmd.request.x_lat = HOME_LAT ;
			        missionAddItem_cmd.request.y_long = HOME_LON;
			        missionAddItem_cmd.request.z_alt = survey_altitude;
			
				if (missionAddItem_client.call(missionAddItem_cmd)!=true)
				{
					ROS_ERROR("missionAddItem service failed " );
				}
/*			
			missionAddItem_cmd.request.frame = 3;
			missionAddItem_cmd.request.command = MAV_CMD_NAV_RETURN_TO_LAUNCH;
		        missionAddItem_client.call(missionAddItem_cmd);
			
			if (missionAddItem_client.call(missionAddItem_cmd)!=true)
			{
				ROS_ERROR("missionAddItem service failed " );
			}
		*/	


			/* 착륙 */

			// mission item - wp (Land)
		 	

		     	missionAddItem_cmd.request.frame = 3;
		        missionAddItem_cmd.request.command = MAV_CMD_NAV_LAND;
	
			missionAddItem_cmd.request.is_current = 0;
			missionAddItem_cmd.request.autocontinue = 1;
				
			if (missionAddItem_client.call(missionAddItem_cmd)!=true)
			{
				ROS_ERROR("missionAddItem service failed " );
			}
 	

	
			// 미션 업로드 
			ROS_INFO("Send missionUpload command ... \n");

        		if (!missionUpload_client.call(missionUpload_cmd))
			{
				ROS_INFO("missionUpload command was sent\n");
			}
			else
			{
				ROS_ERROR ("missionUpload service failed ");
			}

			survey_srv_called = false;
			
	
		}		


		ros::spinOnce();
      	  	rate.sleep();

	}

	return 0;
}
