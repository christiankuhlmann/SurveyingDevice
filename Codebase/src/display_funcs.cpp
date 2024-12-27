#include "display_funcs.h"
#include "inttypes.h"

using namespace OLED;

DisplayHandler::DisplayHandler() {
}

Point::Point(uint16_t px, uint16_t py)
{
	x = px;
	y = py;
}

Point OLED::rotatePoint(const Point p, const uint16_t cx, const uint16_t cy , const float rads)
{
    float x, y;
    x = std::round(cos(rads) * ((float)p.x - (float)cx) - sin(rads) * ((float)p.y - (float)cy) + (float)cx);
    y = std::round(sin(rads) * ((float)p.x - (float)cx) + cos(rads) * ((float)p.y - (float)cy) + (float)cy);
	return Point((uint16_t) x, (uint16_t) y);
}


void DisplayHandler::init()
{
    System_Init();                                                                                                                                                                                                                                                                                   
    Serial.print(F("OLED_Init()...\r\n"));
    OLED_2IN42_Init();
    Driver_Delay_ms(500); 
    OLED_2IN42_Clear(); 

    //0.Create a new image cache
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) { 
        Serial.print("Failed to apply for black memory...\r\n");
        //return -1;
    }

    Paint_NewImage(BlackImage, OLED_2IN42_WIDTH, OLED_2IN42_HEIGHT, 0, BLACK);  

    //1.Select Image
    Paint_SelectImage(BlackImage);
    Paint_SetRotate(0);
    Paint_Clear(BLACK);

    // Paint Image: "Hello world"
    Paint_DrawString_EN(5,5,"Bean Boi", &Font12, WHITE, WHITE);
    OLED_2IN42_Display(BlackImage);

    // Draw Image
    OLED_2IN42_Display(BlackImage);
    Driver_Delay_ms(2000);  
    Paint_Clear(BLACK);  

    // Clear Display
    OLED_2IN42_Clear();

}

void DisplayHandler::drawHeading(float heading)
{
    // Paint Image: "Hello world"
    char disp_str[10];
    sprintf(disp_str,"H:%5.1f", heading);
    Paint_DrawString_EN(X_MARGIN, HEADING_LOCATION_Y, disp_str, &Font12, WHITE, WHITE);
}

void DisplayHandler::drawInclination(float inclination)
{
    char disp_str[10];
    sprintf(disp_str,"I:%5.1f", inclination);
    Paint_DrawString_EN(X_MARGIN, INCLINATION_LOCATION_Y, disp_str, &Font12, WHITE, WHITE);
}

void DisplayHandler::drawDistance(float distance) {
    char disp_str[10];
    sprintf(disp_str,"D:%5.1f", distance);
    Paint_DrawString_EN(X_MARGIN, DISTANCE_LOCATION_Y, disp_str, &Font12, WHITE, WHITE);
}

