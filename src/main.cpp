////////////////////////////////////////////////
/*  Scriptrunner                              */
/*                                            */
/*  Created by Rafa Vico                      */
/*  December 2019                             */
/*                                            */
/*  License: GPL v.2                          */
////////////////////////////////////////////////

///////////////////////////////////
/*  Libraries                    */
///////////////////////////////////
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

///////////////////////////////////
/*  Joystick codes               */
///////////////////////////////////

#define GCW_BUTTON_UP           SDLK_UP
#define GCW_BUTTON_DOWN         SDLK_DOWN
#define GCW_BUTTON_LEFT         SDLK_LEFT
#define GCW_BUTTON_RIGHT        SDLK_RIGHT
#define GCW_BUTTON_A            SDLK_LCTRL
#define GCW_BUTTON_B            SDLK_LALT
#define GCW_BUTTON_X            SDLK_SPACE
#define GCW_BUTTON_Y            SDLK_LSHIFT
#define GCW_BUTTON_L1           SDLK_TAB
#define GCW_BUTTON_R1           SDLK_BACKSPACE
#define GCW_BUTTON_L2           SDLK_PAGEUP
#define GCW_BUTTON_R2           SDLK_PAGEDOWN
#define GCW_BUTTON_SELECT       SDLK_ESCAPE
#define GCW_BUTTON_START        SDLK_RETURN
#define GCW_BUTTON_L3           SDLK_KP_DIVIDE
#define GCW_BUTTON_R3           SDLK_KP_PERIOD
#define GCW_BUTTON_POWER        SDLK_HOME
#define GCW_BUTTON_VOLUP        0 //SDLK_PAUSE
//#define GCW_BUTTON_VOLDOWN      0
#define GCW_JOYSTICK_DEADZONE   1000

///////////////////////////////////
/*  Other defines                */
///////////////////////////////////
#define TRUE   1
#define FALSE  0

#define BORDER_NO       0
#define BORDER_SINGLE   1
#define BORDER_ROUNDED  2

#define MODE_BROWSE     1
#define MODE_EXECUTING  2
#define MODE_FINISH     3
#define MODE_CONFIRM    4
#define MODE_VIEWCODE   5

///////////////////////////////////
/*  Structs                      */
///////////////////////////////////
struct joystick_state
{
  int j1_left;
  int j1_right;
  int j1_up;
  int j1_down;
  int button_l3;
  int j2_left;
  int j2_right;
  int j2_up;
  int j2_down;
  int button_r3;
  int pad_left;
  int pad_right;
  int pad_up;
  int pad_down;
  int button_a;
  int button_b;
  int button_x;
  int button_y;
  int button_l1;
  int button_l2;
  int button_r1;
  int button_r2;
  int button_select;
  int button_start;
  int button_power;
  int button_voldown;
  int button_volup;
  int escape;
  int any;
};

struct script_data
{
  std::string folder;
  std::string filename;
  std::string title;
  std::string desc;
};

///////////////////////////////////
/*  Globals                      */
///////////////////////////////////
SDL_Surface* screen;   		    // screen to work
int done=FALSE;
TTF_Font* font;                 // used font
TTF_Font* font2;                 // used font
SDL_Joystick* joystick;         // used joystick
joystick_state mainjoystick;
Uint8* keys=SDL_GetKeyState(NULL);

int mode_app=MODE_BROWSE;
pthread_t sr_th;
pthread_mutex_t lock_consoleoutput;

int workingicon=0;

// Lists
int script_list_idx=0;
int script_list_selected=0;
std::vector<script_data> script_list;
std::vector<std::string> desc_lines;
int consolelines=0;
int consolelines_idx[100];
std::string consoleoutput;
int console_idx=0;
int automated_list=TRUE;

// graphics
SDL_Surface *img_background;
SDL_Surface *img_buttons[14];
SDL_Surface *img_working[4];
//sonidos
//Mix_Chunk *sound_tone;

///////////////////////////////////
/*  Messages                     */
///////////////////////////////////
const char* msg[8]=
{
  "exit",
  "run",
  "accept",
  "cancel",
  "back",
  "Are you sure?",
  "view",
  "move"
};

///////////////////////////////////
/*  Function declarations        */
///////////////////////////////////
void process_events();
void process_joystick();

///////////////////////////////////
/*  Debug Functions              */
///////////////////////////////////
/*std::string getCurrentDateTime( std::string s )
{
    time_t now = time(0);
    struct tm  tstruct;
    char  buf[80];
    tstruct = *localtime(&now);
    if(s=="now")
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    else if(s=="date")
        strftime(buf, sizeof(buf), "%Y-%m-%d", &tstruct);
    return std::string(buf);
}

void Logger( std::string logMsg )
{

    std::string filePath = "/usr/local/home/log_"+getCurrentDateTime("date")+".txt";
    std::string now = getCurrentDateTime("now");
    std::ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app );
    ofs << now << '\t' << logMsg << '\n';
    ofs.close();
}*/

///////////////////////////////////
/*  Draw a pixel in surface      */
///////////////////////////////////
void putpixel(SDL_Surface *dst, int x, int y, Uint32 pixel)
{
    int byteperpixel = dst->format->BytesPerPixel;
    Uint8 *p = (Uint8*)dst->pixels + y * dst->pitch + x * byteperpixel;
    // Adress to pixel
    *(Uint32 *)p = pixel;
}

///////////////////////////////////
/*  Draw a line in surface       */
///////////////////////////////////
void drawLine(SDL_Surface* dst, int x0, int y0, int x1, int y1, Uint32 pixel)
{
    int i;
    double x = x1 - x0;
    double y = y1 - y0;
    double length = sqrt( x*x + y*y );
    double addx = x / length;
    double addy = y / length;
    x = x0;
    y = y0;

    for ( i = 0; i < length; i += 1) {
        putpixel(dst, x, y, pixel);
        x += addx;
        y += addy;
    }
}

