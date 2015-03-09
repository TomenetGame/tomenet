/* File: main-amy.c */

#include "angband.h"

#ifdef USE_AMY

#include <dos/dos.h>
#include <dos/var.h>
#include <intuition/intuition.h>

/* evileye NEW */
#include <exec/memory.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfxbase.h>
#include <graphics/modeid.h>
#include <graphics/scale.h>
#include <graphics/text.h>

#include <utility/tagitem.h>

/*extranew*/
#include <libraries/asl.h>
/**/
#include <hardware/blit.h>
/*EON*/

#define CUR_A ( td->t.scr->a[ td->cy ][ td->cx ] ) /* from main-ami */
#define CUR_C ( td->t.scr->c[ td->cy ][ td->cx ] )

/*NEW Evil*/

/* Pen number convertion */
#define PEN( p ) ( penconv[ p ] )

/* Message */
#define MSG( x, y, txt ) Term_text_amy( x, y, strlen( txt ), 1, txt );
//#define MSG( x, y, txt ) printf("%s\n",txt);

/* Size of tile image */
#define GFXW 256
#define GFXH 256
#define GFXB 32

/* Filename of tile image */
#define MGFX "Tomenet:xtra/gfx/tiles.raw"

/* Filename of tombstone image */
#define MTOM "Tomenet:xtra/gfx/tomb.raw"

/* KickStart 3.0+ present */
static int v39 = FALSE;

/* Use a public screen */
static int use_pub = FALSE;

/* Convert textual pens to screen pens */
static UWORD penconv[ 16 ] =
{
   0,1,2,4,11,15,9,6,3,1,13,4,11,15,8,5
};

/* Public screen obtained pens */
static LONG pubpens[ 32 ] =
{
  -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1
};
/*EON*/

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Library *SocketBase;

struct Device *TimerBase;

struct Screen *mangscr=NULL;

struct timerequest IOtr;
struct MsgPort *TimerSink=NULL;

struct NewWindow newwindow={
  0,12,
  640,208,
  0,1,
  RAWKEY | VANILLAKEY | MENUPICK,
  ACTIVATE | WINDOWDRAG | WINDOWDEPTH | GIMMEZEROZERO /*| BORDERLESS*/,
  NULL,
  NULL,
  NULL, //Title
  NULL,
  NULL,
  640,208,
  640,208,
  CUSTOMSCREEN
};

void quit_amy(cptr s);

/*
 * Extra data to associate with each "window"
 *
 * Each "window" is represented by a "term_data" structure, which
 * contains a "term" structure, which contains a pointer (t->data)
 * back to the term_data structure.
 */

typedef struct term_data term_data;

struct term_data
{
        term            t;

        cptr            name;

        /* Other fields if needed */

        struct          Window *wptr;

/*NEW evileye */
   BYTE use;                /* Use this window */

   UWORD ww;
   UWORD wh;

   BYTE fw;                 /* Font width */
   BYTE fh;                 /* Font height */
   BYTE fb;                 /* Font baseline */

   struct BitMap *gfxbm;
   struct BitMap *mapbm;
   struct BitMap *mskbm;

   int gfx_w;
   int gfx_h;

   int map_w;
   int map_h;

   int mpt_w;
   int mpt_h;

/*EON*/
        short           cv;
        short           cx;
        short           cy;
};



/*
 * One "term_data" for each "window"
 *
 * XXX XXX XXX The only "window" which you MUST support is the
 * main "term_screen" window, the rest are optional.  If you only
 * support a single window, then most of the junk involving the
 * "term_data" structures is actually not needed, since you can
 * use global variables.  But you should avoid global variables
 * when possible as a general rule...
 */
static term_data screen;
static term_data mirror;
static term_data recall;
static term_data choice;

/* Change these if they aren't to your liking... */

/*
USHORT ColourTab[]={
  0x000,                                //Dark
  0xfff,                                //White
  0xbbb,                                //Slate
  0xf50,                                //Orange
  0xa00,                                //Red
  0x0a4,                                //Green
  0x05f,                                //Blue
  0x850,                                //Umber

  0x888,                                //L-Dark
  0xddd,                                //D-White
  0xf0f,                                //Violet
  0xff0,                                //Yellow
  0xf00,                                //L-Red
  0x0f0,                                //L-Green
  0x5af,                                //L-Blue
  0xe75,                                //L-Umber
};
*/

USHORT ColourTab[]={
  0x000,                                //Dark
  0xfed,                                //White
  0x888,                                //Slate
  0x555,                                //Orange
  0xeb0,                                //Red
  0xca7,                                //Green
  0x864,                                //Blue
  0x432,                                //Umber

  0x0af,                                //L-Dark
  0x00f,                                //D-White
  0x007,                                //Violet
  0xf00,                                //Yellow
  0x800,                                //L-Red
  0x90b,                                //L-Green
  0x061,                                //L-Blue
  0x6f4,                                //L-Umber
};

/*** Special functions ***/



/*** Function hooks needed by "Term" ***/


/*
 * XXX XXX XXX Init a new "term"
 *
 * This function should do whatever is necessary to prepare a new "term"
 * for use by the "term.c" package.  This may include clearing the window,
 * preparing the cursor, setting the font/colors, etc.  Usually, this
 * function does nothing, and the "init_xxx()" function does it all.
 */
