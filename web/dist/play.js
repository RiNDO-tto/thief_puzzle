(() => {
'use strict';
const canvas=document.getElementById('board'),ctx=canvas.getContext('2d');
const status=document.getElementById('status');
let wasm=null, state=null, anim=null;
let stageNames=[], currentStage='stage1-1.txt';
const worldSelect=document.getElementById('world'), stageSelect=document.getElementById('stage');
const stageNumber=n=>Number(n.match(/^stage(\d+)-/)[1]);
const stageLabel=n=>n.replace(/^stage(\d+)-(\d+)\.txt$/, 'ステージ $2');
function updateStageOptions(){
 const world=Number(worldSelect.value);
 stageSelect.replaceChildren();
 stageNames.filter(n=>stageNumber(n)===world).forEach(n=>{
   const option=document.createElement('option');option.value=n;option.textContent=stageLabel(n);stageSelect.append(option);
 });
 if(stageNames.includes(currentStage)&&stageNumber(currentStage)===world)stageSelect.value=currentStage;
}
function selectStage(name){
 if(!wasm||!stageNames.includes(name))return false;
 const ok=wasm.ccall('game_load','number',['string'],[name]);
 if(!ok){status.textContent='ステージの読み込みに失敗しました: '+name;return false;}
 currentStage=name;worldSelect.value=String(stageNumber(name));updateStageOptions();stageSelect.value=name;sync();return true;
}
function nextStage(){const i=stageNames.indexOf(currentStage);if(i>=0&&i+1<stageNames.length)selectStage(stageNames[i+1]);}
async function loadCatalog(){
 const response=await fetch('stages.json');if(!response.ok)throw Error('ステージ一覧を取得できません');
 stageNames=await response.json();
 if(!Array.isArray(stageNames)||!stageNames.length)throw Error('ステージがありません');
 worldSelect.replaceChildren();
 [...new Set(stageNames.map(stageNumber))].forEach(w=>{
  const option=document.createElement('option');option.value=String(w);option.textContent='ワールド '+w;worldSelect.append(option);
 });
 worldSelect.value=String(stageNumber(stageNames[0]));updateStageOptions();
 if(!selectStage(stageNames[0]))throw Error('最初のステージを開けません');
}

const colors={'.':'#f1e7d4','A':'#98c7a0','S':'#ebcb74','X':'#d9a2a2',' ':'#1c283a'};
const duration=160;
const getState=()=>JSON.parse(wasm.UTF8ToString(wasm._game_state()));
function sync(moved=false){
 const previous=state;state=getState();
 anim=moved&&previous?{start:performance.now(),previous}:null;
 status.textContent=currentStage.replace('.txt','')+'　'+(state.controlThief?'操作：泥棒':'操作：警察')+'　'+(state.result===1?'クリア！ ':state.result===2?'ゲームオーバー ':'')+(state.message||'');
 draw();
}
function tileLabel(ch,x,y,size){
 if(ch==='A'||ch==='S'||ch==='X'){
 ctx.fillStyle=ch==='A'?'#285b34':ch==='S'?'#6e5013':'#852c2c';
 ctx.font=`bold ${size*.33}px sans-serif`;ctx.textAlign='center';ctx.textBaseline='middle';ctx.fillText(ch,x+size/2,y+size/2);
 }
}
function actor(x,y,dir,color,label,size,active){
 const cx=x+size/2,cy=y+size/2,r=size*.30;
 ctx.beginPath();ctx.arc(cx,cy,r,0,Math.PI*2);ctx.fillStyle=color;ctx.fill();ctx.lineWidth=active?5:2;ctx.strokeStyle=active?'#fffbca':'#243247';ctx.stroke();
 ctx.fillStyle='#fff';ctx.font=`bold ${size*.28}px sans-serif`;ctx.textAlign='center';ctx.textBaseline='middle';ctx.fillText(label,cx,cy);
 const a=[-Math.PI/2,0,Math.PI/2,Math.PI][dir];
 ctx.beginPath();ctx.moveTo(cx+Math.cos(a)*r*1.05,cy+Math.sin(a)*r*1.05);
 ctx.lineTo(cx+Math.cos(a+.55)*r*.56,cy+Math.sin(a+.55)*r*.56);
 ctx.lineTo(cx+Math.cos(a-.55)*r*.56,cy+Math.sin(a-.55)*r*.56);
 ctx.closePath();ctx.fillStyle='#202937';ctx.fill();
}
function draw(){
 if(!state)return;
 const {width:w,height:h}=state;
 const cell=Math.min(100,Math.floor(570/Math.max(w,h))), ox=(620-w*cell)/2,oy=(620-h*cell)/2;
 ctx.clearRect(0,0,620,620);ctx.fillStyle='#1c283a';ctx.fillRect(0,0,620,620);
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){
 const c=state.cells[y][x],px=ox+x*cell,py=oy+y*cell;
 ctx.fillStyle=colors[c]||colors['.'];ctx.fillRect(px,py,cell,cell);
 ctx.strokeStyle='#d3c8b4';ctx.lineWidth=1;ctx.strokeRect(px,py,cell,cell);tileLabel(c,px,py,cell);
 const mask=state.walls[y*w+x];ctx.strokeStyle='#594737';ctx.lineWidth=Math.max(4,cell*.07);ctx.lineCap='square';ctx.beginPath();
 if(mask&1){ctx.moveTo(px,py);ctx.lineTo(px+cell,py)}
 if(mask&2){ctx.moveTo(px+cell,py);ctx.lineTo(px+cell,py+cell)}
 if(mask&4){ctx.moveTo(px,py+cell);ctx.lineTo(px+cell,py+cell)}
 if(mask&8){ctx.moveTo(px,py);ctx.lineTo(px,py+cell)}ctx.stroke();
 }
 const elapsed=anim?performance.now()-anim.start:duration;
 const t=Math.min(1,elapsed/duration);
 const motions=t<1?state.boxMotions:[];
 const movingDest=new Set(motions.map(m=>`${m[2]},${m[3]}`));
 function crate(x,y){const px=ox+x*cell,py=oy+y*cell,pad=cell*.14;ctx.fillStyle='#b77b43';ctx.fillRect(px+pad,py+pad,cell-2*pad,cell-2*pad);ctx.strokeStyle='#704521';ctx.lineWidth=3;ctx.strokeRect(px+pad,py+pad,cell-2*pad,cell-2*pad);ctx.beginPath();ctx.moveTo(px+pad,py+pad);ctx.lineTo(px+cell-pad,py+cell-pad);ctx.moveTo(px+cell-pad,py+pad);ctx.lineTo(px+pad,py+cell-pad);ctx.stroke()}
 state.boxes.forEach(([x,y])=>{if(!movingDest.has(`${x},${y}`))crate(x,y)});
 motions.forEach(([fx,fy,tx,ty])=>crate(fx+(tx-fx)*t,fy+(ty-fy)*t));
 const [tx,ty,td]=state.thief,[px,py,pd]=state.police;
 actor(ox+tx*cell,oy+ty*cell,td,'#d49b31','T',cell,state.controlThief);
 actor(ox+px*cell,oy+py*cell,pd,'#477cb3','P',cell,!state.controlThief);
 if(t<1)requestAnimationFrame(draw);else anim=null;
}
function move(dir){if(!wasm)return;const ok=wasm._game_move(dir);sync(!!ok)}
function undo(){if(!wasm)return;wasm._game_undo();sync()}
function reset(){if(!wasm)return;wasm._game_reset();sync()}
const keys={w:0,ArrowUp:0,d:1,ArrowRight:1,s:2,ArrowDown:2,a:3,ArrowLeft:3};
window.addEventListener('keydown',e=>{if(Object.prototype.hasOwnProperty.call(keys,e.key)){e.preventDefault();if(!e.repeat)move(keys[e.key])}else if(e.key.toLowerCase()==='z')undo();else if(e.key.toLowerCase()==='r')reset()});
document.querySelectorAll('[data-dir]').forEach(b=>b.addEventListener('click',()=>move(Number(b.dataset.dir))));
document.getElementById('next').addEventListener('click',nextStage);worldSelect.addEventListener('change',updateStageOptions);document.getElementById('load').addEventListener('click',()=>selectStage(stageSelect.value));document.getElementById('undo').addEventListener('click',undo);document.getElementById('reset').addEventListener('click',reset);
window.Module={onRuntimeInitialized(){wasm=window.Module;loadCatalog().catch(e=>{status.textContent=e.message})},printErr(...args){status.textContent=args.join(' ')}};
})();
