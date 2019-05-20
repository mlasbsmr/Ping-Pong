/*  bounce2d 1.0	
 *	bounce a character (default is 'o') around the screen
 *	defined by some parameters
 *
 *	user input: 	s slow down x component, S: slow y component
 *		 	f speed up x component,  F: speed y component
 *			Q quit
 *
 *	blocks on read, but timer tick sends SIGALRM caught by ball_move
 *	build:   cc bounce2d.c set_ticker.c -lcurses -o bounce2d
 */

#include	<curses.h>
#include	<signal.h>
#include    <sys/time.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include<fcntl.h>
#include	"bounce.h"

struct ppball the_ball ;
struct board bd;
/**  the main loop  **/
int state=0;
void set_up();
void wrap_up();
int set_ticker(int);
int bounce_or_lose(struct ppball *,struct board*);
void draw_board(int);
void ball_move(int);
void async_input_handler(int);
void set_fd(void);
int main()
{
	int	c;

	set_up();
	while(!state){
		pause();
	}
	wrap_up();
}


void set_up()
/*
 *	init structure and other stuff
 */
{
	bd.x_pos=RIGHT_EDGE;
    bd.y_pos=TOP_ROW+(BOT_ROW-TOP_ROW)/2+bd.len/2;
	bd.symbol=BD_SYMBOL;
	bd.len=BD_LEN;
	

	//(10,10)
	the_ball.y_pos = Y_INIT;
	the_ball.x_pos = X_INIT;
	//8
	the_ball.y_ttg = the_ball.y_ttm = Y_TTM ;
	//5
	the_ball.x_ttg = the_ball.x_ttm = X_TTM ;

	the_ball.y_dir = 1  ;
	the_ball.x_dir = 1  ;
	//'o'
	the_ball.symbol = DFL_SYMBOL ;


	initscr();
	//disables echo mode for the current screen
	noecho();
	//#define crmode()		cbreak() -> close buffer
	crmode();

	signal( SIGINT , SIG_IGN );
	draw_board(0);
	//mvaddch(y,x,ch)
	mvaddch( the_ball.y_pos, the_ball.x_pos, the_ball.symbol  );
	refresh();
	set_fd();
	signal(SIGIO,async_input_handler);

	signal( SIGALRM, ball_move );
	set_ticker( 1000 / TICKS_PER_SEC );	/* send millisecs per tick */
}

void wrap_up()
{

	set_ticker( 0 );
	endwin();		/* put back to normal	*/
}
void set_fd(){
	int flags;
	//only current process will recieve SIGIO
	fcntl(0,F_SETOWN,getpid());
	flags=fcntl(0,F_GETFL);
	//fcntl(0,F_SETFL,flags|O_ASYNC);
}
void draw_board(int direction){
	signal( SIGALRM , SIG_IGN );
	if(direction==0){
	for(int i=0;i<bd.len;i++)
		mvaddch(bd.y_pos-i,bd.x_pos,bd.symbol);
	}
	else if(direction==1){
        if(bd.y_pos-bd.len+1==TOP_ROW)
            return;
		mvaddch(bd.y_pos,bd.x_pos,BLANK);
		bd.y_pos-=1;
        mvaddch(bd.y_pos-bd.len+1,bd.x_pos,bd.symbol);
	}
	else if(direction==2){
        if(bd.y_pos==BOT_ROW)
            return;
		mvaddch(bd.y_pos-bd.len+1,bd.x_pos,BLANK);
        bd.y_pos+=1;
        mvaddch(bd.y_pos,bd.x_pos,bd.symbol);
	}
	move(LINES-1,COLS-1);
    refresh();
	signal(SIGALRM,ball_move);

}
void async_input_handler(int signum){
		signal( SIGALRM , SIG_IGN );
		char c=getch();
		if(c=='q'||c==EOF){
			state=1;
			return;
		}
		if ( c == 'f' )	     the_ball.x_ttm--;
		else if ( c == 's' ) the_ball.x_ttm++;
		else if ( c == 'F' ) the_ball.y_ttm--;
		else if ( c == 'S' ) the_ball.y_ttm++;
		else if (c=='j') draw_board(1);
		else if(c=='k') draw_board(2);
		signal( SIGALRM , ball_move );
	
}
void ball_move(int signum)
{
	int	y_cur, x_cur, moved;
	int ret;

	signal( SIGALRM , SIG_IGN );		/* dont get caught now 	*/
	y_cur = the_ball.y_pos ;		/* old spot		*/
	x_cur = the_ball.x_pos ;
	moved = 0 ;
	//8-7-6-5-4-3-2-1
	if ( the_ball.y_ttm > 0 && the_ball.y_ttg-- == 1 ){
		the_ball.y_pos += the_ball.y_dir ;	/* move	*/
		the_ball.y_ttg = the_ball.y_ttm  ;	/* reset*/
		moved = 1;
	}
	//5-4-3-2-1
	if ( the_ball.x_ttm > 0 && the_ball.x_ttg-- == 1 ){
		the_ball.x_pos += the_ball.x_dir ;	/* move	*/
		the_ball.x_ttg = the_ball.x_ttm  ;	/* reset*/
		moved = 1;
	}

	if ( moved ){
		mvaddch( y_cur, x_cur, BLANK );
		mvaddch( y_cur, x_cur, BLANK );

		ret=bounce_or_lose( &the_ball,&bd);

	
		if(ret){
			mvaddch( the_ball.y_pos, the_ball.x_pos, the_ball.symbol );
			draw_board(0);
		}
		else{

			clear();
			move(LINES-1,COLS-strlen(END_GAME)-1);
			addstr(END_GAME);
			refresh();
			sleep(2);
			wrap_up();
			exit(0);
		}

		//put curse in at the bottom right
		move(LINES-1,COLS-1);
		refresh();
	}
	
	signal( SIGALRM, ball_move);		/* for unreliable systems */

}

int bounce_or_lose(struct ppball *bp,struct board* bdp)
{
	int	return_val = 1 ;
	int lower_bounce=bdp->y_pos-bdp->len+1;
	int upper_bounce=bdp->y_pos;

		if ( bp->y_pos == TOP_ROW ){
			bp->y_dir = 1 ; 
			return_val = 1 ;
		} 	else if ( bp->y_pos == BOT_ROW ){
					bp->y_dir = -1 ;
	       			return_val = 1;
		}	
		if ( bp->x_pos == LEFT_EDGE ){
			bp->x_dir = 1 ;
	       	return_val = 1 ;
			   
		} 	else if ( bp->x_pos == RIGHT_EDGE ){
			
				bp->x_dir=-1;
				return_val = 1 ;
				if((bp->y_pos<lower_bounce)||(bp->y_pos>upper_bounce)){
				  return_val = 0 ;
			}
		}
		

	return return_val;
}
int set_ticker( int n_msecs )
{
        struct itimerval new_timeset;
        long    n_sec, n_usecs;

        n_sec = n_msecs / 1000 ;		/* int part	*/
        n_usecs = ( n_msecs % 1000 ) * 1000L ;	/* remainder	*/

        new_timeset.it_interval.tv_sec  = n_sec;        /* set reload       */
        new_timeset.it_interval.tv_usec = n_usecs;      /* new ticker value */
        new_timeset.it_value.tv_sec     = n_sec  ;      /* store this       */
        new_timeset.it_value.tv_usec    = n_usecs ;     /* and this         */

	return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