static void Term_init_amy(term *t)
{
        term_data *td = (term_data*)(t->data);
        newwindow.Screen=mangscr;
        td->wptr=NULL;

        td->wptr=OpenWindowTags(&newwindow,
                WA_Top,( td==&screen ? 12 : 230 ),
                WA_Title, ( td==&screen ? "Tomenet V4.0.0" : td->name ),
                TAG_DONE);

        td->fw=GfxBase->DefaultFont->tf_XSize;
        td->fh=GfxBase->DefaultFont->tf_YSize;
        td->fb=GfxBase->DefaultFont->tf_Baseline;

        /* XXX XXX XXX */
}



/*
 * XXX XXX XXX Nuke an old "term"
 *
 * This function is called when an old "term" is no longer needed.  It should
 * do whatever is needed to clean up before the program exits, such as wiping
 * the screen, restoring the cursor, fixing the font, etc.  Often this function
 * does nothing and lets the operating system clean up when the program quits.
 */
static void Term_nuke_amy(term *t)
{
        term_data *td = (term_data*)(t->data);
        if(td->wptr){
          CloseWindow(td->wptr);
          td->wptr=NULL;
        }

/* BAD HACK.... :/  Closing down from here... */
        if(mangscr){
          if(!screen.wptr && !mirror.wptr){
            free_bitmap(screen.gfxbm);
//          free_bitmap(screen.mapbm); /* only used in sizing */
            free_bitmap(screen.mskbm);

            CloseScreen(mangscr);
            if(IOtr.tr_node.io_Device){
              CloseDevice(&IOtr);
            }
            if(SocketBase){
              CloseLibrary(SocketBase);
            }
            if(GfxBase){
              CloseLibrary(GfxBase);
            }
            if(IntuitionBase){
              CloseLibrary(IntuitionBase);
            }
            mangscr=NULL;
          }
        }
        /* XXX XXX XXX */
}



/*
 * XXX XXX XXX Do a "user action" on the current "term"
 *
 * This function allows the visual module to do things.
 *
 * This function is currently unused, but has access to the "info"
 * field of the "term" to hold an extra argument.
 *
 * In general, this function should return zero if the action is successfully
 * handled, and non-zero if the action is unknown or incorrectly handled.
 */
static errr Term_user_amy(int n)
{
        term_data *td = (term_data*)(Term->data);

        /* XXX XXX XXX Handle the request */

        /* Unknown */
        return (1);
}


/*
 * XXX XXX XXX Do a "special thing" to the current "term"
 *
 * This function must react to a large number of possible arguments, each
 * corresponding to a different "action request" by the "term.c" package.
 *
 * The "action type" is specified by the first argument, which must be a
 * constant of the form "TERM_XTRA_*" as given in "term.h", and the second
 * argument specifies the "information" for that argument, if any, and will
 * vary according to the first argument.
 *
 * In general, this function should return zero if the action is successfully
 * handled, and non-zero if the action is unknown or incorrectly handled.
 */
