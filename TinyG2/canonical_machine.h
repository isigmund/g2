/*
 * canonical_machine.h - rs274/ngc canonical machining functions
 * This file is part of the TinyG2 project
 *
 * This code is a loose implementation of Kramer, Proctor and Messina's
 * canonical machining functions as described in the NIST RS274/NGC v3
 *
 * Copyright (c) 2010 - 2013 Alden S. Hart Jr.
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software library without
 * restriction. Specifically, if other files instantiate templates or use macros or
 * inline functions from this file, or you compile this file and link it with  other
 * files to produce an executable, this file does not by itself cause the resulting
 * executable to be covered by the GNU General Public License. This exception does not
 * however invalidate any other reasons why the executable file might be covered by the
 * GNU General Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _CANONICAL_MACHINE_H_
#define _CANONICAL_MACHINE_H_

#ifdef __cplusplus
extern "C"{
#endif

/*****************************************************************************
 * CANONICAL MACHINE STRUCTURES
 */
typedef struct cmSingleton {		// struct to manage cm globals and cycles
	magic_t magic_start;			// magic number to test memory integrity	
	uint8_t combined_state;			// combination of states for display purposes
	uint8_t machine_state;			// machine/cycle/motion is the actual machine state
	uint8_t cycle_state;
	uint8_t motion_state;
	uint8_t hold_state;				// feedhold sub-state machine
	uint8_t limit_flag;				// set true if limit switch was hit
	uint8_t request_feedhold;		// set true to initiate a feedhold
	uint8_t request_cycle_start;	// set true to end feedhold
	uint8_t cycle_start_flag;		// assert cycle start (follows the request)
	uint8_t homing_state;			// homing cycle sub-state machine
	uint8_t homed[AXES];			// individual axis homing flags
	uint8_t	g28_flag;				// true = complete a G28 move
	uint8_t	g30_flag;				// true = complete a G30 move
	uint8_t g10_persist_flag;		// G10 changed offsets - persist them
	magic_t magic_end;
} cmSingleton_t;
extern cmSingleton_t cm;

/* GCODE MODEL - The following GCodeModel/GCodeInput structs are used:
 *
 * - gm keeps the internal gcode state model in normalized, canonical form. 
 *	 All values are unit converted (to mm) and in the machine coordinate 
 *	 system (absolute coordinate system). Gm is owned by the canonical machine 
 *	 layer and should be accessed only through cm_ routines.
 *
 * - gn is used by the gcode interpreter and is re-initialized for each 
 *   gcode block.It accepts data in the new gcode block in the formats 
 *	 present in the block (pre-normalized forms). During initialization 
 *	 some state elements are necessarily restored from gm.
 *
 * - gf is used by the gcode parser interpreter to hold flags for any data 
 *	 that has changed in gn during the parse. gf.target[] values are also used 
 *	 by the canonical machine during set_target().
 *
 * - cfg (config struct in config.h) is also used heavily and contains some 
 *	 values that might be considered to be Gcode model values. The distinction 
 *	 is that all values in the config are persisted and restored, whereas the 
 *	 gm structs are transient. So cfg has the G54 - G59 offsets, but gm has the 
 *	 G92 offsets. cfg has the power-on / reset gcode default values, but gm has
 *	 the operating state for the values (which may have changed).
 */
