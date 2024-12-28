#ifndef H_DISPLAY_FUNCS
#define H_DISPLAY_FUNCS
#include <waveshareoled.h>

namespace OLED
{

#define SCREEN_WIDTH 64  // OLED display width, in pixels
#define SCREEN_HEIGHT 128 // OLED display height, in pixels
#define TOP_BAR_HEIGHT 16

#define WORD_SPACING 18
#define X_MARGIN 5
#define HEADING_LOCATION_Y TOP_BAR_HEIGHT + 5
#define INCLINATION_LOCATION_Y TOP_BAR_HEIGHT + WORD_SPACING + 5
#define DISTANCE_LOCATION_Y TOP_BAR_HEIGHT + WORD_SPACING*2 + 5


struct Point
{
  Point(uint16_t px, uint16_t py);
  uint16_t x, y;
};

Point rotatePoint(const Point p, const uint16_t cx, const uint16_t cy , const float rads);

enum CompassDirection
{
  NORTH,
  NORTH_EAST,
  EAST,
  SOUTH_EAST,
  SOUTH,
  SOUTH_WEST,
  WEST,
  NORTH_WEST,
  UP,
  DOWN
};

const char directionsArr[10][3] {
//   "   NORTH  ",
//   "NORTH EAST",
//   "   EAST   ",
//   "SOUTH EAST",
//   "   SOUTH  ",
//   "SOUTH WEST",
//   "   WEST   ",
//   "NORTH WEST",
//   "    UP    ",
//   "   DOWN   "
  // "   North  ",
  // "North East",
  // "   East   ",
  // "South East",
  // "   South  ",
  // "South West",
  // "   West   ",
  // "North West",
  // "    Up    ",
  // "   Down   "
  "N ",
  "NE",
  "E ",
  "SE",
  "S ",
  "SW",
  "W ",
  "NW",
  "U ",
  "D "
};

enum MenuEnum
{
    MENU_DUMP_DATA = 0,
    MENU_BLUETOOTH = 1,
    MENU_NEW_SURVEY = 2,
    MENU_CHANGE_SURVEY = 3
};

const char menu_arr [4][10] = {"Dump Cal", "Bluetooth", "New srvy", "Chg srvy"};


class DisplayHandler {
  public:

    int rand_no;
    DisplayHandler();
    void drawDistance(float distance);
    void drawHeading(float heading);
    void drawInclination(float inclination);
    void drawSensorCalStatus(int sensor_status);
    void drawBlutooth(bool ble_status);
    void drawBattery(int batt_percentage);
    void init();
    void clearDisplay();
    void clearHIData();
    void update();
    
    // void drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3,
    //                 uint16_t colour, DOT_PIXEL linewidth, LINE_STYLE linestyle);
    // void drawCircleHelper(uint16_t cx, uint16_t cy, uint16_t r, uint8_t quadrant, bool filled = false);

    void displayTopBar(bool bluetooth, int battery_level, int status);

    void drawCentered(String str, uint16_t cx, uint16_t cy, sFONT* font = &Font12, uint16_t fg = WHITE, uint16_t bg= WHITE);
    void drawLeft(String str, uint16_t x, uint16_t y, sFONT* font = &Font12, uint16_t fg= WHITE, uint16_t bg= WHITE);
    void drawCenteredBlack(String str, uint16_t cx, uint16_t cy, sFONT *font = &Font12);
    void drawLeftBlack(String str, uint16_t x, uint16_t y, sFONT *font = &Font12);

    void displayStaticCalib(CompassDirection pointing, CompassDirection facing, const char progress[5] = "");
    void displayLaserCalib(const float angle_deg, const char progress[5]);

    void drawCompass(uint16_t cx, uint16_t cy, uint16_t line_length, uint16_t arrow_length);
    void drawCompassDirection(uint16_t cx, uint16_t cy, uint16_t line_length, uint16_t arrow_length, CompassDirection direction);

    void displayOrientation();
    void displayShot();
    void displayYN(const char prompt[11], bool YN);
    void displayYN(const char prompt_top[11], const char prompt_btm[11], bool YN);
    void displayData();
    void displayLoading(const char prompt_top[11], const char prompt_btm[11], int count);
    void displayMenu(OLED::MenuEnum menu_item);

    float batt_voltage;
    int batt_level;
    int batt_percentage;
    float distance;
    float compass;
    float clino;
    int sensor_status;

  private:

    UBYTE *BlackImage;
    UWORD Imagesize = ((OLED_2IN42_WIDTH%8==0)? (OLED_2IN42_WIDTH/8): (OLED_2IN42_WIDTH/8+1)) * OLED_2IN42_HEIGHT;
    
    const int canvas_center_x = SCREEN_WIDTH/2;
    const int canvas_center_y = TOP_BAR_HEIGHT + (SCREEN_HEIGHT - TOP_BAR_HEIGHT)/2;


};


};
#endif