static errr Term_xtra_amy(int n, int v)
{
        term_data *td = (term_data*)(Term->data);

        struct IntuiMessage *wmsg;

        if(td->wptr==NULL)
          return(1);

        /* Analyze */
        switch (n)
        {
                case TERM_XTRA_EVENT:

                /* XXX XXX XXX Process some pending events */
                /* Wait for at least one event if "v" is non-zero */
                /* otherwise, if no events are ready, return at once. */
                /* When "keypress" events are encountered, the "ascii" */
                /* value corresponding to the key should be sent to the */
                /* "Term_keypress()" function.  Certain "bizarre" keys, */
                /* such as function keys or arrow keys, may send special */
                /* sequences of characters, such as control-underscore, */
                /* plus letters corresponding to modifier keys, plus an */
                /* underscore, plus carriage return, which can be used by */
                /* the main program for "macro" triggers.  This action */
                /* should handle as many events as is efficiently possible */
                /* but is only required to handle a single event, and then */
                /* only if one is ready or "v" is true */
                /* This action is required. */

                if(v){
                  long sig;
                  sig=Wait(1 << td->wptr->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);
                  if(sig&SIGBREAKF_CTRL_C){
                    quit("User Break: quitting\n");
                  }
                }

                if((wmsg=GetMsg(td->wptr->UserPort))){
                  unsigned long class=wmsg->Class;
                  unsigned long code=wmsg->Code;

                  ReplyMsg(wmsg);

                  switch(class){
                    case VANILLAKEY:
                      Term_keypress(code);
                      break;
                    case RAWKEY:
                      if(code>=80 && code<=89){
/*fixme somewhere safe!*/
                        Term_keypress(code+48);
//                      printf("Function key: F%d\n",code-79);
                      }
                      break;
                    case MENUPICK:
//                    printf("Menupick registered: %ld\n",code);
                      break;
                    default:
                  }
                  return(0);
                }

                return (1);

                case TERM_XTRA_FLUSH:

                /* XXX XXX XXX Flush all pending events */
                /* This action should handle all events waiting on the */
                /* queue, optionally discarding all "keypress" events, */
                /* since they will be discarded anyway in "term.c". */
                /* This action is required, but is often not "essential". */

                while((wmsg=GetMsg(td->wptr->UserPort))){
                  unsigned long class=wmsg->Class;
                  unsigned long code=wmsg->Code;

                  ReplyMsg(wmsg);
                }

                return (0);

                case TERM_XTRA_CLEAR:

                /* XXX XXX XXX Clear the entire window */
                /* This action should clear the entire window, and redraw */
                /* any "borders" or other "graphic" aspects of the window. */
                /* This action is required. */

                SetAPen(td->wptr->RPort,PEN(0));        /*Evil - latest*/
                RectFill(td->wptr->RPort,0,0,640,200);
                return (0);

                case TERM_XTRA_SHAPE:

                /* XXX XXX XXX Set the cursor visibility (optional) */
                /* This action should change the visibility of the cursor, */
                /* if possible, to the requested value (0=off, 1=on) */
                /* This action is optional, but can improve both the */
                /* efficiency (and attractiveness) of the program. */

                if(v){
                  td->cv=TRUE;
                }
                else{
                  td->cv=FALSE;
                }
                return (0);

                case TERM_XTRA_FROSH:

                /* XXX XXX XXX Flush a row of output (optional) */
                /* This action should make sure that row "v" of the "output" */
                /* to the window will actually appear on the window. */
                /* This action is optional on most systems. */

                return (0);

                case TERM_XTRA_FRESH:

                /* XXX XXX XXX Flush output (optional) */
                /* This action should make sure that all "output" to the */
                /* window will actually appear on the window. */
                /* This action is optional if all "output" will eventually */
                /* show up on its own, or when actually requested. */

                return (0);

                case TERM_XTRA_NOISE:

                /* XXX XXX XXX Make a noise (optional) */
                /* This action should produce a "beep" noise. */
                /* This action is optional, but nice. */

                DisplayBeep(mangscr);
                return (0);

                case TERM_XTRA_SOUND:

                printf("Sound received\n");

                /* XXX XXX XXX Make a sound (optional) */
                /* This action should produce sound number "v", where */
                /* the "name" of that sound is "sound_names[v]". */
                /* This action is optional, and not important. */

                return (0);

                case TERM_XTRA_BORED:

                /* XXX XXX XXX Handle random events when bored (optional) */
                /* This action is optional, and not important */

                return (0);

                case TERM_XTRA_REACT:

                /* XXX XXX XXX React to global changes (optional) */
                /* For example, this action can be used to react to */
                /* changes in the global "color_table[256][4]" array. */
                /* This action is optional, but can be very useful */

                return (0);

                case TERM_XTRA_ALIVE:

                /* XXX XXX XXX Change the "hard" level (optional) */
                /* This action is used if the program changes "aliveness" */
                /* by being either "suspended" (v=0) or "resumed" (v=1) */
                /* This action is optional, unless the computer uses the */
                /* same "physical screen" for multiple programs, in which */
                /* case this action should clean up to let other programs */
                /* use the screen, or resume from such a cleaned up state. */
                /* This action is currently only used on UNIX machines */

                return (0);

                case TERM_XTRA_LEVEL:

                /* XXX XXX XXX Change the "soft" level (optional) */
                /* This action is used when the term window changes "activation" */
                /* either by becoming "inactive" (v=0) or "active" (v=1) */
                /* This action is optional but can be used to do things like */
                /* activate the proper font / drawing mode for the newly active */
                /* term window.  This action should NOT change which window has */
                /* the "focus", which window is "raised", or anything like that. */
                /* This action is optional if all the other things which depend */
                /* on what term is active handle activation themself. */

                return (0);

                case TERM_XTRA_DELAY:

                /* XXX XXX XXX Delay for some milliseconds (optional) */
                /* This action is important for certain visual effects, and also */
                /* for the parsing of macro sequences on certain machines. */

                if(v>=20) Delay(v/20);
                return (0);
        }

        /* Unknown or Unhandled action */
        return (1);
}


/*
 * XXX XXX XXX Erase some characters
 *
 * This function should erase "n" characters starting at (x,y).
 *
 * You may assume "valid" input if the window is properly sized.
 */
static errr Term_wipe_amy(int x, int y, int n)
{
        term_data *td = (term_data*)(Term->data);

        /* XXX XXX XXX Erase the block of characters */

        if(td->wptr){
          SetAPen(td->wptr->RPort,PEN(0));      /*Evil - latest*/
          RectFill(td->wptr->RPort,x*8,8+y*8,(x+n)*8,15+y*8);
        }

        /* Success */
        return (0);
}


/*
 * XXX XXX XXX Display the cursor
 *
 * This routine should display the cursor at the given location
 * (x,y) in some manner.  On some machines this involves actually
 * moving the physical cursor, on others it involves drawing a fake
 * cursor in some form of graphics mode.  Note the "soft_cursor"
 * flag which tells "term.c" to treat the "cursor" as a "visual"
 * thing and not as a "hardware" cursor.
 *
 * You may assume "valid" input if the window is properly sized.
 *
 * You may use the "Term_grab(x, y, &a, &c)" function, if needed,
 * to determine what attr/char should be "under" the new cursor,
 * for "inverting" purposes or whatever.
 */
static errr Term_curs_amy(int x, int y)
{
        term_data *td = (term_data*)(Term->data);

        /* XXX XXX XXX Display a cursor (see above) */

        if(td->wptr){
          if(td->cv)
            curs_off(td);
          td->cx=x;
          td->cy=y;
          if(td->cv)
            curs_on(td);
        }
        /* Success */
        return (0);
}

static void curs_on(term_data *td){
  SetAPen(td->wptr,CUR_A&0x0f);
  SetBPen(td->wptr,4L);
  Move(td->wptr, td->cx*8, 8+td->cy*8);
  Text(td->wptr,&CUR_C,1);
}