typedef struct GCodeModel {				// Gcode dynamic model
	magic_t magic_start;				// magic number to test memory integity
	uint8_t next_action;				// handles G modal group 1 moves & non-modals
	uint8_t motion_mode;				// Group1: G0, G1, G2, G3, G38.2, G80, G81,
										// G82, G83 G84, G85, G86, G87, G88, G89 
	uint8_t program_flow;				// currently vestigal - captured, but not uses
	uint32_t linenum;					// N word

	float target[AXES]; 				// XYZABC where the move should go
	float position[AXES];				// XYZABC model position (Note: not used in gn or gf) 
	float origin_offset[AXES];			// XYZABC G92 offsets (Note: not used in gn or gf)
	float work_offset[AXES];			// XYZABC work offset to be forwarded to planner
	float work_scaling[AXES];			// XYZABC scale factor to get to work coordinates
	float g28_position[AXES];			// XYZABC stored machine position for G28
	float g30_position[AXES];			// XYZABC stored machine position for G30

	float min_time;						// minimum time possible for the move given axis constraints
	float feed_rate; 					// F - normalized to millimeters/minute
	float inverse_feed_rate; 			// ignored if inverse_feed_rate not active
	float feed_rate_override_factor;	// 1.0000 x F feed rate. Go up or down from there
	float traverse_override_factor;		// 1.0000 x traverse rate. Go down from there
	uint8_t inverse_feed_rate_mode;		// G93 TRUE = inverse, FALSE = normal (G94)
	uint8_t	feed_rate_override_enable;	// TRUE = overrides enabled (M48), F=(M49)
	uint8_t	traverse_override_enable;	// TRUE = traverse override enabled
	uint8_t l_word;						// L word - used by G10s

	uint8_t select_plane;				// G17,G18,G19 - values to set plane to
	uint8_t plane_axis_0;		 		// actual axes of the selected plane
	uint8_t plane_axis_1;		 		// ...(used in gm only)
	uint8_t plane_axis_2; 

	uint8_t units_mode;					// G20,G21 - 0=inches (G20), 1 = mm (G21)
	uint8_t coord_system;				// G54-G59 - select coordinate system 1-9
	uint8_t absolute_override;			// G53 TRUE = move using machine coordinates - this block only (G53)
	uint8_t origin_offset_enable;		// G92 offsets enabled/disabled.  0=disabled, 1=enabled

	uint8_t path_control;				// G61... EXACT_PATH, EXACT_STOP, CONTINUOUS
	uint8_t distance_mode;				// G91   0=use absolute coords(G90), 1=incremental movement

	uint8_t tool;						// T value
	uint8_t change_tool;				// M6
	uint8_t mist_coolant;				// TRUE = mist on (M7), FALSE = off (M9)
	uint8_t flood_coolant;				// TRUE = flood on (M8), FALSE = off (M9)

	uint8_t spindle_mode;				// 0=OFF (M5), 1=CW (M3), 2=CCW (M4)
	float  spindle_speed;				// in RPM
	float  spindle_override_factor;		// 1.0000 x S spindle speed. Go up or down from there
	uint8_t	spindle_override_enable;	// TRUE = override enabled

	uint8_t block_delete_switch;		// set true to enable block deletes (true is default)

// unimplemented gcode parameters
//	float cutter_radius;				// D - cutter radius compensation (0 is off)
//	float cutter_length;				// H - cutter length compensation (0 is off)

	float parameter;					// P - parameter used for dwell time in seconds, G10 coord select...
	float arc_radius;					// R - radius value in arc radius mode
	float arc_offset[3];  				// IJK - used by arc commands
	magic_t magic_end;
}  GCodeModel_t;

