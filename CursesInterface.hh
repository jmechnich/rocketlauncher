#ifndef CURSESINTERFACE_HH
#define CURSESINTERFACE_HH

#include <curses.h>
#include <string>
#include <vector>
#include <utility>

#include <termios.h>

class Command;

struct Action
{
  Action( char key, const std::string& text) 
          : _key(key),_text(text),_cmd(0) {}
  Action( char key, const std::string& text, int lineOffset, int colOffset,
          bool fromCenter, char cmd)
          : _key(key),_text(text),_lineOffset(lineOffset),
            _colOffset(colOffset),_fromCenter(fromCenter),_cmd(cmd) {}
  char _key;
  std::string _text;
  int  _lineOffset;
  int  _colOffset;
  bool _fromCenter;
  char _cmd;
};

class CursesInterface
{
  typedef std::vector<std::pair<Action,Command*> > ActionVec;

public:
  CursesInterface()
          : _lines(0), _cols(0), _halfLines(0), _halfCols(0),
            _debug(false), _status(0), _init(false) {}
  ~CursesInterface() {resetActions();}
  void resetActions()
        {
          for( ActionVec::const_iterator it=_actions.begin();
               it!=_actions.end(); ++it)
              delete it->second;
          _actions.clear();
        }
  void setDebug( bool debug) {_debug=debug;}
  void resizeScreen()
        {
          if(_debug) return;
          getmaxyx(stdscr, _lines, _cols);
          _halfLines = _lines / 2;
          _halfCols = _cols / 2;
        }
  
  void redrawScreen()
        {
          std::string tmp = "WELCOME TO MISSILE COMMAND";
          if(_debug)
          {
            std::cout << tmp << std::endl;
            return;
          }
          clear();
          mvprintw(0,_halfCols - tmp.size()/2, tmp.c_str());
          mvprintw(_halfLines - 2, _halfCols, " ");
          mvprintw(_halfLines - 1, _halfCols, "|");
          mvprintw(_halfLines, _halfCols - 3, " -- -- ");
          mvprintw(_halfLines + 1, _halfCols, "|");
          mvprintw(_halfLines + 2, _halfCols, " ");
          mvprintw(_lines - 1, 0, "press '?' for help, 'q' to quit");
          refresh();
        }
  