///////////////////////////////////
/*  Print text in surface        */
///////////////////////////////////
void draw_text(SDL_Surface* dst, TTF_Font* f, char* string, Sint16 x, Sint16 y, Uint8 fR, Uint8 fG, Uint8 fB)
{
  if(dst && string && f)
  {
    SDL_Color foregroundColor={fR,fG,fB};
    SDL_Surface *textSurface=TTF_RenderText_Blended(f,string,foregroundColor);
    if(textSurface)
    {
      SDL_Rect textLocation={x,y,0,0};
      SDL_BlitSurface(textSurface,NULL,dst,&textLocation);
      SDL_FreeSurface(textSurface);
    }
  }
}

///////////////////////////////////
/*  Return text width             */
///////////////////////////////////
int text_width(char* string,TTF_Font* f=font)
{
  int nx=0,ny=0;
  TTF_SizeText(f,string,&nx,&ny);

  return nx;
}

///////////////////////////////////
/*  Draw a rectangle             */
///////////////////////////////////
void draw_rectangle(int x, int y, int w, int h, SDL_Color* c, int border=BORDER_NO, SDL_Color* bc=NULL)
{
  SDL_Rect dest;

  // if size<=2pixels, don't draw border
  if(w<=2 || h<=2)
  {
    border=BORDER_NO;
    bc=c;
  }
  else if(border==BORDER_ROUNDED && (w<=4 || h<=4))
  {
    border=BORDER_NO;
    bc=c;
  }

  switch(border)
  {
    case BORDER_NO:
      dest.x=x;
      dest.y=y;
      dest.w=w;
      dest.h=h;
      if(c)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format,c->r,c->g,c->b));
      break;
    case BORDER_SINGLE:
      dest.x=x;
      dest.y=y;
      dest.w=w;
      dest.h=h;
      if(bc)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format,bc->r,bc->g,bc->b));
      dest.x=x+1;
      dest.y=y+1;
      dest.w=w-2;
      dest.h=h-2;
      if(c)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format,c->r,c->g,c->b));
      break;
    case BORDER_ROUNDED:
      dest.x=x+1;
      dest.y=y+1;
      dest.w=w-2;
      dest.h=h-2;
      if(bc)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format,bc->r,bc->g,bc->b));
      dest.x=x+2;
      dest.y=y;
      dest.w=w-4;
      dest.h=h;
      if(bc)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format,bc->r,bc->g,bc->b));
      dest.x=x;
      dest.y=y+2;
      dest.w=w;
      dest.h=h-4;
      if(bc)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format,bc->r,bc->g,bc->b));
      dest.x=x+2;
      dest.y=y+1;
      dest.w=w-4;
      dest.h=h-2;
      if(c)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format,c->r,c->g,c->b));
      dest.x=x+1;
      dest.y=y+2;
      dest.w=w-2;
      dest.h=h-4;
      if(c)
        SDL_FillRect(screen, &dest, SDL_MapRGB(screen->format,c->r,c->g,c->b));
      break;
  }
}

void uppertext(char* text)
{
  while(*text)
  {
    *text=toupper((unsigned char)*text);
    text++;
  }
}

///////////////////////////////////
/*  Clear values from joystick   */
/*  structure                    */
///////////////////////////////////
void clear_joystick_state()
{
  mainjoystick.j1_left=0;
  mainjoystick.j1_right=0;
  mainjoystick.j1_up=0;
  mainjoystick.j1_down=0;
  mainjoystick.button_l3=FALSE;
  mainjoystick.j2_left=0;
  mainjoystick.j2_right=0;
  mainjoystick.j2_up=0;
  mainjoystick.j2_down=0;
  mainjoystick.button_r3=FALSE;
  mainjoystick.pad_left=FALSE;
  mainjoystick.pad_right=FALSE;
  mainjoystick.pad_up=FALSE;
  mainjoystick.pad_down=FALSE;
  mainjoystick.button_a=FALSE;
  mainjoystick.button_b=FALSE;
  mainjoystick.button_x=FALSE;
  mainjoystick.button_y=FALSE;
  mainjoystick.button_l1=FALSE;
  mainjoystick.button_l2=FALSE;
  mainjoystick.button_r1=FALSE;
  mainjoystick.button_r2=FALSE;
  mainjoystick.button_select=FALSE;
  mainjoystick.button_start=FALSE;
  mainjoystick.button_power=FALSE;
  mainjoystick.button_voldown=FALSE;
  mainjoystick.button_volup=FALSE;
  mainjoystick.escape=FALSE;
  mainjoystick.any=FALSE;
}

///////////////////////////////////
/*  Load graphic with alpha      */
///////////////////////////////////
void load_imgalpha(const char* file, SDL_Surface *&dstsurface)
{
  SDL_Surface *tmpsurface;

  tmpsurface=IMG_Load(file);
  if(tmpsurface)
  {
    dstsurface=SDL_CreateRGBSurface(SDL_SRCCOLORKEY, tmpsurface->w, tmpsurface->h, 16, 0,0,0,0);
    SDL_BlitSurface(tmpsurface,NULL,dstsurface,NULL);
    SDL_SetColorKey(dstsurface,SDL_SRCCOLORKEY,SDL_MapRGB(screen->format,255,0,255));
    SDL_FreeSurface(tmpsurface);
  }
}