typedef struct GCodeInput {				// Gcode model inputs - meaning depends on context
	uint8_t next_action;				// handles G modal group 1 moves & non-modals
	uint8_t motion_mode;				// Group1: G0, G1, G2, G3, G38.2, G80, G81,
										// G82, G83 G84, G85, G86, G87, G88, G89 
	uint8_t program_flow;				// currently vestigal - captured, but not uses
	uint32_t linenum;					// N word or autoincrement in the model

	float target[AXES]; 				// XYZABC where the move should go

	float min_time;					// minimum time possible for the move given axis constraints
	float feed_rate; 					// F - normalized to millimeters/minute
	float inverse_feed_rate; 			// ignored if inverse_feed_rate not active
	float feed_rate_override_factor;	// 1.0000 x F feed rate. Go up or down from there
	float traverse_override_factor;	// 1.0000 x traverse rate. Go down from there
	uint8_t inverse_feed_rate_mode;		// G93 TRUE = inverse, FALSE = normal (G94)
	uint8_t	feed_rate_override_enable;	// TRUE = overrides enabled (M48), F=(M49)
	uint8_t	traverse_override_enable;	// TRUE = traverse override enabled
	uint8_t override_enables;			// enables for feed and spoindle (GN/GF only)
	uint8_t l_word;						// L word - used by G10s

	uint8_t select_plane;				// G17,G18,G19 - values to set plane to
	uint8_t units_mode;					// G20,G21 - 0=inches (G20), 1 = mm (G21)
	uint8_t coord_system;				// G54-G59 - select coordinate system 1-9
	uint8_t absolute_override;			// G53 TRUE = move using machine coordinates - this block only (G53)
	uint8_t origin_offset_mode;			// G92...TRUE=in origin offset mode
	uint8_t path_control;				// G61... EXACT_PATH, EXACT_STOP, CONTINUOUS
	uint8_t distance_mode;				// G91   0=use absolute coords(G90), 1=incremental movement

	uint8_t tool;						// T value
	uint8_t change_tool;				// M6
	uint8_t mist_coolant;				// TRUE = mist on (M7), FALSE = off (M9)
	uint8_t flood_coolant;				// TRUE = flood on (M8), FALSE = off (M9)

	uint8_t spindle_mode;				// 0=OFF (M5), 1=CW (M3), 2=CCW (M4)
	float  spindle_speed;				// in RPM
	float  spindle_override_factor;	// 1.0000 x S spindle speed. Go up or down from there
	uint8_t	spindle_override_enable;	// TRUE = override enabled

// unimplemented gcode parameters
//	float cutter_radius;				// D - cutter radius compensation (0 is off)
//	float cutter_length;				// H - cutter length compensation (0 is off)

	float parameter;					// P - parameter used for dwell time in seconds, G10 coord select...
	float arc_radius;					// R - radius value in arc radius mode
	float arc_offset[3];  				// IJK - used by arc commands
} GCodeInput_t;

// structure externs - declared in canonical_machine.cpp
extern GCodeModel_t gm;					// active gcode model
extern GCodeInput_t gn;					// gcode input values
extern GCodeInput_t gf;					// gcode input flags

/*****************************************************************************
 * 
 * MACHINE STATE MODEL
 * ref: http://www.synthetos.com/wiki/index.php?title=Projects:TinyG-State-Models
 *
 * The following variables track canonical machine state and state transitions.
 *
 *		- cm.machine_state		- overall state of machine and program execution
 *		- cm.cycle_state		- what cycle the machine is executing (or none)
 *		- cm.motion_state		- state of movement
 *
 * These additional sub-states are also tracked
 *
 *		- mr.hold_state
 *		- mr.feed_override_state
 *		- cm.homing_state
 */
/*	Allowed states and combined states
 *
 *	MACHINE STATE		CYCLE STATE		MOTION_STATE		COMBINED_STATE (FYI)
 *	-------------		------------	-------------		--------------------
 *	MACHINE_UNINIT		na				na					(U)
 *	MACHINE_READY		CYCLE_OFF		MOTION_STOP			(ROS) RESET-OFF-STOP
 *	MACHINE_PROG_STOP	CYCLE_OFF		MOTION_STOP			(SOS) STOP-OFF-STOP
 *	MACHINE_PROG_END	CYCLE_OFF		MOTION_STOP			(EOS) END-OFF-STOP
 *
 *	MACHINE_CYCLE		CYCLE_STARTED	MOTION_STOP			(CSS) CYCLE-START-STOP
 *	MACHINE_CYCLE		CYCLE_STARTED	MOTION_RUN			(CSR) CYCLE-START-RUN
 *	MACHINE_CYCLE		CYCLE_STARTED	MOTION_HOLD			(CSH) CYCLE-START-HOLD
 *	MACHINE_CYCLE		CYCLE_STARTED	MOTION_END_HOLD		(CSE) CYCLE-START-END_HOLD
 *
 *	MACHINE_CYCLE		CYCLE_HOMING	MOTION_STOP			(CHS) CYCLE-HOMING-STOP
 *	MACHINE_CYCLE		CYCLE_HOMING	MOTION_RUN			(CHR) CYCLE-HOMING-RUN
 *	MACHINE_CYCLE		CYCLE_HOMING	MOTION_HOLD			(CHH) CYCLE-HOMING-HOLD
 *	MACHINE_CYCLE		CYCLE_HOMING	MOTION_END_HOLD		(CHE) CYCLE-HOMING-END_HOLD
 */
