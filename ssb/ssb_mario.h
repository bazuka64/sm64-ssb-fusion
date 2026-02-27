typedef struct Controller {
    /*0x00*/ s16 rawStickX;               // Analog stick [-128, 128] positive is right. Used for menus.
    /*0x02*/ s16 rawStickY;               // Analog stick [-128, 128] positive is up. Used for menus.
    /*0x04*/ f32 stickX;                  // Analog stick [-64, 64] positive is right. Used for gameplay.
    /*0x08*/ f32 stickY;                  // Analog stick [-64, 64] positive is up. Used for gameplay.
    /*0x0C*/ f32 stickMag;                // Analog stick distance from center [0, 64]. Used for gameplay.
    /*0x10*/ u16 buttonDown;              // Buttons held down on the current frame.
    /*0x12*/ u16 buttonPressed;           // Buttons pressed on the current frame but not held on the previous frame.
    /*0x14*/ u16 buttonReleased;          // Burrons released on the current frame and held on the previous frame.
    /*0x18*/ void* statusData;    // Pointer to the controller status data in gControllerStatuses.
    /*0x1C*/ void* controllerData; // Pointer to the raw input data in gControllerPads.
    /*0x20*/ s32 port;                    // The port index this controller is plugged into [0, 3].
} Controller; /*0x24*/

#define CONT_A      0x8000
#define CONT_B      0x4000
#define CONT_G	    0x2000
#define CONT_START  0x1000
#define CONT_UP     0x0800
#define CONT_DOWN   0x0400
#define CONT_LEFT   0x0200
#define CONT_RIGHT  0x0100
#define CONT_L      0x0020
#define CONT_R      0x0010
#define CONT_E      0x0008
#define CONT_D      0x0004
#define CONT_C      0x0002
#define CONT_F      0x0001

/* Nintendo's official button names */

#define A_BUTTON	CONT_A
#define B_BUTTON	CONT_B
#define L_TRIG		CONT_L
#define R_TRIG		CONT_R
#define Z_TRIG		CONT_G
#define START_BUTTON	CONT_START
#define U_JPAD		CONT_UP
#define L_JPAD		CONT_LEFT
#define R_JPAD		CONT_RIGHT
#define D_JPAD		CONT_DOWN
#define U_CBUTTONS	CONT_E
#define L_CBUTTONS	CONT_C
#define R_CBUTTONS	CONT_F
#define D_CBUTTONS	CONT_D
#define GCN_X_BUTTON 0x0040
#define GCN_Y_BUTTON 0x0080