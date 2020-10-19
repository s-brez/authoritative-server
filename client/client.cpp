#include <cmath>
#include <cstdio>
#include <ctime>
#include <thread>

#include "core.h"
#include "graphics.h"
#include "net.h"
#include "net_msgs.h"
#include "player.h"

constexpr float32  camera_x_offset = 5.0f;
constexpr float32  camera_y_offset = 7.0f;
constexpr float32  camera_z_offset = 5.0f;


struct Client_Input	{
	bool32 has_focus;
	int32 mouse_x, mouse_y, mouse_delta_x, mouse_delta_y;
	bool32 keys[256];
};

struct Client_Globals {
	Client_Input input;
};

static Client_Globals* get_client_globals(HWND window_handle) {
	return (Client_Globals*)GetWindowLongPtr(window_handle, 0);
}

LRESULT CALLBACK window_callback(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
	Client_Globals* globals = get_client_globals(window_handle);

	switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_KEYDOWN:
			if (globals->input.has_focus) {
				
				assert(w_param < 256);
				globals->input.keys[w_param] = 1;
				
				if (w_param == VK_ESCAPE) {
					ShowCursor(true);
					globals->input.has_focus = 0;
				}
			}
			break;

		case WM_KEYUP:
			if (globals->input.has_focus)
			{
				assert(w_param < 256);
				globals->input.keys[w_param] = 0;
			}
			break;

		case WM_LBUTTONDOWN:
			globals->input.keys[VK_LBUTTON] = 1;
			if (!globals->input.has_focus)
			{
				ShowCursor(false);
			}
			globals->input.has_focus = 1;
			break;

		case WM_LBUTTONUP:
			globals->input.keys[VK_LBUTTON] = 0;
			break;

		case WM_RBUTTONDOWN:
			globals->input.keys[VK_RBUTTON] = 1;
			break;

		case WM_RBUTTONUP:
			globals->input.keys[VK_RBUTTON] = 0;
			break;

		case WM_MOUSEMOVE:
			globals->input.mouse_x = l_param & 0xffff;
			globals->input.mouse_y = (l_param >> 16) & 0xffff;

			if (globals->input.has_focus)
			{
				RECT window_rect;
				GetWindowRect(window_handle, &window_rect);

				int32 mid_x = (window_rect.right - window_rect.left)/2;
				int32 mid_y = (window_rect.bottom - window_rect.top)/2;

				globals->input.mouse_delta_x += mid_x - globals->input.mouse_x;
				globals->input.mouse_delta_y += mid_y - globals->input.mouse_y;
				
				POINT cursor_pos;
				cursor_pos.x = mid_x;
				cursor_pos.y = mid_y;
				ClientToScreen(window_handle, &cursor_pos);
				SetCursorPos(cursor_pos.x, cursor_pos.y);
			}
			break;

		default:
			return DefWindowProc(window_handle, message, w_param, l_param);
			break;
	}

	return 0;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE /*prev_instance*/, LPSTR /*cmd_line*/, int cmd_show) {
	WNDCLASS window_class;
	window_class.style = 0;
	window_class.lpfnWndProc = window_callback;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = sizeof(Client_Globals*);
	window_class.hInstance = instance;
	window_class.hIcon = 0;
	window_class.hCursor = 0;
	window_class.hbrBackground = 0;
	window_class.lpszMenuName = 0;
	window_class.lpszClassName = "mog_window_class";

	ATOM window_class_atom = RegisterClass(&window_class);

	assert(window_class_atom);

	constexpr uint32 c_window_width = 1920;
	constexpr uint32 c_window_height = 1080;

	HWND window_handle; {
		LPCSTR 	window_name 	= "";
		DWORD 	style 			= WS_OVERLAPPED;
		int 	x 				= CW_USEDEFAULT;
		int 	y 				= 0;
		HWND 	parent_window 	= 0;
		HMENU 	menu 			= 0;
		LPVOID 	param 			= 0;

		window_handle 			= CreateWindowA(window_class.lpszClassName, window_name, style, x, y, c_window_width, c_window_height, parent_window, menu, instance, param);

		assert(window_handle);
	}
	ShowWindow(window_handle, cmd_show);

	UINT sleep_granularity_ms = 1;
	bool32 sleep_granularity_was_set = timeBeginPeriod(sleep_granularity_ms) == TIMERR_NOERROR;

	// Init Winsock
	if (!Net::init()) {
		return 0;
	}

	Linear_Allocator allocator;
	linear_allocator_create(&allocator, megabytes(16));

	Linear_Allocator temp_allocator;
	linear_allocator_create_sub_allocator(&allocator, &temp_allocator, megabytes(8));

	// Allocate memory for global vars
	Client_Globals* client_globals = (Client_Globals*)linear_allocator_alloc(&allocator, sizeof(Client_Globals));
	client_globals->input = {};

	SetWindowLongPtr(window_handle, 0, (LONG_PTR)client_globals);
	
	// Init graphics
	Graphics::State* graphics_state = (Graphics::State*)linear_allocator_alloc(&allocator, sizeof(Graphics::State));
	Graphics::init(graphics_state, window_handle, instance, 
					c_window_width, c_window_height, c_max_clients,
					&allocator, &temp_allocator);

	// Init socket and set artificial lag
	Net::Socket sock;
	if (!Net::socket(&sock)){
		return 0;
	}
	Net::socket_set_fake_lag_s(&sock, 0.1f, &allocator); // 100ms of fake lag

	// Init socket buffer
	constexpr uint32 c_socket_buffer_size = c_packet_budget_per_tick;
	uint8* socket_buffer = linear_allocator_alloc(&allocator, c_socket_buffer_size);

	// Set server IP endpoint
	Net::IP_Endpoint server_endpoint = Net::ip_endpoint(127, 0, 0, 1, c_port);

	uint32 join_msg_size = Net::client_msg_join_write(socket_buffer);
	if (!Net::socket_send(&sock, socket_buffer, join_msg_size, &server_endpoint)) {
		return 0;
	}

	Player_Snapshot_State* player_snapshot_states	= (Player_Snapshot_State*)	linear_allocator_alloc(&allocator, sizeof(Player_Snapshot_State) * c_max_clients);
	bool32* players_present							= (bool32*)					linear_allocator_alloc(&allocator, sizeof(bool32) * c_max_clients);
	Matrix_4x4* mvp_matrices						= (Matrix_4x4*)				linear_allocator_alloc(&allocator, sizeof(Matrix_4x4) * (c_max_clients + 1)); // todo(jbr) all these arrays should be named singular not plural

	Player_Snapshot_State*	local_player_snapshot_state			= (Player_Snapshot_State*)	linear_allocator_alloc(&allocator, sizeof(Player_Snapshot_State));
	Player_Extra_State*		local_player_extra_state			= (Player_Extra_State*)		linear_allocator_alloc(&allocator, sizeof(Player_Extra_State));
	*local_player_snapshot_state = {};
	*local_player_extra_state = {};

	struct Predicted_Move {
		float32 dt;
		Player_Input input;
	};
	struct Predicted_Move_Result {
		Player_Snapshot_State snapshot_state;
		Player_Extra_State extra_state;
	};

	constexpr int32			c_prediction_buffer_capacity	= 512;
	constexpr int32			c_prediction_buffer_mask		= c_prediction_buffer_capacity - 1;
	Predicted_Move*			predicted_move					= (Predicted_Move*)linear_allocator_alloc(&allocator, sizeof(Predicted_Move) * c_prediction_buffer_capacity);
	Predicted_Move_Result*	predicted_move_result			= (Predicted_Move_Result*)linear_allocator_alloc(&allocator, sizeof(Predicted_Move_Result) * c_prediction_buffer_capacity);

	constexpr float32	c_fov_y			= 60.0f * c_deg_to_rad;
	constexpr float32	c_aspect_ratio	= c_window_width / (float32)c_window_height;
	constexpr float32	c_near_plane	= 1.0f;
	constexpr float32	c_far_plane		= 100.0f;
	Matrix_4x4			projection_matrix;
	matrix_4x4_projection(&projection_matrix, c_fov_y, c_aspect_ratio, c_near_plane, c_far_plane);

	constexpr int32 c_tick_rate = 60; // todo(jbr) adaptive stable tick rate
	constexpr float32 c_seconds_per_tick = 1.0f / c_tick_rate;

	uint32 local_player_slot = (uint32)-1;
	uint32 prediction_id = 0; // todo(jbr) rolling sequence number, could maybe get away with 8 bits, certainly 9 or 10

	Timer tick_timer = timer();
	
	// Main loop
	int exit_code = 0;
	while (true) {

		// Windows messages
		bool32 got_quit_message = 0;
		MSG message;
		HWND hwnd = 0; // WM_QUIT is not associated with a window, so this must be 0
		UINT filter_min = 0;
		UINT filter_max = 0;
		UINT remove_message = PM_REMOVE;
		while (PeekMessage(&message, hwnd, filter_min, filter_max, remove_message)) {
			if (message.message == WM_QUIT) {
				exit_code = (int)message.wParam;
				got_quit_message = 1;
				break;
			}
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		if (got_quit_message) {
			break;
		}

		// Consume mouse deltas from input so it resets every tick
		int32 mouse_delta_x = client_globals->input.mouse_delta_x;
		int32 mouse_delta_y = client_globals->input.mouse_delta_y;
		client_globals->input.mouse_delta_x = 0; 
		client_globals->input.mouse_delta_y = 0;

		// Process Packets
		uint32 bytes_received;
		Net::IP_Endpoint from;
		while (Net::socket_receive(&sock, socket_buffer, c_socket_buffer_size, &bytes_received, &from)) {
			
			switch ((Net::Server_Message)socket_buffer[0]){
			
				case Net::Server_Message::Join_Result: {
					bool32 success;
					Net::server_msg_join_result_read(socket_buffer, &success, &local_player_slot);
					if (!success) {
						log("[client] Connection to server denied\n");
					}
				}
				break;

				case Net::Server_Message::State: {
					uint32 received_prediction_id;
					Player_Extra_State received_local_player_extra_state;
					Net::server_msg_state_read(
						socket_buffer, 
						&received_prediction_id, 
						&received_local_player_extra_state, 
						player_snapshot_states, 
						players_present,
						c_max_clients);

					int32 ticks_ahead = prediction_id - received_prediction_id;
					assert(ticks_ahead > -1);
					assert(ticks_ahead <= c_prediction_buffer_capacity);
					
					Player_Snapshot_State* received_local_player_snapshot_state = &player_snapshot_states[local_player_slot];

					uint32 index = received_prediction_id & c_prediction_buffer_mask;
					Vec_3f delta_pos = vec_3f_sub(received_local_player_snapshot_state->position, predicted_move_result[index].snapshot_state.position);
					constexpr float32 c_max_error = 0.001f; // 0.1cm
					constexpr float32 c_max_error_sq = c_max_error * c_max_error;
					if (vec_3f_length_sq(delta_pos) > c_max_error_sq) {
						log("[client]error of (%f, %f, %f) detected at prediction id %d, rewinding and replaying\n", delta_pos.x, delta_pos.y, delta_pos.z, received_prediction_id);
						
						*local_player_snapshot_state = *received_local_player_snapshot_state;
						*local_player_extra_state = received_local_player_extra_state;

						for (uint32 replaying_prediction_id = received_prediction_id + 1; 
							replaying_prediction_id < prediction_id; 
							++replaying_prediction_id) {
							uint32					replaying_index			= replaying_prediction_id & c_prediction_buffer_mask;

							Predicted_Move*			replaying_move			= &predicted_move[replaying_index];
							Predicted_Move_Result*	replaying_move_result	= &predicted_move_result[replaying_index];

							tick_player(local_player_snapshot_state, 
										local_player_extra_state, 
										replaying_move->dt, 
										&replaying_move->input);

							replaying_move_result->snapshot_state = *local_player_snapshot_state;
							replaying_move_result->extra_state = *local_player_extra_state;
						}
					}
				}
				break;
			}
		}

		// Tick player if we have one
		if (local_player_slot != (uint32)-1) {
			constexpr float32 c_mouse_sensitivity = 0.003f;

			Player_Input player_input = {};
			player_input.left = client_globals->input.keys['A'];
			player_input.right = client_globals->input.keys['D'];
			player_input.up = client_globals->input.keys['W'];
			player_input.down = client_globals->input.keys['S'];
			player_input.jump = client_globals->input.keys[VK_SPACE];
			player_input.pitch = f32_clamp(local_player_snapshot_state->pitch - (mouse_delta_y * c_mouse_sensitivity), -85.0f * c_deg_to_rad, 85.0f * c_deg_to_rad);
			player_input.yaw = local_player_snapshot_state->yaw + (mouse_delta_x * c_mouse_sensitivity);

			float32 dt = c_seconds_per_tick;
			
			uint32 input_msg_size = Net::client_msg_input_write(socket_buffer, local_player_slot, dt, &player_input, prediction_id);
			Net::socket_send(&sock, socket_buffer, input_msg_size, &server_endpoint);

			tick_player(local_player_snapshot_state, 
						local_player_extra_state, 
						dt, 
						&player_input);

			uint32					index	= prediction_id & c_prediction_buffer_mask;

			Predicted_Move*			move	= &predicted_move[index];
			Predicted_Move_Result*	result	= &predicted_move_result[index];

			move->dt						= dt;
			move->input						= player_input;
			result->snapshot_state			= *local_player_snapshot_state;
			result->extra_state				= *local_player_extra_state;
			
			player_snapshot_states[local_player_slot] = *local_player_snapshot_state;

			++prediction_id;
		}

		// Create view-projection matrix
		constexpr float32 c_camera_offset_distance = 3.0f;
		Vec_3f camera_pos = local_player_snapshot_state->position;
		camera_pos.z = (local_player_snapshot_state->ground + camera_z_offset); 
		

		Quat camera_rotation = quat_mul(quat_angle_axis(vec_3f(0.0f, 0.0f, 1.0f), local_player_snapshot_state->yaw),
										quat_angle_axis(vec_3f(1.0f, 0.0f, 0.0f), local_player_snapshot_state->pitch)); // pitch THEN yaw
		
		Matrix_4x4 view_matrix;
		matrix_4x4_camera(	&view_matrix,
							camera_pos,
							quat_right(camera_rotation),
							quat_forward(camera_rotation),
							quat_up(camera_rotation));
		
		Matrix_4x4 view_projection_matrix;
		matrix_4x4_mul(&view_projection_matrix, &projection_matrix, &view_matrix);

		// Create mvp matrix for scenery (just copy view-projection, scenery is not moved)
		mvp_matrices[0] = view_projection_matrix;

		// Create mvp matrix for each player
		Matrix_4x4 temp_translation_matrix;
		Matrix_4x4 temp_rotation_matrix;
		
		bool32* players_present_end = &players_present[c_max_clients];
		Player_Snapshot_State* player_snapshot_state = &player_snapshot_states[0];
		Matrix_4x4* player_mvp_matrix = &mvp_matrices[1];
		Matrix_4x4 temp_model_matrix;
		for (bool32* players_present_iter = &players_present[0];
			players_present_iter != players_present_end;
			++players_present_iter, ++player_snapshot_state) {
			
			if (*players_present_iter) {
				matrix_4x4_rotation_z(&temp_rotation_matrix, player_snapshot_state->yaw);
				matrix_4x4_translation(&temp_translation_matrix, player_snapshot_state->position);
				matrix_4x4_mul(&temp_model_matrix, &temp_translation_matrix, &temp_rotation_matrix);
				matrix_4x4_mul(player_mvp_matrix, &view_projection_matrix, &temp_model_matrix);
				
				++player_mvp_matrix;
			}
		}
		uint32 num_matrices = (uint32)(player_mvp_matrix - &mvp_matrices[1]);
		Graphics::update_and_draw(graphics_state, mvp_matrices, num_matrices);

		timer_wait_until(&tick_timer, c_seconds_per_tick, sleep_granularity_was_set);
		timer_shift_start(&tick_timer, c_seconds_per_tick);
	}

	uint32 leave_msg_size = Net::client_msg_leave_write(socket_buffer, local_player_slot);
	Net::socket_send(&sock, socket_buffer, leave_msg_size, &server_endpoint);
	Net::socket_close(&sock);

	return exit_code;
}