// *** Note: check config printout strings align with all the state variables

// #### LAYER 8 CRITICAL REGION ###
// #### DO NOT CHANGE THESE ENUMERATIONS WITHOUT COMMUNITY INPUT #### 
enum cmCombinedState {				// check alignment with messages in config.c / msg_stat strings
	COMBINED_INITIALIZING = 0,		// machine is initializing
	COMBINED_READY,					// machine is ready for use
	COMBINED_SHUTDOWN,				// machine is shut down
	COMBINED_PROGRAM_STOP,			// program stop or no more blocks
	COMBINED_PROGRAM_END,			// program end
	COMBINED_RUN,					// motion is running
	COMBINED_HOLD,					// motion is holding
	COMBINED_PROBE,					// probe cycle active
	COMBINED_CYCLE,					// machine is running (cycling)
	COMBINED_HOMING,				// homing is treated as a cycle
	COMBINED_JOG					// jogging is treated as a cycle
};
//#### END CRITICAL REGION ####

enum cmMachineState {
	MACHINE_INITIALIZING = 0,		// machine is initializing
	MACHINE_READY,					// machine is ready for use
	MACHINE_SHUTDOWN,				// machine is in shutdown state
	MACHINE_PROGRAM_STOP,			// program stop or no more blocks
	MACHINE_PROGRAM_END,			// program end
	MACHINE_CYCLE,					// machine is running (cycling)
};

enum cmCycleState {
	CYCLE_OFF = 0,					// machine is idle
	CYCLE_STARTED,					// machine in normal cycle
	CYCLE_PROBE,					// machine in probe cycle
	CYCLE_HOMING,					// homing is treated as a specialized cycle
	CYCLE_JOG						// jogging is treated as a specialized cycle
};

enum cmMotionState {
	MOTION_STOP = 0,
	MOTION_RUN,						// machine is running
	MOTION_HOLD						// feedhold in progress
};

enum cmFeedholdState {				// applies to cm.feedhold_state
	FEEDHOLD_OFF = 0,				// no feedhold in effect
	FEEDHOLD_SYNC, 					// sync to latest aline segment
	FEEDHOLD_PLAN, 					// replan blocks for feedhold
	FEEDHOLD_DECEL,					// decelerate to hold point
	FEEDHOLD_HOLD,					// holding
	FEEDHOLD_END_HOLD				// end hold (transient state)
};

enum cmHomingState {				// applies to cm.homing_state
	HOMING_NOT_HOMED = 0,			// machine is not homed (0=false)
	HOMING_HOMED = 1				// machine is homed (1=true)
};

/* The difference between NextAction and MotionMode is that NextAction is 
 * used by the current block, and may carry non-modal commands, whereas 
 * MotionMode persists across blocks (as G modal group 1)
 */

enum cmNextAction {						// these are in order to optimized CASE statement
	NEXT_ACTION_DEFAULT = 0,			// Must be zero (invokes motion modes)
	NEXT_ACTION_SEARCH_HOME,			// G28.2 homing cycle
	NEXT_ACTION_SET_ABSOLUTE_ORIGIN,	// G28.3 origin set
	NEXT_ACTION_SET_G28_POSITION,		// G28.1 set position in abs coordingates 
	NEXT_ACTION_GOTO_G28_POSITION,		// G28 go to machine position
	NEXT_ACTION_SET_G30_POSITION,		// G30.1
	NEXT_ACTION_GOTO_G30_POSITION,		// G30
	NEXT_ACTION_SET_COORD_DATA,			// G10
	NEXT_ACTION_SET_ORIGIN_OFFSETS,		// G92
	NEXT_ACTION_RESET_ORIGIN_OFFSETS,	// G92.1
	NEXT_ACTION_SUSPEND_ORIGIN_OFFSETS,	// G92.2
	NEXT_ACTION_RESUME_ORIGIN_OFFSETS,	// G92.3
	NEXT_ACTION_DWELL,					// G4
	NEXT_ACTION_STRAIGHT_PROBE			// G38.2
};