/*void load_imgalpha_array(const char* file, int w, int h, SDL_Surface* dstsurface[])
{
  SDL_Surface *tmpsurface;

  tmpsurface=IMG_Load(file);
  int hcells=tmpsurface->h/h;
  int wcells=tmpsurface->w/w;
  if(tmpsurface)
  {
    for(int g=0; g<hcells; g++)
    {
      for(int f=0; f<wcells; f++)
      {
        dstsurface[g*wcells+f]=SDL_CreateRGBSurface(SDL_SRCCOLORKEY, w, h, 16, 0,0,0,0);
        SDL_Rect src;
        src.x=f*w;
        src.y=g*h;
        src.w=w;
        src.h=h;
        SDL_BlitSurface(tmpsurface,&src,dstsurface[g*wcells+f],NULL);
        SDL_SetColorKey(dstsurface[g*wcells+f],SDL_SRCCOLORKEY,SDL_MapRGB(screen->format,255,0,255));
      }
    }
    SDL_FreeSurface(tmpsurface);
  }
}*/

///////////////////////////////////
/*  Init the app                 */
///////////////////////////////////
void init_game()
{
  mkdir("/usr/local/home/.scriptrunner",0);
  pthread_mutex_init(&lock_consoleoutput,NULL);

  // Initalizations
  srand(time(NULL));
  SDL_JoystickEventState(SDL_ENABLE);
  joystick=SDL_JoystickOpen(0);
  SDL_ShowCursor(0);

  Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S16, MIX_DEFAULT_CHANNELS, 1024);

  TTF_Init();
  font=TTF_OpenFont("media/pixelberry.ttf", 8);
  //font2=TTF_OpenFont("media/basis33.ttf", 15);
  //font2=TTF_OpenFont("media/712_serif.ttf", 15);
  font2=TTF_OpenFont("media/pfcb.ttf", 8);

  load_imgalpha("media/background.png",img_background);

  SDL_Surface *tmpsurface;

  tmpsurface=IMG_Load("media/buttons.png");
  if(tmpsurface)
  {
    for(int f=0; f<14; f++)
    {
      img_buttons[f]=SDL_CreateRGBSurface(SDL_SRCCOLORKEY, 10, 10, 16, 0,0,0,0);
      SDL_Rect src;
      src.x=f*10;
      src.y=0;
      src.w=10;
      src.h=10;
      SDL_BlitSurface(tmpsurface,&src,img_buttons[f],NULL);
      SDL_SetColorKey(img_buttons[f],SDL_SRCCOLORKEY,SDL_MapRGB(screen->format,255,0,255));
    }
    SDL_FreeSurface(tmpsurface);
  }

  tmpsurface=IMG_Load("media/working.png");
  if(tmpsurface)
  {
    for(int f=0; f<4; f++)
    {
      img_working[f]=SDL_CreateRGBSurface(SDL_SRCCOLORKEY, 16, 16, 16, 0,0,0,0);
      SDL_Rect src;
      src.x=f*16;
      src.y=0;
      src.w=16;
      src.h=16;
      SDL_BlitSurface(tmpsurface,&src,img_working[f],NULL);
      SDL_SetColorKey(img_working[f],SDL_SRCCOLORKEY,SDL_MapRGB(screen->format,255,0,255));
    }
    SDL_FreeSurface(tmpsurface);
  }
}

///////////////////////////////////
/*  Finish app, free memory      */
///////////////////////////////////
void end_game()
{
  SDL_FillRect(screen, NULL, 0x000000);

  if(SDL_JoystickOpened(0))
    SDL_JoystickClose(joystick);

  // Free graphics
  if(img_background)
    SDL_FreeSurface(img_background);
  for(int f=0; f<10; f++)
    if(img_buttons[f])
      SDL_FreeSurface(img_buttons[f]);

  // Free sounds
  Mix_HaltChannel(-1);
  //Mix_FreeChunk(sound_tone);
  Mix_CloseAudio();
  TTF_CloseFont(font2);
  TTF_CloseFont(font);
}

