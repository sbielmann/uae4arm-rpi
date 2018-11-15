/*
 * [sbielmann] default virtual key definitions for ClockWorkPi Gameshell 
 */

/*
 * Virtual Key for (A) button
 * default: j (106)
 */
#define VK_A SDLK_j

/*
 * Virtual Key for (B) button
 * default: k (107)
 */
#define VK_B SDLK_k

/*
 * Virtual Key for (X) button
 * default: u (117)
 */
#define VK_X SDLK_u

/*
 * Virtual Key for (Y) button
 * default: i (105)
 */
#define VK_Y SDLK_i

/*
 * Virtual Key for (Select) button
 * default: space (32)
 */
#define VK_SELECT SDLK_SPACE

/*
 * Virtual Key for (Start) button
 * default: return (13)
 */
#define VK_START SDLK_RETURN

/*
 * Virtual Key for (up) button
 * default: UP (273)
 */
#define VK_UP SDLK_UP

/*
 * Virtual Key for (down) button
 * default: DOWN (274)
 */
#define VK_DOWN SDLK_DOWN

/*
 * Virtual Key for (right) button
 * default: RIGHT (275)
 */
#define VK_RIGHT SDLK_RIGHT

/*
 * Virtual Key for (left) button
 * default: LEFT (276)
 */
#define VK_LEFT SDLK_LEFT

/*
 * Virtual Key for (MENU) button
 * default: ESC (27)
 */
#define VK_MENU SDLK_ESCAPE



/*
 * Do not change the followling lines. On Gameshell
 * we map left shoulder button of Pandora as Select
 * button, right should button as Start and Escape
 * as Menu button.
 */
#define VK_L VK_SELECT
#define VK_R VK_START
#define VK_ESCAPE VK_MENU