enum cmMotionMode {						// G Modal Group 1
	MOTION_MODE_STRAIGHT_TRAVERSE=0,	// G0 - traverse
	MOTION_MODE_STRAIGHT_FEED,			// G1 - feed
	MOTION_MODE_CW_ARC,					// G2 - arc feed
	MOTION_MODE_CCW_ARC,				// G3 - arc feed
	MOTION_MODE_CANCEL_MOTION_MODE,		// G80
	MOTION_MODE_STRAIGHT_PROBE,			// G38.2
	MOTION_MODE_CANNED_CYCLE_81,		// G81 - drilling
	MOTION_MODE_CANNED_CYCLE_82,		// G82 - drilling with dwell
	MOTION_MODE_CANNED_CYCLE_83,		// G83 - peck drilling
	MOTION_MODE_CANNED_CYCLE_84,		// G84 - right hand tapping
	MOTION_MODE_CANNED_CYCLE_85,		// G85 - boring, no dwell, feed out
	MOTION_MODE_CANNED_CYCLE_86,		// G86 - boring, spindle stop, rapid out
	MOTION_MODE_CANNED_CYCLE_87,		// G87 - back boring
	MOTION_MODE_CANNED_CYCLE_88,		// G88 - boring, spindle stop, manual out
	MOTION_MODE_CANNED_CYCLE_89			// G89 - boring, dwell, feed out
};

enum cmModalGroup {						// Used for detecting gcode errors. See NIST section 3.4
	MODAL_GROUP_G0 = 0, 				// {G10,G28,G28.1,G92} 	non-modal axis commands (note 1)
	MODAL_GROUP_G1,						// {G0,G1,G2,G3,G80}	motion
	MODAL_GROUP_G2,						// {G17,G18,G19}		plane selection
	MODAL_GROUP_G3,						// {G90,G91}			distance mode
	MODAL_GROUP_G5,						// {G93,G94}			feed rate mode
	MODAL_GROUP_G6,						// {G20,G21}			units
	MODAL_GROUP_G7,						// {G40,G41,G42}		cutter radius compensation
	MODAL_GROUP_G8,						// {G43,G49}			tool length offset
	MODAL_GROUP_G9,						// {G98,G99}			return mode in canned cycles
	MODAL_GROUP_G12,					// {G54,G55,G56,G57,G58,G59} coordinate system selection
	MODAL_GROUP_G13,					// {G61,G61.1,G64}		path control mode
	MODAL_GROUP_M4,						// {M0,M1,M2,M30,M60}	stopping
	MODAL_GROUP_M6,						// {M6}					tool change
	MODAL_GROUP_M7,						// {M3,M4,M5}			spindle turning
	MODAL_GROUP_M8,						// {M7,M8,M9}			coolant (M7 & M8 may be active together)
	MODAL_GROUP_M9						// {M48,M49}			speed/feed override switches
};
#define MODAL_GROUP_COUNT (MODAL_GROUP_M9+1)
// Note 1: Our G0 omits G4,G30,G53,G92.1,G92.2,G92.3 as these have no axis components to error check

enum cmCanonicalPlane {				// canonical plane - translates to:
									// 		axis_0	axis_1	axis_2
	CANON_PLANE_XY = 0,				// G17    X		  Y		  Z
	CANON_PLANE_XZ,					// G18    X		  Z		  Y
	CANON_PLANE_YZ					// G19	  Y		  Z		  X							
};

enum cmUnitsMode {
	INCHES = 0,						// G20
	MILLIMETERS,					// G21
	DEGREES							// ABC axes (this value used for displays only)
};

enum cmCoordSystem {
	ABSOLUTE_COORDS = 0,			// machine coordinate system
	G54,							// G54 coordinate system
	G55,							// G55 coordinate system
	G56,							// G56 coordinate system
	G57,							// G57 coordinate system
	G58,							// G58 coordinate system
	G59								// G59 coordinate system
};
#define COORD_SYSTEM_MAX G59		// set this manually to the last one

