(() => {
'use strict';
const canvas=document.getElementById('board'),ctx=canvas.getContext('2d');
const status=document.getElementById('status');
const worldSelect=document.getElementById('world'),stageSelect=document.getElementById('stage');
const imageFiles={T:'thief.jpg',P:'police.jpg',A:'hideout.png',S:'switch.png',X:'spikes.png',B:'box.png',crash:'crash.png',click:'click.png'};
const images={};for(const [key,file] of Object.entries(imageFiles)){const img=new Image();img.src='assets/'+file;img.onload=()=>draw();images[key]=img;}
let wasm=null,state=null,anim=null,effects=[],stageNames=[],currentStage='stage1-1.txt',framePending=false;
const menuOverlay=document.getElementById('menuOverlay'),menuGrid=document.getElementById('menuGrid');
const menuClose=document.getElementById('menuClose'),menuBack=document.getElementById('menuBack');
let menuWorld=null,completed=new Set();
try{completed=new Set(JSON.parse(localStorage.getItem('thief-puzzle-web-cleared-v1')||'[]'));}catch(_){completed=new Set();}
function saveProgress(){try{localStorage.setItem('thief-puzzle-web-cleared-v1',JSON.stringify([...completed]));}catch(_){}}
function showMenu(world=null){menuWorld=world;menuOverlay.hidden=false;renderMenu();}
function closeMenu(){menuOverlay.hidden=true;}
function renderMenu(){
 const worlds=[...new Set(stageNames.map(stageNumber))];
 const done=stageNames.filter(n=>completed.has(n)).length;
 document.getElementById('progressSummary').textContent=`クリア状況 ${done} / ${stageNames.length}`;
 menuBack.hidden=menuWorld===null;menuClose.hidden=!wasm||!state;
 document.getElementById('menuCrumb').textContent=menuWorld===null?'ワールドを選択':'ワールド '+menuWorld;
 menuGrid.replaceChildren();
 if(menuWorld===null){for(const w of worlds){const names=stageNames.filter(n=>stageNumber(n)===w),count=names.filter(n=>completed.has(n)).length;const b=document.createElement('button');b.className='menu-card'+(count===names.length?' done':'');const title=document.createElement('strong');title.textContent='WORLD '+w;const subtitle=document.createElement('small');subtitle.textContent=`${count} / ${names.length} クリア`;b.append(title,subtitle);b.addEventListener('click',()=>showMenu(w));menuGrid.append(b);}}
 else{for(const name of stageNames.filter(n=>stageNumber(n)===menuWorld)){const b=document.createElement('button');b.className='menu-card'+(completed.has(name)?' done':'');const title=document.createElement('strong');title.textContent=stageLabel(name);const subtitle=document.createElement('small');subtitle.textContent=completed.has(name)?'✓ クリア済み':name===currentStage?'▶ プレイ中':'未クリア';b.append(title,subtitle);b.addEventListener('click',()=>{if(selectStage(name))closeMenu();});menuGrid.append(b);}}
}
document.getElementById('openMenu').addEventListener('click',()=>showMenu(null));
menuClose.addEventListener('click',closeMenu);
menuBack.addEventListener('click',()=>showMenu(null));

const duration=160,effectDuration=500;
const stageNumber=n=>Number(n.match(/^stage(\d+)-/)[1]);
const stageLabel=n=>n.replace(/^stage(\d+)-(\d+)\.txt$/,'ステージ $2');
const loaded=k=>images[k]?.complete&&images[k].naturalWidth>0;
const getState=()=>JSON.parse(wasm.UTF8ToString(wasm._game_state()));
function updateStageOptions(){const world=Number(worldSelect.value);stageSelect.replaceChildren();stageNames.filter(n=>stageNumber(n)===world).forEach(n=>{const option=document.createElement('option');option.value=n;option.textContent=stageLabel(n);stageSelect.append(option);});if(stageNames.includes(currentStage)&&stageNumber(currentStage)===world)stageSelect.value=currentStage;}
function selectStage(name){if(!wasm||!stageNames.includes(name))return false;const ok=wasm.ccall('game_load','number',['string'],[name]);if(!ok){status.textContent='ステージの読み込みに失敗しました: '+name;return false;}currentStage=name;worldSelect.value=String(stageNumber(name));updateStageOptions();stageSelect.value=name;effects=[];sync();document.getElementById('currentLabel').textContent=`WORLD ${stageNumber(name)} / ${stageLabel(name)}`;return true;}
function nextStage(){const i=stageNames.indexOf(currentStage);if(i>=0&&i+1<stageNames.length)selectStage(stageNames[i+1]);}
async function loadCatalog(){const response=await fetch('stages.json');if(!response.ok)throw Error('ステージ一覧を取得できません');stageNames=await response.json();if(!Array.isArray(stageNames)||!stageNames.length)throw Error('ステージがありません');worldSelect.replaceChildren();[...new Set(stageNames.map(stageNumber))].forEach(w=>{const option=document.createElement('option');option.value=String(w);option.textContent='ワールド '+w;worldSelect.append(option);});worldSelect.value=String(stageNumber(stageNames[0]));updateStageOptions();if(!selectStage(stageNames[0]))throw Error('最初のステージを開けません');showMenu(null);}
function sync(moved=false){const previous=state;state=getState();const now=performance.now();anim=moved&&previous?{start:now}:null;if(moved){const crashes=state.crashes||[],pushes=state.pushes||[];for(const [x,y] of crashes)effects.push({type:'crash',x,y,start:now});for(const [x,y] of pushes){const overlap=crashes.some(([cx,cy])=>cx===x&&cy===y);effects.push({type:'click',x,y,start:now+(overlap?300:0)});}}status.textContent=currentStage.replace('.txt','')+'　'+(state.controlThief?'操作：泥棒':'操作：警察')+'　'+(state.result===1?'クリア！ ':state.result===2?'ゲームオーバー ':'')+(state.message||'');scheduleDraw();}
function scheduleDraw(){if(framePending)return;framePending=true;requestAnimationFrame(()=>{framePending=false;draw();});}
function img(k,x,y,w,h=w){if(!loaded(k))return false;ctx.drawImage(images[k],x,y,w,h);return true;}
function tileLabel(ch,x,y,size){ctx.fillStyle=ch==='A'?'#285b34':ch==='S'?'#6e5013':'#852c2c';ctx.font=`bold ${size*.33}px sans-serif`;ctx.textAlign='center';ctx.textBaseline='middle';ctx.fillText(ch,x+size/2,y+size/2);}
function actor(x,y,dir,key,size,active){const cx=x+size/2,cy=y+size/2,sz=size*.76;ctx.save();if(active){ctx.strokeStyle='#ffd341';ctx.lineWidth=Math.max(3,size*.045);ctx.strokeRect(cx-sz/2-3,cy-sz/2-3,sz+6,sz+6);}ctx.translate(cx,cy);ctx.rotate(dir*Math.PI/2);if(!img(key,-sz/2,-sz/2,sz)){ctx.fillStyle=key==='T'?'#439af4':'#f05f63';ctx.beginPath();ctx.arc(0,0,sz*.42,0,Math.PI*2);ctx.fill();ctx.fillStyle='white';ctx.font=`bold ${sz*.45}px sans-serif`;ctx.textAlign='center';ctx.textBaseline='middle';ctx.fillText(key,0,0);}ctx.restore();}
function draw(){if(!state)return;const {width:w,height:h}=state;const cell=Math.min(100,Math.floor(560/Math.max(w,h))),ox=(620-w*cell)/2,oy=(620-h*cell)/2;ctx.clearRect(0,0,620,620);ctx.fillStyle='#282d39';ctx.fillRect(0,0,620,620);
for(let y=0;y<h;y++)for(let x=0;x<w;x++){const c=state.cells[y][x],px=ox+x*cell,py=oy+y*cell;if(c===' '){ctx.fillStyle='#202632';ctx.fillRect(px,py,cell,cell);continue;}ctx.fillStyle='#383d4b';ctx.fillRect(px,py,cell,cell);if('ASX'.includes(c)){if(!img(c,px,py,cell))tileLabel(c,px,py,cell);}}
const now=performance.now(),elapsed=anim?now-anim.start:duration,t=Math.min(1,elapsed/duration),smooth=t*t*(3-2*t),motions=t<1?(state.boxMotions||[]):[],movingDest=new Set(motions.map(m=>`${m[2]},${m[3]}`));
function crate(x,y){const px=ox+x*cell,py=oy+y*cell;if(!img('B',px,py,cell)){ctx.fillStyle='#b77b43';ctx.fillRect(px+cell*.1,py+cell*.1,cell*.8,cell*.8);}}
state.boxes.forEach(([x,y])=>{if(!movingDest.has(`${x},${y}`))crate(x,y)});motions.forEach(([fx,fy,tx,ty])=>crate(fx+(tx-fx)*smooth,fy+(ty-fy)*smooth));
// Match the SFML renderer's light walls, drawn over floor tiles and boxes.
ctx.strokeStyle='#d7dce6';ctx.lineWidth=Math.max(5,cell*.08);ctx.lineCap='square';for(let y=0;y<h;y++)for(let x=0;x<w;x++){if(state.cells[y][x]===' ')continue;const px=ox+x*cell,py=oy+y*cell,mask=state.walls[y*w+x];ctx.beginPath();if(mask&1){ctx.moveTo(px,py);ctx.lineTo(px+cell,py);}if(mask&8){ctx.moveTo(px,py);ctx.lineTo(px,py+cell);}if((mask&2)&&(x===w-1||state.cells[y][x+1]===' ')){ctx.moveTo(px+cell,py);ctx.lineTo(px+cell,py+cell);}if((mask&4)&&(y===h-1||state.cells[y+1][x]===' ')){ctx.moveTo(px,py+cell);ctx.lineTo(px+cell,py+cell);}ctx.stroke();}
const [tx,ty,td]=state.thief,[px,py,pd]=state.police;actor(ox+tx*cell,oy+ty*cell,td,'T',cell,state.controlThief);actor(ox+px*cell,oy+py*cell,pd,'P',cell,!state.controlThief);
effects=effects.filter(e=>now<e.start+effectDuration);for(const e of effects){if(now<e.start)continue;const age=(now-e.start)/effectDuration;ctx.save();ctx.globalAlpha=Math.min(1,(1-age)*1.8);const size=cell*1.05;img(e.type,ox+(e.x+.5)*cell-size/2,oy+(e.y+.5)*cell-size/2,size);ctx.restore();}
if(state.result){ctx.fillStyle='rgba(15,19,30,.63)';ctx.fillRect(0,260,620,100);ctx.textAlign='center';ctx.textBaseline='middle';ctx.font='bold 56px system-ui,sans-serif';ctx.fillStyle=state.result===1?'#66d292':'#ff7784';ctx.fillText(state.result===1?'WIN':'LOSE',310,310);}
if(t<1||effects.length)scheduleDraw();else anim=null;
}
function move(dir){if(!wasm||!menuOverlay.hidden)return;const ok=wasm._game_move(dir);sync(!!ok);if(ok&&state.result===1&&!completed.has(currentStage)){completed.add(currentStage);saveProgress();renderMenu();}}
function undo(){if(!wasm||!menuOverlay.hidden)return;wasm._game_undo();effects=[];sync();}
function reset(){if(!wasm||!menuOverlay.hidden)return;wasm._game_reset();effects=[];sync();}
const keys={w:0,ArrowUp:0,d:1,ArrowRight:1,s:2,ArrowDown:2,a:3,ArrowLeft:3};window.addEventListener('keydown',e=>{if(e.key==='Escape'&&!menuOverlay.hidden&&state){closeMenu();return;}if(!menuOverlay.hidden)return;if(e.target instanceof HTMLSelectElement||e.target instanceof HTMLInputElement)return;if(Object.prototype.hasOwnProperty.call(keys,e.key)){e.preventDefault();if(!e.repeat)move(keys[e.key]);}else if(e.key.toLowerCase()==='z')undo();else if(e.key.toLowerCase()==='r')reset();});
document.querySelectorAll('[data-dir]').forEach(b=>b.addEventListener('click',()=>move(Number(b.dataset.dir))));document.getElementById('next').addEventListener('click',nextStage);worldSelect.addEventListener('change',updateStageOptions);document.getElementById('load').addEventListener('click',()=>selectStage(stageSelect.value));document.getElementById('undo').addEventListener('click',undo);document.getElementById('reset').addEventListener('click',reset);
window.Module={onRuntimeInitialized(){wasm=window.Module;loadCatalog().catch(e=>{status.textContent=e.message;});},printErr(...args){status.textContent=args.join(' ');}};
})();