static void curs_off(term_data *td){
  SetAPen(td->wptr,CUR_A&0x0f);
  SetBPen(td->wptr, 0L);
  Move(td->wptr, td->cx*8, 8+td->cy*8);
  Text(td->wptr,&CUR_C,1);
}

/*
 * XXX XXX XXX Draw a "picture" on the screen
 *
 * This routine should display the given attr/char pair at the
 * given location (x,y).  This function is only used if one of
 * the flags "always_pict" or "higher_pict" is defined.
 *
 * You must be sure that the attr/char pair, when displayed, will
 * erase anything (including any visual cursor) that used to be
 * at the given location.  On many machines this is automatic, on
 * others, you must first call "Term_wipe_xxx(x, y, 1)".
 *
 * With the "higher_pict" flag, this function can be used to allow
 * the display of "pseudo-graphic" pictures, for example, by using
 * "(a&0x7F)" as a "row" and "(c&0x7F)" as a "column" to index into
 * a special auxiliary pixmap of special pictures.
 *
 * With the "always_pict" flag, this function can be used to force
 * every attr/char pair to be drawn one at a time, instead of trying
 * to "collect" the attr/char pairs into "strips" with similar "attr"
 * codes, which would be sent to "Term_text_xxx()".
 */
static errr Term_pict_amy(int x, int y, byte a, char c)
{
        term_data *td = (term_data*)(Term->data);
        char s[2];

        /* XXX XXX XXX Draw a "picture" */

        if(!td->wptr){
          return(1);
        }

        if((a&0x80)){
          put_gfx( td->wptr->RPort, x , y, c & 0x7f, a & 0x7f );
        }
        else{
          s[0]=c;
          s[1]=0;
          SetAPen(td->wptr->RPort,PEN(a)); /*evil -latest*/
          Move(td->wptr->RPort,x*8,6+y*8);
          Text(td->wptr->RPort,s,1);
        }


        /* Success */
        return (0);
}


/*
 * XXX XXX XXX Display some text on the screen
 *
 * This function should actually display a string of characters
 * starting at the given location, using the given "attribute",
 * and using the given string of characters, which is terminated
 * with a nul character and which has exactly "n" characters.
 *
 * You may assume "valid" input if the window is properly sized.
 *
 * You must be sure that the string, when written, erases anything
 * (including any visual cursor) that used to be where the text is
 * drawn.  On many machines this happens automatically, on others,
 * you must first call "Term_wipe_xxx()" to clear the area.
 *
 * You may ignore the "color" parameter if you are only supporting
 * a monochrome environment, unless you have set the "draw_blanks"
 * flag, since this routine is normally never called to display
 * "black" (invisible) text, and all other colors should be drawn
 * in the "normal" color in a monochrome environment.
 *
 * Note that this function must correctly handle "black" text if
 * the "always_text" flag is set, if this flag is not set, all the
 * "black" text will be handled by the "Term_wipe_xxx()" hook.
 */
static errr Term_text_amy(int x, int y, int n, byte a, cptr s)
{
        term_data *td = (term_data*)(Term->data);
        int i;

        /* XXX XXX XXX Normally use color "color_data[a & 0x0F]" */

        /* XXX XXX XXX Draw the string */

        if(!td->wptr){
          return(1);
        }

/*evil - new*/
//        if((a&0xc0)){
//          for ( i = 0; i < n; i++ ) put_gfx( td->wptr->RPort, x + i, y, s[ i ] & 0x7f, a & 0x7f );
//      }
//        else{
        if((a&0xc0))
          a=1;
          SetAPen(td->wptr->RPort,PEN(a));      /*evil - latest*/
          Move(td->wptr->RPort,x*8,6+y*8);
          Text(td->wptr->RPort,s,n);
//      }

        /* Success */
        return (0);
}




/*
 * XXX XXX XXX Instantiate a "term_data" structure
 *
 * This is one way to prepare the "term_data" structures and to
 * "link" the various informational pieces together.
 *
 * This function assumes that every window should be 80x24 in size
 * (the standard size) and should be able to queue 256 characters.
 * Technically, only the "main screen window" needs to queue any
 * characters, but this method is simple.
 *
 * Note that "activation" calls the "Term_init_xxx()" hook for
 * the "term" structure, if needed.
 */
static void term_data_link(term_data *td)
{
        term *t = &td->t;

        /* Initialize the term */
        term_init(t, 80, 24, 256);

        /* XXX XXX XXX Choose "soft" or "hard" cursor */
        /* A "soft" cursor must be explicitly "drawn" by the program */
        /* while a "hard" cursor has some "physical" existence and is */
        /* moved whenever text is drawn on the screen.  See "term.c". */
        /* t->soft_cursor = TRUE; */

        /* XXX XXX XXX Avoid the "corner" of the window */
        /* t->icky_corner = TRUE; */

        /* XXX XXX XXX Use "Term_pict()" for all data */
        /* See the "Term_pict_xxx()" function above. */
        /* t->always_pict = TRUE; */
        t->always_pict = FALSE;

        /* XXX XXX XXX Use "Term_pict()" for "special" data */
        /* See the "Term_pict_xxx()" function above. */
        /* t->higher_pict = TRUE; */
        t->higher_pict = TRUE;

        /* XXX XXX XXX Use "Term_text()" for all data */
        /* See the "Term_text_xxx()" function above. */
        /* t->always_text = TRUE; */
        t->always_text = FALSE;

        /* XXX XXX XXX Ignore the "TERM_XTRA_BORED" action */
        /* This may make things slightly more efficient. */
        /* t->never_bored = TRUE; */

        /* XXX XXX XXX Ignore the "TERM_XTRA_FROSH" action */
        /* This may make things slightly more efficient. */
        /* t->never_frosh = TRUE; */

        /* Erase with "white space" */
        t->attr_blank = TERM_WHITE;
        t->char_blank = ' ';

        /* Prepare the init/nuke hooks */
        t->init_hook = Term_init_amy;
        t->nuke_hook = Term_nuke_amy;

        /* Prepare the template hooks */
        t->user_hook = Term_user_amy;
        t->xtra_hook = Term_xtra_amy;
        t->wipe_hook = Term_wipe_amy;
        t->curs_hook = Term_curs_amy;
        t->pict_hook = Term_pict_amy;
        t->text_hook = Term_text_amy;

        /* Remember where we came from */
        t->data = (vptr)(td);

        /* Activate it */
        Term_activate(t);
}