///////////////////////////////////
/*  Process buttons events       */
///////////////////////////////////
void process_events()
{
  SDL_Event event;
  static int joy_pressed=FALSE;

  while(SDL_PollEvent(&event))
  {
    switch(event.type)
    {
      case SDL_KEYDOWN:
        switch(event.key.keysym.sym)
        {
          case GCW_BUTTON_LEFT:
            mainjoystick.pad_left=TRUE;
            break;
          case GCW_BUTTON_RIGHT:
            mainjoystick.pad_right=TRUE;
            break;
          case GCW_BUTTON_UP:
            mainjoystick.pad_up=TRUE;
            break;
          case GCW_BUTTON_DOWN:
            mainjoystick.pad_down=TRUE;
            break;
          case GCW_BUTTON_Y:
            mainjoystick.button_y=TRUE;
            break;
          case GCW_BUTTON_X:
            mainjoystick.button_x=TRUE;
            break;
          case GCW_BUTTON_B:
            mainjoystick.button_b=TRUE;
            break;
          case GCW_BUTTON_A:
            mainjoystick.button_a=TRUE;
            break;
          case GCW_BUTTON_L1:
            mainjoystick.button_l1=TRUE;
            break;
          case GCW_BUTTON_L2:
            mainjoystick.button_l2=TRUE;
            break;
          case GCW_BUTTON_R1:
            mainjoystick.button_r1=TRUE;
            break;
          case GCW_BUTTON_R2:
            mainjoystick.button_r2=TRUE;
            break;
          case GCW_BUTTON_L3:
            mainjoystick.button_l3=TRUE;
            break;
          case GCW_BUTTON_R3:
            mainjoystick.button_r3=TRUE;
            break;
          case GCW_BUTTON_SELECT:
            mainjoystick.button_select=TRUE;
            break;
          case GCW_BUTTON_START:
            mainjoystick.button_start=TRUE;
            break;
          // Volume Up and Volume Down can't be detected individual
          case GCW_BUTTON_VOLUP:
            mainjoystick.button_volup=TRUE;
            mainjoystick.button_voldown=TRUE;
            break;
//          case GCW_BUTTON_VOLDOWN:
//            mainjoystick.button_voldown=1;
//            break;
          case GCW_BUTTON_POWER:
            mainjoystick.button_power=TRUE;
            break;
        }
        mainjoystick.any=TRUE;
        break;
      case SDL_JOYAXISMOTION:
        if(joy_pressed && SDL_JoystickGetAxis(joystick,0)>-GCW_JOYSTICK_DEADZONE && SDL_JoystickGetAxis(joystick,0)<GCW_JOYSTICK_DEADZONE && SDL_JoystickGetAxis(joystick,1)>-GCW_JOYSTICK_DEADZONE && SDL_JoystickGetAxis(joystick,1)<GCW_JOYSTICK_DEADZONE)
        {
          joy_pressed=FALSE;
        }

        if(!joy_pressed)
        {
            switch(event.jaxis.axis)
            {
              case 0:
                if(event.jaxis.value<0)
                {
                  mainjoystick.j1_left=event.jaxis.value;
                  mainjoystick.j1_right=0;
                  if(event.jaxis.value<-GCW_JOYSTICK_DEADZONE)
                  {
                    mainjoystick.any=TRUE;
                    joy_pressed=TRUE;
                  }
                }
                else
                {
                  mainjoystick.j1_right=event.jaxis.value;
                  mainjoystick.j1_left=0;
                  if(event.jaxis.value>GCW_JOYSTICK_DEADZONE)
                  {
                    mainjoystick.any=TRUE;
                    joy_pressed=TRUE;
                  }
                }
                break;
              case 1:
                if(event.jaxis.value<0)
                {
                  mainjoystick.j1_up=event.jaxis.value;
                  mainjoystick.j1_down=0;
                  if(event.jaxis.value<-GCW_JOYSTICK_DEADZONE)
                  {
                    mainjoystick.any=TRUE;
                    joy_pressed=TRUE;
                  }
                }
                else
                {
                  mainjoystick.j1_down=event.jaxis.value;
                  mainjoystick.j1_up=0;
                  if(event.jaxis.value>GCW_JOYSTICK_DEADZONE)
                  {
                    mainjoystick.any=TRUE;
                    joy_pressed=TRUE;
                  }
                }
                break;
              case 2:
                if(event.jaxis.value<0)
                {
                  mainjoystick.j2_left=event.jaxis.value;
                  mainjoystick.j2_right=0;
                  if(event.jaxis.value<-GCW_JOYSTICK_DEADZONE)
                  {
                    mainjoystick.any=TRUE;
                    joy_pressed=TRUE;
                  }
                }
                else
                {
                  mainjoystick.j2_right=event.jaxis.value;
                  mainjoystick.j2_left=0;
                  if(event.jaxis.value>GCW_JOYSTICK_DEADZONE)
                  {
                    mainjoystick.any=TRUE;
                    joy_pressed=TRUE;
                  }
                }
                break;
              case 3:
                if(event.jaxis.value<0)
                {
                  mainjoystick.j2_up=event.jaxis.value;
                  mainjoystick.j2_down=0;
                  if(event.jaxis.value<-GCW_JOYSTICK_DEADZONE)
                  {
                    mainjoystick.any=TRUE;
                    joy_pressed=TRUE;
                  }
                }
                else
                {
                  mainjoystick.j2_down=event.jaxis.value;
                  mainjoystick.j2_up=0;
                  if(event.jaxis.value>GCW_JOYSTICK_DEADZONE)
                  {
                    mainjoystick.any=TRUE;
                    joy_pressed=TRUE;
                  }
                }
                break;
            }
        }
        break;
      }
  }
}

