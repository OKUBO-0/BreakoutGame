// Original, deterministic PCM assets; no external music or sample licenses.
import fs from 'node:fs/promises';
import path from 'node:path';
const rate=44100;
const output=path.resolve('SourceArt/Audio');
await fs.mkdir(output,{recursive:true});
function wav(samples){
 const b=Buffer.alloc(44+samples.length*2);
 b.write('RIFF');b.writeUInt32LE(b.length-8,4);b.write('WAVEfmt ',8);b.writeUInt32LE(16,16);
 b.writeUInt16LE(1,20);b.writeUInt16LE(1,22);b.writeUInt32LE(rate,24);b.writeUInt32LE(rate*2,28);
 b.writeUInt16LE(2,32);b.writeUInt16LE(16,34);b.write('data',36);b.writeUInt32LE(samples.length*2,40);
 for(let i=0;i<samples.length;i++)b.writeInt16LE(Math.round(Math.max(-1,Math.min(1,samples[i]))*32767),44+i*2);
 return b;
}
const note=n=>440*2**((n-69)/12);
// Eight bars at 120 BPM. Gentle plucks, rounded bass and quiet percussion.
const bgm=new Float64Array(rate*16);
function tone(start,duration,midi,gain,kind='pluck'){
 const first=Math.round(start*rate),count=Math.round(duration*rate),hz=note(midi);
 for(let j=0;j<count&&first+j<bgm.length;j++){
  const t=j/rate,u=t/duration,phase=2*Math.PI*hz*t;
  const envelope=Math.min(t/0.006,1)*Math.exp(-t*(kind==='bass'?5:10))*Math.min((1-u)*20,1);
  const value=kind==='bass'?Math.sin(phase)+0.15*Math.sin(phase*2):Math.sin(phase)+0.22*Math.sin(phase*2)+0.08*Math.sin(phase*3);
  bgm[first+j]+=gain*envelope*value;
 }
}
const chords=[[60,64,67],[57,60,64],[53,57,60],[55,59,62]];
const melody=[72,76,79,76,74,72,67,71,72,76,81,79,77,76,74,71];
for(let bar=0;bar<8;bar++){
 const chord=chords[bar%4];
 for(let step=0;step<8;step++)tone(bar*2+step*0.25,0.23,chord[[0,1,2,1,0,1,2,1][step]]+12,0.075);
 for(let beat=0;beat<4;beat++){
  tone(bar*2+beat*0.5,0.43,chord[0]-24,0.15,'bass');
  const first=Math.round((bar*2+beat*0.5)*rate);
  for(let j=0;j<rate*0.1&&first+j<bgm.length;j++){
   const t=j/rate;bgm[first+j]+=0.09*Math.sin(2*Math.PI*(65*t+1.5*(1-Math.exp(-t*35))))*Math.exp(-t*40);
  }
 }
 tone(bar*2+0.5,0.45,melody[bar*2],0.12);
 tone(bar*2+1.5,0.45,melody[bar*2+1],0.12);
}
// End each tail before the loop seam; keep peaks comfortably below clipping.
const peak=Math.max(...Array.from(bgm.subarray(0,4096)).map(Math.abs),0.001);
let maximum=peak;for(const sample of bgm)maximum=Math.max(maximum,Math.abs(sample));
for(let i=0;i<bgm.length;i++)bgm[i]*=0.65/maximum;
const knock=new Float64Array(Math.round(rate*0.13));
let seed=0x10404;
for(let i=0;i<knock.length;i++){
 const t=i/rate;seed=(Math.imul(seed,1664525)+1013904223)>>>0;
 const noise=seed/0xffffffff*2-1;
 knock[i]=Math.min(t/0.001,1)*(0.52*Math.sin(2*Math.PI*880*t)+0.18*Math.sin(2*Math.PI*1320*t)+0.07*noise)*Math.exp(-t*55);
}
for(const [name,samples] of [['Breakout_BGM.wav',bgm],['Breakout_SE_Knock.wav',knock]]){
 const target=path.join(output,name);await fs.writeFile(target,wav(samples));console.log(target);
}