void DisplayHandler::drawCompass(uint16_t cx, uint16_t cy, uint16_t line_length, uint16_t arrow_length) {
	// display.fillRect(0, 0, SCREEN_WIDTH, 28, SH110X_WHITE);
	Paint_DrawLine(cx-line_length,cy, cx+line_length, cy, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
	Paint_DrawLine(cx,cy-line_length, cx, cy+line_length, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

	// North Triangle
	fillTriangle( 	cx,cy-line_length,
                    cx-arrow_length,cy-line_length+arrow_length,
                    cx+arrow_length,cy-line_length+arrow_length,
                    WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
                    
	// North Triangle
	fillTriangle( 	cx,cy+line_length,
                    cx-arrow_length,cy+line_length-arrow_length,
                    cx+arrow_length,cy+line_length-arrow_length,
                    WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

	// East Triangle
	fillTriangle( 	cx+line_length,cy,
                    cx+line_length-arrow_length,cy-arrow_length,
                    cx+line_length-arrow_length,cy+arrow_length,
                    WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  // West Triangle
	fillTriangle( 	cx-line_length,cy,
                    cx-line_length+arrow_length,cy-arrow_length,
                    cx-line_length+arrow_length,cy+arrow_length,
                    WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
}

void DisplayHandler::drawCentered(String str, uint16_t cx, uint16_t cy, sFONT *font)
{
    uint16_t xsize = font->Width;
    uint16_t ysize = font->Width;
    // Serial.printf("X: %i, Y: %i\n", (int)(cx - (uint16_t)((str.length()/2.0)*xsize)), (int)(cy - (uint16_t)(ysize/2)), str.c_str());
    Paint_DrawString_EN(cx - ((str.length()/2.0)*xsize), cy-(uint16_t)(ysize/2), str.c_str(), font, WHITE, WHITE);
}

void DisplayHandler::drawCenteredBlack(String str, uint16_t cx, uint16_t cy, sFONT *font)
{
    uint16_t xsize = font->Width;
    uint16_t ysize = font->Width;
    // Serial.printf("X: %i, Y: %i\n", (int)(cx - (uint16_t)((str.length()/2.0)*xsize)), (int)(cy - (uint16_t)(ysize/2)), str.c_str());
    Paint_DrawString_EN(cx - ((str.length()/2.0)*xsize), cy-(uint16_t)(ysize/2), str.c_str(), font, WHITE, BLACK);
}
   
void DisplayHandler::displayStaticCalib(CompassDirection pointing, CompassDirection facing, const char progress[5])
{

	// Paint_DrawRectangle(0, 0, SCREEN_WIDTH, TOP_BAR_HEIGHT, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    drawCentered(String("Calib"),SCREEN_WIDTH/2,7, &Font12);
	drawCentered(String(progress),SCREEN_WIDTH/2,20, &Font12);
    // drawCentered(String("CALIB"),SCREEN_WIDTH/2,TOP_BAR_HEIGHT + Font8.Height,&Font8);
	// drawCentered(String(progress),SCREEN_WIDTH/2,TOP_BAR_HEIGHT + Font8.Height*2+2,&Font8);

	// drawCentered("Pointing",                    SCREEN_WIDTH/2, 30,                  &Font8);
	// drawCentered(directionsArr[(int)pointing],  SCREEN_WIDTH/2, 30+Font8.Height+2,   &Font8);
    drawCentered("Pointing",                    SCREEN_WIDTH/2, 37,                  &Font12);
	drawCompassDirection(                       3*SCREEN_WIDTH/8, 62,                13,2,pointing);
    drawCentered(directionsArr[(int)pointing],  3*SCREEN_WIDTH/4, 62-3,              &Font12);

	// drawCentered("Facing",                      SCREEN_WIDTH/2, 85,                  &Font8);
	// drawCentered(directionsArr[(int)facing],    SCREEN_WIDTH/2, 85+Font8.Height+2,   &Font8);
    drawCentered("Facing",                      SCREEN_WIDTH/2, 85,                  &Font12);
	drawCompassDirection(                       3*SCREEN_WIDTH/8, 110,               13,2,facing);
    drawCentered(directionsArr[(int)facing],    3*SCREEN_WIDTH/4, 110-3,             &Font12);

}

void DisplayHandler::displayLaserCalib(const float angle_deg, const char progress[5])
{
	const uint16_t radius = 15;
	Point center(canvas_center_x,canvas_center_y); // Centre point of the canvas
	Point p_default(center.x,center.y-radius); // Top of circle
	Point p_start(center.x,center.y-(radius+10)); // Top of circle
	Point p_end(center.x,center.y-(radius+10)); // Top of circle

	// int quad = floor(fmod(angle,M_2_PI)*0.95/M_PI_2);
    int quad = floor(angle_deg*0.975/90);
	Serial.println(quad);

    if (angle_deg < 10) quad = 4;

    switch (quad){

        case 4:
            Paint_DrawCircle(center.x,center.y,radius,WHITE,DOT_PIXEL_1X1,DRAW_FILL_EMPTY);
        break;

        case 3:
            // Draw quadrant 4
            Paint_DrawPoint(center.x-radius,center.y,WHITE,DOT_PIXEL_1X1,DOT_STYLE_DFT);
            drawCircleHelper(center.x,center.y,radius,0x1,WHITE,DOT_PIXEL_1X1,DRAW_FILL_EMPTY);

        case 2:
            // Draw quadrant 3
            Paint_DrawPoint(center.x,center.y+radius,WHITE,DOT_PIXEL_1X1,DOT_STYLE_DFT);
            drawCircleHelper(center.x,center.y,radius,0x8,WHITE,DOT_PIXEL_1X1,DRAW_FILL_EMPTY);

        case 1:
            // Draw quadrant 2
            Paint_DrawPoint(center.x+radius,center.y,WHITE,DOT_PIXEL_1X1,DOT_STYLE_DFT);
            drawCircleHelper(center.x,center.y,radius,0x4,WHITE,DOT_PIXEL_1X1,DRAW_FILL_EMPTY);

        case 0:
            // Draw quadrant 1
            Paint_DrawPoint(center.x,center.y-radius,WHITE,DOT_PIXEL_1X1,DOT_STYLE_DFT);
            drawCircleHelper(center.x,center.y,radius,0x2,WHITE,DOT_PIXEL_1X1,DRAW_FILL_EMPTY);

            p_start = rotatePoint(p_start,center.x,center.y,DEG_TO_RAD*angle_deg);
            p_end = rotatePoint(p_end,center.x,center.y,(quad+1)*M_PI_2);
            fillTriangle(center.x,center.y, p_start.x,p_start.y, p_end.x, p_end.y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
            uint16_t arrow_length = 7;

            // Draw arrow at end of direction line
            p_start.x = p_default.x;
            p_start.y = p_default.y;
            p_end.x = p_start.x - (uint16_t)((float)arrow_length / M_SQRT2);
            p_end.y = p_start.y + (uint16_t)((float)arrow_length / M_SQRT2);
            p_end = rotatePoint(p_end,center.x,center.y,DEG_TO_RAD*angle_deg);
            p_start = rotatePoint(p_start,center.x,center.y,DEG_TO_RAD*angle_deg);
            Paint_DrawLine(p_start.x, p_start.y, p_end.x, p_end.y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

            p_start.x = p_default.x;
            p_start.y = p_default.y;
            p_end.x = p_start.x - (uint16_t)((float)arrow_length / M_SQRT2 * 1.4);
            p_end.y = p_start.y - (uint16_t)((float)arrow_length / M_SQRT2 * 0.6);
            p_end = rotatePoint(p_end,center.x,center.y,DEG_TO_RAD*angle_deg);
            p_start = rotatePoint(p_start,center.x,center.y,DEG_TO_RAD*angle_deg);
            Paint_DrawLine(p_start.x, p_start.y, p_end.x, p_end.y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        break;
    }

	drawCentered(String("CALIB"),SCREEN_WIDTH/2,TOP_BAR_HEIGHT,&Font12);
	drawCentered(String(progress),SCREEN_WIDTH/2,TOP_BAR_HEIGHT+Font12.Height+2,&Font12);
    char str_contents [4];
    sprintf(str_contents,"%03.0f", angle_deg);
	drawCentered(String(str_contents),center.x,center.y-3,&Font12);
}

void DisplayHandler::drawCompassDirection(uint16_t cx, uint16_t cy, uint16_t line_length, uint16_t arrow_length, CompassDirection direction)
{
	float angle;
	float N_arrow = 1;
	switch(direction)
	{
		case UP:
		angle = 0;
		N_arrow = 1;
		break;

		case DOWN:
		angle = 180;
		N_arrow = 1;
		break;

		case NORTH:
		angle = 0;
		N_arrow = 3;
		drawCompass(cx,cy,line_length,arrow_length);
		break;

		case NORTH_EAST:
		angle = M_PI_4;
		N_arrow = 3;
		drawCompass(cx,cy,line_length,arrow_length);
		break;

		case EAST:
		angle = 2*M_PI_4;
		N_arrow = 3;
		drawCompass(cx,cy,line_length,arrow_length);
		break;

		case SOUTH_EAST:
		angle = 3*M_PI_4;
		N_arrow = 3;
		drawCompass(cx,cy,line_length,arrow_length);
		break;

		case SOUTH:
		angle = 4*M_PI_4;
		N_arrow = 3;
		drawCompass(cx,cy,line_length,arrow_length);
		break;

		case SOUTH_WEST:
		angle = 5*M_PI_4;
		N_arrow = 3;
		drawCompass(cx,cy,line_length,arrow_length);
		break;

		case WEST:
		angle = 6*M_PI_4;
		N_arrow = 3;
		drawCompass(cx,cy,line_length,arrow_length);
		break;

		case NORTH_WEST:
		angle = 7*M_PI_4;
		N_arrow = 3;
		drawCompass(cx,cy,line_length,arrow_length);
		break;
		
	}


	Point p_default(cx,cy-line_length);
	Point p_start(cx,cy);
	Point p_end(cx,cy-line_length);
	p_end = rotatePoint(p_end,cx,cy,angle);
	Paint_DrawLine(p_start.x, p_start.y, p_end.x, p_end.y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);


	for (int i=0; i<N_arrow; i++)
	{
		p_start.x = p_default.x;
		p_start.y = p_default.y + i*arrow_length;
		p_end.x = p_start.x - arrow_length;
		p_end.y = p_start.y + arrow_length;
		p_end = rotatePoint(p_end,cx,cy,angle);
		p_start = rotatePoint(p_start,cx,cy,angle);
		Paint_DrawLine(p_start.x, p_start.y, p_end.x, p_end.y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

		p_start.x = p_default.x;
		p_start.y = p_default.y + i*arrow_length;
		p_end.x = p_start.x + arrow_length;
		p_end.y = p_start.y + arrow_length;
		p_end = rotatePoint(p_end,cx,cy,angle);
		p_start = rotatePoint(p_start,cx,cy,angle);
		Paint_DrawLine(p_start.x, p_start.y, p_end.x, p_end.y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
	}
}

void DisplayHandler::displayYN(const char prompt_top[11], const char prompt_btm[11], bool YN)
{

	const int prompt_height = canvas_center_y - 40;
	const int selector_height = canvas_center_y;

	// String str(prompt);
	drawCentered(prompt_top,canvas_center_x,prompt_height);
	drawCentered(prompt_btm,canvas_center_x+2,prompt_height+15);

	if (YN)
	{
		Paint_DrawRectangle(canvas_center_x - 30, selector_height - 5,
							canvas_center_x - 5, selector_height + 10,
							WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

		Paint_DrawRectangle(canvas_center_x + 5, selector_height - 5,
							canvas_center_x + 30, selector_height + 10,
							WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

		drawCenteredBlack("YES",canvas_center_x - 17, selector_height);
		drawCentered("NO",canvas_center_x + 17, selector_height);

	} else {
		Paint_DrawRectangle(canvas_center_x - 30, selector_height - 5,
							canvas_center_x - 5, selector_height + 10,
							WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

		Paint_DrawRectangle(canvas_center_x + 5, selector_height - 5,
							canvas_center_x + 30, selector_height + 10,
							WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

		drawCentered("YES",canvas_center_x - 17, selector_height);
		drawCenteredBlack("NO",canvas_center_x + 17, selector_height);
	}
}

void DisplayHandler::clearDisplay() {
//    OLED_2IN42_Clear(); 
   Paint_Clear(BLACK);  
}

void DisplayHandler::clearHIData()
{
    Paint_Clear(BLACK);  
    // Paint_DrawRectangle(X_MARGIN,HEADING_LOCATION_Y,64,DISTANCE_LOCATION_Y,BLACK,DOT_PIXEL_1X1,DRAW_FILL_FULL);
    // OLED_2IN42_Display(BlackImage);
}

void DisplayHandler::update() {
   OLED_2IN42_Display(BlackImage);
}

void DisplayHandler::displayLoading(const char prompt_top[11], const char prompt_btm[11], int count)
{
	const int n_points = 10;
	Point initial_point(canvas_center_x, canvas_center_y-10);
	Point loading_dot(0,0);

	const int prompt_height = TOP_BAR_HEIGHT + 10;

	drawCentered(prompt_top,canvas_center_x,prompt_height);
	drawCentered(prompt_btm,canvas_center_x,prompt_height+12);

	for (int i = 0; i<n_points; i++)
	{
		loading_dot = rotatePoint(initial_point, canvas_center_x, canvas_center_y+10, i%10 * M_PI/5);
		if (i == count % n_points)
		{
			Paint_DrawCircle(loading_dot.x, loading_dot.y, 2, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		} else {
			Paint_DrawCircle(loading_dot.x, loading_dot.y, 2, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
		}
	}
}

// void DisplayHandler::Sensor_cal_status(int sensor_status) {
//   display.setTextSize(2);
//   display.setCursor(23, 4);
//   display.print(sensor_status);
//   // display.display();
// }

void DisplayHandler::drawBattery(int batt_percentage) {
	// Draw horizontal rectangle at top righ
	static int battery_width = 18; // 5px per 5%
	static int battery_height = TOP_BAR_HEIGHT - 4;
	static int n_bars = 0;

	// Body of battery
	Paint_DrawRectangle(64 - battery_width - 6, 	4,
						64 - 6,					 	TOP_BAR_HEIGHT - 4,
						WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);

	// Tip of battery (8x4)
	Paint_DrawRectangle(64 - 6, TOP_BAR_HEIGHT/2 - 2,
						64 - 4, TOP_BAR_HEIGHT/2 + 2,
						WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);


	// Capacity of battery
	if (batt_percentage > 10)
	{
		n_bars = batt_percentage / 25; // Rounded down bat/20;

		for (int i=0; i<n_bars+1; i++)
		{
			Paint_DrawRectangle(64 - battery_width - 6 + 1 + i*4+1, 	6,
								64 - battery_width - 6 + 1 + i*4 + 3, 	TOP_BAR_HEIGHT - 5,
								WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		}
	}
}


// void DisplayHandler::drawBlutooth(bool ble_status)
// {
//   // insert switch case to update
//   const unsigned char PROGMEM Bluetooth_icon[] = {
//       0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x01, 0xfe, 0x00, 0x07, 0xff, 0x00, 0x07, 0xcf, 0x80, 0x0f,
//       0xc7, 0xc0, 0x0f, 0xc3, 0xc0, 0x0e, 0xd9, 0xc0, 0x0f, 0x03, 0xc0, 0x0f, 0x87, 0xc0, 0x0f, 0x87,
//       0xc0, 0x0f, 0xcf, 0xc0, 0x0f, 0x87, 0xc0, 0x0f, 0x03, 0xc0, 0x0e, 0x59, 0xc0, 0x0f, 0xc3, 0xc0,
//       0x0f, 0xc7, 0xc0, 0x07, 0xcf, 0x80, 0x07, 0xff, 0x80, 0x03, 0xff, 0x00, 0x00, 0xfe, 0x00, 0x00,
//       0x00, 0x00};

//   if (ble_status == false)
//   {
//     display.fillRect(0, 0, 22, 22, SH110X_BLACK); // clears the blutooth symbol
//     display.display();
//   }
//   else
//   {
//     display.drawBitmap(0, 0, Bluetooth_icon, 22, 22, 10, SH110X_WHITE);
//     display.display();
//   }
// }