///////////////////////////////////
/*  Process keyboard and joystick*/
/*  (no events), and save in     */
/*  mainjoystick variable        */
///////////////////////////////////
void process_joystick()
{
  SDL_Event event;
  while(SDL_PollEvent(&event));

  if(keys[GCW_BUTTON_B])
    mainjoystick.button_b=TRUE;
  if(keys[GCW_BUTTON_A])
    mainjoystick.button_a=TRUE;
  if(keys[GCW_BUTTON_Y])
    mainjoystick.button_y=TRUE;
  if(keys[GCW_BUTTON_X])
    mainjoystick.button_x=TRUE;

  if(keys[GCW_BUTTON_LEFT])
    mainjoystick.pad_left=TRUE;
  if(keys[GCW_BUTTON_RIGHT])
    mainjoystick.pad_right=TRUE;
  if(keys[GCW_BUTTON_UP])
    mainjoystick.pad_up=TRUE;
  if(keys[GCW_BUTTON_DOWN])
    mainjoystick.pad_down=TRUE;

  if(SDL_JoystickGetAxis(joystick,0)<-GCW_JOYSTICK_DEADZONE)
    mainjoystick.j1_left=SDL_JoystickGetAxis(joystick,0);
  if(SDL_JoystickGetAxis(joystick,0)>GCW_JOYSTICK_DEADZONE)
    mainjoystick.j1_right=SDL_JoystickGetAxis(joystick,0);
  if(SDL_JoystickGetAxis(joystick,1)<-GCW_JOYSTICK_DEADZONE)
    mainjoystick.j1_up=SDL_JoystickGetAxis(joystick,1);
  if(SDL_JoystickGetAxis(joystick,1)>GCW_JOYSTICK_DEADZONE)
    mainjoystick.j1_down=SDL_JoystickGetAxis(joystick,1);
  if(SDL_JoystickGetAxis(joystick,2)<-GCW_JOYSTICK_DEADZONE)
    mainjoystick.j2_left=SDL_JoystickGetAxis(joystick,2);
  if(SDL_JoystickGetAxis(joystick,2)>GCW_JOYSTICK_DEADZONE)
    mainjoystick.j2_right=SDL_JoystickGetAxis(joystick,2);
  if(SDL_JoystickGetAxis(joystick,3)<-GCW_JOYSTICK_DEADZONE)
    mainjoystick.j2_up=SDL_JoystickGetAxis(joystick,3);
  if(SDL_JoystickGetAxis(joystick,3)>GCW_JOYSTICK_DEADZONE)
    mainjoystick.j2_down=SDL_JoystickGetAxis(joystick,3);

  /*if(keys[GCW_BUTTON_POWER])
    mainjoystick.button_power=1;
  if(keys[GCW_BUTTON_VOLUP])
  {
    mainjoystick.button_volup=1;
    mainjoystick.button_voldown=1;
  }
  if(keys[GCW_BUTTON_VOLDOWN])
  {
    mainjoystick.button_volup=1;
    mainjoystick.button_voldown=1;
  }*/

  if(keys[GCW_BUTTON_START])
    mainjoystick.button_start=TRUE;
  if(keys[GCW_BUTTON_SELECT])
    mainjoystick.button_select=TRUE;

  if(keys[GCW_BUTTON_L1])
    mainjoystick.button_l1=TRUE;
  if(keys[GCW_BUTTON_R1])
    mainjoystick.button_r1=TRUE;
  if(keys[GCW_BUTTON_L2])
    mainjoystick.button_l2=TRUE;
  if(keys[GCW_BUTTON_R2])
    mainjoystick.button_r2=TRUE;

  if(keys[GCW_BUTTON_L3])
    mainjoystick.button_l3=TRUE;
  if(keys[GCW_BUTTON_R3])
    mainjoystick.button_r3=TRUE;
}

void clear_consolelines_idx()
{
  for(int f=0; f<100; f++)
    consolelines_idx[f]=0;
  consolelines=0;
}

void update_consoleoutput_index(int diff)
{
  for(int f=0; f<100; f++)
    if(consolelines_idx[f]>0)
      consolelines_idx[f]=consolelines_idx[f]-diff;
}

void format_consoleoutput()
{
  consolelines=0;
  clear_consolelines_idx();

  for(int f=consoleoutput.size()-1; f>0; f--)
  {
    switch(consoleoutput[f])
    {
      // return
      case 10:
        consolelines_idx[consolelines]=f;
        consolelines++;
        if(consolelines>99)
        {
          consoleoutput=consoleoutput.substr(f+1);
          update_consoleoutput_index(f+1);
          break;
        }
        break;
      // backspace
      case 8:
        if(f>1 && consoleoutput[f-1]>=32 && consoleoutput[f-1]<=126)
        {
          consoleoutput=consoleoutput.substr(0,f-1)+consoleoutput.substr(f+1);
          update_consoleoutput_index(2);
        }
        break;
    }
  }
}

void get_stdoutfromcommand(std::string cmd)
{
  consoleoutput="";

  FILE* stream;
  const int max_buffer = 256;
  char buffer[max_buffer];
  //cmd = "'"+cmd+"'";
  cmd.append(" 2>&1");

  stream = popen(cmd.c_str(), "r");
  if (stream)
  {
    while (!feof(stream))
    {
      if (fgets(buffer, max_buffer, stream) != NULL)
      {
        pthread_mutex_lock(&lock_consoleoutput);
        consoleoutput.append(buffer);
        format_consoleoutput();
        pthread_mutex_unlock(&lock_consoleoutput);
      }
    }
    pclose(stream);
  }
  //return data;
}

///////////////////////////////////
/*  Thread                       */
///////////////////////////////////
static void* runscript_thd(void* p)
{
  get_stdoutfromcommand(script_list[script_list_selected].folder+"'"+script_list[script_list_selected].filename+"'");
  mode_app=MODE_FINISH;
	return NULL;
}

void format_desc(std::string desc)
{
  desc_lines.clear();
  desc_lines.push_back("");

  std::istringstream buf(desc);
  for(std::string word; buf>>word;)
  {
    std::string line=desc_lines[desc_lines.size()-1]+word;
    if(text_width((char*)line.c_str())>245)
      desc_lines.push_back(word);
    else
    {
      if(desc_lines[desc_lines.size()-1].size()>0)
        desc_lines[desc_lines.size()-1]=desc_lines[desc_lines.size()-1]+" "+word;
      else
        desc_lines[desc_lines.size()-1]=word;
    }
  }
}

std::string trim(const std::string& str)
{
  size_t first = str.find_first_not_of(' ');
  if (std::string::npos == first)
    return str;
  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}

int read_script_data(std::string filename,script_data& sc)
{
  int is_script=FALSE;
  std::ifstream file(filename.c_str());
  std::string str;

  if(std::getline(file,str))
  { // read 1st line, check if it is a script
    if(str.substr(0,2)=="#!")
    {
      for(int f=0; f<4; f++)
      {
        if(std::getline(file,str))
        {
          trim(str);
          if(str.substr(0,1)=="#")
          {
            str=trim(str.substr(1));
            if(str.substr(0,5)=="title")
            {
              str=trim(str.substr(5));
              if(str.substr(0,1)=="=")
              {
                str=trim(str.substr(1));
                sc.title=str;
                is_script=TRUE;
              }
            }
            else if(str.substr(0,4)=="desc")
            {
              str=trim(str.substr(4));
              if(str.substr(0,1)=="=")
              {
                str=trim(str.substr(1));
                sc.desc=str;
                is_script=TRUE;
              }
            }
          }
        }
      }
    }
  }
  file.close();

  return is_script;
}