  int open()
        {
          if(_debug) {
            termios old = {0};
            if(tcgetattr(0, &old) < 0) perror("tcsetattr()");
            old.c_lflag &= ~ICANON;
            old.c_lflag &= ~ECHO;
            old.c_cc[VMIN] = 1;
            old.c_cc[VTIME] = 0;
            if(tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");
            _term = old;
            redrawScreen();
            _init=true;
            return 0;
          }
          initscr();
          noecho();
          keypad(stdscr, TRUE);
          cbreak();
          nodelay(stdscr, TRUE);
          curs_set(0);
          resizeScreen();
          redrawScreen();
          _init=true;
          return 0;
        }
  int close()
        {
          if(!_init) return 0;
          if(_debug) {
            _term.c_lflag |= ICANON;
            _term.c_lflag |= ECHO;
            if (tcsetattr(0, TCSADRAIN, &_term) < 0)
                perror ("tcsetattr ~ICANON");
            return 0;
          }
          endwin();
          return 0;
        }
  void addAction( const Action& a, Command* c)
        {_actions.push_back(std::make_pair(a,c));}
  
  void print_status( const std::string& s="")
        {
          if(_debug) {
            if(s.size()) {std::cout << s << std::endl;}
            return;
          }
          mvprintw( _lines-2, 0, std::string(_cols,' ').c_str());
          if(s.size()) mvprintw( _lines-2, 0, s.c_str());
          refresh();
        }
  void setStatus(int status)
        {
          if(status<0) _status=0;
          else _status=status;
        }
  
  int resetControls(char cmd)
        {
          if(_debug) return 0;
          // redraw symbols of all non-hidden actions
          for( ActionVec::const_iterator it=_actions.begin();
               it!=_actions.end(); ++it) {
            const Action& a = it->first;
            if(!a._cmd) continue;
            int row=a._lineOffset, col=a._colOffset;
            if( a._fromCenter)
            {
              row+=_halfLines;
              col+=_halfCols;
            }
            
            move(row,col);
            if( (_status & a._cmd)) addch(a._key | A_STANDOUT);
            else if( cmd == a._cmd) addch(a._key | A_BOLD);
            else                    addch(a._key | A_NORMAL);
          }
          refresh();
          return 0;
        }

  void announce( const std::string& s="")
        {
          if(_debug) {
            if(s.size()) {std::cout << s << std::endl;}
            return;
          }
          if(s.size()) {
            attron(A_STANDOUT);
            mvprintw(4,_halfCols - s.size()/2,s.c_str());
            attroff(A_STANDOUT);
            refresh();
          }
          else {
            mvprintw(4,0,std::string(_cols,' ').c_str());
            refresh();
          }
        }
  
  // if != 0, then there is data to be read on stdin
  int kbhit()
        {
          // timeout structure passed into select
          struct timeval tv;
          // fd_set passed into select
          fd_set fds;
          // Set up the timeout.  here we can wait for 1 second
          tv.tv_sec = 0;
          tv.tv_usec = 500000;
          // Zero out the fd_set - make sure it's pristine
          FD_ZERO(&fds);
          // Set the FD that we want to read
          FD_SET(STDIN_FILENO, &fds);
          //STDIN_FILENO is 0
          // select takes the last file descriptor value + 1 in the fdset to check,
          // the fdset for reads, writes, and errors.  We are only passing in reads.
          // the last parameter is the timeout.  select will return if an FD is ready or 
          // the timeout has occurred
          select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
          
          // return 0 if STDIN is not ready to be read.
          return FD_ISSET(STDIN_FILENO, &fds);
        }
  
  int process()
        {
          int c=0;
          if(_debug) {
            if(kbhit()) read(0, &c, 1);
          }
          else c=getch();
          if( c==-1 || c==0) return 0;
          if( c==KEY_RESIZE)
          {
            resizeScreen();
            redrawScreen();
            return 0;
          }
          if(_debug){
            std::cout <<std::endl;
            std::cerr << "Got key '" << c << "'" << std::endl;
          }
          for( ActionVec::const_iterator it=_actions.begin();
               it!=_actions.end(); ++it) {
            const Action& a = it->first;
            if( a._key == c) {
              if(_debug) std::cerr << "Executing action '" << a._text
                                   << "' with key '" << a._key << "'"
                                   << std::endl;
              print_status(a._text);
              if( it->second) it->second->execute();
              if( a._cmd) resetControls( a._cmd);
              break;
            }
          }
          return 0;
        }
  
  std::string getString( const std::string& caption)
        {
          if( _debug)
          {
            std::string buf;
            std::cout << caption << " " << std::flush;
            std::cin >> buf;
            std::cout << std::endl;
            return buf;
          }
          char buf[11]={0};
          move(_lines-3,0);
          printw(caption.c_str());
          move(_lines-3,caption.size()+1);
          nodelay(stdscr,false);
          echo();
          curs_set(1);
          getnstr(buf,10);
          curs_set(0);
          noecho();
          nodelay(stdscr,true);
          move(_lines-3,0);
          printw(std::string(_cols,' ').c_str());
          return buf;
        }

  WINDOW* makeHelpWindow()
        {
          int width=_cols-4;
          if(width<0) width=_cols;
          int height=_actions.size()+6;
          if(height>_lines) height=_lines;
          int x=_halfLines-height/2; int y=_halfCols-width/2;
          int xoff=2;
          WINDOW* hw = newwin(height,width,x,y);
          keypad(hw, TRUE);
          idlok(hw,true);
          scrollok(hw,true);
          box(hw,0,0);
          int col=1;
          mvwprintw(hw,col,xoff,"Key  Action");
          std::ostringstream oss;
          oss << "Key  Action" << std::endl;
          col+= 2; ActionVec::const_iterator it=_actions.begin();
          for(;it!=_actions.end();++col,++it) {
            const Action& a = it->first;
            oss.str("");
            oss << " '" << a._key << "'" << "  " << a._text;
            mvwprintw(hw,col,xoff,oss.str().c_str());
          }
          col+=1;
          mvwprintw(hw,col,xoff,"Press any key to continue");
          wrefresh(hw);
          return hw;
        }
  
  void showHelp()
        {
          if(_debug)
          {
            std::cout << helpString() << std::endl;
            return;
          }
          WINDOW* hw = makeHelpWindow();
          int c;
          while((c=wgetch(hw))) {
            if(c==KEY_RESIZE) {
              delwin(hw);
              clear();
              resizeScreen();
              redrawScreen();
              hw = makeHelpWindow();
              continue;
            }
            if(c==KEY_LEFT||c==KEY_RIGHT||c==KEY_UP||c==KEY_DOWN) continue;
            if(_debug) std::cerr << "Got key '" << c << "'" << std::endl;
            break;
          }
          delwin(hw);
          clear();
          redrawScreen();
        }
  
  std::string helpString() const
        {
          std::ostringstream oss;
          oss << "Key  Action" << std::endl;
          for( ActionVec::const_iterator it=_actions.begin();
               it!=_actions.end(); ++it) {
            const Action& a = it->first;
            oss << " '" << a._key << "'" << "  " << a._text << std::endl;
          }
          return oss.str();
        }
  
private:
  int _lines;
  int _cols;
  int _halfLines;
  int _halfCols;
  ActionVec _actions;
  bool _debug;
  int _status;
  termios _term;
  bool _init;
};

#endif