/*
 * A "normal" system uses "main.c" for the "main()" function, and
 * simply adds a call to "init_xxx()" to that function, conditional
 * on some form of "USE_XXX" define.
 */

char modestr[256]="";

/*
 * XXX XXX XXX Initialization function
 */
int init_amy(void)
{
        term_data *td;

/*NEW Evileye*/
   /* Term data pointers */
   term_data *ts = &screen;
   term_data *tc = &choice;
   term_data *tr = &recall;
   term_data *tm = &mirror;
//extra new
   char *s;
   unsigned long scr_m;
/*EON*/
        /* XXX XXX XXX Initialize the system */

        ts->use=TRUE;
        tm->use=TRUE;

        quit_aux=&quit_amy;

        IntuitionBase=OpenLibrary("intuition.library",0L);
        GfxBase=OpenLibrary("graphics.library",0L);
        SocketBase=OpenLibrary("bsdsocket.library",0L);

/*Evileye NEW*/
        v39 = ( IntuitionBase->LibNode.lib_Version >= 39 );
/*EON*/

        if(!SocketBase){
          quit("It helps to be running a TCP/IP stack\n");
        }

        IOtr.tr_node.io_Device = NULL;

        if (OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)&IOtr, 0))  {
          quit("Can't open timer device.\n");
        }

        TimerBase=IOtr.tr_node.io_Device;

        IOtr.tr_node.io_Message.mn_ReplyPort = TimerSink;
        IOtr.tr_node.io_Command = TR_ADDREQUEST;
        IOtr.tr_node.io_Error = 0;

/*evileye*/
        if((GetVar("tomenet_scr",modestr,256,0L))==-1){
          request_mode(modestr);
          SetVar("tomenet_scr",modestr,strlen(modestr),GVF_GLOBAL_ONLY);
        }
        scr_m = strtol( modestr, &s, 0 );
        if ( scr_m == 0 || scr_m == INVALID_ID ){
          scr_m = ( DEFAULT_MONITOR_ID | HIRES_KEY );
        }
/**/

        mangscr = OpenScreenTags( NULL,
             SA_Width, 640,
             SA_Height, 268*2,
             SA_Depth, 4,
             SA_DisplayID, scr_m,
//           SA_Font, scrattr,
             SA_Type, CUSTOMSCREEN,
             SA_Title, "TomeNET Screen",
             SA_ShowTitle, FALSE,
             SA_Quiet, FALSE,
             SA_Behind, FALSE,
             SA_AutoScroll, TRUE,
             SA_Overscan, OSCAN_TEXT,
             v39 ? SA_Interleaved : TAG_IGNORE, TRUE,
             TAG_END);

        LoadRGB4(&mangscr->ViewPort,&ColourTab[0],16);

        /* Recall window */
        td = &recall;
        WIPE(td, term_data);
        td->name = "Recall";
        /* XXX XXX XXX Extra stuff */
        term_data_link(&recall);
        term_recall = &recall.t;

        /* Choice window */
        td = &choice;
        WIPE(td, term_data);
        td->name = "Choice";
        /* XXX XXX XXX Extra stuff */
        term_data_link(&choice);
        term_choice = &choice.t;

        /* Mirror window */
        td = &mirror;
        WIPE(td, term_data);
        td->name = "Mirror";
        /* XXX XXX XXX Extra stuff */
        term_data_link(&mirror);
        term_mirror = &mirror.t;

        /* Screen window */
        td = &screen;
        WIPE(td, term_data);
        td->name = "Screen";
        /* XXX XXX XXX Extra stuff */
        term_data_link(&screen);
        term_screen = &screen.t;