void load_scripts()
{
  automated_list=TRUE;
  console_idx=0;
  consoleoutput="";
  clear_consolelines_idx();

  script_list.clear();

  DIR* dirp = opendir("scripts");
  struct dirent * dp;
  if(dirp)
  {
    while((dp = readdir(dirp)) != NULL)
    {
      script_data sc;
      sc.folder="scripts/";
      sc.filename=std::string(dp->d_name);

      if(sc.filename!="" && sc.filename!="." && sc.filename!=".." && sc.filename[0]!='.')
      {
        if(read_script_data(sc.folder+sc.filename,sc))
          script_list.push_back(sc);
      }
    }
    closedir(dirp);
  }

  dirp = opendir("/usr/local/home/.scriptrunner");
  if(dirp)
  {
    while((dp = readdir(dirp)) != NULL)
    {
      script_data sc;
      sc.folder="/usr/local/home/.scriptrunner/";
      sc.filename=std::string(dp->d_name);

      if(sc.filename!="" && sc.filename!="." && sc.filename!=".." && sc.filename[0]!='.')
      {
        if(read_script_data(sc.folder+sc.filename,sc))
          script_list.push_back(sc);
      }
    }
    closedir(dirp);
  }
}

void view_script()
{
  std::ifstream file(script_list[script_list_selected].folder+"/"+script_list[script_list_selected].filename);
  std::string str;
  consoleoutput="";

  while(std::getline(file,str))
  {
    consoleoutput.append(str+"\n");
  }
  file.close();
  format_consoleoutput();
  automated_list=FALSE;
  console_idx=0;
}

///////////////////////////////////
/*  Update browse mode           */
///////////////////////////////////
void update_browse()
{
  clear_joystick_state();
  //process_joystick();
  process_events();

  if(mainjoystick.button_start)
    done=TRUE;
  if(mainjoystick.pad_up && script_list_selected>0)
  {
    script_list_selected--;
    if(script_list_selected<script_list_idx)
      script_list_idx--;
  }
  if(mainjoystick.pad_down && script_list_selected<(script_list.size()-1))
  {
    script_list_selected++;
    if(script_list_selected-script_list_idx>19)
      script_list_idx++;
  }
  if(mainjoystick.button_l1)
  {
    script_list_selected-=15;
    if(script_list_selected<0)
      script_list_selected=0;
    if(script_list_selected<script_list_idx)
      script_list_idx=script_list_selected;
  }
  if(mainjoystick.button_r1)
  {
    script_list_selected+=15;
    if(script_list_selected>=script_list.size())
      script_list_selected=script_list.size()-1;
    if(script_list_selected-script_list_idx>19)
    {
      script_list_idx=script_list.size()-19;
      if(script_list_idx<0)
        script_list_idx=0;
    }
  }
  if(mainjoystick.button_a)
  {
    mode_app=MODE_CONFIRM;
  }
  if(mainjoystick.button_x)
  {
    view_script();
    mode_app=MODE_VIEWCODE;
  }
}

///////////////////////////////////
/*  Draw browse mode             */
///////////////////////////////////
void draw_browse()
{
  //SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,55,37,56));

  // background
  SDL_Rect dest;
  dest.x=0;
  dest.y=0;
  if(img_background)
    SDL_BlitSurface(img_background,NULL,screen,&dest);

  // script list
  if(script_list.size()>0)
  {
    format_desc(script_list[script_list_selected].desc);

    int y;
    if((25+(script_list_selected-script_list_idx+1)*10+5+desc_lines.size()*10)>220)
      y=25-((25+(script_list_selected-script_list_idx+1)*10+5+desc_lines.size()*10)-220);
    else
      y=25;
    int idcount=0;
    // print script list
    while(y<240 && (script_list_idx+idcount)<script_list.size())
    {
      if(script_list_idx<script_list.size())
      {
        if(script_list_idx+idcount==script_list_selected)
        {
          // print description
          SDL_Color c={104,43,130};
          if(desc_lines.size()>0)
          {
            SDL_Color b={55,37,56};
            draw_rectangle(50,y,256,14+desc_lines.size()*10,&b,BORDER_SINGLE,&c);   // description box
            for(int f=0; f<desc_lines.size(); f++)
              draw_text(screen,font,(char*)desc_lines[f].c_str(),55,y+9+f*10,110,238,255);
            //y=y+5+desc_lines.size()*10;
          }
          draw_rectangle(15,y,290,10,&c);   // line of selected script
          dest.x=320-25-text_width((char*)msg[1])-15;
          dest.y=y;
          // view button
          if(img_buttons[6])
            SDL_BlitSurface(img_buttons[6],NULL,screen,&dest);
          draw_text(screen,font,(char*)msg[1],dest.x+15,y,255,255,0);
          dest.x=dest.x-10-text_width((char*)msg[6])-15;
          // run button
          if(img_buttons[8])
            SDL_BlitSurface(img_buttons[8],NULL,screen,&dest);
          draw_text(screen,font,(char*)msg[6],dest.x+15,y,255,255,0);
        }
        // script name
        draw_text(screen,font,(char*)script_list[script_list_idx+idcount].title.c_str(),20,y,255,255,255);
        //draw_text(screen,font,(char*)script_list[script_list_idx+idcount].filename.c_str(),120,y,255,255,255);
        if(script_list_idx+idcount==script_list_selected)
          y=y+15+desc_lines.size()*10;
        else
          y+=10;
        idcount++;
      }
    }
  }

  // header
  SDL_Color col={255,255,255};
  SDL_Color bor={255,65,65};
  draw_rectangle(0,-5,320,25,&col,BORDER_ROUNDED,&bor);
  draw_text(screen,font,(char*)"Scriptrunner",10,3,55,37,56);
  std::stringstream scnt;
  scnt<<(script_list_selected+1);
  scnt<<"/";
  scnt<<script_list.size();
  std::string scrcount;
  scnt>>scrcount;
  draw_text(screen,font,(char*)scrcount.c_str(),320-15-text_width((char*)scrcount.c_str()),5,55,37,56);

  // foot
  col={178,188,194};
  draw_rectangle(0,225,320,15,&col);
  dest.x=320-10-text_width((char*)msg[0])-15;
  dest.y=227;
  if(img_buttons[5])
    SDL_BlitSurface(img_buttons[5],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[0],dest.x+15,dest.y,55,37,56);
}

