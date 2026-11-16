#include "Game.hpp"
#include <iostream>
#include <string>
#include <cassert>
Board board(int n){Board b;b.width=n;b.height=1;b.cell={{}};b.cell[0].assign(n,'.');b.wall.assign(1,std::vector<std::array<bool,4>>(n,{false,false,false,false}));return b;}
void test(std::string name,std::string layout,bool thiefControl,int dir,std::string expected,bool valid=true){
 Board b=board(layout.size()); State s; s.controlThief=thiefControl;
 for(int x=0;x<(int)layout.size();++x){char c=layout[x];if(c=='T')s.thief={x,0,RIGHT};if(c=='P')s.police={x,0,LEFT};if(c=='B')s.boxes.push_back({x,0});if(c=='X')b.cell[0][x]='X';}
 if(thiefControl){s.thief.direction=RIGHT;s.police.direction=RIGHT;}else{s.police.direction=LEFT;s.thief.direction=RIGHT;}
 bool ok=takeTurn(b,s,dir);std::string out(layout.size(),'.');for(int x=0;x<(int)layout.size();++x)if(b.cell[0][x]=='X')out[x]='X';for(auto p:s.boxes)out[p.x]='B';out[s.thief.x]='T';out[s.police.x]='P';
 std::cout<<name<<" "<<ok<<" "<<out<<" expected "<<expected<<std::endl; assert(ok==valid);assert(out==expected);
}
int main(){test("same", "TBP..",true,RIGHT,".TBP.");test("two_gap", "TB..BP",false,LEFT,".TBBP.");test("one_gap", "TB.BP",false,LEFT,".TBP.");test("no_gap", "TBBP",false,LEFT,"TBBP",false);test("middle", "TB.B.BP",false,LEFT,".TBBBP.");test("charcrush", "TB.P",false,LEFT,".TP.");test("spike", "TBX.P",true,RIGHT,".TX.P");}
