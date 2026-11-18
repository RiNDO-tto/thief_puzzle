#include "../src/Game.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <emscripten/emscripten.h>
namespace {
Board board;
State state;
State initial;
std::vector<State> history;
std::string snapshot;
std::string esc(const std::string& s) {
  std::string o;
  for (unsigned char c : s) {
    if(c=='"') o+="\\\"";
    else if(c=='\\') o+="\\\\";
    else if(c=='\n') o+="\\n";
    else if(c=='\r') o+="\\r";
    else if(c<32) o+=' ';
    else o+=c;
  }
  return o;
}
void update() {
  std::ostringstream out;
  out<<"{\"width\":"<<board.width<<",\"height\":"<<board.height<<",\"cells\":[";
  for(int y=0;y<board.height;y++) {
    if(y)out<<',';out<<'"';
    for(int x=0;x<board.width;x++)out<<board.cell[y][x];
    out<<'"';
  }
  out<<"],\"walls\":[";
  for(int y=0;y<board.height;y++)for(int x=0;x<board.width;x++){
    if(y||x)out<<',';
    int mask=0;for(int d=0;d<4;d++)if(board.wall[y][x][d])mask|=1<<d;
    out<<mask;
  }
  out<<"],\"thief\":["<<state.thief.x<<','<<state.thief.y<<','<<state.thief.direction
     <<"],\"police\":["<<state.police.x<<','<<state.police.y<<','<<state.police.direction
     <<"],\"controlThief\":"<<(state.controlThief?"true":"false")
     <<",\"result\":"<<static_cast<int>(state.result)
     <<",\"message\":\""<<esc(state.message)<<"\",\"boxes\":[";
  for(size_t i=0;i<state.boxes.size();i++){
    if(i)out<<',';out<<'['<<state.boxes[i].x<<','<<state.boxes[i].y<<']';
  }
  out<<"],\"boxMotions\":[";
  for(size_t i=0;i<state.boxMotions.size();i++){
    if(i)out<<',';
    auto m=state.boxMotions[i];
    out<<'['<<m.from.x<<','<<m.from.y<<','<<m.to.x<<','<<m.to.y<<']';
  }
  out<<"],\"crashes\":[";
  for(size_t i=0;i<state.crashes.size();i++){
    if(i)out<<',';out<<'['<<state.crashes[i].x<<','<<state.crashes[i].y<<']';
  }
  out<<"],\"pushes\":[";
  for(size_t i=0;i<state.pushes.size();i++){
    if(i)out<<',';out<<'['<<state.pushes[i].x<<','<<state.pushes[i].y<<']';
  }
  out<<"]}";
  snapshot=out.str();
}
}
extern "C" {
EMSCRIPTEN_KEEPALIVE int game_load(const char* name) {
  if (!name) return 0;
  std::string filename(name);
  // Accept only canonical stage names, never arbitrary filesystem paths.
  if (filename.size() < 12 || filename.rfind("stage", 0) != 0 ||
      filename.substr(filename.size() - 4) != ".txt") return 0;
  for (size_t i = 5; i + 4 < filename.size(); ++i) {
    const char c = filename[i];
    if ((c < '0' || c > '9') && c != '-') return 0;
  }
  Board nextBoard;
  State nextState;
  if (!loadStage("/stages/" + filename, nextBoard, nextState)) return 0;
  board = std::move(nextBoard);
  state = std::move(nextState);
  initial = state;
  history.clear();
  update();
  return 1;
}
EMSCRIPTEN_KEEPALIVE int game_init() {
  return game_load("stage1-1.txt");
}
EMSCRIPTEN_KEEPALIVE const char* game_state() {return snapshot.c_str();}
EMSCRIPTEN_KEEPALIVE int game_move(int d) {
  if(d<0||d>3)return 0;
  State prev=state;
  if(!takeTurn(board,state,d)){update();return 0;}
  history.push_back(std::move(prev));update();return 1;
}
EMSCRIPTEN_KEEPALIVE void game_undo() {
  if(!history.empty()){state=history.back();history.pop_back();}
  state.boxMotions.clear();state.crashes.clear();state.pushes.clear();update();
}
EMSCRIPTEN_KEEPALIVE void game_reset() {
  state=initial;history.clear();update();
}
}