/*Evileye NEW*/

        if((GetVar("tomenet_gfx",modestr,256,0L))==-1){
          char ch;
          Term_activate(&screen.t);
          MSG(0,0,"Do you want graphics (y/n):");
          Term_inkey(&ch,TRUE,TRUE);
          Term_wipe_amy(0,0,26);
          if(ch=='y'||ch=='Y')
            strcpy(modestr,"true");
          else
            strcpy(modestr,"false");
          SetVar("tomenet_gfx",modestr,strlen(modestr),GVF_GLOBAL_ONLY);
        }
        if(!(stricmp(modestr,"true")))
          use_graphics=TRUE;

        /* Load and convert graphics */
        if ( use_graphics )
        {
           MSG( 0, 0, "Loading graphics" );
           if ( !load_gfx() ) quit( NULL );

           /* Check if conversion is necessary */
           if ( use_pub || ( depth_of_bitmap( ts->wptr->RPort->BitMap ) != depth_of_bitmap( ts->gfxbm )))
           {
             MSG( 0, 1, "Remapping graphics" );
             if ( !conv_gfx() ) quit( "Out of memory while remapping graphics.\n" );
           }

           /* Scale the graphics to fit font sizes */
/*temp out Evileye *//*
           if ( tm->use ) if ( !size_gfx( tm ) ) quit( "Out of memory while scaling graphics.\n" );
           if ( tr->use ) if ( !size_gfx( tr ) ) quit( "Out of memory while scaling graphics.\n" );
           if ( tc->use ) if ( !size_gfx( tc ) ) quit( "Out of memory while scaling graphics.\n" );
           if ( ts->use ) if ( !size_gfx( ts ) ) quit( "Out of memory while scaling graphics.\n" );
*/
        }
/*EON*/
        return(0);
}

void quit_amy(cptr s){
        int j;

        Net_cleanup();

        /* Nuke each term */
        for (j = ANGBAND_TERM_MAX - 1; j >= 0; j--)
        {
                /* Unused */
                if (!ang_term[j]) continue;

                /* Nuke it */
                term_nuke(ang_term[j]);
        }

        if(mangscr){
          CloseScreen(mangscr);
          mangscr=NULL;
        }

        if(GfxBase)
          CloseLibrary(GfxBase);
        if(IntuitionBase)
          CloseLibrary(IntuitionBase);
}


/*Evileye NEW*/

///}
///{ "load_gfx()"

int load_gfx( void )
{
   term_data *ts = &screen;
   BPTR file;
   char *p;

   int plane, row, error = FALSE;

   /* Allocate bitmaps */
   if (( ts->gfxbm = alloc_bitmap( GFXW, GFXH, 4, BMF_CLEAR, ts->wptr->RPort->BitMap )) == NULL ) return( FALSE );
   if (( ts->mskbm = alloc_bitmap( GFXW, GFXH, 1, BMF_CLEAR, ts->wptr->RPort->BitMap )) == NULL ) return( FALSE );

   /* Open file */
   if (( file = Open( MGFX, MODE_OLDFILE )) == NULL )
   {
      MSG( 0, 0, "Unable to open graphics file" ); Delay( 100 );
      return ( FALSE );
   }

   /* Read file into bitmap */
   for ( plane = 0; plane < 4 && !error; plane++ )
   {
      p = ts->gfxbm->Planes[ plane ];
      for ( row = 0; row < GFXH && !error; row++ )
      {
         error = ( Read( file, p, GFXB ) != GFXB );
         p += ts->gfxbm->BytesPerRow;
      }
   }

   /* Read mask data into bitmap */
   p = ts->mskbm->Planes[ 0 ];
   for ( row = 0; row < GFXH && !error; row++ )
   {
      error = ( Read( file, p, GFXB ) != GFXB );
      p += ts->mskbm->BytesPerRow;
   }

   /* Close file */
   Close( file );

   /* Did we get any errors while reading? */
   if ( error )
   {
      MSG( 0, 0, "Error while reading graphics file" ); Delay( 100 );
      return ( FALSE );
   }

   /* Success */
   return ( TRUE );
}
///}
///{ "conv_gfx()"