void update_executing()
{
  clear_joystick_state();
  //process_joystick();
  process_events();

  /*if(mainjoystick.button_start)
    done=TRUE;
  if(mainjoystick.button_b)
    mode_app=MODE_BROWSE;*/
  if(mainjoystick.pad_up && console_idx>0)
  {
    console_idx--;
    automated_list=FALSE;
  }
  if(mainjoystick.pad_down && console_idx<(consolelines-1))
  {
    console_idx++;
    automated_list=FALSE;
  }
  if(mainjoystick.button_l1)
  {
    console_idx-=10;
    if(console_idx<0)
      console_idx=0;
    automated_list=FALSE;
  }
  if(mainjoystick.button_r1)
  {
    console_idx+=10;
    if(console_idx>=consolelines)
      console_idx=consolelines-1;
    automated_list=FALSE;
  }
  /*static int sfon=10;
  if(mainjoystick.button_l2)
  {
    sfon--;
    TTF_CloseFont(font2);
    font2=TTF_OpenFont("media/pfcb.ttf", sfon);
  }
  if(mainjoystick.button_r2)
  {
    sfon++;
    TTF_CloseFont(font2);
    font2=TTF_OpenFont("media/pfcb.ttf",sfon);
  }*/
}

void draw_executing()
{
  SDL_Rect dest;

  // background
  if(mode_app==MODE_EXECUTING || mode_app==MODE_FINISH)
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,55,37,56));
  else
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format,56,152,255));

  // console output
  int x=10;
  int y=20;
  if(automated_list)
  {
    if(consolelines>14)
      console_idx=consolelines-14;
    /*else
      console_idx=0;*/
  }
  pthread_mutex_lock(&lock_consoleoutput);
  for(int f=consolelines_idx[consolelines-console_idx]; f<consoleoutput.size(); f++)
  {
    // Return
    if(consoleoutput[f]==10)
    {
      if(f!=consolelines_idx[consolelines-console_idx])
      {
        x=10;
        y+=10;
      }
    }
    // printable character
    else if(consoleoutput[f]>=32 && consoleoutput[f]<=126)
    {
      char cr[2];
      cr[0]=consoleoutput[f];
      cr[1]=0;
      draw_text(screen,font2,cr,x,y,255,255,255);
      x=x+text_width(cr,font2);
      if(x>305)
      {
        x=10;
        y+=10;
      }
    //draw_text(screen,font,(char*)consoleoutput.c_str(),10,20,255,255,255);
    }
    /*else
    {
      std::stringstream nnss;
      nnss<<"["<<int(consoleoutput[f])<<"]";
      std::string nn;
      nnss>>nn;
      draw_text(screen,font2,(char*)nn.c_str(),x,y,255,255,255);
      x=x+text_width((char*)nn.c_str(),font2);
      if(x>305)
      {
        x=10;
        y+=10;
      }
    }*/
  }
  pthread_mutex_unlock(&lock_consoleoutput);

  // header
  SDL_Color col={255,255,255};
  SDL_Color bor={255,65,65};
  draw_rectangle(0,-5,320,25,&col,BORDER_ROUNDED,&bor);
  draw_text(screen,font,(char*)"Scriptrunner",10,3,55,37,56);

  if(mode_app==MODE_EXECUTING)
  {
    dest.x=320-10-16;
    dest.y=2;
    if(img_working[workingicon])
      SDL_BlitSurface(img_working[workingicon],NULL,screen,&dest);
    workingicon++;
    if(workingicon>2)
      workingicon=0;
  }

  // foot
  col={178,188,194};
  draw_rectangle(0,225,320,15,&col);

  // moving buttons
  dest.x=10+15+10+text_width((char*)msg[4]);
  dest.y=227;
  if(img_buttons[0])
    SDL_BlitSurface(img_buttons[0],NULL,screen,&dest);
  dest.x+=10;
  if(img_buttons[12])
    SDL_BlitSurface(img_buttons[12],NULL,screen,&dest);
  dest.x+=10;
  if(img_buttons[13])
    SDL_BlitSurface(img_buttons[13],NULL,screen,&dest);
  dest.x+=10;
  if(img_buttons[1])
    SDL_BlitSurface(img_buttons[1],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[7],dest.x+15,dest.y,55,37,56);

  /*dest.x=320-10-text_width((char*)msg[0])-15;
  dest.y=227;
  if(img_buttons[5])
    SDL_BlitSurface(img_buttons[5],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[0],dest.x+15,dest.y,55,37,56);*/
}

