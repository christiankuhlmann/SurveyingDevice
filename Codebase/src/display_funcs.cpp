#include "display_funcs.h"

using namespace OLED;

DisplayHandler::DisplayHandler() {
}

Point::Point(uint16_t px, uint16_t py)
{
	x = px;
	y = py;
}

Point rotatePoint(const OLED::Point p, const uint16_t cx, const uint16_t cy , const float rads)
{
	return Point(	std::round(cos(rads) * (p.x - cx) - sin(rads) * (p.y - cy) + cx),
                  	std::round(sin(rads) * (p.x - cx) + cos(rads) * (p.y - cy) + cy));
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

    Paint_NewImage(BlackImage, OLED_2IN42_WIDTH, OLED_2IN42_HEIGHT, 270, BLACK);  

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


void DisplayHandler::drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3,
                    uint16_t colour, DOT_PIXEL linewidth, LINE_STYLE linestyle)
{
    Paint_DrawLine(x1, y1, x2, y2, colour, linewidth, linestyle);
    Paint_DrawLine(x1, y1, x3, y3, colour, linewidth, linestyle);
    Paint_DrawLine(x2, y2, x3, y3, colour, linewidth, linestyle);

}

void DisplayHandler::drawCompass(uint16_t cx, uint16_t cy, uint16_t line_length, uint16_t arrow_length) {
	// display.fillRect(0, 0, SCREEN_WIDTH, 28, SH110X_WHITE);
	Paint_DrawLine(cx-line_length,cy, cx+line_length, cy, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
	Paint_DrawLine(cx,cy-line_length, cx, cy+line_length, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

	// North Triangle
	drawTriangle( 	cx,cy+line_length,
                    cx-arrow_length,cy+line_length-arrow_length,
                    cx+arrow_length,cy+line_length-arrow_length,
                    WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

	// South Triangle
	drawTriangle( 	cx,cy-line_length,
                    cx-arrow_length,cy-line_length+arrow_length,
                    cx+arrow_length,cy-line_length+arrow_length,
                    WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

	// East Triangle
	drawTriangle( 	cx+line_length,cy,
                    cx+line_length-arrow_length,cy-arrow_length,
                    cx+line_length-arrow_length,cy+arrow_length,
                    WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

  // West Triangle
	drawTriangle( 	cx-line_length,cy,
                    cx-line_length+arrow_length,cy-arrow_length,
                    cx-line_length+arrow_length,cy+arrow_length,
                    WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
}

void DisplayHandler::drawCentered(String str, uint16_t cx, uint16_t cy, uint16_t size = 12)
{
    Paint_DrawString_EN(cx - (uint16_t)((str.length()/2.0)*size*6), cy-size*6/2, str.c_str(), &Font12, WHITE, WHITE);
	// display.setCursor(cx - (int)((str.length()/2.0)*size*6), cy-size*6/2);
}

void DisplayHandler::drawCircleHelper(uint16_t cx, uint16_t cy, uint16_t r, uint8_t quadrant, bool filled)
{
    if (!filled) Paint_DrawCircle(cx, cy, r, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    else Paint_DrawCircle(cx, cy, r, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    switch (quadrant)
    {
    case 1: // Top right
        Paint_DrawRectangle(cx, cy ,cx-r, cy-r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Top Left
        Paint_DrawRectangle(cx, cy ,cx+r, cy+r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Bottom Right
        Paint_DrawRectangle(cx, cy ,cx-r, cy+r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Bottom Left
        break;
    case 2: // Bottom right
        Paint_DrawRectangle(cx, cy ,cx-r, cy-r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Top Left
        Paint_DrawRectangle(cx, cy ,cx+r, cy-r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Top Right
        Paint_DrawRectangle(cx, cy ,cx-r, cy+r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Bottom Left
        break;
    case 3: // Bottom left
        Paint_DrawRectangle(cx, cy ,cx-r, cy-r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Top Left
        Paint_DrawRectangle(cx, cy ,cx+r, cy-r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Top Right
        Paint_DrawRectangle(cx, cy ,cx+r, cy+r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Bottom Right
        break;
    case 4: // Top left
        Paint_DrawRectangle(cx, cy ,cx+r, cy-r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Top Right
        Paint_DrawRectangle(cx, cy ,cx+r, cy+r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Bottom Right
        Paint_DrawRectangle(cx, cy ,cx-r, cy+r, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL); // Bottom Left
        break;
    default:
        break;
    }
    
    

}

void DisplayHandler::drawStaticCalib(CompassDirection pointing, CompassDirection facing, const char progress[5])
{
	String str1("CALIB ");
	String str2(progress);
	String str3 = str1 + str2;

	Paint_DrawRectangle(0, 0, SCREEN_WIDTH, 28, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
	drawCentered(str3.c_str(),SCREEN_WIDTH/2,7,2);

	drawCentered("Pointing",SCREEN_WIDTH/4,25,1);
	drawCentered(directionsArr[(int)pointing],SCREEN_WIDTH/4,40,1);

	drawCompassDirection(SCREEN_WIDTH/4,90,28,3,pointing);

	drawCentered("Facing",3*SCREEN_WIDTH/4,25,1);
	drawCentered(directionsArr[(int)facing],3*SCREEN_WIDTH/4,40,1);

	drawCompassDirection(3*SCREEN_WIDTH/4,90,28,3,facing);
}

void DisplayHandler::drawLaserCalib(const float angle, const char centre_text[7], const char progress[5])
{
	String str1("CALIB ");
	String str2(progress);
	String str3 = str1 + str2;

	drawCentered(str3.c_str(),SCREEN_WIDTH/2,7,2);

	const int radius = 45;
	Point center(canvas_center_x,canvas_center_y);
	Point p_default(center.x,center.y-radius);
	Point p_start(center.x,center.y-(radius+10));
	Point p_end(center.x,center.y-(radius+10));

	int quad = floor(angle*0.95/M_PI_2);
	Serial.println(quad);

	p_start = rotatePoint(p_start,center.x,center.y,angle);
	p_end = rotatePoint(p_end,center.x,center.y,(quad+1)*M_PI_2);
	

	// display.drawCircle(center.x,center.y,radius,SH110X_WHITE);
	switch (quad){

		case 3:
			// Draw quadrant 4
			drawCircleHelper(center.x,center.y,radius,4);

		case 2:
			// Draw quadrant 3
			drawCircleHelper(center.x,center.y,radius,3);

		case 1:
			// Draw quadrant 2
			drawCircleHelper(center.x,center.y,radius,2);

		case 0:
			// Draw quadrant 1
			drawCircleHelper(center.x,center.y,radius,1);

		default:
		break;
	}

	drawTriangle(center.x,center.y, p_start.x,p_start.y, p_end.x, p_end.y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

	int arrow_length = 10;
	p_start.x = p_default.x;
	p_start.y = p_default.y;
	p_end.x = p_start.x - arrow_length / M_SQRT2;
	p_end.y = p_start.y + arrow_length / M_SQRT2;
	p_end = rotatePoint(p_end,center.x,center.y,angle);
	p_start = rotatePoint(p_start,center.x,center.y,angle);
	Paint_DrawLine(p_start.x, p_start.y, p_end.x, p_end.y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

	p_start.x = p_default.x;
	p_start.y = p_default.y;
	p_end.x = p_start.x - arrow_length / M_SQRT2 * 1.4;
	p_end.y = p_start.y - arrow_length / M_SQRT2 * 0.6;
	p_end = rotatePoint(p_end,center.x,center.y,angle);
	p_start = rotatePoint(p_start,center.x,center.y,angle);
	Paint_DrawLine(p_start.x, p_start.y, p_end.x, p_end.y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

	drawCentered(String(centre_text)+String("deg"),center.x,center.y,2);
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

// void DisplayHandler::displayYN(const char prompt[11], bool YN)
// {

// 	const int prompt_height = canvas_center_y - 30;
// 	const int selector_height = canvas_center_y;

// 	// String str(prompt);
// 	drawCentered(prompt,canvas_center_x,prompt_height,2);

// 	if (YN)
// 	{
// 		display.fillRect(	canvas_center_x - 60, selector_height - 15,
// 							50, 30,
// 							SH110X_WHITE);

// 		display.fillRect(	canvas_center_x - 58,selector_height - 13,
// 							46, 26,
// 							SH110X_BLACK);

// 		drawCentered("YES",canvas_center_x - 34, selector_height-1, 2);
// 		drawCentered("NO",canvas_center_x + 34, selector_height-1, 2);

// 	} else {

// 		display.fillRect(	canvas_center_x + 8, selector_height - 15,
// 							50, 30,
// 							SH110X_WHITE);

// 		display.fillRect(	canvas_center_x + 10, selector_height - 13,
// 							46, 26,
// 							SH110X_BLACK);

// 		drawCentered("YES",canvas_center_x - 34, selector_height-1, 2);
// 		drawCentered("NO",canvas_center_x + 34, selector_height-1, 2);
// 	}
// }

// void DisplayHandler::displayYN(const char prompt_top[11], const char prompt_btm[11], bool YN)
// {

// 	const int prompt_height = canvas_center_y - 40;
// 	const int selector_height = canvas_center_y + 7;

// 	// String str(prompt);
// 	drawCentered(prompt_top,canvas_center_x,prompt_height,2);
// 	drawCentered(prompt_btm,canvas_center_x,prompt_height+18,2);

// 	if (YN)
// 	{
// 		display.fillRect(	canvas_center_x - 60, selector_height - 15,
// 							50, 30,
// 							SH110X_WHITE);

// 		display.fillRect(	canvas_center_x - 58,selector_height - 13,
// 							46, 26,
// 							SH110X_BLACK);

// 		drawCentered("YES",canvas_center_x - 34, selector_height-1, 2);
// 		drawCentered("NO",canvas_center_x + 34, selector_height-1, 2);

// 	} else {

// 		display.fillRect(	canvas_center_x + 8, selector_height - 15,
// 							50, 30,
// 							SH110X_WHITE);

// 		display.fillRect(	canvas_center_x + 10, selector_height - 13,
// 							46, 26,
// 							SH110X_BLACK);

// 		drawCentered("YES",canvas_center_x - 34, selector_height-1, 2);
// 		drawCentered("NO",canvas_center_x + 34, selector_height-1, 2);
// 	}
// }



// void DisplayHandler::Sensor_cal_status(int sensor_status) {
//   display.setTextSize(2);
//   display.setCursor(23, 4);
//   display.print(sensor_status);
//   // display.display();
// }

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

// void DisplayHandler::drawBattery(int batt_percentage) {
//   display.drawRect(90, 0, 32, 15, SH110X_WHITE);
//   display.drawRect(122, 4, 3, 6, SH110X_WHITE);
//   batt_level = (batt_percentage/100.00)*32;
//   display.fillRect(90, 0, batt_level, 15, SH110X_WHITE);
//   display.fillRect(91+batt_level, 1, 30-batt_level, 13, SH110X_BLACK);
//   display.setTextSize(2);
//   display.setCursor(50, 0);
//   display.print(batt_percentage);
//   display.print("%");
//   display.display();
// }

void DisplayHandler::clearDisplay() {
   OLED_2IN42_Clear(); 
   Paint_Clear(BLACK);  
}

void DisplayHandler::update() {
   OLED_2IN42_Display(BlackImage);
}

void DisplayHandler::displayLoading(const char prompt_top[11], const char prompt_btm[11], int count)
{
	const int n_points = 10;
	Point initial_point(canvas_center_x, canvas_center_y+3);
	Point loading_dot(0,0);

	const int prompt_height = canvas_center_y - 40;
	const int selector_height = canvas_center_y + 7;

	drawCentered(prompt_top,canvas_center_x,prompt_height,2);
	drawCentered(prompt_btm,canvas_center_x,prompt_height+18,2);

	for (int i = 0; i<n_points; i++)
	{
		loading_dot = rotatePoint(initial_point, canvas_center_x, canvas_center_y+22, i%10 * M_PI/5);
		if (i == count % n_points)
		{
			Paint_DrawCircle(loading_dot.x, loading_dot.y, 3, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		} else {
			Paint_DrawCircle(loading_dot.x, loading_dot.y, 2, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
		}
	}
}