enum cmPathControlMode {			// G Modal Group 13
	PATH_EXACT_PATH = 0,			// G61
	PATH_EXACT_STOP,				// G61.1
	PATH_CONTINUOUS					// G64 and typically the default mode
};

enum cmDistanceMode {
	ABSOLUTE_MODE = 0,				// G90
	INCREMENTAL_MODE				// G91
};

enum cmOriginOffset {
	ORIGIN_OFFSET_SET=0,			// G92 - set origin offsets
	ORIGIN_OFFSET_CANCEL,			// G92.1 - zero out origin offsets
	ORIGIN_OFFSET_SUSPEND,			// G92.2 - do not apply offsets, but preserve the values
	ORIGIN_OFFSET_RESUME			// G92.3 - resume application of the suspended offsets
};

enum cmProgramFlow {
	PROGRAM_STOP = 0,
	PROGRAM_END
};

enum cmSpindleState {				// spindle state settings (See hardware.h for bit settings)
	SPINDLE_OFF = 0,
	SPINDLE_CW,
	SPINDLE_CCW
};

enum cmCoolantState {				// mist and flood coolant states
	COOLANT_OFF = 0,				// all coolant off
	COOLANT_ON,						// request coolant on or indicates both coolants are on
	COOLANT_MIST,					// indicates mist coolant on
	COOLANT_FLOOD					// indicates flood coolant on
};

enum cmDirection {					// used for spindle and arc dir
	DIRECTION_CW = 0,
	DIRECTION_CCW
};

enum cmAxisMode {					// axis modes (ordered: see _cm_get_feed_time())
	AXIS_DISABLED = 0,				// kill axis
	AXIS_STANDARD,					// axis in coordinated motion w/standard behaviors
	AXIS_INHIBITED,					// axis is computed but not activated
	AXIS_RADIUS						// rotary axis calibrated to circumference
};	// ordering must be preserved. See _cm_get_feed_time() and seek time()
#define AXIS_MAX_LINEAR AXIS_INHIBITED
#define AXIS_MAX_ROTARY AXIS_RADIUS

/*****************************************************************************
 * FUNCTION PROTOTYPES
 */
/*--- helper functions for canonical machining functions ---*/
uint8_t cm_get_combined_state(void); 
uint8_t cm_get_machine_state(void);
uint8_t cm_get_cycle_state(void);
uint8_t cm_get_motion_state(void);
uint8_t cm_get_hold_state(void);
uint8_t cm_get_homing_state(void);

uint8_t cm_get_motion_mode(void);
uint8_t cm_get_coord_system(void);
uint8_t cm_get_units_mode(void);
uint8_t cm_get_select_plane(void);
uint8_t cm_get_path_control(void);
uint8_t cm_get_distance_mode(void);
uint8_t cm_get_inverse_feed_rate_mode(void);
uint8_t cm_get_spindle_mode(void);
uint32_t cm_get_model_linenum(void);
uint8_t	cm_get_block_delete_switch(void);

uint8_t cm_isbusy(void);

void cm_set_motion_mode(uint8_t motion_mode);
void cm_set_absolute_override(uint8_t absolute_override);
void cm_set_spindle_mode(uint8_t spindle_mode);
void cm_set_spindle_speed_parameter(float speed);
void cm_set_tool_number(uint8_t tool);

float cm_get_coord_offset(uint8_t axis);
float *cm_get_coord_offset_vector(float vector[]);
float cm_get_model_work_position(uint8_t axis);
//float *cm_get_model_work_position_vector(float position[]);
float cm_get_model_canonical_target(uint8_t axis);
float *cm_get_model_canonical_position_vector(float vector[]);
float cm_get_runtime_machine_position(uint8_t axis);
float cm_get_runtime_work_position(uint8_t axis);
float cm_get_runtime_work_offset(uint8_t axis);

void cm_set_arc_offset(float i, float j, float k);
void cm_set_arc_radius(float r);
void cm_set_target(float target[], float flag[]);
void cm_set_gcode_model_endpoint_position(uint8_t status);
void cm_set_model_linenum(uint32_t linenum);