int conv_gfx( void )
{
   term_data *ts = &screen;
   struct BitMap *tmpbm;
   struct BitMap *sbm = ts->wptr->RPort->BitMap;
   long stdpens[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
   long mskpens[] = { 0, -1 };
   int depth;
   BYTE *src, *dst;
   int plane, row, byt;

   /* Get depth of display */
   depth = depth_of_bitmap( sbm );

   /* We don't want 16 or 24 bit displays (yet) */
   if ( depth > 8 )
   {
      MSG( 0, 1, "Sorry, max. 8 bit display supported." ); Delay( 100 );
      return ( FALSE );
   }

   /* Allocate new bitmap with screen's depth */
   if (( tmpbm = alloc_bitmap( GFXW, GFXH, depth, BMF_CLEAR, sbm )) == NULL )
   {
      MSG( 0, 1, "Unable to allocate temporary bitmap." ); Delay( 100 );
      return ( FALSE );
   }

   /* Simple remapping from 4 to 5 planes? */
   if ( depth == 5 && !use_pub )
   {
      for ( plane = 0; plane < 4; plane++ )
      {
         src = ts->gfxbm->Planes[ plane ];
         dst = tmpbm->Planes[ plane ];
         for ( row = 0; row < GFXH; row++ )
         {
            for ( byt = 0; byt < GFXB; byt++ )
            {
               dst[ byt ] = src[ byt ];
            }
            src += ts->gfxbm->BytesPerRow;
            dst += tmpbm->BytesPerRow;
         }
      }
   }

   /* Complex remapping */
   else
   {
      /* Remap old bitmat into new bitmap */
      remap_bitmap( ts->gfxbm, tmpbm, use_pub ? pubpens : stdpens, GFXW, GFXH );
   }

   /* Free old bitmap */
   free_bitmap( ts->gfxbm );
   ts->gfxbm = tmpbm;

   /* Allocate new bitmap with screen's depth */
   if (( tmpbm = alloc_bitmap( GFXW, GFXH, depth, BMF_CLEAR, sbm )) == NULL )
   {
      MSG( 0, 1, "Unable to allocate temporary bitmap." ); Delay( 100 );
      return ( FALSE );
   }

   /* Simple remapping from 4 to 5 planes? */
   if ( depth == 5 && !use_pub )
   {
      for ( plane = 0; plane < 4; plane++ )
      {
         src = ts->mskbm->Planes[ 0 ];
         dst = tmpbm->Planes[ plane ];
         for ( row = 0; row < GFXH; row++ )
         {
            for ( byt = 0; byt < GFXB; byt++ )
            {
               dst[ byt ] = src[ byt ];
            }
            src += ts->mskbm->BytesPerRow;
            dst += tmpbm->BytesPerRow;
         }
      }
   }

   /* Complex remapping */
   else
   {
      /* Remap old bitmap into new bitmap */
      remap_bitmap( ts->mskbm, tmpbm, mskpens, GFXW, GFXH );
   }

   /* Free old bitmap */
   free_bitmap( ts->mskbm );
   ts->mskbm = tmpbm;

   /* Done */
   return ( TRUE );
}

///}
///{ "size_gfx()"

int size_gfx( term_data *td )
{
   term_data *ts = &screen;
   int depth;
   struct BitMap *sbm = td->wptr->RPort->BitMap;
   struct BitMap *tmpbm;

   /* Calculate tile bitmap dimensions */
   td->gfx_w = 32 * td->fw;
   td->gfx_h = 32 * td->fh;

   /* Calculate map bitmap dimensions */
   td->mpt_w = td->ww / MAX_WID;
   td->mpt_h = td->wh / MAX_HGT;
   td->map_w = td->mpt_w * 32;
   td->map_h = td->mpt_h * 32;

   /* Scale tile graphics into map size */
   depth = depth_of_bitmap( ts->gfxbm );
   if (( td->mapbm = alloc_bitmap( td->map_w, td->map_h, depth, BMF_CLEAR, sbm )) == NULL ) return( FALSE );
   scale_bitmap( ts->gfxbm, GFXW, GFXH, td->mapbm, td->map_w, td->map_h );

   /* Scale tile graphics */
   depth = depth_of_bitmap( ts->gfxbm );
   if (( tmpbm = alloc_bitmap( td->gfx_w, td->gfx_h, depth, BMF_CLEAR, sbm )) == NULL ) return( FALSE );
   scale_bitmap( ts->gfxbm, GFXW, GFXH, tmpbm, td->gfx_w, td->gfx_h );
   if ( td->gfxbm ) free_bitmap( td->gfxbm );
   td->gfxbm = tmpbm;

   /* Scale tile mask */
   depth = depth_of_bitmap( ts->mskbm );
   if (( tmpbm = alloc_bitmap( td->gfx_w, td->gfx_h, depth, BMF_CLEAR, sbm )) == NULL ) return( FALSE );
   scale_bitmap( ts->mskbm, GFXW, GFXH, tmpbm, td->gfx_w, td->gfx_h );
   if ( td->mskbm ) free_bitmap( td->mskbm );
   td->mskbm = tmpbm;

   /* Success */
   return( TRUE );
}

///}
///{ "put_gfx()"

static void put_gfx( struct RastPort *rp, int x, int y, int chr, int col )
{
   term_data *td = (term_data*)(Term->data);
   int fw = td->fw;
   int fh = td->fh;
   int x0 = x * fw;
   int y0 = y * fh;
   int x1 = x0 + fw - 1;
   int y1 = y0 + fh - 1;
   int a  = col & 0x1f;
   int c  = chr & 0x1f;

   /* Just a black tile */
   if ( a == 0 && c == 0 )
   {
      SetAPen( rp, PEN( 0 ));
      RectFill( rp, x0, y0, x1, y1 );
      return;
   }

   /* Player - Remap for race and class */
   if ( a == 12 && c == 0 )
   {
      a = (( p_ptr->pclass * 10 + p_ptr->prace) >> 5 ) + 12;
      c = (( p_ptr->pclass * 10 + p_ptr->prace) & 0x1f );
   }

   /* Draw tile through mask */
   if ( col & 0x40 )
   {
      SetAPen( rp, PEN(0) );
      RectFill( rp, x0, y0, x1, y1 );
      BltMaskBitMapRastPort( td->gfxbm, c * fw, a * fh, rp, x0, y0, fw, fh, (ABC|ANBC|ABNC), td->mskbm->Planes[ 0 ] );
   }

   /* Draw full tile */
   else
   {
      BltBitMapRastPort( td->gfxbm, c * fw, a * fh, rp, x0, y0, fw, fh, 0xc0 );
   }
}

///}
///{ "alloc_bitmap()"

struct BitMap *alloc_bitmap( int width, int height, int depth, ULONG flags, struct BitMap *friend )
{
   int p;
   struct BitMap *bitmap;
   unsigned char *bp;

   /* Kickstart 39+ */
   if( v39 ) return ( AllocBitMap( width, height, depth, flags, friend ));

   /* Kickstart 38- */
   else
   {
      /* Allocate bitmap structure */
      if (( bitmap = AllocMem( sizeof( struct BitMap ), MEMF_PUBLIC | MEMF_CLEAR )))
      {
         InitBitMap( bitmap, depth, width, height );
         /* Allocate bitplanes */
         for ( p = 0; p < depth; p++ )
         {
            bp = AllocRaster( width, height );
            if ( !bp ) break;
            bitmap->Planes[ p ] = bp;
         }

         /* Out of memory */
         if ( p != depth )
         {
            /* Free bitplanes */
            while ( --p >= 0 )
            {
               FreeRaster( bitmap->Planes[ p ], width, height );
            }
            /* Free bitmap structure */
            FreeMem( bitmap, sizeof( struct BitMap ));
            bitmap = NULL;
         }
      }
      return ( bitmap );
   }
}

///}
///{ "free_bitmap()"

void free_bitmap( struct BitMap *bitmap )
{
   int p;

   /* Check for NULL */
   if ( !bitmap ) return;

   /* Kickstart 39+ */
   if ( v39 ) FreeBitMap( bitmap );

   /* Kickstart 38- */
   else
   {
      /* Free bitplanes */
      for ( p = 0; p < bitmap->Depth; p++ )
      {
         FreeRaster( bitmap->Planes[ p ], bitmap->BytesPerRow * 8, bitmap->Rows );
      }
      /* Free bitmap structure */
      FreeMem( bitmap, sizeof( struct BitMap ));
   }
}

///}
///{ scale_bitmap()

void scale_bitmap( struct BitMap *srcbm, int srcw, int srch, struct BitMap *dstbm, int dstw, int dsth )
{
   struct BitScaleArgs bsa;

   /* Scale bitmap */
   bsa.bsa_SrcBitMap   = srcbm;
   bsa.bsa_DestBitMap  = dstbm;
   bsa.bsa_SrcX        = 0;
   bsa.bsa_SrcY        = 0;
   bsa.bsa_SrcWidth    = srcw;
   bsa.bsa_SrcHeight   = srch;
   bsa.bsa_DestX       = 0;
   bsa.bsa_DestY       = 0;
   bsa.bsa_XSrcFactor  = srcw;
   bsa.bsa_YSrcFactor  = srch;
   bsa.bsa_XDestFactor = dstw;
   bsa.bsa_YDestFactor = dsth;
   bsa.bsa_Flags       = 0;
   BitMapScale( &bsa );
}

///}
///{ "remap_bitmap()"

void remap_bitmap( struct BitMap *srcbm, struct BitMap *dstbm, long *pens, int width, int height )
{
   int x,y,p,c,ox,lpr,sd,dd;
   int bm[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
   UBYTE *sp[ 4 ];
   UBYTE *dp[ 8 ];
   ULONG ls[ 4 ];
   ULONG ld[ 8 ];
   ULONG mask;

   /* Source bitplanes */
   sd = depth_of_bitmap( srcbm );
   for ( p = 0; p < sd; p++ )
   {
      sp[ p ] = srcbm->Planes[ p ];
   }

   /* Destination bitplanes */
   dd = depth_of_bitmap( dstbm );
   for ( p = 0; p < dd; p++ )
   {
      dp[ p ] = dstbm->Planes[ p ];
   }

   /* Number of longwords per row */
   lpr = width / 32;

   /* Convert graphics */
   for( y = 0; y < height; y++ )
   {
      ox = 0;
      for ( x = 0 ; x < lpr; x++ )
      {
         /* Read source longwords */
         for ( p = 0; p < sd; p++ ) ls[ p ] = *(ULONG *)( sp[ p ] + ox);

         /* Clear destination longwords */
         for ( p = 0; p < dd; ld[ p++ ] = 0 );

         /* Remap */
         for ( mask = 0x80000000; mask != 0; mask >>= 1)
         {
            /* Find color index */
            for ( p = c = 0; p < sd; p++ ) if ( ls[ p ] & mask ) c |= bm[ p ];

            /* Remap */
            c = pens[ c ];

            /* Update destination longwords */
            for ( p = 0; p < dd; p++ ) if ( c & bm[ p ] ) ld[ p ] |= mask;
         }

         /* Write destination longwords */
         for ( p = 0; p < dd; p++ ) *(ULONG *)( dp[ p ] + ox ) = ld[ p ];

         /* Update offset */
         ox += 4;
      }

      /* Update pointers to get to next line */
      for ( p = 0; p < sd; sp[ p++ ] += srcbm->BytesPerRow );
      for ( p = 0; p < dd; dp[ p++ ] += dstbm->BytesPerRow );
   }
}

///}
///{ "depth_of_bitmap()"

int depth_of_bitmap( struct BitMap *bm )
{
   int depth;

   if ( v39 )
   {
      depth = GetBitMapAttr( bm, BMA_DEPTH );
   }
   else
   {
      depth = bm->Depth;
   }

   return ( depth );
}

///}

/*EON*/

///}
///{ "request_mode()"

/* added for screenmode - evil */

void request_mode( char *str )
{
   struct Library *AslBase = NULL;
   struct ScreenModeRequester *req = NULL;

   /* Blank string as default */
   *str = 0;

   /* Open asl.library */
   if ( (AslBase = OpenLibrary ( "asl.library", 37 )) )
   {
      /* Allocate screenmode requester */
      if ( (req = AllocAslRequestTags( ASL_ScreenModeRequest, TAG_DONE )) )
      {
         /* Open screenmode requester */
         if( AslRequestTags( req, TAG_DONE ))
         {
            /* Store font name and size */
            sprintf( str, "0x%lX", req->sm_DisplayID );
         }
         /* Free requester */
         FreeAslRequest( req );
      }
      /* Close asl.library */
      CloseLibrary( AslBase );
   }
}

/* eoa */

#endif /* USE_AMY */