void update_finish()
{
  clear_joystick_state();
  process_events();

  if(mainjoystick.button_start)
    done=TRUE;
  if(mainjoystick.button_b)
  {
    load_scripts();
    mode_app=MODE_BROWSE;
  }
  if(mainjoystick.pad_up && console_idx>0)
  {
    console_idx--;
    automated_list=FALSE;
  }
  if(mainjoystick.pad_down && console_idx<(consolelines-1))
  {
    console_idx++;
    automated_list=FALSE;
  }
  if(mainjoystick.button_l1)
  {
    console_idx-=10;
    if(console_idx<0)
      console_idx=0;
    automated_list=FALSE;
  }
  if(mainjoystick.button_r1)
  {
    console_idx+=10;
    if(console_idx>=consolelines)
      console_idx=consolelines-1;
    automated_list=FALSE;
  }
}

void draw_finish()
{
  draw_executing();

  // header
  SDL_Rect dest;
  dest.x=320-10-16;
  dest.y=2;
  if(img_working[3])
    SDL_BlitSurface(img_working[3],NULL,screen,&dest);

  // foot
  dest.x=10;
  dest.y=227;
  if(img_buttons[7])
    SDL_BlitSurface(img_buttons[7],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[4],25,dest.y,55,37,56);

  dest.x=320-10-text_width((char*)msg[0])-15;
  dest.y=227;
  if(img_buttons[5])
    SDL_BlitSurface(img_buttons[5],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[0],dest.x+15,dest.y,55,37,56);
}

void update_confirm()
{
  clear_joystick_state();
  process_events();

  if(mainjoystick.button_start)
    done=TRUE;
  if(mainjoystick.button_b)
    mode_app=MODE_BROWSE;
  if(mainjoystick.button_a)
  {
    if(script_list.size()>0)
    {
      pthread_create(&sr_th, NULL, runscript_thd, NULL);
      automated_list=TRUE;
      //get_stdoutfromcommand("scripts/"+script_list[script_list_selected].filename);
      mode_app=MODE_EXECUTING;
    }
  }

}

void draw_confirm()
{
  draw_browse();
  SDL_Color col={55,56,128};
  SDL_Color bor={255,255,255};

  int w=5+10+5+text_width((char*)msg[2])+10+10+5+text_width((char*)msg[3])+5;
  draw_rectangle((320-w)/2,100,w,37,&col,BORDER_ROUNDED,&bor);
  draw_text(screen,font,(char*)msg[5],(320-text_width((char*)msg[5]))/2,105,255,255,255);

  SDL_Rect dest;
  dest.x=160-15-text_width((char*)msg[2])-5;
  dest.y=120;
  if(img_buttons[6])
    SDL_BlitSurface(img_buttons[6],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[2],160-5-text_width((char*)msg[2]),dest.y,255,255,0);
  dest.x=160+5;
  if(img_buttons[7])
    SDL_BlitSurface(img_buttons[7],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[3],180,dest.y,255,255,0);
}

void update_viewcode()
{
  clear_joystick_state();
  process_events();

  if(mainjoystick.button_start)
    done=TRUE;
  if(mainjoystick.button_b)
  {
    mode_app=MODE_BROWSE;
  }
  if(mainjoystick.pad_up && console_idx>0)
  {
    console_idx--;
  }
  if(mainjoystick.pad_down && console_idx<(consolelines-1))
  {
    console_idx++;
  }
  if(mainjoystick.button_l1)
  {
    console_idx-=10;
    if(console_idx<0)
      console_idx=0;
  }
  if(mainjoystick.button_r1)
  {
    console_idx+=10;
    if(console_idx>=consolelines)
      console_idx=consolelines-1;
  }
}

void draw_viewcode()
{
  draw_executing();

  // header
  std::string str="["+script_list[script_list_selected].filename+"]";
  draw_text(screen,font,(char*)str.c_str(),10+10+text_width((char*)"Scriptrunner"),3,55,37,56);

  // foot
  SDL_Rect dest;
  dest.x=10;
  dest.y=227;
  if(img_buttons[7])
    SDL_BlitSurface(img_buttons[7],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[4],25,dest.y,55,37,56);

  dest.x=320-10-text_width((char*)msg[0])-15;
  dest.y=227;
  if(img_buttons[5])
    SDL_BlitSurface(img_buttons[5],NULL,screen,&dest);
  draw_text(screen,font,(char*)msg[0],dest.x+15,dest.y,55,37,56);
}

///////////////////////////////////
/*  Init                         */
///////////////////////////////////
int main(int argc, char *argv[])
{
  if(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO | SDL_INIT_AUDIO)<0)
		return 0;

  screen = SDL_SetVideoMode(320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
  if (screen==NULL)
    return 0;

  init_game();
  load_scripts();

  const int GAME_FPS=15;
  Uint32 start_time;

  while(!done)
	{
    start_time=SDL_GetTicks();

    switch(mode_app)
    {
      case MODE_BROWSE:
        draw_browse();
        update_browse();
        break;
      case MODE_EXECUTING:
        draw_executing();
        update_executing();
        break;
      case MODE_FINISH:
        draw_finish();
        update_finish();
        break;
      case MODE_CONFIRM:
        draw_confirm();
        update_confirm();
        break;
      case MODE_VIEWCODE:
        draw_viewcode();
        update_viewcode();
        break;
    }

    SDL_Flip(screen);

    // set FPS 60
    if(1000/GAME_FPS>SDL_GetTicks()-start_time)
      SDL_Delay(1000/GAME_FPS-(SDL_GetTicks()-start_time));
	}

	pthread_mutex_destroy(&lock_consoleoutput);
	pthread_cancel(sr_th);
	pthread_join(sr_th, NULL);
  end_game();

  return 1;
}