/*--- canonical machining functions ---*/
void canonical_machine_init(void);
void canonical_machine_shutdown(uint8_t value);					// emergency shutdown

stat_t cm_set_machine_axis_position(uint8_t axis, const float position);	// set absolute position
stat_t cm_flush_planner(void);									// flush planner queue with coordinate resets

stat_t cm_select_plane(uint8_t plane);							// G17, G18, G19
stat_t cm_set_units_mode(uint8_t mode);							// G20, G21

stat_t cm_homing_cycle_start(void);								// G28.2
stat_t cm_homing_callback(void);								// G28.2 main loop callback
stat_t cm_set_absolute_origin(float origin[], float flags[]);	// G28.3  (special function)

stat_t cm_set_g28_position(void);								// G28.1
stat_t cm_goto_g28_position(float target[], float flags[]); 	// G28
stat_t cm_set_g30_position(void);								// G30.1
stat_t cm_goto_g30_position(float target[], float flags[]);		// G30

stat_t cm_set_coord_system(uint8_t coord_system);				// G54 - G59
stat_t cm_set_coord_offsets(uint8_t coord_system, float offset[], float flag[]); // G10 L2
stat_t cm_set_distance_mode(uint8_t mode);						// G90, G91
stat_t cm_set_origin_offsets(float offset[], float flag[]);		// G92
stat_t cm_reset_origin_offsets(void); 							// G92.1
stat_t cm_suspend_origin_offsets(void); 						// G92.2
stat_t cm_resume_origin_offsets(void);				 			// G92.3

stat_t cm_straight_traverse(float target[], float flags[]);
stat_t cm_set_feed_rate(float feed_rate);						// F parameter
stat_t cm_set_inverse_feed_rate_mode(uint8_t mode);				// True= inv mode
stat_t cm_set_path_control(uint8_t mode);						// G61, G61.1, G64
stat_t cm_straight_feed(float target[], float flags[]);			// G1
stat_t cm_arc_feed(float target[], float flags[], 				// G2, G3
					float i, float j, float k, 
					float radius, uint8_t motion_mode);
stat_t cm_dwell(float seconds);									// G4, P parameter

stat_t cm_set_spindle_speed(float speed);						// S parameter
stat_t cm_start_spindle_clockwise(void);						// M3
stat_t cm_start_spindle_counterclockwise(void);					// M4
stat_t cm_stop_spindle_turning(void);							// M5
stat_t cm_spindle_control(uint8_t spindle_mode);				// integrated spindle control

stat_t cm_mist_coolant_control(uint8_t mist_coolant); 			// M7
stat_t cm_flood_coolant_control(uint8_t flood_coolant);			// M8, M9

stat_t cm_override_enables(uint8_t flag); 						// M48, M49
stat_t cm_feed_rate_override_enable(uint8_t flag); 				// M50
stat_t cm_feed_rate_override_factor(uint8_t flag);				// M50.1
stat_t cm_traverse_override_enable(uint8_t flag); 				// M50.2
stat_t cm_traverse_override_factor(uint8_t flag);				// M50.3
stat_t cm_spindle_override_enable(uint8_t flag); 				// M51
stat_t cm_spindle_override_factor(uint8_t flag);				// M51.1

stat_t cm_change_tool(uint8_t tool);							// M6, T
uint8_t cm_select_tool(uint8_t tool);							// T parameter

// canonical machine commands not called from gcode dispatcher
void cm_message(char_t *message);								// msg to console (e.g. Gcode comments)
void cm_cycle_start(void);										// (no Gcode)
void cm_cycle_end(void); 										// (no Gcode)
void cm_feedhold(void);											// (no Gcode)
void cm_program_stop(void);										// M0
void cm_optional_program_stop(void);							// M1
void cm_program_end(void);										// M2
void cm_exec_program_stop(void);
void cm_exec_program_end(void);

#ifdef __cplusplus
}
#endif

#endif // _CANONICAL_MACHINE_H_